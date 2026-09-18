// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file process_determinism_spec.h
 * @brief Determinism and consistency guarantees for process module operations.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * Specifies which process module behaviors are deterministic and which are
 * non-deterministic. Enables operators to understand repeatability guarantees
 * and expected behavior under concurrent updates and model conflicts.
 *
 * @section determinism_model Determinism Model
 *
 * The process module distinguishes between:
 * - **Deterministic:** Identical input produces identical output every time
 * - **Non-deterministic:** Output may vary due to timing, concurrency, or algorithm design
 * - **Conflict-resolved:** Non-deterministic, but conflict resolution is deterministic
 *
 * @section deterministic_operations Deterministic Operations
 *
 * | Operation | Input | Output | Condition | Notes |
 * |-----------|-------|--------|-----------|-------|
 * | BPMN parse | BPMN source | Process graph | UUID v5 with stable namespace | Fully deterministic |
 * | State transition | Instance + event | New state | Valid state machine | BPMN 2.0 semantics |
 * | Model retrieve | Model ID + version | Snapshot | Version exists | Snapshot isolation |
 * | Link retrieval | Instance ID + link type | Link list | Consistent snapshot | Per-snapshot deterministic |
 * | Community detect | Graph | Communities | Algorithm deterministic | Same input → same output |
 * | Serialization | Model | XML/JSON | Canonical format | Deterministic codec |
 *
 * @section nondeterministic_operations Non-Deterministic Operations
 *
 * | Operation | Reason | Mitigation |
 * |-----------|--------|-----------|
 * | Subprocess execution order | Thread scheduling | Documented as non-deterministic |
 * | LLM context generation | LLM stochasticity | Use fixed seed if reproducibility required |
 * | Query AUTO routing | Heuristic keyword match | Deterministic routing; docs specify heuristic |
 * | Community order | Graph iteration order | Deterministic per snapshot; order not stable across versions |
 *
 * @section conflict_resolution_determinism Conflict Resolution Determinism
 *
 * When concurrent updates conflict:
 * - **Resolution mechanism:** Last-Write-Wins (LWW) using monotonic version clocks
 * - **Deterministic outcome:** Unique winner determined by version clock (no ties)
 * - **Repeatable behavior:** Same concurrent operations always resolve the same way
 * - **Observable conflict:** Error code PROC_LINKING_FAILED or implicit LWW (configurable)
 *
 * @section rollback_semantics Rollback Semantics
 *
 * **No automatic rollback.** The process module does not support nested transactions
 * or automatic rollback on error. Manual remediation required:
 *
 * 1. **Link creation fails (LINKING_INCIDENT):**
 *    - Link not created; orphaned link references cleaned up by manual repair
 *    - Retry: Inspect latest model state and retry with fresh link payload
 *
 * 2. **Model update conflicts (VALIDATION_INCIDENT):**
 *    - Conflict detected via version clock mismatch
 *    - Retry: Re-fetch model at latest version and retry update
 *
 * 3. **Retrieval inconsistency:**
 *    - Snapshot captured at consistent instant; no inconsistency expected
 *    - If staleness observed: Re-query or reindex
 *
 * @section use_example Usage Example
 *
 * @code{.cpp}
 * // Deterministic parsing: identical BPMN → identical model
 * auto model1 = parser.deserialize(bpmn_source);  // Parse 1
 * auto model2 = parser.deserialize(bpmn_source);  // Parse 2
 * assert(model1.id == model2.id);                 // Same UUID v5
 * assert(model1.nodes == model2.nodes);           // Identical structure
 *
 * // Non-deterministic: subprocess execution order may vary
 * // But: final state is still valid per BPMN 2.0
 *
 * // Conflict resolution is deterministic (LWW)
 * auto v1 = mgr.update(model_id, payload1, version=100);  // Thread 1, ts=t1
 * auto v2 = mgr.update(model_id, payload2, version=100);  // Thread 2, ts=t2
 * // If t2 > t1: Thread 2 wins; payload2 is in model at version 101
 * // Outcome is deterministic; always same thread wins given same timing
 * @endcode
 *
 * @section contract_freeze Contract Freeze
 * This specification is frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string_view>

