# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

# Ensure consistent MSVC Runtime for x64-windows-static triplet
if(MSVC)
    # Force static runtime linking for consistent behavior with gtest
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

# Optional toggles for unstable/legacy suites
option(THEMIS_ENABLE_LORA_TESTS "Build experimental LoRA framework tests" OFF)

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
)

# ============================================================================
# LoRA Framework Tests (NEW)
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
        message(STATUS "Adding LoRA framework tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_lora_framework
            test_lora_framework.cpp
        )
        
        target_link_libraries(test_lora_framework PRIVATE
            ${TEST_LIBS}
            themis_core
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
else()
    message(STATUS "LoRA framework tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# LoRA Framework Comprehensive Tests (Issue: Unit Tests für LoRA Framework)
# ============================================================================

if(THEMIS_ENABLE_LORA_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework_comprehensive.cpp")
        message(STATUS "Adding LoRA framework comprehensive tests (THEMIS_ENABLE_LORA_TESTS=ON)")
        
        add_executable(test_lora_framework_comprehensive
            test_lora_framework_comprehensive.cpp
        )
        
        target_link_libraries(test_lora_framework_comprehensive PRIVATE
            ${TEST_LIBS}
            themis_core
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
else()
    message(STATUS "LoRA comprehensive tests disabled (THEMIS_ENABLE_LORA_TESTS=OFF)")
endif()

# ============================================================================
# PITR (Point-in-Time Recovery) Tests - Phase 3
# ============================================================================

# Test: Snapshot Manager
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_manager.cpp")
    message(STATUS "Adding Snapshot Manager tests")
    
    add_executable(test_snapshot_manager
        test_snapshot_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/transaction/snapshot_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/cdc/changefeed.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/rocksdb_wrapper.cpp
    )
    
    target_link_libraries(test_snapshot_manager PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_snapshot_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME SnapshotManagerTests COMMAND test_snapshot_manager)
    
    set_tests_properties(SnapshotManagerTests PROPERTIES
        LABELS "pitr;snapshot;mvcc"
        TIMEOUT 60
    )
else()
    message(STATUS "Snapshot Manager tests not found")
endif()

# Test: PITR Manager
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_pitr_manager.cpp")
    message(STATUS "Adding PITR Manager tests")
    
    add_executable(test_pitr_manager
        test_pitr_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/pitr_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/transaction/snapshot_manager.cpp
        ${CMAKE_SOURCE_DIR}/src/cdc/changefeed.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/rocksdb_wrapper.cpp
    )
    
    target_link_libraries(test_pitr_manager PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_pitr_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME PITRManagerTests COMMAND test_pitr_manager)
    
    set_tests_properties(PITRManagerTests PROPERTIES
        LABELS "pitr;recovery;mvcc"
        TIMEOUT 120
    )
else()
    message(STATUS "PITR Manager tests not found")
endif()

# ============================================================================
# Schema Manager Tests (Issue makr-code/ThemisDB#1)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_schema_manager.cpp")
    message(STATUS "Adding Schema Manager tests")
    
    add_executable(test_schema_manager
        test_schema_manager.cpp
    )
    
    target_link_libraries(test_schema_manager PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_schema_manager PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SchemaManagerTests COMMAND test_schema_manager)
    
    # Set test properties
    set_tests_properties(SchemaManagerTests PROPERTIES
        LABELS "schema;metadata;self-awareness"
        TIMEOUT 300
    )
else()
    message(STATUS "Schema Manager tests not found")
endif()

# ============================================================================
# Snapshot Manager Integration Tests (Phase 1)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_snapshot_integration.cpp")
    message(STATUS "Adding SnapshotManager integration tests")
    
    add_executable(test_snapshot_integration
        test_snapshot_integration.cpp
    )
    
    target_link_libraries(test_snapshot_integration PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        fmt::fmt
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_snapshot_integration PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SnapshotIntegrationTests COMMAND test_snapshot_integration)
    
    # Set test properties
    set_tests_properties(SnapshotIntegrationTests PROPERTIES
        LABELS "snapshot;transaction;mvcc;integration"
        TIMEOUT 600
    )
else()
    message(STATUS "SnapshotManager integration tests not found")
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
    message(STATUS "Diff Engine tests not found")
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
# Security Signature Verifier Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_signature_verifier.cpp")
    message(STATUS "Adding signature verifier tests")
    
    add_executable(test_signature_verifier
        test_signature_verifier.cpp
    )
    
    target_link_libraries(test_signature_verifier PRIVATE
        ${TEST_LIBS}
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
    )
    
    # Link to signature_verifier object files if available
    if(TARGET signature_verifier_obj)
        target_link_libraries(test_signature_verifier PRIVATE signature_verifier_obj)
    else()
        # Compile the source directly for now
        target_sources(test_signature_verifier PRIVATE
            ${CMAKE_SOURCE_DIR}/src/llm/security/signature_verifier.cpp
        )
    endif()
    
    target_compile_definitions(test_signature_verifier PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME SignatureVerifierTests COMMAND test_signature_verifier)
    
    # Set test properties
    set_tests_properties(SignatureVerifierTests PROPERTIES
        LABELS "security;signature;crypto"
        TIMEOUT 120
    )
else()
    message(STATUS "Signature verifier tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_signature_verifier.cpp")
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
