/**
 * @file adaptive_index.h
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
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "storage/rocksdb_wrapper.h"

namespace themis {

class QueryPatternTracker {
public:
    struct QueryPattern {
        std::string collection;
        std::string field;
        std::string operation;  // "eq", "range", "in", "join"
        int64_t count = 0;
        int64_t total_time_ms = 0;
        int64_t last_seen_ms = 0;
        
        // Phase 2: Cache-aware metrics
        int64_t cache_misses = 0;           // L3 cache misses for this pattern
        int64_t cache_hits = 0;             // L3 cache hits for this pattern
        double avg_cache_miss_penalty_ms = 0.0;  // Average penalty from cache miss
        
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
        static QueryPattern fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Query Pattern Tracker.
     * @return Return value.
     */
    explicit QueryPatternTracker();
    ~QueryPatternTracker() = default;
    
    void recordPattern(const std::string& collection,
                      const std::string& field,
                      const std::string& operation,
                      int64_t execution_time_ms,
                      bool cache_miss = false,
                      double cache_miss_penalty_ms = 0.0);
    
    std::vector<QueryPattern> getPatterns(const std::string& collection = "") const;
    
    std::vector<QueryPattern> getTopPatterns(size_t limit = 10) const;
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

private:
    mutable std::mutex mutex_;
    // Key: "collection:field:operation"
    std::map<std::string, QueryPattern> patterns_;
    
    /**
     * @brief Make Key.
     * @param[in] collection Input parameter.
     * @param[in] field Input parameter.
     * @param[in] operation Input parameter.
     * @return Return value.
     */
    std::string makeKey(const std::string& collection,
                       const std::string& field,
                       const std::string& operation) const;
    
    /**
     * @brief Get Current Time Ms.
     * @return Return value.
     */
    int64_t getCurrentTimeMs() const;
};

class SelectivityAnalyzer {
public:
    struct SelectivityStats {
        std::string collection;
        std::string field;
        int64_t total_documents = 0;
        int64_t unique_values = 0;
        int64_t null_count = 0;
        double selectivity = 0.0;  // unique_values / total_documents
        std::string distribution;   // "uniform", "skewed", "sparse"
        
        // Phase 2: Cache-aware metrics
        double estimated_l3_cache_fit_ratio = 0.0;  // % of index that fits in L3 (20MB)
        double estimated_cache_miss_rate = 0.0;      // Estimated cache miss rate
        
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
        static SelectivityStats fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Selectivity Analyzer.
     * @param[in,out] db Input/output parameter.
     * @return Return value.
     */
    explicit SelectivityAnalyzer(rocksdb::TransactionDB* db);
    ~SelectivityAnalyzer() = default;
    
    SelectivityStats analyze(const std::string& collection,
                            const std::string& field,
                            size_t sample_size = 1000);
    
    /**
     * @brief Calculate Index Benefit.
     * @param[in] stats Input parameter.
     * @return Return value.
     */
    double calculateIndexBenefit(const SelectivityStats& stats) const;
    
    SelectivityStats analyzeCacheAware(const SelectivityStats& stats,
                                       size_t l3_cache_size_mb = 20) const;

private:
    rocksdb::TransactionDB* db_;
    
    std::string determineDistribution(const std::map<std::string, int>& value_counts,
                                     int64_t total) const;
};

class IndexSuggestionEngine {
public:
    struct IndexSuggestion {
        std::string collection;
        std::string field;
        std::string index_type;  // "range", "hash", "composite"
        double score = 0.0;      // 0.0 - 1.0 (higher = more beneficial)
        std::string reason;
        nlohmann::json metadata;
        
        // Estimated impact
        int64_t queries_affected = 0;
        int64_t estimated_speedup_ms = 0;
        
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
        static IndexSuggestion fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Index Suggestion Engine.
     * @param[in,out] tracker Input/output parameter.
     * @param[in,out] analyzer Input/output parameter.
     * @return Return value.
     */
    explicit IndexSuggestionEngine(QueryPatternTracker* tracker,
                                  SelectivityAnalyzer* analyzer);
    ~IndexSuggestionEngine() = default;
    
