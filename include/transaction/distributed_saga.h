/**
 * @file distributed_saga.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <nlohmann/json.hpp>

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
    
    // Consensus tracking (QW-39)
    bool                     consensus_reached{false};  ///< Write was replicated to quorum
    int64_t                  consensus_timestamp_ms{0}; ///< When consensus was achieved
    int                      quorum_size{0};            ///< Total replicas in quorum
    int                      ack_count{0};              ///< Number of replicas acked
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
// Remote step (for multi-cluster / service-mesh orchestration)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A step executed against a remote service endpoint.
 *
 * Used by executeDistributed() to orchestrate cross-cluster SAGAs where each
 * step is an RPC/HTTP call to a distinct microservice.
 */
struct RemoteStep {
    /// HTTP/gRPC endpoint of the target service (e.g. "http://inventory:8080").
    std::string service_endpoint;

    /// Forward operation path (e.g. "/reserve").
    std::string operation;

    /// Parameters forwarded to the remote service.
    nlohmann::json params;

    /// Compensating operation path (e.g. "/release").
    std::string compensate_operation;

    /// Parameters forwarded during compensation.
    nlohmann::json compensate_params;

    /// Logical name of the step (used for dependency declarations).
    std::string name;

    /// Names of other remote steps that must complete before this one.
    std::set<std::string> depends_on;

    /// Per-step timeout for the forward call.
    std::chrono::milliseconds forward_timeout{std::chrono::milliseconds(5000)};

    /// Per-step timeout for the compensating call.
    std::chrono::milliseconds compensate_timeout{std::chrono::milliseconds(10000)};

    /// Maximum retry attempts on transient failure.
    size_t max_retries{3};

    /// Initial backoff between retries (doubled each attempt, capped at 30 s).
    std::chrono::milliseconds retry_backoff{std::chrono::milliseconds(100)};
};

// ─────────────────────────────────────────────────────────────────────────────
// Distributed SAGA definition (multi-cluster / remote-step variant)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A complete distributed SAGA expressed as a set of remote service calls.
 */
struct DistributedSAGADefinition {
    /// Globally unique identifier for this SAGA instance.
    std::string saga_id;

    /// Remote steps to execute.
    std::vector<RemoteStep> steps;

    /// Arbitrary shared context propagated to all remote calls.
    std::map<std::string, std::string> context;
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA visualization output
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result of visualize() — contains both a machine-readable DOT graph
 *        and a human-readable text summary.
 */
struct SagaVisualization {
    /// Graphviz DOT source describing the SAGA execution graph.
    std::string dot_graph;

