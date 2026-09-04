/**
 * @file eviction_strategies.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/cache_strategies.h"
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <queue>
#include <chrono>
#include <string>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief LRU (Least Recently Used) eviction strategy.
 *
 * Evicts the least recently accessed entry.
 * Maintains a list with most recently used at front.
 */
class LRUEvictionStrategy : public IEvictionStrategy {
public:
    void onAccess(std::string_view key) override {
        auto it = position_map_.find(std::string(key));
        if (it != position_map_.end()) {
            // Move to front (most recent)
            access_list_.splice(access_list_.begin(), access_list_, it->second);
        }
    }

    void onInsert(std::string_view key, [[maybe_unused]] uint64_t timestamp_ms) override {
        std::string key_str(key);
        auto it = position_map_.find(key_str);

        if (it != position_map_.end()) {
            // Already exists, move to front
            access_list_.splice(access_list_.begin(), access_list_, it->second);
        } else {
            // New entry, add to front
            access_list_.push_front(key_str);
            position_map_[key_str] = access_list_.begin();
        }
    }

    void onRemove(std::string_view key) override {
        auto it = position_map_.find(std::string(key));
        if (it != position_map_.end()) {
            access_list_.erase(it->second);
            position_map_.erase(it);
        }
    }

    std::optional<std::string> selectVictim() override {
        if (access_list_.empty()) {
            return std::nullopt;
        }
        return access_list_.back();  // Least recently used
    }

    void clear() override {
        access_list_.clear();
        position_map_.clear();
    }

    size_t size() const override {
        return access_list_.size();
    }

    std::string_view getName() const override {
        return "LRU";
    }

private:
    std::list<std::string> access_list_;  // Front = most recent, back = least recent
    std::unordered_map<std::string, std::list<std::string>::iterator> position_map_;
};

/**
 * @brief LFU (Least Frequently Used) eviction strategy.
 *
 * Evicts the least frequently accessed entry.
 * Tracks access frequency for each key.
 * Ties are broken by oldest recorded access timestamp.
 */
class LFUEvictionStrategy : public IEvictionStrategy {
public:
    void onAccess(std::string_view key) override {
        auto it = frequency_map_.find(std::string(key));
        if (it != frequency_map_.end()) {
            it->second.count++;
            it->second.last_access_ms = getCurrentTimeMs();
        }
    }

    void onInsert(std::string_view key, uint64_t timestamp_ms) override {
        std::string key_str(key);
        auto it = frequency_map_.find(key_str);
        
        if (it != frequency_map_.end()) {
            it->second.count++;
            it->second.last_access_ms = timestamp_ms;
        } else {
            frequency_map_[key_str] = {1, timestamp_ms};
        }
    }

    void onRemove(std::string_view key) override {
        frequency_map_.erase(std::string(key));
    }

    std::optional<std::string> selectVictim() override {
        if (frequency_map_.empty()) {
            return std::nullopt;
        }

        // Find entry with lowest frequency (tie-break by oldest access)
        auto victim_it = frequency_map_.begin();
        for (auto it = frequency_map_.begin(); it != frequency_map_.end(); ++it) {
            if (it->second.count < victim_it->second.count ||
                (it->second.count == victim_it->second.count && 
                 it->second.last_access_ms < victim_it->second.last_access_ms)) {
                victim_it = it;
            }
        }
        
        return victim_it->first;
    }

    void clear() override {
        frequency_map_.clear();
    }

    size_t size() const override {
        return frequency_map_.size();
    }

    std::string_view getName() const override {
        return "LFU";
    }

private:
    struct FrequencyData {
        uint64_t count = 0;
        uint64_t last_access_ms;
    };

    std::unordered_map<std::string, FrequencyData> frequency_map_;

    uint64_t getCurrentTimeMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

/**
 * @brief TTL (Time To Live) eviction strategy.
 *
 * Evicts entries that have exceeded their TTL.
 * Also evicts oldest entry when no expired entries exist.
 */
class TTLEvictionStrategy : public IEvictionStrategy {
public:
    explicit TTLEvictionStrategy(uint64_t default_ttl_ms = 3600000)  // 1 hour default
        : default_ttl_ms_(default_ttl_ms) {}

    void onAccess([[maybe_unused]] std::string_view key) override {
        // TTL strategy metadata is timestamp-based and does not change on read.
    }

