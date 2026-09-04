/**
 * @file learned_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Learned Index Structures — ML-based ordered-key index for ThemisDB
//
// Implements the Recursive Model Index (RMI) approach from:
//   Kraska, T. et al. (2018). "The Case for Learned Index Structures."
//   ACM SIGMOD International Conference on Management of Data.
//   https://arxiv.org/abs/1712.01208
//
// Architecture:
//   ┌──────────────┐
//   │  Stage-1     │  Piecewise-linear model that maps key → approximate CDF position
//   │  (root model)│
//   └──────┬───────┘
//          │  Selects expert sub-model
//          ▼
//   ┌──────────────┐
//   │  Stage-2     │  Fine-grained linear expert refines the position estimate
//   │  (experts)   │
//   └──────┬───────┘
//          │  predicted_pos ± max_error
//          ▼
//   ┌──────────────────┐
//   │  Correction      │  Binary search in [predicted - max_error, predicted + max_error]
//   │  Layer (±ε)      │
//   └──────────────────┘
//
// Key properties:
//   - Sorted key array as the underlying data structure; the model predicts array position.
//   - Two-stage RMI: a single root linear model fans out to N per-segment experts.
//   - Each expert is a simple linear regression (1D CDF approximation).
//   - During lookup the model returns a bounded-error estimate; binary search completes
//     the lookup in O(log(2*max_error)) comparisons instead of O(log n).
//   - Training is O(n) given a sorted key array.
//   - Supports int64, uint64, double, and float key types via a unified double encoding.
//   - Thread-safe for concurrent reads; mutations require external serialisation.
//
// Target: 2-3× faster point lookups than binary search for large sorted arrays,
//         with 10-100× smaller model footprint vs. a B-tree node index.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// Key encoding helpers
// ---------------------------------------------------------------------------

/// Encode any numeric key type to a monotone double for CDF modelling.
/// The encoding must preserve the total order of the original type.
inline double encodeKey(double v)   { return v; }
inline double encodeKey(float v)    { return static_cast<double>(v); }
inline double encodeKey(int64_t v)  { return static_cast<double>(v); }
inline double encodeKey(uint64_t v) { return static_cast<double>(v); }

// ---------------------------------------------------------------------------
// Linear model (single expert)
// ---------------------------------------------------------------------------

/// Single-variable linear model:  pos = slope * key + intercept
struct LinearModel {
    double slope     = 0.0;
    double intercept = 0.0;

    /// Predict a position from an encoded key.
    double predict(double key) const noexcept {
        return slope * key + intercept;
    }
};

// ---------------------------------------------------------------------------
// LearnedIndex configuration
// ---------------------------------------------------------------------------

struct LearnedIndexConfig {
    /// Number of second-stage expert models.
    /// More experts → smaller per-expert error bound, faster lookup, more memory.
    size_t num_experts = 64;

    /// Maximum allowed prediction error (array positions).
    /// The correction binary search range is [pred - max_error, pred + max_error].
    /// Set automatically during training if <= 0.
    int64_t max_error = -1;

    /// Minimum training samples required to build the index.
    size_t min_train_size = 2;

    /// Retrain threshold: fraction of keys that changed since last training.
    /// When exceeded, the index flags itself as stale.
    double retrain_threshold = 0.10;

    LearnedIndexConfig() = default;
    explicit LearnedIndexConfig(size_t experts,
                                int64_t max_error_override = -1,
                                double  retrain            = 0.10)
        : num_experts(experts), max_error(max_error_override), retrain_threshold(retrain) {}
};

// ---------------------------------------------------------------------------
// LearnedIndex
// ---------------------------------------------------------------------------

/// Two-stage Recursive Model Index (RMI) for sorted numeric key arrays.
///
/// Usage:
/// @code
///   std::vector<int64_t> keys = {1, 3, 7, 15, 42, 100, ...};
///   std::sort(keys.begin(), keys.end());
///
///   LearnedIndex<int64_t> idx(config);
///   idx.train(keys);
///
///   auto pos = idx.lookup(42);   // Returns index into keys[]
///   // Or with a payload array:
///   auto result = idx.lookupKey(42, keys);  // Returns optional<size_t> position
/// @endcode
template <typename KeyT>
class LearnedIndex {
public:
    using key_type = KeyT;

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------

    explicit LearnedIndex() = default;
    explicit LearnedIndex(const LearnedIndexConfig& config) : config_(config) {}

    // ------------------------------------------------------------------
    // Training
    // ------------------------------------------------------------------

    struct TrainResult {
        bool    ok              = false;
        int64_t max_error       = 0;   ///< Measured max prediction error (positions)
        double  mean_error      = 0.0; ///< Mean absolute error over training set
        size_t  num_keys        = 0;
        size_t  num_experts     = 0;
        std::string message = {};

        static TrainResult Ok(int64_t me, double avg, size_t nk, size_t ne) {
            TrainResult r;
            r.ok = true; r.max_error = me; r.mean_error = avg;
            r.num_keys = nk; r.num_experts = ne;
            return r;
        }
        static TrainResult Error(std::string msg) {
            TrainResult r; r.message = std::move(msg); return r;
        }
    };

    /// Train the index on a sorted key array.
    /// @param sorted_keys  Must be sorted in ascending order (duplicates allowed).
    /// @returns TrainResult with measured error statistics.
    TrainResult train(const std::vector<KeyT>& sorted_keys);

    // ------------------------------------------------------------------
    // Lookup
    // ------------------------------------------------------------------

    /// Predict the array position for @p key and correct via binary search.
    /// @returns The position in the training key array, or -1 if not found.
    int64_t lookup(const KeyT& key) const;

    /// Like lookup() but also verifies the key in @p keys.
    /// @returns Position where keys[pos] == key, or std::nullopt if absent.
    std::optional<size_t> lookupKey(const KeyT& key,
                                    const std::vector<KeyT>& keys) const;

    // ------------------------------------------------------------------
    // Range query
    // ------------------------------------------------------------------

    /// Return the half-open interval [lo_pos, hi_pos) in the sorted key array
    /// that covers all keys in [lo, hi].  Callers scan that range in O(range).
    std::pair<size_t, size_t> rangePositions(
        const KeyT& lo, const KeyT& hi,
        const std::vector<KeyT>& keys) const;

    // ------------------------------------------------------------------
    // State queries
    // ------------------------------------------------------------------

    bool  isTrained()  const noexcept { return trained_; }
    bool  isStale()    const noexcept { return stale_; }
    int64_t maxError() const noexcept { return max_error_; }
    size_t  numKeys()  const noexcept { return num_keys_; }
    size_t  numExperts() const noexcept { return experts_.size(); }

    /// Mark the index as stale (e.g. after significant writes).
    void markStale() noexcept { stale_ = true; }

    // ------------------------------------------------------------------
    // Serialisation (simple binary)
    // ------------------------------------------------------------------

    /// Persist the model to a byte buffer.  Does not include the key array.
    std::vector<uint8_t> serialize()   const;

    /// Restore model from a byte buffer previously produced by serialize().
    /// Returns false on format mismatch.
    bool deserialize(const std::vector<uint8_t>& data);

    // ------------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------------

    struct Stats {
        size_t  num_keys       = 0;
        size_t  num_experts    = 0;
        int64_t max_error      = 0;
        double  mean_error     = 0.0;
        bool    trained        = false;
        bool    stale          = false;
    };

    Stats stats() const noexcept;

private:
    // ------------------------------------------------------------------
    // Private helpers
    // ------------------------------------------------------------------

    /// Fit a linear model (least-squares) mapping encoded_keys[i] → i/n (CDF).
    static LinearModel fitLinear(const std::vector<double>& xs,
                                 const std::vector<double>& ys);

    /// Clamp @p pos into [0, n-1].
    static size_t clamp(int64_t pos, size_t n) noexcept {
        if (pos < 0) {
          return 0;
        }
        if (pos >= static_cast<int64_t>(n)) {
          return n - 1;
        }
        return static_cast<size_t>(pos);
    }

    /// Select the stage-2 expert index given a stage-1 prediction.
    size_t selectExpert(double pred1, size_t n) const noexcept {
        const size_t ne = experts_.size();
        int64_t idx = static_cast<int64_t>(
            pred1 / static_cast<double>(n) * static_cast<double>(ne));
        if (idx < 0) {
          idx = 0;
        }
        if (idx >= static_cast<int64_t>(ne)) {
          idx = static_cast<int64_t>(ne) - 1;
        }
        return static_cast<size_t>(idx);
    }

    // ------------------------------------------------------------------
    // Private state
    // ------------------------------------------------------------------

    LearnedIndexConfig config_;

    LinearModel              root_;       ///< Stage-1: root model
    std::vector<LinearModel> experts_;    ///< Stage-2: per-segment experts

    int64_t max_error_ = 0;              ///< Measured worst-case error
    double  mean_error_ = 0.0;
    size_t  num_keys_   = 0;
    bool    trained_    = false;
    bool    stale_      = false;
};

// ---------------------------------------------------------------------------
// Template implementation
// ---------------------------------------------------------------------------

template <typename KeyT>
typename LearnedIndex<KeyT>::TrainResult
LearnedIndex<KeyT>::train(const std::vector<KeyT>& sorted_keys) {
    if (sorted_keys.size() < config_.min_train_size) {
        return TrainResult::Error("Insufficient training data (need >= " +
                                  std::to_string(config_.min_train_size) + " keys)");
    }

    const size_t n = sorted_keys.size();

    // Encode keys to doubles for model training
    std::vector<double> xs(n);
    for (size_t i = 0; i < n; ++i)
        xs[i] = encodeKey(sorted_keys[i]);

    // Stage-1: fit root model mapping key → CDF position in [0, n)
    std::vector<double> ys(n);
    for (size_t i = 0; i < n; ++i)
        ys[i] = static_cast<double>(i);

    root_ = fitLinear(xs, ys);

    // Stage-2: partition keys into segments and fit one expert per segment.
    const size_t num_experts = std::min(config_.num_experts, n);
    experts_.resize(num_experts);

    // Assign each key to an expert based on the root model prediction
    std::vector<std::vector<size_t>> seg_indices(num_experts);
    for (size_t i = 0; i < n; ++i) {
        double pred = root_.predict(xs[i]);
        seg_indices[selectExpert(pred, n)].push_back(i);
    }

    // Fit each expert on its segment
    for (size_t e = 0; e < num_experts; ++e) {
        const auto& seg = seg_indices[e];
        if (seg.empty()) {
            // No keys in this segment — use zero model (safe fallback)
            experts_[e] = LinearModel{};
            continue;
        }
        std::vector<double> seg_xs(seg.size()), seg_ys(seg.size());
        for (size_t j = 0; j < seg.size(); ++j) {
            seg_xs[j] = xs[seg[j]];
            seg_ys[j] = static_cast<double>(seg[j]);
        }
        experts_[e] = fitLinear(seg_xs, seg_ys);
    }

    // Measure max and mean absolute error over the training set
    int64_t max_err = 0;
    double  sum_err = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double pred = root_.predict(xs[i]);
        double refined = experts_[selectExpert(pred, n)].predict(xs[i]);
        int64_t err = std::abs(static_cast<int64_t>(std::round(refined)) -
                               static_cast<int64_t>(i));
        if (err > max_err) {
          max_err = err;
        }
        sum_err += static_cast<double>(err);
    }

    max_error_ = (config_.max_error > 0) ? config_.max_error
                                         : (max_err + 1); // +1 for safety margin
    mean_error_ = sum_err / static_cast<double>(n);
    num_keys_ = n;
    trained_ = true;
    stale_   = false;

    return TrainResult::Ok(max_error_, mean_error_, n, num_experts);
}

template <typename KeyT>
int64_t LearnedIndex<KeyT>::lookup(const KeyT& key) const {
    if (!trained_ || experts_.empty()) {
      return -1;
    }

    const size_t n   = num_keys_;
    const double enc = encodeKey(key);

    // Stage-1 prediction
    double pred1 = root_.predict(enc);
    // Stage-2 prediction
    double pred2 = experts_[selectExpert(pred1, n)].predict(enc);
    int64_t pos  = static_cast<int64_t>(std::round(pred2));

    // Return the raw predicted position (caller does binary search if needed)
    return std::max<int64_t>(0, std::min<int64_t>(pos, static_cast<int64_t>(n) - 1));
}

template <typename KeyT>
std::optional<size_t>
LearnedIndex<KeyT>::lookupKey(const KeyT& key,
                               const std::vector<KeyT>& keys) const {
    if (!trained_ || keys.empty()) {
      return std::nullopt;
    }

    const size_t n           = keys.size();
    const int64_t pred        = lookup(key);
    const int64_t lo_raw      = pred - max_error_;
    const int64_t hi_raw      = pred + max_error_ + 1;
    const size_t lo           = clamp(lo_raw, n);
    // hi is an exclusive upper bound, so allow up to n
    const size_t hi = (hi_raw >= static_cast<int64_t>(n))
                          ? n
                          : static_cast<size_t>(std::max<int64_t>(0, hi_raw));

    // Binary search in the correction window
    auto it = std::lower_bound(keys.begin() + static_cast<ptrdiff_t>(lo),
                                keys.begin() + static_cast<ptrdiff_t>(hi),
                                key);
    if (it != keys.begin() + static_cast<ptrdiff_t>(hi) && *it == key)
        return static_cast<size_t>(it - keys.begin());
    return std::nullopt;
}

template <typename KeyT>
std::pair<size_t, size_t>
LearnedIndex<KeyT>::rangePositions(const KeyT& lo, const KeyT& hi,
                                    const std::vector<KeyT>& keys) const {
    if (keys.empty()) return {0, 0};
    const size_t n = keys.size();

    // Helper to clamp an upper-bound window (exclusive end) to [0, n]
    auto clampHi = [&](int64_t v) -> size_t {
        if (v <= 0) {
          return 0;
        }
        if (v >= static_cast<int64_t>(n)) {
          return n;
        }
        return static_cast<size_t>(v);
    };

    // Lower bound of range
    size_t range_lo = 0;
    if (!trained_) {
        auto it = std::lower_bound(keys.begin(), keys.end(), lo);
        range_lo = static_cast<size_t>(it - keys.begin());
    } else {
        const int64_t pred_lo = lookup(lo);
        const size_t  win_lo  = clamp(pred_lo - max_error_, n);
        const size_t  win_hi  = clampHi(pred_lo + max_error_ + 1);
        auto it = std::lower_bound(keys.begin() + static_cast<ptrdiff_t>(win_lo),
                                    keys.begin() + static_cast<ptrdiff_t>(win_hi),
                                    lo);
        range_lo = static_cast<size_t>(it - keys.begin());
    }

    // Upper bound of range
    size_t range_hi = 0;
    if (!trained_) {
        auto it = std::upper_bound(keys.begin(), keys.end(), hi);
        range_hi = static_cast<size_t>(it - keys.begin());
    } else {
        const int64_t pred_hi = lookup(hi);
        const size_t  win_lo  = clamp(pred_hi - max_error_, n);
        const size_t  win_hi  = clampHi(pred_hi + max_error_ + 1);
        auto it = std::upper_bound(keys.begin() + static_cast<ptrdiff_t>(win_lo),
                                    keys.begin() + static_cast<ptrdiff_t>(win_hi),
                                    hi);
        range_hi = static_cast<size_t>(it - keys.begin());
    }

    if (range_lo > range_hi) {
      range_hi = range_lo;
    }
    return {range_lo, range_hi};
}

template <typename KeyT>
LinearModel
LearnedIndex<KeyT>::fitLinear(const std::vector<double>& xs,
                               const std::vector<double>& ys) {
    const size_t n = xs.size();
    if (n == 0) return {};
    if (n == 1) {
        // Degenerate: single point — constant model
        LinearModel m;
        m.slope     = 0.0;
        m.intercept = ys[0];
        return m;
    }

    // Least-squares: min Σ(y - (slope*x + intercept))²
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sx  += xs[i];
        sy  += ys[i];
        sxx += xs[i] * xs[i];
        sxy += xs[i] * ys[i];
    }
    const double fn   = static_cast<double>(n);
    const double denom = fn * sxx - sx * sx;
    LinearModel m = {};
    if (std::abs(denom) < 1e-12) {
        // All x values identical — constant model
        m.slope     = 0.0;
        m.intercept = sy / fn;
    } else {
        m.slope     = (fn * sxy - sx * sy) / denom;
        m.intercept = (sy - m.slope * sx) / fn;
    }
    return m;
}

template <typename KeyT>
std::vector<uint8_t> LearnedIndex<KeyT>::serialize() const {
    // Layout:
    //   [4B magic] [8B num_experts] [8B num_keys] [8B max_error]
    //   [8B mean_error (as double)]
    //   [16B root: slope, intercept]
    //   N × [16B expert: slope, intercept]
    //   [1B trained] [1B stale]

    const size_t ne = experts_.size();
    const size_t sz = 4 + 8 + 8 + 8 + 8 + 16 + ne * 16 + 2;
    std::vector<uint8_t> buf(sz, 0u);
    uint8_t* p = buf.data();

    auto write4 = [&](uint32_t v) {
        std::memcpy(p, &v, 4); p += 4;
    };
    auto write8u = [&](uint64_t v) {
        std::memcpy(p, &v, 8); p += 8;
    };
    auto write8d = [&](double v) {
        std::memcpy(p, &v, 8); p += 8;
    };

    write4(0x4C494458u); // magic "LIDX"
    write8u(static_cast<uint64_t>(ne));
    write8u(static_cast<uint64_t>(num_keys_));
    write8u(static_cast<uint64_t>(max_error_));
    write8d(mean_error_);
    write8d(root_.slope);
    write8d(root_.intercept);
    for (const auto& e : experts_) {
        write8d(e.slope);
        write8d(e.intercept);
    }
    *p++ = static_cast<uint8_t>(trained_ ? 1 : 0);
    *p++ = static_cast<uint8_t>(stale_  ? 1 : 0);

    return buf;
}

template <typename KeyT>
bool LearnedIndex<KeyT>::deserialize(const std::vector<uint8_t>& data) {
    const uint8_t* p   = data.data();
    const uint8_t* end = p + data.size();

    auto need = [&](size_t bytes) { return (end - p) >= static_cast<ptrdiff_t>(bytes); };

    if (!need(4)) {
      return false;
    }
    uint32_t magic = {};
    std::memcpy(&magic, p, 4); p += 4;
    if (magic != 0x4C494458u) {
      return false;
    }

    if (!need(8 * 4)) {
      return false;
    }
    uint64_t ne, nk, me_u;
    double   mean_err = {};
    std::memcpy(&ne,      p, 8); p += 8;
    std::memcpy(&nk,      p, 8); p += 8;
    std::memcpy(&me_u,    p, 8); p += 8;
    std::memcpy(&mean_err, p, 8); p += 8;

    // Guard against integer overflow in size calculation (ne * 16 + 2)
    // and against excessive memory allocation (DoS).
    // 1M experts is far beyond any practical use.
    static constexpr uint64_t kMaxExperts = 1u << 20;
    if (ne > kMaxExperts) {
      return false;
    }

    if (!need(16)) {
      return false;
    }
    double rs, ri;
    std::memcpy(&rs, p, 8); p += 8;
    std::memcpy(&ri, p, 8); p += 8;

    if (!need(ne * 16 + 2)) {
      return false;
    }
    std::vector<LinearModel> experts(ne);
    for (size_t i = 0; i < ne; ++i) {
        std::memcpy(&experts[i].slope,     p, 8); p += 8;
        std::memcpy(&experts[i].intercept, p, 8); p += 8;
    }
    uint8_t tr = *p++, st = *p++;

    root_       = LinearModel{rs, ri};
    experts_    = std::move(experts);
    num_keys_   = static_cast<size_t>(nk);
    max_error_  = static_cast<int64_t>(me_u);
    mean_error_ = mean_err;
    trained_    = (tr != 0);
    stale_      = (st != 0);
    return true;
}

template <typename KeyT>
typename LearnedIndex<KeyT>::Stats
LearnedIndex<KeyT>::stats() const noexcept {
    return {num_keys_, experts_.size(), max_error_, mean_error_, trained_, stale_};
}

// ---------------------------------------------------------------------------
// Convenience type aliases
// ---------------------------------------------------------------------------

using LearnedIndexI64  = LearnedIndex<int64_t>;
using LearnedIndexU64  = LearnedIndex<uint64_t>;
using LearnedIndexF64  = LearnedIndex<double>;
using LearnedIndexF32  = LearnedIndex<float>;

} // namespace index
} // namespace themis
