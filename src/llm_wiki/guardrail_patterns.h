// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file guardrail_patterns.h
 * @brief Prompt injection and unsafe content guardrail pattern detection.
 *
 * Provides compile-time and runtime pattern matching for detecting:
 *  - Shell command injection ("sudo", "rm -rf", "exec", etc.)
 *  - Code evaluation patterns ("eval(", "exec(", "__import__", etc.)
 *  - Encoding bypass ("base64 decode", "hex decode", etc.)
 *  - Privilege escalation / privilege confusion patterns
 *  - Direct control flow redirection (goto, setjmp abuses, etc.)
 *
 * All patterns are matched case-insensitively. Whitespace normalization
 * (collapsing runs of whitespace) is applied before matching.
 *
 * ## Usage
 *
 * @code
 *   WikiGuardrails guardrails;
 *   if (guardrails.isUnsafeQuery("tell me sudo commands")) {
 *       result.query_flagged_for_prompt_injection = true;
 *   }
 *   if (guardrails.isUnsafeContent(chunk_text)) {
 *       result.filtered_unsafe_chunks++;
 *   }
 * @endcode
 *
 * @version 1.0.0 (Phase 3 hardening)
 */

#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace themis {
namespace llm_wiki {

// ============================================================================
// Pattern registry
// ============================================================================

namespace guardrail_patterns {

// Shell command execution
static constexpr std::string_view kShellPatterns[] = {
    "sudo",      "su ",       "chmod",      "chown",       "rm ",
    "rm-rf",     "rmdir",     "dd if=",     "mkfs",        "mount",
    "umount",    "kill",      "pkill",      "service",     "/bin/bash",
    "/bin/sh",   "bash -c",   "sh -c",      "nc -l",       "netcat",
    "curl|",     "wget|",     "cat|",       "grep|",       "awk|",
};

// Code execution and injection
static constexpr std::string_view kCodeExecutionPatterns[] = {
    "eval(",      "exec(",       "__import__",    "compile(",
    "exec_code",  "execute_code", "subprocess",    "popen(",
    "system(",    "os.system",   "popen",         "execvp",
    "spawn",      "fork",        "clone",         "dlopen(",
};

// Encoding bypass / obfuscation
static constexpr std::string_view kEncodingPatterns[] = {
    "base64 decode",   "hex decode",       "url decode",    "unicode decode",
    "rot13",           "cipher",           "obfuscat",      "rot-13",
    "base 64",         "b64",              "unescape",      "unhex",
};

// Privilege and trust confusion
static constexpr std::string_view kPrivilegePatterns[] = {
    "setuid",      "setgid",         "sudo",         "admin",
    "root",        "superuser",      "privilege",    "credential",
    "password",    "token",          "secret",       "key",
    "grant",       "permission",
};

// Control flow redirection
static constexpr std::string_view kControlFlowPatterns[] = {
    "goto",        "longjmp",        "setjmp",       "jmp_buf",
    "signal",      "handler",        "interrupt",    "trap",
    "atexit",      "on_exit",        "abort",        "exit_code",
};

} // namespace guardrail_patterns

// ============================================================================
// Normalization helpers
// ============================================================================

/**
 * @brief Normalize For Guardrail Check.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), std::isspace(), push_back(), std::tolower(), empty(), back(), pop_back().
 */
inline std::string normalizeForGuardrailCheck(std::string_view text) {
    std::string result = {};
    result.reserve(text.size());
    
    bool in_space = true;  // treat leading whitespace as "in space"
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!in_space) {
                result.push_back(' ');
                in_space = true;
            }
        } else {
            const int lowered = std::tolower(static_cast<unsigned char>(c));
            result.push_back(static_cast<char>(lowered));
            in_space = false;
        }
    }
    
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

// ============================================================================
// Guardrail check interface
// ============================================================================

class WikiGuardrails {
public:
    WikiGuardrails() = default;
    ~WikiGuardrails() = default;

    // Non-copyable, non-movable (stateless singleton pattern).
    WikiGuardrails(const WikiGuardrails&) = delete;
    WikiGuardrails& operator=(const WikiGuardrails&) = delete;
    WikiGuardrails(WikiGuardrails&&) = delete;
    WikiGuardrails& operator=(WikiGuardrails&&) = delete;

    [[nodiscard]] bool isUnsafeQuery(std::string_view query_text) const noexcept {
        return checkPatterns(query_text);
    }

    [[nodiscard]] bool isUnsafeContent(std::string_view chunk_text) const noexcept {
        // Phase 3: Content uses same guardrails as query.
        // Phase 5: May add content-specific allowlists (e.g., markdown code blocks).
        return checkPatterns(chunk_text);
    }

private:
    [[nodiscard]] bool checkPatterns(std::string_view text) const noexcept {
        std::string normalized = normalizeForGuardrailCheck(text);
        
        // Check shell patterns
        for (auto pattern : guardrail_patterns::kShellPatterns) {
            if (normalized.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Check code execution patterns
        for (auto pattern : guardrail_patterns::kCodeExecutionPatterns) {
            if (normalized.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Check encoding bypass patterns
        for (auto pattern : guardrail_patterns::kEncodingPatterns) {
            if (normalized.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Check privilege patterns
        for (auto pattern : guardrail_patterns::kPrivilegePatterns) {
            if (normalized.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        // Check control flow patterns
        for (auto pattern : guardrail_patterns::kControlFlowPatterns) {
            if (normalized.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
};

} // namespace llm_wiki
} // namespace themis
