/**
 * @file bounded_lru_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/bounded_lru_cache.h"

#include <mutex>

#include "utils/logger.h"

namespace themis {
namespace cache {

BoundedLRUCache::BoundedLRUCache(const Config &config) : head_(nullptr), tail_(nullptr), config_(config) {}

BoundedLRUCache::~BoundedLRUCache() {
    clear();
}

std::optional<nlohmann::json> BoundedLRUCache::get(const std::string &key) {
    // Fast miss path: a shared (read) lock is sufficient to check whether the
    // key is absent.  This allows concurrent reads without any write contention.
    {
        std::shared_lock<std::shared_mutex> rlock(mutex_);
        if (cache_.find(key) == cache_.end()) {
            if (config_.enable_statistics) {
                misses_.fetch_add(1, std::memory_order_relaxed);
            }
            return std::nullopt;
        }
    }

    // Hit or possibly-expired path: need an exclusive lock to update last_access,
    // call moveToFront, or remove an expired entry.  Re-check under the write lock
    // because the entry may have been evicted between the two lock acquisitions.
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it == cache_.end()) {
        // Expected race: another thread may have evicted this entry between
        // releasing the shared_lock and acquiring the unique_lock.  Treat as a
        // miss — this is correct and not a bug.
        if (config_.enable_statistics) {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }
        return std::nullopt;
    }

    auto node = it->second;
    if (!node) [[unlikely]] {
        // Defensive: shared_ptr in cache_ should never be null, but guard anyway.
        cache_.erase(it);
        if (config_.enable_statistics) {
            misses_.fetch_add(1, std::memory_order_relaxed);
        }
        return std::nullopt;
    }

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

void BoundedLRUCache::put(const std::string &key, nlohmann::json value, uint32_t ttl_seconds) {
    // C4: AI/LLM safety — validate entry size and TTL before acquiring the lock.
    // Serialise once here; the string is used as a size proxy (not stored).
    {
        const std::string serialized = value.dump();
        if (serialized.size() > config_.max_entry_size_bytes) {
            THEMIS_WARN("{{\"event\":\"entry_size_exceeded\",\"operation\":\"bounded_lru_put\","
                        "\"size_bytes\":{},\"max_bytes\":{}}}",
                        serialized.size(), config_.max_entry_size_bytes);
            return; // INVALID_ARGUMENT: entry too large
        }
    }
    if (ttl_seconds > 0 && ttl_seconds > config_.max_ttl_seconds) {
        THEMIS_WARN("{{\"event\":\"invalid_ttl\",\"operation\":\"bounded_lru_put\","
                    "\"ttl_seconds\":{},\"max_ttl_seconds\":{}}}",
                    ttl_seconds, config_.max_ttl_seconds);
        return; // INVALID_ARGUMENT: TTL exceeds cap
    }

    const auto ttl = (ttl_seconds > 0) ? std::chrono::seconds(ttl_seconds) : config_.ttl;

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // Update existing entry
        auto node = it->second;
        if (!node) [[unlikely]] {
            // Defensive: corrupt map entry — remove and fall through to insert.
            cache_.erase(it);
        } else {
            node->entry.value       = std::move(value);
            node->entry.expiry      = std::chrono::steady_clock::now() + ttl;
            node->entry.last_access = std::chrono::steady_clock::now();
            moveToFront(node);
            return;
        }
    }

    // Evict if at capacity
    if (cache_.size() >= config_.max_entries) {
        removeLRU();
    }

    // Create new entry
    CacheEntry entry;
    entry.value       = std::move(value);
    entry.expiry      = std::chrono::steady_clock::now() + ttl;
    entry.last_access = std::chrono::steady_clock::now();

    // Create new node
    auto node   = std::make_shared<Node>();
    node->key   = key;
    node->entry = std::move(entry);

    // Add to cache and list
    cache_[key] = node;
    addToFront(node);
}

bool BoundedLRUCache::remove(const std::string &key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return false;
    }

    auto node = it->second;
    if (!node) [[unlikely]] {
        // Defensive: corrupt map entry — erase and report success.
        cache_.erase(it);
        return true;
    }
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
    stats.hits         = hits_.load(std::memory_order_relaxed);
    stats.misses       = misses_.load(std::memory_order_relaxed);

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
    if (!node) [[unlikely]] {
        return;
    }
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
    if (!node) [[unlikely]] {
        return;
    }
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
    // Break shared_ptr cycles to allow immediate ref-count release.
    node->prev = nullptr;
    node->next = nullptr;
}

void BoundedLRUCache::addToFront(std::shared_ptr<Node> node) {
    if (!node) [[unlikely]] {
        return;
    }
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

    if (lru->prev) {
        tail_       = lru->prev;
        tail_->next = nullptr;
    } else {
        head_ = nullptr;
        tail_ = nullptr;
    }
    // Release the outgoing node's list links to break any shared_ptr cycles.
    lru->prev = nullptr;
    lru->next = nullptr;
}

bool BoundedLRUCache::isExpired(const CacheEntry &entry) const {
    return std::chrono::steady_clock::now() > entry.expiry;
}

bool BoundedLRUCache::contains(const std::string &key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return false;
    }
    if (!it->second) [[unlikely]] {
        return false;
    }
    return !isExpired(it->second->entry);
}

std::size_t BoundedLRUCache::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return static_cast<int>(cache_.size());
}

} // namespace cache
} // namespace themis
