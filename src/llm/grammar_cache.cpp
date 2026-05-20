/*
 * ThemisDB | File: grammar_cache.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 97
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=31 | delta=28 | status=divergent
 * External Severity (v3): C=7, H=23, M=1
 * PR: #2962 feat(llm): Implement JSON schema binding and tool/function calling ... (2026-03-12T06:07:04Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
