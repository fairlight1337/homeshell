# Test executable
add_executable(homeshell_tests
    test_status.cpp
    test_config.cpp
    test_filesystem_helper.cpp
    test_prompt_formatter.cpp
    test_ls_command.cpp
    test_cat_command.cpp
    test_mkdir_touch_rm_commands.cpp
    test_encrypted_mount.cpp
    test_virtual_filesystem.cpp
    test_archive_commands.cpp
    test_tree_command.cpp
    test_system_commands.cpp
    test_sysinfo_commands.cpp
    test_acpi_info_command.cpp
    test_cpu_info_command.cpp
    test_df_command.cpp
    test_du_command.cpp
    test_output_redirection.cpp
    test_utility_commands.cpp
    test_hostname_uptime_free_head_wc.cpp
    test_cp_mv_ln.cpp
    test_tee_diff.cpp
    test_sha256_md5_base64.cpp
    test_ps_tar_curl.cpp
    test_find_command.cpp
    test_grep_command.cpp
    test_locate_commands.cpp
    test_tail_command.cpp
    test_less_command.cpp
)

# Disable clang-tidy for tests
set_target_properties(homeshell_tests PROPERTIES CXX_CLANG_TIDY "")

# Apply coverage flags to tests if enabled
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(homeshell_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage -O0 -g)
        target_link_options(homeshell_tests PRIVATE --coverage)
        message(STATUS "Coverage flags applied to homeshell_tests")
    endif()
endif()

target_link_libraries(homeshell_tests
    PRIVATE
        homeshell
        gtest
        gtest_main
        nlohmann_json::nlohmann_json
        fmt::fmt
        miniz
        ncurses
)

target_include_directories(homeshell_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

# Discover tests
gtest_discover_tests(homeshell_tests)

