/**
 * @file stopwords.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

class Stopwords {
public:
    // Returns a default stopword set for a given language code ("en", "de", "none").
    static std::unordered_set<std::string> defaults(const std::string& language);
    
    // Merge default stopwords with a custom list (both assumed lowercase)
    static std::unordered_set<std::string> merge(const std::unordered_set<std::string>& base,
                                                 const std::vector<std::string>& custom);
};

} // namespace utils
} // namespace themis
