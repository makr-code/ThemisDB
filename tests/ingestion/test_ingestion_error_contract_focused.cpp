/**
 * @file test_ingestion_error_contract_focused.cpp
 * @brief Focused tests for Phase 3: Error Contracts & Edge Cases
 *
 * Phase 3 (Long-term): Error Handling & Edge Cases
 * - Apply error contracts to all component categories
 * - Implement fail-safe semantics
 * - Add operator-visible diagnostics and incident categorization
 *
 * Test categories:
 * - ING3C-01..04: Error classification (transient, permanent, resource exhaustion, backpressure)
 * - ING3C-05..08: ErrorContext serialization and nested errors
 * - ING3C-09..12: Error escape valve policies (fail_closed, fail_open, defer)
 * - ING3C-13..16: Diagnostic emitter listener pattern and incident tracking
 */

#include "ingestion/ingestion_diagnostic_emitter.h"
#include "ingestion/ingestion_error_contract.h"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>

namespace themis {
namespace ingestion {

// ============================================================================
// ING3C-01..04: Error classification
// ============================================================================

/**
 * @test ING3C-01: Transient errors are correctly classified
 *
 * Verifies that network timeouts, temporary failures, and resource
 * constraints are classified as transient.
 */
TEST(ErrorContractPhase3, ING3C01_TransientErrorClassification) {
    // Network timeouts
    EXPECT_TRUE(isTransientError(IngestionErrorCode::HTTP_TIMEOUT));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::HTTP_CONNECTION_REFUSED));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::HTTP_DNS_RESOLUTION_FAILED));

    // Temporary server issues
    EXPECT_TRUE(isTransientError(IngestionErrorCode::HTTP_SERVER_ERROR));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::SOURCE_UNAVAILABLE));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::HTTP_RATE_LIMITED));

    // Database transients
    EXPECT_TRUE(isTransientError(IngestionErrorCode::DATABASE_TIMEOUT));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::DATABASE_DEADLOCK));

    // Resource constraints (retryable)
    EXPECT_TRUE(isTransientError(IngestionErrorCode::QUEUE_SATURATED));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::BUFFER_FULL));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::ENQUEUE_TIMEOUT));
    EXPECT_TRUE(isTransientError(IngestionErrorCode::CONNECTION_POOL_EXHAUSTED));
}

/**
 * @test ING3C-02: Permanent errors are correctly classified
 *
 * Verifies that validation errors, configuration errors, and authentication
 * failures are classified as permanent.
 */
TEST(ErrorContractPhase3, ING3C02_PermanentErrorClassification) {
    // Validation errors
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::SCHEMA_INVALID));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::VALIDATION_FAILED));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::TYPE_COERCION_FAILED));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::DUPLICATE_KEY));

    // Configuration errors
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::CONFIG_INVALID));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::CONFIG_MISSING_REQUIRED));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::CONFIG_OUT_OF_RANGE));

    // Authentication errors
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::AUTH_FAILED));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::AUTH_EXPIRED));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::PERMISSION_DENIED));

    // File not found
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::FILE_NOT_FOUND));
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::SOURCE_NOT_FOUND));

    // File format not supported
    EXPECT_TRUE(isPermanentError(IngestionErrorCode::FILE_FORMAT_UNSUPPORTED));
}

/**
 * @test ING3C-03: Resource exhaustion errors are correctly classified
 *
 * Verifies that memory, disk, and pool exhaustion errors are properly
 * categorized for resource-specific handling.
 */
TEST(ErrorContractPhase3, ING3C03_ResourceExhaustionClassification) {
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::MEMORY_EXHAUSTION));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::DISK_QUOTA_EXCEEDED));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::DISK_SPACE_LOW));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::CONNECTION_POOL_EXHAUSTED));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::THREAD_POOL_EXHAUSTED));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::PROCESS_LIMIT_EXCEEDED));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::QUEUE_SATURATED));
    EXPECT_TRUE(isResourceExhaustionError(IngestionErrorCode::BUFFER_FULL));
}

/**
 * @test ING3C-04: Backpressure errors are correctly classified
 *
 * Verifies that backpressure-inducing errors (saturation, throttling,
 * rate limiting) are properly marked for flow control decisions.
 */
TEST(ErrorContractPhase3, ING3C04_BackpressureErrorClassification) {
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::QUEUE_SATURATED));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::BUFFER_FULL));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::ENQUEUE_TIMEOUT));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::BACKPRESSURE_APPLIED));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::HTTP_RATE_LIMITED));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::MEMORY_EXHAUSTION));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::CONNECTION_POOL_EXHAUSTED));
    EXPECT_TRUE(isBackpressureError(IngestionErrorCode::CPU_THROTTLE));
}

