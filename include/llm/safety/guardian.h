/**
 * @file guardian.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>

namespace themis::llm::safety {

struct GuardDecision {
    bool allowed = true;
    std::string sanitized_prompt;
    std::string reason;
    std::vector<std::string> matched_topics;
};

class PromptGuardian {
public:
    GuardDecision evaluate(const std::string& prompt) const;

private:
    static std::string normalize(const std::string& text);
    static bool containsContextualRisk(const std::string& normalized,
                                       std::vector<std::string>& matched_topics,
                                       std::string& reason);
};

} // namespace themis::llm::safety
