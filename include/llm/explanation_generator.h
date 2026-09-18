/**
 * @file explanation_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

class ExplanationGenerator {
public:
    enum class Format {
        USER_FRIENDLY,    // Plain language for end users
        TECHNICAL,        // Detailed for developers/auditors
        COMPLIANCE,       // Formal for regulatory compliance
        JSON              // Structured JSON format
    };
    
    std::string generateExplanation(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors,
        Format format = Format::USER_FRIENDLY
    );
    
    /**
     * @brief Generate Reasoning Chain.
     * @param[in] query Input parameter.
     * @param[in] intermediate_results Input parameter.
     * @return Return value.
     */
    std::vector<std::string> generateReasoningChain(
        const std::string& query,
        const json& intermediate_results
    );
    
    json identifyKeyFactors(
        const std::string& query,
        const std::string& response,
        const json& context = json::object()
    );
    
    std::string explainConfidence(
        float confidence,
        const std::vector<std::string>& alternatives = {}
    );
    
    /**
     * @brief Generate Compliance Explanation.
     * @param[in] query Input parameter.
     * @param[in] response Input parameter.
     * @param[in] model_info Input parameter.
     * @param[in] reasoning_steps Input parameter.
     * @param[in] key_factors Input parameter.
     * @param[in] confidence Input parameter.
     * @return Return value.
     */
    std::string generateComplianceExplanation(
        const std::string& query,
        const std::string& response,
        const std::string& model_info,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors,
        float confidence
    );

private:
    /**
     * @brief Helper methods for different formats
     * @param[in] query Input parameter.
     * @param[in] response Input parameter.
     * @param[in] reasoning_steps Input parameter.
     * @param[in] key_factors Input parameter.
     * @return Return value.
     */
    std::string formatUserFriendly(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    /**
     * @brief Format Technical.
     * @param[in] query Input parameter.
     * @param[in] response Input parameter.
     * @param[in] reasoning_steps Input parameter.
     * @param[in] key_factors Input parameter.
     * @return Return value.
     */
    std::string formatTechnical(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    /**
     * @brief Format Compliance.
     * @param[in] query Input parameter.
     * @param[in] response Input parameter.
     * @param[in] reasoning_steps Input parameter.
     * @param[in] key_factors Input parameter.
     * @return Return value.
     */
    std::string formatCompliance(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    /**
     * @brief Format Json.
     * @param[in] query Input parameter.
     * @param[in] response Input parameter.
     * @param[in] reasoning_steps Input parameter.
     * @param[in] key_factors Input parameter.
     * @return Return value.
     */
    std::string formatJson(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    /**
     * @brief Helper to extract keywords from text
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractKeywords(const std::string& text);
    
    /**
     * @brief Helper to calculate text similarity (simple word overlap)
     * @param[in] text1 Input parameter.
     * @param[in] text2 Input parameter.
     * @return Return value.
     */
    float calculateSimilarity(const std::string& text1, const std::string& text2);
};

} // namespace llm
} // namespace themis
