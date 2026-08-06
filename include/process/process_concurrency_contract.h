// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file process_concurrency_contract.h
 * @brief Explicit concurrency and thread-safety contracts for process module operations.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * Defines thread-safety guarantees, atomicity scopes, and conflict resolution
 * semantics for process model CRUD, import/export, linking, and retrieval
 * operations under concurrent access and high model churn.
 *
 * @section thread_safety_model Thread-Safety Model
 *
 * The process module uses a **snapshot isolation with versioning** model:
 * - Serializers are stateless and fully thread-safe (no locks required)
 * - Model Manager uses snapshot isolation via version clocks
 * - Linker employs fine-grained link-level locking
 * - Retriever is read-only and inherits thread-safety from underlying storage
 *
 * @section concurrency_guarantees Concurrency Guarantees by Layer
 *
 * | Layer                 | Pattern              | Thread-Safe | Atomicity | Notes |
 * |-----------------------|----------------------|-------------|-----------|-------|
 * | BpmnSerializer        | Stateless            | Yes         | N/A       | No shared state |
 * | CmmnSerializer        | Stateless            | Yes         | N/A       | No shared state |
 * | ProcessModelManager   | Snapshot isolation   | Yes         | Single model op | Version clocks |
 * | ProcessLinker         | Fine-grained locks   | Yes         | Single link op | Link-level locking |
 * | ProcessLightRetriever | Read-only snapshots  | Yes         | N/A       | Snapshot isolation |
 *
 * @section conflict_resolution Conflict Resolution
 * - **Write-Write Conflicts:** Last-Write-Wins (LWW) using monotonic version clocks
 * - **Link Staleness:** Detected at read-time; broken links reported via diagnostics
 * - **No cascading deletes:** Manual remediation required
 * - **Rollback:** Manual retry with latest version; no automatic rollback
 *
 * @section high_churn_behavior High-Churn Scenario Guarantees
 *
 * **Definition:** High churn = >100 model updates/sec or >1000 concurrent links/sec
 *
 * **Expected Performance:**
 * - Model serialization: 5-50 ms per model (independent of churn)
 * - Link creation: 1-10 ms per link (scales with contention)
 * - Conflict probability: 5-15% under >500 concurrent operations (LWW resolves)
 * - **No silent failures:** All conflicts explicitly reported
 *
 * @section deadlock_prevention Deadlock Prevention
 * - Linker uses consistent lock ordering (instance_id → link_id)
 * - Serializers hold no locks (stateless)
 * - Model Manager avoids nested locking
 *
 * @section use_example Usage Example
 *
 * @code{.cpp}
 * // Concurrent reads from snapshot (thread-safe, no locks)
 * auto model = mgr.retrieve(model_id, version);  // Snapshot isolation
 *
 * // Concurrent updates (LWW resolution)
 * auto result1 = mgr.update(model_id, payload1);  // Writer 1
 * auto result2 = mgr.update(model_id, payload2);  // Writer 2 wins (LWW)
 *
 * // Concurrent link operations (fine-grained locking)
 * linker.attachDocument(instance_id, doc_id);     // Thread 1
 * linker.attachDocument(instance_id, doc_id2);    // Thread 2
 * // Both succeed; contention serialized at link level
 *
 * // Concurrent retrieval (read-only, fully thread-safe)
 * auto ctx1 = retriever.retrieve(instance_id);    // Thread 1
 * auto ctx2 = retriever.retrieve(instance_id);    // Thread 2
 * // Both complete without locks; return consistent snapshots
 * @endcode
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.x; breaking changes require major version bump
 * and deprecation path.
 */

#include <cstdint>
#include <string_view>

