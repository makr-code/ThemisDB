#pragma once

#include "llm/lora_framework/lora_adapter_manager.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/lora_storage_service.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace applications {

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
        std::string base_model = "llama-2-7b";
        std::string docs_database_path = "data/docs_database.json";
        
        // Training settings
        lora::LoRAHyperparameters hyperparameters;
        int feedback_batch_size = 100;  // Train after N feedback items
        std::chrono::hours training_interval{24}; // Or train daily
        
        // Quality settings
        float min_accuracy_threshold = 0.80f;
        bool enable_ab_testing = true;
        bool enable_auto_rollback = true;
    };
    
    explicit ThemisHelpLoRA(const Config& config = Config{});
    ~ThemisHelpLoRA();
    
    // Disable copy
    ThemisHelpLoRA(const ThemisHelpLoRA&) = delete;
    ThemisHelpLoRA& operator=(const ThemisHelpLoRA&) = delete;
    
    /**
     * @brief Query with LoRA adapter
     * @param question User question
     * @return Generated answer
     */
    std::string query(const std::string& question);
    
    /**
     * @brief Add positive feedback for an answer
     * @param question User question
     * @param answer System answer
     */
    void addPositiveFeedback(const std::string& question, const std::string& answer);
    
    /**
     * @brief Add negative feedback with correction
     * @param question User question
     * @param answer System answer (incorrect)
     * @param correction User's correction
     */
    void addNegativeFeedback(
        const std::string& question, 
        const std::string& answer,
        const std::string& correction
    );
    
    /**
     * @brief Trigger training from accumulated feedback
     * @return Training result
     */
    lora::TrainingResult trainFromFeedback();
    
    /**
     * @brief Train from documentation corpus
     * @return Training result
     */
    lora::TrainingResult trainFromDocumentation();
    
    /**
     * @brief Get performance metrics
     * @return Metrics as JSON
     */
    json getMetrics() const;
    
    /**
     * @brief Get feedback statistics
     * @return Statistics as JSON
     */
    json getFeedbackStats() const;
    
    /**
     * @brief Check if adapter is loaded
     * @return true if loaded
     */
    bool isAdapterLoaded() const;
    
    /**
     * @brief Reload adapter (e.g., after training)
     * @return true if reloaded successfully
     */
    bool reloadAdapter();
    
    /**
     * @brief Get current adapter version
     * @return Version string
     */
    std::string getAdapterVersion() const;
    
    /**
     * @brief Rollback to previous version
     * @return true if rolled back successfully
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
 * @brief Feedback entry structure
 */
struct FeedbackEntry {
    std::string question;
    std::string answer;
    bool is_positive;
    std::string correction;  // Only for negative feedback
    std::chrono::system_clock::time_point timestamp;
    bool used_for_training = false;
    
    json toJSON() const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        return json{
            {"question", question},
            {"answer", answer},
            {"is_positive", is_positive},
            {"correction", correction},
            {"timestamp", time_t},
            {"used_for_training", used_for_training}
        };
    }
};

} // namespace applications
} // namespace llm
} // namespace themis
