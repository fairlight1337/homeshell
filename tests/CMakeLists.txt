# Test executable
add_executable(homeshell_tests
    test_status.cpp
    test_config.cpp
    test_filesystem_helper.cpp
    test_prompt_formatter.cpp
    test_ls_command.cpp
)

# Disable clang-tidy for tests
set_target_properties(homeshell_tests PROPERTIES CXX_CLANG_TIDY "")

target_link_libraries(homeshell_tests
    PRIVATE
        homeshell
        gtest
        gtest_main
        nlohmann_json::nlohmann_json
        fmt::fmt
)

target_include_directories(homeshell_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

# Discover tests
gtest_discover_tests(homeshell_tests)

