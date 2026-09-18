/**
 * @file themis_help_lora.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct PerformanceMetrics {
    /**
     * @brief Performance Metrics.
     * @return Return value.
     */
    virtual ~PerformanceMetrics() = default;
    int64_t total_queries = 0;
    int64_t successful_queries = 0;
    int64_t failed_queries = 0;
    double success_rate = 0.0;
    double average_latency_ms = 0.0;
    double cache_hit_rate = 0.0;
};

struct FeedbackStats {
    /**
     * @brief Feedback Stats.
     * @return Return value.
     */
    virtual ~FeedbackStats() = default;
    size_t total_feedback = 0;
    size_t positive_feedback = 0;
    size_t negative_feedback = 0;
    double positive_ratio = 0.0;
};

class ThemisHelpLoRA {
public:
    struct Config {
        using ModelPathProviderFn = std::function<std::string(const std::string& model_id)>;

        std::string adapter_id = "themis_help_lora";
        std::string base_model_id = "llama-2-7b";
        std::string docs_database_path = "data/docs_database.json";
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

    };

    /**
     * @brief Themis Help Lo RA.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ThemisHelpLoRA(const Config& config);
    ThemisHelpLoRA();
    ~ThemisHelpLoRA();
    
    // Disable copy
    ThemisHelpLoRA(const ThemisHelpLoRA&) = delete;
    ThemisHelpLoRA& operator=(const ThemisHelpLoRA&) = delete;
    
    std::string query(const std::string& question, const std::string& user_id = "anonymous");
    
    void addPositiveFeedback(
        const std::string& question, 
        const std::string& answer,
        const std::string& user_id = "anonymous"
    );
    
    void addNegativeFeedback(
        const std::string& question, 
        const std::string& answer,
        const std::string& correction,
        const std::string& user_id = "anonymous"
    );
    
    /**
     * @brief Train From Feedback.
     * @return True when the operation succeeds.
     */
    bool trainFromFeedback();
    
    /**
     * @brief Train From Documentation.
     * @return True when the operation succeeds.
     */
    bool trainFromDocumentation();
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    PerformanceMetrics getMetrics() const;
    
    /**
     * @brief Get Feedback Stats.
     * @return Return value.
     */
    FeedbackStats getFeedbackStats() const;
    
    /**
     * @brief Get Version.
     * @return Return value.
     */
    std::string getVersion() const;
    
    /**
     * @brief Is Trained.
     * @return True when the operation succeeds.
     */
    bool isTrained() const;

    /**
     * @brief Is Adapter Loaded.
     * @return True when the operation succeeds.
     */
    bool isAdapterLoaded() const;

    /**
     * @brief Reload Adapter.
     * @return True when the operation succeeds.
     */
    bool reloadAdapter();

    /**
     * @brief Get Adapter Version.
     * @return Return value.
     */
    std::string getAdapterVersion() const;

    /**
     * @brief Rollback To Previous Version.
     * @return True when the operation succeeds.
     */
    bool rollbackToPreviousVersion();
    
private:
    /**
     * @brief Increment Version.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    static std::string incrementVersion(const std::string& version);
    
    /**
     * @brief Decrement Version.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    static std::string decrementVersion(const std::string& version);

    class Impl;
    std::unique_ptr<Impl> impl_;
};

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
 * @brief Generate Model Request Id.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), std::to_string().
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
