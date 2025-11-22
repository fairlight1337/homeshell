cmake_minimum_required(VERSION 3.20)

# Project configuration
project(homeshell VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable static linking where possible
option(STATIC_LINKING "Enable static linking" ON)

if(STATIC_LINKING)
    # Static linking flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    
    # Prefer static libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    
    message(STATUS "Static linking enabled: libstdc++ and libgcc will be statically linked")
endif()

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

# Force all libraries to be built as static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Add CLI11 for command-line argument parsing (header-only)
add_subdirectory(external/CLI11)

# Add nlohmann/json for JSON parsing (header-only)
add_subdirectory(external/json)

# Add fmt for modern formatting with color support
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/fmt)

# Add replxx for interactive line editing
add_subdirectory(external/replxx)

# Add SQLCipher for encrypted storage
add_subdirectory(external/sqlcipher)

# Add miniz for zip/unzip functionality
# Build miniz directly without modifying the submodule
add_library(miniz STATIC
    external/miniz/miniz.c
    external/miniz/miniz_tdef.c
    external/miniz/miniz_tinfl.c
    external/miniz/miniz_zip.c
)

target_include_directories(miniz
    PUBLIC
        ${CMAKE_SOURCE_DIR}/external/miniz
        # Include compatibility headers for miniz_export.h
        ${CMAKE_SOURCE_DIR}/include/miniz_compat
)

# Suppress warnings from miniz (third-party code)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(miniz PRIVATE -w)
endif()

# Add Google Test for unit testing
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(external/googletest)
    include(GoogleTest)
endif()

# Library target
add_library(homeshell
    src/Homeshell.cpp
    src/EncryptedMount.cpp
    src/VirtualFilesystem.cpp
    src/OutputRedirection.cpp
)

target_include_directories(homeshell
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/sqlcipher>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(homeshell PUBLIC cxx_std_20)

# Executable target
add_executable(homeshell-linux
    src/main.cpp
)

target_link_libraries(homeshell
    PUBLIC
        sqlcipher
)

# Find ncurses for the editor
find_package(Curses REQUIRED)

# Try to find static ncurses library if static linking is enabled
if(STATIC_LINKING)
    # Look for static ncurses library
    find_library(NCURSES_STATIC_LIBRARY
        NAMES libncurses.a
        PATHS /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu
        NO_DEFAULT_PATH
    )
    
    # Look for static tinfo library (required by static ncurses)
    find_library(TINFO_STATIC_LIBRARY
        NAMES libtinfo.a
        PATHS /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu
        NO_DEFAULT_PATH
    )
    
    if(NCURSES_STATIC_LIBRARY AND TINFO_STATIC_LIBRARY)
        set(NCURSES_LINK_LIBRARIES ${NCURSES_STATIC_LIBRARY} ${TINFO_STATIC_LIBRARY})
        message(STATUS "Using static ncurses: ${NCURSES_STATIC_LIBRARY}")
        message(STATUS "Using static tinfo: ${TINFO_STATIC_LIBRARY}")
    elseif(NCURSES_STATIC_LIBRARY)
        # Try without tinfo (some systems don't separate them)
        set(NCURSES_LINK_LIBRARIES ${NCURSES_STATIC_LIBRARY})
        message(STATUS "Using static ncurses: ${NCURSES_STATIC_LIBRARY}")
    else()
        set(NCURSES_LINK_LIBRARIES ${CURSES_LIBRARIES})
        message(STATUS "Static ncurses not found, using dynamic: ${CURSES_LIBRARIES}")
    endif()
else()
    set(NCURSES_LINK_LIBRARIES ${CURSES_LIBRARIES})
endif()

target_link_libraries(homeshell-linux
    PRIVATE
        homeshell
        CLI11::CLI11
        nlohmann_json::nlohmann_json
        fmt::fmt
        replxx
        miniz
        ${NCURSES_LINK_LIBRARIES}
)

target_include_directories(homeshell-linux
    PRIVATE
        ${CURSES_INCLUDE_DIRS}
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

# Tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

