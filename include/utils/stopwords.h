/**
 * @file stopwords.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

/** @brief Stopwords. */
class Stopwords {
public:
    /**
     * @brief Returns a default stopword set for a given language code ("en", "de", "none").
     * @param[in] language Input parameter.
     * @return Return value.
     */
    static std::unordered_set<std::string> defaults(const std::string& language);
    
    /**
     * @brief Merge default stopwords with a custom list (both assumed lowercase)
     * @param[in] base Input parameter.
     * @param[in] custom Input parameter.
     * @return Return value.
     */
    static std::unordered_set<std::string> merge(const std::unordered_set<std::string>& base,
                                                 const std::vector<std::string>& custom);
};

} // namespace utils
} // namespace themis
