/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            response_parser.h                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file response_parser.h
 * @brief Response parsing for LLM judge outputs
 */

#pragma once

#include "rag/rag_judge.h"
#include <string>
#include &lt;optional&gt;
#include <nlohmann/json.hpp>

namespace themis::rag::judge {

/**
 * @brief Parsed response from LLM judge
 */
struct ParsedResponse {
    bool success;
    std::optional<double> score;
    std::optional<double> confidence;
    std::string reasoning;
    std::vector<std::string> supporting_claims;
    std::vector<std::string> unsupported_claims;
    std::string error_message;
    /// True when this response was produced by the stub/mock inference path
    /// (config.use_mock_mode == true or allow_mock == true with engine == nullptr).
    /// Callers should discard or flag these results in production dashboards.
    /// Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 7.
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
