/**
 * @file feedback_store.h
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
#include <optional>
#include <memory>
#include <cstdint>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>
#include "llm/i_feedback_plugin.h"

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace llm {

// Forward declaration for graph edge type
namespace lora {
    enum class LoRAEdgeType;
    struct LoRAGraphEdge;
}


enum class FeedbackType {
    POSITIVE,   // User found the response helpful
    NEGATIVE    // User found the response unhelpful or incorrect
};

enum class ValidationStatus {
    PENDING,    // Not yet validated
    APPROVED,   // Approved for training
    REJECTED,   // Rejected (spam, invalid, etc.)
    FLAGGED     // Flagged for manual review
};

class FeedbackStore {
public:
    using SpamKeywordsProviderFn = std::function<std::vector<std::string>()>;

    struct FeedbackEntry {
        std::string id;                        // UUID or generated ID
        std::string interaction_id;            // Reference to LLM interaction
        std::string user_id;                   // User who provided feedback
        FeedbackType type;                     // POSITIVE or NEGATIVE
        std::string question;                  // Original question
        std::string answer;                    // System answer
        std::string correction;                // User's correction (for negative feedback)
        std::string comment;                   // Optional user comment
        int64_t timestamp_ms = 0;              // Creation timestamp
        ValidationStatus validation_status;    // Validation state
        std::string model_version;             // Model version that generated the answer
        std::string adapter_id;                // LoRA adapter ID (if used)
        std::string adapter_version;           // LoRA adapter version
        bool used_for_training = false;        // Whether used in training
        int training_batch_id = 0;             // Training batch ID (0 = not trained)
        nlohmann::json metadata;               // Additional fields

        // Serialization
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static FeedbackEntry fromJson(const nlohmann::json& j);
    };

    struct ListOptions {
        size_t limit = 100;                          // Max entries to return
        std::optional<std::string> start_after_id;   // Pagination cursor
        std::optional<FeedbackType> filter_type;     // Filter by feedback type
        std::optional<ValidationStatus> filter_status; // Filter by validation status
        std::optional<std::string> filter_model;     // Filter by model version
        std::optional<std::string> filter_adapter;   // Filter by adapter ID
        std::optional<int64_t> since_timestamp_ms;   // Filter by time
        std::optional<bool> unused_for_training;     // Only unused entries
    };

    struct Stats {
        size_t total_feedback = 0;
        size_t positive_count = 0;
        size_t negative_count = 0;
        size_t pending_validation = 0;
        size_t approved_count = 0;
        size_t rejected_count = 0;
        size_t unused_for_training = 0;
        size_t used_for_training = 0;
        double positive_ratio = 0.0;
    };

    explicit FeedbackStore(rocksdb::TransactionDB* db, 
                          rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~FeedbackStore() = default;
    
    /**
     * @brief Set Validation Plugin.
     * @param[in] plugin Input parameter.
     */
    void setValidationPlugin(std::shared_ptr<IFeedbackPlugin> plugin);
    
    /**
     * @brief Get Validation Plugin.
     * @return Return value.
     */
    std::shared_ptr<IFeedbackPlugin> getValidationPlugin() const;

    /**
     * @brief Set Spam Keywords Provider.
     * @param[in] provider Input parameter.
     */
    static void setSpamKeywordsProvider(SpamKeywordsProviderFn provider);

    /**
     * @brief Clear Spam Keywords Provider.
     */
    static void clearSpamKeywordsProvider();

    /**
     * @brief Create Feedback.
     * @param[in] feedback Input parameter.
     * @return Return value.
     */
    FeedbackEntry createFeedback(FeedbackEntry feedback);

    /**
     * @brief Get Feedback.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<FeedbackEntry> getFeedback(const std::string& id) const;

    /**
     * @brief List Feedback.
     * @return Return value.
     */
    std::vector<FeedbackEntry> listFeedback() const;
    /**
     * @brief List Feedback.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<FeedbackEntry> listFeedback(const ListOptions& options) const;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Delete Feedback.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteFeedback(const std::string& id);

    /**
     * @brief Update Validation Status.
     * @param[in] id Input parameter.
     * @param[in] status Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateValidationStatus(const std::string& id, ValidationStatus status);

    /**
     * @brief Mark Used For Training.
     * @param[in] id Input parameter.
     * @param[in] batch_id Identifier of the batch.
     * @return True when the operation succeeds.
     */
    bool markUsedForTraining(const std::string& id, int batch_id);

    /**
     * @brief Validate Feedback.
     * @param[in] feedback Input parameter.
     * @return Return value.
     */
    static ValidationStatus validateFeedback(const FeedbackEntry& feedback);

    /**
     * @brief Set Spam Keywords Provider Fn.
     * @param[in] fn Input parameter.
     */
    static void setSpamKeywordsProviderFn(SpamKeywordsProviderFn fn);

    /**
     * @brief Clear Spam Keywords Provider Fn.
     */
    static void clearSpamKeywordsProviderFn();

    /**
     * @brief Clear.
     */
    void clear();
    
    // ===== Graph Link Methods =====
    
    bool createAdapterLink(
        const std::string& feedback_id,
        const std::string& adapter_id,
        const nlohmann::json& metadata = nlohmann::json::object());
    
    /**
     * @brief Get Feedback For Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<FeedbackEntry> getFeedbackForAdapter(
        const std::string& adapter_id,
        const ListOptions& options) const;
    /**
     * @brief Get Feedback For Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::vector<FeedbackEntry> getFeedbackForAdapter(
        const std::string& adapter_id) const;
    
    /**
     * @brief Get Linked Adapters.
     * @param[in] feedback_id Identifier of the feedback.
     * @return Return value.
     */
    std::vector<std::string> getLinkedAdapters(const std::string& feedback_id) const;
    
    /**
     * @brief Is Linked To Adapter.
     * @param[in] feedback_id Identifier of the feedback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return True when the operation succeeds.
     */
    bool isLinkedToAdapter(
        const std::string& feedback_id,
        const std::string& adapter_id) const;

    /**
     * @brief Get Spam Keywords.
     * @return Return value.
     */
    static std::vector<std::string> getSpamKeywords();

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_; // nullptr = default CF
    std::shared_ptr<IFeedbackPlugin> validation_plugin_;

    static constexpr const char* KEY_PREFIX = "help_feedback:";
    static constexpr const char* GRAPH_EDGE_PREFIX = "feedback_graph_edge:";
    
    /**
     * @brief Make Key.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::string makeKey(const std::string& id) const;
    /**
     * @brief Make Graph Edge Key.
     * @param[in] feedback_id Identifier of the feedback.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    std::string makeGraphEdgeKey(const std::string& feedback_id, 
                                  const std::string& adapter_id) const;
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    std::string generateId() const;
    
    /**
     * @brief Spam detection configuration (deprecated, use plugin instead)
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isLikelySpam(const std::string& text);
    
    /**
     * @brief Helper: Apply plugin validation if available
     * @param[in,out] feedback Input/output parameter.
     * @return Return value.
     */
    ValidationStatus applyPluginValidation(FeedbackEntry& feedback);
};

} // namespace llm
} // namespace themis
