/**
 * @file eviction.h
 * @brief Cache eviction scheduler with iterator-safe candidate selection.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
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

using EvictionScoringFn = std::function<double(const EvictionCandidate&)>;

// ---------------------------------------------------------------------------
// EvictionResult
// ---------------------------------------------------------------------------

struct EvictionResult {
    std::vector<std::string> evicted_keys;  ///< Keys selected for removal.
    std::size_t              freed_bytes;   ///< Total bytes freed.
    std::size_t              candidates_evaluated; ///< Candidates considered.
};

// ---------------------------------------------------------------------------
// EvictionScheduler
// ---------------------------------------------------------------------------

class EvictionScheduler {
public:
    /**
     * @brief Eviction Scheduler.
     * @param[in] scoring_fn Input parameter.
     * @return Return value.
     */
    explicit EvictionScheduler(EvictionScoringFn scoring_fn);

    ~EvictionScheduler() = default;

    EvictionScheduler(const EvictionScheduler&)            = delete;
    EvictionScheduler& operator=(const EvictionScheduler&) = delete;
    EvictionScheduler(EvictionScheduler&&)                 noexcept = default;
    EvictionScheduler& operator=(EvictionScheduler&&)      noexcept = default;

    [[nodiscard]] EvictionResult select(
        std::vector<EvictionCandidate>& candidates,
        std::size_t target_free_bytes) const;

    [[nodiscard]] double score(const EvictionCandidate& c) const noexcept;

    /**
     * @brief Set scoring fn.
     * @param[in] scoring_fn Input parameter.
     */
    void set_scoring_fn(EvictionScoringFn scoring_fn);

    // -----------------------------------------------------------------------
    // Built-in scoring policies
    // -----------------------------------------------------------------------

    /**
     * @brief Lru policy.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static EvictionScoringFn lru_policy() noexcept;

    /**
     * @brief Lfu policy.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static EvictionScoringFn lfu_policy() noexcept;

    /**
     * @brief Size aware lru policy.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static EvictionScoringFn size_aware_lru_policy() noexcept;

private:
    EvictionScoringFn scoring_fn_;
};

}  // namespace cache
}  // namespace themis
