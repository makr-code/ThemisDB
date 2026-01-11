#ifndef THEMIS_LLM_FEEDBACK_STORE_H
#define THEMIS_LLM_FEEDBACK_STORE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <chrono>
#include <nlohmann/json.hpp>

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {
namespace llm {

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
 * - Spam detection and validation
 * - Versioning and timestamping
 * - Integration with LLM interactions
 * - Query and filtering capabilities
 * 
 * Storage: RocksDB with JSON serialization
 * Key format: "help_feedback:{feedback_id}"
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
     * @brief List feedback entries with optional filters
     * @param options List options (pagination, filters)
     * @return Vector of feedback entries
     */
    std::vector<FeedbackEntry> listFeedback() const;
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
     * @param feedback Feedback entry to validate
     * @return ValidationStatus result
     */
    static ValidationStatus validateFeedback(const FeedbackEntry& feedback);

    /**
     * @brief Clear all feedback entries
     */
    void clear();

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_; // nullptr = default CF

    static constexpr const char* KEY_PREFIX = "help_feedback:";
    
    std::string makeKey(const std::string& id) const;
    std::string generateId() const;
    
    // Spam detection configuration
    static const std::vector<std::string>& getSpamKeywords();
    static bool isLikelySpam(const std::string& text);
};

} // namespace llm
} // namespace themis

#endif // THEMIS_LLM_FEEDBACK_STORE_H
