/**
 * @file lora_config.h
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
#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief LoRA hyperparameters for training
 */
struct LoRAHyperparameters {
    virtual ~LoRAHyperparameters() = default;
    int rank = 8;                          // LoRA rank (r)
    float alpha = 16.0f;                   // LoRA alpha scaling
    float dropout = 0.1f;                  // Dropout rate
    float learning_rate = 3e-4f;           // Learning rate
    int batch_size = 4;                    // Batch size for training
    int num_epochs = 3;                    // Number of training epochs
    int max_seq_length = 512;              // Maximum sequence length
    
    // Optimizer settings
    std::string optimizer = "adamw";       // Optimizer type: "sgd", "adam", "adamw"
    float beta1 = 0.9f;                    // Adam beta1 (momentum)
    float beta2 = 0.999f;                  // Adam beta2 (RMSprop)
    float epsilon = 1e-8f;                 // Adam epsilon (numerical stability)
    float weight_decay = 0.01f;            // Weight decay / L2 regularization
    float momentum = 0.0f;                 // SGD momentum
    
    // Learning rate scheduling
    std::string lr_scheduler = "constant"; // LR scheduler: "constant", "linear_warmup", "cosine", "cosine_warmup", "step", "exponential"
    int warmup_steps = 0;                  // Warmup steps for schedulers
    float lr_decay_gamma = 0.1f;           // Decay factor for step/exponential schedulers
    int lr_step_size = 100;                // Step size for step decay
    
    // Target modules to apply LoRA
    std::vector<std::string> target_modules = {"q_proj", "v_proj"};
    
    json toJSON() const {
        return json{
            {"rank", rank},
            {"alpha", alpha},
            {"dropout", dropout},
            {"learning_rate", learning_rate},
            {"batch_size", batch_size},
            {"num_epochs", num_epochs},
            {"max_seq_length", max_seq_length},
            {"optimizer", optimizer},
            {"beta1", beta1},
            {"beta2", beta2},
            {"epsilon", epsilon},
            {"weight_decay", weight_decay},
            {"momentum", momentum},
            {"lr_scheduler", lr_scheduler},
            {"warmup_steps", warmup_steps},
            {"lr_decay_gamma", lr_decay_gamma},
            {"lr_step_size", lr_step_size},
            {"target_modules", target_modules}
        };
    }
    
    static LoRAHyperparameters fromJSON(const json& j) {
        LoRAHyperparameters params = {};
        if (j.contains("rank")) {
          params.rank = j["rank"];
        }
        if (j.contains("alpha")) {
          params.alpha = j["alpha"];
        }
        if (j.contains("dropout")) {
          params.dropout = j["dropout"];
        }
        if (j.contains("learning_rate")) {
          params.learning_rate = j["learning_rate"];
        }
        if (j.contains("batch_size")) {
          params.batch_size = j["batch_size"];
        }
        if (j.contains("num_epochs")) {
          params.num_epochs = j["num_epochs"];
        }
        if (j.contains("max_seq_length")) {
          params.max_seq_length = j["max_seq_length"];
        }
        if (j.contains("optimizer")) {
          params.optimizer = j["optimizer"];
        }
        if (j.contains("beta1")) {
          params.beta1 = j["beta1"];
        }
        if (j.contains("beta2")) {
          params.beta2 = j["beta2"];
        }
        if (j.contains("epsilon")) {
          params.epsilon = j["epsilon"];
        }
        if (j.contains("weight_decay")) {
          params.weight_decay = j["weight_decay"];
        }
        if (j.contains("momentum")) {
          params.momentum = j["momentum"];
        }
        if (j.contains("lr_scheduler")) {
          params.lr_scheduler = j["lr_scheduler"];
        }
        if (j.contains("warmup_steps")) {
          params.warmup_steps = j["warmup_steps"];
        }
        if (j.contains("lr_decay_gamma")) {
          params.lr_decay_gamma = j["lr_decay_gamma"];
        }
        if (j.contains("lr_step_size")) {
          params.lr_step_size = j["lr_step_size"];
        }
        if (j.contains("target_modules")) {
          params.target_modules = j["target_modules"].get<std::vector<std::string>>();
        }
        return params;
    }
};

/**
 * @brief QLoRA (Quantized LoRA) configuration
 * 
 * Configuration for memory-efficient QLoRA training with quantized base models.
 */
