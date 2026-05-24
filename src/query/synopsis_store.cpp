/*
 * ThemisDB | File: synopsis_store.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 66
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=35 | delta=32 | status=divergent
 * External Severity (v3): C=6, H=26, M=3
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "query/synopsis_store.h"
#include <algorithm>
#include <cassert>

namespace themis {
namespace query {

SynopsisStore::SynopsisStore(size_t max_tuples, size_t max_bytes)
    : max_tuples_(max_tuples), max_bytes_(max_bytes) {}

bool SynopsisStore::insert(SynopsisTuple tuple) {
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

std::deque<SynopsisTuple> SynopsisStore::expire(int64_t window_start_us) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::deque<SynopsisTuple> expired;
    while (!tuples_.empty() && tuples_.front().event_ts_us < window_start_us) {
        total_bytes_ -= tuples_.front().payload.size();
        expired.push_back(std::move(tuples_.front()));
        tuples_.pop_front();
    }
    return expired;
}

std::deque<SynopsisTuple> SynopsisStore::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tuples_;
}

size_t SynopsisStore::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return tuples_.size();
}

size_t SynopsisStore::bytes() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_bytes_;
}

void SynopsisStore::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    tuples_.clear();
    total_bytes_ = 0;
}

}  // namespace query
}  // namespace themis
