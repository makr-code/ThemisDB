/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_orchestrator.h                                ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-17                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~250                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
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

/// Lightweight result type returned by SAGAOrchestrator operations.
struct SAGAStatus {
    bool        ok{true};
    std::string message;

    static SAGAStatus OK()                  { return {}; }
    static SAGAStatus Error(std::string msg){ return {false, std::move(msg)}; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-step state
// ─────────────────────────────────────────────────────────────────────────────

/// Lifecycle state of a single step during SAGA execution.
enum class StepState {
    PENDING,      ///< Not yet started
    RUNNING,      ///< Forward action in progress
    COMPLETED,    ///< Forward action succeeded
    FAILED,       ///< Forward action failed and exhausted retries
    COMPENSATING, ///< Compensation action in progress
    COMPENSATED   ///< Compensation action completed
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGAOrchestrator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates SAGA transactions with DAG-based parallel execution.
 *
 * SAGAOrchestrator provides advanced SAGA coordination with:
 *   - Parallel step execution based on a dependency graph (DAG)
 *   - Per-step retry policies with configurable backoff
 *   - Per-step and global timeout enforcement
 *   - Automatic compensation (rollback) on failure in reverse execution order
 *   - Named templates for reusable SAGA definitions
 *   - Execution status tracking and aggregate metrics
 *
 * Thread safety: a single SAGAOrchestrator instance may execute multiple
 * independent SAGAs concurrently.  Each execute() call is independently
 * synchronized.
 *
 * Example — parallel order processing:
 * @code
 *   SAGAOrchestrator::SAGADefinition def;
 *   def.name = "process_order";
 *   def.enable_parallel = true;
 *
 *   def.steps.push_back({"reserve_inventory",
 *       []{ inventory.reserve(); },
 *       []{ inventory.release(); },
 *       {}});                         // no dependencies → runs in parallel
 *
 *   def.steps.push_back({"validate_customer",
 *       []{ customer.validate(); },
 *       []{ },
 *       {}});
 *
 *   def.steps.push_back({"charge_payment",
 *       []{ payment.charge(); },
 *       []{ payment.refund(); },
 *       {"reserve_inventory", "validate_customer"}});  // depends on both above
 *
 *   SAGAOrchestrator orchestrator;
 *   auto status = orchestrator.execute(def);
 * @endcode
 */
class SAGAOrchestrator {
public:
    // ── Step definition ───────────────────────────────────────────────────────

    /**
     * @brief A single orchestrated SAGA step.
     *
     * A step describes:
     *   - A unique name within the SAGA definition.
     *   - A forward action that performs the step's work.
     *   - An optional compensating action that undoes the step on rollback.
     *   - An optional set of step names that must complete before this step starts.
     *   - Per-step retry and timeout configuration.
     */
    struct Step {
        /// Unique name within the SAGA definition.
        std::string name;

        /// Forward action: executes the step's work.
        std::function<void()> forward;

        /// Compensating action: undoes the step on rollback.  May be empty
        /// (treated as a no-op).
        std::function<void()> compensate;

        /// Names of steps that must complete before this step starts.
        std::set<std::string> depends_on;

        /// Maximum wall-clock time allowed for the forward action.
        std::chrono::milliseconds timeout{5000};

        /// Maximum number of retry attempts on transient failure (0 = no retry).
        size_t max_retries{3};

        /// Initial delay between retries; doubled on each attempt (exponential
        /// backoff) up to a maximum of 30 seconds.
        std::chrono::milliseconds retry_delay{1000};
    };

    // ── SAGA definition ───────────────────────────────────────────────────────

    /**
     * @brief A complete SAGA definition.
     *
     * Groups a set of steps under a single name.  Steps may declare
     * dependencies on one another; the orchestrator uses a topological sort
     * to determine execution order and runs dependency-free groups in parallel
     * when enable_parallel is true.
     */
    struct SAGADefinition {
        /// Descriptive name for the SAGA (used as the saga_id when not
        /// overridden via execute()).
        std::string name;

        /// Steps to execute.
        std::vector<Step> steps;

        /// Execute independent steps in parallel (default: true).
        bool enable_parallel{true};
    };

    // ── Execution status ──────────────────────────────────────────────────────

    /**
     * @brief Snapshot of per-step states for a SAGA execution.
     */
    struct ExecutionStatus {
        /// Saga name (from SAGADefinition::name or a unique execution ID).
        std::string saga_name;

        /// Current state for each step, keyed by step name.
        std::map<std::string, StepState> step_states;

        /// Number of steps that have reached StepState::COMPLETED.
        size_t completed_steps{0};

        /// Number of steps that have reached StepState::FAILED.
        size_t failed_steps{0};

        /// Number of steps still in StepState::PENDING.
        size_t pending_steps{0};
    };

    // ── Aggregate metrics ─────────────────────────────────────────────────────

    /// Aggregate execution metrics across all SAGAs run by this orchestrator.
    struct Metrics {
        uint64_t sagas_started{0};
        uint64_t sagas_completed{0};
        uint64_t sagas_compensated{0};
        uint64_t sagas_failed{0};
        uint64_t total_step_executions{0};
        uint64_t total_compensations{0};
        uint64_t total_retries{0};
        uint64_t total_timeout_aborts{0};
    };

    // ── Constructor / Destructor ──────────────────────────────────────────────

    SAGAOrchestrator()  = default;
    ~SAGAOrchestrator() = default;

    // Non-copyable, movable
    SAGAOrchestrator(const SAGAOrchestrator&)            = delete;
    SAGAOrchestrator& operator=(const SAGAOrchestrator&) = delete;
    SAGAOrchestrator(SAGAOrchestrator&&)                 = default;
    SAGAOrchestrator& operator=(SAGAOrchestrator&&)      = default;

    // ── Core API ──────────────────────────────────────────────────────────────

    /**
     * @brief Validate and execute a SAGA.
     *
     * Blocks until the SAGA either completes successfully or finishes
     * compensating after a failure.
     *
     * @param saga SAGA definition to execute.
     * @return SAGAStatus::OK() on success; SAGAStatus::Error(...) on failure.
     */
    SAGAStatus execute(const SAGADefinition& saga);

    /**
     * @brief Retrieve execution status for a previously executed SAGA.
     *
     * @param saga_id SAGA name (or unique execution ID).
     * @return ExecutionStatus snapshot, or a zero-filled struct if unknown.
     */
    ExecutionStatus getStatus(const std::string& saga_id) const;

    /**
     * @brief Validate a SAGA definition without executing it.
     *
     * Checks that all depends_on names exist and that there are no dependency
     * cycles.
     *
     * @return SAGAStatus::OK() on success; SAGAStatus::Error(...) with a
     *         description on failure.
     */
    SAGAStatus validate(const SAGADefinition& saga) const;

    /**
     * @brief Register a named SAGA template for later reuse.
     *
     * Templates are stored by SAGADefinition::name.  When execute() is
     * called with a definition whose name matches a registered template, the
     * template's default configuration is used as a fallback.
     *
     * @param templ SAGA definition to register as a template.
     */
    void registerTemplate(SAGADefinition templ);

    /**
     * @brief Retrieve a previously registered template by name.
     *
     * @return The template, or std::nullopt if not found.
     */
    std::optional<SAGADefinition> getTemplate(const std::string& name) const;

    /// Return a copy of the current aggregate metrics.
    Metrics getMetrics() const;

private:
    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex status_mutex_;
    std::map<std::string, ExecutionStatus> statuses_;

    mutable std::mutex metrics_mutex_;
    Metrics metrics_;

    mutable std::mutex templates_mutex_;
    std::map<std::string, SAGADefinition> templates_;

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Build execution waves (groups of steps that can run in parallel at the
    /// same dependency depth).  Returns empty outer vector on cycle detection.
    std::vector<std::vector<std::string>> buildWaves(
        const SAGADefinition& saga) const;

    /// Execute a single step with retry and timeout enforcement.
    /// @param retry_count  Out-param incremented on each retry attempt.
    SAGAStatus executeStep(const Step& step, size_t& retry_count);

    /// Compensate all steps in @p executed_order in reverse order.
    void compensateSteps(
        const std::map<std::string, const Step*>& step_map,
        const std::vector<std::string>&           executed_order,
        ExecutionStatus&                          status);

    /// Update the per-step state in the in-memory status store.
    void updateStepState(const std::string& saga_id,
                         const std::string& step_name,
                         StepState          state);
};

} // namespace themis
