/**
 * @file process_pattern_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Process Pattern Matcher - Find Similar Processes
 * 
 * This component enables similarity-based process discovery:
 * - Compare process instances with ideal models
 * - Find processes matching specific patterns
 * - Support graph, vector, and behavioral similarity
 * 
 * ## Use Cases
 * 
 * ### 1. Find Processes Similar to Ideal Model
 * ```cpp
 * ProcessPatternMatcher matcher(db);
 * 
 * Pattern ideal = {
 *     .activities = {"Antrag", "Prüfung", "Genehmigung"},
 *     .edges = {{"Antrag", "Prüfung"}, {"Prüfung", "Genehmigung"}}
 * };
 * 
 * auto results = matcher.findSimilar(ideal, 0.7, SimilarityMethod::HYBRID, 10);
 * ```
 * 
 * ### 2. Compare Process with Ideal
 * ```cpp
 * auto comparison = matcher.compareWithIdeal("V-2024-0001", ideal_model);
 * std::cout << "Fitness: " << comparison.fitness << std::endl;
 * std::cout << "Deviations: " << comparison.deviations.size() << std::endl;
 * ```
 * 
 * ## Similarity Methods
 * 
 * - **GRAPH**: Structure-based (nodes, edges, paths)
 * - **VECTOR**: Semantic similarity using embeddings
 * - **BEHAVIORAL**: Execution behavior and ordering
 * - **HYBRID**: Weighted combination of all methods
 * 
 * ## Integration
 * 
 * - Uses existing VectorIndex for semantic search
 * - Uses GraphIndex for structural analysis
 * - Extends ProcessMining with pattern matching
 */

// ============================================================================
// Enumerations & Types
// ============================================================================

/**
 * @brief Similarity computation methods
 */
enum class SimilarityMethod {
    GRAPH,          ///< Graph-based similarity (structure)
    VECTOR,         ///< Vector-based similarity (semantics)
    BEHAVIORAL,     ///< Behavioral similarity (execution)
    HYBRID          ///< Weighted combination of all methods
};

/**
 * @brief Process pattern definition
 * 
 * A pattern can be specified at various levels:
 * - Activities only: just the sequence of activity names
 * - Structure: activities + edges defining the control flow
 * - Embedding: pre-computed vector representation
 */
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

/**
 * @brief Result of similarity search
 */
struct SimilarityResult {
    std::string case_id;                    ///< Process instance ID
    std::string process_name;               ///< Process name (if available)
    double overall_similarity;              ///< Combined similarity score (0-1)
    
    // Breakdown by method
    struct MetricBreakdown {
        double graph_similarity;            ///< Structural similarity
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

/**
 * @brief Configuration for pattern matching
 */
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

/**
 * @brief Main class for process pattern matching
 */
class ProcessPatternMatcher {
public:
    struct Status {
        bool is_ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
        bool ok() const { return is_ok; }
    };
    
    /**
     * @brief Constructor
     * @param db Database reference
     * @param vector_index Optional vector index for semantic search
     * @param graph_index Optional graph index for structural queries
     */
    explicit ProcessPatternMatcher(
        RocksDBWrapper& db,
        VectorIndex* vector_index = nullptr,
        GraphIndex* graph_index = nullptr
    );
    
    // ===== Pattern Matching =====
    
    /**
     * @brief Find processes similar to a given pattern
     * 
     * @param pattern The ideal pattern to match against
     * @param config Configuration for matching
     * @return Vector of similar processes ranked by similarity
     * 
     * @code
     * ProcessPattern ideal = {
     *     .activities = {"Antrag", "Prüfung", "Genehmigung"},
     *     .edges = {{"Antrag", "Prüfung"}, {"Prüfung", "Genehmigung"}}
     * };
     * 
     * PatternMatchConfig config;
     * config.method = SimilarityMethod::HYBRID;
     * config.min_similarity = 0.7;
     * 
     * auto results = matcher.findSimilar(ideal, config);
     * @endcode
     */
    std::pair<Status, std::vector<SimilarityResult>> findSimilar(
        const ProcessPattern& pattern,
        const PatternMatchConfig& config
    );
    
    /**
     * @brief Compare a specific process instance with ideal model
     * 
     * @param case_id Process instance to compare
     * @param ideal_pattern Expected/ideal process pattern
     * @return Detailed comparison including deviations
     */
    std::pair<Status, ProcessMining::ConformanceResult> compareWithIdeal(
        const std::string& case_id,
        const ProcessPattern& ideal_pattern
    );
    
