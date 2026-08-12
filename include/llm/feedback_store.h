/**
 * @file feedback_store.h
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


/**
 * @brief Feedback type enumeration
 */
enum class FeedbackType {
    POSITIVE,   // User found the response helpful
    NEGATIVE    // User found the response unhelpful or incorrect
};

/**
 * @brief Feedback validation status
 */
enum class ValidationStatus {
    PENDING,    // Not yet validated
    APPROVED,   // Approved for training
    REJECTED,   // Rejected (spam, invalid, etc.)
    FLAGGED     // Flagged for manual review
};

/**
 * @brief Feedback Store - persists and retrieves user feedback for LoRA training
 * 
 * Features:
 * - Support for positive and negative feedback
 * - Optional plugin-based validation and preprocessing
 * - Graph link integration (FEEDBACK_FOR edges to LoRA adapters)
 * - Versioning and timestamping
 * - Integration with LLM interactions
 * - Query and filtering capabilities
 * 
 * Storage: RocksDB with JSON serialization
 * Key format: "help_feedback:{feedback_id}"
 * 
 * Graph Links:
 * - Feedback entries can be linked to LoRA adapters via FEEDBACK_FOR edges
 * - Enables querying feedback by adapter
 * - Supports adapter lineage tracking
 */
class FeedbackStore {
public:
    using SpamKeywordsProviderFn = std::function<std::vector<std::string>()>;

