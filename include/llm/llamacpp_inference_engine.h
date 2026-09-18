#pragma once

#include <string>
#include <vector>

/**
 * @file llamacpp_inference_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

namespace themis {
namespace llm {

struct ValidationResult {
    /**
     * @brief Validation Result.
     * @return Return value.
     */
    virtual ~ValidationResult() = default;
    bool is_valid = false;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
    struct Metrics {
        int token_count = 0;
        int word_count = 0;
        int char_count = 0;
        double avg_word_length = 0.0;
        bool is_truncated = false;       // Hit context limit?
        bool is_utf8_valid = true;
        double semantic_coherence = 1.0;  // Estimated 0-1
        int newline_count = 0;
        int sentence_count = 0;
    } metrics;
    
    ValidationResult() : is_valid(true) {}
};

class LLMOutputValidator {
public:
    struct Config {
        int min_length = 1;              // Minimum response length (characters)
        int max_length = 100000;         // Maximum response length
        bool require_utf8 = true;        // Enforce UTF-8 encoding
        bool check_truncation = true;    // Detect incomplete responses
        bool check_coherence = true;     // Estimate semantic coherence
        double min_coherence = 0.3;      // Minimum coherence score (0-1)
        bool allow_empty = false;        // Allow empty responses
    };
    
    /**
     * @brief LLMOutput Validator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LLMOutputValidator(const Config& config);
    LLMOutputValidator();
    
    /**
     * @brief Validate.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    ValidationResult validate(const std::string& text);
    
    /**
     * @brief Validate With Tokens.
     * @param[in] text Input parameter.
     * @param[in] token_count Input parameter.
     * @param[in] max_tokens Input parameter.
     * @return Return value.
     */
    ValidationResult validateWithTokens(
        const std::string& text,
        int token_count,
        int max_tokens
    );
    
private:
    Config config_;
    
    // Validation helpers
    /**
     * @brief Is Valid UTF8.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool isValidUTF8(const std::string& text);
    /**
     * @brief Detect Truncation.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool detectTruncation(const std::string& text);
    /**
     * @brief Estimate Coherence.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    double estimateCoherence(const std::string& text);
    /**
     * @brief Has Common Errors.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCommonErrors(const std::string& text);
    
    // Metrics calculation
    /**
     * @brief Count Words.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countWords(const std::string& text);
    /**
     * @brief Count Sentences.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countSentences(const std::string& text);
    /**
     * @brief Calculate Avg Word Length.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    double calculateAvgWordLength(const std::string& text);
    
    // Error pattern detection
    /**
     * @brief Has Repeating Patterns.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasRepeatingPatterns(const std::string& text);
    /**
     * @brief Has Invalid Control Chars.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasInvalidControlChars(const std::string& text);
};

} // namespace llm
} // namespace themis