namespace themis::process {

/**
 * @brief Concurrency pattern for a process module layer.
 */
enum class ConcurrencyPattern : int32_t {
    /// No shared mutable state; fully thread-safe without locks
    STATELESS = 5700,
    /// Snapshot isolation with version clocks; readers do not block writers
    SNAPSHOT_ISOLATION = 5701,
    /// Fine-grained per-entity locking; independent entities do not block each other
    FINE_GRAINED_LOCKING = 5702,
    /// Read-only; inherits thread-safety from underlying storage
    READ_ONLY_SNAPSHOT = 5703,
};

/**
 * @brief Atomicity scope for a concurrency operation.
 */
enum class AtomicityScope : int32_t {
    /// Single entity operation (model, link, or instance)
    SINGLE_ENTITY = 5710,
    /// Group of independent entities (no ordering guarantees between them)
    ENTITY_GROUP = 5711,
    /// No atomicity guarantees; read-only operation
    READ_ONLY = 5712,
};

/**
 * @brief Conflict resolution strategy for concurrent writes.
 */
enum class ConflictResolution : int32_t {
    /// Last write wins; determined by monotonic version clock
    LAST_WRITE_WINS = 5720,
    /// Conflict reported to caller; retry required
    EXPLICIT_CONFLICT = 5721,
    /// No conflicts possible; stateless or read-only operation
    NO_CONFLICTS = 5722,
};

/**
 * @brief Thread-safety guarantee for BpmnSerializer.
 *
 * **Pattern:** STATELESS (no shared mutable state)  
 * **Atomicity:** N/A  
 * **Conflict Resolution:** NO_CONFLICTS
 *
 * Multiple threads may call serialize()/deserialize() concurrently without
 * coordination. Serializer maintains no per-instance state.
 *
 * @invariant Thread-safe for any number of concurrent callers.
 * @invariant No locking required.
 * @invariant Each invocation is independent.
 */
struct BpmnSerializerConcurrencyContract {
    static constexpr ConcurrencyPattern pattern = ConcurrencyPattern::STATELESS;
    static constexpr AtomicityScope atomicity = AtomicityScope::READ_ONLY;
    static constexpr ConflictResolution conflict_resolution = ConflictResolution::NO_CONFLICTS;

    /**
     * @brief Human-readable description of this contract.
     */
    static std::string_view describe() {
        return "BPMN serializer is stateless; fully thread-safe without coordination.";
    }
};

/**
 * @brief Thread-safety guarantee for CmmnSerializer.
 *
 * **Pattern:** STATELESS  
 * **Atomicity:** N/A  
 * **Conflict Resolution:** NO_CONFLICTS
 *
 * Same guarantees as BpmnSerializer.
 */
struct CmmnSerializerConcurrencyContract {
    static constexpr ConcurrencyPattern pattern = ConcurrencyPattern::STATELESS;
    static constexpr AtomicityScope atomicity = AtomicityScope::READ_ONLY;
    static constexpr ConflictResolution conflict_resolution = ConflictResolution::NO_CONFLICTS;

    static std::string_view describe() {
        return "CMMN serializer is stateless; fully thread-safe without coordination.";
    }
};

/**
 * @brief Thread-safety guarantee for ProcessModelManager.
 *
 * **Pattern:** SNAPSHOT_ISOLATION  
 * **Atomicity:** SINGLE_ENTITY (one model operation at a time)  
 * **Conflict Resolution:** LAST_WRITE_WINS (version clock ordering)
 *
 * Multiple threads may call create/retrieve/update/delete on different models
 * concurrently. Concurrent updates to the **same model** use LWW conflict
 * resolution with monotonic version clocks.
 *
 * @invariant Concurrent retrieve() calls return consistent snapshots.
 * @invariant Concurrent update() calls are serialized via version clock.
 * @invariant Version numbers form a total order (no ties).
 * @invariant No write-write transactions or rollback.
 */
struct ProcessModelManagerConcurrencyContract {
    static constexpr ConcurrencyPattern pattern = ConcurrencyPattern::SNAPSHOT_ISOLATION;
    static constexpr AtomicityScope atomicity = AtomicityScope::SINGLE_ENTITY;
    static constexpr ConflictResolution conflict_resolution = ConflictResolution::LAST_WRITE_WINS;

