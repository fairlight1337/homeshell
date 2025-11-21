cmake_minimum_required(VERSION 3.20)

# Project configuration
project(homeshell VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set version flavor
set(PROJECT_VERSION_FLAVOR "stable" CACHE STRING "Version flavor string")

# Configure version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/homeshell/version.h.in
    ${CMAKE_BINARY_DIR}/include/homeshell/version.h
    @ONLY
)

# Add CLI11 for command-line argument parsing
add_subdirectory(external/CLI11)

# Add nlohmann/json for JSON parsing
add_subdirectory(external/json)

# Add fmt for modern formatting with color support
add_subdirectory(external/fmt)

# Add replxx for interactive line editing
add_subdirectory(external/replxx)

# Library target
add_library(homeshell
    src/Homeshell.cpp
)

target_include_directories(homeshell
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(homeshell PUBLIC cxx_std_20)

# Executable target
add_executable(homeshell-linux
    src/main.cpp
)

target_link_libraries(homeshell-linux
    PRIVATE
        homeshell
        CLI11::CLI11
        nlohmann_json::nlohmann_json
        fmt::fmt
        replxx
)

# Find clang-format
find_program(CLANG_FORMAT_EXECUTABLE
    NAMES clang-format
    DOC "Path to clang-format executable"
)

# Format target
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/include/*.hpp
        ${CMAKE_SOURCE_DIR}/include/*.h
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE}
            -i
            --style=file:${CMAKE_SOURCE_DIR}/config/.clang-format
            ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on source files"
        VERBATIM
    )
else()
    message(WARNING "clang-format not found. 'make format' target will not be available.")
endif()

# Clang-tidy integration
find_program(CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
    DOC "Path to clang-tidy executable"
)

if(CLANG_TIDY_EXECUTABLE)
    set(CMAKE_CXX_CLANG_TIDY
        ${CLANG_TIDY_EXECUTABLE}
        --config-file=${CMAKE_SOURCE_DIR}/config/.clang-tidy
    )
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXECUTABLE}")
else()
    message(WARNING "clang-tidy not found. Static analysis will not be available.")
endif()