struct QLoRAConfig {
    bool enabled = false;                              // Enable QLoRA training mode
    std::string quantization_type = "nf4";             // "nf4", "int8", "none"
    size_t block_size = 64;                            // Block size for quantization
    bool use_double_quantization = false;              // Quantize quantization constants
    bool layer_by_layer = true;                        // Layer-by-layer quantization (saves memory)
    
    // Future: Paged optimizer settings
    bool use_paged_optimizer = false;                  // Use paged optimizer (future)
    std::string optimizer_offload = "none";            // "cpu", "none" (future)
    
    json toJSON() const {
        return json{
            {"enabled", enabled},
            {"quantization_type", quantization_type},
            {"block_size", block_size},
            {"use_double_quantization", use_double_quantization},
            {"layer_by_layer", layer_by_layer},
            {"use_paged_optimizer", use_paged_optimizer},
            {"optimizer_offload", optimizer_offload}
        };
    }
    
    static QLoRAConfig fromJSON(const json& j) {
        QLoRAConfig config = {};
        if (j.contains("enabled")) {
          config.enabled = j["enabled"];
        }
        if (j.contains("quantization_type")) {
          config.quantization_type = j["quantization_type"];
        }
        if (j.contains("block_size")) {
          config.block_size = j["block_size"];
        }
        if (j.contains("use_double_quantization")) {
          config.use_double_quantization = j["use_double_quantization"];
        }
        if (j.contains("layer_by_layer")) {
          config.layer_by_layer = j["layer_by_layer"];
        }
        if (j.contains("use_paged_optimizer")) {
          config.use_paged_optimizer = j["use_paged_optimizer"];
        }
        if (j.contains("optimizer_offload")) {
          config.optimizer_offload = j["optimizer_offload"];
        }
        return config;
    }
};

/**
 * @brief LoRA adapter metadata
 */
struct AdapterMetadata {
    virtual ~AdapterMetadata() = default;
    std::string adapter_id;
    std::string version = {};
    std::string base_model;
    std::string description;
    int training_samples = 0;
    float validation_accuracy = 0.0f;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    json custom_metadata;
    uint32_t encryption_key_version = 0;  // KEK version used for encryption (0 = unencrypted or latest)
    
    // Cross-shard sync fields
    std::string checksum;                  // SHA-256 checksum for integrity
    std::string signature;                 // Digital signature for authenticity
    
    json toJSON() const {
        auto created_time_t = std::chrono::system_clock::to_time_t(created_at);
        auto updated_time_t = std::chrono::system_clock::to_time_t(updated_at);
        
        return json{
            {"adapter_id", adapter_id},
            {"version", version},
            {"base_model", base_model},
            {"description", description},
            {"training_samples", training_samples},
            {"validation_accuracy", validation_accuracy},
            {"created_at", created_time_t},
            {"updated_at", updated_time_t},
            {"custom_metadata", custom_metadata},
            {"encryption_key_version", encryption_key_version},
            {"checksum", checksum},
            {"signature", signature}
        };
    }
    
    static AdapterMetadata fromJSON(const json& j) {
        AdapterMetadata metadata = {};
        if (j.contains("adapter_id")) {
          metadata.adapter_id = j["adapter_id"];
        }
        if (j.contains("version")) {
          metadata.version = j["version"];
        }
        if (j.contains("base_model")) {
          metadata.base_model = j["base_model"];
        }
        if (j.contains("description")) {
          metadata.description = j["description"];
        }
        if (j.contains("training_samples")) {
          metadata.training_samples = j["training_samples"];
        }
        if (j.contains("validation_accuracy")) {
          metadata.validation_accuracy = j["validation_accuracy"];
        }
        if (j.contains("created_at")) {
            std::time_t created = j["created_at"];
            metadata.created_at = std::chrono::system_clock::from_time_t(created);
        }
        if (j.contains("updated_at")) {
            std::time_t updated = j["updated_at"];
            metadata.updated_at = std::chrono::system_clock::from_time_t(updated);
        }
        if (j.contains("custom_metadata")) {
          metadata.custom_metadata = j["custom_metadata"];
        }
        if (j.contains("encryption_key_version")) {
          metadata.encryption_key_version = j["encryption_key_version"];
        }
        if (j.contains("checksum")) {
          metadata.checksum = j["checksum"];
        }
        if (j.contains("signature")) {
          metadata.signature = j["signature"];
        }
        return metadata;
    }
};

