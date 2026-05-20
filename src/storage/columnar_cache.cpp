/*
 * ThemisDB | File: columnar_cache.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 299
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=81 | delta=78 | status=divergent
 * External Severity (v3): C=11, H=61, M=9
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file columnar_cache.cpp
 * @brief ColumnarCache + PinGuard implementation.
 */

#include "storage/columnar_cache.h"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace themis {
namespace storage {

// ============================================================================
// ColumnSegment::byteSize
// ============================================================================

size_t ColumnSegment::byteSize() const noexcept {
    size_t n = row_count;
    switch (dtype) {
        case SegmentDType::Int64:  return n * sizeof(int64_t) + n;   // data + null bitmap
        case SegmentDType::Double: return n * sizeof(double)  + n;
        case SegmentDType::Bool:   return n + n;
        case SegmentDType::String: {
            size_t total = n; // null bitmap
            for (const auto& s : string_data) total += s.size() + sizeof(std::string);
            return total;
        }
    }
    return n;
}

// ============================================================================
// PinGuard
// ============================================================================

PinGuard::PinGuard(ColumnarCache* cache,
                   SegmentKey     key,
                   const ColumnSegment* seg) noexcept
    : cache_(cache), key_(std::move(key)), segment_(seg) {}

PinGuard::~PinGuard() noexcept {
    release();
}

PinGuard::PinGuard(PinGuard&& o) noexcept
    : cache_(o.cache_), key_(std::move(o.key_)), segment_(o.segment_) {
    o.cache_   = nullptr;
    o.segment_ = nullptr;
}

PinGuard& PinGuard::operator=(PinGuard&& o) noexcept {
    if (this != &o) {
        release();
        cache_   = o.cache_;
        key_     = std::move(o.key_);
        segment_ = o.segment_;
        o.cache_   = nullptr;
        o.segment_ = nullptr;
    }
    return *this;
}

void PinGuard::release() noexcept {
    if (cache_ && segment_) {
        cache_->decrementPin(key_);
        cache_   = nullptr;
        segment_ = nullptr;
    }
}

// ============================================================================
// ColumnarCache
// ============================================================================

ColumnarCache::ColumnarCache(Config config) : cfg_(std::move(config)) {}

// ---------------------------------------------------------------------------
// put
// ---------------------------------------------------------------------------

void ColumnarCache::put(ColumnSegment segment) {
    const SegmentKey key = segment.key; // copy before moving segment
    const size_t seg_bytes = segment.byteSize();

    std::lock_guard<std::mutex> lk(mu_);

    // If key already exists: update in-place without changing LRU position.
    auto it = store_.find(key);
    if (it != store_.end()) {
        bytes_used_ -= it->second.segment.byteSize();
        it->second.segment = std::move(segment);
        bytes_used_ += seg_bytes;
        // Promote to MRU in LRU list.
        auto lit = lru_map_.find(key);
        if (lit != lru_map_.end()) {
            lru_list_.erase(lit->second);
            lru_list_.push_front(key);
            lit->second = lru_list_.begin();
        }
        return;
    }

    // New entry: evict first if needed.
    bytes_used_ += seg_bytes;
    evictLRU();

    store_.emplace(key, Entry{std::move(segment), 0});
    lru_list_.push_front(key);
    lru_map_[key] = lru_list_.begin();
}

// ---------------------------------------------------------------------------
// get
// ---------------------------------------------------------------------------

PinGuard ColumnarCache::get(const SegmentKey& key) {
    std::lock_guard<std::mutex> lk(mu_);

    auto it = store_.find(key);
    if (it == store_.end()) {
        ++miss_count_;
        return PinGuard{};
    }
    ++hit_count_;

    // Promote to MRU.
    auto lit = lru_map_.find(key);
    if (lit != lru_map_.end()) {
        lru_list_.erase(lit->second);
        lru_list_.push_front(key);
        lit->second = lru_list_.begin();
    }

    ++(it->second.pin_count);
    return PinGuard(this, key, &it->second.segment);
}

// ---------------------------------------------------------------------------
// contains
// ---------------------------------------------------------------------------

bool ColumnarCache::contains(const SegmentKey& key) const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    return store_.count(key) > 0;
}

