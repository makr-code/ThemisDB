/**
 * @file response_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/response_parser.h"
#include <stdexcept>
#include "utils/logger.h"
#include <regex>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <limits>

// simdjson fast-path for the JSON extraction hot-path.
// Disabled on MSVC/unity builds to avoid namespace/parser collisions in vendor headers.
#if !defined(_MSC_VER) && __has_include(<simdjson.h>)
#  include <simdjson.h>
#  define THEMIS_RAG_SIMDJSON 1
#else
#  define THEMIS_RAG_SIMDJSON 0
#endif

namespace themis::rag::judge {

// Regex patterns for fallback parsing
const char* ResponseParser::SCORE_PATTERN_1 = R"((?:score|rating)[\s:]+([0-9.]+)(?:/5|%|\s|$))";
const char* ResponseParser::SCORE_PATTERN_2 = R"(([0-9.]+)\s*(?:out of|/)\s*([0-9.]+))";
const char* ResponseParser::SCORE_PATTERN_3 = R"(^[\s]*([1-5])[\s]*$)";
const char* ResponseParser::EXPLANATION_PATTERN = R"((?:reasoning|explanation|rationale)[\s:]+(.+))";
const char* ResponseParser::CLAIMS_PATTERN = R"((?:supporting_claims|supported|verified)[\s:]+\[(.+?)\])";

ParsedResponse ResponseParser::parse(const std::string& response) {
    // Try JSON parsing first
    auto result = parseJSON(response);
    
    if (result.success) {
        THEMIS_DEBUG("Successfully parsed JSON response");
        return result;
    }
    
    // Fallback to regex-based parsing
    THEMIS_DEBUG("JSON parsing failed, using regex fallback");
    result = parseWithRegex(response);
    
    if (!result.success) {
        THEMIS_WARN("Failed to parse response with both JSON and regex");
    }
    
    return result;
}

ParsedResponse ResponseParser::parseJSON(const std::string& response) {
    ParsedResponse result;
    result.success = false;
    
    try {
        // Try to find JSON object in response
        size_t start = response.find('{');
        size_t end = response.rfind('}');
        
        if (start == std::string::npos || end == std::string::npos || start >= end) {
            result.error_message = "No JSON object found in response";
            return result;
        }
        
        std::string json_str = response.substr(start, end - start + 1);

        // Fast structural validation via simdjson before full nlohmann parse.
        // On invalid JSON this saves the full nlohmann parse overhead (~3–5× faster).
#if THEMIS_RAG_SIMDJSON
        static thread_local simdjson::ondemand::parser sj_parser;
        simdjson::padded_string padded(json_str);
        auto doc = sj_parser.iterate(padded);
        if (doc.error()) {
            result.error_message = std::string("JSON structural error (simdjson): ") +
                                   simdjson::error_message(doc.error());
            return result;
        }
#endif
        nlohmann::json j = nlohmann::json::parse(json_str);
        
        // Validate schema
        if (!validateSchema(j)) {
            result.error_message = "JSON schema validation failed";
            return result;
        }
        
        // Extract fields
        if (j.contains("score")) {
            double score = j["score"].is_string() ? 
                std::stod(j["score"].get<std::string>()) : 
                j["score"].get<double>();
            result.score = score;
        }
        
        if (j.contains("confidence")) {
            double confidence = j["confidence"].is_string() ?
                std::stod(j["confidence"].get<std::string>()) :
                j["confidence"].get<double>();
            result.confidence = confidence;
        }
        
        if (j.contains("reasoning")) {
            result.reasoning = j["reasoning"].get<std::string>();
        } else if (j.contains("explanation")) {
            result.reasoning = j["explanation"].get<std::string>();
        }
        
        if (j.contains("supporting_claims") && j["supporting_claims"].is_array()) {
            for (const auto& claim : j["supporting_claims"]) {
                result.supporting_claims.push_back(claim.get<std::string>());
            }
        }
        
        if (j.contains("unsupported_claims") && j["unsupported_claims"].is_array()) {
            for (const auto& claim : j["unsupported_claims"]) {
                result.unsupported_claims.push_back(claim.get<std::string>());
            }
        }
        
        result.success = validate(result);
        
    } catch (const nlohmann::json::exception& e) {
        result.error_message = std::string("JSON parse error: ") + e.what();
        THEMIS_DEBUG("JSON parsing failed: {}", e.what());
    } catch (const std::exception& e) {
        result.error_message = std::string("Parse error: ") + e.what();
        THEMIS_DEBUG("Parsing failed: {}", e.what());
    }
    
    return result;
}

ParsedResponse ResponseParser::parseWithRegex(const std::string& response) {
    ParsedResponse result;
    result.success = false;
    
    // Extract score using multiple patterns
    auto score_opt = extractScore(response);
    if (score_opt) {
        result.score = *score_opt;
    }
    
    // Extract explanation
    result.reasoning = extractExplanation(response);
    
    // Extract supported claims
    result.supporting_claims = extractClaims(response, true);
    
    // Extract unsupported claims
    result.unsupported_claims = extractClaims(response, false);
    
    // Set default confidence
    result.confidence = 0.7;
    
    result.success = result.score.has_value();
    if (!result.success) {
        result.error_message = "Failed to extract score from response";
    }
    
    return result;
}

nlohmann::json ResponseParser::parseJSONResponse(const std::string& response) {
    try {
        size_t start = response.find('{');
        size_t end = response.rfind('}');
        if (start == std::string::npos || end == std::string::npos || start >= end) {
            return nlohmann::json::object();
        }
        std::string json_str = response.substr(start, end - start + 1);
        // Fast structural validation before full parse
#if THEMIS_RAG_SIMDJSON
        static thread_local simdjson::ondemand::parser sj_parser_json_response;
        simdjson::padded_string padded_json_response(json_str);
        auto doc_json_response = sj_parser_json_response.iterate(padded_json_response);
        if (doc_json_response.error()) {
            THEMIS_DEBUG("parseJSONResponse: simdjson rejected: {}",
                         simdjson::error_message(doc_json_response.error()));
            return nlohmann::json::object();
        }
#endif
        return nlohmann::json::parse(json_str);
    } catch (const std::exception& e) {
        THEMIS_DEBUG("parseJSONResponse failed: {}", e.what());
        return nlohmann::json::object();
    }
}

bool ResponseParser::validate(const ParsedResponse& parsed) {
    if (!parsed.score) {
        return false;
    }
    
    double score = *parsed.score;
    if (score < 0.0 || score > 5.0) {
        THEMIS_WARN("Score {} out of valid range [0, 5]", score);
        return false;
    }
    
    if (parsed.confidence && (*parsed.confidence < 0.0 || *parsed.confidence > 1.0)) {
        THEMIS_WARN("Confidence {} out of valid range [0, 1]", *parsed.confidence);
        return false;
    }
    
    return true;
}

double ResponseParser::normalizeScore(double score, double min_range, double max_range) {
    // Normalize to 0-1 range
    constexpr double EPSILON = std::numeric_limits<double>::epsilon();
    if (std::abs(max_range - min_range) < EPSILON) {
        return 0.5;
    }
    
    double normalized = (score - min_range) / (max_range - min_range);
    
    // Clamp to [0, 1]
    normalized = std::max(0.0, std::min(1.0, normalized));
    
    return normalized;
}

std::optional<double> ResponseParser::extractScore(const std::string& text) {
    std::regex pattern1(SCORE_PATTERN_1, std::regex::icase);
    std::regex pattern2(SCORE_PATTERN_2, std::regex::icase);
    std::smatch match;
    
    // Try pattern 1: "score: 4.5" or "rating: 85%"
    if (std::regex_search(text, match, pattern1)) {
        try {
            double score = std::stod(match[1].str());
            
            // Check if it's a percentage
            if (text.find('%', match.position()) != std::string::npos) {
                score /= 100.0;
                score *= 5.0; // Convert to 1-5 scale
            }
            
            return score;
        } catch (const std::exception& e) {
            THEMIS_DEBUG("Failed to parse score from pattern 1: {}", e.what());
        }
    }
    
    // Try pattern 2: "4 out of 5" or "3.5/5"
    if (std::regex_search(text, match, pattern2)) {
        try {
            double numerator = std::stod(match[1].str());
            double denominator = std::stod(match[2].str());
            
            constexpr double EPSILON = std::numeric_limits<double>::epsilon();
            if (std::abs(denominator) > EPSILON) {
                // Normalize to 5-point scale
                double score = (numerator / denominator) * 5.0;
                return score;
            }
        } catch (const std::exception& e) {
            THEMIS_DEBUG("Failed to parse score from pattern 2: {}", e.what());
        }
    }
    
    // Try to find standalone number at start
    std::regex pattern3(SCORE_PATTERN_3);
    if (std::regex_match(text, match, pattern3)) {
        try {
            return std::stod(match[1].str());
        } catch (const std::exception& e) {
            THEMIS_DEBUG("Failed to parse score from pattern 3: {}", e.what());
        }
    }
    
    return std::nullopt;
}

std::string ResponseParser::extractExplanation(const std::string& response) {
    std::regex pattern(EXPLANATION_PATTERN, std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(response, match, pattern)) {
        std::string explanation = match[1].str();
        
        // Trim whitespace
        explanation.erase(0, explanation.find_first_not_of(" \t\n\r"));
        explanation.erase(explanation.find_last_not_of(" \t\n\r") + 1);
        
        // Truncate if too long (max 1000 chars)
        if (explanation.length() > 1000) {
            explanation = explanation.substr(0, 997) + "...";
        }
        
        return explanation;
    }
    
    // If no pattern found, return first few sentences
    size_t end = response.find_first_of(".!?", 0);
    if (end != std::string::npos && end < 500) {
        std::string explanation = response.substr(0, end + 1);
        explanation.erase(0, explanation.find_first_not_of(" \t\n\r"));
        return explanation;
    }
    
    // Return first 200 chars as fallback
    if (response.length() > 200) {
        return response.substr(0, 197) + "...";
    }
    
    return response;
}

std::vector<std::string> ResponseParser::extractClaims(
    const std::string& response,
    bool supported
) {
    std::vector<std::string> claims;
    
    // Try to find JSON array of claims
    std::string key = supported ? "supporting_claims" : "unsupported_claims";
    std::regex pattern(key + R"(\s*:\s*\[(.*?)\])", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(response, match, pattern)) {
        std::string claims_str = match[1].str();
        
        // Split by comma (simple parsing)
        std::istringstream stream(claims_str);
        std::string claim;
        
        while (std::getline(stream, claim, ',')) {
            // Remove quotes and trim
            claim.erase(std::remove(claim.begin(), claim.end(), '\"'), claim.end());
            claim.erase(0, claim.find_first_not_of(" \t\n\r"));
            claim.erase(claim.find_last_not_of(" \t\n\r") + 1);
            
            if (!claim.empty()) {
                claims.push_back(claim);
            }
        }
    }
    
    return claims;
}

bool ResponseParser::validateSchema(const nlohmann::json& json) {
    // Check for required fields
    if (!json.contains("score") && !json.contains("rating")) {
        THEMIS_DEBUG("JSON missing 'score' or 'rating' field");
        return false;
    }
    
    // Optional but recommended fields
    if (!json.contains("reasoning") && !json.contains("explanation")) {
        THEMIS_DEBUG("JSON missing 'reasoning' or 'explanation' field (optional but recommended)");
    }
    
    return true;
}

} // namespace themis::rag::judge

