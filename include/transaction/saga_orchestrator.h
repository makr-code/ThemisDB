/**
 * @file saga_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief OK.
     * @return Return value.
     * @details Implements OK without additional internal calls.
     */
    static SagaOrchestratorStatus OK()                       { return {}; }
    /**
     * @brief Error.
     * @param[in] msg Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
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

struct SAGAStep {
    std::string name;

    std::function<void()> forward;

    std::function<void()> compensate;

    std::set<std::string> depends_on;

    std::function<bool()> condition;

    std::chrono::milliseconds timeout{0};

    size_t max_retries{0};

    std::chrono::milliseconds retry_delay{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA definition
// ─────────────────────────────────────────────────────────────────────────────

struct SAGADefinition {
    std::string id;

    std::string name;

    std::vector<SAGAStep> steps;

    std::map<std::string, std::string> context;

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
    bool enable_parallel{true};

    std::chrono::milliseconds default_timeout{std::chrono::milliseconds(5000)};

    std::chrono::milliseconds default_retry_delay{std::chrono::milliseconds(1000)};

    std::string journal_path;

    // ── Circuit Breaker Configuration (AC-9/AC-10) ──────────────────────────
    uint32_t circuit_breaker_threshold{5};

    std::chrono::milliseconds circuit_breaker_timeout{std::chrono::milliseconds(30000)};
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGAOrchestrator
// ─────────────────────────────────────────────────────────────────────────────

class SAGAOrchestrator {
public:
    using Config = SAGAOrchestratorConfig;

    explicit SAGAOrchestrator(Config config = {});
    ~SAGAOrchestrator() = default;

    SAGAOrchestrator(const SAGAOrchestrator&)            = delete;
    SAGAOrchestrator& operator=(const SAGAOrchestrator&) = delete;
    SAGAOrchestrator(SAGAOrchestrator&&)                 noexcept = default;
    SAGAOrchestrator& operator=(SAGAOrchestrator&&)      noexcept = default;


    /**
     * @brief Execute.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    SagaOrchestratorStatus execute(const SAGADefinition& saga);

    /**
     * @brief Validate.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    SagaOrchestratorStatus validate(const SAGADefinition& saga) const;


    /**
     * @brief Get Status.
     * @param[in] saga_id Identifier of the saga.
     * @return Return value.
     */
    std::optional<SAGAExecutionStatus> getStatus(const std::string& saga_id) const;

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

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    Metrics getMetrics() const;


    /**
     * @brief Register Template.
     * @param[in] template_name Name of the template.
     * @param[in] tmpl Input parameter.
     */
    void registerTemplate(const std::string& template_name, SAGADefinition tmpl);

    SAGADefinition instantiateTemplate(
        const std::string& template_name,
        const std::string& instance_id,
        std::map<std::string, std::string> context_overrides = {}
    ) const;


    /**
     * @brief Render Workflow.
     * @param[in] saga Input parameter.
     * @return Return value.
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

    // ── Circuit Breaker State ──────────────────────────────────────────────
    // Per-step failure tracking for circuit breaker (AC-9/AC-10)
    mutable std::mutex circuit_breaker_mutex_;
    std::unordered_map<std::string, uint32_t> consecutive_failures_;  ///< step_name → failure count
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_failure_time_;
    std::unordered_map<std::string, bool> half_open_probe_in_flight_;

    /**
     * @brief Is Circuit Breaker Open.
     * @param[in] step_name Name of the step.
     * @return True when the operation succeeds.
     */
    bool isCircuitBreakerOpen(const std::string& step_name) const;

    /**
     * @brief Record Circuit Breaker Failure.
     * @param[in] step_name Name of the step.
     */
    void recordCircuitBreakerFailure(const std::string& step_name);

    /**
     * @brief Record Circuit Breaker Success.
     * @param[in] step_name Name of the step.
     */
    void recordCircuitBreakerSuccess(const std::string& step_name);

    /**
     * @brief Try Acquire Circuit Breaker Execution.
     * @param[in] step_name Name of the step.
     * @return True when the operation succeeds.
     */
    bool tryAcquireCircuitBreakerExecution(const std::string& step_name);


    /**
     * @brief Topological Sort.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    std::vector<std::string> topologicalSort(const SAGADefinition& saga) const;

    // Map type for fast step lookup
    using StepMap = std::unordered_map<std::string, const SAGAStep*>;

    /**
     * @brief Build Step Map.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    static StepMap buildStepMap(const SAGADefinition& saga);

    /**
     * @brief Execute Step.
     * @param[in] step Input parameter.
     * @param[in] saga_id Identifier of the saga.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    StepState executeStep(const SAGAStep& step,
                          const std::string& saga_id,
                          const Config& cfg);

    /**
     * @brief Compensate All.
     * @param[in] saga Input parameter.
     * @param[in] step_map Input parameter.
     * @param[in] executed_order Input parameter.
     * @param[in,out] status_rec Input/output parameter.
     */
    void compensateAll(const SAGADefinition& saga,
                       const StepMap& step_map,
                       const std::vector<std::string>& executed_order,
                       SAGAExecutionStatus& status_rec);

    /**
     * @brief Compensate Step.
     * @param[in] step Input parameter.
     * @param[in,out] status_rec Input/output parameter.
     */
    void compensateStep(const SAGAStep& step,
                        SAGAExecutionStatus& status_rec);

    void journalWrite(const std::string& saga_id,
                      const std::string& event,
                      const std::string& detail = {});
};

} // namespace themis