// ---------------------------------------------------------------------------
// evict
// ---------------------------------------------------------------------------

bool ColumnarCache::evict(const SegmentKey& key) {
    std::function<void(const SegmentKey&)> on_evict_cb;
    bool evicted = false;

    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end() || it->second.pin_count > 0) {
            return false;
        }
        bytes_used_ -= it->second.segment.byteSize();
        if (cfg_.on_evict) on_evict_cb = cfg_.on_evict;

        auto lit = lru_map_.find(key);
        if (lit != lru_map_.end()) {
            lru_list_.erase(lit->second);
            lru_map_.erase(lit);
        }
        store_.erase(it);
        evicted = true;
    }

    if (evicted && on_evict_cb) on_evict_cb(key);
    return evicted;
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

void ColumnarCache::clear() {
    std::vector<SegmentKey> evicted_keys;
    std::function<void(const SegmentKey&)> on_evict_cb;

    {
        std::lock_guard<std::mutex> lk(mu_);
        on_evict_cb = cfg_.on_evict;
        for (auto it = store_.begin(); it != store_.end(); ) {
            if (it->second.pin_count == 0) {
                bytes_used_ -= it->second.segment.byteSize();
                if (on_evict_cb) evicted_keys.push_back(it->first);
                auto lit = lru_map_.find(it->first);
                if (lit != lru_map_.end()) {
                    lru_list_.erase(lit->second);
                    lru_map_.erase(lit);
                }
                it = store_.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (on_evict_cb) {
        for (const auto& k : evicted_keys) on_evict_cb(k);
    }
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

size_t ColumnarCache::size() const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    return store_.size();
}

size_t ColumnarCache::pinnedCount() const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    size_t n = 0;
    for (const auto& [k, e] : store_) {
        if (e.pin_count > 0) ++n;
    }
    return n;
}

size_t ColumnarCache::bytesUsed() const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    return bytes_used_;
}

uint64_t ColumnarCache::hitCount() const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    return hit_count_;
}

uint64_t ColumnarCache::missCount() const noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    return miss_count_;
}

// ---------------------------------------------------------------------------
// decrementPin (called by PinGuard destructor)
// ---------------------------------------------------------------------------

void ColumnarCache::decrementPin(const SegmentKey& key) noexcept {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = store_.find(key);
    if (it != store_.end() && it->second.pin_count > 0) {
        --(it->second.pin_count);
    }
}

// ---------------------------------------------------------------------------
// evictLRU (internal, called under mu_)
// ---------------------------------------------------------------------------

void ColumnarCache::evictLRU() {
    std::vector<SegmentKey> to_notify;

    while (bytes_used_ > cfg_.max_bytes && !lru_list_.empty()) {
        // Try from the LRU end.
        bool evicted_one = false;
        for (auto it = lru_list_.end(); it != lru_list_.begin(); ) {
            --it;
            const SegmentKey& k = *it;
            auto sit = store_.find(k);
            if (sit != store_.end() && sit->second.pin_count == 0) {
                bytes_used_ -= sit->second.segment.byteSize();
                if (cfg_.on_evict) to_notify.push_back(k);
                lru_map_.erase(k);
                store_.erase(sit);
                it = lru_list_.erase(it);
                evicted_one = true;
                break;
            }
        }
        if (!evicted_one) break; // Only pinned segments remain — give up.
    }

    // Fire callbacks outside the scan loop but still under mu_.
    // (Users must not call back into the cache from the callback.)
    if (cfg_.on_evict) {
        for (const auto& k : to_notify) cfg_.on_evict(k);
    }
}

} // namespace storage
} // namespace themis
