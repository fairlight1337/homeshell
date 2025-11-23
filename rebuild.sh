#!/bin/bash
set -e

echo "=== Cleaning build directory ==="
rm -rf build

echo "=== Rebuilding SQLCipher amalgamation ==="
cd external/sqlcipher
./configure CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=2" LDFLAGS="-lcrypto"
make sqlite3.c sqlite3.h

# Create wrapper file with proper defines and includes
echo "=== Creating sqlite3_wrapper.c ==="
cat > sqlite3_wrapper.c << 'EOF'
/* Wrapper to ensure proper headers are included before sqlite3.c */
#define _GNU_SOURCE
#include <stdint.h>
#include "sqlite3.c"
EOF
echo "Created sqlite3_wrapper.c"
cd ../..

echo "=== Building MicroPython embed files ==="
cd external
make -f micropython_build.mk
cd ..

echo "=== Configuring CMake ==="
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..

echo "=== Building project ==="
make -j$(nproc)

echo "=== Build complete! ==="
ls -lh homeshell-linux

