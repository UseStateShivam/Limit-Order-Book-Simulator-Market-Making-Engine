cmake_minimum_required(VERSION 3.20)

enable_testing()

include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(lob_tests
    unit/test_order_arena.cpp
    unit/test_order_book.cpp
    unit/test_matching_engine.cpp
    unit/test_strategy.cpp
    unit/test_accounting.cpp
    unit/test_csv_parser.cpp
    property/test_differential.cpp
    integration/test_end_to_end.cpp
)

target_link_libraries(lob_tests PRIVATE
    lob_core
    GTest::gtest_main
)

target_include_directories(lob_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}
)

target_compile_definitions(lob_tests PRIVATE
    TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/data/sample"
)

include(GoogleTest)
gtest_discover_tests(lob_tests)
