/*
 * ThemisDB | File: sd_prompt_sanitizer.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace themis {
namespace imggen {

/**
 * @brief Content-policy sanitizer for image generation prompts.
 *
 * Blocks prompts containing any of the configured keywords (case-insensitive).
 * Can be loaded from a YAML/text file (one keyword per line) or constructed
 * directly from a vector of strings.
 */
class SDPromptSanitizer {
public:
    explicit SDPromptSanitizer(const std::vector<std::string>& blocked_keywords = {});

    /**
     * @brief Load blocked keywords from a plain-text file (one keyword per line).
     *        Lines starting with '#' are treated as comments.
     */
    static SDPromptSanitizer fromFile(const std::string& path);

    /** @return false if the prompt contains any blocked keyword. */
    bool isAllowed(const std::string& prompt) const;

    /**
     * @brief Remove all blocked keywords from the prompt.
     * @return Sanitized prompt string.
     */
    std::string sanitize(const std::string& prompt) const;

    size_t blockedCount() const { return blocked_keywords_.size(); }

private:
    std::vector<std::string> blocked_keywords_;  // stored lower-case

    static std::string toLower(const std::string& s);
};

} // namespace imggen
} // namespace themis
