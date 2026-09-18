/**
 * @file lora_training_config.h
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
#include <map>
#include <optional>
#include <yaml-cpp/yaml.h>
#include "lora_config.h"
#include "feedback_plugin.h"

namespace themis {
namespace llm {
namespace lora {

class LoRATrainingConfig {
public:
    /**
     * @brief Lo RATraining Config.
     * @return Return value.
     */
    virtual ~LoRATrainingConfig() = default;
    struct TrainingDataSource {
        bool enabled = true;
        std::string path;
        float weight = 1.0f;
        std::map<std::string, std::string> preprocessing;
    };
    
    struct FeedbackWeighting {
        float direct_response_weight = 1.0f;
        float exact_cache_weight = 0.4f;
        float semantic_cache_base_weight = 0.3f;
        float similarity_weight_factor = 0.5f;
        bool disable_cache_training = false;
        
        std::map<std::string, float> type_weights;     // positive, negative, neutral
        std::map<int, float> rating_weights;           // 1-5
    };
    
    struct TrainingTrigger {
        bool automatic_enabled = true;
        size_t min_batch_size = 50;
        size_t max_batch_size = 200;
        bool use_effective_size = true;
        int max_wait_hours = 24;
        std::string cron_schedule;
        float min_avg_rating = 3.5f;
        float min_positive_ratio = 0.6f;
    };
    
    struct QualityConfig {
        // A/B testing
        bool ab_testing_enabled = true;
        float traffic_split = 0.1f;
        int duration_hours = 24;
        float min_improvement = 0.05f;
        
        // Auto-rollback
        bool auto_rollback_enabled = true;
        std::vector<std::string> rollback_triggers;
        int cooldown_hours = 6;
        
        // Thresholds
        float min_accuracy = 0.80f;
        float max_perplexity = 50.0f;
    };
    
    struct AdapterConfig {
        std::string adapter_id;
        bool enabled = true;
        
        // Base model
        std::string base_model_name;
        std::string base_model_path;
        std::string base_model_type;
        
        // Hyperparameters
        LoRAHyperparameters hyperparameters;
        
        // Training data
        std::map<std::string, TrainingDataSource> training_data;
        
        // Feedback configuration
        FeedbackWeighting feedback_weighting;
        
        // Training triggers
        TrainingTrigger triggers;
        
        // Quality settings
        QualityConfig quality;
        
        // Device settings
        std::string device = "cuda";
        int device_id = 0;
        bool mixed_precision = true;
        int gradient_accumulation_steps = 4;
    };
    
    /**
     * @brief Load From File.
     * @param[in] config_path Path to the retention policy configuration file.
     * @return Return value.
     */
    static LoRATrainingConfig loadFromFile(const std::string& config_path);
    
    /**
     * @brief Load From String.
     * @param[in] yaml_content Input parameter.
     * @return Return value.
     */
    static LoRATrainingConfig loadFromString(const std::string& yaml_content);
    
    /**
     * @brief Get Adapter Config.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::optional<AdapterConfig> getAdapterConfig(const std::string& adapter_id) const;
    
    /**
     * @brief Get All Adapter Configs.
     * @return Return value.
     */
    std::vector<AdapterConfig> getAllAdapterConfigs() const;
    
    /**
     * @brief Create Cache Weighting Plugin.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::shared_ptr<CacheAwareWeightingPlugin> createCacheWeightingPlugin(
        const std::string& adapter_id
    ) const;
    
    /**
     * @brief Create Training Trigger Plugin.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::shared_ptr<TrainingTriggerPlugin> createTrainingTriggerPlugin(
        const std::string& adapter_id
    ) const;
    
    /**
     * @brief Validate.
     * @return True when the operation succeeds.
     */
    bool validate() const;
    
    /**
     * @brief Get Validation Errors.
     * @return Return value.
     */
    std::vector<std::string> getValidationErrors() const;

private:
    std::map<std::string, AdapterConfig> adapters_;
    
    // Global settings
    int max_concurrent_trainings_ = 2;
    int max_retry_attempts_ = 3;
    
    // Helper methods
    /**
     * @brief Parse Adapter Config.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static AdapterConfig parseAdapterConfig(
        const std::string& adapter_id,
        const YAML::Node& node
    );
    
    /**
     * @brief Parse Hyperparameters.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static LoRAHyperparameters parseHyperparameters(const YAML::Node& node);
    /**
     * @brief Parse Feedback Weighting.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static FeedbackWeighting parseFeedbackWeighting(const YAML::Node& node);
    /**
     * @brief Parse Training Trigger.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static TrainingTrigger parseTrainingTrigger(const YAML::Node& node);
    /**
     * @brief Parse Quality Config.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static QualityConfig parseQualityConfig(const YAML::Node& node);
    /**
     * @brief Parse Training Data Source.
     * @param[in] node Input parameter.
     * @return Return value.
     */
    static TrainingDataSource parseTrainingDataSource(const YAML::Node& node);
};

} // namespace lora
} // namespace llm
} // namespace themis
