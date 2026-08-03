#pragma once

/**
 * @file i_feedback_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Plugin Interface**: Abstract interface for LLM feedback mechanisms.
 *       No .cpp implementation needed. Implementations provided by plugin system.
 */

#include <nlohmann/json.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief Feedback validation result
 */
enum class FeedbackValidationResult {
    ACCEPT,     // Accept feedback as-is
    REJECT,     // Reject feedback (spam, invalid)
    FLAG,       // Flag for manual review
    MODIFY      // Accept but with modifications
};

/**
 * @brief Result of feedback validation with optional modifications
 */
struct ValidationResponse {
    FeedbackValidationResult result = FeedbackValidationResult::ACCEPT;
    std::optional<std::string> reason;           // Reason for rejection/flag
    std::optional<json> modified_metadata;       // Modified metadata if MODIFY
    std::optional<std::string> modified_comment; // Modified comment if MODIFY
    float confidence_score = 1.0f;               // Confidence in validation (0-1)
    json plugin_data;                            // Plugin-specific data
};

/**
 * @brief Feedback data for validation (simplified structure)
 */
struct FeedbackData {
    std::string question;
    std::string answer;
    std::string correction;
    std::string comment;
    std::string user_id;
    std::string adapter_id;
    std::string model_version;
    bool is_positive = true;
    json metadata;
};

/**
 * @brief Plugin interface for feedback validation and preprocessing
 * 
 * Plugins can implement custom logic for:
 * - Spam detection (e.g., using ML models)
 * - Content moderation
 * - PII detection and redaction
 * - Quality scoring
 * - Custom transformations
 * 
 * Example implementations:
 * - SpamFilterPlugin: ML-based spam detection
 * - PIIDetectionPlugin: Detect and redact PII
 * - QualityScorePlugin: Assign quality scores
 * - CustomAnalyticsPlugin: Extract custom metrics
 */
class IFeedbackPlugin {
public:
    virtual ~IFeedbackPlugin() = default;
    
    /**
     * @brief Get plugin name
     */
    [[nodiscard]] virtual std::string getName() const = 0;
    
    /**
     * @brief Get plugin version
     */
    [[nodiscard]] virtual std::string getVersion() const = 0;
    
    /**
     * @brief Get plugin description
     */
    [[nodiscard]] virtual std::string getDescription() const = 0;
    
    /**
     * @brief Initialize plugin with configuration
     * @param config Plugin-specific configuration
     * @return true if initialization successful
     */
    [[nodiscard]] virtual bool initialize(const json& config) = 0;
    
    /**
     * @brief Validate and optionally preprocess feedback
     * 
     * This is called before feedback is stored in the database.
     * Plugins can:
     * - Accept/reject/flag feedback
     * - Modify metadata or comments (e.g., redact PII)
     * - Add plugin-specific data for later use
     * 
     * @param feedback Feedback data to validate
     * @return ValidationResponse with result and optional modifications
     */
    [[nodiscard]] virtual ValidationResponse validate(const FeedbackData& feedback) = 0;
    
    /**
     * @brief Post-storage hook (optional)
     * 
     * Called after feedback is successfully stored.
     * Useful for:
     * - Analytics collection
     * - Triggering workflows
     * - Sending notifications
     * 
     * @param feedback_id ID of stored feedback
     * @param feedback Original feedback data
     */
    virtual void onFeedbackStored(
        [[maybe_unused]] const std::string& feedback_id,
        [[maybe_unused]] const FeedbackData& feedback) {
        // Default: no-op
    }
    
    /**
     * @brief Shutdown plugin
     * 
     * Called when plugin is being unloaded.
     * Plugins should clean up resources here.
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Get plugin statistics (optional)
     * 
     * Return plugin-specific statistics like:
     * - Number of validations performed
     * - Spam detection accuracy
     * - Processing time metrics
     * 
     * @return JSON object with statistics
     */
    virtual json getStatistics() const {
        return json::object();
    }
};

/**
 * @brief Default no-op feedback plugin
 * 
 * This plugin accepts all feedback without validation.
 * Useful as a starting point or for disabling validation.
 */
class NoOpFeedbackPlugin : public IFeedbackPlugin {
public:
    ~NoOpFeedbackPlugin() override = default;

    std::string getName() const override {
        return "noop";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "No-op feedback plugin - accepts all feedback";
    }
    
    bool initialize([[maybe_unused]] const json& config) override {
        return true;
    }
    
    ValidationResponse validate([[maybe_unused]] const FeedbackData& feedback) override {
        ValidationResponse response;
        response.result = FeedbackValidationResult::ACCEPT;
        return response;
    }
    
    void shutdown() override {
        // No-op
    }
};

/**
 * @brief Basic spam detection plugin (example implementation)
 * 
 * This is a simple example plugin that demonstrates the interface.
 * Production plugins should use more sophisticated ML-based detection.
 */
class BasicSpamDetectionPlugin : public IFeedbackPlugin {
public:
    ~BasicSpamDetectionPlugin() override = default;

    std::string getName() const override {
        return "basic_spam_detection";
    }
    
    std::string getVersion() const override {
        return "1.0.0";
    }
    
    std::string getDescription() const override {
        return "Basic spam detection using keyword matching";
    }
    
    bool initialize(const json& config) override;
    ValidationResponse validate(const FeedbackData& feedback) override;
    void shutdown() override;
    json getStatistics() const override;
    
private:
    std::vector<std::string> spam_keywords_;
    size_t validation_count_ = 0;
    size_t rejected_count_ = 0;
    
    bool containsSpamKeywords(const std::string& text) const;
    bool isLowQuality(const FeedbackData& feedback) const;
};

} // namespace llm
} // namespace themis
