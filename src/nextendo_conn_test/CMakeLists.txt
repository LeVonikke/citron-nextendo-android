# SPDX-FileCopyrightText: 2026 citron Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(nextendo_conn_test
    nextendo_conn_test.cpp
)

create_target_directory_groups(nextendo_conn_test)

target_link_libraries(nextendo_conn_test PRIVATE common web_service)

if(UNIX AND NOT APPLE)
    install(TARGETS nextendo_conn_test)
endif()