namespace themis::process {

/**
 * @brief Determinism classification for a process operation.
 */
enum class DeterminismClass : int32_t {
    /// Identical input always produces identical output
    FULLY_DETERMINISTIC = 5800,
    /// Output varies due to timing, scheduling, or algorithm design (but is valid)
    NON_DETERMINISTIC = 5801,
    /// Conflict resolution is deterministic; outcome determined by version clock
    CONFLICT_RESOLVED_DETERMINISTIC = 5802,
    /// Read-only operation; inherits determinism from snapshot isolation
    SNAPSHOT_DETERMINISTIC = 5803,
};

/**
 * @brief Scope of conflict resolution in a model update.
 */
enum class ConflictScope : int32_t {
    /// Single model operation; no cross-model conflicts
    SINGLE_MODEL = 5810,
    /// Entire model namespace; version clock governs all updates
    MODEL_NAMESPACE = 5811,
    /// No conflicts possible; read-only or stateless operation
    NO_CONFLICTS = 5812,
};

/**
 * @brief Guarantees for BPMN model parsing and import.
 *
 * **Determinism:** FULLY_DETERMINISTIC  
 * **Condition:** UUID generation uses RFC 4122 v5 with stable namespace  
 * **Output:** Identical BPMN source produces identical model structure
 *
 * @invariant Multiple parses of the same BPMN produce identical model IDs
 * @invariant Node and link ordering within model is stable
 * @invariant No random element names or ordering
 * @invariant Parse tree traversal is deterministic
 */
struct BpmnParsingDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::FULLY_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::NO_CONFLICTS;

    /// UUID v5 namespace for stable model ID generation
    static std::string_view uuid_namespace() {
        return "themis-bpmn-parser";
    }

    /// Expected behavior when parsing the same BPMN twice
    static std::string_view describe() {
        return "BPMN parsing is fully deterministic. "
               "Identical source produces identical model structure and ID.";
    }
};

/**
 * @brief Guarantees for CMMN model parsing and import.
 *
 * **Determinism:** FULLY_DETERMINISTIC  
 * **Condition:** UUID generation uses RFC 4122 v5 with stable namespace  
 *
 * Same guarantees as BPMN parsing.
 */
struct CmmnParsingDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::FULLY_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::NO_CONFLICTS;

    static std::string_view uuid_namespace() {
        return "themis-cmmn-parser";
    }

    static std::string_view describe() {
        return "CMMN parsing is fully deterministic. "
               "Identical source produces identical model structure and ID.";
    }
};

/**
 * @brief Guarantees for process instance state transitions.
 *
 * **Determinism:** CONFLICT_RESOLVED_DETERMINISTIC  
 * **Condition:** Single instance state machine; no cross-instance state changes  
 * **Ordering:** Deterministic within a single instance; no races on state fields
 *
 * @invariant State transitions follow BPMN 2.0 token semantics
 * @invariant Valid transitions are deterministic
 * @invariant Invalid transitions always produce same error
 * @invariant Subprocess invocation order is non-deterministic (see NOTE below)
 *
 * @note **Non-deterministic aspect:** Subprocess execution order may vary under
 *       concurrent execution due to thread scheduling. However, final state is
 *       still valid per BPMN 2.0 (may represent different execution path orders).
 */
struct StateTransitionDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::CONFLICT_RESOLVED_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::SINGLE_MODEL;

    static std::string_view describe() {
        return "State transitions within a single instance are deterministic. "
               "Transition outcome determined by current state and incoming event. "
               "Subprocess execution order may vary (non-deterministic).";
    }
};

/**
 * @brief Guarantees for concurrent model updates under churn.
 *
 * **Determinism:** CONFLICT_RESOLVED_DETERMINISTIC  
 * **Mechanism:** Last-Write-Wins with monotonic version clocks  
 * **Ordering:** Version numbers form a total order; no ties
 *
 * When two threads concurrently update the same model:
 * 1. Both updates are stamped with unique version clocks
 * 2. Higher version number wins (LWW)
 * 3. Loser either retries or receives conflict error
 * 4. Outcome is deterministic: same updates with same timing always produce same winner
 *
 * @invariant Version clocks are monotonically increasing
 * @invariant No two updates have same version number
 * @invariant Winner is deterministic given concurrent operation timestamps
 * @invariant Loser receives explicit error (VALIDATION_INCIDENT or implicit LWW)
 */
struct ModelUpdateDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::CONFLICT_RESOLVED_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::MODEL_NAMESPACE;

    /// Expected conflict probability under >500 concurrent updates to same model
    static constexpr double expected_conflict_probability = 0.10;  // 10%

    static std::string_view describe() {
        return "Model updates use Last-Write-Wins conflict resolution. "
               "Winner determined by version clock; outcome is deterministic. "
               "Loser must retry with latest version.";
    }
};

