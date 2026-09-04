/**
 * @file response_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis::rag::judge {

/**
 * @brief Parsed response from LLM judge
 */
struct ParsedResponse {
    bool success = 0;
    std::optional<double> score;
    std::optional<double> confidence;
    std::string reasoning;
    std::vector<std::string> supporting_claims;
    std::vector<std::string> unsupported_claims;
    std::string error_message;
    /// Compatibility field retained for callers that previously distinguished
    /// mock results. Mock-mode fallback has been removed, so production and
    /// unavailable paths both leave this field false.
    bool is_mock = false;
};

/**
 * @brief Response parser for LLM judge outputs
 * 
 * Parses structured JSON outputs with fallback to regex-based parsing
 * when JSON is malformed or missing.
 */
class ResponseParser {
public:
    /**
     * @brief Parse LLM response to extract score and explanation
     * @param response Raw LLM response string
     * @return Parsed response structure
     */
    static ParsedResponse parse(const std::string& response);
    
    /**
     * @brief Parse JSON response
     * @param response Raw response string
     * @return Parsed response or error
     */
    static ParsedResponse parseJSON(const std::string& response);
    
    /**
     * @brief Parse response using regex fallback
     * @param response Raw response string
     * @return Parsed response or error
     */
    static ParsedResponse parseWithRegex(const std::string& response);

    /**
     * @brief Extract JSON object from response (best-effort)
     * @param response Raw response string
     * @return Parsed JSON object (empty if parsing fails)
     */
    static nlohmann::json parseJSONResponse(const std::string& response);
    
    /**
     * @brief Validate parsed response
     * @param parsed Parsed response to validate
     * @return true if response is valid
     */
    static bool validate(const ParsedResponse& parsed);
    
    /**
     * @brief Normalize score to 0-1 range
     * @param score Original score
     * @param min_range Minimum of original range
     * @param max_range Maximum of original range
     * @return Normalized score (0-1)
     */
    static double normalizeScore(double score, double min_range = 1.0, double max_range = 5.0);
    
    /**
     * @brief Extract score from text using various patterns
     * @param text Text to extract score from
     * @return Extracted score or nullopt
     */
    static std::optional<double> extractScore(const std::string& text);
    
    /**
     * @brief Extract explanation/reasoning from response
     * @param response Response text
     * @return Extracted explanation
     */
    static std::string extractExplanation(const std::string& response);
    
    /**
     * @brief Extract claims from response
     * @param response Response text
     * @param supported true for supported claims, false for unsupported
     * @return Vector of claims
     */
    static std::vector<std::string> extractClaims(
        const std::string& response,
        bool supported = true
    );
    
    /**
     * @brief Validate JSON schema
     * @param json JSON object to validate
     * @return true if schema is valid
     */
    static bool validateSchema(const nlohmann::json& json);

private:
    // Regex patterns for fallback parsing
    static const char* SCORE_PATTERN_1; // "score: 0.85" or "score: 85%"
    static const char* SCORE_PATTERN_2; // "rating: 4/5" or "4 out of 5"
    static const char* SCORE_PATTERN_3; // standalone number at start
    static const char* EXPLANATION_PATTERN; // "reasoning:" or "explanation:"
    static const char* CLAIMS_PATTERN; // "supported:" or "verified:"
};

} // namespace themis::rag::judge
