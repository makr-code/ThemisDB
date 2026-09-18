/**
 * @file process_pattern_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "analytics/process_mining.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include <vector>
#include <map>
#include <string>
#include <optional>
#include <mutex>
#include <set>
#include <algorithm>

namespace themis {


// ============================================================================
// Enumerations & Types
// ============================================================================

enum class SimilarityMethod {
    GRAPH,          ///< Graph-based similarity (structure)
    VECTOR,         ///< Vector-based similarity (semantics)
    BEHAVIORAL,     ///< Behavioral similarity (execution)
    HYBRID          ///< Weighted combination of all methods
};

struct ProcessPattern {
    std::string id;                                         ///< Pattern identifier
    std::string name;                                       ///< Human-readable name
    std::vector<std::string> activities;                    ///< Activity names
    std::vector<std::pair<std::string, std::string>> edges; ///< Control flow edges
    
    // Optional: Pre-computed embeddings
    std::optional<std::vector<float>> pattern_embedding;    ///< Embedding of entire pattern
    std::map<std::string, std::vector<float>> activity_embeddings; ///< Per-activity embeddings
    
    // Optional: Behavioral constraints
    struct BehavioralConstraint {
        std::string type;  ///< "sequence", "choice", "parallel", "loop"
        std::vector<std::string> involved_activities;
    };
    std::vector<BehavioralConstraint> constraints;
    
    // Optional: Tolerance settings
    double structural_tolerance = 0.2;  ///< Allow 20% structural differences
    double semantic_tolerance = 0.1;    ///< Allow 10% semantic differences
};

struct SimilarityResult {
    std::string case_id;                    ///< Process instance ID
    std::string process_name;               ///< Process name (if available)
    double overall_similarity;              ///< Combined similarity score (0-1)
    
    // Breakdown by method
    struct MetricBreakdown {
        double graph_similarity = 0;            ///< Structural similarity
        double vector_similarity;           ///< Semantic similarity
        double behavioral_similarity;       ///< Behavioral similarity
        
        // Detailed metrics
        double node_overlap;                ///< Jaccard similarity of nodes
        double edge_overlap;                ///< Jaccard similarity of edges
        double path_similarity;             ///< Longest common subsequence
        double edit_distance;               ///< Graph edit distance (normalized)
    } metrics;
    
    // Matched elements
    std::vector<std::string> matched_activities;            ///< Activities that matched
    std::vector<std::pair<std::string, std::string>> matched_edges; ///< Edges that matched
    std::vector<std::string> extra_activities;              ///< Activities in result but not in pattern
    std::vector<std::string> missing_activities;            ///< Activities in pattern but not in result
    
    // Performance data
    int64_t computation_time_us;            ///< Time taken to compute similarity
};

struct PatternMatchConfig {
    SimilarityMethod method = SimilarityMethod::HYBRID;
    
    // Weights for hybrid method (sum should be 1.0)
    double graph_weight = 0.4;
    double vector_weight = 0.3;
    double behavioral_weight = 0.3;
    
    // Thresholds
    double min_similarity = 0.0;            ///< Minimum similarity to include in results
    int max_results = 100;                  ///< Maximum number of results
    
    // Performance tuning
    bool use_index = true;                  ///< Use HNSW index for vector search
    bool use_cache = true;                  ///< Cache frequent patterns
    bool parallel = true;                   ///< Parallelize computation
    
    // Tolerance settings (override pattern defaults if set)
    std::optional<double> structural_tolerance;
    std::optional<double> semantic_tolerance;
};

// ============================================================================
// ProcessPatternMatcher Class
// ============================================================================

class RocksDBWrapper;
class VectorIndex;
class GraphIndex;

class ProcessPatternMatcher {
public:
    struct Status {
        bool is_ok = true;
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
        bool ok() const { return is_ok; }
    };
    
    explicit ProcessPatternMatcher(
        RocksDBWrapper& db,
        VectorIndex* vector_index = nullptr,
        GraphIndex* graph_index = nullptr
    );
    
    // ===== Pattern Matching =====
    
    std::pair<Status, std::vector<SimilarityResult>> findSimilar(
        const ProcessPattern& pattern,
        const PatternMatchConfig& config
    );
    
    std::pair<Status, ProcessMining::ConformanceResult> compareWithIdeal(
        const std::string& case_id,
        const ProcessPattern& ideal_pattern
    );
    
    std::pair<Status, bool> hasPattern(
        const std::string& case_id,
        const ProcessPattern& pattern,
        double threshold = 0.8
    );

    /**
     * @brief Match Activity Pattern.
     * @param[in] log Input parameter.
     * @param[in] pattern Input parameter.
     * @param[in,out] out_matching_trace_indices Input/output parameter.
     * @return Return value.
     */
    Status matchActivityPattern(
        const EventLog& log,
        const std::vector<std::string>& pattern,
        std::vector<int>& out_matching_trace_indices
    );
    
    // ===== Batch Operations =====
    
    std::pair<Status, std::map<std::string, SimilarityResult>> findPatternsInBatch(
        const std::vector<std::string>& case_ids,
        const ProcessPattern& pattern,
        const PatternMatchConfig& config
    );
    
    // ===== Pattern Library =====
    
    std::pair<Status, std::map<std::string, ProcessPattern>> loadAdministrativeModels();
    
    std::pair<Status, ProcessPattern> getAdministrativeModel(const std::string& model_id);
    
    // ===== Statistics & Analysis =====
    
    struct PatternStatistics {
        int total_patterns_cached = 0;
        int total_comparisons_performed = 0;
        double avg_computation_time_ms = 0.0;
        std::map<std::string, int> pattern_frequency;  ///< pattern_id -> usage count
        std::map<std::string, double> avg_similarity;  ///< pattern_id -> average score
    };
    std::pair<Status, PatternStatistics> getStatistics() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();

