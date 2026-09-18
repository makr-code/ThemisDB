/**
 * @file feedback_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_feedback.h"
#include <memory>
#include <vector>
#include <string>

namespace themis {
namespace llm {
namespace lora {

class FeedbackPlugin {
public:
    /**
     * @brief Feedback Plugin.
     * @return Return value.
     */
    virtual ~FeedbackPlugin() = default;
    
    [[nodiscard]] virtual bool validate(const Feedback& feedback) const = 0;
    
    /**
     * @brief Process.
     * @param[in,out] feedback Input/output parameter.
     */
    virtual void process(Feedback& feedback) = 0;
    
    [[nodiscard]] virtual bool onTrainingTrigger(const std::vector<Feedback>& batch) const = 0;
    
    [[nodiscard]] virtual std::string getName() const = 0;
};

class BaseFeedbackPlugin : public FeedbackPlugin {
public:
    ~BaseFeedbackPlugin() override = default;

    bool validate(const Feedback& feedback) const override {
        // Basic validation: check required fields
        if (feedback.adapter_id.empty()) {
          return false;
        }
        if (feedback.user_id.empty()) {
          return false;
        }
        if (feedback.rating < 1 || feedback.rating > 5) {
          return false;
        }
        return true;
    }

    void process([[maybe_unused]] Feedback& feedback) override {
        // Default: no-op
    }

    bool onTrainingTrigger(const std::vector<Feedback>& batch) const override {
        // Default: trigger when batch reaches 100 items
        return batch.size() >= 100;
    }
    
    std::string getName() const override {
        return "BaseFeedbackPlugin";
    }
};

class PrivacyFilterPlugin : public BaseFeedbackPlugin {
public:
    ~PrivacyFilterPlugin() override = default;
    void process(Feedback& feedback) override;
    std::string getName() const override { return "PrivacyFilterPlugin"; }
};

class ContentValidationPlugin : public BaseFeedbackPlugin {
public:
    ~ContentValidationPlugin() override = default;
    bool validate(const Feedback& feedback) const override;
    std::string getName() const override { return "ContentValidationPlugin"; }
    
private:
    /**
     * @brief Contains Spam.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSpam(const std::string& text) const;
    /**
     * @brief Contains Profanity.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsProfanity(const std::string& text) const;
};

class TrainingTriggerPlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        size_t min_batch_size = 50;
        size_t max_batch_size = 200;
        float min_avg_rating = 3.0f;
        std::chrono::hours max_wait_time{24};
    };
    
    /**
     * @brief Training Trigger Plugin.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TrainingTriggerPlugin(const Config& config)
        : config_(config) {}
    
    TrainingTriggerPlugin() : TrainingTriggerPlugin(Config{}) {}
    ~TrainingTriggerPlugin() override = default;
    
    bool onTrainingTrigger(const std::vector<Feedback>& batch) const override;
    std::string getName() const override { return "TrainingTriggerPlugin"; }
    
private:
    Config config_;
    /**
     * @brief Calculate Average Rating.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    float calculateAverageRating(const std::vector<Feedback>& batch) const;
};

class CacheAwareWeightingPlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        float direct_response_weight = 1.0f;         // Weight for direct LLM responses
        float exact_cache_weight = 0.4f;             // Weight for exact cache hits
        float semantic_cache_base_weight = 0.3f;     // Base weight for semantic cache
        float similarity_weight_factor = 0.5f;       // Factor for similarity-based weight
        bool disable_cache_training = false;         // If true, don't train on cached at all
    };
    
    /**
     * @brief Cache Aware Weighting Plugin.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CacheAwareWeightingPlugin(const Config& config)
        : config_(config) {}
    
    CacheAwareWeightingPlugin() : CacheAwareWeightingPlugin(Config{}) {}
    ~CacheAwareWeightingPlugin() override = default;
    
    void process(Feedback& feedback) override;
    std::string getName() const override { return "CacheAwareWeightingPlugin"; }
    
private:
    Config config_;
    /**
     * @brief Calculate Cache Weight.
     * @param[in] feedback Input parameter.
     * @return Return value.
     */
    float calculateCacheWeight(const Feedback& feedback) const;
};

} // namespace lora
} // namespace llm
} // namespace themis
