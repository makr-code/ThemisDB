/**
 * @file deadlock_predictor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// @brief Machine-learning-inspired deadlock predictor for the transaction subsystem.
///
/// DeadlockPredictor learns from historical transaction and deadlock events to:
///   - Score the probability that a proposed set of locks will deadlock.
///   - Recommend a safe lock-acquisition order: keys are sorted by ascending
///     aggregate conflict weight (danger score) with lexicographic tie-breaking.
///     Keys with higher danger scores—i.e. those that have co-occurred in
///     contentious or deadlocked transactions more often—are acquired last.
///   - Suggest an adaptive transaction timeout calibrated to the observed hold
///     time of the keys being requested.
///
/// ### Algorithm
/// The predictor maintains two complementary data structures:
///
/// 1. **LockPattern registry** – every completed transaction is recorded as a
///    (keys, hold_time) pair.  Patterns that appear in deadlock cycles are given
///    a higher *conflict weight*.
///
/// 2. **Pair-conflict matrix** – whenever two keys co-occur in the same
///    transaction, the cell (keyA, keyB) is incremented by 1 (co-occurrence
///    heuristic).  When a deadlock cycle is recorded via `recordDeadlock()`,
///    those same pairs are further incremented by `Config::deadlock_weight_multiplier`
///    to reinforce high-risk combinations.  The deadlock probability for a
///    proposed lock set is then derived from the normalised sum of pair-conflict
///    weights over all pairs in the set.
///
/// ### Thread safety
/// All public methods are thread-safe; internal state is protected by a single
/// `std::mutex`.
///
/// @version v1.9.0
class DeadlockPredictor {
public:
    // ── Types ─────────────────────────────────────────────────────────────────

    using TransactionId = uint64_t;

    /// A single historical lock-acquisition pattern recorded from one transaction.
    struct LockPattern {
        std::vector<std::string>         keys;       ///< Keys acquired (in order)
        std::chrono::microseconds        hold_time;  ///< Total lock hold duration
        uint64_t                         frequency{1}; ///< Times this exact pattern was seen
        bool                             was_deadlocked{false}; ///< Pattern led to a deadlock
    };

    /// Configuration knobs for the predictor.
    struct Config {
        /// Maximum number of LockPattern entries to retain.
        size_t max_patterns{10'000};

        /// Maximum number of (key, key) conflict pairs to track.
        size_t max_conflict_pairs{100'000};

        /// Minimum number of recorded events before predictions are made.
        /// Returns 0.0 probability until this threshold is reached.
        size_t min_samples_for_prediction{5};

        /// Weight multiplier applied to conflict counts that involved an actual
        /// deadlock (vs. merely observed co-occurrence).
        double deadlock_weight_multiplier{3.0};

        /// Percentile (0–100) used to derive the recommended timeout from
        /// observed hold times.  Default: 90th percentile.
        int timeout_percentile{90};

        /// Floor for the recommended timeout so that it is never unreasonably
        /// short.
        std::chrono::milliseconds min_recommended_timeout{50};

        /// Ceiling for the recommended timeout.
        std::chrono::milliseconds max_recommended_timeout{30'000};
    };

    // ── Construction / configuration ─────────────────────────────────────────

    DeadlockPredictor() = default;
    explicit DeadlockPredictor(Config config);

    /// Replace the current configuration.  Thread-safe.
    void setConfig(Config config);
    Config getConfig() const;

    // ── Training API ──────────────────────────────────────────────────────────

    /// Record a completed transaction's lock-acquisition history.
    ///
    /// @param txn_id           Identifier of the completed transaction.
    /// @param locks_acquired   Ordered list of keys that were locked.
    /// @param duration         Total time the transaction held its locks.
    void recordTransaction(TransactionId txn_id,
                           const std::vector<std::string>& locks_acquired,
                           std::chrono::microseconds duration);

    /// Record that @p keys were involved in a deadlock cycle.
    /// This increases the conflict weight for every pair in @p keys.
    ///
    /// @param keys  Keys that were part of the deadlock cycle.
    void recordDeadlock(const std::vector<std::string>& keys);

    // ── Prediction API ────────────────────────────────────────────────────────

    /// Estimate the probability [0.0, 1.0] that acquiring @p proposed_locks
    /// while @p active_transactions are running will result in a deadlock.
    ///
    /// Returns 0.0 when fewer than Config::min_samples_for_prediction events
    /// have been recorded.
    ///
    /// @param proposed_locks       Keys the caller intends to lock.
    /// @param active_transactions  IDs of transactions currently in flight
    ///                             (used for active-load scaling).
    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks,
        const std::set<TransactionId>&  active_transactions) const;

    /// Return the recommended key acquisition order for @p keys.
    ///
    /// Keys are sorted by ascending aggregate conflict weight (danger score):
    /// a key's danger score is the sum of all pair-conflict weights it shares
    /// with other keys in the input set.  Keys with lower danger scores are
    /// placed first (safer to acquire earlier).  Ties are broken lexicographically
    /// for determinism.
    ///
    /// If no historical data is available the keys are returned in lexicographic
    /// order.
    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys) const;

    /// Suggest a transaction timeout calibrated to the observed hold times of
    /// the given @p keys, clamped to [Config::min_recommended_timeout,
    /// Config::max_recommended_timeout].
    ///
    /// Falls back to Config::min_recommended_timeout when no data is available.
    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys) const;

    // ── Introspection ─────────────────────────────────────────────────────────

    /// Number of transactions recorded so far.
    size_t recordedTransactionCount() const;

    /// Number of deadlock events recorded so far.
    size_t recordedDeadlockCount() const;

    /// Return all retained LockPattern entries (copy).  Primarily for testing.
    std::vector<LockPattern> getPatterns() const;

    /// Reset all internal state.
    void reset();

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Canonical pair key: always (min, max) so lookup is symmetric.
    static std::string makePairKey(const std::string& a, const std::string& b);

    /// Compute the raw conflict score for a set of proposed locks.
    double computeConflictScore(const std::vector<std::string>& keys) const;

    /// Return the p-th percentile of a sorted vector of microsecond values.
    static std::chrono::microseconds percentile(
        std::vector<std::chrono::microseconds> values, int p);

    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex mutex_;

    Config config_;

    /// Circular buffer of recorded lock patterns.
    std::deque<LockPattern> patterns_;

    /// NUL-separated pair key → conflict weight.
    /// Incremented by 1 for every co-occurring pair in `recordTransaction()`;
    /// incremented by `Config::deadlock_weight_multiplier` for pairs confirmed
    /// to deadlock via `recordDeadlock()`.
    std::unordered_map<std::string, double> pair_conflicts_;

    /// Per-key: aggregate hold times for timeout estimation.
    std::unordered_map<std::string, std::vector<std::chrono::microseconds>> hold_times_;

    /// Number of deadlock events seen.
    uint64_t deadlock_count_{0};

    /// Total recorded transactions.
    uint64_t transaction_count_{0};

    /// Maximum raw conflict score observed (used for normalisation).
    double max_conflict_score_{0.0};
};

} // namespace themis