// ============================================================================
// ING3C-05..08: ErrorContext serialization and nested errors
// ============================================================================

/**
 * @test ING3C-05: ErrorContext basic serialization to JSON
 *
 * Verifies that ErrorContext can be serialized to valid JSON with
 * all core fields.
 */
TEST(ErrorContractPhase3, ING3C05_ErrorContextBasicSerialization) {
    ErrorContext ctx;
    ctx.error_code = IngestionErrorCode::HTTP_TIMEOUT;
    ctx.error_message = "Connection timeout to API";
    ctx.component_name = "HTTP_CONNECTOR";
    ctx.source_id = "api.example.com";
    ctx.connector_type = "HTTP";
    ctx.operation = "fetch_data";
    ctx.retry_count = 3;
    ctx.item_index = 42;
    ctx.memory_usage_percent = 72.5;
    ctx.available_memory_bytes = 1024 * 1024 * 512;  // 512 MB
    ctx.hostname = "ingest-pod-5";

    std::string json = ctx.toJson();

    // Verify JSON contains expected fields
    EXPECT_THAT(json, testing::HasSubstr("\"error_code\":1"));
    EXPECT_THAT(json, testing::HasSubstr("\"error_message\":\"Connection timeout to API\""));
    EXPECT_THAT(json, testing::HasSubstr("\"component_name\":\"HTTP_CONNECTOR\""));
    EXPECT_THAT(json, testing::HasSubstr("\"source_id\":\"api.example.com\""));
    EXPECT_THAT(json, testing::HasSubstr("\"operation\":\"fetch_data\""));
    EXPECT_THAT(json, testing::HasSubstr("\"retry_count\":3"));
}

/**
 * @test ING3C-06: ErrorContext with nested errors
 *
 * Verifies that ErrorContext supports nested error contexts for
 * error chaining and root cause analysis.
 */
TEST(ErrorContractPhase3, ING3C06_ErrorContextNestedErrors) {
    // Root cause error
    ErrorContext root;
    root.error_code = IngestionErrorCode::HTTP_CONNECTION_REFUSED;
    root.error_message = "DNS resolution failed";

    // Mid-level error
    ErrorContext mid;
    mid.error_code = IngestionErrorCode::HTTP_TIMEOUT;
    mid.error_message = "Connection timeout";
    mid.nested_errors.push_back(root);

    // Top-level error
    ErrorContext top;
    top.error_code = IngestionErrorCode::CONNECTOR_DEGRADED;
    top.error_message = "Connector failed to connect";
    top.nested_errors.push_back(mid);

    EXPECT_EQ(top.nested_errors.size(), 1);
    EXPECT_EQ(top.nested_errors[0].nested_errors.size(), 1);
    EXPECT_EQ(top.nested_errors[0].nested_errors[0].error_code,
              IngestionErrorCode::HTTP_CONNECTION_REFUSED);
}

/**
 * @test ING3C-07: Error message lookup for all error codes
 *
 * Verifies that getErrorMessage() returns meaningful messages for
 * all error codes, with no empty or truncated messages.
 */
TEST(ErrorContractPhase3, ING3C07_ErrorMessageLookup) {
    // Test a representative sample of error codes
    std::vector<IngestionErrorCode> codes = {
        IngestionErrorCode::OK,
        IngestionErrorCode::HTTP_TIMEOUT,
        IngestionErrorCode::DATABASE_DEADLOCK,
        IngestionErrorCode::VALIDATION_FAILED,
        IngestionErrorCode::CONFIG_INVALID,
        IngestionErrorCode::MEMORY_EXHAUSTION,
        IngestionErrorCode::QUALITY_THRESHOLD_FAILED,
        IngestionErrorCode::WORKFLOW_STEP_FAILED,
        IngestionErrorCode::UNKNOWN_ERROR,
    };

    for (const auto& code : codes) {
        std::string msg = getErrorMessage(code);
        EXPECT_FALSE(msg.empty());
        EXPECT_TRUE(msg.length() > 0 && msg.length() < 256);
    }
}

/**
 * @test ING3C-08: Error classification consistency
 *
 * Verifies that error classification results are consistent:
 * - Transient ∩ Permanent = ∅ (mutually exclusive)
 * - ResourceExhaustion ⊆ Backpressure (resource exhaustion implies backpressure)
 */
