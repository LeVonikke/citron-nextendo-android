// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.citron.citron_emu.utils

import android.content.Intent
import android.net.Uri
import androidx.annotation.Keep
import org.citron.citron_emu.CitronApplication
import org.citron.citron_emu.model.NextendoFriend

object NativeNextendo {
    /**
     * Points the Nextendo backend client at the CA bundle extracted from the bundled
     * nextendo_ca.pem asset. Must be called once at startup, before any other Nextendo
     * network call -- see CitronApplication.onCreate.
     */
    external fun setCaCertPath(path: String)

    /**
     * Whether a Nextendo account is currently linked on this device.
     */
    external fun isLinked(): Boolean

    /**
     * The linked account's display name, or an empty string if not linked.
     */
    external fun getUsername(): String

    /**
     * The linked account's friend code, or an empty string if not linked.
     */
    external fun getFriendCode(): String

    /**
     * Blocking -- runs the OAuth loopback sign-in flow and does not return until it completes
     * or times out (up to 5 minutes). Must be called from a background thread/coroutine only;
     * calling this from the main thread freezes the whole app for the duration.
     *
     * Returns an empty string on success, or a human-readable error message on failure.
     */
    external fun signIn(): String

    /**
     * Clears the linked account and any cached friends state.
     */
    external fun signOut()

    /**
     * Confirmed friends and incoming (awaiting accept/decline) requests together, distinguished
     * by [NextendoFriend.isIncomingRequest]. Blocking network call -- background thread only.
     */
    external fun getFriends(): Array<NextendoFriend>

    /**
     * Friend codes a request has been sent to but that haven't shown up as a confirmed friend
     * yet. Local-only, no network call.
     */
    external fun getOutgoingRequestCodes(): Array<String>

    /**
     * Sends a friend request by code. Blocking network call -- background thread only. Returns
     * an empty string on success, or a human-readable error message on failure.
     */
    external fun addFriendByCode(friendCode: String): String

    /** Blocking network call -- background thread only. Empty string return means success. */
    external fun acceptFriend(pid: Long): String

    /** Blocking network call -- background thread only. Empty string return means success. */
    external fun declineFriend(pid: Long): String

    /** Blocking network call -- background thread only. Empty string return means success. */
    external fun removeFriend(pid: Long): String

    /**
     * Whether titleId is on Nextendo's cloud-save-compatible title list. Local check, no
     * network call. Callers should also check [isLinked] before offering cloud save actions.
     */
    external fun isCloudSaveEligible(titleId: Long): Boolean

    /**
     * Blocking -- background thread only. Downloads and applies the cloud save for titleId. If
     * force is false and a local save already exists, the cloud save is not applied (no
     * overwrite). Fire-and-forget: failures are logged natively, not reported back here.
     */
    external fun pullSave(titleId: Long, force: Boolean)

    /**
     * Blocking -- background thread only. Zips the local save for titleId and uploads it.
     * Fire-and-forget: failures are logged natively, not reported back here.
     */
    external fun pushSave(titleId: Long)

    /**
     * Native callback fired synchronously from within [signIn] once the sign-in URL is ready.
     * Opens it in the user's browser so they can complete the OAuth flow. No further app-side
     * handling is needed afterwards -- [signIn] unblocks itself once its embedded loopback
     * server receives the redirect, independent of whether/when the user returns to the app.
     */
    @Keep
    @JvmStatic
    fun onOpenSignInUrl(url: String) {
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url)).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        CitronApplication.appContext.startActivity(intent)
    }
}
