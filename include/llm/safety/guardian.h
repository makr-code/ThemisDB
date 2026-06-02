/*
 * ThemisDB | File: guardian.h | Version: 0.0.1 | Last Modified: 2026-06-01 11:06:12
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 26
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
