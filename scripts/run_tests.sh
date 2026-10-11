#!/usr/bin/env bash
set -e

echo "=== Building and Running All Tests ==="
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
