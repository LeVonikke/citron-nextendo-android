// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/nextendo/history_sync.h"

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/nextendo_account.h"
#include "common/string_util.h"

#ifdef ENABLE_WEB_SERVICE
#include "web_service/nextendo_api.h"
#endif

namespace Nextendo::HistorySync {

namespace {

std::mutex g_mutex;
bool g_loaded = false;
std::unordered_map<u64, u64> g_seconds_by_title; // title_id -> cumulative seconds

std::filesystem::path FilePath() {
    return Common::FS::GetCitronPath(Common::FS::CitronPath::ConfigDir) / "nextendo_play_time.txt";
}

// Caller holds g_mutex.
void EnsureLoaded() {
    if (g_loaded) {
        return;
    }
    g_loaded = true;

    const std::string contents = Common::FS::ReadStringFromFile(FilePath(), Common::FS::FileType::TextFile);
    std::vector<std::string> lines;
    Common::SplitString(contents, '\n', lines);

    for (const auto& line : lines) {
        const auto eq = line.find('=');
        if (eq == std::string::npos || eq == 0) {
            continue;
        }
        const std::string key = Common::StripSpaces(line.substr(0, eq));
        const std::string value = Common::StripSpaces(line.substr(eq + 1));
        try {
            g_seconds_by_title[std::stoull(key, nullptr, 16)] = std::stoull(value);
        } catch (...) {
            // Skip a malformed line rather than losing every other title's time over it.
        }
    }
}

// Caller holds g_mutex.
void WriteFile() {
    void(Common::FS::CreateParentDirs(FilePath()));
    std::string contents;
    for (const auto& [title_id, seconds] : g_seconds_by_title) {
        contents += fmt::format("{:016x}={}\n", title_id, seconds);
    }
    void(Common::FS::WriteStringToFile(FilePath(), Common::FS::FileType::TextFile, contents));
}

// RFC 4648 standard (padded) base64 -- HistoryEntry::icon_base64 expects the same encoding
// desktop produces via QByteArray::toBase64(), which this build has no Qt to call.
std::string Base64StdEncode(std::span<const u8> data) {
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 3 <= data.size(); i += 3) {
        const u32 n = (u32{data[i]} << 16) | (u32{data[i + 1]} << 8) | u32{data[i + 2]};
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += table[n & 0x3F];
    }
    const std::size_t rem = data.size() - i;
    if (rem == 1) {
        const u32 n = u32{data[i]} << 16;
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += "==";
    } else if (rem == 2) {
        const u32 n = (u32{data[i]} << 16) | (u32{data[i + 1]} << 8);
        out += table[(n >> 18) & 0x3F];
        out += table[(n >> 12) & 0x3F];
        out += table[(n >> 6) & 0x3F];
        out += '=';
    }
    return out;
}

} // Anonymous namespace

void RecordSessionAndSync([[maybe_unused]] u64 program_id, [[maybe_unused]] u64 session_seconds,
                          [[maybe_unused]] const std::string& game_name,
                          [[maybe_unused]] std::span<const u8> icon_bytes) {
#ifdef ENABLE_WEB_SERVICE
    if (program_id == 0 || session_seconds == 0) {
        return;
    }

    u64 total_seconds;
    {
        std::lock_guard lock{g_mutex};
        EnsureLoaded();
        total_seconds = (g_seconds_by_title[program_id] += session_seconds);
        WriteFile();
    }

    if (!Common::NextendoAccount::IsLinked()) {
        return;
    }

    WebService::NextendoApi::HistoryEntry entry;
    entry.title_id = fmt::format("{:016X}", program_id);
    entry.name = game_name;
    entry.seconds = total_seconds;
    entry.last_played = fmt::format(
        "{:%Y-%m-%dT%H:%M:%SZ}", fmt::gmtime(std::chrono::system_clock::to_time_t(
                                     std::chrono::system_clock::now())));
    entry.icon_base64 = Base64StdEncode(icon_bytes);

    LOG_INFO(Frontend, "Nextendo history (Android): title={:016X} session_s={} total_s={}",
             program_id, session_seconds, total_seconds);

    // Detached: shutdown must not block on the network.
    std::thread{[entry] { WebService::NextendoApi::SyncHistory({entry}); }}.detach();
#endif
}

} // namespace Nextendo::HistorySync
