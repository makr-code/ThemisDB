/**
 * @file guardian.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/safety/guardian.h"

#include "llm/prompt_safety_utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <unordered_map>

namespace themis::llm::safety {

namespace {

constexpr std::array<std::pair<std::string_view, std::array<std::string_view, 4>>, 4> kTopicSignals{{
    {"credential_theft", {"password", "token", "secret", "credential"}},
    {"explosive_harm", {"bomb", "explosive", "detonate", "ied"}},
    {"malware_delivery", {"ransomware", "malware", "backdoor", "keylogger"}},
    {"data_exfiltration", {"exfiltrate", "dump database", "steal data", "export pii"}},
}};

constexpr std::array<std::string_view, 10> kActionSignals{{
    "build", "craft", "create", "bypass", "steal", "exfiltrate", "detonate", "deploy", "disable", "evade"
}};

bool containsToken(const std::string& text, std::string_view token) {
    return text.find(token) != std::string::npos;
}

} // namespace

GuardDecision PromptGuardian::evaluate(const std::string& prompt) const {
    GuardDecision out;

    std::string blocked_rule = {};
    std::string blocked_reason = {};
    if (!prompt_safety::sanitizePromptWithSharedPolicy(prompt, out.sanitized_prompt, &blocked_rule, &blocked_reason)) {
        out.allowed = false;
        out.reason = blocked_rule + ": " + blocked_reason;
        return out;
    }

    const std::string normalized = normalize(out.sanitized_prompt);
    if (containsContextualRisk(normalized, out.matched_topics, out.reason)) {
        out.allowed = false;
    }

    return out;
}

std::string PromptGuardian::normalize(const std::string& text) {
    static const std::unordered_map<char, char> leet_map = {
        {'0', 'o'}, {'1', 'i'}, {'3', 'e'}, {'4', 'a'}, {'5', 's'}, {'7', 't'}, {'@', 'a'}, {'$', 's'}
    };

    std::string out = {};
    out.reserve(text.size());

    bool previous_space = false;
    for (unsigned char raw : text) {
        char c = static_cast<char>(std::tolower(raw));
        if (auto it = leet_map.find(c); it != leet_map.end()) {
            c = it->second;
        }

        if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ') {
            if (c == ' ') {
                if (!previous_space) {
                    out.push_back(c);
                }
                previous_space = true;
            } else {
                out.push_back(c);
                previous_space = false;
            }
        } else if (!previous_space) {
            out.push_back(' ');
            previous_space = true;
        }
    }

    if (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }

    return out;
}

bool PromptGuardian::containsContextualRisk(const std::string& normalized,
                                            std::vector<std::string>& matched_topics,
                                            std::string& reason) {
    bool has_action = false;
    for (const auto action : kActionSignals) {
        if (containsToken(normalized, action)) {
            has_action = true;
            break;
        }
    }

    if (!has_action) {
        return false;
    }

    for (const auto& [topic, terms] : kTopicSignals) {
        for (const auto term : terms) {
            if (containsToken(normalized, term)) {
                matched_topics.emplace_back(topic);
                break;
            }
        }
    }

    if (matched_topics.empty()) {
        return false;
    }

    reason = "Prompt blocked due to contextual high-risk topic/action combination";
    return true;
}

} // namespace themis::llm::safety
