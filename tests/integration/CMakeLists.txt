# Integration Tests CMakeLists.txt

# Note: This file is a template for organizing integration tests.
# Uncomment and configure sections as tests are implemented.

option(THEMIS_ENABLE_PROCESS_MINING_TESTS "Build process mining integration tests" OFF)

# Integration test infrastructure
set(INTEGRATION_TEST_COMMON
    test_fixture.h
    test_data_generator.h
)

# Helper function to add integration test
function(add_integration_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(${TEST_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    
    target_compile_definitions(${TEST_NAME} PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    set_tests_properties(${TEST_NAME} PROPERTIES
        LABELS "integration"
        TIMEOUT 300  # 5 minutes timeout for integration tests
    )
endfunction()

# ============================================================================
# Base Module Integration Tests
# ============================================================================

# HotReloadManager integration tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hot_reload_manager_integration_test.cpp")
    add_integration_test(
        hot_reload_manager_integration_test
        hot_reload_manager_integration_test.cpp
    )
    target_link_libraries(hot_reload_manager_integration_test PRIVATE
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    set_tests_properties(hot_reload_manager_integration_test PROPERTIES
        LABELS "integration;base;hot_reload"
    )
endif()

# ============================================================================
# Storage Integration Tests
# ============================================================================

# Backup/Recovery integration tests
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/storage/backup_recovery_integration_test.cpp")
    add_integration_test(
        backup_recovery_integration_test
        storage/backup_recovery_integration_test.cpp
    )
    set_tests_properties(backup_recovery_integration_test PROPERTIES
        LABELS "integration;storage;backup"
    )
endif()

# ============================================================================
# LLM Integration Tests (if available)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/llm/llm_inference_integration_test.cpp")
    add_integration_test(
        llm_inference_integration_test
        llm/llm_inference_integration_test.cpp
    )
    set_tests_properties(llm_inference_integration_test PROPERTIES
        LABELS "integration;llm;inference"
    )
endif()

# ============================================================================
# RPC Integration Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/rpc_service_integration_test.cpp")
    add_integration_test(
        rpc_service_integration_test
        rpc/rpc_service_integration_test.cpp
    )
    target_sources(rpc_service_integration_test PRIVATE
        ${THEMIS_ROOT_DIR}/src/server/rpc/rpc_service_impl.cpp
    )
    target_link_libraries(rpc_service_integration_test PRIVATE
        # Add gRPC dependencies
        # gRPC::grpc++
    )
    set_tests_properties(rpc_service_integration_test PROPERTIES
        LABELS "integration;rpc;grpc"
    )
endif()

# ============================================================================
# Security Integration Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/encryption_key_rotation_integration_test.cpp")
    add_integration_test(
        encryption_key_rotation_integration_test
        security/encryption_key_rotation_integration_test.cpp
    )
    target_link_libraries(encryption_key_rotation_integration_test PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    set_tests_properties(encryption_key_rotation_integration_test PROPERTIES
        LABELS "integration;security;encryption"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/zero_trust_access_control_integration_test.cpp")
    add_integration_test(
        zero_trust_access_control_integration_test
        security/zero_trust_access_control_integration_test.cpp
    )
    target_link_libraries(zero_trust_access_control_integration_test PRIVATE
        themis_core
    )
    set_tests_properties(zero_trust_access_control_integration_test PROPERTIES
        LABELS "integration;security;zero-trust"
    )
endif()

# ============================================================================
# End-to-End Integration Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/end_to_end/full_query_flow_e2e_test.cpp")
    add_integration_test(
        full_query_flow_e2e_test
        end_to_end/full_query_flow_e2e_test.cpp
    )
    set_tests_properties(full_query_flow_e2e_test PROPERTIES
        LABELS "integration;e2e;query"
        TIMEOUT 600  # 10 minutes for complex E2E tests
    )
endif()

# ============================================================================
# Existing Integration Test (keep original)
# ============================================================================

if(THEMIS_ENABLE_PROCESS_MINING_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_process_mining_e2e.cpp")
    add_integration_test(
        test_process_mining_e2e
        test_process_mining_e2e.cpp
    )
    set_tests_properties(test_process_mining_e2e PROPERTIES
        LABELS "integration;process_mining;e2e"
    )
endif()

# ============================================================================
# Integration Test Summary
# ============================================================================

message(STATUS "Integration tests configured in: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  - Test infrastructure: test_fixture.h, test_data_generator.h")
message(STATUS "  - To enable tests: uncomment sections in CMakeLists.txt as implementations complete")
message(STATUS "  - Run with: ctest -R integration")
