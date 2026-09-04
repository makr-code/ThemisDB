/**
 * @file explanation_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/explanation_generator.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>
#include <iomanip>

namespace themis {
namespace llm {

std::string ExplanationGenerator::generateExplanation(
    const std::string& query,
    const std::string& response,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors,
    Format format) {
    
    switch (format) {
        case Format::USER_FRIENDLY:
            return formatUserFriendly(query, response, reasoning_steps, key_factors);
        case Format::TECHNICAL:
            return formatTechnical(query, response, reasoning_steps, key_factors);
        case Format::COMPLIANCE:
            return formatCompliance(query, response, reasoning_steps, key_factors);
        case Format::JSON:
            return formatJson(query, response, reasoning_steps, key_factors);
        default:
            return formatUserFriendly(query, response, reasoning_steps, key_factors);
    }
}

std::string ExplanationGenerator::formatUserFriendly(
    const std::string& query,
    const std::string& response,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors) {
    
    std::ostringstream out = {};
    
    out << "🤔 How did I arrive at this answer?\n\n";
    out << "Your question: \"" << query << "\"\n\n";
    
    if (!reasoning_steps.empty()) {
        out << "My thinking process:\n";
        for (size_t i = 0; i < reasoning_steps.size(); i++) {
            out << "  " << (i + 1) << ". " << reasoning_steps[i] << "\n";
        }
        out << "\n";
    }
    
    if (!key_factors.empty() && key_factors.is_object()) {
        out << "Key factors I considered:\n";
        for (const auto& [key, value] : key_factors.items()) {
            out << "  • " << key << ": ";
            if (value.is_string()) {
                out << value.get<std::string>();
            } else {
                out << value.dump();
            }
            out << "\n";
        }
        out << "\n";
    }
    
    out << "My answer: " << response << "\n";
    
    return out.str();
}

std::string ExplanationGenerator::formatTechnical(
    const std::string& query,
    const std::string& response,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors) {
    
    std::ostringstream out = {};
    
    out << "=== AI Decision Technical Analysis ===\n\n";
    out << "INPUT QUERY:\n" << query << "\n\n";
    
    out << "REASONING CHAIN:\n";
    if (!reasoning_steps.empty()) {
        for (size_t i = 0; i < reasoning_steps.size(); i++) {
            out << "[Step " << (i + 1) << "] " << reasoning_steps[i] << "\n";
        }
    } else {
        out << "(No explicit reasoning steps recorded)\n";
    }
    out << "\n";
    
    out << "KEY DECISION FACTORS:\n";
    if (!key_factors.empty()) {
        out << key_factors.dump(2) << "\n";
    } else {
        out << "(No key factors identified)\n";
    }
    out << "\n";
    
    out << "OUTPUT RESPONSE:\n" << response << "\n\n";
    
    out << "=== End Technical Analysis ===\n";
    
    return out.str();
}

std::string ExplanationGenerator::formatCompliance(
    const std::string& query,
    const std::string& response,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors) {
    
    std::ostringstream out = {};
    
    out << "AUTOMATED DECISION EXPLANATION\n";
    out << "(GDPR Article 22 / EU AI Act Compliance)\n\n";
    
    out << "1. DECISION CONTEXT\n";
    out << "   Input Query: " << query << "\n\n";
    
    out << "2. PROCESSING LOGIC\n";
    if (!reasoning_steps.empty()) {
        out << "   The automated system processed this request through the following steps:\n";
        for (size_t i = 0; i < reasoning_steps.size(); i++) {
            out << "   " << (i + 1) << ". " << reasoning_steps[i] << "\n";
        }
    } else {
        out << "   The automated system processed this request using its trained model.\n";
    }
    out << "\n";
    
    out << "3. DECISION FACTORS\n";
    if (!key_factors.empty() && key_factors.is_object()) {
        out << "   The following factors influenced this automated decision:\n";
        for (const auto& [key, value] : key_factors.items()) {
            out << "   - " << key << ": " << value.dump() << "\n";
        }
    } else {
        out << "   Standard model parameters were applied.\n";
    }
    out << "\n";
    
    out << "4. DECISION OUTPUT\n";
    out << "   Result: " << response << "\n\n";
    
    out << "5. YOUR RIGHTS\n";
    out << "   Under GDPR Article 22, you have the right to:\n";
    out << "   - Request human review of this decision\n";
    out << "   - Obtain additional information about the decision logic\n";
    out << "   - Contest this automated decision\n";
    out << "   - Request correction if the decision is based on incorrect data\n\n";
    
    return out.str();
}

std::string ExplanationGenerator::formatJson(
    const std::string& query,
    const std::string& response,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors) {
    
    json explanation;
    explanation["query"] = query;
    explanation["response"] = response;
    explanation["reasoning_steps"] = reasoning_steps;
    explanation["key_factors"] = key_factors;
    
    return explanation.dump(2);
}

std::vector<std::string> ExplanationGenerator::generateReasoningChain(
    const std::string& query,
    const json& intermediate_results) {
    
    std::vector<std::string> steps;
    
    // Generate basic reasoning steps from query analysis
    steps.push_back("Analyzed the input query to understand intent");
    
    // Add intermediate results as reasoning steps
    if (!intermediate_results.empty() && intermediate_results.is_object()) {
        for (const auto& [key, value] : intermediate_results.items()) {
            std::ostringstream step = {};
            step << "Processed " << key << ": ";
            if (value.is_string()) {
                step << value.get<std::string>();
            } else if (value.is_number()) {
                step << value.dump();
            } else {
                step << "(complex data)";
            }
            steps.push_back(step.str());
        }
    }
    
    steps.push_back("Generated response based on processed information");
    
    return steps;
}

json ExplanationGenerator::identifyKeyFactors(
    const std::string& query,
    const std::string& response,
    const json& context) {
    
    json factors;
    
    // Extract keywords from query
    auto query_keywords = extractKeywords(query);
    auto response_keywords = extractKeywords(response);
    
    // Identify common keywords (these likely influenced the response)
    std::vector<std::string> common_keywords = {};

    for (const auto& qk : query_keywords) {
        if (std::find(response_keywords.begin(), response_keywords.end(), qk) 
            != response_keywords.end()) {
            common_keywords.push_back(qk);
        }
    }
    
    if (!common_keywords.empty()) {
        factors["query_terms_used"] = common_keywords;
    }
    
    // Calculate response relevance
    float similarity = calculateSimilarity(query, response);
    factors["query_response_similarity"] = std::round(similarity * 100.0f) / 100.0f;
    
    // Add context factors if provided
    if (!context.empty() && context.is_object()) {
        for (const auto& [key, value] : context.items()) {
            factors["context_" + key] = value;
        }
    }
    
    // Query complexity
    size_t query_words = query_keywords.size();
    if (query_words < 5) {
        factors["query_complexity"] = "simple";
    } else if (query_words < 15) {
        factors["query_complexity"] = "moderate";
    } else {
        factors["query_complexity"] = "complex";
    }
    
    return factors;
}

std::string ExplanationGenerator::explainConfidence(
    float confidence,
    const std::vector<std::string>& alternatives) {
    
    std::ostringstream out = {};
    
    // Convert to percentage
    int confidence_pct = static_cast<int>(confidence * 100);
    
    out << "Confidence Level: " << confidence_pct << "%\n\n";
    
    if (confidence >= 0.9f) {
        out << "This is a high-confidence response. The AI is very certain about this answer.";
    } else if (confidence >= 0.7f) {
        out << "This is a moderate-confidence response. The AI is reasonably certain, "
            << "but there may be some uncertainty.";
    } else if (confidence >= 0.5f) {
        out << "This is a low-confidence response. The AI has significant uncertainty "
            << "about this answer. Human review is recommended.";
    } else {
        out << "This is a very low-confidence response. The AI is highly uncertain. "
            << "Human review is strongly recommended.";
    }
    
    if (!alternatives.empty()) {
        out << "\n\nAlternative responses considered:\n";
        for (size_t i = 0; i < std::min(alternatives.size(), size_t(3)); i++) {
            out << "  " << (i + 1) << ". " << alternatives[i] << "\n";
        }
    }
    
    return out.str();
}

std::string ExplanationGenerator::generateComplianceExplanation(
    const std::string& query,
    const std::string& response,
    const std::string& model_info,
    const std::vector<std::string>& reasoning_steps,
    const json& key_factors,
    float confidence) {
    
    std::ostringstream out = {};
    
    out << "═══════════════════════════════════════════════════════════\n";
    out << "  AUTOMATED DECISION-MAKING EXPLANATION\n";
    out << "  EU AI Act & GDPR Article 22 Compliance Document\n";
    out << "═══════════════════════════════════════════════════════════\n\n";
    
    out << "SECTION 1: SYSTEM IDENTIFICATION\n";
    out << "  Model: " << model_info << "\n";
    out << "  Decision Confidence: " << std::fixed << std::setprecision(1) 
        << (confidence * 100.0f) << "%\n\n";
    
    out << "SECTION 2: INPUT\n";
    out << "  User Query: \"" << query << "\"\n\n";
    
    out << "SECTION 3: DECISION LOGIC\n";
    if (!reasoning_steps.empty()) {
        out << "  Processing Steps:\n";
        for (size_t i = 0; i < reasoning_steps.size(); i++) {
            out << "    Step " << (i + 1) << ": " << reasoning_steps[i] << "\n";
        }
    } else {
        out << "  The system applied its trained model to generate a response.\n";
    }
    out << "\n";
    
    out << "SECTION 4: INFLUENCING FACTORS\n";
    if (!key_factors.empty() && key_factors.is_object()) {
        for (const auto& [key, value] : key_factors.items()) {
            out << "  - " << key << ": " << value.dump() << "\n";
        }
    } else {
        out << "  Standard model parameters were applied.\n";
    }
    out << "\n";
    
    out << "SECTION 5: OUTPUT\n";
    out << "  System Response: " << response << "\n\n";
    
    out << "SECTION 6: REGULATORY COMPLIANCE\n";
    out << "  This explanation is provided in compliance with:\n";
    out << "  - GDPR Article 22 (Right to explanation)\n";
    out << "  - EU AI Act (Transparency requirements)\n";
    out << "  - eIDAS Regulation (Audit trail requirements)\n\n";
    
    out << "SECTION 7: USER RIGHTS\n";
    out << "  You have the right to:\n";
    out << "  1. Request human review of this automated decision\n";
    out << "  2. Obtain additional information about the decision-making logic\n";
    out << "  3. Contest this automated decision\n";
    out << "  4. Request correction if based on incorrect personal data\n";
    out << "  5. Lodge a complaint with a supervisory authority\n\n";
    
    if (confidence < 0.7f) {
        out << "NOTICE: This decision has lower confidence and has been flagged\n";
        out << "for human review. You may request immediate human oversight.\n\n";
    }
    
    out << "═══════════════════════════════════════════════════════════\n";
    
    return out.str();
}

// Helper methods

std::vector<std::string> ExplanationGenerator::extractKeywords(const std::string& text) {
    // Early return for empty or very short text
    if (text.empty() || text.length() < 3) {
        return {};
    }
    
    // Reserve space to reduce allocations
    std::vector<std::string> keywords;
    keywords.reserve(text.length() / 6); // Rough estimate: avg 6 chars per word
    
    // Common stop words to skip (could be expanded or made configurable)
    // Note: In a production system, consider using a more comprehensive stop word list
    // and possibly stemming/lemmatization for better keyword extraction.
    // This method currently uses a simple heuristic to extract keywords. In a real implementation,
    // we would use an NLP library like spaCy or NLTK for more accurate keyword extraction, including
    // stemming and lemmatization. For now, it just extracts alphanumeric words and filters out common stop words.
    static const std::set<std::string> stop_words = {
        "the", "and", "for", "are", "was", "with", "this", "that",
        "from", "they", "have", "been", "will", "their", "what"
    };
    
    // Process text character by character to avoid istringstream overhead
    std::string word = {};
    word.reserve(20); // Most words are < 20 chars; this reduces allocations, maybe tuned based on typical input
    
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            word += std::tolower(static_cast<unsigned char>(c));
        } else if (!word.empty()) {
            // Word boundary - process accumulated word
            if (word.length() >= 3 && stop_words.find(word) == stop_words.end()) {
                keywords.push_back(word);
            }
            word.clear();
        }
    }
    
    // Process last word if any
    if (!word.empty() && word.length() >= 3 && 
        stop_words.find(word) == stop_words.end()) {
        keywords.push_back(word);
    }
    
    return keywords;
}

float ExplanationGenerator::calculateSimilarity(
    const std::string& text1, 
    const std::string& text2) {
    
    auto keywords1 = extractKeywords(text1);
    auto keywords2 = extractKeywords(text2);
    
    if (keywords1.empty() || keywords2.empty()) {
        return 0.0f;
    }
    
    // Convert to sets for intersection
    std::set<std::string> set1(keywords1.begin(), keywords1.end());
    std::set<std::string> set2(keywords2.begin(), keywords2.end());
    
    // Count common words
    size_t common = 0;
    for (const auto& word : set1) {
        if (set2.count(word) > 0) {
            common++;
        }
    }
    
    // Jaccard similarity
    size_t total_unique = static_cast<int>(set1.size()) + static_cast<int>(set2.size()) - common;
    if (total_unique == 0) {
        return 0.0f;
    }
    
    return static_cast<float>(common) / static_cast<float>(total_unique);
}

} // namespace llm
} // namespace themis

