/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_saga.h                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:21:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     339                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • ffeccdf868  2026-03-01  feat(transaction): implement DistributedSagaCoordinator f... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <future>
#include <unordered_map>
#include <vector>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Result type (mirrors TransactionManager::Status for consistency)
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedSagaStatus {
    bool ok = true;
    std::string message;

    static DistributedSagaStatus OK()    { return {}; }
    static DistributedSagaStatus Error(std::string msg) {
        return {false, std::move(msg)};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-SAGA execution state
// ─────────────────────────────────────────────────────────────────────────────

enum class SagaExecutionState {
    PENDING,       ///< Not yet started
    RUNNING,       ///< Steps in progress
    COMPLETED,     ///< All steps succeeded and committed
    COMPENSATING,  ///< At least one step failed; compensations running
    COMPENSATED,   ///< All compensations executed (SAGA rolled back)
    FAILED         ///< Unrecoverable failure (compensation also failed)
};

// ─────────────────────────────────────────────────────────────────────────────
// Step definition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single step in a distributed SAGA.
 *
 * Each step describes:
 *   - Which remote node (node_id) executes it (informational; the caller
 *     encodes the node routing inside the forward/compensate callables).
 *   - A forward action that performs the step's work.
 *   - A compensating action that undoes the step if the SAGA must roll back.
 *   - Optional dependencies on other steps (for DAG-based parallelism).
 *   - Per-step retry and timeout configuration.
 */
struct DistributedSagaStep {
    using Action = std::function<DistributedSagaStatus()>;

    /// Unique name of the step within a SAGA definition.
    std::string name;

    /// Identifier of the node / service that owns this step (informational).
    std::string node_id;

    /// Forward action: executes the step's work.
    Action forward;

    /// Compensating action: undoes the step on rollback.
    /// May be empty (no-op compensation) if the step is idempotent / irreversible.
    Action compensate;

    /// Names of steps that must complete before this step can start.
    std::set<std::string> depends_on;

    /// Per-step timeout for the forward action.
    std::chrono::milliseconds forward_timeout{5000};

    /// Per-step timeout for the compensating action.
    std::chrono::milliseconds compensate_timeout{10000};

    /// Maximum number of retry attempts on transient failure (0 = no retry).
    size_t max_retries{3};

    /// Initial backoff delay between retries; doubled on each attempt (capped at 30 s).
    std::chrono::milliseconds retry_backoff{std::chrono::milliseconds(100)};
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA definition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A complete distributed SAGA definition.
 *
 * A SAGA definition groups a set of steps together under a single identifier.
 * Steps may declare dependencies on one another; the coordinator uses a
 * topological sort to determine execution order and runs dependency-free
 * groups in parallel.
 */
struct DistributedSagaDefinition {
    /// Globally unique identifier for this SAGA instance.
    std::string saga_id;

    /// Steps to execute (may reference each other via depends_on).
    std::vector<DistributedSagaStep> steps;

    /// Arbitrary key-value context shared across steps (passed read-only to actions
    /// via closure captures; the coordinator does not inspect or modify this map).
    std::map<std::string, std::string> context;
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-step runtime record (used for status reporting and recovery)
// ─────────────────────────────────────────────────────────────────────────────

struct StepRecord {
    enum class Phase { PENDING, EXECUTING, DONE, COMPENSATING, COMPENSATED, FAILED };

    std::string              name;
    Phase                    phase{Phase::PENDING};
    std::string              error_message;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point finished_at;
    size_t                   attempts{0};       ///< Forward action attempts
    size_t                   comp_attempts{0};  ///< Compensation attempts
};

// ─────────────────────────────────────────────────────────────────────────────
// Execution report returned from execute()
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedSagaReport {
    std::string          saga_id;
    SagaExecutionState   state{SagaExecutionState::PENDING};
    std::string          failure_reason;     ///< Set when state != COMPLETED
    std::vector<StepRecord> step_records;    ///< One entry per step
    int64_t              total_duration_ms{0};

    /// True only when all steps ran and committed successfully.
    [[nodiscard]] bool succeeded() const {
        return state == SagaExecutionState::COMPLETED;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator configuration (defined outside the class so it can be used as a
// default argument in the constructor declaration)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for DistributedSagaCoordinator.
 */
struct DistributedSagaCoordinatorConfig {
    /// Enable parallel execution of independent steps (default: true).
    bool enable_parallel{true};

    /// Path to the SAGA journal file for durable state tracking.
    /// Leave empty to disable persistence.
    std::string journal_path;

    /// Global SAGA timeout (0 = no global timeout).
    std::chrono::milliseconds saga_timeout{std::chrono::milliseconds(0)};

    /// Default step forward timeout (overridden by DistributedSagaStep::forward_timeout).
    std::chrono::milliseconds default_forward_timeout{std::chrono::milliseconds(5000)};

    /// Default step compensate timeout.
    std::chrono::milliseconds default_compensate_timeout{std::chrono::milliseconds(10000)};
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates distributed SAGA transactions across multiple nodes.
 *
 * The coordinator:
 *   1. Validates the SAGA definition (no cycles, all dependencies exist).
 *   2. Executes steps in dependency order (topological sort), running
 *      independent steps in parallel when enable_parallel is true.
 *   3. On any step failure, triggers compensation in reverse execution order.
 *   4. Retries transient failures up to the per-step max_retries limit.
 *   5. Enforces per-step timeouts via std::async with wait_for.
 *   6. Optionally persists SAGA state to a journal file for crash recovery.
 *
 * Thread safety: a single coordinator instance may execute multiple independent
 * SAGAs concurrently (each execute() call is independently synchronized).
 *
 * @note Individual SAGA steps must not capture raw pointers that may be
 *       destroyed while the SAGA is still running.
 */
class DistributedSagaCoordinator {
public:
    /**
     * @brief Configuration for the coordinator.
     *
     * Defined outside the class (as DistributedSagaCoordinatorConfig) and
     * aliased here for ergonomic usage.
     */
    using Config = DistributedSagaCoordinatorConfig;

    explicit DistributedSagaCoordinator(Config config = {});
    ~DistributedSagaCoordinator() = default;

    // Non-copyable, movable
    DistributedSagaCoordinator(const DistributedSagaCoordinator&)            = delete;
    DistributedSagaCoordinator& operator=(const DistributedSagaCoordinator&) = delete;
    DistributedSagaCoordinator(DistributedSagaCoordinator&&)                 = default;
    DistributedSagaCoordinator& operator=(DistributedSagaCoordinator&&)      = default;

    // ── Core API ──────────────────────────────────────────────────────────────

    /**
     * @brief Validate and execute a distributed SAGA.
     *
     * Blocks until the SAGA either completes successfully or finishes
     * compensating after a failure.  Returns a detailed execution report.
     *
     * @param saga  SAGA definition to execute.
     * @return      Execution report with final state and per-step records.
     */
    DistributedSagaReport execute(const DistributedSagaDefinition& saga);

    /**
     * @brief Validate a SAGA definition without executing it.
     *
     * Checks that all depends_on names exist and that there are no dependency
     * cycles.
     *
     * @return OK() on success, Error(...) with description on failure.
     */
    DistributedSagaStatus validate(const DistributedSagaDefinition& saga) const;

    // ── Status / Metrics ─────────────────────────────────────────────────────

    /**
     * @brief Retrieve the execution report for a previously executed SAGA.
     *
     * @return The report, or std::nullopt if the saga_id is unknown.
     */
    std::optional<DistributedSagaReport> getReport(const std::string& saga_id) const;

    /**
     * @brief Return the current aggregate metrics.
     */
    struct Metrics {
        uint64_t sagas_started{0};
        uint64_t sagas_completed{0};
        uint64_t sagas_compensated{0};
        uint64_t sagas_failed{0};
        uint64_t total_step_executions{0};
        uint64_t total_compensations{0};
        uint64_t total_step_retries{0};
        uint64_t total_timeout_aborts{0};
    };

    Metrics getMetrics() const;

private:
    Config config_;

    mutable std::mutex reports_mutex_;
    std::map<std::string, DistributedSagaReport> reports_;

    // Aggregate metrics (updated atomically)
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Topologically sort steps; returns empty vector on cycle detection.
    std::vector<std::string> topologicalSort(
        const DistributedSagaDefinition& saga
    ) const;

    // Record index type for O(1) step-record lookup
    using RecordIndex = std::unordered_map<std::string, StepRecord*>;

    /// Execute all steps in a dependency wave (parallel if enabled).
    DistributedSagaStatus executeWave(
        const std::vector<std::string>&                    wave,
        const std::map<std::string, DistributedSagaStep>&  step_map,
        RecordIndex&                                       index,
        std::string&                                       failure_reason
    );

    /// Execute a single step with retry and timeout.
    DistributedSagaStatus executeStep(
        const DistributedSagaStep& step,
        StepRecord&                record
    );

    /// Compensate all completed steps in reverse execution order.
    void compensate(
        const std::map<std::string, DistributedSagaStep>& step_map,
        const std::vector<std::string>&                   executed_order,
        RecordIndex&                                      index
    );

    /// Compensate a single step with retry and timeout.
    DistributedSagaStatus compensateStep(
        const DistributedSagaStep& step,
        StepRecord&                record
    );

    /// Append a JSON entry to the journal file (no-op when journal_path is empty).
    void journalWrite(const std::string& saga_id, const std::string& event,
                      const std::string& detail = {});
};

} // namespace themis
