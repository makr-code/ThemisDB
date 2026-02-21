/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llamacpp_inference_engine.h                        ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:33:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c6716ede7  2026-02-16  Add ThemisDB Order Request Plugin with shortcodes, AJAX h... ║
    • 430c8d278  2026-01-18  P0: LLM Production Readiness - State Machine, Async Loadi... ║
    • 54c99c7e6  2026-01-18  P0: Complete llama.cpp integration with unified implement... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>

/**
 * @file llamacpp_inference_engine.h
 * @brief LLM output validation for production RAG pipelines
 * 
 * Comprehensive output validator to detect:
 * - Empty/truncated responses (context limit hit)
 * - Invalid UTF-8 encoding
 * - Semantic anomalies
 * - Token limit violations
 * 
 * Prevents cascading failures in RAG pipelines by validating
 * LLM outputs before they're used for retrieval or response generation.
 */

namespace themis {
namespace llm {

/**
 * @brief LLM output validation result
 */
struct ValidationResult {
    bool is_valid;
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

/**
 * @brief LLM Output Validator for production deployments
 * 
 * Validates LLM-generated text to prevent cascading failures
 * in RAG pipelines and downstream applications.
 * 
 * Usage:
 * ```cpp
 * LLMOutputValidator validator;
 * auto result = validator.validate(llm_response);
 * if (!result.is_valid) {
 *     for (const auto& error : result.errors) {
 *         spdlog::error("LLM validation error: {}", error);
 *     }
 *     return Status::Error("Invalid LLM output");
 * }
 * ```
 */
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
    
    explicit LLMOutputValidator(const Config& config);
    LLMOutputValidator();
    
    /**
     * @brief Validate LLM output
     * @param text Generated text to validate
     * @return Validation result with errors/warnings
     */
    ValidationResult validate(const std::string& text);
    
    /**
     * @brief Validate with token count (if available)
     * @param text Generated text
     * @param token_count Number of tokens generated
     * @param max_tokens Maximum tokens allowed
     * @return Validation result
     */
    ValidationResult validateWithTokens(
        const std::string& text,
        int token_count,
        int max_tokens
    );
    
private:
    Config config_;
    
    // Validation helpers
    bool isValidUTF8(const std::string& text);
    bool detectTruncation(const std::string& text);
    double estimateCoherence(const std::string& text);
    bool hasCommonErrors(const std::string& text);
    
    // Metrics calculation
    int countWords(const std::string& text);
    int countSentences(const std::string& text);
    double calculateAvgWordLength(const std::string& text);
    
    // Error pattern detection
    bool hasRepeatingPatterns(const std::string& text);
    bool hasInvalidControlChars(const std::string& text);
};

} // namespace llm
} // namespace themis
