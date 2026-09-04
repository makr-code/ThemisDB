/**
 * @file llm_security_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/llm_security_utils.h"

namespace themis {
namespace llm {

std::string sanitizeApiKey(const std::string& api_key) {
    if (api_key.empty()) {
        return "<not set>";
    }
    constexpr size_t kVisible = 4;
    if (static_cast<int>(api_key.size()) <= kVisible * 2) {
        return std::string(api_key.size(), '*');
    }
    return api_key.substr(0, kVisible) +
           "***...***" +
           api_key.substr(api_key.size() - kVisible);
}

} // namespace llm
} // namespace themis