    static std::string_view describe() {
        return "Model Manager uses snapshot isolation with version clocks. "
               "Concurrent reads do not block writes. Write-write conflicts resolved via LWW.";
    }
};

/**
 * @brief Thread-safety guarantee for ProcessLinker.
 *
 * **Pattern:** FINE_GRAINED_LOCKING  
 * **Atomicity:** SINGLE_ENTITY (one link at a time)  
 * **Conflict Resolution:** LAST_WRITE_WINS (per-link version clock)
 *
 * Linker uses per-link locking, allowing concurrent link operations on
 * disjoint instances to proceed without blocking. Link-to-self updates
 * are atomic.
 *
 * @invariant Concurrent link operations on disjoint instances do not block.
 * @invariant Per-link locking prevents write-write conflicts.
 * @invariant Broken links (stale references) detected at read-time.
 * @invariant No cascading deletes; manual remediation required.
 */
struct ProcessLinkerConcurrencyContract {
    static constexpr ConcurrencyPattern pattern = ConcurrencyPattern::FINE_GRAINED_LOCKING;
    static constexpr AtomicityScope atomicity = AtomicityScope::SINGLE_ENTITY;
    static constexpr ConflictResolution conflict_resolution = ConflictResolution::LAST_WRITE_WINS;

    static std::string_view describe() {
        return "Linker uses fine-grained per-link locking. "
               "Concurrent operations on disjoint instances proceed without blocking.";
    }
};

/**
 * @brief Thread-safety guarantee for ProcessLightRetriever.
 *
 * **Pattern:** READ_ONLY_SNAPSHOT  
 * **Atomicity:** READ_ONLY  
 * **Conflict Resolution:** NO_CONFLICTS
 *
 * Retriever is a read-only façade over the graph layer. All queries operate
 * over consistent snapshots. No mutations or locks.
 *
 * @invariant All operations are read-only; no state mutation.
 * @invariant No locks required; snapshot-based reads.
 * @invariant Thread-safe for unlimited concurrent queries.
 */
struct ProcessLightRetrieverConcurrencyContract {
    static constexpr ConcurrencyPattern pattern = ConcurrencyPattern::READ_ONLY_SNAPSHOT;
    static constexpr AtomicityScope atomicity = AtomicityScope::READ_ONLY;
    static constexpr ConflictResolution conflict_resolution = ConflictResolution::NO_CONFLICTS;

    static std::string_view describe() {
        return "Light Retriever is read-only; inherits thread-safety from underlying storage. "
               "All queries return consistent snapshots.";
    }
};

/**
 * @brief Guidelines for high-churn scenario behavior.
 *
 * **Definition:** High churn = >100 model updates/sec or >1000 concurrent links/sec
 *
 * **Expected Performance Envelope:**
 * - Model serialization: 5-50 ms (independent of churn)
 * - Link creation: 1-10 ms per link (scales with contention)
 * - Conflict probability: 5-15% under >500 concurrent operations
 * - No silent failures; all conflicts explicitly reported
 */
struct HighChurnScenarioGuidelines {
    /// Maximum acceptable probability of LWW conflicts under >500 concurrent writes
    static constexpr double max_conflict_probability = 0.15;  // 15%

    /// Minimum expected throughput for link creation (links/sec) under high churn
    static constexpr int32_t min_link_throughput = 100;

    /// Maximum acceptable model serialization latency under any churn condition (ms)
    static constexpr int32_t max_serialize_latency_ms = 50;

    /// Soft alert threshold for concurrent operations (logs warning if exceeded)
    static constexpr int32_t churn_alert_threshold = 500;
};

/**
 * @brief Validate that a concurrency pattern is compatible with a given use case.
 *
 * @param pattern The concurrency pattern to validate.
 * @param requires_atomicity Whether the use case requires atomic operations.
 * @param is_high_churn Whether the use case involves high model churn.
 * @return true if the pattern is suitable for the use case.
 */
[[nodiscard]] inline bool isPatternSuitable(
    ConcurrencyPattern pattern,
    bool requires_atomicity,
    bool is_high_churn
) {
    // Stateless and read-only patterns are always suitable
    if (pattern == ConcurrencyPattern::STATELESS ||
        pattern == ConcurrencyPattern::READ_ONLY_SNAPSHOT) {
        return true;
    }

    // Snapshot isolation and fine-grained locking support high churn
    if (is_high_churn) {
        return pattern == ConcurrencyPattern::SNAPSHOT_ISOLATION ||
               pattern == ConcurrencyPattern::FINE_GRAINED_LOCKING;
    }

    return true;
}

} // namespace themis::process
