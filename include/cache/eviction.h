/**
 * @file eviction.h
 * @brief Cache eviction scheduler with iterator-safe candidate selection.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2C)
 * @note Status: Production Ready
 *
 * Provides a policy-driven cache eviction engine that selects entries for
 * removal based on pluggable scoring callbacks.  All iterator arithmetic over
 * the candidate list uses `themis::security::SafeIterator` to prevent the
 * unsigned-arithmetic-wraparound and past-end dereference vulnerabilities
 * identified in the Sprint 7 gap scan (gap IDs B012, B013).
 *
 * **CWE Remediations:**
 * - CWE-129: signed/unsigned overflow guards in candidate selection;
 *   `AdvanceSafe::advance()` replaces raw distance arithmetic.
 * - CWE-416: `BoundsChecker::check_dereference()` guards every
 *   entry dereference in selection loops.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// EvictionCandidate
// ---------------------------------------------------------------------------

/**
 * @brief Descriptor of one candidate entry considered for eviction.
 */
struct EvictionCandidate {
    std::string key;            ///< Cache entry key.
    std::size_t size_bytes;     ///< Memory footprint of this entry.
    std::int64_t last_access_ns; ///< Last access timestamp (nanoseconds since epoch).
    std::uint64_t access_count; ///< Total number of accesses.
    double        score;        ///< Eviction urgency score (higher → evict first).
};

// ---------------------------------------------------------------------------
// EvictionPolicy
// ---------------------------------------------------------------------------

/**
 * @brief Scoring callback: assigns an eviction priority to one candidate.
 *
 * Higher scores are evicted first.  Must be a pure function of the candidate
 * state; must not throw.
 */
using EvictionScoringFn = std::function<double(const EvictionCandidate&)>;

// ---------------------------------------------------------------------------
// EvictionResult
// ---------------------------------------------------------------------------

/**
 * @brief Output of one eviction scheduling run.
 */
struct EvictionResult {
    std::vector<std::string> evicted_keys;  ///< Keys selected for removal.
    std::size_t              freed_bytes;   ///< Total bytes freed.
    std::size_t              candidates_evaluated; ///< Candidates considered.
};

// ---------------------------------------------------------------------------
// EvictionScheduler
// ---------------------------------------------------------------------------

/**
 * @brief Selects cache entries for eviction using a pluggable scoring function.
 *
 * **Type B safety:**
 * - All offset arithmetic in candidate selection uses `AdvanceSafe::advance()`.
 * - Every candidate entry dereference is guarded by `BoundsChecker::check_dereference()`.
 * - Unsigned offset calculations are checked for wraparound before use.
 *
 * **Usage:**
 * ```cpp
 * EvictionScheduler sched(lru_scoring_fn);
 * EvictionResult r = sched.select(candidates, target_bytes);
 * for (const auto& key : r.evicted_keys) { cache.erase(key); }
 * ```
 */
class EvictionScheduler {
public:
    /**
     * @brief Construct scheduler with a scoring callback.
     * @param scoring_fn Eviction urgency scorer; must not be null.
     * @throws std::invalid_argument if `scoring_fn` is null.
     */
    explicit EvictionScheduler(EvictionScoringFn scoring_fn);

    ~EvictionScheduler() = default;

    EvictionScheduler(const EvictionScheduler&)            = delete;
    EvictionScheduler& operator=(const EvictionScheduler&) = delete;
    EvictionScheduler(EvictionScheduler&&)                 noexcept = default;
    EvictionScheduler& operator=(EvictionScheduler&&)      noexcept = default;

    /**
     * @brief Score and sort candidates, then select entries to free target bytes.
     *
     * Scoring is applied to every entry in `candidates`.  Entries are sorted
     * descending by score (highest first).  Entries are then selected in order
     * until `target_free_bytes` have been accumulated, or all candidates are
     * exhausted.
     *
     * @param candidates         Mutable candidate list (sorted in-place by score).
     * @param target_free_bytes  Minimum bytes to free; must be > 0.
     * @return `EvictionResult` describing the selected keys.
     * @throws std::invalid_argument if `target_free_bytes == 0`.
     *
     * **Iterator safety:**
     * - Uses `RangeValidator` to validate the candidate range before the
     *   scoring loop.
     * - Uses `BoundsChecker::check_dereference()` before reading each entry.
     */
    [[nodiscard]] EvictionResult select(
        std::vector<EvictionCandidate>& candidates,
        std::size_t target_free_bytes) const;

    /**
     * @brief Score a single candidate.
     * @param c Candidate to score.
     * @return Eviction urgency score.
     */
    [[nodiscard]] double score(const EvictionCandidate& c) const noexcept;

    /**
     * @brief Replace the scoring function at runtime.
     * @param scoring_fn New scorer; must not be null.
     * @throws std::invalid_argument if `scoring_fn` is null.
     */
    void set_scoring_fn(EvictionScoringFn scoring_fn);

    // -----------------------------------------------------------------------
    // Built-in scoring policies
    // -----------------------------------------------------------------------

    /**
     * @brief LRU scoring: score = -last_access_ns (least-recently-used first).
     */
    static EvictionScoringFn lru_policy() noexcept;

    /**
     * @brief LFU scoring: score = 1.0 / (access_count + 1).
     */
    static EvictionScoringFn lfu_policy() noexcept;

    /**
     * @brief Size-aware LRU: combines recency and size for memory pressure.
     */
    static EvictionScoringFn size_aware_lru_policy() noexcept;

private:
    EvictionScoringFn scoring_fn_;
};

}  // namespace cache
}  // namespace themis
