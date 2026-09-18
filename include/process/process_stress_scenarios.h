// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file process_stress_scenarios.h
 * @brief Definitions of stress scenarios for parser, linker, and retrieval edge testing.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * Defines the set of edge-case scenarios that the process module must handle
 * gracefully under stress conditions. Used to drive Phase 2 hardening and
 * stress testing.
 *
 * @section scenario_classes Scenario Classes
 *
 * Stress scenarios are organized into three classes:
 * 1. **Parser Edge Scenarios** – Deep nesting, large models, malformed input
 * 2. **Linker Edge Scenarios** – Orphaned links, circular references, bulk creation
 * 3. **Retrieval Edge Scenarios** – Empty graphs, large contexts, timeouts
 *
 * Each scenario defines:
 * - **Input:** Concrete test input specification
 * - **Expected Behavior:** Deterministic output or error code
 * - **Trigger:** When to activate (e.g., condition that triggers the scenario)
 * - **Success Criteria:** Metrics to verify (latency, no deadlock, no silent failure)
 *
 * @section usage Usage in Tests
 *
 * @code{.cpp}
 * // Stress test: Deep nesting
 * auto scenario = ProcessStressScenarios::DeepNestingStress{};
 * auto model = scenario.generateTestInput();  // Generate nested BPMN
 * auto result = parser.deserialize(model);
 *
 * if (scenario.isSuccess(result)) {
 *     log_success("Deep nesting stress: PASS (latency=" + result.latency_ms + "ms)");
 * } else if (scenario.isFatalFailure(result)) {
 *     log_failure("Deep nesting stress: FATAL FAILURE");
 *     FAIL();
 * } else {
 *     log_expected_error("Deep nesting stress: " + toString(result.error_code));
 * }
 * @endcode
 *
 * @section benchmark_expectations Benchmark Expectations
 *
 * All stress scenarios have associated latency envelopes (P95/P99) that should
 * be validated against release baselines (see PERFORMANCE_EXPECTATIONS.md).
 */

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis::process {

/**
 * @brief Severity level for stress scenario expectations.
 */
enum class StressScenarioSeverity : int32_t {
    /// Scenario should complete successfully (e.g., large model parsing)
    SHOULD_SUCCEED = 5900,
    /// Scenario should fail with deterministic error (e.g., depth exceeded)
    SHOULD_FAIL_GRACEFULLY = 5901,
    /// Scenario is a performance boundary (latency may be at envelope limit)
    PERFORMANCE_BOUNDARY = 5902,
};

/**
 * @brief Result of a stress scenario test.
 *
 * Used to classify whether a scenario completed successfully, failed gracefully,
 * or encountered a fatal/silent failure.
 */
struct StressScenarioResult {
    /// Did the scenario complete as expected (success or graceful failure)?
    bool is_expected;
    /// Did a fatal or silent failure occur?
    bool is_fatal;
    /// Latency in milliseconds
    int32_t latency_ms;
    /// Optional error code if failure occurred
    std::optional<int32_t> error_code;
    /// Optional diagnostic message
    std::optional<std::string> message;

    /**
     * @brief Check if this result is a fatal failure.
     * @return true if result indicates unrecoverable failure or silent data loss
     */
    [[nodiscard]] bool isFatal() const { return is_fatal; }

    /**
     * @brief Check if this result is within performance envelope.
     * @param max_latency_ms Maximum acceptable latency
     * @return true if latency_ms <= max_latency_ms
     */
    [[nodiscard]] bool isWithinPerformanceEnvelope(int32_t max_latency_ms) const {
        return latency_ms <= max_latency_ms;
    }
};

/**
 * @brief Deep nesting stress scenario for BPMN parser.
 *
 * **Input:** Process model with nested sub-processes up to maximum depth  
 * **Maximum Depth:** 100 levels (configurable)  
 * **Expected Behavior:** Success with deterministic output (should complete in <50 ms)  
 * **Trigger:** `PROC_MAX_DEPTH_EXCEEDED` if max depth exceeded  
 * **Success Criteria:**
 * - Deserialization succeeds
 * - Output model is deterministic (same as non-nested version)
 * - Latency <50 ms (P95)
 * - No stack overflow or memory corruption
 */
struct DeepNestingStress {
    /// Maximum nesting depth for this scenario (default: 100)
    int32_t max_depth = 100;

    /// Expected latency ceiling (ms) at P95
    int32_t expected_latency_p95_ms = 50;

    /// Generate a test input with nested sub-processes
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    /// Get scenario description
    [[nodiscard]] std::string_view describe() const {
        return "Deep nesting stress: Process model with 100 levels of nested sub-processes";
    }
};

/**
 * @brief Large element count stress scenario for BPMN parser.
 *
 * **Input:** Process model with >10,000 nodes, gateways, and transitions  
 * **Element Count:** 10,000+ (configurable)  
 * **Expected Behavior:** Success in <500 ms; or graceful failure with error code  
 * **Trigger:** `PROC_MAX_ELEMENTS_EXCEEDED` if element count exceeded  
 * **Success Criteria:**
 * - Deserialization succeeds or fails with explicit error
 * - Latency <500 ms (P95)
 * - No incomplete or corrupted model state
 * - Memory freed on failure
 */
struct LargeElementCountStress {
    /// Element count for this scenario (default: 10,000)
    int32_t element_count = 10000;

    /// Expected latency ceiling (ms) at P95
    int32_t expected_latency_p95_ms = 500;

    /// Generate a test input with large element count
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Large element count stress: Process model with 10,000+ elements";
    }
};

