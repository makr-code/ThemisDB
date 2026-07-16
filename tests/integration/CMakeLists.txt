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
# Cross-Module Pipeline Integration Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/query_execution_pipeline_test.cpp")
    add_integration_test(
        query_execution_pipeline_test
        pipeline/query_execution_pipeline_test.cpp
    )
    set_tests_properties(query_execution_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;query"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/ingestion_pipeline_test.cpp")
    add_integration_test(
        ingestion_pipeline_test
        pipeline/ingestion_pipeline_test.cpp
    )
    set_tests_properties(ingestion_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;ingestion"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/rag_ai_pipeline_test.cpp")
    add_integration_test(
        rag_ai_pipeline_test
        pipeline/rag_ai_pipeline_test.cpp
    )
    set_tests_properties(rag_ai_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;rag"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/transaction_replication_pipeline_test.cpp")
    add_integration_test(
        transaction_replication_pipeline_test
        pipeline/transaction_replication_pipeline_test.cpp
    )
    set_tests_properties(transaction_replication_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;transaction;replication"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/security_pipeline_test.cpp")
    add_integration_test(
        security_pipeline_test
        pipeline/security_pipeline_test.cpp
    )
    set_tests_properties(security_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;security"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/analytics_export_pipeline_test.cpp")
    add_integration_test(
        analytics_export_pipeline_test
        pipeline/analytics_export_pipeline_test.cpp
    )
    set_tests_properties(analytics_export_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;analytics;export"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/application_profile_pipeline_test.cpp")
    add_integration_test(
        application_profile_pipeline_test
        pipeline/application_profile_pipeline_test.cpp
    )
    set_tests_properties(application_profile_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;e2e;application_profile"
    )
endif()

# ============================================================================
# Wave 7 — Final Sign-off, Recovery/Resilience, Endurance/Stability
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w7a_final_journey_signoff_test.cpp")
    add_integration_test(
        w7a_final_journey_signoff_test
        pipeline/w7a_final_journey_signoff_test.cpp
    )
    set_tests_properties(w7a_final_journey_signoff_test PROPERTIES
        LABELS "integration;pipeline;wave7;w7a;release_critical;final_signoff"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w7b_recovery_resilience_test.cpp")
    add_integration_test(
        w7b_recovery_resilience_test
        pipeline/w7b_recovery_resilience_test.cpp
    )
    set_tests_properties(w7b_recovery_resilience_test PROPERTIES
        LABELS "integration;pipeline;wave7;w7b;release_critical;recovery_resilience"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w7c_endurance_stability_test.cpp")
    add_integration_test(
        w7c_endurance_stability_test
        pipeline/w7c_endurance_stability_test.cpp
    )
    set_tests_properties(w7c_endurance_stability_test PROPERTIES
        LABELS "integration;pipeline;wave7;w7c;endurance;stability_cert"
        TIMEOUT 600
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
# Focused integration test registrations migrated from root tests/CMakeLists
# ============================================================================

foreach(_src_name
        test_content_processing
        test_cross_functional_plugin_query_metrics
        test_distributed_training_e2e
        test_graphql_e2e
        test_rpc_database_operations
)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_src_name}.cpp")
        add_integration_test(
            ${_src_name}_focused
            ${_src_name}.cpp
        )
        set_tests_properties(${_src_name}_focused PROPERTIES
            LABELS "integration;focused;unit"
            TIMEOUT 300
        )
    endif()
endforeach()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_cross_functional_voice_observability.cpp")
    add_integration_test(
        test_cross_functional_voice_observability_focused
        test_cross_functional_voice_observability.cpp
    )
    set_tests_properties(test_cross_functional_voice_observability_focused PROPERTIES
        LABELS "integration;voice;observability;focused"
        TIMEOUT 300
    )
endif()

if(THEMIS_ENABLE_PROCESS_MINING_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_process_mining_e2e.cpp")
    add_integration_test(
        test_process_mining_e2e_focused
        test_process_mining_e2e.cpp
    )
    set_tests_properties(test_process_mining_e2e_focused PROPERTIES
        LABELS "integration;process-mining;e2e;focused"
        TIMEOUT 600
    )
endif()

# ============================================================================
# Integration Test Summary
# ============================================================================

message(STATUS "Integration tests configured in: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  - Test infrastructure: test_fixture.h, test_data_generator.h")
message(STATUS "  - Pipeline suite label: pipeline_integration")
message(STATUS "  - To enable tests: uncomment sections in CMakeLists.txt as implementations complete")
message(STATUS "  - Run with: ctest -R integration")
