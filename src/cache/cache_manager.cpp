/**
 * @file cache_manager.cpp
 * @brief Cache manager implementation with policy coordination and move semantics
 * @version 0.1.0
 * @note Gap Fix: CWE-457, CWE-672
 */

#include "cache/cache_manager.h"
#include "cache/cache_eviction_policy.h"
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include "utils/logger.h"

namespace themis {
namespace cache {

// =============================================================================
// CacheManager Implementation
// =============================================================================

CacheManager::CacheManager(const CacheManagerConfig& config)
    : config_(config),
      next_handler_id_(1),
      is_moved_from_(false) {
    
    if (config.default_cache_size == 0) {
        throw std::invalid_argument("default_cache_size must be > 0");
    }
}

CacheManager::~CacheManager() noexcept {
    cleanup();
}

CacheManager::CacheManager(CacheManager&& other) noexcept
    : config_(other.config_),
      caches_(std::move(other.caches_)),
      event_handlers_(std::move(other.event_handlers_)),
      next_handler_id_(other.next_handler_id_),
      is_moved_from_(false) {
    
    other.is_moved_from_ = true;
}

CacheManager& CacheManager::operator=(CacheManager&& other) noexcept {
    if (this == &other || other.is_moved_from_) {
        return *this;
    }

    cleanup();

    config_ = other.config_;
    caches_ = std::move(other.caches_);
    event_handlers_ = std::move([[maybe_unused]] other.event_handlers_);
    next_handler_id_ = other.next_handler_id_;
    is_moved_from_ = false;

    other.is_moved_from_ = true;

    return *this;
}

bool CacheManager::register_cache(const std::string& cache_name, size_t size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot register cache on moved-from manager");
    }

    if (cache_name.empty()) {
        throw std::invalid_argument("Cache name cannot be empty");
    }

    if (caches_.find(cache_name) != caches_.end()) {
        return false;  // Already registered
    }

    CacheEntry entry;
    entry.name = cache_name;
    entry.size = (size > 0) ? size : config_.default_cache_size;

    caches_[cache_name] = std::move(entry);

    // Dispatch registration event
    CacheEvent event;
    event.type = CacheEvent::Type::CLEAR;
    event.cache_name = cache_name;
    dispatch_event([[maybe_unused]] event);

    return true;
}

bool CacheManager::unregister_cache(const std::string& cache_name) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot unregister cache on moved-from manager");
    }

    auto it = caches_.find(cache_name);
    if (it == caches_.end()) {
        return false;  // Not found
    }

    caches_.erase(it);
    return true;
}

std::vector<std::string> CacheManager::get_cache_names() const {
    std::vector<std::string> names = {};

    for (const auto& pair : caches_) {
        names.push_back(pair.first);
    }
    return names;
}

bool CacheManager::set_eviction_policy(const std::string& cache_name, 
                                       CacheEvictionPolicy&& policy) noexcept {
    if (is_moved_from_) {
        return false;
    }

    auto it = caches_.find(cache_name);
    if (it == caches_.end()) {
        return false;
    }

    it->second.policy = policy.clone();

    CacheEvent event;
    event.type = CacheEvent::Type::POLICY_CHANGE;
    event.cache_name = cache_name;
    event.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    dispatch_event([[maybe_unused]] event);
    return true;
}

const CacheEvictionPolicy* CacheManager::get_eviction_policy(const std::string& cache_name) const {
    auto it = caches_.find(cache_name);
    if (it == caches_.end()) {
        return nullptr;
    }
    return it->second.policy.get();
}

uint32_t CacheManager::register_event_handler([[maybe_unused]] EventHandler&& handler) {
    if (is_moved_from_) {
        throw std::logic_error([[maybe_unused]] "Cannot register handler on moved-from manager");
    }

    EventHandlerEntry entry;
    entry.id = next_handler_id_++;
    entry.handler = std::move([[maybe_unused]] handler);

    event_handlers_.push_back([[maybe_unused]] std::move(entry));
    return entry.id;
}

bool CacheManager::unregister_event_handler([[maybe_unused]] uint32_t handler_id) {
    auto it = std::find_if(event_handlers_.begin(), event_handlers_.end(),
                          [handler_id]([[maybe_unused]] const EventHandlerEntry& e) { 
                              return e.id == handler_id; 
                          });

    if ([[maybe_unused]] it == event_handlers_.end()) {
        return false;
    }

    event_handlers_.erase([[maybe_unused]] it);
    return true;
}

void CacheManager::dispatch_event([[maybe_unused]] const CacheEvent& event) noexcept {
    if (is_moved_from_) {
        return;
    }

    for ([[maybe_unused]] auto& entry : event_handlers_) {
        try {
            if ([[maybe_unused]] entry.handler) {
                entry.handler([[maybe_unused]] event);
            }
        } catch (...) {
            THEMIS_WARN("cache_manager: unhandled exception caught");
            // Log but continue processing other handlers
        }
    }
}

std::optional<CacheManager::CacheStats> CacheManager::get_cache_stats(const std::string& cache_name) const {
    auto it = caches_.find(cache_name);
    if (it == caches_.end()) {
        return std::nullopt;
    }

    CacheStats stats;
    stats.capacity = it->second.size;
    // Would need actual cache implementation to fill hits/misses/etc.
    return stats;
}

void CacheManager::clear_all() {
    if (is_moved_from_) {
        throw std::logic_error("Cannot clear on moved-from manager");
    }

    for (auto& pair : caches_) {
        CacheEvent event;
        event.type = CacheEvent::Type::CLEAR;
        event.cache_name = pair.first;
        dispatch_event([[maybe_unused]] event);
    }
}

void CacheManager::cleanup() noexcept {
    caches_.clear();
    event_handlers_.clear();
}

} // namespace cache
} // namespace themis
