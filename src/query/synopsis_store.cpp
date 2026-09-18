/**
 * @file synopsis_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/synopsis_store.h"
#include <algorithm>
#include <cassert>

namespace themis {
namespace query {

SynopsisStore::SynopsisStore(size_t max_tuples, size_t max_bytes)
    : max_tuples_(max_tuples), max_bytes_(max_bytes) {}

/**
 * @brief Insert.
 * @param[in] tuple Input parameter.
 * @return True on success.
 * @details Calls: lock(), size(), push_back(), std::move().
 */
bool SynopsisStore::insert(SynopsisTuple tuple) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    if (tuples_.size() >= max_tuples_) {
        return false;
    }
    const auto extra = tuple.payload.size();
    if (total_bytes_ + extra > max_bytes_) {
        return false;
    }
    total_bytes_ += extra;
    tuples_.push_back(std::move(tuple));
    return true;
}

/**
 * @brief Expire.
 * @param[in] window_start_us Input parameter.
 * @return Return value.
 * @details Calls: lock(), empty(), front(), size(), push_back(), std::move(), pop_front().
 */
std::deque<SynopsisTuple> SynopsisStore::expire(int64_t window_start_us) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::deque<SynopsisTuple> expired = {};

    while (!tuples_.empty() && tuples_.front().event_ts_us < window_start_us) {
        total_bytes_ -= tuples_.front().payload.size();
        expired.push_back(std::move(tuples_.front()));
        tuples_.pop_front();
    }
    return expired;
}

std::deque<SynopsisTuple> SynopsisStore::snapshot() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return tuples_;
}

size_t SynopsisStore::size() const noexcept {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return tuples_.size();
}

size_t SynopsisStore::bytes() const noexcept {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return total_bytes_;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void SynopsisStore::clear() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    tuples_.clear();
    total_bytes_ = 0;
}

}  // namespace query
}  // namespace themis
