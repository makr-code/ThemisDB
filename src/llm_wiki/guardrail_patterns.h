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

/**
 * @brief Canonical guardrail pattern library.
 *
 * Organized by attack category. Each pattern is a substring that, if detected
 * in normalized query/content, flags the input as unsafe.
 *
 * Normalization: all lowercase, runs of whitespace collapsed to single space.
 */
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
 * @brief Normalize text for pattern matching: lowercase + whitespace collapse.
 *
 * @param text  Input text.
 * @return      Normalized copy (lowercase, runs of whitespace → single space).
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
            result.push_back(std::tolower(static_cast<unsigned char>(c)));
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

/**
 * @brief Primary guardrail checker for the LLM Wiki plugin.
 *
 * Thread-safe (no mutable state; read-only pattern arrays).
 */
class WikiGuardrails {
public:
    WikiGuardrails() = default;
    ~WikiGuardrails() = default;

    // Non-copyable, non-movable (stateless singleton pattern).
    WikiGuardrails(const WikiGuardrails&) = delete;
    WikiGuardrails& operator=(const WikiGuardrails&) = delete;
    WikiGuardrails(WikiGuardrails&&) = delete;
    WikiGuardrails& operator=(WikiGuardrails&&) = delete;

    /**
     * @brief Check if a query contains prompt-injection patterns.
     *
     * A query is flagged as unsafe if the normalized text contains any
     * guardrail pattern from the registries.
     *
     * @param query_text  User-supplied query string.
     * @return            True if dangerous patterns detected.
     */
    [[nodiscard]] bool isUnsafeQuery(std::string_view query_text) const noexcept {
        return checkPatterns(query_text);
    }

    /**
     * @brief Check if a content chunk contains unsafe patterns.
     *
     * Content guardrails are typically more lenient than query guardrails
     * (e.g., a page titled "Understanding sudo" is safe content but
     * a query requesting "show me how to use sudo" is flagged).
     *
     * For now, uses the same pattern set; this can be customized per phase.
     *
     * @param chunk_text  Content text to check.
     * @return            True if dangerous patterns detected.
     */
    [[nodiscard]] bool isUnsafeContent(std::string_view chunk_text) const noexcept {
        // Phase 3: Content uses same guardrails as query.
        // Phase 5: May add content-specific allowlists (e.g., markdown code blocks).
        return checkPatterns(chunk_text);
    }

private:
    /**
     * @brief Internal pattern matching against all registries.
     *
     * @param text  Input text (not normalized yet).
     * @return      True if any pattern matches (case-insensitive, with whitespace normalization).
     */
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
