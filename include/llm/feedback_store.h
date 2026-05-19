/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_store.h                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     308                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <chrono>
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
        int64_t timestamp_ms;                  // Creation timestamp
        ValidationStatus validation_status;    // Validation state
        std::string model_version;             // Model version that generated the answer
        std::string adapter_id;                // LoRA adapter ID (if used)
        std::string adapter_version;           // LoRA adapter version
        bool used_for_training;                // Whether used in training
        int training_batch_id;                 // Training batch ID (0 = not trained)
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
        size_t total_feedback;
        size_t positive_count;
        size_t negative_count;
        size_t pending_validation;
        size_t approved_count;
        size_t rejected_count;
        size_t unused_for_training;
        size_t used_for_training;
        double positive_ratio;
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
    static const std::vector<std::string>& getSpamKeywords();
    static bool isLikelySpam(const std::string& text);
    
    /**
     * @brief Apply plugin validation and, for MODIFY decisions, rewrite the
     *        entry's comment and metadata in-place before persisting.
     *
     * @param feedback  Feedback entry to validate; modified in-place when the
     *                  plugin returns MODIFY with non-empty field overrides.
     * @return Resolved ValidationStatus (APPROVED after a MODIFY).
     */
    ValidationStatus applyPluginValidation(FeedbackEntry& feedback);
};

} // namespace llm
} // namespace themis
