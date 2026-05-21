#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== MyDB Build Script ==="

# Check dependencies
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "ERROR: '$1' not found. Please install it."
        exit 1
    fi
}

check_dep cmake
check_dep g++

echo "✓ All dependencies found"

# Configure
echo "→ Configuring..."
mkdir -p "$BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | tail -5

# Build
echo "→ Building..."
cmake --build "$BUILD_DIR" --parallel 4

echo "✓ Build successful!"
echo ""

# Mode switch
if [[ "$1" == "--test" ]]; then
    echo "=== Running Tests ==="
    cd "$BUILD_DIR"
    ./mydb_test
elif [[ "$1" == "--demo" ]]; then
    echo "=== Running Demo ==="
    cd "$BUILD_DIR"
    ./mydb "$SCRIPT_DIR/demo.sql"
elif [[ "$1" == "--file" && -n "$2" ]]; then
    echo "=== Executing SQL file: $2 ==="
    cd "$BUILD_DIR"
    ./mydb "$2"
else
    echo "=== Starting Interactive Shell ==="
    cd "$BUILD_DIR"
    ./mydb
fi
