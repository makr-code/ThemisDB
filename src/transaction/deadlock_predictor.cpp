/**
 * @file deadlock_predictor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/deadlock_predictor.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {

// ── Construction / configuration ──────────────────────────────────────────────

DeadlockPredictor::DeadlockPredictor(Config config)
    : config_(std::move(config)) {}

void DeadlockPredictor::setConfig(Config config) {
    std::lock_guard<std::mutex> lk(mutex_);
    config_ = std::move(config);
}

DeadlockPredictor::Config DeadlockPredictor::getConfig() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_;
}

// ── Training API ──────────────────────────────────────────────────────────────

void DeadlockPredictor::recordTransaction(
        TransactionId /* txn_id */,
        const std::vector<std::string>& locks_acquired,
        std::chrono::microseconds duration)
{
    if (locks_acquired.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    ++transaction_count_;

    // Record per-key hold times (evenly split across all locked keys).
    std::chrono::microseconds per_key{duration.count() /
                                      static_cast<long>(locks_acquired.size())};
    for (const auto& key : locks_acquired) {
        auto& vec = hold_times_[key];
        vec.push_back(per_key);
        // Keep the per-key history bounded.
        if (vec.size() > config_.max_patterns) {
            vec.erase(vec.begin());
        }
    }

    // Store the pattern (deduplicate by exact key sequence).
    bool found = false;
    for (auto& p : patterns_) {
        if (p.keys == locks_acquired) {
            ++p.frequency;
            // Running average of hold time.
            p.hold_time = std::chrono::microseconds(
                (p.hold_time.count() * (p.frequency - 1) + duration.count()) /
                p.frequency);
            found = true;
            break;
        }
    }
    if (!found) {
        LockPattern pat;
        pat.keys       = locks_acquired;
        pat.hold_time  = duration;
        pat.frequency  = 1;
        patterns_.push_back(std::move(pat));

        // Evict oldest entry when the buffer is full.
        if (patterns_.size() > config_.max_patterns) {
            patterns_.pop_front();
        }
    }

    // Increment co-occurrence counts for every pair in this transaction.
    // These base co-occurrence weights allow the predictor to build a conflict
    // map even before any deadlock is observed.  Deadlock events then apply an
    // additional multiplier (deadlock_weight_multiplier) so that pairs confirmed
    // to deadlock receive substantially higher scores.
    for (size_t i = 0; i < locks_acquired.size(); ++i) {
        for (size_t j = i + 1; j < locks_acquired.size(); ++j) {
            const std::string pk = makePairKey(locks_acquired[i], locks_acquired[j]);
            double new_val = (pair_conflicts_[pk] += 1.0);

            // Track running maximum so normalisation is always up-to-date.
            if (new_val > max_conflict_score_) {
                max_conflict_score_ = new_val;
            }

            // Evict when the map is too large (remove the entry with the lowest
            // count to keep the most significant conflict pairs).
            if (pair_conflicts_.size() > config_.max_conflict_pairs) {
                auto it_min = std::min_element(
                    pair_conflicts_.begin(), pair_conflicts_.end(),
                    [](const auto& a, const auto& b) {
                        return a.second < b.second;
                    });
                pair_conflicts_.erase(it_min);
            }
        }
    }
}

void DeadlockPredictor::recordDeadlock(const std::vector<std::string>& keys) {
    if (keys.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lk(mutex_);

    ++deadlock_count_;

    // Apply a higher weight to pairs that participated in a real deadlock.
    for (size_t i = 0; i < keys.size(); ++i) {
        for (size_t j = i + 1; j < keys.size(); ++j) {
            const std::string pk = makePairKey(keys[i], keys[j]);
            double new_val = (pair_conflicts_[pk] += config_.deadlock_weight_multiplier);

            // Track running maximum so normalisation is always up-to-date.
            if (new_val > max_conflict_score_) {
                max_conflict_score_ = new_val;
            }

            // Apply the same eviction policy as recordTransaction() to keep the
            // map bounded and prevent unbounded memory growth.
            if (pair_conflicts_.size() > config_.max_conflict_pairs) {
                auto it_min = std::min_element(
                    pair_conflicts_.begin(), pair_conflicts_.end(),
                    [](const auto& a, const auto& b) {
                        return a.second < b.second;
                    });
                // If we are evicting the entry whose value equals max_conflict_score_,
                // we need to recompute the max from the remaining entries.
                double evicted_val = it_min->second;
                pair_conflicts_.erase(it_min);
                if (evicted_val >= max_conflict_score_) {
                    max_conflict_score_ = 0.0;
                    for (const auto& [_, v] : pair_conflicts_) {
                        if (v > max_conflict_score_) max_conflict_score_ = v;
                    }
                }
            }
        }
    }

    // Mark any matching stored patterns as deadlocked.
    for (auto& p : patterns_) {
        // Check whether the pattern contains all keys in the deadlock cycle.
        bool all_present = true;
        for (const auto& k : keys) {
            if (std::find(p.keys.begin(), p.keys.end(), k) == p.keys.end()) {
                all_present = false;
                break;
            }
        }
        if (all_present) {
            p.was_deadlocked = true;
        }
    }
}

// ── Prediction API ────────────────────────────────────────────────────────────

double DeadlockPredictor::predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks,
        const std::set<TransactionId>&  active_transactions) const
{
    std::lock_guard<std::mutex> lk(mutex_);

    if (transaction_count_ < config_.min_samples_for_prediction ||
        proposed_locks.empty()) {
        return 0.0;
    }

    double score = computeConflictScore(proposed_locks);

    if (max_conflict_score_ <= 0.0) {
        return 0.0;
    }

    // Normalise to [0, 1].
    double probability = score / max_conflict_score_;

    // Scale slightly by the number of concurrent active transactions: more
    // concurrent transactions → higher contention.
    if (!active_transactions.empty()) {
        double contention_factor = 1.0 +
            std::log1p(static_cast<double>(active_transactions.size())) * 0.05;
        probability *= contention_factor;
    }

    return std::min(probability, 1.0);
}

std::vector<std::string> DeadlockPredictor::recommendLockOrder(
        const std::vector<std::string>& keys) const
{
    std::lock_guard<std::mutex> lk(mutex_);

    if (keys.empty()) {
        return {};
    }

    // Build a "danger score" per key based on how often it has appeared in
    // conflict pairs.  Keys with lower danger scores are safer to acquire first.
    std::unordered_map<std::string, double> danger;
    for (const auto& key : keys) {
        danger[key] = 0.0;
    }
    for (const auto& [pair_key, weight] : pair_conflicts_) {
        // pair_key is encoded as "a\x00b" (NUL-byte separator) – split on '\x00'.
        auto sep = pair_key.find('\x00');
        if (sep == std::string::npos) continue;

        std::string a = pair_key.substr(0, sep);
        std::string b = pair_key.substr(sep + 1);

        if (danger.count(a)) danger[a] += weight;
        if (danger.count(b)) danger[b] += weight;
    }

    // Sort: lower danger → acquire earlier; break ties lexicographically.
    std::vector<std::string> result(keys);
    std::sort(result.begin(), result.end(),
              [&](const std::string& x, const std::string& y) {
                  double dx = danger.count(x) ? danger.at(x) : 0.0;
                  double dy = danger.count(y) ? danger.at(y) : 0.0;
                  if (dx != dy) return dx < dy;
                  return x < y;
              });
    return result;
}

std::chrono::milliseconds DeadlockPredictor::recommendTimeout(
        const std::vector<std::string>& keys) const
{
    std::lock_guard<std::mutex> lk(mutex_);

    if (keys.empty()) {
        return config_.min_recommended_timeout;
    }

    // Collect all observed hold times for the requested keys.
    std::vector<std::chrono::microseconds> samples;
    for (const auto& key : keys) {
        auto it = hold_times_.find(key);
        if (it != hold_times_.end()) {
            samples.insert(samples.end(), it->second.begin(), it->second.end());
        }
    }

    if (samples.empty()) {
        return config_.min_recommended_timeout;
    }

    auto p_us = percentile(std::move(samples), config_.timeout_percentile);

    // Convert to milliseconds, add a 2× safety margin.
    auto timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(p_us) * 2;

    if (timeout_ms < config_.min_recommended_timeout) {
        return config_.min_recommended_timeout;
    }
    if (timeout_ms > config_.max_recommended_timeout) {
        return config_.max_recommended_timeout;
    }
    return timeout_ms;
}

// ── Introspection ─────────────────────────────────────────────────────────────

size_t DeadlockPredictor::recordedTransactionCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<size_t>(transaction_count_);
}

