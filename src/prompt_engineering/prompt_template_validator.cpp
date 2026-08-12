/**
 * @file prompt_template_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_template_validator.h"

#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <regex>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// Helper utilities for injection detection
// ============================================================================

static bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        }
    );
    return it != haystack.end();
}

// ============================================================================
// PromptTemplateValidator
// ============================================================================

PromptTemplateValidator::PromptTemplateValidator(bool require_id)
    : require_id_(require_id)
{}

TemplateValidationResult
PromptTemplateValidator::validate(const nlohmann::json& j) const {
    TemplateValidationResult result;

    // Must be an object at the top level
    if (!j.is_object()) {
        result.errors.push_back("Template JSON must be an object");
        result.valid = false;
        return result;
    }

    // --- id ---
    if (require_id_) {
        if (!j.contains("id") || !j["id"].is_string()) {
            result.errors.push_back("Field 'id' must be a non-empty string");
        } else if (j["id"].get<std::string>().empty()) {
            result.errors.push_back("Field 'id' must not be empty");
        }
    }

    // --- name ---
    if (!j.contains("name") || !j["name"].is_string()) {
        result.errors.push_back("Field 'name' must be a non-empty string");
    } else if (j["name"].get<std::string>().empty()) {
        result.errors.push_back("Field 'name' must not be empty");
    }

    // --- version ---
    if (!j.contains("version") || !j["version"].is_string()) {
        result.errors.push_back("Field 'version' must be a non-empty string");
    } else if (j["version"].get<std::string>().empty()) {
        result.errors.push_back("Field 'version' must not be empty");
    }

    // --- content ---
    if (!j.contains("content") || !j["content"].is_string()) {
        result.errors.push_back("Field 'content' must be a non-empty string");
    } else if (j["content"].get<std::string>().empty()) {
        result.errors.push_back("Field 'content' must not be empty");
    }

    // --- description (advisory) ---
    if (!j.contains("description") || !j["description"].is_string()) {
        result.errors.push_back("Field 'description' must be a string");
    } else if (j["description"].get<std::string>().empty()) {
        result.warnings.push_back("Field 'description' is empty – consider adding one");
    }

    // --- active ---
    if (!j.contains("active") || !j["active"].is_boolean()) {
        result.errors.push_back("Field 'active' must be a boolean");
    }

    // --- metadata ---
    if (j.contains("metadata")) {
        if (!j["metadata"].is_object() && !j["metadata"].is_null()) {
            result.errors.push_back("Field 'metadata' must be a JSON object or null");
        }
    }

    // --- images ---
    if (j.contains("images")) {
        if (!j["images"].is_array()) {
            result.errors.push_back("Field 'images' must be an array");
        } else {
            const auto& images = j["images"];
            for (std::size_t i = 0; i < images.size(); ++i) {
                const auto& img = images[i];
                if (!img.is_object()) {
                    result.errors.push_back(
                        "images[" + std::to_string(i) + "] must be an object");
                    continue;
                }
                if (!img.contains("alt_text") || !img["alt_text"].is_string() ||
                    img["alt_text"].get<std::string>().empty()) {
                    result.errors.push_back(
                        "images[" + std::to_string(i) +
                        "] 'alt_text' must be a non-empty string");
                }
            }
        }
    }

    result.valid = result.errors.empty();
    return result;
}

TemplateValidationResult
PromptTemplateValidator::validate(const std::string& json_str) const {
    TemplateValidationResult result;
    try {
        auto j = nlohmann::json::parse(json_str);
        result = validate(j);
        
        // Phase 3 hardening: Check content for injection patterns
        if (j.contains("content") && j["content"].is_string()) {
            auto injection_result = detectInjectionPatterns(j["content"].get<std::string>());
            result.warnings.insert(result.warnings.end(), 
                                   injection_result.warnings.begin(), 
                                   injection_result.warnings.end());
            // Injection detection adds errors if high-severity patterns found
            if (!injection_result.errors.empty()) {
                result.errors.insert(result.errors.end(),
                                     injection_result.errors.begin(),
                                     injection_result.errors.end());
                result.valid = false;
            }
        }
        
        return result;
    } catch (const nlohmann::json::parse_error& e) {
        result.valid = false;
        result.errors.push_back(std::string("JSON parse error: ") + e.what());
        return result;
    }
}

// ============================================================================
// Injection Detection Methods (Phase 3 Hardening)
// ============================================================================

TemplateValidationResult
PromptTemplateValidator::detectInjectionPatterns(const std::string& content) const {
    TemplateValidationResult result;
    
    // SQL injection patterns are low-severity in prompt templates (warnings only)
    if (hasSQLInjectionPattern(content)) {
        result.warnings.push_back("SQL injection pattern detected in template content");
    }
    
    // Command injection is high-severity: can lead to shell execution
    if (hasCommandInjectionPattern(content)) {
        result.errors.push_back("High-severity: command injection pattern detected in template content");
    }
    
    // Path traversal is low-severity in prompt context (warning only)
    if (hasPathTraversalPattern(content)) {
        result.warnings.push_back("Path traversal pattern detected in template content");
    }
    
    // Template injection is high-severity: can manipulate prompt structure
    if (hasTemplateInjectionPattern(content)) {
        result.errors.push_back("High-severity: template injection pattern detected in template content");
    }
    
    result.valid = result.errors.empty();
    return result;
}

bool PromptTemplateValidator::hasSQLInjectionPattern(const std::string& content) const {
    // Check for common SQL injection indicators
    const std::vector<std::string> sql_keywords = {
        "UNION", "SELECT", "INSERT", "DELETE", "DROP", "UPDATE",
        "CREATE", "ALTER", "EXEC", "EXECUTE", "GRANT", "REVOKE"
    };
    
    for (const auto& keyword : sql_keywords) {
        if (containsIgnoreCase(content, keyword)) {
            // Additional check: look for quote patterns suggesting SQL injection
            if (content.find("'") != std::string::npos || 
                content.find("\"") != std::string::npos ||
                content.find("--") != std::string::npos ||
                content.find("/*") != std::string::npos) {
                return true;
            }
        }
    }
    
    return false;
}

bool PromptTemplateValidator::hasCommandInjectionPattern(const std::string& content) const {
    // Check for shell metacharacters commonly used in command injection
    const std::vector<std::string> shell_patterns = {
        "|", "||", "&", "&&", ";", "`", "$(",
        "\n", "\r", "<", ">", "$(", "${", 
    };
    
    for (const auto& pattern : shell_patterns) {
        if (content.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool PromptTemplateValidator::hasPathTraversalPattern(const std::string& content) const {
    // Check for path traversal sequences
    if (content.find("../") != std::string::npos ||
        content.find("..\\") != std::string::npos ||
        content.find("~") != std::string::npos) {
        return true;
    }
    
    // Check for absolute path attempts
    if (content.find("/etc/") != std::string::npos ||
        content.find("C:\\") != std::string::npos ||
        content.find("/root/") != std::string::npos ||
        content.find("/home/") != std::string::npos) {
        return true;
    }
    
    return false;
}

bool PromptTemplateValidator::hasTemplateInjectionPattern(const std::string& content) const {
    // Check for malformed template variable patterns or triple-brace escapes
    if (content.find("{{{") != std::string::npos ||
        content.find("}}}") != std::string::npos) {
        return true;
    }
    
    // Check for suspicious variable reference patterns with unusual escaping
    if (content.find("{{ ") != std::string::npos ||
        content.find(" }}") != std::string::npos) {
        // This is a warning-level pattern; track it for analysis
        // but don't fail validation
        return false;  // Soft pattern
    }
    
    // Check for Jinja2/Template-style code execution patterns
    if (containsIgnoreCase(content, "{% for ") ||
        containsIgnoreCase(content, "{% if ") ||
        containsIgnoreCase(content, "{% macro ") ||
        containsIgnoreCase(content, "{% import ")) {
        return true;
    }
    
    return false;
}

} // namespace prompt_engineering
} // namespace themis
