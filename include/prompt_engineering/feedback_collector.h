/*
 * ThemisDB | File: feedback_collector.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file feedback_collector.h
 * @brief Feedback collection system for prompt quality improvement
 * 
 * Collects and analyzes user feedback, system errors, and quality issues
 * to drive autonomous prompt optimization. Integrates with the self-improvement
 * orchestration system.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace rocksdb { class ColumnFamilyHandle; }

namespace themis {
class RocksDBWrapper;

// Forward declaration — full header included in .cpp only.
namespace distributed_knowledge { class CrossShardFeedbackSync; }

namespace prompt_engineering {

/**
 * @brief Types of feedback that can be recorded
 */
enum class FeedbackType {
    USER_POSITIVE,          ///< User explicitly marked as good/helpful
    USER_NEGATIVE,          ///< User explicitly marked as bad/unhelpful
    HALLUCINATION_DETECTED, ///< System detected potential hallucination
    TIMEOUT,                ///< Query execution timed out
    PARSE_ERROR,            ///< Failed to parse LLM response
    VALIDATION_FAILED,      ///< Response failed validation checks
    CONTEXT_MISSING,        ///< Required context was missing
    AMBIGUOUS_OUTPUT,       ///< Output was ambiguous or unclear
    SECURITY_ISSUE,         ///< Security-related concern detected
    PERFORMANCE_ISSUE       ///< Performance degradation observed
};

/**
 * @brief Convert FeedbackType to string
 */
std::string feedbackTypeToString(FeedbackType type);

/**
 * @brief Convert string to FeedbackType
 */
std::optional<FeedbackType> stringToFeedbackType(const std::string& str);

/**
 * @brief A single feedback entry
 */
struct FeedbackEntry {
    std::string id;                      ///< Unique feedback ID
    std::string prompt_id;               ///< Associated prompt template ID
    FeedbackType type;                   ///< Type of feedback
    std::string query;                   ///< Original query/input
    std::string response;                ///< LLM response
    std::string feedback_text;           ///< Optional feedback message
    nlohmann::json metadata;             ///< Additional context
    double severity = 0.5;               ///< Severity score (0.0-1.0)
    std::chrono::system_clock::time_point timestamp;  ///< When recorded
    std::string checksum;                ///< FNV-1a 64-bit audit checksum of key fields
    
    /**
     * @brief Convert entry to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Parse entry from JSON
     */
    static FeedbackEntry fromJson(const nlohmann::json& j);

    /**
     * @brief Compute a simple audit checksum over key fields
     */
    std::string computeChecksum() const;
};

/**
 * @brief Aggregated feedback statistics
 */
struct FeedbackStats {
    std::string prompt_id;                          ///< Prompt template ID
    size_t total_feedback = 0;                      ///< Total feedback entries
    std::unordered_map<FeedbackType, size_t> counts_by_type;  ///< Count per type
    double positive_ratio = 0.0;                    ///< Positive feedback ratio
    double negative_ratio = 0.0;                    ///< Negative feedback ratio
    size_t hallucination_count = 0;                 ///< Number of hallucinations
    size_t error_count = 0;                         ///< Number of errors
    std::vector<std::string> common_issues;         ///< Most common issues
    std::chrono::system_clock::time_point last_feedback;  ///< Most recent feedback
    
    /**
     * @brief Convert stats to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Failed query pattern
 */
struct FailedQueryPattern {
    std::string pattern;                 ///< Pattern description
    size_t occurrences = 0;              ///< Number of times seen
    std::vector<std::string> examples;   ///< Example queries
    FeedbackType primary_type;           ///< Most common feedback type
    double avg_severity = 0.0;           ///< Average severity
};

/**
 * @brief Feedback collector
 * 
 * Collects and analyzes feedback about prompt quality:
 * - User feedback (positive/negative)
 * - System-detected issues (hallucinations, errors)
 * - Performance problems
 * - Failed query analysis
 * 
 * Thread-safe for concurrent feedback recording.
 */
class FeedbackCollector {
public:
    /**
     * @brief Constructor for in-memory collection
     */
    FeedbackCollector();
    
