/**
 * @file distributed_saga.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief OK.
     * @return Return value.
     * @details Implements OK without additional internal calls.
     */
    static DistributedSagaStatus OK()    { return {}; }
    /**
     * @brief Error.
     * @param[in] msg Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
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

struct DistributedSagaStep {
    using Action = std::function<DistributedSagaStatus()>;

    std::string name;

    std::string node_id;

    Action forward;

    Action compensate;

    std::set<std::string> depends_on;

    std::chrono::milliseconds forward_timeout{5000};

    std::chrono::milliseconds compensate_timeout{10000};

    size_t max_retries{3};

    std::chrono::milliseconds retry_backoff{std::chrono::milliseconds(100)};
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA definition
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedSagaDefinition {
    std::string saga_id;

    std::vector<DistributedSagaStep> steps;

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

    [[nodiscard]] bool succeeded() const {
        return state == SagaExecutionState::COMPLETED;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Remote step (for multi-cluster / service-mesh orchestration)
// ─────────────────────────────────────────────────────────────────────────────

struct RemoteStep {
    std::string service_endpoint;

    std::string operation = {};

    nlohmann::json params;

    std::string compensate_operation;

    nlohmann::json compensate_params;

    std::string name;

    std::set<std::string> depends_on;

    std::chrono::milliseconds forward_timeout{std::chrono::milliseconds(5000)};

    std::chrono::milliseconds compensate_timeout{std::chrono::milliseconds(10000)};

    size_t max_retries{3};

    std::chrono::milliseconds retry_backoff{std::chrono::milliseconds(100)};
};

// ─────────────────────────────────────────────────────────────────────────────
// Distributed SAGA definition (multi-cluster / remote-step variant)
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedSAGADefinition {
    std::string saga_id;

    std::vector<RemoteStep> steps;

    std::map<std::string, std::string> context;
};

// ─────────────────────────────────────────────────────────────────────────────
// SAGA visualization output
// ─────────────────────────────────────────────────────────────────────────────

struct SagaVisualization {
    std::string dot_graph;

    std::string text_summary;
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator configuration (defined outside the class so it can be used as a
// default argument in the constructor declaration)
// ─────────────────────────────────────────────────────────────────────────────

using RemoteStepExecutor =
    std::function<DistributedSagaStatus(
        const std::string& /*endpoint*/,
        const std::string& /*operation*/,
        const nlohmann::json& /*params*/)>;


struct ConsensusVerificationResult {
    bool verified{false};

    int quorum_size{0};

    int ack_count{0};

    std::string detail;
};

using ConsensusVerifier =
    std::function<ConsensusVerificationResult(
        const std::string& /*step_name*/,
        const std::string& /*node_id*/)>
;
struct DistributedSagaCoordinatorConfig {
    bool enable_parallel{true};

    std::string journal_path;

    std::chrono::milliseconds saga_timeout{std::chrono::milliseconds(0)};

    std::chrono::milliseconds default_forward_timeout{std::chrono::milliseconds(5000)};

    std::chrono::milliseconds default_compensate_timeout{std::chrono::milliseconds(10000)};

    bool enable_consensus_verification{true};

    ConsensusVerifier consensus_verifier;

    RemoteStepExecutor remote_executor;
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator
// ─────────────────────────────────────────────────────────────────────────────

class DistributedSagaCoordinator {
public:
    using Config = DistributedSagaCoordinatorConfig;

    explicit DistributedSagaCoordinator(Config config = {});
    ~DistributedSagaCoordinator() = default;

    // Non-copyable, movable
    DistributedSagaCoordinator(const DistributedSagaCoordinator&)            = delete;
    DistributedSagaCoordinator& operator=(const DistributedSagaCoordinator&) = delete;
    DistributedSagaCoordinator(DistributedSagaCoordinator&&)                 noexcept = default;
    DistributedSagaCoordinator& operator=(DistributedSagaCoordinator&&)      noexcept = default;


    /**
     * @brief Execute.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    DistributedSagaReport execute(const DistributedSagaDefinition& saga);

    /**
     * @brief Validate.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    DistributedSagaStatus validate(const DistributedSagaDefinition& saga) const;


    /**
     * @brief Execute Distributed.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    DistributedSagaReport executeDistributed(const DistributedSAGADefinition& saga);

    /**
     * @brief Get Distributed Status.
     * @param[in] saga_id Identifier of the saga.
     * @return Return value.
     */
    std::optional<DistributedSagaReport> getDistributedStatus(
        const std::string& saga_id) const;


    /**
     * @brief Recover In Progress SAGAs.
     * @return Return value.
     */
    std::vector<std::string> recoverInProgressSAGAs();


    /**
     * @brief Visualize.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    SagaVisualization visualize(const DistributedSagaDefinition& saga) const;


    /**
     * @brief Force Compensate.
     * @param[in] saga_id Identifier of the saga.
     * @return True when the operation succeeds.
     */
    bool forceCompensate(const std::string& saga_id);

    /**
     * @brief Force Complete.
     * @param[in] saga_id Identifier of the saga.
     * @return True when the operation succeeds.
     */
    bool forceComplete(const std::string& saga_id);


    /**
     * @brief Get Report.
     * @param[in] saga_id Identifier of the saga.
     * @return Return value.
     */
    std::optional<DistributedSagaReport> getReport(const std::string& saga_id) const;

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

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    Metrics getMetrics() const;

private:
    Config config_;

    mutable std::mutex reports_mutex_;
    std::map<std::string, DistributedSagaReport> reports_;

    // Aggregate metrics (updated atomically)
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;


    /**
     * @brief Topological Sort.
     * @param[in] saga Input parameter.
     * @return Return value.
     */
    std::vector<std::string> topologicalSort(
        const DistributedSagaDefinition& saga
    ) const;

    // Record index type for O(1) step-record lookup
    using RecordIndex = std::unordered_map<std::string, StepRecord*>;

    DistributedSagaStatus executeWave(
        const std::vector<std::string>&                    wave,
        const std::map<std::string, DistributedSagaStep>&  step_map,
        RecordIndex&                                       index,
        std::string&                                       failure_reason,
        std::optional<std::chrono::steady_clock::time_point> deadline
    );

    /**
     * @brief Execute Step.
     * @param[in] step Input parameter.
     * @param[in,out] record Input/output parameter.
     * @param[in] deadline Input parameter.
     * @return Return value.
     */
    DistributedSagaStatus executeStep(
        const DistributedSagaStep& step,
        StepRecord&                record,
        std::optional<std::chrono::steady_clock::time_point> deadline
    );

    bool verifyStepConsensus(
        const std::string& step_name,
        const std::string& node_id,
        StepRecord& record,
        std::string* failure_detail = nullptr
    );

    void compensate(
        const std::map<std::string, DistributedSagaStep>& step_map,
        const std::vector<std::string>&                   executed_order,
        RecordIndex&                                      index
    );

    /**
     * @brief Compensate Step.
     * @param[in] step Input parameter.
     * @param[in,out] record Input/output parameter.
     * @return Return value.
     */
    DistributedSagaStatus compensateStep(
        const DistributedSagaStep& step,
        StepRecord&                record
    );

    void journalWrite(const std::string& saga_id, const std::string& event,
                      const std::string& detail = {});

    /**
     * @brief Remote Step To Local.
     * @param[in] remote Input parameter.
     * @return Return value.
     */
    DistributedSagaStep remoteStepToLocal(const RemoteStep& remote) const;
};

} // namespace themis
