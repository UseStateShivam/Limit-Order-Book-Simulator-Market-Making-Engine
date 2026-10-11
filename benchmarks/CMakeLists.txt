cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# Fetch Google Benchmark
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(lob_benchmarks
    matching_benchmarks.cpp
    order_book_benchmarks.cpp
    replay_benchmarks.cpp
)

target_link_libraries(lob_benchmarks PRIVATE
    lob_core
    benchmark::benchmark_main
)

target_include_directories(lob_benchmarks PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}
)
