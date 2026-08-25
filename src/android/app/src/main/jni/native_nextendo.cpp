// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>
#include <utility>

#include <jni.h>

#include "citron/nextendo_compatible_titles.h"
#include "common/android/android_common.h"
#include "common/android/id_cache.h"
#include "common/logging.h"
#include "common/nextendo_account.h"
#include "common/nextendo_friends.h"
#include "common/nextendo_outgoing_requests.h"
#include "core/nextendo/save_sync.h"
#include "native.h"

#ifdef ENABLE_WEB_SERVICE
#include "web_service/nextendo_api.h"
#endif

extern "C" {

// Called once at app startup (see CitronApplication.onCreate) with the on-disk path the CA
// bundle asset was extracted to. Must happen before any other Nextendo network call.
void Java_org_citron_citron_1emu_utils_NativeNextendo_setCaCertPath(JNIEnv* env, jclass clazz,
                                                                 jstring jpath) {
#ifdef ENABLE_WEB_SERVICE
    WebService::NextendoApi::SetCaCertPathOverride(Common::Android::GetJString(env, jpath));
#endif
}

jboolean Java_org_citron_citron_1emu_utils_NativeNextendo_isLinked(JNIEnv* env, jclass clazz) {
    return static_cast<jboolean>(Common::NextendoAccount::IsLinked());
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_getUsername(JNIEnv* env, jclass clazz) {
    return Common::Android::ToJString(env, Common::NextendoAccount::GetUsername());
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_getFriendCode(JNIEnv* env, jclass clazz) {
    return Common::Android::ToJString(env, Common::NextendoAccount::GetFriendCode());
}

// Blocking -- runs the OAuth loopback flow and does not return until it completes or times out
// (up to 5 minutes). Must be called from a background thread only; calling this from the JVM
// main thread freezes the whole app for the duration. Returns an empty string on success, or a
// human-readable error message on failure.
jstring Java_org_citron_citron_1emu_utils_NativeNextendo_signIn(JNIEnv* env, jclass clazz) {
#ifdef ENABLE_WEB_SERVICE
    const auto open_url = [env](const std::string& url) {
        jstring jurl = Common::Android::ToJString(env, url);
        env->CallStaticVoidMethod(Common::Android::GetNativeNextendoClass(),
                                  Common::Android::GetOnOpenSignInUrlMethod(), jurl);
        env->DeleteLocalRef(jurl);
    };

    const auto result = WebService::NextendoApi::SignInWithBrowser(open_url);
    if (!result.ok) {
        return Common::Android::ToJString(env, result.error);
    }

    Common::NextendoAccount::Save(result.pid, result.username, result.friend_code, result.token);
    Common::NextendoFriends::SetLocalStatus(Common::NextendoFriends::PresenceOnline);
    LOG_INFO(Frontend, "NativeNextendo::signIn: linked as {}", result.username);
    return Common::Android::ToJString(env, "");
#else
    return Common::Android::ToJString(env, "This build has no web services support.");
#endif
}

void Java_org_citron_citron_1emu_utils_NativeNextendo_signOut(JNIEnv* env, jclass clazz) {
    Common::NextendoAccount::Clear();
    Common::NextendoFriends::Set({});
}

namespace {

#ifdef ENABLE_WEB_SERVICE
jobject MakeNextendoFriend(JNIEnv* env, const WebService::NextendoApi::Friend& f,
                           jboolean is_request) {
    jstring jname = Common::Android::ToJString(env, f.name);
    jstring jcode = Common::Android::ToJString(env, f.friend_code);
    jstring japp_name = Common::Android::ToJString(env, f.app_name);
    jobject entry =
        env->NewObject(Common::Android::GetNextendoFriendClass(),
                       Common::Android::GetNextendoFriendConstructor(),
                       static_cast<jlong>(f.pid), jname, jcode,
                       static_cast<jint>(f.presence_status), japp_name, is_request);
    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(jcode);
    env->DeleteLocalRef(japp_name);
    return entry;
}
#endif

} // namespace

// Combines confirmed friends and incoming (awaiting accept/decline) requests into one array --
// they come back from one GetFriends() call together, no need for the caller to make two.
// Each element's isIncomingRequest field on the Kotlin side tells them apart.
jobjectArray Java_org_citron_citron_1emu_utils_NativeNextendo_getFriends(JNIEnv* env, jclass clazz) {
#ifdef ENABLE_WEB_SERVICE
    const auto result = WebService::NextendoApi::GetFriends();
    if (!result.ok) {
        LOG_WARNING(Frontend, "NativeNextendo::getFriends: {}", result.error);
    }
    const auto total = result.friends.size() + result.requests.size();
    jobjectArray out =
        env->NewObjectArray(static_cast<jsize>(total), Common::Android::GetNextendoFriendClass(), nullptr);
    jsize index = 0;
    for (const auto& f : result.friends) {
        jobject entry = MakeNextendoFriend(env, f, JNI_FALSE);
        env->SetObjectArrayElement(out, index++, entry);
        env->DeleteLocalRef(entry);
    }
    for (const auto& f : result.requests) {
        jobject entry = MakeNextendoFriend(env, f, JNI_TRUE);
        env->SetObjectArrayElement(out, index++, entry);
        env->DeleteLocalRef(entry);
    }
    return out;
#else
    return env->NewObjectArray(0, Common::Android::GetNextendoFriendClass(), nullptr);
#endif
}

// Friend codes you've sent a request to but that haven't shown up as a confirmed friend yet --
// tracked locally since the server never reports your own outgoing requests back to you.
jobjectArray Java_org_citron_citron_1emu_utils_NativeNextendo_getOutgoingRequestCodes(JNIEnv* env,
                                                                                   jclass clazz) {
    const auto entries = Common::NextendoOutgoingRequests::Get();
    jobjectArray out = env->NewObjectArray(static_cast<jsize>(entries.size()),
                                           Common::Android::GetStringClass(), nullptr);
    for (size_t i = 0; i < entries.size(); ++i) {
        jstring jcode = Common::Android::ToJString(env, entries[i].friend_code);
        env->SetObjectArrayElement(out, static_cast<jsize>(i), jcode);
        env->DeleteLocalRef(jcode);
    }
    return out;
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_addFriendByCode(JNIEnv* env, jclass clazz,
                                                                     jstring jfriend_code) {
#ifdef ENABLE_WEB_SERVICE
    const auto code = Common::Android::GetJString(env, jfriend_code);
    const auto error = WebService::NextendoApi::AddFriendByCode(code);
    if (error.empty()) {
        Common::NextendoOutgoingRequests::Add(code);
    }
    return Common::Android::ToJString(env, error);
#else
    return Common::Android::ToJString(env, "This build has no web services support.");
#endif
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_acceptFriend(JNIEnv* env, jclass clazz,
                                                                  jlong jpid) {
#ifdef ENABLE_WEB_SERVICE
    return Common::Android::ToJString(env,
                                      WebService::NextendoApi::AcceptFriend(static_cast<u64>(jpid)));
#else
    return Common::Android::ToJString(env, "This build has no web services support.");
#endif
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_declineFriend(JNIEnv* env, jclass clazz,
                                                                   jlong jpid) {
#ifdef ENABLE_WEB_SERVICE
    return Common::Android::ToJString(
        env, WebService::NextendoApi::DeclineFriend(static_cast<u64>(jpid)));
#else
    return Common::Android::ToJString(env, "This build has no web services support.");
#endif
}

jstring Java_org_citron_citron_1emu_utils_NativeNextendo_removeFriend(JNIEnv* env, jclass clazz,
                                                                  jlong jpid) {
#ifdef ENABLE_WEB_SERVICE
    return Common::Android::ToJString(env,
                                      WebService::NextendoApi::RemoveFriend(static_cast<u64>(jpid)));
#else
    return Common::Android::ToJString(env, "This build has no web services support.");
#endif
}

// Whether title_id is in Nextendo::CompatibleTitles -- only one game version per title can
// reach the server, and there's no server-side endpoint to check this, so it's a client-side
// table lookup. Doesn't require a linked account; callers should still check isLinked().
jboolean Java_org_citron_citron_1emu_utils_NativeNextendo_isCloudSaveEligible(JNIEnv* env,
                                                                           jclass clazz,
                                                                           jlong jtitle_id) {
    return static_cast<jboolean>(
        Nextendo::CompatibleTitles::Table().count(static_cast<u64>(jtitle_id)) != 0);
}

// Blocking network call -- background thread only. Fire-and-forget like the desktop
// NextendoController::ManualSaveDownload it mirrors: SaveSync::Pull() only logs, it doesn't
// report a granular error back to the caller. force=true skips the no-overwrite check.
void Java_org_citron_citron_1emu_utils_NativeNextendo_pullSave(JNIEnv* env, jclass clazz,
                                                             jlong jtitle_id, jboolean jforce) {
#ifdef ENABLE_WEB_SERVICE
    Nextendo::SaveSync::Pull(EmulationSession::GetInstance().System(),
                             static_cast<u64>(jtitle_id), static_cast<bool>(jforce));
#endif
}

// Blocking network call -- background thread only. Same fire-and-forget contract as pullSave.
void Java_org_citron_citron_1emu_utils_NativeNextendo_pushSave(JNIEnv* env, jclass clazz,
                                                             jlong jtitle_id) {
#ifdef ENABLE_WEB_SERVICE
    auto& system = EmulationSession::GetInstance().System();
    auto zip = Nextendo::SaveSync::CaptureForPush(system, static_cast<u64>(jtitle_id));
    Nextendo::SaveSync::UploadCaptured(static_cast<u64>(jtitle_id), std::move(zip));
#endif
}

} // extern "C"
