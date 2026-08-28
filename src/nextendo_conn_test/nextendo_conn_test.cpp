// SPDX-FileCopyrightText: 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Standalone connectivity smoke test for the Nextendo Network backend. Runs the exact
// WebService::NextendoApi code the app ships (same nextendo_api.cpp, same ApplyCaCertPath,
// same base-url sanitization) against the real server, with no account/token needed.
//
// Not a substitute for testing sign-in on an actual Android device -- ApplyCaCertPath takes a
// different branch there (#if defined(__ANDROID__), the bundled nextendo_ca.pem override)
// than the #elif defined(__linux__) system-bundle-scan branch this host build exercises. What
// it DOES catch, before anything ships: backend down/unreachable, DNS/TLS breakage, the
// server-side API contract changing shape, and any BaseUrl()/SanitizeBaseUrl() regression.
//
// Usage: nextendo_conn_test [--base-url https://host] -- overrides NEXTENDO_API env var lookup
// Exit code 0 = all checks passed, 1 = one or more failed.

#include <cstdio>
#include <cstdlib>
#include <string>

#include "web_service/nextendo_api.h"

namespace {

int g_failures = 0;

void Check(const char* name, bool ok, const std::string& detail = {}) {
    std::printf("[%s] %s%s%s\n", ok ? "PASS" : "FAIL", name, detail.empty() ? "" : " -- ",
                detail.c_str());
    if (!ok) {
        ++g_failures;
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 2 && std::string(argv[1]) == "--base-url") {
#ifdef _WIN32
        _putenv_s("NEXTENDO_API", argv[2]);
#else
        setenv("NEXTENDO_API", argv[2], 1);
#endif
    }

    std::printf("Nextendo Network connectivity check -- base URL: %s\n",
                WebService::NextendoApi::BaseUrl().c_str());

    const auto ping = WebService::NextendoApi::PingBackend();
    Check("GET /api/health", ping.has_value(),
          ping ? ("round-trip " + std::to_string(*ping) + "ms") : std::string("no response"));

    const auto counts = WebService::NextendoApi::GetOnlineCounts();
    Check("GET /api/online-counts", !counts.empty(),
          std::to_string(counts.size()) + " titles reported");

    if (g_failures > 0) {
        std::fprintf(stderr,
                      "\n%d check(s) failed -- Nextendo backend is not reachable from this "
                      "build. Do not publish a release until this passes.\n",
                      g_failures);
        return 1;
    }
    std::printf("\nAll checks passed.\n");
    return 0;
}
