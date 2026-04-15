/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            explanation_generator.h                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:07:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief Explanation Generator
 * 
 * Generates human-readable explanations for AI decisions.
 * Supports multiple explanation formats for different audiences.
 * 
 * Compliance features:
 * - GDPR Article 22: Right to explanation
 * - EU AI Act: Transparency requirements
 * - Plain language summaries for end users
 * - Technical details for auditors
 */
class ExplanationGenerator {
public:
    /**
     * @brief Explanation format/audience
     */
    enum class Format {
        USER_FRIENDLY,    // Plain language for end users
        TECHNICAL,        // Detailed for developers/auditors
        COMPLIANCE,       // Formal for regulatory compliance
        JSON              // Structured JSON format
    };
    
    /**
     * @brief Generate human-readable explanation
     * 
     * @param query Original query/input
     * @param response AI-generated response
     * @param reasoning_steps Step-by-step reasoning chain
     * @param key_factors Factors influencing the decision
     * @param format Desired explanation format
     * @return Human-readable explanation
     */
    std::string generateExplanation(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors,
        Format format = Format::USER_FRIENDLY
    );
    
    /**
     * @brief Generate reasoning chain
     * 
     * Creates step-by-step breakdown of how the AI arrived at its decision.
     * 
     * @param query Original query
     * @param intermediate_results Intermediate computation results
     * @return Vector of reasoning steps
     */
    std::vector<std::string> generateReasoningChain(
        const std::string& query,
        const json& intermediate_results
    );
    
    /**
     * @brief Identify key factors influencing decision
     * 
     * Analyzes query and response to determine what factors
     * had the most influence on the AI's decision.
     * 
     * @param query Original query
     * @param response AI response
     * @param context Additional context
     * @return JSON object with key factors and their weights
     */
    json identifyKeyFactors(
        const std::string& query,
        const std::string& response,
        const json& context = json::object()
    );
    
    /**
     * @brief Generate confidence interval explanation
     * 
     * @param confidence Confidence score (0.0-1.0)
     * @param alternatives Alternative responses considered
     * @return Explanation of confidence level
     */
    std::string explainConfidence(
        float confidence,
        const std::vector<std::string>& alternatives = {}
    );
    
    /**
     * @brief Generate compliance-ready explanation
     * 
     * Creates formal explanation suitable for regulatory compliance
     * (GDPR Article 22, EU AI Act).
     * 
     * @param query Original query
     * @param response AI response
     * @param model_info Model name and version
     * @param reasoning_steps Reasoning chain
     * @param key_factors Key decision factors
     * @param confidence Confidence score
     * @return Formal compliance explanation
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
    // Helper methods for different formats
    std::string formatUserFriendly(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    std::string formatTechnical(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    std::string formatCompliance(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    std::string formatJson(
        const std::string& query,
        const std::string& response,
        const std::vector<std::string>& reasoning_steps,
        const json& key_factors
    );
    
    // Helper to extract keywords from text
    std::vector<std::string> extractKeywords(const std::string& text);
    
    // Helper to calculate text similarity (simple word overlap)
    float calculateSimilarity(const std::string& text1, const std::string& text2);
};

} // namespace llm
} // namespace themis
