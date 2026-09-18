/**
 * @file deadlock_predictor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace themis {

class DeadlockPredictor {
public:
    // ── Types ─────────────────────────────────────────────────────────────────

    using TransactionId = uint64_t;

    struct LockPattern {
        std::vector<std::string>         keys;       ///< Keys acquired (in order)
        std::chrono::microseconds        hold_time;  ///< Total lock hold duration
        uint64_t                         frequency{1}; ///< Times this exact pattern was seen
        bool                             was_deadlocked{false}; ///< Pattern led to a deadlock
    };

    struct Config {
        size_t max_patterns{10'000};

        size_t max_conflict_pairs{100'000};

        size_t min_samples_for_prediction{5};

        double deadlock_weight_multiplier{3.0};

        int timeout_percentile{90};

        std::chrono::milliseconds min_recommended_timeout{50};

        std::chrono::milliseconds max_recommended_timeout{30'000};
    };

    // ── Construction / configuration ─────────────────────────────────────────

    DeadlockPredictor() = default;
    /**
     * @brief Deadlock Predictor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit DeadlockPredictor(Config config);

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(Config config);
    /**
     * @brief Get Config.
     * @return Return value.
     */
    Config getConfig() const;


    /**
     * @brief Record Transaction.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] locks_acquired Input parameter.
     * @param[in] duration Input parameter.
     */
    void recordTransaction(TransactionId txn_id,
                           const std::vector<std::string>& locks_acquired,
                           std::chrono::microseconds duration);

    /**
     * @brief Record Deadlock.
     * @param[in] keys Input parameter.
     */
    void recordDeadlock(const std::vector<std::string>& keys);


    /**
     * @brief Predict Deadlock Probability.
     * @param[in] proposed_locks Input parameter.
     * @param[in] active_transactions Input parameter.
     * @return Return value.
     */
    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks,
        const std::set<TransactionId>&  active_transactions) const;

    /**
     * @brief Recommend Lock Order.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys) const;

    /**
     * @brief Recommend Timeout.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys) const;


    /**
     * @brief Recorded Transaction Count.
     * @return Return value.
     */
    size_t recordedTransactionCount() const;

    /**
     * @brief Recorded Deadlock Count.
     * @return Return value.
     */
    size_t recordedDeadlockCount() const;

    /**
     * @brief Get Patterns.
     * @return Return value.
     */
    std::vector<LockPattern> getPatterns() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:

    /**
     * @brief Make Pair Key.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static std::string makePairKey(const std::string& a, const std::string& b);

    /**
     * @brief Compute Conflict Score.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    double computeConflictScore(const std::vector<std::string>& keys) const;

    /**
     * @brief Percentile.
     * @param[in] values Input parameter.
     * @param[in] p Input parameter.
     * @return Return value.
     */
    static std::chrono::microseconds percentile(
        std::vector<std::chrono::microseconds> values, int p);

    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex mutex_;

    Config config_;

    std::deque<LockPattern> patterns_;

    std::unordered_map<std::string, double> pair_conflicts_;

    std::unordered_map<std::string, std::vector<std::chrono::microseconds>> hold_times_;

    uint64_t deadlock_count_{0};

    uint64_t transaction_count_{0};

    double max_conflict_score_{0.0};
};

} // namespace themis