    /// Human-readable plain-text execution summary.
    std::string text_summary;
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator configuration (defined outside the class so it can be used as a
// default argument in the constructor declaration)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Pluggable transport for executing a single remote step.
 *
 * Signature: (endpoint, operation, params) → DistributedSagaStatus
 *
 * Production deployments inject an HTTP or gRPC client here. When this
 * function is not configured, executeDistributed() now rejects execution
 * fail-closed.
 */
using RemoteStepExecutor =
    std::function<DistributedSagaStatus(
        const std::string& /*endpoint*/,
        const std::string& /*operation*/,
        const nlohmann::json& /*params*/)>;


/**
 * @brief Result payload for consensus verification of a completed step write.
 */
struct ConsensusVerificationResult {
    /// True when quorum durability has been confirmed.
    bool verified{false};

    /// Quorum size expected for this write (e.g. 2 of 3 replicas).
    int quorum_size{0};

    /// Number of replica acknowledgements observed.
    int ack_count{0};

    /// Optional diagnostic detail for logs and failure reasons.
    std::string detail;
};

/**
 * @brief Pluggable consensus verification callback.
 *
 * Signature: (step_name, node_id) -> ConsensusVerificationResult
 */
using ConsensusVerifier =
    std::function<ConsensusVerificationResult(
        const std::string& /*step_name*/,
        const std::string& /*node_id*/)>
;
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

    /// Enable distributed consensus verification for write durability (QW-39).
    /// When true, after each step succeeds locally, the coordinator verifies that
    /// the write was replicated to a quorum of replicas before declaring success.
    /// If a consensus_verifier is configured, failed verification causes retries
    /// and eventually fail-closed behavior when retries are exhausted.
    /// If no consensus_verifier is configured, a single-node fallback (1/1 ack)
    /// is applied for backward compatibility.
    bool enable_consensus_verification{true};

    /// Optional callback that validates quorum durability for a completed step.
    /// When set, this callback is authoritative for consensus decisions.
    ConsensusVerifier consensus_verifier;

    /// Pluggable transport for remote step execution.
    /// Must be configured for executeDistributed(); otherwise execution is
    /// rejected fail-closed.
    RemoteStepExecutor remote_executor;
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
    *
    * Fail-closed invariant:
    *  - Returns FAILED with `remote_executor_not_configured` when
    *    Config::remote_executor is not set.
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
        * Fail-closed invariants:
        *  - Duplicate saga_id executions are rejected.
        *  - Per-step execution is bounded by step timeout and optional global
        *    saga_timeout budget.
        *  - A step is rejected when any declared dependency is not in DONE phase.
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

    // ── Remote / Multi-cluster API ────────────────────────────────────────────

    /**
     * @brief Execute a distributed SAGA expressed as remote service calls.
     *
     * Each RemoteStep is converted to a DistributedSagaStep whose forward and
     * compensate actions invoke the pluggable remote_executor from the config.
     * The SAGA then runs through the same DAG-based execution engine as execute().
     *
     * @param saga  Remote SAGA definition.
     * @return      Execution report with final state and per-step records.
     */
    DistributedSagaReport executeDistributed(const DistributedSAGADefinition& saga);

    /**
     * @brief Query the current execution state of a distributed SAGA.
     *
     * @return Report if a SAGA with this ID is known, nullopt otherwise.
     */
    std::optional<DistributedSagaReport> getDistributedStatus(
        const std::string& saga_id) const;

    // ── Crash Recovery API ────────────────────────────────────────────────────

    /**
     * @brief Recover SAGAs that were left in RUNNING or COMPENSATING state.
     *
     * Reads the journal file (config_.journal_path) and for each SAGA that
     * has a STARTED entry but no terminal entry (COMPLETED / COMPENSATED /
     * FAILED), records a synthetic FAILED recovery report so callers can
     * inspect them and re-execute or force-compensate as needed.
     *
     * This provides the foundation for automatic crash recovery:
     * after a coordinator restart, call this method before accepting new work
     * to identify and handle orphaned SAGAs.
     *
     * @return List of saga_ids that were recovered (found in inconsistent state).
     */
    std::vector<std::string> recoverInProgressSAGAs();

    // ── Visualization & Debugging API ─────────────────────────────────────────

    /**
     * @brief Generate a visualization of a SAGA definition.
     *
     * Returns a Graphviz DOT representation of the step dependency graph plus
     * a human-readable plain-text summary.  If an execution report exists for
     * the given saga_id the nodes are annotated with their final phase.
     *
     * @param saga   SAGA definition to visualize.
     * @return       Visualization containing dot_graph and text_summary fields.
     */
    SagaVisualization visualize(const DistributedSagaDefinition& saga) const;

    // ── Manual Intervention API ───────────────────────────────────────────────

    /**
     * @brief Force a stuck SAGA into the COMPENSATED state.
     *
     * Intended for manual intervention on SAGAs that have been left in an
     * inconsistent state (e.g. after a partial coordinator crash).  Marks the
     * report for the given saga_id as COMPENSATED without executing any
     * compensation actions.  Returns false if the saga_id is not known.
     *
     * @param saga_id  ID of the SAGA to force-compensate.
     * @return         true on success, false if saga_id is unknown.
     */
    bool forceCompensate(const std::string& saga_id);

    /**
     * @brief Force a stuck SAGA into the COMPLETED state.
     *
     * Marks the report for the given saga_id as COMPLETED without executing
     * any further steps.  Returns false if the saga_id is not known.
     *
     * @param saga_id  ID of the SAGA to force-complete.
     * @return         true on success, false if saga_id is unknown.
     */
    bool forceComplete(const std::string& saga_id);

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
        uint64_t consensus_checks_total{0};
        uint64_t consensus_checks_failed{0};
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
        std::string&                                       failure_reason,
        std::optional<std::chrono::steady_clock::time_point> deadline
    );

    /// Execute a single step with retry and timeout.
    DistributedSagaStatus executeStep(
        const DistributedSagaStep& step,
        StepRecord&                record,
        std::optional<std::chrono::steady_clock::time_point> deadline
    );

    /// Verify distributed consensus for write durability (QW-39).
    /// After a step executes successfully, verify that its write was replicated
    /// to a quorum of replicas for durability guarantees.
    /// @param step_name Name of the step that executed
    /// @param node_id Node/service identifier for the write
    /// @param record Mutable step record for consensus metadata population
    /// @param failure_detail Optional textual reason when verification fails
    /// @return true if consensus verified (or verification disabled), false if unconfirmed
    bool verifyStepConsensus(
        const std::string& step_name,
        const std::string& node_id,
        StepRecord& record,
        std::string* failure_detail = nullptr
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

    /// Convert a RemoteStep into a DistributedSagaStep using the configured executor.
    DistributedSagaStep remoteStepToLocal(const RemoteStep& remote) const;
};

} // namespace themis