/**
 * @brief Malformed XML recovery scenario for BPMN parser.
 *
 * **Input:** BPMN with missing required attributes, broken nesting, invalid schema  
 * **Varieties:** Missing required attributes, unclosed tags, invalid attribute values  
 * **Expected Behavior:** Explicit error with actionable message (not silent skip)  
 * **Trigger:** `PROC_DESERIALISE_FAILED` with remediation hint  
 * **Success Criteria:**
 * - Error code explicitly returned
 * - Error message is actionable (suggests fix)
 * - No partial/corrupted model state
 * - Latency <100 ms (P95)
 */
struct MalformedXmlRecoveryStress {
    /// Generate a test input with specific malformation type
    [[nodiscard]] std::string generateTestInputMissingAttribute() const;
    [[nodiscard]] std::string generateTestInputBrokenNesting() const;
    [[nodiscard]] std::string generateTestInputInvalidAttribute() const;

    /// Check if result is success (graceful failure with error) for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Malformed XML recovery: BPMN with missing attributes, broken nesting";
    }
};

/**
 * @brief Unsupported gateway detection scenario for BPMN parser.
 *
 * **Input:** BPMN with unsupported gateway types (COMPLEX_AND, COMPLEX_OR, etc.)  
 * **Expected Behavior:** Explicit error before import; no silent accept  
 * **Trigger:** `PROC_UNSUPPORTED_ELEMENT` with element details  
 * **Success Criteria:**
 * - Error code explicitly returned
 * - Unsupported element name is in error message
 * - No import of partially supported model
 * - Latency <50 ms (P95)
 */
struct UnsupportedGatewayStress {
    /// Generate a test input with unsupported gateway
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (explicit error) for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Unsupported gateway detection: BPMN with COMPLEX_AND/COMPLEX_OR gateways";
    }
};

/**
 * @brief Orphaned link resolution scenario for process linker.
 *
 * **Input:** Links that reference deleted or non-existent models/instances  
 * **Expected Behavior:** Links persist but marked as stale; detected at read-time  
 * **Trigger:** `RETRIEVAL_INCIDENT` on read  
 * **Success Criteria:**
 * - Link is not corrupted or deleted
 * - Staleness is detected at read-time
 * - Diagnostic message explains link is stale
 * - Operator can manually repair or delete
 */
struct OrphanedLinkResolutionStress {
    /// Generate a test input with orphaned links
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (link stale but detectable) for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Orphaned link resolution: Links to deleted models detected at read-time";
    }
};

/**
 * @brief Circular reference detection scenario for process linker.
 *
 * **Input:** Processes that trigger each other in a cycle (A→B→A)  
 * **Expected Behavior:** Allowed and detected; documented as non-deterministic  
 * **Trigger:** Warning log; no error code  
 * **Success Criteria:**
 * - Links are created successfully (no error)
 * - Circular reference is logged as warning
 * - No deadlock or infinite loop
 * - Subprocess execution order documented as non-deterministic
 */
struct CircularReferenceDetectionStress {
    /// Generate a test input with circular references
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (no deadlock, warning logged)
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Circular reference detection: Processes triggering each other in a cycle";
    }
};

/**
 * @brief Bulk link creation scenario for process linker.
 *
 * **Input:** 10,000+ links added concurrently  
 * **Link Count:** 10,000+  
 * **Expected Behavior:** Latency 1-10 ms per link; no deadlocks  
 * **Trigger:** `PROC_MAX_CONTEXT_EXCEEDED` if context size exceeds limit  
 * **Success Criteria:**
 * - All links created successfully (or explicit error)
 * - Throughput 100+ links/sec
 * - No deadlocks or race conditions
 * - Latency scales linearly with link count
 */
struct BulkLinkCreationStress {
    /// Link count for this scenario (default: 10,000)
    int32_t link_count = 10000;

    /// Expected latency per link (ms)
    int32_t expected_latency_per_link_ms = 5;

    /// Generate a test input with bulk links
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Bulk link creation stress: 10,000+ links added concurrently";
    }
};

/**
 * @brief Link attribute mutation under churn scenario for process linker.
 *
 * **Input:** Concurrent updates to same link metadata  
 * **Expected Behavior:** Last-Write-Wins conflict resolution  
 * **Trigger:** Implicit LWW; winner determined by version clock  
 * **Success Criteria:**
 * - All updates resolve without deadlock
 * - Final state is consistent with one update (no merge)
 * - Version numbers form total order
 * - Conflict count is deterministic given same timing
 */
