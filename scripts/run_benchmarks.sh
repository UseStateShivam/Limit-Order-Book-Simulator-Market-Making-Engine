#!/usr/bin/env bash
set -e

echo "=== Building and Running Performance Benchmarks ==="
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --target lob_benchmarks --parallel
./build/benchmarks/lob_benchmarks --benchmark_counters_tabular=true
