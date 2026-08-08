/**
 * @file regex_detection_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: regex_detection_engine.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "pii_detection_engine.h"
#include <regex>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace utils {

/**
 * @brief Configuration for a single regex pattern
 */
struct RegexPattern {
    std::string name;           // Pattern name (maps to PIIType)
    std::string description;
    std::string regex_str;      // Original regex string from YAML
    std::regex compiled_regex;  // Compiled regex
    std::regex::flag_type flags;
    double confidence;          // Default confidence for this pattern
    std::string redaction_mode; // "strict", "partial", or "none"
    std::vector<std::string> field_hints; // Field names that suggest this PII type
    std::string validation;     // Validation method: "none", "luhn", "checksum"
    bool enabled;
};

/**
 * @brief Regex-based PII detection engine
 * 
 * Fast pattern-based detection for structured PII:
 * - Email addresses (RFC 5322 simplified)
 * - Phone numbers (international formats)
 * - Social Security Numbers (US format)
 * - Credit card numbers (with Luhn validation)
 * - IBAN (European bank accounts)
 * - IP addresses
 * - URLs
 * 
 * Features:
 * - YAML-based pattern configuration
 * - Runtime reload with validation
 * - Safe fallback to embedded defaults
 * - Configurable confidence levels and redaction modes
 * - Field name heuristics for JSON scanning
 * - Luhn algorithm for credit card validation
 * 
 * Example YAML configuration:
 * @code{.yaml}
 * patterns:
 *   - name: EMAIL
 *     regex: '[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}'
 *     flags: ["icase"]
 *     confidence: 0.95
 *     redaction_mode: "partial"
 *     field_hints: ["email", "mail", "email_address"]
 *     enabled: true
 * @endcode
 */
class RegexDetectionEngine : public IPIIDetectionEngine {
public:
    RegexDetectionEngine();
    ~RegexDetectionEngine() override = default;
    
    // IPIIDetectionEngine interface
    std::string getName() const override;
    std::string getVersion() const override;
    bool isEnabled() const override;
    PluginSignature getSignature() const override;
    bool initialize(const nlohmann::json& config) override;
    bool reload(const nlohmann::json& config) override;
    std::vector<PIIFinding> detectInText(const std::string& text) const override;
    PIIType classifyFieldName(const std::string& field_name) const override;
    std::string getRedactionRecommendation(PIIType type) const override;
    std::string getLastError() const override;
    nlohmann::json getMetadata() const override;

    /**
     * @brief Returns the longest regex string length across all enabled patterns.
     *
     * PIIStreamScanner uses this value to set the cross-chunk lookahead buffer
     * so that patterns whose match spans straddle a chunk boundary are still
     * detected.  A sliding-window overlap equal to maxPatternLength() is applied.
     */
    size_t maxPatternLength() const override;
    
private:
    // Configuration
    bool enabled_;
    mutable std::string last_error_;
    PluginSignature signature_;  // Embedded signature for this engine
    
    // Thread safety for reload operations
    mutable std::mutex mutex_;
    
    // Active patterns (protected by mutex_)
    std::vector<RegexPattern> patterns_;
    
    // Field name hint lookup (lowercase field name -> PIIType)
    std::unordered_map<std::string, PIIType> field_name_hints_;
    
    // Redaction mode per type
    std::unordered_map<PIIType, std::string> redaction_modes_;
    
    // Settings from YAML
    double min_confidence_;
    bool enable_field_hints_;
    std::string default_redaction_mode_;
    size_t max_regex_length_;
    
    // Initialization and loading
    void loadEmbeddedDefaults();
    bool loadPatternsFromConfig(const nlohmann::json& config);
    bool validateAndCompilePattern(RegexPattern& pattern);
    void rebuildFieldHints();
    
    // Validation helpers
    bool validateRegexComplexity(const std::string& regex_str) const;
    std::regex::flag_type parseRegexFlags(const std::vector<std::string>& flag_strings) const;
    
    // Helper: Luhn algorithm for credit card validation
    bool luhnCheck(const std::string& number) const;
    
    // Phase A.1 Hardening: Input validation and safety checks
    /**
     * @brief Validate UTF-8 input for malformed sequences and excessive size.
     * @param text Input text to validate
     * @return true if valid, false if malformed or oversized
     * @note Handles combining characters, BOM, emoji, and invalid UTF-8
     */
    [[nodiscard]] bool validateUTF8Input(std::string_view text) const;
    
    /**
     * @brief Detect known ReDoS (Regular Expression Denial of Service) patterns.
     * @param pattern Regex pattern string to check
     * @return true if pattern is known to be dangerous, false otherwise
     * @note Detects nested quantifiers: (a+)+, (a*)*
     * @note Detects alternation with overlap: (a|a)*, (x|x|x)*
     */
    [[nodiscard]] bool detectReDoSPattern(const std::string& pattern) const;
    
    /**
     * @brief Enforce maximum input size limit.
     * @param text Input to check
     * @return true if within bounds, false if exceeds limit
     * @note Default limit: 10MB; configurable via max_input_size_
     */
    [[nodiscard]] bool checkInputBounds(std::string_view text) const;
    
    // Configuration for hardening
    size_t max_input_size_{10 * 1024 * 1024}; // 10MB default
    bool detect_redos_patterns_{true};
    bool validate_utf8_{true};
};

} // namespace utils
} // namespace themis