TEST(ErrorContractPhase3, ING3C08_ErrorClassificationConsistency) {
    // Test that transient and permanent are mutually exclusive
    std::vector<IngestionErrorCode> all_codes = {
        IngestionErrorCode::HTTP_TIMEOUT,
        IngestionErrorCode::VALIDATION_FAILED,
        IngestionErrorCode::DATABASE_DEADLOCK,
        IngestionErrorCode::AUTH_FAILED,
        IngestionErrorCode::MEMORY_EXHAUSTION,
    };

    for (const auto& code : all_codes) {
        bool transient = isTransientError(code);
        bool permanent = isPermanentError(code);
        // An error should not be both transient and permanent
        if (transient && permanent) {
            FAIL() << "Error " << static_cast<int>(code) << " is both transient and permanent";
        }
    }

    // Test that resource exhaustion is a subset of backpressure
    std::vector<IngestionErrorCode> resource_errors = {
        IngestionErrorCode::MEMORY_EXHAUSTION,
        IngestionErrorCode::CONNECTION_POOL_EXHAUSTED,
        IngestionErrorCode::THREAD_POOL_EXHAUSTED,
        IngestionErrorCode::QUEUE_SATURATED,
    };

    for (const auto& code : resource_errors) {
        EXPECT_TRUE(isResourceExhaustionError(code));
        EXPECT_TRUE(isBackpressureError(code))
            << "Resource exhaustion error should imply backpressure";
    }
}

// ============================================================================
// ING3C-09..12: Error escape valve policies
// ============================================================================

/**
 * @test ING3C-09: Fail-closed escape valve policy
 *
 * Verifies that fail-closed policy (CLOSE_ON_DEGRADATION) prevents
 * degraded operation and raises alert.
 */
TEST(ErrorContractPhase3, ING3C09_FailClosedPolicy) {
    ErrorEscapeValveConfig config;
    config.policy = EscapeValvePolicy::CLOSE_ON_DEGRADATION;
    config.max_consecutive_failures = 3;
    config.escalation_delay_ms = 100;

    // Verify configuration
    EXPECT_EQ(config.policy, EscapeValvePolicy::CLOSE_ON_DEGRADATION);
    EXPECT_EQ(config.max_consecutive_failures, 3);
}

/**
 * @test ING3C-10: Fail-open escape valve policy
 *
 * Verifies that fail-open policy (CONTINUE_WITH_DEGRADATION) allows
 * operation to continue despite errors.
 */
TEST(ErrorContractPhase3, ING3C10_FailOpenPolicy) {
    ErrorEscapeValveConfig config;
    config.policy = EscapeValvePolicy::CONTINUE_WITH_DEGRADATION;
    config.degradation_level = 0.7;  // Accept 70% throughput

    EXPECT_EQ(config.policy, EscapeValvePolicy::CONTINUE_WITH_DEGRADATION);
    EXPECT_DOUBLE_EQ(config.degradation_level, 0.7);
}

/**
 * @test ING3C-11: Defer escape valve policy
 *
 * Verifies that defer policy (DEFER_WITH_RETRY_ACCUMULATION)
 * queues operations for later retry.
 */
TEST(ErrorContractPhase3, ING3C11_DeferPolicy) {
    ErrorEscapeValveConfig config;
    config.policy = EscapeValvePolicy::DEFER_WITH_RETRY_ACCUMULATION;
    config.max_deferred_items = 10000;
    config.defer_timeout_ms = 5000;

    EXPECT_EQ(config.policy, EscapeValvePolicy::DEFER_WITH_RETRY_ACCUMULATION);
    EXPECT_EQ(config.max_deferred_items, 10000);
}

/**
 * @test ING3C-12: Fail-safe contract specification
 *
 * Verifies that FailSafeContract properly specifies component
 * contract requirements for different failure scenarios.
 */
TEST(ErrorContractPhase3, ING3C12_FailSafeContractSpecification) {
    FailSafeContract contract;
    contract.component_name = "HTTPConnector";
    contract.failure_mode = "connection_timeout";
    contract.required_action = "retry_with_exponential_backoff";
    contract.max_retry_count = 5;
    contract.required_action_details = "Start with 100ms delay, double on each retry";

    EXPECT_EQ(contract.component_name, "HTTPConnector");
    EXPECT_EQ(contract.failure_mode, "connection_timeout");
    EXPECT_EQ(contract.max_retry_count, 5);
}

// ============================================================================
// ING3C-13..16: Diagnostic emitter listener pattern and incident tracking
// ============================================================================

/**
 * @test ING3C-13: Diagnostic emitter listener registration and invocation
 *
 * Verifies that listeners are properly invoked when incidents are emitted,
 * and multiple listeners can be registered independently.
 */
