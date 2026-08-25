// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.citron.citron_emu.model

/**
 * Mirrors WebService::NextendoApi::Friend. Constructed directly from JNI (native_nextendo.cpp,
 * NativeNextendo.getFriends) -- field order must match the cached constructor signature in
 * id_cache.cpp exactly.
 */
data class NextendoFriend(
    val pid: Long,
    val name: String,
    val friendCode: String,
    val presenceStatus: Int,
    val appName: String,
    val isIncomingRequest: Boolean
) {
    companion object {
        const val PRESENCE_OFFLINE = 0
        const val PRESENCE_ONLINE = 1
        const val PRESENCE_IN_GAME = 2
    }
}
