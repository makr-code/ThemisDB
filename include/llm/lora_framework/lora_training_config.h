/**
 * @file lora_training_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief LoRA training configuration loaded from YAML
 * 
 * Loads complete training configuration including:
 * - Hyperparameters
 * - Training data sources
 * - Feedback weighting
 * - Training triggers
 * - Quality thresholds
 * - Monitoring settings
 */
class LoRATrainingConfig {
public:
    virtual ~LoRATrainingConfig() = default;
    /**
     * @brief Training data source configuration
     */
    struct TrainingDataSource {
        bool enabled = true;
        std::string path;
        float weight = 1.0f;
        std::map<std::string, std::string> preprocessing;
    };
    
    /**
     * @brief Feedback weighting configuration
     */
    struct FeedbackWeighting {
        float direct_response_weight = 1.0f;
        float exact_cache_weight = 0.4f;
        float semantic_cache_base_weight = 0.3f;
        float similarity_weight_factor = 0.5f;
        bool disable_cache_training = false;
        
        std::map<std::string, float> type_weights;     // positive, negative, neutral
        std::map<int, float> rating_weights;           // 1-5
    };
    
    /**
     * @brief Training trigger configuration
     */
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
    
    /**
     * @brief Quality assurance configuration
     */
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
    
    /**
     * @brief Adapter-specific configuration
     */
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
     * @brief Load configuration from YAML file
     * @param config_path Path to YAML configuration file
     * @return Configuration object
     */
    static LoRATrainingConfig loadFromFile(const std::string& config_path);
    
    /**
     * @brief Load configuration from YAML string
     * @param yaml_content YAML content as string
     * @return Configuration object
     */
    static LoRATrainingConfig loadFromString(const std::string& yaml_content);
    
    /**
     * @brief Get adapter configuration by ID
     * @param adapter_id Adapter identifier
     * @return Adapter configuration if found
     */
    std::optional<AdapterConfig> getAdapterConfig(const std::string& adapter_id) const;
    
    /**
     * @brief Get all adapter configurations
     * @return Vector of all adapter configs
     */
    std::vector<AdapterConfig> getAllAdapterConfigs() const;
    
    /**
     * @brief Create cache weighting plugin from config
     * @param adapter_id Adapter identifier
     * @return Configured plugin
     */
    std::shared_ptr<CacheAwareWeightingPlugin> createCacheWeightingPlugin(
        const std::string& adapter_id
    ) const;
    
    /**
     * @brief Create training trigger plugin from config
     * @param adapter_id Adapter identifier
     * @return Configured plugin
     */
    std::shared_ptr<TrainingTriggerPlugin> createTrainingTriggerPlugin(
        const std::string& adapter_id
    ) const;
    
    /**
     * @brief Validate configuration
     * @return true if valid, false otherwise
     */
    bool validate() const;
    
    /**
     * @brief Get validation errors
     * @return Vector of error messages
     */
    std::vector<std::string> getValidationErrors() const;

private:
    std::map<std::string, AdapterConfig> adapters_;
    
    // Global settings
    int max_concurrent_trainings_ = 2;
    int max_retry_attempts_ = 3;
    
    // Helper methods
    static AdapterConfig parseAdapterConfig(
        const std::string& adapter_id,
        const YAML::Node& node
    );
    
    static LoRAHyperparameters parseHyperparameters(const YAML::Node& node);
    static FeedbackWeighting parseFeedbackWeighting(const YAML::Node& node);
    static TrainingTrigger parseTrainingTrigger(const YAML::Node& node);
    static QualityConfig parseQualityConfig(const YAML::Node& node);
    static TrainingDataSource parseTrainingDataSource(const YAML::Node& node);
};

} // namespace lora
} // namespace llm
} // namespace themis
