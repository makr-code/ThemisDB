/**
 * @file grammar_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class GrammarCache {
public:
    /**
     * @brief Grammar Cache.
     * @return Return value.
     */
    virtual ~GrammarCache() = default;
    struct Config {
        size_t max_cached_grammars = 0;  // Maximum number of grammars to cache
        bool enabled = false;                 // Enable caching
        
        Config() : max_cached_grammars(100), enabled(true) {}
    };
    
    GrammarCache();
    /**
     * @brief Grammar Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GrammarCache(const Config& config);
    
    /**
     * @brief Get.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Grammar> get(const std::string& name) const;
    
    /**
     * @brief Put.
     * @param[in] name Input parameter.
     * @param[in] grammar Input parameter.
     * @return True when the operation succeeds.
     */
    bool put(const std::string& name, std::shared_ptr<Grammar> grammar);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;
    
    /**
     * @brief Contains.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool contains(const std::string& name) const;
    
    /**
     * @brief Remove.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& name);
    
private:
    Config config_;
    std::unordered_map<std::string, std::shared_ptr<Grammar>> cache_;
    mutable std::mutex mutex_;
};

} // namespace llm
} // namespace themis
