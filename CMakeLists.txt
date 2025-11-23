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
# SQLCipher is built separately using configure/make, which generates sqlite3.c
# We use a wrapper file that includes stdint.h first to fix uint64_t errors
add_library(sqlcipher STATIC
    external/sqlcipher/sqlite3_wrapper.c
)
target_include_directories(sqlcipher PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/sqlcipher>
)
target_compile_definitions(sqlcipher PRIVATE
    SQLITE_HAS_CODEC
    SQLITE_TEMP_STORE=2
    SQLITE_EXTRA_INIT=sqlcipher_extra_init
    SQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown
)
target_link_libraries(sqlcipher PRIVATE crypto)

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

# Add MicroPython embedded interpreter
# First, check if micropython_embed directory exists
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/micropython_embed)
    message(STATUS "MicroPython embed files not found, generating...")
    execute_process(
        COMMAND make -f micropython_build.mk
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external
        RESULT_VARIABLE MICROPYTHON_BUILD_RESULT
    )
    if(NOT MICROPYTHON_BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build MicroPython embed files")
    endif()
endif()

# Collect all MicroPython source files
file(GLOB MICROPYTHON_SOURCES
    ${CMAKE_SOURCE_DIR}/external/micropython_embed/py/*.c
    ${CMAKE_SOURCE_DIR}/external/micropython_embed/shared/runtime/*.c
    ${CMAKE_SOURCE_DIR}/external/micropython_embed/port/*.c
)

add_library(micropython STATIC 
    ${MICROPYTHON_SOURCES}
    ${CMAKE_SOURCE_DIR}/src/micropython_port.c
)

target_include_directories(micropython
    PUBLIC
        ${CMAKE_SOURCE_DIR}/external/micropython_embed
        ${CMAKE_SOURCE_DIR}/external/micropython_embed/port
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/micropython_embed/py
        ${CMAKE_SOURCE_DIR}/external/micropython_embed/genhdr
)

# MicroPython compile options
target_compile_definitions(micropython PRIVATE
    MICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
)

# Suppress warnings from MicroPython (third-party code)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(micropython PRIVATE -w)
endif()

# MicroPython requires math library
target_link_libraries(micropython PRIVATE m)

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
    src/FileDatabase.cpp
    src/commands/PythonCommand.cpp
    src/commands/ChmodCommand.cpp
    src/commands/VersionCommand.cpp
    src/commands/FindCommand.cpp
)

target_include_directories(homeshell
    BEFORE PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/sqlcipher>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/micropython_embed>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(homeshell PUBLIC cxx_std_20)

# homeshell needs SQLITE_HAS_CODEC to access SQLCipher encryption functions
target_compile_definitions(homeshell PUBLIC SQLITE_HAS_CODEC)

# Executable target
add_executable(homeshell-linux
    src/main.cpp
)

target_link_libraries(homeshell
    PUBLIC
        sqlcipher
        micropython
        fmt::fmt
        replxx
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
        micropython
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

# ============================================================================
# Documentation Generation (Doxygen)
# ============================================================================
find_package(Doxygen)

if(DOXYGEN_FOUND)
    # Configure Doxygen
    set(DOXYGEN_PROJECT_NAME "Homeshell")
    set(DOXYGEN_PROJECT_BRIEF "A modern C++ shell with encrypted virtual filesystem support")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_INPUT "${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src")
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE NO)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_HTML_OUTPUT "html")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
    set(DOXYGEN_MARKDOWN_SUPPORT YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
    set(DOXYGEN_WARN_NO_PARAMDOC YES)
    set(DOXYGEN_WARN_AS_ERROR NO)
    set(DOXYGEN_QUIET NO)
    set(DOXYGEN_FILE_PATTERNS "*.hpp *.h *.cpp *.c *.md")
    set(DOXYGEN_EXCLUDE_PATTERNS "*/external/* */build/* */.git/*")
    set(DOXYGEN_ENABLE_PREPROCESSING YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF NO)
    set(DOXYGEN_HAVE_DOT NO)
    
    # Generate Doxyfile
    doxygen_add_docs(docs
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/README.md
        COMMENT "Generating HTML documentation with Doxygen"
    )
    
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    message(STATUS "Documentation will be generated in: ${DOXYGEN_OUTPUT_DIRECTORY}/html")
    message(STATUS "Run 'make docs' to generate documentation")
else()
    message(WARNING "Doxygen not found. Documentation generation will not be available.")
endif()

# ============================================================================
# Documentation Coverage Check
# ============================================================================
if(DOXYGEN_FOUND)
    # Create a target to check for undocumented functions
    add_custom_target(docs-coverage
        COMMAND ${CMAKE_COMMAND} -E echo "Checking for undocumented items..."
        COMMAND ${DOXYGEN_EXECUTABLE} -w html header.html footer.html customdoxygen.css > /dev/null 2>&1 || true
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/docs-coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "PROJECT_NAME = \"Homeshell\"" > "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "INPUT = ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "RECURSIVE = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "EXTRACT_ALL = NO" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "WARNINGS = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "WARN_IF_UNDOCUMENTED = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "WARN_IF_DOC_ERROR = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "WARN_NO_PARAMDOC = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "WARN_AS_ERROR = NO" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "QUIET = YES" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "GENERATE_HTML = NO" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "GENERATE_LATEX = NO" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "EXCLUDE_PATTERNS = */external/* */build/*" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "FILE_PATTERNS = *.hpp *.h *.cpp *.c" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "" >> "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage"
        COMMAND ${CMAKE_COMMAND} -E echo "=== Undocumented Items Report ==="
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/docs-coverage/Doxyfile.coverage" 2>&1 | grep -E "(Warning:|not documented)" | tee "${CMAKE_BINARY_DIR}/docs-coverage/undocumented.txt" || true
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Report saved to: ${CMAKE_BINARY_DIR}/docs-coverage/undocumented.txt"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Checking documentation coverage"
        VERBATIM
    )
    
    message(STATUS "Run 'make docs-coverage' to check for undocumented items")
endif()

# ============================================================================
# Code Coverage (gcov/lcov)
# ============================================================================
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags
        set(COVERAGE_COMPILE_FLAGS "--coverage" "-fprofile-arcs" "-ftest-coverage" "-O0" "-g")
        set(COVERAGE_LINK_FLAGS "--coverage")
        
        # Apply to homeshell library
        target_compile_options(homeshell PRIVATE ${COVERAGE_COMPILE_FLAGS})
        target_link_options(homeshell PRIVATE ${COVERAGE_LINK_FLAGS})
        
        # Apply to main executable (needed since it links to instrumented library)
        target_compile_options(homeshell-linux PRIVATE ${COVERAGE_COMPILE_FLAGS})
        target_link_options(homeshell-linux PRIVATE ${COVERAGE_LINK_FLAGS})
        
        message(STATUS "Code coverage enabled")
        message(STATUS "Coverage compile flags: ${COVERAGE_COMPILE_FLAGS}")
        message(STATUS "Coverage link flags: ${COVERAGE_LINK_FLAGS}")
        message(STATUS "Coverage applied to: homeshell library, homeshell-linux executable")
    else()
        message(WARNING "Code coverage only supported with GCC or Clang")
    endif()
endif()

# Find lcov for coverage report generation
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
    # Coverage target
    add_custom_target(coverage
        # Clean previous coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning previous coverage data..."
        COMMAND ${LCOV_EXECUTABLE} --directory . --zerocounters
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/coverage"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/coverage"
        
        # Run tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure || true
        
        # Capture coverage data
        COMMAND ${CMAKE_COMMAND} -E echo "Capturing coverage data..."
        COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file "${CMAKE_BINARY_DIR}/coverage/coverage.info" --ignore-errors source
        
        # Extract only src/ and include/ directories (removes everything else)
        COMMAND ${CMAKE_COMMAND} -E echo "Filtering coverage data (keeping only src/ and include/)..."
        COMMAND ${LCOV_EXECUTABLE} --extract "${CMAKE_BINARY_DIR}/coverage/coverage.info" 
            "*/homeshell/src/*" "*/homeshell/include/*"
            --output-file "${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info"
        
        # Generate text summary
        COMMAND ${CMAKE_COMMAND} -E echo "Generating text summary..."
        COMMAND ${LCOV_EXECUTABLE} --list "${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info" > "${CMAKE_BINARY_DIR}/coverage/coverage_summary.txt"
        
        # Generate HTML report
        COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML report..."
        COMMAND ${GENHTML_EXECUTABLE} 
            "${CMAKE_BINARY_DIR}/coverage/coverage_filtered.info" 
            --output-directory "${CMAKE_BINARY_DIR}/coverage/html"
            --title "Homeshell Code Coverage"
            --show-details
            --legend
        
        # Print summary
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "=== Coverage Summary ==="
        COMMAND ${CMAKE_COMMAND} -E cat "${CMAKE_BINARY_DIR}/coverage/coverage_summary.txt"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Text report: ${CMAKE_BINARY_DIR}/coverage/coverage_summary.txt"
        COMMAND ${CMAKE_COMMAND} -E echo "HTML report: ${CMAKE_BINARY_DIR}/coverage/html/index.html"
        
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS homeshell_tests
        COMMENT "Generating code coverage report"
        VERBATIM
    )
    
    message(STATUS "lcov found: ${LCOV_EXECUTABLE}")
    message(STATUS "genhtml found: ${GENHTML_EXECUTABLE}")
    message(STATUS "Run 'cmake -DENABLE_COVERAGE=ON ..' then 'make coverage' to generate coverage reports")
else()
    if(NOT LCOV_EXECUTABLE)
        message(WARNING "lcov not found. Install it for coverage reporting (sudo apt-get install lcov)")
    endif()
    if(NOT GENHTML_EXECUTABLE)
        message(WARNING "genhtml not found. Install it for HTML coverage reports (part of lcov package)")
    endif()
endif()

# ============================================================================
# Tests
# ============================================================================
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

