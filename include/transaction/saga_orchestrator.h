/**
 * @file saga_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Result type
// ─────────────────────────────────────────────────────────────────────────────

struct SagaOrchestratorStatus {
    bool        ok{true};
    std::string message;

    static SagaOrchestratorStatus OK()                       { return {}; }
    static SagaOrchestratorStatus Error(std::string msg)     { return {false, std::move(msg)}; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-step runtime state
// ─────────────────────────────────────────────────────────────────────────────

enum class StepState {
    PENDING,       ///< Not yet started
    RUNNING,       ///< Forward action executing
    COMPLETED,     ///< Forward action succeeded
    FAILED,        ///< Forward action failed after all retries
    SKIPPED,       ///< Condition evaluated to false; step was not executed
    COMPENSATING,  ///< Compensation action executing
    COMPENSATED    ///< Compensation action succeeded
};

// ─────────────────────────────────────────────────────────────────────────────
// Step definition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single step in a SAGAOrchestrator workflow.
 *
 * Unlike DistributedSagaCoordinator (which uses DistributedSagaStatus returns),
 * SAGAOrchestrator steps use void callables and signal failure via exceptions.
 * This matches the simpler local-service orchestration use case where throwing
 * std::exception (or a derived class) is the standard error signal.
 */
struct SAGAStep {
    /// Unique name within the SAGA definition.
    std::string name;

    /// Forward action: performs the step's work.  Throws on failure.
    std::function<void()> forward;

    /// Compensating action: undoes the step on rollback.
    /// May be empty (nullptr) for idempotent / non-reversible steps.
    std::function<void()> compensate;

    /// Names of steps that must COMPLETE before this step can start.
    std::set<std::string> depends_on;

    /// Optional guard: if provided, the step is only executed when this
    /// callable returns true.  When it returns false the step is SKIPPED
    /// (no forward, no compensation).
    std::function<bool()> condition;

    /// Per-step timeout for the forward action (0 = use orchestrator default).
    std::chrono::milliseconds timeout{0};

    /// Maximum number of retry attempts on exception (0 = no retry).
    size_t max_retries{0};

