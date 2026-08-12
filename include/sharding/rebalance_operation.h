/**
 * @file rebalance_operation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>
#include <memory>

namespace themis {
namespace sharding {

/** @brief Lifecycle states for one rebalance operation instance. */
enum class RebalanceState {
    PLANNED,        // Initial state, not started
    IN_PROGRESS,    // Currently executing
    COMPLETED,      // Successfully completed
    FAILED,         // Failed during execution
    ROLLED_BACK     // Rolled back after failure
};

/** @brief Mutable progress snapshot for a running rebalance operation. */
struct RebalanceProgress {
    uint64_t records_migrated = 0;
    uint64_t total_records = 0;
    uint64_t bytes_transferred = 0;
    double progress_percent = 0.0;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point estimated_completion;
};

/** @brief Immutable execution configuration for a rebalance operation. */
struct RebalanceOperationConfig {
    std::string source_shard_id;
    std::string target_shard_id;
    uint64_t token_range_start;
    uint64_t token_range_end;
    std::string operator_cert_path;     // Certificate of operator authorizing the rebalance
    std::string ca_cert_path;           // CA certificate for verification
    uint32_t batch_size = 1000;         // Records per batch
    bool verify_data = true;            // Verify data integrity
    bool enable_rollback = true;        // Enable automatic rollback on failure
};

/**
 * Manages rebalance operations for shard data migration
 * 
 * Handles:
 * - Operator authorization via PKI certificates
 * - State machine for rebalance lifecycle
 * - Progress tracking
 * - Rollback support on failure
 * - Token range migration planning
 */
class RebalanceOperation {
public:
    /** @brief Optional callback notified after progress updates. */
    using ProgressCallback = std::function<void(const RebalanceProgress&)>;

    /** @brief Construct operation and validate basic shard/token-range invariants. */
    explicit RebalanceOperation(const RebalanceOperationConfig& config);
    ~RebalanceOperation() = default;

    /** @brief Start operation after operator signature validation. */
    bool start(const std::string& operator_signature);

    /** @brief Mark operation as completed from IN_PROGRESS state. */
    bool complete();

    /** @brief Mark operation as failed with terminal error message. */
    bool fail(const std::string& error_message);

    /** @brief Transition failed operation into rolled-back state. */
    bool rollback();

    /** @brief Return current atomic operation state. */
    RebalanceState getState() const;

    /** @brief Return current progress snapshot copy. */
    RebalanceProgress getProgress() const;

    /** @brief Install callback invoked after progress updates. */
    void setProgressCallback(ProgressCallback callback);

    /** @brief Update migration counters and recompute completion estimate. */
    void updateProgress(uint64_t records_migrated, uint64_t bytes_transferred);

    /** @brief Validate operator signature and authorization material. */
    bool validateOperator(const std::string& operator_signature);

private:
    RebalanceOperationConfig config_;
    std::atomic<RebalanceState> state_;
    mutable std::mutex mutex_;
    
    RebalanceProgress progress_;
    ProgressCallback progress_callback_;
    
    std::string error_message_;
    bool operator_validated_ = false;

    /** @brief Perform guarded state transition using atomic compare-exchange. */
    bool transitionState(RebalanceState from, RebalanceState to);
};

} // namespace sharding
} // namespace themis
