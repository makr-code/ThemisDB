/**
 * @file lora_training_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_training_config.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

LoRATrainingConfig LoRATrainingConfig::loadFromFile(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        return loadFromString(YAML::Dump(config));
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load LoRA training config from " + 
                               config_path + ": " + e.what());
    }
}

LoRATrainingConfig LoRATrainingConfig::loadFromString(const std::string& yaml_content) {
    LoRATrainingConfig config;
    
    try {
        YAML::Node root = YAML::Load(yaml_content);
        
        // Load adapters
        if (root["adapters"]) {
            for (const auto& adapter : root["adapters"]) {
                std::string adapter_id = adapter.first.as<std::string>();
                auto adapter_config = parseAdapterConfig(adapter_id, adapter.second);
                config.adapters_[adapter_id] = adapter_config;
            }
        }
        
        // Load global settings
        if (root["training_defaults"]) {
            auto defaults = root["training_defaults"];
            if (defaults["concurrency"]) {
                if (defaults["concurrency"]["max_concurrent_trainings"]) {
                    config.max_concurrent_trainings_ = 
                        defaults["concurrency"]["max_concurrent_trainings"].as<int>();
                }
            }
            if (defaults["retry"]) {
                if (defaults["retry"]["max_attempts"]) {
                    config.max_retry_attempts_ = 
                        defaults["retry"]["max_attempts"].as<int>();
                }
            }
        }
        
        spdlog::info("Loaded LoRA training config with {} adapters", 
                    config.adapters_.size());
        
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse LoRA training config: " + 
                               std::string(e.what()));
    }
    
    return config;
}

std::optional<LoRATrainingConfig::AdapterConfig> 
LoRATrainingConfig::getAdapterConfig(const std::string& adapter_id) const {
    auto it = adapters_.find(adapter_id);
    if (it != adapters_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LoRATrainingConfig::AdapterConfig> 
LoRATrainingConfig::getAllAdapterConfigs() const {
    std::vector<AdapterConfig> configs = {};

    for (const auto& [id, config] : adapters_) {
        if (config.enabled) {
            configs.push_back(config);
        }
    }
    return configs;
}

std::shared_ptr<CacheAwareWeightingPlugin> 
LoRATrainingConfig::createCacheWeightingPlugin(const std::string& adapter_id) const {
    auto adapter_config = getAdapterConfig(adapter_id);
    if (!adapter_config) {
        throw std::runtime_error("Adapter config not found: " + adapter_id);
    }
    
    CacheAwareWeightingPlugin::Config plugin_config;
    plugin_config.direct_response_weight = 
        adapter_config->feedback_weighting.direct_response_weight;
    plugin_config.exact_cache_weight = 
        adapter_config->feedback_weighting.exact_cache_weight;
    plugin_config.semantic_cache_base_weight = 
        adapter_config->feedback_weighting.semantic_cache_base_weight;
    plugin_config.similarity_weight_factor = 
        adapter_config->feedback_weighting.similarity_weight_factor;
    plugin_config.disable_cache_training = 
        adapter_config->feedback_weighting.disable_cache_training;
    
    return std::make_shared<CacheAwareWeightingPlugin>(plugin_config);
}

std::shared_ptr<TrainingTriggerPlugin> 
LoRATrainingConfig::createTrainingTriggerPlugin(const std::string& adapter_id) const {
    auto adapter_config = getAdapterConfig(adapter_id);
    if (!adapter_config) {
        throw std::runtime_error("Adapter config not found: " + adapter_id);
    }
    
    TrainingTriggerPlugin::Config plugin_config;
    plugin_config.min_batch_size = adapter_config->triggers.min_batch_size;
    plugin_config.max_batch_size = adapter_config->triggers.max_batch_size;
    plugin_config.min_avg_rating = adapter_config->triggers.min_avg_rating;
    plugin_config.max_wait_time = 
        std::chrono::hours{adapter_config->triggers.max_wait_hours};
    
    return std::make_shared<TrainingTriggerPlugin>(plugin_config);
}

bool LoRATrainingConfig::validate() const {
    return getValidationErrors().empty();
}

std::vector<std::string> LoRATrainingConfig::getValidationErrors() const {
    std::vector<std::string> errors;
    
    // Check if we have at least one adapter
    if (adapters_.empty()) {
        errors.push_back("No adapters configured");
    }
    
    // Validate each adapter
    for (const auto& [id, config] : adapters_) {
        if (config.base_model_name.empty()) {
            errors.push_back("Adapter " + id + ": base_model_name is empty");
        }
        if (config.base_model_path.empty()) {
            errors.push_back("Adapter " + id + ": base_model_path is empty");
        }
        if (config.hyperparameters.rank <= 0) {
            errors.push_back("Adapter " + id + ": invalid rank");
        }
        if (config.hyperparameters.learning_rate <= 0.0f) {
            errors.push_back("Adapter " + id + ": invalid learning_rate");
        }
        if (config.feedback_weighting.direct_response_weight < 0.0f ||
            config.feedback_weighting.direct_response_weight > 1.0f) {
            errors.push_back("Adapter " + id + ": invalid direct_response_weight");
        }
    }
    
    return errors;
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

LoRATrainingConfig::AdapterConfig 
LoRATrainingConfig::parseAdapterConfig(
    const std::string& adapter_id,
    const YAML::Node& node
) {
    AdapterConfig config;
    config.adapter_id = adapter_id;
    
    if (node["enabled"]) {
        config.enabled = node["enabled"].as<bool>();
    }
    
    // Base model
    if (node["base_model"]) {
        auto base = node["base_model"];
        if (base["name"]) {
          config.base_model_name = base["name"].as<std::string>();
        }
        if (base["path"]) {
          config.base_model_path = base["path"].as<std::string>();
        }
        if (base["type"]) {
          config.base_model_type = base["type"].as<std::string>();
        }
    }
    
    // Hyperparameters
    if (node["hyperparameters"]) {
        config.hyperparameters = parseHyperparameters(node["hyperparameters"]);
    }
    
    // Training data
    if (node["training_data"]) {
        for (const auto& data : node["training_data"]) {
            std::string source_name = data.first.as<std::string>();
            config.training_data[source_name] = 
                parseTrainingDataSource(data.second);
        }
    }
    
    // Feedback weighting
    if (node["training_data"] && node["training_data"]["feedback"]) {
        auto feedback = node["training_data"]["feedback"];
        if (feedback["weighting"]) {
            config.feedback_weighting = 
                parseFeedbackWeighting(feedback["weighting"]);
        }
    }
    
    // Training triggers
    if (node["triggers"]) {
        config.triggers = parseTrainingTrigger(node["triggers"]);
    }
    
    // Quality config
    if (node["quality"]) {
        config.quality = parseQualityConfig(node["quality"]);
    }
    
    // Device settings
    if (node["pipeline"] && node["pipeline"]["execution"]) {
        auto exec = node["pipeline"]["execution"];
        if (exec["device"]) {
          config.device = exec["device"].as<std::string>();
        }
        if (exec["device_id"]) {
          config.device_id = exec["device_id"].as<int>();
        }
        if (exec["mixed_precision"]) 
            config.mixed_precision = exec["mixed_precision"].as<bool>();
        if (exec["gradient_accumulation_steps"])
            config.gradient_accumulation_steps = 
                exec["gradient_accumulation_steps"].as<int>();
    }
    
    return config;
}

LoRAHyperparameters LoRATrainingConfig::parseHyperparameters(const YAML::Node& node) {
    LoRAHyperparameters params;
    
    if (node["rank"]) {
      params.rank = node["rank"].as<int>();
    }
    if (node["alpha"]) {
      params.alpha = node["alpha"].as<float>();
    }
    if (node["dropout"]) {
      params.dropout = node["dropout"].as<float>();
    }
    if (node["learning_rate"]) 
        params.learning_rate = node["learning_rate"].as<float>();
    if (node["batch_size"]) {
      params.batch_size = node["batch_size"].as<int>();
    }
    if (node["num_epochs"]) {
      params.num_epochs = node["num_epochs"].as<int>();
    }
    if (node["max_seq_length"]) 
        params.max_seq_length = node["max_seq_length"].as<int>();
    
    if (node["target_modules"]) {
        params.target_modules.clear();
        for (const auto& module : node["target_modules"]) {
            params.target_modules.push_back(module.as<std::string>());
        }
    }
    
    return params;
}

LoRATrainingConfig::FeedbackWeighting 
LoRATrainingConfig::parseFeedbackWeighting(const YAML::Node& node) {
    FeedbackWeighting weighting;
    
    if (node["direct_response_weight"])
        weighting.direct_response_weight = node["direct_response_weight"].as<float>();
    if (node["exact_cache_weight"])
        weighting.exact_cache_weight = node["exact_cache_weight"].as<float>();
    if (node["semantic_cache_base_weight"])
        weighting.semantic_cache_base_weight = 
            node["semantic_cache_base_weight"].as<float>();
    if (node["similarity_weight_factor"])
        weighting.similarity_weight_factor = 
            node["similarity_weight_factor"].as<float>();
    if (node["disable_cache_training"])
        weighting.disable_cache_training = 
            node["disable_cache_training"].as<bool>();
    
    // Type weights
    if (node["type_weights"]) {
        for (const auto& weight : node["type_weights"]) {
            std::string type = weight.first.as<std::string>();
            float value = weight.second.as<float>();
            weighting.type_weights[type] = value;
        }
    }
    
    // Rating weights
    if (node["rating_weights"]) {
        for (const auto& weight : node["rating_weights"]) {
            int rating = weight.first.as<int>();
            float value = weight.second.as<float>();
            weighting.rating_weights[rating] = value;
        }
    }
    
    return weighting;
}

LoRATrainingConfig::TrainingTrigger 
LoRATrainingConfig::parseTrainingTrigger(const YAML::Node& node) {
    TrainingTrigger trigger;
    
    if (node["automatic"]) {
        auto automatic = node["automatic"];
        if (automatic["enabled"])
            trigger.automatic_enabled = automatic["enabled"].as<bool>();
        
        if (automatic["batch_size"]) {
            auto batch = automatic["batch_size"];
            if (batch["min"]) {
              trigger.min_batch_size = batch["min"].as<size_t>();
            }
            if (batch["max"]) {
              trigger.max_batch_size = batch["max"].as<size_t>();
            }
            if (batch["use_effective_size"])
                trigger.use_effective_size = batch["use_effective_size"].as<bool>();
        }
        
        if (automatic["time"]) {
            auto time = automatic["time"];
            if (time["max_wait_hours"])
                trigger.max_wait_hours = time["max_wait_hours"].as<int>();
            if (time["cron_schedule"])
                trigger.cron_schedule = time["cron_schedule"].as<std::string>();
        }
        
        if (automatic["quality"]) {
            auto quality = automatic["quality"];
            if (quality["min_avg_rating"])
                trigger.min_avg_rating = quality["min_avg_rating"].as<float>();
            if (quality["min_positive_ratio"])
                trigger.min_positive_ratio = 
                    quality["min_positive_ratio"].as<float>();
        }
    }
    
    return trigger;
}

LoRATrainingConfig::QualityConfig 
LoRATrainingConfig::parseQualityConfig(const YAML::Node& node) {
    QualityConfig quality;
    
    if (node["ab_testing"]) {
        auto ab = node["ab_testing"];
        if (ab["enabled"]) {
          quality.ab_testing_enabled = ab["enabled"].as<bool>();
        }
        if (ab["traffic_split"]) 
            quality.traffic_split = ab["traffic_split"].as<float>();
        if (ab["duration_hours"]) 
            quality.duration_hours = ab["duration_hours"].as<int>();
        if (ab["min_improvement"]) 
            quality.min_improvement = ab["min_improvement"].as<float>();
    }
    
    if (node["auto_rollback"]) {
        auto rollback = node["auto_rollback"];
        if (rollback["enabled"]) 
            quality.auto_rollback_enabled = rollback["enabled"].as<bool>();
        if (rollback["cooldown_hours"])
            quality.cooldown_hours = rollback["cooldown_hours"].as<int>();
        if (rollback["triggers"]) {
            for (const auto& trigger : rollback["triggers"]) {
                quality.rollback_triggers.push_back(trigger.as<std::string>());
            }
        }
    }
    
    return quality;
}

LoRATrainingConfig::TrainingDataSource 
LoRATrainingConfig::parseTrainingDataSource(const YAML::Node& node) {
    TrainingDataSource source;
    
    if (node["enabled"]) {
      source.enabled = node["enabled"].as<bool>();
    }
    if (node["path"]) {
      source.path = node["path"].as<std::string>();
    }
    if (node["weight"]) {
      source.weight = node["weight"].as<float>();
    }
    
    if (node["preprocessing"]) {
        for (const auto& prep : node["preprocessing"]) {
            std::string key = prep.first.as<std::string>();
            std::string value = prep.second.as<std::string>();
            source.preprocessing[key] = value;
        }
    }
    
    return source;
}

} // namespace lora
} // namespace llm
} // namespace themis

