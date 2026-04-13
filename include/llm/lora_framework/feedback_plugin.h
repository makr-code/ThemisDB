/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_plugin.h                                  ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "lora_feedback.h"
#include <memory>
#include <vector>
#include <string>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Plugin interface for extensible feedback validation and processing
 * 
 * Allows customization of feedback handling, validation, and training triggers
 * without modifying core feedback system code.
 * 
 * Example use cases:
 * - Privacy filtering (remove PII from feedback)
 * - Content validation (spam detection, profanity filtering)
 * - Custom training triggers (batch size, quality thresholds)
 * - Data enrichment (sentiment analysis, categorization)
 */
class FeedbackPlugin {
public:
    virtual ~FeedbackPlugin() = default;
    
    /**
     * @brief Validate feedback before storage
     * 
     * @param feedback The feedback to validate
     * @return true if feedback is valid, false to reject
     * 
     * Example: Check for spam, required fields, valid ratings, etc.
     */
    virtual bool validate(const Feedback& feedback) const = 0;
    
    /**
     * @brief Process feedback after validation, before storage
     * 
     * @param feedback The feedback to process (can be modified)
     * 
     * Example: Remove PII, normalize text, add metadata, etc.
     */
    virtual void process(Feedback& feedback) = 0;
    
    /**
     * @brief Called when training might be triggered
     * 
     * @param batch Accumulated feedback batch
     * @return true to trigger training, false to wait for more feedback
     * 
     * Example: Check batch size, quality score, time since last training, etc.
     */
    virtual bool onTrainingTrigger(const std::vector<Feedback>& batch) const = 0;
    
    /**
     * @brief Get plugin name for logging and debugging
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief Base plugin implementation with sensible defaults
 */
class BaseFeedbackPlugin : public FeedbackPlugin {
public:
    bool validate(const Feedback& feedback) const override {
        // Basic validation: check required fields
        if (feedback.adapter_id.empty()) return false;
        if (feedback.user_id.empty()) return false;
        if (feedback.rating < 1 || feedback.rating > 5) return false;
        return true;
    }
    
    void process(Feedback& feedback) override {
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

/**
 * @brief Privacy filter plugin - removes PII from feedback text
 */
class PrivacyFilterPlugin : public BaseFeedbackPlugin {
public:
    void process(Feedback& feedback) override;
    std::string getName() const override { return "PrivacyFilterPlugin"; }
};

/**
 * @brief Content validation plugin - validates feedback content
 */
class ContentValidationPlugin : public BaseFeedbackPlugin {
public:
    bool validate(const Feedback& feedback) const override;
    std::string getName() const override { return "ContentValidationPlugin"; }
    
private:
    bool containsSpam(const std::string& text) const;
    bool containsProfanity(const std::string& text) const;
};

/**
 * @brief Training trigger plugin - advanced training trigger logic
 */
class TrainingTriggerPlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        size_t min_batch_size = 50;
        size_t max_batch_size = 200;
        float min_avg_rating = 3.0f;
        std::chrono::hours max_wait_time{24};
    };
    
    explicit TrainingTriggerPlugin(const Config& config)
        : config_(config) {}
    
    TrainingTriggerPlugin() : TrainingTriggerPlugin(Config{}) {}
    
    bool onTrainingTrigger(const std::vector<Feedback>& batch) const override;
    std::string getName() const override { return "TrainingTriggerPlugin"; }
    
private:
    Config config_;
    float calculateAverageRating(const std::vector<Feedback>& batch) const;
};

/**
 * @brief Cache-aware weighting plugin
 * 
 * Adjusts training weights based on cache status:
 * - Direct (non-cached) responses: weight = 1.0
 * - Cached responses (exact match): weight = 0.3-0.5 (configurable)
 * - Cached responses (semantic match): weight based on similarity
 * 
 * Rationale:
 * - Cached responses are already validated by previous use
 * - Lower weight prevents overtraining on popular queries
 * - Semantic matches get graduated weight based on similarity
 */
class CacheAwareWeightingPlugin : public BaseFeedbackPlugin {
public:
    struct Config {
        float direct_response_weight = 1.0f;         // Weight for direct LLM responses
        float exact_cache_weight = 0.4f;             // Weight for exact cache hits
        float semantic_cache_base_weight = 0.3f;     // Base weight for semantic cache
        float similarity_weight_factor = 0.5f;       // Factor for similarity-based weight
        bool disable_cache_training = false;         // If true, don't train on cached at all
    };
    
    explicit CacheAwareWeightingPlugin(const Config& config)
        : config_(config) {}
    
    CacheAwareWeightingPlugin() : CacheAwareWeightingPlugin(Config{}) {}
    
    void process(Feedback& feedback) override;
    std::string getName() const override { return "CacheAwareWeightingPlugin"; }
    
private:
    Config config_;
    float calculateCacheWeight(const Feedback& feedback) const;
};

} // namespace lora
} // namespace llm
} // namespace themis