/**
 * @brief LoRA adapter information
 */
struct AdapterInfo {
    virtual ~AdapterInfo() = default;
    std::string adapter_id;
    std::string version;
    std::string base_model;
    std::string description;
    size_t memory_bytes = 0;
    bool is_loaded = false;
    bool is_pinned = false;
    LoRAHyperparameters hyperparameters;
    AdapterMetadata metadata;
    
    json toJSON() const {
        return json{
            {"adapter_id", adapter_id},
            {"version", version},
            {"base_model", base_model},
            {"description", description},
            {"memory_bytes", memory_bytes},
            {"is_loaded", is_loaded},
            {"is_pinned", is_pinned},
            {"hyperparameters", hyperparameters.toJSON()},
            {"metadata", metadata.toJSON()}
        };
    }
};

/**
 * @brief Cache statistics for adapter manager
 */
struct CacheStats {
    virtual ~CacheStats() = default;
    size_t total_loads = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t evictions = 0;
    size_t current_size = 0;
    size_t max_size = 0;
    
    float hitRate() const {
        if (total_loads == 0) {
          return 0.0f;
        }
        return static_cast<float>(cache_hits) / static_cast<float>(total_loads);
    }
    
    json toJSON() const {
        return json{
            {"total_loads", total_loads},
            {"cache_hits", cache_hits},
            {"cache_misses", cache_misses},
            {"evictions", evictions},
            {"current_size", current_size},
            {"max_size", max_size},
            {"hit_rate", hitRate()}
        };
    }
};

/**
 * @brief LoRA adapter configuration
 */
struct LoRAConfig {
    // Adapter settings
    std::string adapter_id;
    std::string adapter_path;
    std::string base_model;
    float scaling = 1.0f;
    
    // Hyperparameters
    LoRAHyperparameters hyperparameters;
    
    // Cache settings
    bool enable_cache = true;
    size_t max_cache_size = 5;            // Maximum number of cached adapters
    std::chrono::seconds cache_ttl{3600}; // Time-to-live for cached adapters
    
    // Storage settings
    std::string storage_backend = "themisdb"; // "themisdb", "filesystem", "s3"
    std::string storage_path = "data/lora_adapters";
    bool enable_versioning = true;
    int max_versions = 5;
    
    json toJSON() const {
        return json{
            {"adapter_id", adapter_id},
            {"adapter_path", adapter_path},
            {"base_model", base_model},
            {"scaling", scaling},
            {"hyperparameters", hyperparameters.toJSON()},
            {"enable_cache", enable_cache},
            {"max_cache_size", max_cache_size},
            {"cache_ttl", cache_ttl.count()},
            {"storage_backend", storage_backend},
            {"storage_path", storage_path},
            {"enable_versioning", enable_versioning},
            {"max_versions", max_versions}
        };
    }
    
    static LoRAConfig fromJSON(const json& j) {
        LoRAConfig config = {};
        if (j.contains("adapter_id")) {
          config.adapter_id = j["adapter_id"];
        }
        if (j.contains("adapter_path")) {
          config.adapter_path = j["adapter_path"];
        }
        if (j.contains("base_model")) {
          config.base_model = j["base_model"];
        }
        if (j.contains("scaling")) {
          config.scaling = j["scaling"];
        }
        if (j.contains("hyperparameters")) {
          config.hyperparameters = LoRAHyperparameters::fromJSON(j["hyperparameters"]);
        }
        if (j.contains("enable_cache")) {
          config.enable_cache = j["enable_cache"];
        }
        if (j.contains("max_cache_size")) {
          config.max_cache_size = j["max_cache_size"];
        }
        if (j.contains("cache_ttl")) {
          config.cache_ttl = std::chrono::seconds(j["cache_ttl"].get<int>());
        }
        if (j.contains("storage_backend")) {
          config.storage_backend = j["storage_backend"];
        }
        if (j.contains("storage_path")) {
          config.storage_path = j["storage_path"];
        }
        if (j.contains("enable_versioning")) {
          config.enable_versioning = j["enable_versioning"];
        }
        if (j.contains("max_versions")) {
          config.max_versions = j["max_versions"];
        }
        return config;
    }
};

} // namespace lora
} // namespace llm
} // namespace themis

