/**
 * @file guardian.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

/** @brief Prompt guardian. */
class PromptGuardian {
public:
    /**
     * @brief TBD: Describe evaluate.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    GuardDecision evaluate(const std::string& prompt) const;

private:
    /**
     * @brief TBD: Describe normalize.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string normalize(const std::string& text);
    /**
     * @brief TBD: Describe containsContextualRisk.
     * @param[in] normalized Input parameter.
     * @param[in,out] matched_topics Input/output parameter.
     * @param[in,out] reason Input/output parameter.
     * @return True on success.
     */
    static bool containsContextualRisk(const std::string& normalized,
                                       std::vector<std::string>& matched_topics,
                                       std::string& reason);
};

} // namespace themis::llm::safety