/**
 * @brief Guarantees for link consistency under model deletion and churn.
 *
 * **Determinism:** SNAPSHOT_DETERMINISTIC  
 * **Staleness Detection:** Read-time validation for broken references  
 * **No Cascading Deletes:** Manual cleanup required
 *
 * When a model is deleted:
 * - Existing links to that model become stale (not corrupted)
 * - Stale links are detected when read and reported via RETRIEVAL_INCIDENT
 * - Operator must manually repair or delete stale links
 * - No automatic cascading delete (prevents accidental data loss)
 *
 * @invariant Links persists even if target model is deleted
 * @invariant Stale links are detectable at read-time
 * @invariant No silent data loss or corruption
 * @invariant Manual repair required; no automatic rollback
 */
struct LinkConsistencyDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::SNAPSHOT_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::NO_CONFLICTS;

    static std::string_view describe() {
        return "Links to deleted models become stale but are not corrupted. "
               "Staleness detected at read-time. Manual cleanup required; "
               "no automatic cascading deletes.";
    }
};

/**
 * @brief Guarantees for retrieval and graph traversal consistency.
 *
 * **Determinism:** SNAPSHOT_DETERMINISTIC  
 * **Isolation:** All queries operate over a consistent snapshot at a single instant  
 * **Ordering:** Graph traversal order is deterministic per snapshot
 *
 * Queries return consistent snapshots captured at a single instant in time.
 * Concurrent queries may see different snapshots (if model changes between queries),
 * but each snapshot is internally consistent.
 *
 * @invariant All edges and nodes in a query result are from the same snapshot instant
 * @invariant Graph traversal (BFS, PPR) order is deterministic for a given snapshot
 * @invariant Repeated queries with identical input yield identical results (same snapshot)
 * @invariant Newer queries may see updates from model changes (different snapshot)
 *
 * @note **Non-deterministic aspect:** Community detection algorithm is deterministic,
 *       but community ordering may vary across model versions (different graph structure).
 */
struct RetrievalConsistencyDeterminismSpec {
    static constexpr DeterminismClass determinism = DeterminismClass::SNAPSHOT_DETERMINISTIC;
    static constexpr ConflictScope conflict_scope = ConflictScope::NO_CONFLICTS;

    static std::string_view describe() {
        return "Retrieval queries operate over consistent snapshots. "
               "Graph traversal is deterministic per snapshot. "
               "Concurrent queries may see different snapshots.";
    }
};

/**
 * @brief Guidelines for reproducing process behavior in testing.
 *
 * Use these guidelines to ensure test cases are reproducible and deterministic.
 */
struct ReproducibilityGuidelines {
    /// Use fixed UUIDs (RFC 4122 v5) for models to ensure consistent IDs across test runs
    static std::string_view use_fixed_uuids_hint() {
        return "BPMN parsing uses RFC 4122 v5 for model IDs. "
               "Use fixed namespace in tests for reproducibility.";
    }

    /// Avoid dependency on subprocess execution order in test assertions
    static std::string_view avoid_subprocess_ordering_hint() {
        return "Subprocess execution order is non-deterministic. "
               "Test only final state, not execution order.";
    }

    /// Use version numbers to ensure model consistency in multi-threaded tests
    static std::string_view use_version_clocks_hint() {
        return "Model updates use version clocks for conflict resolution. "
               "Verify version numbers in assertions, not timestamps.";
    }

    /// Capture snapshots at known points to ensure consistent retrieval
    static std::string_view capture_snapshots_at_known_points_hint() {
        return "Retrieval is snapshot-based. Capture snapshot versions "
               "before retrieval to ensure reproducible results.";
    }
};

/**
 * @brief Validate determinism classification for a use case.
 *
 * @param required_determinism The required determinism level.
 * @param actual_determinism The actual determinism level provided by operation.
 * @return true if actual determinism satisfies requirement.
 */
[[nodiscard]] inline bool isDeterminismSufficient(
    DeterminismClass required_determinism,
    DeterminismClass actual_determinism
) {
    // Fully deterministic satisfies all requirements
    if (actual_determinism == DeterminismClass::FULLY_DETERMINISTIC) {
        return true;
    }

    // Conflict-resolved deterministic satisfies requirements >= conflict-resolved
    if (required_determinism <= DeterminismClass::CONFLICT_RESOLVED_DETERMINISTIC &&
        actual_determinism >= DeterminismClass::CONFLICT_RESOLVED_DETERMINISTIC) {
        return true;
    }

    // Snapshot-deterministic sufficient for snapshot-based use cases
    if (required_determinism == DeterminismClass::SNAPSHOT_DETERMINISTIC &&
        actual_determinism == DeterminismClass::SNAPSHOT_DETERMINISTIC) {
        return true;
    }

    return false;
}

} // namespace themis::process
