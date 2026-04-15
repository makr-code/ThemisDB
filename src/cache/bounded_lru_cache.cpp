/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bounded_lru_cache.cpp                              ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/bounded_lru_cache.h"
#include <mutex>

namespace themis {
namespace cache {

BoundedLRUCache::BoundedLRUCache(const Config& config)
    : config_(config)
    , head_(nullptr)
    , tail_(nullptr) {
}

BoundedLRUCache::~BoundedLRUCache() {
    clear();
}

std::optional<nlohmann::json> BoundedLRUCache::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        if (config_.enable_statistics) {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }
        return std::nullopt;
    }
    
    auto node = it->second;
    
    // Check if expired
    if (isExpired(node->entry)) {
        // Remove expired entry
        removeNode(node);
        cache_.erase(it);
        if (config_.enable_statistics) {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }
        return std::nullopt;
    }
    
    // Update last access time
    node->entry.last_access = std::chrono::steady_clock::now();
    
    // Move to front (most recently used)
    moveToFront(node);
    
    if (config_.enable_statistics) {
        hits_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return node->entry.value;
}

void BoundedLRUCache::put(const std::string& key, const nlohmann::json& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // Update existing entry
        auto node = it->second;
        node->entry.value = value;
        node->entry.expiry = std::chrono::steady_clock::now() + config_.ttl;
        node->entry.last_access = std::chrono::steady_clock::now();
        moveToFront(node);
        return;
    }
    
    // Evict if at capacity
    if (cache_.size() >= config_.max_entries) {
        removeLRU();
    }
    
    // Create new entry
    CacheEntry entry;
    entry.value = value;
    entry.expiry = std::chrono::steady_clock::now() + config_.ttl;
    entry.last_access = std::chrono::steady_clock::now();
    
    // Create new node
    auto node = std::make_shared<Node>();
    node->key = key;
    node->entry = entry;
    
    // Add to cache and list
    cache_[key] = node;
    addToFront(node);
}

bool BoundedLRUCache::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return false;
    }
    
    auto node = it->second;
    removeNode(node);
    cache_.erase(it);
    
    return true;
}

bool BoundedLRUCache::evictLRUIfNeeded() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (cache_.size() >= config_.max_entries) {
        removeLRU();
        return true;
    }
    
    return false;
}

BoundedLRUCache::Statistics BoundedLRUCache::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    Statistics stats;
    stats.current_size = cache_.size();
    stats.hits = hits_.load(std::memory_order_relaxed);
    stats.misses = misses_.load(std::memory_order_relaxed);
    
    return stats;
}

void BoundedLRUCache::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    cache_.clear();
    head_ = nullptr;
    tail_ = nullptr;
    hits_.store(0, std::memory_order_relaxed);
    misses_.store(0, std::memory_order_relaxed);
}

void BoundedLRUCache::moveToFront(std::shared_ptr<Node> node) {
    if (node == head_) {
        // Already at front
        return;
    }
    
    // Remove from current position
    if (node->prev) {
        node->prev->next = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    if (node == tail_) {
        tail_ = node->prev;
    }
    
    // Add to front
    node->prev = nullptr;
    node->next = head_;
    if (head_) {
        head_->prev = node;
    }
    head_ = node;
    
    if (!tail_) {
        tail_ = node;
    }
}

void BoundedLRUCache::removeNode(std::shared_ptr<Node> node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        head_ = node->next;
    }
    
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        tail_ = node->prev;
    }
}

void BoundedLRUCache::addToFront(std::shared_ptr<Node> node) {
    node->prev = nullptr;
    node->next = head_;
    
    if (head_) {
        head_->prev = node;
    }
    head_ = node;
    
    if (!tail_) {
        tail_ = node;
    }
}

void BoundedLRUCache::removeLRU() {
    if (!tail_) {
        return;
    }
    
    auto lru = tail_;
    cache_.erase(lru->key);
    
    if (tail_->prev) {
        tail_ = tail_->prev;
        tail_->next = nullptr;
    } else {
        head_ = nullptr;
        tail_ = nullptr;
    }
}

bool BoundedLRUCache::isExpired(const CacheEntry& entry) const {
    return std::chrono::steady_clock::now() > entry.expiry;
}

} // namespace cache
} // namespace themis
