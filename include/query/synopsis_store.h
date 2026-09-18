/**
 * @file synopsis_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SynopsisTuple {
    int64_t     event_ts_us{0};  ///< Event timestamp in microseconds since epoch
    std::string payload;         ///< JSON-serialised tuple data
};

class SynopsisStore {
public:
    explicit SynopsisStore(size_t max_tuples = 10'000'000,
                           size_t max_bytes  = 1ULL << 30);

    [[nodiscard]] bool insert(SynopsisTuple tuple);

    /**
     * @brief Expire.
     * @param[in] window_start_us Input parameter.
     * @return Return value.
     */
    std::deque<SynopsisTuple> expire(int64_t window_start_us);

    [[nodiscard]] std::deque<SynopsisTuple> snapshot() const;

    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] size_t bytes() const noexcept;

    /**
     * @brief Clear.
     */
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
