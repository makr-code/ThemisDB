/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grammar_cache.h                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:33:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7ff413ebb  2026-02-17  Add test certificates for CA and plugin signer ║
    • d13bfcea9  2026-01-05  Implement Grafana Metrics Exporter and Dashboard Generator ║
    • 3a5ea7949  2026-01-05  Add grammar-constrained generation infrastructure (Phase ... ║
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
