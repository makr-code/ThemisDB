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
        LABELS "integration;pipeline_integration;pipeline;query;release_critical;resilience;deterministic"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/ingestion_pipeline_test.cpp")
    add_integration_test(
        ingestion_pipeline_test
        pipeline/ingestion_pipeline_test.cpp
    )
    set_tests_properties(ingestion_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;ingestion;release_critical;resilience;degradation"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/rag_ai_pipeline_test.cpp")
    add_integration_test(
        rag_ai_pipeline_test
        pipeline/rag_ai_pipeline_test.cpp
    )
    set_tests_properties(rag_ai_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;rag;release_critical;degradation;deterministic"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/transaction_replication_pipeline_test.cpp")
    add_integration_test(
        transaction_replication_pipeline_test
        pipeline/transaction_replication_pipeline_test.cpp
    )
    set_tests_properties(transaction_replication_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;transaction;replication;release_critical;resilience;degradation"
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/security_pipeline_test.cpp")
    add_integration_test(
        security_pipeline_test
        pipeline/security_pipeline_test.cpp
    )
    set_tests_properties(security_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;security;release_critical;resilience"
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
        LABELS "integration;pipeline_integration;pipeline;e2e;application_profile;release_critical;resilience;degradation;deterministic"
    )
endif()

# ============================================================================
# Wave 2 Cross-Module Pipeline Tests (W2-A / W2-B)
# ============================================================================

# W2-A + W2-B: Ingest → Index → Query composed cross-module flow (CMX-01..CMX-06)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/cross_module_ingest_index_query_test.cpp")
    add_integration_test(
        cross_module_ingest_index_query_test
        pipeline/cross_module_ingest_index_query_test.cpp
    )
    set_tests_properties(cross_module_ingest_index_query_test PROPERTIES
        LABELS "integration;pipeline_integration;cross_module;wave2;ingest;query"
    )
endif()

# W2-B: Negative / recovery cross-module paths (REC-01..REC-07)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/cross_module_recovery_pipeline_test.cpp")
    add_integration_test(
        cross_module_recovery_pipeline_test
        pipeline/cross_module_recovery_pipeline_test.cpp
    )
    set_tests_properties(cross_module_recovery_pipeline_test PROPERTIES
        LABELS "integration;pipeline_integration;cross_module;wave2;recovery"
    )
endif()

# ============================================================================
# Wave 3 Integration Tests
# ============================================================================

# W3-A: Full-Function Critical Flows (FFW-01..FFW-08)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w3a_full_function_critical_flows_test.cpp")
    add_integration_test(
        w3a_full_function_critical_flows_test
        pipeline/w3a_full_function_critical_flows_test.cpp
    )
    set_tests_properties(w3a_full_function_critical_flows_test PROPERTIES
        LABELS "integration;pipeline_integration;wave3;w3a;e2e"
    )
endif()

# W3-B: Data/State Integrity & Recovery (DIR-01..DIR-08)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w3b_data_integrity_recovery_test.cpp")
    add_integration_test(
        w3b_data_integrity_recovery_test
        pipeline/w3b_data_integrity_recovery_test.cpp
    )
    set_tests_properties(w3b_data_integrity_recovery_test PROPERTIES
        LABELS "integration;pipeline_integration;wave3;w3b;recovery"
    )
endif()

# ============================================================================
# Wave 6 — Release-Candidate Hardening, Stress/Soak, Failure Injection
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w6a_critical_journey_hardening_test.cpp")
    add_integration_test(
        w6a_critical_journey_hardening_test
        pipeline/w6a_critical_journey_hardening_test.cpp
    )
    set_tests_properties(w6a_critical_journey_hardening_test PROPERTIES
        LABELS "integration;pipeline_integration;wave6;w6a;release_candidate"
        TIMEOUT 120
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w6b_stress_soak_stability_test.cpp")
    add_integration_test(
        w6b_stress_soak_stability_test
        pipeline/w6b_stress_soak_stability_test.cpp
    )
    set_tests_properties(w6b_stress_soak_stability_test PROPERTIES
        LABELS "integration;pipeline_integration;wave6;w6b;stress_soak"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w6c_failure_injection_recovery_test.cpp")
    add_integration_test(
        w6c_failure_injection_recovery_test
        pipeline/w6c_failure_injection_recovery_test.cpp
    )
    set_tests_properties(w6c_failure_injection_recovery_test PROPERTIES
        LABELS "integration;pipeline_integration;wave6;w6c;failure_injection"
        TIMEOUT 120
    )
endif()

# ============================================================================
# Wave 5 Integration Tests
# W5-A: Production-Critical E2E Journey Coverage (E2E-01..E2E-08)
# W5-B: Failure Injection & Recovery Validation  (FIR-01..FIR-08)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w5a_e2e_critical_journeys_test.cpp")
    add_integration_test(
        w5a_e2e_critical_journeys_test
        pipeline/w5a_e2e_critical_journeys_test.cpp
    )
    set_tests_properties(w5a_e2e_critical_journeys_test PROPERTIES
        LABELS "integration;pipeline_integration;wave5;w5a;release_critical;e2e"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w5b_failure_injection_recovery_test.cpp")
    add_integration_test(
        w5b_failure_injection_recovery_test
        pipeline/w5b_failure_injection_recovery_test.cpp
    )
    set_tests_properties(w5b_failure_injection_recovery_test PROPERTIES
        LABELS "integration;pipeline_integration;wave5;w5b;release_critical;recovery"
        TIMEOUT 300
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
        LABELS "integration;pipeline_integration;pipeline;wave7;w7a;release_critical;final_signoff"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w7b_recovery_resilience_test.cpp")
    add_integration_test(
        w7b_recovery_resilience_test
        pipeline/w7b_recovery_resilience_test.cpp
    )
    set_tests_properties(w7b_recovery_resilience_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave7;w7b;release_critical;recovery_resilience"
        TIMEOUT 300
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w7c_endurance_stability_test.cpp")
    add_integration_test(
        w7c_endurance_stability_test
        pipeline/w7c_endurance_stability_test.cpp
    )
    set_tests_properties(w7c_endurance_stability_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave7;w7c;endurance;stability_cert"
        TIMEOUT 600
    )
endif()

# ============================================================================
# Wave 8 — Post-Release Reliability Hardening
# W8-A: Incident Regression Shielding         (IRS-01..IRS-08)
# W8-B: Contract & Compatibility Reliability  (CCR-01..CCR-08)
# W8-C: Determinism, Flake Burn-down & CI     (DFQ-01..DFQ-08)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w8a_incident_regression_shielding_test.cpp")
    add_integration_test(
        w8a_incident_regression_shielding_test
        pipeline/w8a_incident_regression_shielding_test.cpp
    )
    set_tests_properties(w8a_incident_regression_shielding_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave8;w8a;release_critical;incident_regression"
        TIMEOUT 120
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w8b_contract_compatibility_reliability_test.cpp")
    add_integration_test(
        w8b_contract_compatibility_reliability_test
        pipeline/w8b_contract_compatibility_reliability_test.cpp
    )
    set_tests_properties(w8b_contract_compatibility_reliability_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave8;w8b;release_critical;contract_compat"
        TIMEOUT 120
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w8c_determinism_flake_ci_signal_test.cpp")
    add_integration_test(
        w8c_determinism_flake_ci_signal_test
        pipeline/w8c_determinism_flake_ci_signal_test.cpp
    )
    set_tests_properties(w8c_determinism_flake_ci_signal_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave8;w8c;release_critical;determinism;flake_burndown;ci_signal"
        TIMEOUT 120
    )
endif()

# ============================================================================
# Wave 9 — Security Hardening, SLA Compliance, Chaos Fault Tolerance
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w9a_security_hardening_audit_test.cpp")
    add_integration_test(
        w9a_security_hardening_audit_test
        pipeline/w9a_security_hardening_audit_test.cpp
    )
    set_tests_properties(w9a_security_hardening_audit_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave9;w9a;release_critical;security_hardening"
        TIMEOUT 120
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w9b_sla_compliance_uptime_test.cpp")
    add_integration_test(
        w9b_sla_compliance_uptime_test
        pipeline/w9b_sla_compliance_uptime_test.cpp
    )
    set_tests_properties(w9b_sla_compliance_uptime_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave9;w9b;release_critical;sla_compliance"
        TIMEOUT 120
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline/w9c_chaos_fault_tolerance_test.cpp")
    add_integration_test(
        w9c_chaos_fault_tolerance_test
        pipeline/w9c_chaos_fault_tolerance_test.cpp
    )
    set_tests_properties(w9c_chaos_fault_tolerance_test PROPERTIES
        LABELS "integration;pipeline_integration;pipeline;wave9;w9c;release_critical;chaos_fault_tolerance"
        TIMEOUT 120
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

# ============================================================================
# Phase 5: Voice Module Endurance and Stress Testing
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/voice/test_voice_endurance_stress.cpp")
    add_integration_test(
        test_voice_endurance_stress
        voice/test_voice_endurance_stress.cpp
    )
    target_link_libraries(test_voice_endurance_stress PRIVATE
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )
    set_tests_properties(test_voice_endurance_stress PROPERTIES
        LABELS "integration;voice;endurance;stress;phase5"
        TIMEOUT 600
    )
    message(STATUS "  Voice Phase 5.6: Endurance and stress testing (multi-session load, resource growth, memory leaks)")
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
# Phase 3 Optimization Tests (P3-03 / P3-04)
# ============================================================================

# P3-03: Resource Pooling — Connection/Thread/Buffer Manager Tests (36 tests)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_resource_pooling.cpp")
    add_integration_test(
        test_resource_pooling
        test_resource_pooling.cpp
    )
    target_link_libraries(test_resource_pooling PRIVATE
        themis_core
        Threads::Threads
    )
    set_tests_properties(test_resource_pooling PROPERTIES
        LABELS "integration;phase3;p3-03;resource_pooling;performance"
        TIMEOUT 300
    )
endif()

# P3-04: Load Balancing & Query Scheduling Tests (24 tests)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_load_balancing.cpp")
    add_integration_test(
        test_load_balancing
        test_load_balancing.cpp
    )
    target_link_libraries(test_load_balancing PRIVATE
        themis_core
        Threads::Threads
    )
    set_tests_properties(test_load_balancing PROPERTIES
        LABELS "integration;phase3;p3-04;load_balancing;scheduling;performance"
        TIMEOUT 300
    )
endif()

# ----------------------------------------------------------------------------
# Block C — P5-S01: Wire Protocol Retry/Backoff Hardening (16 tests)
# ----------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wire_protocol_retry.cpp")
    add_integration_test(
        test_wire_protocol_retry
        test_wire_protocol_retry.cpp
    )
    target_link_libraries(test_wire_protocol_retry PRIVATE
        themis_core
        Threads::Threads
        Boost::system
    )
    set_tests_properties(test_wire_protocol_retry PROPERTIES
        LABELS "integration;phase5;p5-s01;server_hardening;retry_backoff"
        TIMEOUT 120
    )
endif()

# Block C — P5-S02: HTTP Server Timeout + Graceful Shutdown Hardening (16 tests)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_http_server_hardening.cpp")
    add_integration_test(
        test_http_server_hardening
        test_http_server_hardening.cpp
    )
    target_link_libraries(test_http_server_hardening PRIVATE
        themis_core
        Threads::Threads
        spdlog::spdlog
    )
    set_tests_properties(test_http_server_hardening PROPERTIES
        LABELS "integration;phase5;p5-s02;server_hardening;graceful_shutdown"
        TIMEOUT 120
    )
endif()

# ============================================================================
# Integration Test Summary
# ============================================================================

message(STATUS "Integration tests configured in: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  - Test infrastructure: test_fixture.h, test_data_generator.h")
message(STATUS "  - Pipeline suite label: pipeline_integration")
message(STATUS "  - Release-critical label: release_critical")
message(STATUS "  - To enable tests: uncomment sections in CMakeLists.txt as implementations complete")
message(STATUS "  - Run with: ctest -R integration")

# ---------------------------------------------------------------------------
# Register all built integration test executables labelled release_critical
# into the global property consumed by the themis_release_critical_tests
# aggregate target defined in the parent tests/CMakeLists.txt.
# ---------------------------------------------------------------------------
foreach(_rc_integration_target IN ITEMS
    query_execution_pipeline_test
    ingestion_pipeline_test
    rag_ai_pipeline_test
    transaction_replication_pipeline_test
    security_pipeline_test
    application_profile_pipeline_test
    w5a_e2e_critical_journeys_test
    w5b_failure_injection_recovery_test
    w7a_final_journey_signoff_test
    w7b_recovery_resilience_test
    w8a_incident_regression_shielding_test
    w8b_contract_compatibility_reliability_test
    w8c_determinism_flake_ci_signal_test
    w9a_security_hardening_audit_test
    w9b_sla_compliance_uptime_test
    w9c_chaos_fault_tolerance_test
)
    if(TARGET ${_rc_integration_target})
        set_property(GLOBAL APPEND PROPERTY THEMIS_RELEASE_CRITICAL_TARGETS
            ${_rc_integration_target})
    endif()
endforeach()

# ============================================================================
# Wave D Soak Tests
# Labels: wave_d;soak;not_release_critical
# These tests are excluded from the fast release_critical gate.
# Run via: ctest -L "wave_d" --timeout 7200
# ============================================================================
foreach(_soak_test IN ITEMS
    test_replication_soak_60min
    test_sharding_distributed_write_soak
    test_telemetry_soak
)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_soak_test}.cpp")
        add_integration_test(${_soak_test} ${_soak_test}.cpp)
        set_tests_properties(${_soak_test} PROPERTIES
            LABELS "wave_d;soak;not_release_critical"
            TIMEOUT 7200
        )
    endif()
endforeach()
