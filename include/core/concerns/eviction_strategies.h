/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            eviction_strategies.h                              ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     322                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/cache_strategies.h"
#include <unordered_map>
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

    void onInsert(std::string_view key, uint64_t timestamp_ms) override {
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
        uint64_t count;
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

    void onAccess(std::string_view key) override {
        // TTL strategy doesn't change on access
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
 * Eviction first tries L1, then L2 if L1 is empty.
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
        // New entries go to L1
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
        // Prefer evicting from L2 (slower tier)
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

} // namespace concerns
} // namespace core
} // namespace themis
