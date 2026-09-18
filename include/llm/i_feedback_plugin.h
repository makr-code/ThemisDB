#pragma once

/**
 * @file i_feedback_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

enum class FeedbackValidationResult {
    ACCEPT,     // Accept feedback as-is
    REJECT,     // Reject feedback (spam, invalid)
    FLAG,       // Flag for manual review
    MODIFY      // Accept but with modifications
};

struct ValidationResponse {
    FeedbackValidationResult result = FeedbackValidationResult::ACCEPT;
    std::optional<std::string> reason;           // Reason for rejection/flag
    std::optional<json> modified_metadata;       // Modified metadata if MODIFY
    std::optional<std::string> modified_comment; // Modified comment if MODIFY
    float confidence_score = 1.0f;               // Confidence in validation (0-1)
    json plugin_data;                            // Plugin-specific data
};

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

class IFeedbackPlugin {
public:
    /**
     * @brief IFeedback Plugin.
     * @return Return value.
     */
    virtual ~IFeedbackPlugin() = default;
    
    [[nodiscard]] virtual std::string getName() const = 0;
    
    [[nodiscard]] virtual std::string getVersion() const = 0;
    
    [[nodiscard]] virtual std::string getDescription() const = 0;
    
    [[nodiscard]] virtual bool initialize(const json& config) = 0;
    
    [[nodiscard]] virtual ValidationResponse validate(const FeedbackData& feedback) = 0;
    
    virtual void onFeedbackStored(
        [[maybe_unused]] const std::string& feedback_id,
        [[maybe_unused]] const FeedbackData& feedback) {
        // Default: no-op
    }
    
    /**
     * @brief Shutdown.
     */
    virtual void shutdown() = 0;
    
    virtual json getStatistics() const {
        return json::object();
    }
};

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
    
    /**
     * @brief Contains Spam Keywords.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSpamKeywords(const std::string& text) const;
    /**
     * @brief Is Low Quality.
     * @param[in] feedback Input parameter.
     * @return True when the operation succeeds.
     */
    bool isLowQuality(const FeedbackData& feedback) const;
};

} // namespace llm
} // namespace themis
