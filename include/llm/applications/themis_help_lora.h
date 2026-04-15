/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_help_lora.h                                 ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     248                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        std::string adapter_id = "themis_help_lora";
        std::string base_model_id = "llama-2-7b";
        std::string docs_database_path = "data/docs_database.json";
        
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