    /// Initial delay between retries; doubled on each attempt (capped at 30 s).
    /// 0 = use orchestrator default (SAGAOrchestratorConfig::default_retry_delay).
    std::chrono::milliseconds retry_delay{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA definition
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A complete SAGA workflow definition.
 */
struct SAGADefinition {
    /// Unique identifier for this SAGA instance (used as key for getStatus).
    std::string id;

    /// Human-readable name.
    std::string name;

    /// Steps to execute. Steps may declare dependencies via depends_on.
    std::vector<SAGAStep> steps;

    /// Arbitrary string context shared across steps (read-only; orchestrator
    /// does not interpret this map).
    std::map<std::string, std::string> context;

    /// Enable parallel execution of independent steps (default: true).
    bool enable_parallel{true};
};

// ─────────────────────────────────────────────────────────────────────────────
// Execution status (returned by getStatus / accessible after execute)
// ─────────────────────────────────────────────────────────────────────────────

struct SAGAExecutionStatus {
    std::string                          saga_id;
    std::string                          saga_name;
    std::map<std::string, StepState>     step_states;
    size_t                               completed_steps{0};
    size_t                               failed_steps{0};
    size_t                               pending_steps{0};
    size_t                               skipped_steps{0};
    int64_t                              total_duration_ms{0};
    std::string                          failure_reason;
};

// ─────────────────────────────────────────────────────────────────────────────
// Orchestrator configuration
// ─────────────────────────────────────────────────────────────────────────────

struct SAGAOrchestratorConfig {
    /// Enable parallel execution of independent steps (default: true).
    /// Individual SAGADefinition::enable_parallel overrides this per-saga.
    bool enable_parallel{true};

    /// Default forward timeout used when SAGAStep::timeout == 0.
    std::chrono::milliseconds default_timeout{std::chrono::milliseconds(5000)};

    /// Default retry delay used when SAGAStep::retry_delay == 0.
    std::chrono::milliseconds default_retry_delay{std::chrono::milliseconds(1000)};

    /// Path for optional journal (empty = disabled).
    std::string journal_path;
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGAOrchestrator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Advanced SAGA orchestrator with parallel DAG execution, conditional
 *        branching, per-step retry policies, timeout management, and SAGA
 *        template support.
 *
 * ## Key differences from DistributedSagaCoordinator
 * - Steps use `void()` callables (throw on error) — suitable for local service
 *   calls where C++ exceptions are the idiomatic error channel.
 * - Provides SAGA **templates**: register a named SAGADefinition skeleton and
 *   instantiate it later with per-instance context overrides.
 * - Provides a text-based **workflow visualizer** that renders the DAG as ASCII
 *   art (useful for logging, debugging, and configuration review).
 * - Exposes `getStatus()` keyed on saga_id for post-execution inspection.
 *
 * ## Thread safety
 * Multiple concurrent `execute()` calls against the same orchestrator instance
 * are safe.  Template registration is also thread-safe.
 *
 * ## Usage
 * ```cpp
 * SAGAOrchestrator orchestrator;
 *
 * SAGADefinition order_saga;
 * order_saga.id   = "order-123";
 * order_saga.name = "process_order";
 * order_saga.enable_parallel = true;
 *
 * order_saga.steps.push_back({
 *     "reserve_inventory",
 *     []{ inventory.reserve(); },
 *     []{ inventory.release(); },
 *     {}                              // no dependencies
 * });
 *
 * order_saga.steps.push_back({
 *     "validate_customer",
 *     []{ customer.validate(); },
 *     {},                             // no compensation
 *     {}
 * });
 *
 * order_saga.steps.push_back({
 *     "charge_payment",
 *     []{ payment.charge(); },
 *     []{ payment.refund(); },
 *     {"reserve_inventory", "validate_customer"}  // depends on both
 * });
 *
 * auto status = orchestrator.execute(order_saga);
 * if (!status.ok) { handle_failure(status.message); }
 * ```
 */
class SAGAOrchestrator {
public:
    using Config = SAGAOrchestratorConfig;

    explicit SAGAOrchestrator(Config config = {});
    ~SAGAOrchestrator() = default;

    SAGAOrchestrator(const SAGAOrchestrator&)            = delete;
    SAGAOrchestrator& operator=(const SAGAOrchestrator&) = delete;
    SAGAOrchestrator(SAGAOrchestrator&&)                 = default;
    SAGAOrchestrator& operator=(SAGAOrchestrator&&)      = default;

    // ── Core execution API ────────────────────────────────────────────────────

    /**
     * @brief Execute a SAGA definition synchronously.
     *
     * Steps are run in dependency order (topological sort). Independent steps
     * run in parallel when enable_parallel is true.  On any step failure,
     * all completed steps are compensated in reverse execution order.
     *
     * @return OK() on full success, Error(...) with description on failure.
     */
    SagaOrchestratorStatus execute(const SAGADefinition& saga);

    /**
     * @brief Validate a SAGA definition without executing it.
     *
     * Checks that all depends_on names exist and that there are no dependency
     * cycles.
     */
    SagaOrchestratorStatus validate(const SAGADefinition& saga) const;

    // ── Status / Metrics ──────────────────────────────────────────────────────

    /**
     * @brief Retrieve the execution status for a previously executed SAGA.
     *
     * @return Status record, or std::nullopt if saga_id is unknown.
     */
    std::optional<SAGAExecutionStatus> getStatus(const std::string& saga_id) const;

    /**
     * @brief Aggregate orchestrator metrics.
     */
    struct Metrics {
        uint64_t sagas_started{0};
        uint64_t sagas_completed{0};
        uint64_t sagas_compensated{0};
        uint64_t sagas_failed{0};
        uint64_t total_step_executions{0};
        uint64_t total_step_retries{0};
        uint64_t total_compensations{0};
        uint64_t total_steps_skipped{0};
    };

    Metrics getMetrics() const;

    // ── Template support ──────────────────────────────────────────────────────

    /**
     * @brief Register a named SAGA template.
     *
     * Templates are skeleton SAGADefinitions whose callables capture shared
     * infrastructure (service handles, etc.).  Callers instantiate a template
     * by name, supplying an instance-specific `id` and context overrides.
     *
     * @param template_name  Key used to retrieve the template.
     * @param tmpl           SAGA definition to store as a template.
     */
    void registerTemplate(const std::string& template_name, SAGADefinition tmpl);

    /**
     * @brief Instantiate a previously registered template.
     *
     * Copies the template, sets the instance id, and merges the provided
     * context overrides (overrides take precedence over template defaults).
     *
     * @param template_name     Name of the registered template.
     * @param instance_id       Unique ID for the new SAGA instance.
     * @param context_overrides Key-value pairs merged into template context.
     * @return Instantiated SAGADefinition ready to pass to execute().
     * @throws std::out_of_range if template_name is not registered.
     */
    SAGADefinition instantiateTemplate(
        const std::string& template_name,
        const std::string& instance_id,
        std::map<std::string, std::string> context_overrides = {}
    ) const;

    // ── Visual workflow ───────────────────────────────────────────────────────

    /**
     * @brief Render the SAGA dependency graph as a text string.
     *
     * Each step is shown on its own line with a unicode arrow (→) connecting
     * it to its direct dependents.  Steps with no dependents are leaf nodes.
     * Example output:
     * ```
     * SAGA: process_order
     * ─────────────────────────────────────────────
     * reserve_inventory → charge_payment
     * validate_customer → charge_payment
     * charge_payment    → ship_order
     * ship_order        (terminal)
     * ─────────────────────────────────────────────
     * ```
     */
    std::string renderWorkflow(const SAGADefinition& saga) const;

private:
    Config config_;

    mutable std::mutex status_mutex_;
    std::unordered_map<std::string, SAGAExecutionStatus> statuses_;

    mutable std::mutex templates_mutex_;
    std::unordered_map<std::string, SAGADefinition> templates_;

    // Aggregate metrics (lock-protected for thread safety)
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;

    // Journal mutex: serialises concurrent journalWrite() calls
    mutable std::mutex journal_mutex_;

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Topologically sort steps; returns empty vector on cycle detection.
    std::vector<std::string> topologicalSort(const SAGADefinition& saga) const;

    // Map type for fast step lookup
    using StepMap = std::unordered_map<std::string, const SAGAStep*>;

    /// Build map: step_name → pointer into saga.steps.
    static StepMap buildStepMap(const SAGADefinition& saga);

    /// Execute a single step with retry and optional timeout.
    /// Returns the final StepState (COMPLETED, SKIPPED, or FAILED).
    /// Thread-safe: does NOT write to status_rec; caller applies the returned state.
    StepState executeStep(const SAGAStep& step,
                          const std::string& saga_id,
                          const Config& cfg);

    /// Compensate all completed steps in reverse execution order.
    void compensateAll(const SAGADefinition& saga,
                       const StepMap& step_map,
                       const std::vector<std::string>& executed_order,
                       SAGAExecutionStatus& status_rec);

    /// Compensate a single step (best-effort; does not throw).
    void compensateStep(const SAGAStep& step,
                        SAGAExecutionStatus& status_rec);

    /// Append a JSON line to the optional journal file.
    void journalWrite(const std::string& saga_id,
                      const std::string& event,
                      const std::string& detail = {});
};

} // namespace themis
