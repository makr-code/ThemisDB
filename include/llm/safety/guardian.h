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
