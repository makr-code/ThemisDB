/**
 * @file grammar_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/grammar_cache.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

GrammarCache::GrammarCache()
    : GrammarCache(Config{}) {
}

GrammarCache::GrammarCache(const Config& config)
    : config_(config) {
    
    spdlog::debug("GrammarCache initialized with max_cached_grammars={}, enabled={}", 
                  config_.max_cached_grammars, config_.enabled);
}

std::shared_ptr<Grammar> GrammarCache::get(const std::string& name) const {
    if (!config_.enabled) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        spdlog::debug("Grammar cache HIT for '{}'", name);
        return it->second;
    }
    
    spdlog::debug("Grammar cache MISS for '{}'", name);
    return nullptr;
}

bool GrammarCache::put(const std::string& name, std::shared_ptr<Grammar> grammar) {
    if (!config_.enabled) {
        return false;
    }
    
    if (!grammar) {
        spdlog::warn("Attempted to cache null grammar with name '{}'", name);
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if cache is full
    if (static_cast<int>(cache_.size()) >= config_.max_cached_grammars && cache_.find(name) == cache_.end()) {
        spdlog::warn("Grammar cache is full ({}), cannot add '{}'", 
                     config_.max_cached_grammars, name);
        return false;
    }
    
    cache_[name] = grammar;
    spdlog::debug("Cached grammar '{}' (cache size: {})", name,static_cast<int>(cache_.size()));
    return true;
}

void GrammarCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    spdlog::debug("Grammar cache cleared");
}

size_t GrammarCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(cache_.size());
}

bool GrammarCache::contains(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.find(name) != cache_.end();
}

bool GrammarCache::remove(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        cache_.erase(it);
        spdlog::debug("Removed grammar '{}' from cache", name);
        return true;
    }
    
    return false;
}

} // namespace llm
} // namespace themis