    void onInsert(std::string_view key, uint64_t timestamp_ms) override {
        timestamp_map_[std::string(key)] = timestamp_ms;
    }

    void onRemove(std::string_view key) override {
        timestamp_map_.erase(std::string(key));
    }

    std::optional<std::string> selectVictim() override {
        if (timestamp_map_.empty()) {
            return std::nullopt;
        }

        uint64_t now = getCurrentTimeMs();
        
        // First, look for expired entries
        for (const auto& [key, timestamp] : timestamp_map_) {
            if (now - timestamp > default_ttl_ms_) {
                return key;
            }
        }

        // If no expired entries, evict oldest
        auto oldest_it = timestamp_map_.begin();
        for (auto it = timestamp_map_.begin(); it != timestamp_map_.end(); ++it) {
            if (it->second < oldest_it->second) {
                oldest_it = it;
            }
        }
        
        return oldest_it->first;
    }

    void clear() override {
        timestamp_map_.clear();
    }

    size_t size() const override {
        return timestamp_map_.size();
    }

    std::string_view getName() const override {
        return "TTL";
    }

    void setDefaultTTL(uint64_t ttl_ms) {
        default_ttl_ms_ = ttl_ms;
    }

private:
    uint64_t default_ttl_ms_;
    std::unordered_map<std::string, uint64_t> timestamp_map_;

    uint64_t getCurrentTimeMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
};

/**
 * @brief TwoTier eviction strategy.
 *
 * Combines two strategies: a fast L1 (e.g., LRU) and slower L2 (e.g., LFU).
 * Insertions are routed to L1 until full, then directly to L2.
 * Eviction prefers L2 first, then falls back to L1.
 */
class TwoTierEvictionStrategy : public IEvictionStrategy {
public:
    TwoTierEvictionStrategy(
        std::unique_ptr<IEvictionStrategy> l1_strategy,
        std::unique_ptr<IEvictionStrategy> l2_strategy,
        size_t l1_capacity
    ) : l1_strategy_(std::move(l1_strategy)),
        l2_strategy_(std::move(l2_strategy)),
        l1_capacity_(l1_capacity) {}

    void onAccess(std::string_view key) override {
        l1_strategy_->onAccess(key);
        l2_strategy_->onAccess(key);
    }

    void onInsert(std::string_view key, uint64_t timestamp_ms) override {
        // New entries go to L1 until it reaches configured capacity.
        if (l1_strategy_->size() < l1_capacity_) {
            l1_strategy_->onInsert(key, timestamp_ms);
        } else {
            l2_strategy_->onInsert(key, timestamp_ms);
        }
    }

    void onRemove(std::string_view key) override {
        l1_strategy_->onRemove(key);
        l2_strategy_->onRemove(key);
    }

    std::optional<std::string> selectVictim() override {
        // Prefer evicting from L2 (slower tier).
        auto l2_victim = l2_strategy_->selectVictim();
        if (l2_victim) {
            return l2_victim;
        }
        
        // Fall back to L1 if L2 is empty
        return l1_strategy_->selectVictim();
    }

    void clear() override {
        l1_strategy_->clear();
        l2_strategy_->clear();
    }

    size_t size() const override {
        return l1_strategy_->size() + l2_strategy_->size();
    }

    std::string_view getName() const override {
        return "TwoTier";
    }

private:
    std::unique_ptr<IEvictionStrategy> l1_strategy_;
    std::unique_ptr<IEvictionStrategy> l2_strategy_;
    size_t l1_capacity_;
};

/**
 * @brief ARC (Adaptive Replacement Cache) eviction strategy.
 *
 * Implements the ARC algorithm (Megiddo & Modha, FAST '03) as a pure
 * victim-selector that can be plugged into any key/value cache tier.
 *
 * Maintains four lists:
 *   T1  Recently-seen keys currently tracked (recency queue).
 *   T2  Frequently-seen keys currently tracked (frequency queue).
 *   B1  Ghost keys evicted from T1 (no data, adaptation signal only).
 *   B2  Ghost keys evicted from T2 (no data, adaptation signal only).
 *
 * The partition target `p` self-tunes: B1 hit → p increases (favour recency);
 * B2 hit → p decreases (favour frequency).
 *
 * This implementation uses unordered ghost sets (`B1`, `B2`) rather than
 * ordered ghost queues, so ghost-entry trimming does not preserve strict ARC
 * ordering guarantees but retains the core adaptation signal.
 */
class ARCEvictionStrategy : public IEvictionStrategy {
public:
    explicit ARCEvictionStrategy(size_t capacity = 128)
        : capacity_(capacity > 0 ? capacity : 128), p_(0) {}