    /**
     * @brief Check if a process matches a specific pattern
     * 
     * @param case_id Process instance to check
     * @param pattern Pattern to match
     * @param threshold Minimum similarity threshold (0-1)
     * @return True if similarity >= threshold
     */
    std::pair<Status, bool> hasPattern(
        const std::string& case_id,
        const ProcessPattern& pattern,
        double threshold = 0.8
    );
    
    // ===== Batch Operations =====
    
    /**
     * @brief Find patterns across multiple process instances
     * 
     * @param case_ids List of process instances to analyze
     * @param pattern Pattern to search for
     * @param config Matching configuration
     * @return Similarity results for each case
     */
    std::pair<Status, std::map<std::string, SimilarityResult>> findPatternsInBatch(
        const std::vector<std::string>& case_ids,
        const ProcessPattern& pattern,
        const PatternMatchConfig& config
    );
    
    // ===== Pattern Library =====
    
    /**
     * @brief Load administrative process models from config
     * 
     * Loads predefined patterns from:
     * - config/process_models/administrative_process_models.yaml
     * 
     * Models include:
     * - Bauantragsverfahren (Building Permits)
     * - Beschaffungsprozesse (Procurement)
     * - Personalverwaltung (HR)
     * - Haushaltsplanung (Budget Planning)
     * 
     * @return Map of model_id -> ProcessPattern
     */
    std::pair<Status, std::map<std::string, ProcessPattern>> loadAdministrativeModels();
    
    /**
     * @brief Get a specific administrative model by ID
     * 
     * @param model_id Model identifier (e.g., "bauantrag_standard")
     * @return ProcessPattern or error if not found
     */
    std::pair<Status, ProcessPattern> getAdministrativeModel(const std::string& model_id);
    
    // ===== Statistics & Analysis =====
    
    /**
     * @brief Get statistics about pattern usage
     * 
     * @return Statistics including:
     * - Number of cached patterns
     * - Most frequent patterns
     * - Average similarity scores
     */
    struct PatternStatistics {
        int total_patterns_cached = 0;
        int total_comparisons_performed = 0;
        double avg_computation_time_ms = 0.0;
        std::map<std::string, int> pattern_frequency;  ///< pattern_id -> usage count
        std::map<std::string, double> avg_similarity;  ///< pattern_id -> average score
    };
    std::pair<Status, PatternStatistics> getStatistics() const;
    
    /**
     * @brief Clear pattern cache
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
    
    // ===== Similarity Computation Methods =====
    
    /**
     * @brief Compute graph-based structural similarity
     * 
     * Uses:
     * - Node overlap (Jaccard similarity)
     * - Edge overlap (Jaccard similarity)
     * - Path-based similarity (LCS)
     * - Graph edit distance (approximation)
     */
    double computeGraphSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute vector-based semantic similarity
     * 
     * Uses:
     * - Activity embeddings
     * - Trace2Vec (aggregated activity embeddings)
     * - Cosine similarity
     */
    double computeVectorSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute behavioral similarity
     * 
     * Uses:
     * - Sequence patterns (longest common subsequence)
     * - Weak order relations
     * - Execution frequencies
     */
    double computeBehavioralSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id
    ) const;
    
    /**
     * @brief Compute hybrid similarity (weighted combination)
     */
    double computeHybridSimilarity(
        const ProcessPattern& pattern,
        const EventLog& log,
        const std::string& case_id,
        const PatternMatchConfig& config
    ) const;
    
    // ===== Helper Functions =====
    
    /**
     * @brief Extract process trace for a case
     */
    std::pair<Status, ProcessTrace> getTrace(const std::string& case_id) const;
    
    /**
     * @brief Compute Jaccard similarity between two sets
     */
    template<typename T>
    double jaccardSimilarity(const std::set<T>& a, const std::set<T>& b) const {
        if (a.empty() && b.empty()) return 1.0;
        
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
     * @brief Compute longest common subsequence length
     */
    int longestCommonSubsequence(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b
    ) const;
    
    /**
     * @brief Embed activity sequence using VectorIndex
     */
    std::vector<float> embedActivities(const std::vector<std::string>& activities) const;
    
    /**
     * @brief Compute cosine similarity between two vectors
     */
    double cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) const;
};

} // namespace themis
