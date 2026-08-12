/**
 * @file themis_help_lora.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/lora_config.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/feedback_store.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace applications {

using json = nlohmann::json;
using llm::FeedbackType;  // Make FeedbackType available in this namespace

/**
 * @brief Performance metrics for ThemisHelpLoRA
 */
struct PerformanceMetrics {
    virtual ~PerformanceMetrics() = default;
    int64_t total_queries = 0;
    int64_t successful_queries = 0;
    int64_t failed_queries = 0;
    double success_rate = 0.0;
    double average_latency_ms = 0.0;
    double cache_hit_rate = 0.0;
};

/**
 * @brief Feedback statistics
 */
struct FeedbackStats {
    virtual ~FeedbackStats() = default;
    size_t total_feedback = 0;
    size_t positive_feedback = 0;
    size_t negative_feedback = 0;
    double positive_ratio = 0.0;
};

/**
 * @brief ThemisDB Documentation Assistant with LoRA fine-tuning
 * 
 * First application of the LoRA framework for domain-specific task:
 * - Improve accuracy for ThemisDB-specific questions
 * - Learn from user corrections and feedback
 * - Adapt to evolving documentation
 * - Reduce hallucinations on ThemisDB features
 */
class ThemisHelpLoRA {
public:
    /**
     * @brief Configuration for ThemisHelpLoRA
     */
    struct Config {
        using ModelPathProviderFn = std::function<std::string(const std::string& model_id)>;

        std::string adapter_id = "themis_help_lora";
        std::string base_model_id = "llama-2-7b";
        std::string docs_database_path = "data/docs_database.json";
        /**
         * @brief Optional GGUF path resolver for @ref base_model_id.
         *
         * When set, this callback is queried first for both lazy inference-time
         * model loading and training-service base-model initialization.
         * Return an empty string to fall back to the default local path
         * `models/<base_model_id>.gguf`.
         */
        ModelPathProviderFn model_path_provider;
        
        // Remote model loading (Ollama support)
        bool enable_remote_loading = false;
        std::string ollama_url = "http://localhost:11434";
        std::string ollama_model_name = "llama2:7b";
        std::string model_config_yaml = "config/llm_remote_models.yaml";
        bool auto_download_model = true;

        // Dependencies (to be injected)
        rocksdb::TransactionDB* db = nullptr;
        std::shared_ptr<storage::BlobStorageManager> blob_manager;

        // Training settings
        lora::LoRAHyperparameters hyperparameters;
        int feedback_batch_size = 100;  // Train after N feedback items
        std::chrono::hours training_interval{24}; // Or train daily

        // Quality settings
        float min_accuracy_threshold = 0.80f;
        bool enable_ab_testing = true;
        bool enable_auto_rollback = true;

        /**
         * @brief Optional model-path resolver injected at startup.
         *
         * When set, the resolver is called with @p base_model_id and must
         * return the absolute filesystem path to the GGUF model file.
         * Implement via `LLMModelStorage::resolveGGUFPath(model_id)` and wire
         * at server startup.
         *
         * When not set, the component falls back to the relative path
         * `"models/" + base_model_id + ".gguf"`, which is only correct when
         * the server working directory contains a `models/` sub-directory.
         *
         * @param model_id The base_model_id string from this Config.
         * @return Absolute path to the GGUF file, or empty on resolution failure.
         */
    };

    explicit ThemisHelpLoRA(const Config& config);
    ThemisHelpLoRA();
    ~ThemisHelpLoRA();
    
    // Disable copy
    ThemisHelpLoRA(const ThemisHelpLoRA&) = delete;
    ThemisHelpLoRA& operator=(const ThemisHelpLoRA&) = delete;
    
    /**
     * @brief Query with LoRA adapter
     * @param question User question
     * @param user_id Optional user ID for logging (default: "anonymous")
     * @return Generated answer
     */
    std::string query(const std::string& question, const std::string& user_id = "anonymous");
    
    /**
     * @brief Add positive feedback for an answer
     * @param question User question
     * @param answer System answer
     * @param user_id Optional user ID (default: "anonymous")
     */
    void addPositiveFeedback(
        const std::string& question, 
        const std::string& answer,
        const std::string& user_id = "anonymous"
    );
    
    /**
     * @brief Add negative feedback with correction
     * @param question User question
     * @param answer System answer (incorrect)
     * @param correction User's correction
     * @param user_id Optional user ID (default: "anonymous")
     */
    void addNegativeFeedback(
        const std::string& question, 
        const std::string& answer,
        const std::string& correction,
        const std::string& user_id = "anonymous"
    );
    
    /**
     * @brief Trigger training from accumulated feedback
     * @return true if training successful
     */
    bool trainFromFeedback();
    
    /**
     * @brief Train from documentation corpus
     * @return true if training successful
     */
    bool trainFromDocumentation();
    
    /**
     * @brief Get performance metrics
     * @return Metrics structure
     */
    PerformanceMetrics getMetrics() const;
    
    /**
     * @brief Get feedback statistics
     * @return Statistics structure
     */
    FeedbackStats getFeedbackStats() const;
    
    /**
     * @brief Get current adapter version
     * @return Version string
     */
    std::string getVersion() const;
    
    /**
     * @brief Check if adapter is trained
     * @return true if trained
     */
    bool isTrained() const;

    /**
     * @brief Check if the adapter is currently loaded
     */
    bool isAdapterLoaded() const;

    /**
     * @brief Reload the adapter after an update
     */
    bool reloadAdapter();

    /**
     * @brief Get current adapter version
     */
    std::string getAdapterVersion() const;

    /**
     * @brief Roll back to the previous adapter version
     */
    bool rollbackToPreviousVersion();
    
private:
    /**
     * @brief Helper to increment version string
     * @param version Current version
     * @return Incremented version
     */
    static std::string incrementVersion(const std::string& version);
    
    /**
     * @brief Helper to decrement version string
     * @param version Current version
     * @return Decremented version
     */
    static std::string decrementVersion(const std::string& version);

    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Feedback item for internal buffering
 */
struct FeedbackItem {
    std::string question;
    std::string answer;
    std::string correction;
    FeedbackType feedback_type;
    std::string user_id;
    std::chrono::system_clock::time_point timestamp;
    bool used_for_training = false;
};

/**
 * @brief Helper function to generate unique request IDs
 */
inline std::string generateModelRequestId() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return "req_" + std::to_string(millis);
}

} // namespace applications
} // namespace llm
} // namespace themis
