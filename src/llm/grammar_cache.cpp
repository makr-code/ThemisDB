/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grammar_cache.cpp                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:26:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

std::shared_ptr<Grammar> GrammarCache::get(const std::string& name) {
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
    if (cache_.size() >= config_.max_cached_grammars && cache_.find(name) == cache_.end()) {
        spdlog::warn("Grammar cache is full ({}), cannot add '{}'", 
                     config_.max_cached_grammars, name);
        return false;
    }
    
    cache_[name] = grammar;
    spdlog::debug("Cached grammar '{}' (cache size: {})", name, cache_.size());
    return true;
}

void GrammarCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    spdlog::debug("Grammar cache cleared");
}

size_t GrammarCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
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