private:
    RocksDBWrapper& db_;
    VectorIndex* vector_index_;
    GraphIndex* graph_index_;
    mutable ProcessMining process_mining_;
    
    // Cache for frequent patterns
    mutable std::map<std::string, std::vector<SimilarityResult>> pattern_cache_;
    mutable std::map<std::string, ProcessPattern> model_cache_;
    
    // Statistics
    mutable PatternStatistics statistics_;
    
    
    /**
     * @brief Compute Graph Similarity.
     * @param[in] pattern Input parameter.
     * @param[in] log Input parameter.
     * @param[in] case_id Identifier of the case.
     * @return Return value.
     */
    double computeGraphSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute Vector Similarity.
     * @param[in] pattern Input parameter.
     * @param[in] log Input parameter.
     * @param[in] case_id Identifier of the case.
     * @return Return value.
     */
    double computeVectorSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute Behavioral Similarity.
     * @param[in] pattern Input parameter.
     * @param[in] log Input parameter.
     * @param[in] case_id Identifier of the case.
     * @return Return value.
     */
    double computeBehavioralSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute Hybrid Similarity.
     * @param[in] pattern Input parameter.
     * @param[in] log Input parameter.
     * @param[in] case_id Identifier of the case.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    double computeHybridSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id,
        const PatternMatchConfig& config
    ) const;
    
    // ===== Helper Functions =====
    
    std::pair<Status, ProcessTrace> getTrace(const std::string& case_id) const;
    
    template<typename T>
    double jaccardSimilarity(const std::set<T>& a, const std::set<T>& b) const {
        if (a.empty() && b.empty()) {
          return 1.0;
        }
        
        std::set<T> intersection;
        std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                            std::inserter(intersection, intersection.begin()));
        
        std::set<T> union_set;
        std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                      std::inserter(union_set, union_set.begin()));
        
        return union_set.empty() ? 0.0 : 
               static_cast<double>(intersection.size()) / union_set.size();
    }
    
    /**
     * @brief Longest Common Subsequence.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    int longestCommonSubsequence(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b
    ) const;
    
    /**
     * @brief Embed Activities.
     * @param[in] activities Input parameter.
     * @return Return value.
     */
    std::vector<float> embedActivities(const std::vector<std::string>& activities) const;
    
    /**
     * @brief Cosine Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    double cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
};

} // namespace themis
