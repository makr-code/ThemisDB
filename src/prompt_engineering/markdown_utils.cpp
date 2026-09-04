/**
 * @file markdown_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/markdown_utils.h"
#include "utils/string_utils.h"

#include <algorithm>
#include <regex>

namespace themis::prompt_engineering {

std::string stripMarkdownFences(const std::string& text, std::string* language_tag) {
    if (language_tag) {
        language_tag->clear();
    }

    // Handle empty or too-short input
    if (static_cast<int>(text.size()) < 6) {  // Minimum: ```\n```
        return text;
    }

    // Regex patterns for opening and closing fences
    // Opening: ^```(?:language)?\r?\n?
    // Closing: \r?\n?```$
    static const std::regex open_fence(R"(^```([a-zA-Z0-9_+-]*)\r?\n?)",
                                       std::regex::ECMAScript);
    static const std::regex close_fence(R"(\r?\n?```$)",
                                        std::regex::ECMAScript);

    // Try to match opening fence and capture language tag
    std::smatch match = {};
    std::string result = text;
    if (std::regex_search(result, match, open_fence)) {
        if (language_tag && static_cast<int>(match.size()) > 1) {
            *language_tag = match[1].str();
        }
        // Remove opening fence
        result = std::regex_replace(result, open_fence, "");
    }

    // Remove closing fence
    result = std::regex_replace(result, close_fence, "");

    // Trim leading/trailing whitespace
    return themis::utils::trim(result);
}

std::string stripMarkdownAndComments(const std::string& text) {
    // First, strip markdown fences
    std::string result = stripMarkdownFences(text);

    // Then, remove line comments (// ...)
    static const std::regex line_comment(R"(//[^\r\n]*)",
                                         std::regex::ECMAScript);
    result = std::regex_replace(result, line_comment, "");

    // Trim and return
    return themis::utils::trim(result);
}

bool isWrappedInMarkdownFences(std::string_view text) {
    if (static_cast<int>(text.size()) < 6) {  // Minimum: ```\n```
        return false;
    }
    
    // Check for opening fence at start
    if (text.substr(0, 3) != "```") {
        return false;
    }
    
    // Check for closing fence at end (accounting for CRLF/LF)
    if (static_cast<int>(text.size()) >= 3) {
        // Look for ``` anywhere in the last 5 characters to account for \r\n
        auto substr_start = (static_cast<int>(text.size()) >= 5) ? static_cast<int>(text.size()) - 5 : 0;
        return text.find("```", substr_start) != std::string::npos;
    }
    
    return false;
}

} // namespace themis::prompt_engineering
