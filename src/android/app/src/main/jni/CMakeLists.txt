# SPDX-FileCopyrightText: 2023 yuzu Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(citron-android SHARED
    emu_window/emu_window.cpp
    emu_window/emu_window.h
    native.cpp
    native.h
    native_config.cpp
    native_nextendo.cpp
    android_settings.cpp
    game_metadata.cpp
    native_log.cpp
    android_config.cpp
    android_config.h
    native_input.cpp
)

set_property(TARGET citron-android PROPERTY IMPORTED_LOCATION ${FFmpeg_LIBRARY_DIR})

target_link_libraries(citron-android PRIVATE audio_core common core input_common frontend_common network web_service Vulkan::Headers)
target_link_libraries(citron-android PRIVATE android camera2ndk EGL glad jnigraphics log)
if (ARCHITECTURE_arm64)
    target_link_libraries(citron-android PRIVATE adrenotools)
endif()

# Every other frontend (citron, citron-cmd, dedicated_room) gets this; citron-android didn't,
# so native_nextendo.cpp's `#ifdef ENABLE_WEB_SERVICE` was always false here even with the
# library built+linked -- sign-in/friends/cloud-save silently reported "no web services support".
if (ENABLE_WEB_SERVICE)
    target_compile_definitions(citron-android PRIVATE -DENABLE_WEB_SERVICE)
endif()

set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} citron-android)