    void onAccess(std::string_view key) override {
        std::string k(key);
        // T1 hit → promote to T2
        auto it1 = t1_map_.find(k);
        if (it1 != t1_map_.end()) {
            t2_list_.push_front(k);
            t2_map_[k] = t2_list_.begin();
            t1_list_.erase(it1->second);
            t1_map_.erase(it1);
            return;
        }
        // T2 hit → move to MRU end
        auto it2 = t2_map_.find(k);
        if (it2 != t2_map_.end()) {
            t2_list_.splice(t2_list_.begin(), t2_list_, it2->second);
        }
    }

    void onInsert(std::string_view key, uint64_t /*timestamp_ms*/) override {
        std::string k(key);
        // Already live — treat as access
        if (t1_map_.count(k) || t2_map_.count(k)) {
            onAccess(key);
            return;
        }
        // Ghost hit in B1: raise p (favour recency)
        if (b1_set_.count(k)) {
            size_t b1sz = b1_set_.size();
            size_t b2sz = b2_set_.size();
            size_t delta = (b1sz == 0 || b2sz >= b1sz) ? 1 : b2sz / b1sz;
            p_ = std::min(p_ + std::max<size_t>(delta, 1), capacity_);
            b1_set_.erase(k);
            t2_list_.push_front(k);
            t2_map_[k] = t2_list_.begin();
            return;
        }
        // Ghost hit in B2: lower p (favour frequency)
        if (b2_set_.count(k)) {
            size_t b1sz = b1_set_.size();
            size_t b2sz = b2_set_.size();
            size_t delta = (b2sz == 0 || b1sz >= b2sz) ? 1 : b1sz / b2sz;
            size_t step  = std::max<size_t>(delta, 1);
            p_ = (p_ >= step) ? p_ - step : 0;
            b2_set_.erase(k);
            t2_list_.push_front(k);
            t2_map_[k] = t2_list_.begin();
            return;
        }
        // New key: insert into T1 (recency queue)
        t1_list_.push_front(k);
        t1_map_[k] = t1_list_.begin();
    }

    void onRemove(std::string_view key) override {
        std::string k(key);
        // Evicted from T1 → moves to B1 ghost
        auto it1 = t1_map_.find(k);
        if (it1 != t1_map_.end()) {
            t1_list_.erase(it1->second);
            t1_map_.erase(it1);
            b1_set_.insert(k);
            if (b1_set_.size() > capacity_) {
                b1_set_.erase(b1_set_.begin());
            }
            return;
        }
        // Evicted from T2 → moves to B2 ghost
        auto it2 = t2_map_.find(k);
        if (it2 != t2_map_.end()) {
            t2_list_.erase(it2->second);
            t2_map_.erase(it2);
            b2_set_.insert(k);
            if (b2_set_.size() > capacity_) {
                b2_set_.erase(b2_set_.begin());
            }
            return;
        }
        // Not live — clean up any ghost reference
        b1_set_.erase(k);
        b2_set_.erase(k);
    }

    std::optional<std::string> selectVictim() override {
        if (t1_list_.empty() && t2_list_.empty()) {
            return std::nullopt;
        }
        // ARC policy: prefer evicting from T1 when |T1| > p or T2 is empty
        if (!t1_list_.empty() && (t1_list_.size() > p_ || t2_list_.empty())) {
            return t1_list_.back();
        }
        if (!t2_list_.empty()) {
            return t2_list_.back();
        }
        return t1_list_.back();
    }

    void clear() override {
        t1_list_.clear(); t1_map_.clear();
        t2_list_.clear(); t2_map_.clear();
        b1_set_.clear();  b2_set_.clear();
        p_ = 0;
    }

    size_t size() const override {
        return t1_map_.size() + t2_map_.size();
    }

    std::string_view getName() const override { return "ARC"; }

private:
    using List = std::list<std::string>;
    using Map  = std::unordered_map<std::string, std::list<std::string>::iterator>;

    size_t capacity_;
    size_t p_;

    List t1_list_, t2_list_;
    Map  t1_map_,  t2_map_;
    std::unordered_set<std::string> b1_set_, b2_set_;
};

} // namespace concerns
} // namespace core
} // namespace themis
