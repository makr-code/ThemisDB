# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

# Find GTest
find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found - tests will not be built")
    message(WARNING "Install GTest or use vcpkg: vcpkg install gtest")
    return()
endif()

message(STATUS "GTest found - building tests")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

# Common test libraries
set(TEST_LIBS
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

# ============================================================================
# LoRA Framework Tests (NEW)
# ============================================================================

# Check for LoRA framework test file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
    message(STATUS "Adding LoRA framework tests")
    
    add_executable(test_lora_framework
        test_lora_framework.cpp
    )
    
    target_link_libraries(test_lora_framework PRIVATE
        ${TEST_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
        # themis_llm
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_lora_framework PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME LoRAFrameworkTests COMMAND test_lora_framework)
    
    # Set test properties
    set_tests_properties(LoRAFrameworkTests PROPERTIES
        LABELS "lora;llm;framework"
        TIMEOUT 300
    )
else()
    message(STATUS "LoRA framework tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
endif()

# ============================================================================
# LoRA Framework Comprehensive Tests (Issue: Unit Tests für LoRA Framework)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework_comprehensive.cpp")
    message(STATUS "Adding LoRA framework comprehensive tests")
    
    add_executable(test_lora_framework_comprehensive
        test_lora_framework_comprehensive.cpp
    )
    
    target_link_libraries(test_lora_framework_comprehensive PRIVATE
        ${TEST_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
        # themis_llm
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_lora_framework_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME LoRAFrameworkComprehensiveTests COMMAND test_lora_framework_comprehensive)
    
    # Set test properties
    set_tests_properties(LoRAFrameworkComprehensiveTests PROPERTIES
        LABELS "lora;llm;framework;comprehensive"
        TIMEOUT 600
    )
    
    message(STATUS "  - Comprehensive LoRA tests: 50+ test cases covering:")
    message(STATUS "    * Storage operations (save, load, delete, versioning)")
    message(STATUS "    * Adapter lifecycle (load, unload, hot-swap, caching)")
    message(STATUS "    * Training service (on-the-fly, batch, callbacks)")
    message(STATUS "    * Multi-adapter management (quantization, multi-GPU)")
    message(STATUS "    * Thread-safety (concurrent operations)")
    message(STATUS "    * Error handling and edge cases")
    message(STATUS "    * Memory management and leak detection")
    message(STATUS "    * Performance benchmarks")
else()
    message(STATUS "LoRA framework comprehensive tests not found")
endif()

# ============================================================================
# Snapshot Manager Tests (Phase 1: Named Snapshots)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_manager.cpp")
    message(STATUS "Adding SnapshotManager tests")
    
    add_executable(test_snapshot_manager
        test_snapshot_manager.cpp
    )
    
    target_link_libraries(test_snapshot_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_snapshot_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SnapshotManagerTests COMMAND test_snapshot_manager)
    
    # Set test properties
    set_tests_properties(SnapshotManagerTests PROPERTIES
        LABELS "snapshot;transaction;mvcc"
        TIMEOUT 300
    )
else()
    message(STATUS "SnapshotManager tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_manager.cpp")
endif()

# ============================================================================
# Diff Engine Tests (Phase 2: Git-like features)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_diff_engine.cpp")
    message(STATUS "Adding Diff Engine tests")
    
    add_executable(test_diff_engine
        test_diff_engine.cpp
    )
    
    target_link_libraries(test_diff_engine PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_diff_engine PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME DiffEngineTests COMMAND test_diff_engine)
    
    # Set test properties
    set_tests_properties(DiffEngineTests PROPERTIES
        LABELS "diff;analytics;mvcc"
        TIMEOUT 300
    )
else()
    message(STATUS "Diff Engine tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_diff_engine.cpp")
endif()

# ============================================================================
# Existing Tests
# ============================================================================

# Add existing test files here as they are discovered
# Example pattern:
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
#     file(GLOB TEST_SOURCES "test_*.cpp")
#     foreach(TEST_SOURCE ${TEST_SOURCES})
#         get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
#         add_executable(${TEST_NAME} ${TEST_SOURCE})
#         target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LIBS})
#         add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#     endforeach()
# endif()

# ============================================================================
# Integration Tests Subdirectory
# ============================================================================

# Add integration tests if directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt")
    message(STATUS "Adding integration tests from: ${CMAKE_CURRENT_SOURCE_DIR}/integration")
    add_subdirectory(integration)
else()
    message(STATUS "Integration tests directory not found or no CMakeLists.txt")
endif()

# ============================================================================
# Test Discovery & Reporting
# ============================================================================

# Enable CTest XML output
set(CTEST_OUTPUT_ON_FAILURE ON)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS test_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all ThemisDB tests..."
)

# Add custom target to run only integration tests
add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R integration --output-on-failure --verbose
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ThemisDB integration tests..."
)

message(STATUS "Tests configured successfully")
message(STATUS "  - Build tests with: cmake --build . --target test_lora_framework")
message(STATUS "  - Run tests with: ctest --output-on-failure")
message(STATUS "  - Run integration tests with: ctest -R integration")
message(STATUS "  - Or use: make run_tests / make run_integration_tests")
