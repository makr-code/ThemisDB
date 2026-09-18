/**
 * @file transaction_batcher.h
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
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {

class TransactionBatcher {
public:
    // ── Status ────────────────────────────────────────────────────────────────

    struct Status {
        bool        ok{true};
        std::string message;

        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK()                   { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    // ── BatchConfig ───────────────────────────────────────────────────────────

    struct BatchConfig {
        std::chrono::microseconds window{5000};

        size_t max_batch_size{1000};

        size_t min_batch_size{10};

        bool enable_adaptive{true};
    };

    // ── BatchPolicy ───────────────────────────────────────────────────────────

    struct BatchPolicy {
        std::chrono::microseconds window{0};

        size_t max_batch_size{0};

        size_t min_batch_size{0};
    };

    // ── Stats ─────────────────────────────────────────────────────────────────

    struct Stats {
        uint64_t batches_flushed{0};

        uint64_t transactions_committed{0};

        uint64_t transactions_failed{0};

        double avg_batch_size{0.0};

        double avg_latency_ms{0.0};

        uint64_t adaptive_adjustments{0};
    };

    // ── Constructor / Destructor ──────────────────────────────────────────────

    TransactionBatcher();

    ~TransactionBatcher();

    // Non-copyable, non-movable (owns a background thread and mutexes)
    TransactionBatcher(const TransactionBatcher&)            = delete;
    TransactionBatcher& operator=(const TransactionBatcher&) = delete;
    TransactionBatcher(TransactionBatcher&&)                 = delete;
    TransactionBatcher& operator=(TransactionBatcher&&)      = delete;


    /**
     * @brief Set Batch Config.
     * @param[in] config Input parameter.
     */
    void setBatchConfig(const BatchConfig& config);

    /**
     * @brief Get Batch Config.
     * @return Return value.
     */
    BatchConfig getBatchConfig() const;

    /**
     * @brief Set Table Policy.
     * @param[in] table Input parameter.
     * @param[in] policy Input parameter.
     */
    void setTablePolicy(const std::string& table, const BatchPolicy& policy);

    /**
     * @brief Get Table Policy.
     * @param[in] table Input parameter.
     * @return Return value.
     */
    BatchPolicy getTablePolicy(const std::string& table) const;

    // ── Core API ──────────────────────────────────────────────────────────────

    std::future<Status> submitAsync(std::function<Status()> commit_fn,
                                    const std::string& table_hint = "");

    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

private:
    // ── Internal types ────────────────────────────────────────────────────────

    struct PendingEntry {
        std::function<Status()>               commit_fn;
        std::promise<Status>                  promise;
        std::string                           table_hint;
        std::chrono::steady_clock::time_point submitted_at;
        std::chrono::steady_clock::time_point deadline;
    };

    // ── Helper: resolve effective policy for a given table hint ───────────────

    struct EffectivePolicy {
        std::chrono::microseconds window;
        size_t                    max_batch_size;
        size_t                    min_batch_size;
    };
    /**
     * @brief Effective Policy For.
     * @param[in] table Input parameter.
     * @return Return value.
     */
    EffectivePolicy effectivePolicyFor(const std::string& table) const;


    /**
     * @brief Flush Loop.
     */
    void flushLoop();

    /**
     * @brief Execute Batch.
     * @param[in,out] batch Input/output parameter.
     */
    void executeBatch(std::vector<PendingEntry>& batch);

    /**
     * @brief Adapt Window.
     * @param[in] batch_size Input parameter.
     * @param[in] elapsed Input parameter.
     */
    void adaptWindow(size_t batch_size, std::chrono::microseconds elapsed);

    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex    config_mutex_;
    BatchConfig           config_;
    std::unordered_map<std::string, BatchPolicy> table_policies_;

    mutable std::mutex              queue_mutex_;
    std::condition_variable         queue_cv_;
    std::condition_variable         flush_cv_;
    std::deque<PendingEntry>        queue_;
    bool                            flush_requested_{false};
    bool                            batch_in_progress_{false};

    std::thread               flush_thread_;
    std::atomic<bool>         stopping_{false};

    mutable std::mutex        stats_mutex_;
    Stats                     stats_;

    // Adaptive state (guarded by config_mutex_)
    std::chrono::microseconds adaptive_window_{5000};
    std::deque<double>        recent_throughputs_;  // items/sec samples
};

} // namespace themis