    /**
     * @brief Constructor with RocksDB persistence
     * @param db RocksDB wrapper (not owned)
     * @param cf Column family handle (optional, uses default if null)
     */
    FeedbackCollector(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    /**
     * @brief Record feedback about a prompt execution
     * @param prompt_id Prompt template ID
     * @param query Original query/input
     * @param response LLM response
     * @param type Type of feedback
     * @param feedback_text Optional feedback message
     * @param severity Severity score (0.0-1.0, default 0.5)
     * @param metadata Additional context
     * @return Feedback entry ID
     */
    std::string recordFeedback(
        const std::string& prompt_id,
        const std::string& query,
        const std::string& response,
        FeedbackType type,
        const std::string& feedback_text = "",
        double severity = 0.5,
        const nlohmann::json& metadata = {}
    );
    
    /**
     * @brief Get feedback entries for a specific prompt
     * @param prompt_id Prompt template ID
     * @param limit Maximum number of entries to return (0 = all)
     * @param type_filter Optional filter by feedback type
     * @return Vector of feedback entries
     */
    std::vector<FeedbackEntry> getFeedback(
        const std::string& prompt_id,
        size_t limit = 0,
        std::optional<FeedbackType> type_filter = std::nullopt
    ) const;
    
    /**
     * @brief Get aggregated feedback statistics for a prompt
     * @param prompt_id Prompt template ID
     * @return Feedback statistics
     */
    FeedbackStats getStats(const std::string& prompt_id) const;
    
    /**
     * @brief Get all prompts with negative feedback above threshold
     * @param threshold Minimum negative feedback ratio (default 0.3 = 30%)
     * @param min_feedback Minimum feedback count required (default 10)
     * @return Vector of prompt IDs
     */
    std::vector<std::string> getPromptsWithNegativeFeedback(
        double threshold = 0.3,
        size_t min_feedback = 10
    ) const;
    
    /**
     * @brief Get failed queries for analysis
     * @param prompt_id Prompt template ID
     * @param limit Maximum number of queries to return (default 100)
     * @param type_filter Optional filter by feedback type
     * @return Vector of (query, response, feedback_type) tuples
     */
    std::vector<std::tuple<std::string, std::string, FeedbackType>> 
    getFailedQueries(
        const std::string& prompt_id,
        size_t limit = 100,
        std::optional<FeedbackType> type_filter = std::nullopt
    ) const;
    
    /**
     * @brief Analyze failed query patterns
     * @param prompt_id Prompt template ID
     * @param min_occurrences Minimum occurrences to be considered a pattern (default 3)
     * @return Vector of identified patterns
     */
    std::vector<FailedQueryPattern> analyzeFailurePatterns(
        const std::string& prompt_id,
        size_t min_occurrences = 3
    ) const;
    
    /**
     * @brief Get feedback within a time range
     * @param prompt_id Prompt template ID
     * @param start Start time
     * @param end End time
     * @return Vector of feedback entries
     */
    std::vector<FeedbackEntry> getFeedbackInTimeRange(
        const std::string& prompt_id,
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end
    ) const;
    
    /**
     * @brief Delete old feedback entries
     * @param older_than Delete entries older than this
     * @return Number of entries deleted
     */
    size_t pruneOldFeedback(const std::chrono::system_clock::time_point& older_than);
    
    /**
     * @brief Clear all feedback for a specific prompt
     * @param prompt_id Prompt template ID
     * @return Number of entries deleted
     */
    size_t clearFeedback(const std::string& prompt_id);

    /**
     * @brief Return the total number of feedback entries recorded since last clear.
     *
     * Used by Loop 4 (RLAIF) in ContinuousLearningOrchestrator to decide
     * whether sufficient preference-pair data has accumulated to trigger a
     * training round (threshold: >= 100 entries).
     *
     * @return Total number of entries across all prompt IDs.
     */
    [[nodiscard]] size_t newEntryCount() const;
    
    /**
     * @brief Get feedback entries for a specific prompt (paginated)
     * 
     * Supports efficient chunked access to large feedback archives.
     * 
     * @param prompt_id Prompt template ID
     * @param offset Number of entries to skip (0-based)
     * @param page_size Maximum entries per page (0 = all remaining)
     * @param type_filter Optional filter by feedback type
     * @return Vector of feedback entries for the requested page
     */
    std::vector<FeedbackEntry> getFeedbackPaged(
        const std::string& prompt_id,
        size_t offset,
        size_t page_size,
        std::optional<FeedbackType> type_filter = std::nullopt
    ) const;

    /**
     * @brief Detect outlier feedback entries by severity
     * 
     * Returns entries whose severity deviates more than @p z_threshold
     * standard deviations from the mean.
     * 
     * @param prompt_id Prompt template ID
     * @param z_threshold Z-score threshold (default 2.0)
     * @return Vector of outlier entries
     */
    std::vector<FeedbackEntry> detectOutliers(
        const std::string& prompt_id,
        double z_threshold = 2.0
    ) const;

    /**
     * @brief Get summary across all prompts
     * @return JSON object with aggregate statistics
     */
    nlohmann::json getSummary() const;

    // ── DK-5: Cross-shard feedback propagation ────────────────────────────────

    /**
     * @brief Minimal embedding model interface for cross-shard feedback.
     *
     * Implement this (or inject a lambda via `setEmbeddingModel`) to provide
     * the 384-dimensional float embedding that is broadcast — in place of the
     * raw query text — when cross-shard sync is enabled.
     */
    struct IEmbeddingModel {
        [[nodiscard]] virtual std::vector<float> embed(const std::string& text) const = 0;
        virtual ~IEmbeddingModel() = default;
    };

    /**
     * @brief Inject a `CrossShardFeedbackSync` hook (DK-5 DI-setter).
     *
     * When set together with an embedding model, `recordFeedback()` will
     * publish an anonymised `FeedbackSummary` (embedding only, no raw text)
     * to all peer shards via the sync component.
     */
    void setCrossShardSync(
        std::shared_ptr<distributed_knowledge::CrossShardFeedbackSync> sync);

    /**
     * @brief Inject an embedding model for cross-shard summary generation.
     *
     * Required alongside `setCrossShardSync()` for the cross-shard publish
     * path.  When absent, cross-shard publish is silently skipped and a
     * warning is logged.
     */
    void setEmbeddingModel(std::shared_ptr<IEmbeddingModel> model);

private:
    mutable std::mutex mutex_;
    
    // In-memory storage: prompt_id -> feedback entries
    std::unordered_map<std::string, std::vector<FeedbackEntry>> feedback_;
    
    // Optional persistence
    RocksDBWrapper* db_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_ = nullptr;

    // DK-5: Cross-shard feedback propagation (both must be set to enable)
    std::shared_ptr<distributed_knowledge::CrossShardFeedbackSync> cross_shard_sync_;
    std::shared_ptr<IEmbeddingModel>                               embedding_model_;
    
    static constexpr const char* KEY_PREFIX = "feedback:";
    /// Secondary time-based index prefix: "idx:time:{prompt_id}:{ts_us}:{id}" → entry_key
    static constexpr const char* IDX_TIME_PREFIX = "idx:time:";
    
    /**
     * @brief Generate unique feedback ID
     */
    std::string generateId() const;
    
    /**
     * @brief Persist feedback to RocksDB (primary key + time-based secondary index)
     */
    void persist(const FeedbackEntry& entry);

    /**
     * @brief Remove primary record and its secondary index entry from RocksDB
     */
    void deleteFromDB(const FeedbackEntry& entry);
    
    /**
     * @brief Load feedback from RocksDB
     */
    void loadFromDB();
    
    /**
     * @brief Build a zero-padded microsecond timestamp string for use in index keys
     *
     * The zero-padding ensures lexicographic order equals chronological order.
     */
    static std::string formatTimestampKey(
        const std::chrono::system_clock::time_point& tp);
    
    /**
     * @brief Calculate feedback statistics
     */
    FeedbackStats calculateStats(const std::vector<FeedbackEntry>& entries) const;
    
    /**
     * @brief Extract query patterns using simple text analysis
     */
    std::vector<FailedQueryPattern> extractPatterns(
        const std::vector<FeedbackEntry>& entries,
        size_t min_occurrences
    ) const;
};

} // namespace prompt_engineering
} // namespace themis
