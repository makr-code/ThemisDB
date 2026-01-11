#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Feedback entry for LoRA adapter responses
 * 
 * Stores user feedback about model responses and links them to specific
 * LoRA adapters for continuous learning and improvement.
 */
struct Feedback {
    std::string id;                               // Unique feedback ID
    std::string adapter_id;                       // Associated LoRA adapter ID
    std::string user_id;                          // User who provided feedback
    
    // Feedback content
    int rating = 0;                               // Rating (e.g., 1-5 stars)
    std::string feedback_text;                    // Optional text feedback
    
    // Context
    std::string prompt;                           // Original prompt/question
    std::string response;                         // Model response
    std::optional<std::string> model_response_id; // Link to model response (optional)
    
    // Metadata
    std::chrono::system_clock::time_point timestamp;
    bool flagged_for_training = false;            // Flag for training inclusion
    std::string training_category;                // "positive", "negative", "neutral"
    json custom_metadata;                         // Extensible metadata
    
    /**
     * @brief Serialize to JSON
     */
    json toJSON() const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        json j = {
            {"id", id},
            {"adapter_id", adapter_id},
            {"user_id", user_id},
            {"rating", rating},
            {"feedback_text", feedback_text},
            {"prompt", prompt},
            {"response", response},
            {"timestamp", time_t},
            {"flagged_for_training", flagged_for_training},
            {"training_category", training_category}
        };
        
        if (model_response_id.has_value()) {
            j["model_response_id"] = *model_response_id;
        }
        
        if (!custom_metadata.empty()) {
            j["custom_metadata"] = custom_metadata;
        }
        
        return j;
    }
    
    /**
     * @brief Deserialize from JSON
     */
    static Feedback fromJSON(const json& j) {
        Feedback fb;
        
        if (j.contains("id")) fb.id = j["id"].get<std::string>();
        if (j.contains("adapter_id")) fb.adapter_id = j["adapter_id"].get<std::string>();
        if (j.contains("user_id")) fb.user_id = j["user_id"].get<std::string>();
        if (j.contains("rating")) fb.rating = j["rating"].get<int>();
        if (j.contains("feedback_text")) fb.feedback_text = j["feedback_text"].get<std::string>();
        if (j.contains("prompt")) fb.prompt = j["prompt"].get<std::string>();
        if (j.contains("response")) fb.response = j["response"].get<std::string>();
        
        if (j.contains("model_response_id")) {
            fb.model_response_id = j["model_response_id"].get<std::string>();
        }
        
        if (j.contains("timestamp")) {
            std::time_t tt = j["timestamp"].get<std::time_t>();
            fb.timestamp = std::chrono::system_clock::from_time_t(tt);
        } else {
            fb.timestamp = std::chrono::system_clock::now();
        }
        
        if (j.contains("flagged_for_training")) {
            fb.flagged_for_training = j["flagged_for_training"].get<bool>();
        }
        if (j.contains("training_category")) {
            fb.training_category = j["training_category"].get<std::string>();
        }
        if (j.contains("custom_metadata")) {
            fb.custom_metadata = j["custom_metadata"];
        }
        
        return fb;
    }
};

/**
 * @brief Filter options for feedback queries
 */
struct FeedbackFilter {
    std::optional<std::string> adapter_id;        // Filter by adapter
    std::optional<std::string> user_id;           // Filter by user
    std::optional<int> min_rating;                // Minimum rating
    std::optional<bool> flagged_for_training;     // Filter by training flag
    std::optional<std::string> training_category; // Filter by category
    std::optional<std::chrono::system_clock::time_point> since; // Filter by date
    size_t limit = 100;                           // Max results
    size_t offset = 0;                            // Pagination offset
};

} // namespace lora
} // namespace llm
} // namespace themis
