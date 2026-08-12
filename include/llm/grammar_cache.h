/**
 * @file grammar_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    virtual ~GrammarCache() = default;
    /**
     * @brief Configuration for grammar cache
     */
    struct Config {
        size_t max_cached_grammars = 0;  // Maximum number of grammars to cache
        bool enabled = false;                 // Enable caching
        
        Config() : max_cached_grammars(100), enabled(true) {}
    };
    
    /**
     * @brief Construct grammar cache with default configuration.
     */
    GrammarCache();
    /**
     * @brief Construct grammar cache with configuration.
     * @param config Cache configuration.
     */
    explicit GrammarCache(const Config& config);
    
    /**
     * @brief Get a grammar from cache by name
     * @param name Grammar name (e.g., "json_strict", "xml")
     * @return Shared pointer to grammar or nullptr if not found
     */
    std::shared_ptr<Grammar> get(const std::string& name) const;
    
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
