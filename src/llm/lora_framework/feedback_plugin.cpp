#include "llm/lora_framework/feedback_plugin.h"
#include <algorithm>
#include <cctype>
#include <regex>

namespace themis {
namespace llm {
namespace lora {

// ═══════════════════════════════════════════════════════════
// PrivacyFilterPlugin Implementation
// ═══════════════════════════════════════════════════════════

void PrivacyFilterPlugin::process(Feedback& feedback) {
    // Remove common PII patterns from feedback text
    
    // Email addresses
    static const std::regex email_pattern(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
    );
    feedback.feedback_text = std::regex_replace(
        feedback.feedback_text,
        email_pattern,
        "[EMAIL]"
    );
    
    // Phone numbers (simple pattern)
    static const std::regex phone_pattern(
        R"(\b\d{3}[-.]?\d{3}[-.]?\d{4}\b)"
    );
    feedback.feedback_text = std::regex_replace(
        feedback.feedback_text,
        phone_pattern,
        "[PHONE]"
    );
    
    // Credit card numbers (simple pattern)
    static const std::regex cc_pattern(
        R"(\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b)"
    );
    feedback.feedback_text = std::regex_replace(
        feedback.feedback_text,
        cc_pattern,
        "[CC_NUMBER]"
    );
    
    // Social Security Numbers (US format)
    static const std::regex ssn_pattern(
        R"(\b\d{3}-\d{2}-\d{4}\b)"
    );
    feedback.feedback_text = std::regex_replace(
        feedback.feedback_text,
        ssn_pattern,
        "[SSN]"
    );
}

// ═══════════════════════════════════════════════════════════
// ContentValidationPlugin Implementation
// ═══════════════════════════════════════════════════════════

bool ContentValidationPlugin::validate(const Feedback& feedback) const {
    // First run base validation
    if (!BaseFeedbackPlugin::validate(feedback)) {
        return false;
    }
    
    // Check for spam
    if (containsSpam(feedback.feedback_text)) {
        return false;
    }
    
    // Check for profanity (optional - can be configured)
    // if (containsProfanity(feedback.feedback_text)) {
    //     return false;
    // }
    
    // Check minimum length
    if (!feedback.feedback_text.empty() && feedback.feedback_text.length() < 3) {
        return false;
    }
    
    // Check maximum length
    if (feedback.feedback_text.length() > 10000) {
        return false;
    }
    
    return true;
}

bool ContentValidationPlugin::containsSpam(const std::string& text) const {
    // Simple spam detection - check for common spam patterns
    if (text.empty()) return false;
    
    // Convert to lowercase for comparison
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check for excessive repeated characters
    int repeat_count = 0;
    char last_char = '\0';
    for (char c : lower_text) {
        if (c == last_char) {
            repeat_count++;
            if (repeat_count > 10) return true; // Excessive repetition
        } else {
            repeat_count = 1;
            last_char = c;
        }
    }
    
    // Check for excessive URLs
    static const std::regex url_pattern(
        R"(https?://[^\s]+)"
    );
    auto urls_begin = std::sregex_iterator(text.begin(), text.end(), url_pattern);
    auto urls_end = std::sregex_iterator();
    int url_count = std::distance(urls_begin, urls_end);
    if (url_count > 3) return true; // Too many URLs
    
    return false;
}

bool ContentValidationPlugin::containsProfanity(const std::string& text) const {
    // Placeholder - can be implemented with a profanity filter library
    // or word list if needed
    return false;
}

// ═══════════════════════════════════════════════════════════
// TrainingTriggerPlugin Implementation
// ═══════════════════════════════════════════════════════════

bool TrainingTriggerPlugin::onTrainingTrigger(const std::vector<Feedback>& batch) const {
    // Check minimum batch size
    if (batch.size() < config_.min_batch_size) {
        return false;
    }
    
    // Check maximum batch size (trigger training if exceeded)
    if (batch.size() >= config_.max_batch_size) {
        return true;
    }
    
    // Check average rating threshold
    float avg_rating = calculateAverageRating(batch);
    if (avg_rating < config_.min_avg_rating) {
        // Low rating - might indicate model issues, trigger training sooner
        return batch.size() >= config_.min_batch_size;
    }
    
    // Check time since oldest feedback
    if (!batch.empty()) {
        auto now = std::chrono::system_clock::now();
        auto oldest = batch.front().timestamp;
        auto age = std::chrono::duration_cast<std::chrono::hours>(now - oldest);
        
        if (age >= config_.max_wait_time) {
            return true; // Time threshold exceeded
        }
    }
    
    return false;
}

float TrainingTriggerPlugin::calculateAverageRating(const std::vector<Feedback>& batch) const {
    if (batch.empty()) return 0.0f;
    
    int64_t sum = 0;
    for (const auto& fb : batch) {
        sum += fb.rating;
    }
    
    return static_cast<float>(sum) / static_cast<float>(batch.size());
}

} // namespace lora
} // namespace llm
} // namespace themis
