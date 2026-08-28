// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <string>

#include "common/common_types.h"

// Play-time history push against the Nextendo account server. Desktop already has this via
// GMainWindow::SyncNextendoHistory() + PlayTimeManager (Qt-only, persists cumulative time per
// title, called from OnEmulationStopped()); this is the Android equivalent -- Android has no
// PlayTimeManager at all, so this owns its own small persisted cumulative-seconds store instead
// of assuming one exists. Kept here (not in src/citron) so citron-android can use it too, same
// reasoning as save_sync.h/ldn_counts.h.
namespace Nextendo::HistorySync {

// Adds session_seconds to the persisted cumulative total for program_id, then -- if an account is
// linked and the new total is nonzero -- pushes the updated total (title, icon included) to the
// server from a detached thread, matching desktop's "shutdown must not block on the network".
// No-op if program_id is 0, session_seconds is 0, or ENABLE_WEB_SERVICE is off.
void RecordSessionAndSync(u64 program_id, u64 session_seconds, const std::string& game_name,
                          std::span<const u8> icon_bytes);

} // namespace Nextendo::HistorySync
