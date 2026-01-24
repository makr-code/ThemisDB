# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

# Ensure consistent MSVC Runtime with vcpkg triplet
if(MSVC)
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY} (from vcpkg triplet)")
endif()

# Find GTest
find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found - tests will not be built")
    message(WARNING "Install GTest or use vcpkg: vcpkg install gtest")
    return()
endif()

message(STATUS "GTest found - building unified test binary")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

# ============================================================================
# Unified Test Binary (Google Test Best Practice)
# ============================================================================
# Following Google Test best practices: one main binary with gtest_main,
# all test sources without their own main() function.

message(STATUS "Creating unified test binary: themis_tests")

# Collect all test sources (excluding manual/standalone tests)
file(GLOB_RECURSE ALL_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

# Filter out non-gtest files (manual tests, benchmarks, etc.)
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_minimal\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_manual.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*benchmark.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_wire_protocol_manual\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_env\\.cpp$")

list(LENGTH ALL_TEST_SOURCES TEST_SOURCE_COUNT)
message(STATUS "Found ${TEST_SOURCE_COUNT} test files")

# Create single unified test executable
add_executable(themis_tests ${ALL_TEST_SOURCES})

target_link_libraries(themis_tests PRIVATE
    GTest::gtest
    GTest::gtest_main  # Provides main() - no custom main needed in sources
    themis_core
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    RocksDB::rocksdb
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    protobuf::libprotobuf
    gRPC::grpc++
    Threads::Threads
)

if(TARGET llama)
    target_link_libraries(themis_tests PRIVATE llama)
endif()

target_compile_definitions(themis_tests PRIVATE
    THEMIS_TEST_BUILD=1
    THEMIS_ENABLE_LLM=$<BOOL:${THEMIS_ENABLE_LLM}>
)

# Register with CTest
add_test(NAME AllTests COMMAND themis_tests)

set_tests_properties(AllTests PROPERTIES
    LABELS "all;unified"
    TIMEOUT 600
)

# ============================================================================
# Custom Test Targets
# ============================================================================

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all ThemisDB tests..."
)

message(STATUS "Tests configured successfully")
message(STATUS "  - Build unified test binary: cmake --build . --target themis_tests")
message(STATUS "  - Run all tests: ctest --output-on-failure")
message(STATUS "  - Run specific test: ./themis_tests --gtest_filter=TestName.*")
message(STATUS "  - List all tests: ./themis_tests --gtest_list_tests")