struct LinkAttributeMutationStress {
    /// Generate a test input with concurrent mutations
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (LWW resolution applied)
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Link attribute mutation: Concurrent updates to same link metadata";
    }
};

/**
 * @brief Empty graph query scenario for process retriever.
 *
 * **Input:** Retrieve from model with no instances  
 * **Expected Behavior:** Return empty context gracefully  
 * **Trigger:** No error; empty result set  
 * **Success Criteria:**
 * - Query completes successfully
 * - Return value is empty (no crash on empty)
 * - Latency <10 ms (P95)
 * - No silent failures
 */
struct EmptyGraphQueryStress {
    /// Generate a test input (empty model)
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Empty graph query: Retrieve from model with no instances";
    }
};

/**
 * @brief Large context size scenario for process retriever.
 *
 * **Input:** Graph traversal yields >1 MB context  
 * **Expected Behavior:** Truncate context and log warning  
 * **Trigger:** `PROC_MAX_CONTEXT_EXCEEDED` with truncation marker  
 * **Success Criteria:**
 * - Query completes (no OOM crash)
 * - Context is truncated cleanly
 * - Truncation marker in result
 * - Latency <500 ms (P95)
 */
struct LargeContextSizeStress {
    /// Context size threshold (bytes)
    int32_t context_size_limit = 1000000;  // 1 MB

    /// Generate a test input with large graph
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (truncated gracefully)
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Large context size: Graph traversal yields >1 MB; truncation expected";
    }
};

/**
 * @brief Community detection timeout scenario for process retriever.
 *
 * **Input:** Large graph (>100k edges) times out during community detection  
 * **Expected Behavior:** Fallback to LOCAL retrieval  
 * **Trigger:** `PROC_EXECUTION_TIMEOUT` with fallback logging  
 * **Success Criteria:**
 * - Query completes (no hanging)
 * - Fallback to LOCAL is logged
 * - Result is still usable
 * - Latency <1000 ms (P95)
 */
struct CommunityDetectionTimeoutStress {
    /// Edge count for large graph
    int32_t edge_count = 100000;

    /// Timeout threshold (ms)
    int32_t timeout_ms = 5000;

    /// Generate a test input with large graph
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success (fallback applied)
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Community detection timeout: Large graph times out; fallback expected";
    }
};

/**
 * @brief Concurrent query churn scenario for process retriever.
 *
 * **Input:** 100+ concurrent retrieval queries  
 * **Query Count:** 100+  
 * **Expected Behavior:** All queries complete within deadline; no silent failures  
 * **Trigger:** Tail latency monitored (P99 < 1000 ms)  
 * **Success Criteria:**
 * - All queries complete successfully
 * - No deadlocks or resource exhaustion
 * - P95 latency <500 ms, P99 <1000 ms
 * - No silent failures or corrupted results
 */
struct ConcurrentQueryChurnStress {
    /// Number of concurrent queries
    int32_t query_count = 100;

    /// Expected P95 latency (ms)
    int32_t expected_latency_p95_ms = 500;

    /// Expected P99 latency (ms)
    int32_t expected_latency_p99_ms = 1000;

    /// Generate a test input with concurrent queries
    [[nodiscard]] std::string generateTestInput() const;

    /// Check if result is success for this scenario
    [[nodiscard]] bool isSuccess(const StressScenarioResult& result) const;

    [[nodiscard]] std::string_view describe() const {
        return "Concurrent query churn: 100+ queries executed concurrently";
    }
};

/**
 * @brief Container for all stress scenarios.
 *
 * Provides centralized access to all defined scenarios for test automation.
 */
struct ProcessStressScenarios {
    // Parser scenarios
    static const DeepNestingStress& deepNesting();
    static const LargeElementCountStress& largeElementCount();
    static const MalformedXmlRecoveryStress& malformedXmlRecovery();
    static const UnsupportedGatewayStress& unsupportedGateway();

    // Linker scenarios
    static const OrphanedLinkResolutionStress& orphanedLinkResolution();
    static const CircularReferenceDetectionStress& circularReferenceDetection();
    static const BulkLinkCreationStress& bulkLinkCreation();
    static const LinkAttributeMutationStress& linkAttributeMutation();

    // Retriever scenarios
    static const EmptyGraphQueryStress& emptyGraphQuery();
    static const LargeContextSizeStress& largeContextSize();
    static const CommunityDetectionTimeoutStress& communityDetectionTimeout();
    static const ConcurrentQueryChurnStress& concurrentQueryChurn();

    /// Count of all defined scenarios
    static int32_t totalScenarioCount() { return 12; }

    /// Get scenario by index (0-based)
    [[nodiscard]] static std::string_view getScenarioName(int32_t index);
};

} // namespace themis::process
