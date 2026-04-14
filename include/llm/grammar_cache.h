/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grammar_cache.h                                    ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:23:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     109                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/grammar.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace llm {

/**
 * @brief Cache for compiled grammars
 * 
 * Grammars can be expensive to compile, so we cache them by name.
 * This allows reusing grammars across multiple requests.
 * 
 * Thread-safe for concurrent access.
 */
class GrammarCache {
public:
    /**
     * @brief Configuration for grammar cache
     */
    struct Config {
        size_t max_cached_grammars;  // Maximum number of grammars to cache
        bool enabled;                 // Enable caching
        
        Config() : max_cached_grammars(100), enabled(true) {}
    };
    
    /**
     * @brief Construct grammar cache with configuration
     * @param config Cache configuration
     */
    GrammarCache();
    explicit GrammarCache(const Config& config);
    
    /**
     * @brief Get a grammar from cache by name
     * @param name Grammar name (e.g., "json_strict", "xml")
     * @return Shared pointer to grammar or nullptr if not found
     */
    std::shared_ptr<Grammar> get(const std::string& name);
    
    /**
     * @brief Put a grammar into cache
     * @param name Grammar name
     * @param grammar Grammar to cache
     * @return true if cached successfully, false if cache is full
     */
    bool put(const std::string& name, std::shared_ptr<Grammar> grammar);
    
    /**
     * @brief Clear all cached grammars
     */
    void clear();
    
    /**
     * @brief Get number of cached grammars
     * @return Number of grammars in cache
     */
    size_t size() const;
    
    /**
     * @brief Check if cache contains a grammar
     * @param name Grammar name
     * @return true if grammar exists in cache
     */
    bool contains(const std::string& name) const;
    
    /**
     * @brief Remove a specific grammar from cache
     * @param name Grammar name
     * @return true if removed, false if not found
     */
    bool remove(const std::string& name);
    
private:
    Config config_;
    std::unordered_map<std::string, std::shared_ptr<Grammar>> cache_;
    mutable std::mutex mutex_;
};

} // namespace llm
} // namespace themis
