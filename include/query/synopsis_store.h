/**
 * @file synopsis_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/window_spec.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <mutex>

namespace themis {
namespace query {

/**
 * @brief A single tuple stored inside the synopsis ring-buffer.
 */
struct SynopsisTuple {
    int64_t     event_ts_us{0};  ///< Event timestamp in microseconds since epoch
    std::string payload;         ///< JSON-serialised tuple data
};

/**
 * @brief In-memory ring-buffer synopsis store for a single continuous query.
 *
 * Stores tuples ordered by event timestamp.  Expired tuples (older than the
 * current window start) are evicted by expire().
 *
 * Thread safety: all public methods are guarded by an internal mutex.
 *
 * The RocksDB-backed persistent variant is a future hardening step
 * (Phase 8.5).  This implementation satisfies the Phase 8.1–8.4 contract.
 */
class SynopsisStore {
public:
    /**
     * @brief Construct a SynopsisStore with the given capacity limits.
     *
     * @param max_tuples  Maximum number of tuples (default 10 M).
     * @param max_bytes   Maximum total payload bytes (default 1 GiB).
     */
    explicit SynopsisStore(size_t max_tuples = 10'000'000,
                           size_t max_bytes  = 1ULL << 30);

    /**
     * @brief Insert a tuple into the store.
     *
     * @return false if either capacity limit would be exceeded; the tuple is
     *         discarded and the caller must count it as dropped.
     */
    [[nodiscard]] bool insert(SynopsisTuple tuple);

    /**
     * @brief Remove all tuples with event_ts_us < window_start_us.
     *
     * @return the tuples that were evicted (for incremental aggregation).
     */
    std::deque<SynopsisTuple> expire(int64_t window_start_us);

    /** @return All tuples currently in the store (snapshot). */
    [[nodiscard]] std::deque<SynopsisTuple> snapshot() const;

    /** @return Number of tuples currently stored. */
    [[nodiscard]] size_t size() const noexcept;

    /** @return Total byte footprint of stored payloads. */
    [[nodiscard]] size_t bytes() const noexcept;

    /** Clear all stored tuples. */
    void clear();

private:
    mutable std::mutex            mutex_;
    std::deque<SynopsisTuple>     tuples_;
    size_t                        total_bytes_{0};
    size_t                        max_tuples_{0};
    size_t                        max_bytes_{0};
};

}  // namespace query
}  // namespace themis