size_t DeadlockPredictor::recordedDeadlockCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<size_t>(deadlock_count_);
}

std::vector<DeadlockPredictor::LockPattern> DeadlockPredictor::getPatterns() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return {patterns_.begin(), patterns_.end()};
}

void DeadlockPredictor::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    patterns_.clear();
    pair_conflicts_.clear();
    hold_times_.clear();
    deadlock_count_   = 0;
    transaction_count_ = 0;
    max_conflict_score_ = 0.0;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

// static
std::string DeadlockPredictor::makePairKey(const std::string& a, const std::string& b) {
    // Use NUL byte (ASCII 0) as separator so keys containing ':' are unambiguous.
    if (a <= b) {
        return a + '\x00' + b;
    }
    return b + '\x00' + a;
}

double DeadlockPredictor::computeConflictScore(
        const std::vector<std::string>& keys) const
{
    double score = 0.0;
    for (size_t i = 0; i < keys.size(); ++i) {
        for (size_t j = i + 1; j < keys.size(); ++j) {
            const std::string pk = makePairKey(keys[i], keys[j]);
            auto it = pair_conflicts_.find(pk);
            if (it != pair_conflicts_.end()) {
                score += it->second;
            }
        }
    }
    return score;
}

// static
std::chrono::microseconds DeadlockPredictor::percentile(
        std::vector<std::chrono::microseconds> values, int p)
{
    if (values.empty()) {
        return std::chrono::microseconds{0};
    }
    std::sort(values.begin(), values.end());
    // Clamp p to [0, 100].
    p = std::max(0, std::min(100, p));
    // p == 0 → return the minimum (first element after sort).
    if (p == 0) {
        return values.front();
    }
    size_t idx = static_cast<size_t>(
        std::ceil(static_cast<double>(p) / 100.0 *
                  static_cast<double>(values.size())) - 1);
    idx = std::min(idx, values.size() - 1);
    return values[idx];
}

} // namespace themis