    /**
     * @brief Feedback entry structure
     */
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
        nlohmann::json toJson() const;
        static FeedbackEntry fromJson(const nlohmann::json& j);
    };

    /**
     * @brief List options for querying feedback
     */
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

    /**
     * @brief Feedback statistics
     */
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

    /**
     * @brief Construct FeedbackStore
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     */
    explicit FeedbackStore(rocksdb::TransactionDB* db, 
                          rocksdb::ColumnFamilyHandle* cf = nullptr);

    ~FeedbackStore() = default;
    
    /**
     * @brief Set validation plugin (optional)
     * @param plugin Validation plugin to use (nullptr = no validation)
     * 
     * When a plugin is set, all feedback will be validated through it.
     * If validation fails, the feedback may be rejected or flagged.
     */
    void setValidationPlugin(std::shared_ptr<IFeedbackPlugin> plugin);
    
    /**
     * @brief Get current validation plugin
     */
    std::shared_ptr<IFeedbackPlugin> getValidationPlugin() const;

    /**
     * @brief Set spam-keyword provider callback for runtime-configurable spam detection.
     *
     * When set, the provider is queried during validation and its returned keyword list
     * is used for substring-based spam matching. If the provider is not set, throws, or
     * returns an empty list, FeedbackStore falls back to the built-in default keywords.
     *
     * @param provider Callback returning the current spam keywords.
     */
    static void setSpamKeywordsProvider(SpamKeywordsProviderFn provider);

    /**
     * @brief Remove installed spam-keyword provider (fallback to defaults).
     */
    static void clearSpamKeywordsProvider();

    /**
     * @brief Store a new feedback entry
     * @param feedback Feedback to store (id will be generated if empty)
     * @return Stored feedback with generated ID
     */
    FeedbackEntry createFeedback(FeedbackEntry feedback);

    /**
     * @brief Retrieve feedback by ID
     * @param id Feedback ID
     * @return Feedback entry if found, nullopt otherwise
     */
    std::optional<FeedbackEntry> getFeedback(const std::string& id) const;

    /**
     * @brief List feedback entries with default options.
     * @return Vector of feedback entries.
     */
    std::vector<FeedbackEntry> listFeedback() const;
    /**
     * @brief List feedback entries with optional filters.
     * @param options List options (pagination, filters).
     * @return Vector of feedback entries.
     */
    std::vector<FeedbackEntry> listFeedback(const ListOptions& options) const;

    /**
     * @brief Get feedback statistics
     * @return Stats struct
     */
    Stats getStats() const;

    /**
     * @brief Delete feedback by ID
     * @param id Feedback ID
     * @return true if deleted, false if not found
     */
    bool deleteFeedback(const std::string& id);

    /**
     * @brief Update feedback validation status
     * @param id Feedback ID
     * @param status New validation status
     * @return true if updated, false if not found
     */
    bool updateValidationStatus(const std::string& id, ValidationStatus status);

    /**
     * @brief Mark feedback as used for training
     * @param id Feedback ID
     * @param batch_id Training batch ID
     * @return true if updated, false if not found
     */
    bool markUsedForTraining(const std::string& id, int batch_id);

    /**
     * @brief Validate feedback entry (basic spam/quality checks)
     * 
     * NOTE: This is now optional. When a validation plugin is set,
     * it will be used instead of this basic validation.
     * 
     * @param feedback Feedback entry to validate
     * @return ValidationStatus result
     */
    static ValidationStatus validateFeedback(const FeedbackEntry& feedback);

    /**
     * @brief Install a runtime spam keywords provider.
     *
     * When set, getSpamKeywords() returns the result of this callable instead
     * of the built-in static list, enabling runtime keyword updates.
     * @param fn Callable returning a vector of lowercase spam keyword strings.
     */
    static void setSpamKeywordsProviderFn(SpamKeywordsProviderFn fn);

    /**
     * @brief Remove the spam keywords provider bridge (reverts to static list).
     */
    static void clearSpamKeywordsProviderFn();

    /**
     * @brief Clear all feedback entries
     */
    void clear();
    
    // ===== Graph Link Methods =====
    
    /**
     * @brief Create graph link between feedback and LoRA adapter
     * 
     * Creates a FEEDBACK_FOR edge from feedback to adapter.
     * This enables querying feedback by adapter and tracking adapter lineage.
     * 
     * @param feedback_id Feedback entry ID
     * @param adapter_id LoRA adapter ID
     * @param metadata Optional edge metadata
     * @return true if link created successfully
     */
    bool createAdapterLink(
        const std::string& feedback_id,
        const std::string& adapter_id,
        const nlohmann::json& metadata = nlohmann::json::object());
    
    /**
     * @brief Get feedback linked to a specific adapter
     * 
     * @param adapter_id LoRA adapter ID
     * @param options List options for filtering
     * @return Vector of feedback entries
     */
    std::vector<FeedbackEntry> getFeedbackForAdapter(
        const std::string& adapter_id,
        const ListOptions& options) const;
    std::vector<FeedbackEntry> getFeedbackForAdapter(
        const std::string& adapter_id) const;
    
    /**
     * @brief Get adapters linked to a specific feedback
     * 
     * @param feedback_id Feedback ID
     * @return Vector of adapter IDs
     */
    std::vector<std::string> getLinkedAdapters(const std::string& feedback_id) const;
    
    /**
     * @brief Check if feedback is linked to an adapter
     * 
     * @param feedback_id Feedback ID
     * @param adapter_id Adapter ID
     * @return true if linked
     */
    bool isLinkedToAdapter(
        const std::string& feedback_id,
        const std::string& adapter_id) const;

    /**
     * @brief Get the active spam-keyword list.
     *
     * Returns the injected provider output when one is configured and
     * produces a non-empty list; otherwise returns the built-in static list.
     *
     * @return Active spam-keyword list used by feedback validation.
     */
    static std::vector<std::string> getSpamKeywords();

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_; // nullptr = default CF
    std::shared_ptr<IFeedbackPlugin> validation_plugin_;

    static constexpr const char* KEY_PREFIX = "help_feedback:";
    static constexpr const char* GRAPH_EDGE_PREFIX = "feedback_graph_edge:";
    
    std::string makeKey(const std::string& id) const;
    std::string makeGraphEdgeKey(const std::string& feedback_id, 
                                  const std::string& adapter_id) const;
    std::string generateId() const;
    
    // Spam detection configuration (deprecated, use plugin instead)
    static bool isLikelySpam(const std::string& text);
    
    // Helper: Apply plugin validation if available
    ValidationStatus applyPluginValidation(FeedbackEntry& feedback);
};

} // namespace llm
} // namespace themis
