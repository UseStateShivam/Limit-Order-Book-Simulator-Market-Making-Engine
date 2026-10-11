cmake_minimum_required(VERSION 3.20)
project(LimitOrderBookMarketMaker
    VERSION 1.0.0
    DESCRIPTION "High-Performance Limit Order Book Simulator & Quantitative Market-Making Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create an interface library for our project's strict compiler warnings
add_library(project_warnings INTERFACE)
if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /permissive- /utf-8)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(project_warnings INTERFACE /O2 /Oi /Ot /GL)
    endif()
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wno-sign-conversion)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(project_warnings INTERFACE -O3 -march=native -flto)
    endif()
endif()

# Core static library
add_library(lob_core STATIC
    src/core/config.cpp
    src/book/order_book.cpp
    src/matching/matching_engine.cpp
    src/matching/reference_engine.cpp
    src/market_data/csv_parser.cpp
    src/market_data/synthetic_generator.cpp
    src/strategy/market_maker_strategy.cpp
    src/analytics/metrics.cpp
    src/simulation/simulator.cpp
)

target_link_libraries(lob_core PUBLIC
    project_warnings
)

target_include_directories(lob_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main CLI executable (lob_sim)
add_executable(lob_sim
    src/main.cpp
    src/app/cli.cpp
)

target_link_libraries(lob_sim PRIVATE
    lob_core
    project_warnings
)

target_include_directories(lob_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Example executables
add_executable(example_basic_matching examples/basic_matching.cpp)
target_link_libraries(example_basic_matching PRIVATE lob_core project_warnings)

add_executable(example_synthetic_market examples/synthetic_market.cpp)
target_link_libraries(example_synthetic_market PRIVATE lob_core project_warnings)

add_executable(example_market_maker examples/market_maker.cpp)
target_link_libraries(example_market_maker PRIVATE lob_core project_warnings)

# Subdirectories
option(BUILD_TESTING "Build the test suite" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_BENCHMARKS "Build the benchmark suite" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
