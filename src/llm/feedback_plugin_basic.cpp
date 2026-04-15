/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_plugin_basic.cpp                          ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:41:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/i_feedback_plugin.h"
#include "utils/logger.h"
#include <algorithm>
#include <regex>

namespace themis {
namespace llm {

// BasicSpamDetectionPlugin implementation

bool BasicSpamDetectionPlugin::initialize(const json& config) {
    // Load spam keywords from config or use defaults
    if (config.contains("spam_keywords") && config["spam_keywords"].is_array()) {
        for (const auto& keyword : config["spam_keywords"]) {
            if (keyword.is_string()) {
                spam_keywords_.push_back(keyword.get<std::string>());
            }
        }
    }
    
    // Default spam keywords if none provided
    if (spam_keywords_.empty()) {
        spam_keywords_ = {
            "buy now", "click here", "viagra", "casino", "lottery",
            "free money", "million dollars", "nigerian prince",
            "weight loss", "work from home", "make money fast"
        };
    }
    
    THEMIS_INFO("BasicSpamDetectionPlugin initialized with {} spam keywords", 
                spam_keywords_.size());
    return true;
}

bool BasicSpamDetectionPlugin::containsSpamKeywords(const std::string& text) const {
    if (text.empty()) {
        return false;
    }
    
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), 
                   lower_text.begin(), ::tolower);
    
    for (const auto& keyword : spam_keywords_) {
        if (lower_text.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool BasicSpamDetectionPlugin::isLowQuality(const FeedbackData& feedback) const {
    // Configuration constants
    constexpr size_t MIN_LENGTH = 5;
    constexpr size_t MAX_LENGTH = 10000;
    constexpr size_t MIN_REPETITION = 10;
    
    // Too short
    if (feedback.question.length() < MIN_LENGTH || feedback.answer.length() < MIN_LENGTH) {
        return true;
    }
    
    // Excessive repetition
    std::regex repeat_pattern(R"((.)\1{10,})");
    if (std::regex_search(feedback.question, repeat_pattern) ||
        std::regex_search(feedback.answer, repeat_pattern)) {
        return true;
    }
    
    // Too long (likely copy-paste spam)
    if (feedback.question.length() > MAX_LENGTH || feedback.answer.length() > MAX_LENGTH) {
        return true;
    }
    
    // Negative feedback without correction or comment
    if (!feedback.is_positive && 
        feedback.correction.empty() && 
        feedback.comment.empty()) {
        return true;
    }
    
    return false;
}

ValidationResponse BasicSpamDetectionPlugin::validate(const FeedbackData& feedback) {
    validation_count_++;
    
    ValidationResponse response;
    response.result = FeedbackValidationResult::ACCEPT;
    response.confidence_score = 1.0f;
    
    // Check for spam keywords
    if (containsSpamKeywords(feedback.question) ||
        containsSpamKeywords(feedback.answer) ||
        containsSpamKeywords(feedback.correction) ||
        containsSpamKeywords(feedback.comment)) {
        response.result = FeedbackValidationResult::REJECT;
        response.reason = "Contains spam keywords";
        response.confidence_score = 0.9f;
        rejected_count_++;
        THEMIS_DEBUG("Feedback rejected: spam keywords detected");
        return response;
    }
    
    // Check for low quality
    if (isLowQuality(feedback)) {
        response.result = FeedbackValidationResult::FLAG;
        response.reason = "Low quality feedback - needs manual review";
        response.confidence_score = 0.7f;
        THEMIS_DEBUG("Feedback flagged: low quality");
        return response;
    }
    
    THEMIS_DEBUG("Feedback accepted by spam detection plugin");
    return response;
}

void BasicSpamDetectionPlugin::shutdown() {
    THEMIS_INFO("BasicSpamDetectionPlugin shutting down. Stats: {} validated, {} rejected",
                validation_count_, rejected_count_);
}

json BasicSpamDetectionPlugin::getStatistics() const {
    return json{
        {"validation_count", validation_count_},
        {"rejected_count", rejected_count_},
        {"rejection_rate", validation_count_ > 0 
            ? static_cast<double>(rejected_count_) / validation_count_ 
            : 0.0},
        {"spam_keywords_count", spam_keywords_.size()}
    };
}

} // namespace llm
} // namespace themis