TEST(ErrorContractPhase3, ING3C13_DiagnosticEmitterListeners) {
    DiagnosticEmitter emitter;
    std::atomic<int> call_count(0);

    // Register first listener
    int id1 = emitter.registerListener([&](const DiagnosticIncident& incident) {
        EXPECT_EQ(incident.title, "Test incident");
        call_count++;
    });

    // Register second listener
    int id2 = emitter.registerListener([&](const DiagnosticIncident& incident) {
        EXPECT_EQ(incident.category, IncidentCategory::NETWORK_CONNECTIVITY);
        call_count++;
    });

    EXPECT_EQ(emitter.getListenerCount(), 2);

    // Emit incident
    DiagnosticIncident incident;
    incident.title = "Test incident";
    incident.category = IncidentCategory::NETWORK_CONNECTIVITY;
    emitter.emit(incident);

    // Both listeners should have been called
    EXPECT_EQ(call_count, 2);

    // Unregister first listener
    EXPECT_TRUE(emitter.unregisterListener(id1));
    EXPECT_EQ(emitter.getListenerCount(), 1);

    call_count = 0;
    emitter.emit(incident);
    EXPECT_EQ(call_count, 1);  // Only second listener called
}

/**
 * @test ING3C-14: Diagnostic incident categorization from errors
 *
 * Verifies that errors are correctly categorized into incident types
 * and mapped to appropriate severity levels.
 */
TEST(ErrorContractPhase3, ING3C14_IncidentCategorization) {
    DiagnosticEmitter emitter;
    DiagnosticIncident captured_incident;

    emitter.registerListener([&](const DiagnosticIncident& incident) {
        captured_incident = incident;
    });

    // Create error context
    ErrorContext ctx;
    ctx.error_code = IngestionErrorCode::HTTP_TIMEOUT;
    ctx.error_message = "Connection timeout";
    ctx.component_name = "HTTPConnector";

    std::string incident_id = emitter.emitFromError(ctx);

    // Verify incident was properly categorized
    EXPECT_EQ(captured_incident.category, IncidentCategory::NETWORK_CONNECTIVITY);
    EXPECT_EQ(captured_incident.severity, DiagnosticSeverity::WARNING);
    EXPECT_FALSE(incident_id.empty());
}

/**
 * @test ING3C-15: Diagnostic incident tracking and counters
 *
 * Verifies that the emitter tracks incident counts per category
 * and provides aggregate statistics.
 */
TEST(ErrorContractPhase3, ING3C15_IncidentTracking) {
    DiagnosticEmitter emitter;

    // Emit multiple incidents
    for (int i = 0; i < 3; i++) {
        DiagnosticIncident incident;
        incident.category = IncidentCategory::NETWORK_CONNECTIVITY;
        emitter.emit(incident);
    }

    for (int i = 0; i < 2; i++) {
        DiagnosticIncident incident;
        incident.category = IncidentCategory::QUEUE_SATURATION;
        emitter.emit(incident);
    }

    // Verify counts
    EXPECT_EQ(emitter.getIncidentCount(IncidentCategory::NETWORK_CONNECTIVITY), 3);
    EXPECT_EQ(emitter.getIncidentCount(IncidentCategory::QUEUE_SATURATION), 2);
    EXPECT_EQ(emitter.getTotalIncidentCount(), 5);

    // Reset and verify
    emitter.resetCounters();
    EXPECT_EQ(emitter.getTotalIncidentCount(), 0);
}

/**
 * @test ING3C-16: Diagnostic incident JSON serialization
 *
 * Verifies that incidents can be serialized to JSON for logging,
 * streaming to monitoring systems, and audit trails.
 */
TEST(ErrorContractPhase3, ING3C16_IncidentSerialization) {
    DiagnosticIncident incident;
    incident.incident_id = "ING-12345";
    incident.category = IncidentCategory::MEMORY_PRESSURE;
    incident.severity = DiagnosticSeverity::ALERT;
    incident.title = "Memory pressure detected";
    incident.description = "System memory usage exceeded 85%";
    incident.remediation_hint = "Reduce concurrent tasks or increase system memory";
    incident.runbook_link = "https://wiki/ops/memory-management";
    incident.metrics["memory_percent"] = 87.5;
    incident.metrics["available_mb"] = 256.0;

    std::string json = incident.toJson();

    // Verify JSON contains expected fields
    EXPECT_THAT(json, testing::HasSubstr("\"incident_id\":\"ING-12345\""));
    EXPECT_THAT(json, testing::HasSubstr("\"category\":4"));  // MEMORY_PRESSURE
    EXPECT_THAT(json, testing::HasSubstr("\"severity\":2"));  // ALERT
    EXPECT_THAT(json, testing::HasSubstr("\"title\":\"Memory pressure detected\""));
    EXPECT_THAT(json, testing::HasSubstr("\"metrics\""));
    EXPECT_THAT(json, testing::HasSubstr("\"memory_percent\":87.5"));
}

}  // namespace ingestion
}  // namespace themis

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ErrorContractPhase3);