    std::vector<IndexSuggestion> generateSuggestions(
        const std::string& collection = "",
        double min_score = 0.5,
        size_t limit = 10);
    
    std::vector<IndexSuggestion> generateCacheAwareIndexes(
        const std::string& collection = "",
        float target_cache_hit_rate = 0.70f,
        double min_score = 0.5,
        size_t limit = 10);
    
    /**
     * @brief Index Exists.
     * @param[in] collection Input parameter.
     * @param[in] field Input parameter.
     * @return True when the operation succeeds.
     */
    bool indexExists(const std::string& collection,
                    const std::string& field) const;

    /**
     * @brief Register Index.
     * @param[in] collection Input parameter.
     * @param[in] field Input parameter.
     */
    void registerIndex(const std::string& collection, const std::string& field);

    /**
     * @brief Unregister Index.
     * @param[in] collection Input parameter.
     * @param[in] field Input parameter.
     */
    void unregisterIndex(const std::string& collection, const std::string& field);

private:
    QueryPatternTracker* tracker_;
    SelectivityAnalyzer* analyzer_;
    mutable std::mutex analyzerMutex_;

    // In-memory registry of indexes that already exist.
    // Key format: "<collection>:<field>"
    mutable std::shared_mutex existingIndexesMutex_;
    std::unordered_set<std::string> existingIndexes_;
    
    /**
     * @brief Calculate Score.
     * @param[in] pattern Input parameter.
     * @param[in] stats Input parameter.
     * @return Return value.
     */
    double calculateScore(const QueryPatternTracker::QueryPattern& pattern,
                         const SelectivityAnalyzer::SelectivityStats& stats) const;
    
    /**
     * @brief Recommend Index Type.
     * @param[in] pattern Input parameter.
     * @param[in] stats Input parameter.
     * @return Return value.
     */
    std::string recommendIndexType(const QueryPatternTracker::QueryPattern& pattern,
                                   const SelectivityAnalyzer::SelectivityStats& stats) const;
    
    /**
     * @brief Generate Reason.
     * @param[in] pattern Input parameter.
     * @param[in] stats Input parameter.
     * @param[in] index_type Input parameter.
     * @return Return value.
     */
    std::string generateReason(const QueryPatternTracker::QueryPattern& pattern,
                              const SelectivityAnalyzer::SelectivityStats& stats,
                              const std::string& index_type) const;
};

class AdaptiveIndexManager {
public:
    /**
     * @brief Adaptive Index Manager.
     * @param[in,out] db Input/output parameter.
     * @return Return value.
     */
    explicit AdaptiveIndexManager(rocksdb::TransactionDB* db);
    ~AdaptiveIndexManager() = default;
    
    // Component access
    /**
     * @brief Get Pattern Tracker.
     * @return Pointer to the result.
     * @details Implements getPatternTracker without additional internal calls.
     */
    QueryPatternTracker* getPatternTracker() { return &tracker_; }
    /**
     * @brief Get Selectivity Analyzer.
     * @return Pointer to the result.
     * @details Implements getSelectivityAnalyzer without additional internal calls.
     */
    SelectivityAnalyzer* getSelectivityAnalyzer() { return &analyzer_; }
    /**
     * @brief Get Suggestion Engine.
     * @return Pointer to the result.
     * @details Implements getSuggestionEngine without additional internal calls.
     */
    IndexSuggestionEngine* getSuggestionEngine() { return &engine_; }
    
    std::vector<IndexSuggestionEngine::IndexSuggestion> getSuggestions(
        const std::string& collection = "",
        double min_score = 0.5,
        size_t limit = 10);
    
    std::vector<QueryPatternTracker::QueryPattern> getPatterns(
        const std::string& collection = "");

private:
    rocksdb::TransactionDB* db_;
    QueryPatternTracker tracker_;
    SelectivityAnalyzer analyzer_;
    IndexSuggestionEngine engine_;
};

} // namespace themis
