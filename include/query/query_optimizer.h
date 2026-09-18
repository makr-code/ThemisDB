/**
 * @file query_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <map>
#include <memory>
#include <mutex>

#include "query/query_engine.h"
#include "query/adaptive_optimizer.h"
#include "query/optimizer_cost_model.h"

// Forward-declare PerQueryCostModel to avoid a hard dependency;
// callers that want to use it must include the full header.
namespace themis { namespace performance { namespace phase3 { class PerQueryCostModel; } } }

// Forward-declare StatisticsCollector and MetricsCollector to avoid hard dependencies.
namespace themis { class StatisticsCollector; }
namespace themis { namespace observability { class MetricsCollector; } }

namespace themis {
namespace query {

using SecondaryIndexManager = ::themis::SecondaryIndexManager;

class QueryOptimizer {
public:
    struct Estimation {
        PredicateEq pred;
        size_t estimatedCount = 0; // bis maxProbe gezählt
        bool capped = false;       // true, wenn abgeschnitten (>= maxProbe)
    };

    struct Plan {
        std::vector<PredicateEq> orderedPredicates; // aufsteigend nach erwarteter Selektivität
        std::vector<Estimation> details;            // für Logging/Diagnose
        
        // NLP-based metadata (PR #317)
        double nlp_complexity = 0.0;                // Query complexity estimate (0.0-1.0)
        std::vector<std::string> nlp_suggested_indexes; // Suggested index types
        std::map<std::string, std::string> nlp_hints;   // Semantic optimization hints

        // Serialization + execution-path recommendation (SerializationStrategyAdvisor)
        OptimizerCostModel::SerializationAdvice serialization_advice;
        
        // ==================== SCOPE VALIDATION (Phase 2 Agent 2) ====================
        // Scope boundaries enforced at plan time to prevent result overflow
        struct ScopeBounds {
            std::string scope_id;                   // scope identifier (database/collection/tenant)
            size_t max_result_rows = 0;             // max rows this plan can return
            size_t max_result_bytes = 0;            // max bytes this plan can produce
            bool enforce_federation_isolation = false; // true if federated query isolation required
        };
        ScopeBounds scope_bounds;                   // scope limits for this plan (GAP-2)
        
        // Validates that scope bounds are set and consistent
        bool has_valid_scope_bounds() const noexcept {
            return !scope_bounds.scope_id.empty() && 
                   (scope_bounds.max_result_rows > 0 || scope_bounds.max_result_bytes > 0);
        }
    };

    QueryOptimizer(SecondaryIndexManager& secIdx,
                   StatisticsCollector* stats_collector = nullptr,
                   observability::MetricsCollector* metrics_collector = nullptr);

    // Schätzt Selektivitäten der Gleichheitsprädikate und liefert eine Ordnung (kleinste zuerst)
    Plan chooseOrderForAndQuery(const ConjunctiveQuery& q, size_t maxProbePerPred = 1000) const;

    // =============================
    // Serialization Advisor tuning
    // =============================

    /**
     * @brief Set Advisor Cost Constants.
     * @param[in] c Input parameter.
     */
    void setAdvisorCostConstants(const OptimizerCostModel::CostConstants& c);

    /**
     * @brief Advisor Cost Constants.
     * @return Return value.
     */
    OptimizerCostModel::CostConstants advisorCostConstants() const;
    
    // NLP-enhanced query optimization (PR #317 Phase 1)
    // Combines traditional cost-based optimization with NLP-based semantic analysis
    Plan chooseOrderForAndQueryWithNLP(const ConjunctiveQuery& q, 
                                       const std::string& original_query_text,
                                       size_t maxProbePerPred = 1000) const;

    // Führt die Anfrage mit der geplanten Reihenfolge aus (sequenziell)
    Result<std::vector<std::string>>
    executeOptimizedKeys(QueryEngine& engine, const ConjunctiveQuery& q, const Plan& plan) const;

    Result<std::vector<BaseEntity>>
    executeOptimizedEntities(QueryEngine& engine, const ConjunctiveQuery& q, const Plan& plan) const;

    Result<size_t>
    executeOptimizedCount(QueryEngine& engine, const ConjunctiveQuery& q, const Plan& plan) const;

    /**
     * @brief ============================= Per-Query Cost Model Integration (Phase 3, Issue #2419) =============================
     * @param[in] new_cost_model Input parameter.
     */

    void attachPerQueryCostModel(
        std::shared_ptr<performance::phase3::PerQueryCostModel> new_cost_model);

    /**
     * @brief Per Query Cost Model.
     * @return Return value.
     */
    std::shared_ptr<performance::phase3::PerQueryCostModel> perQueryCostModel() const;

    Result<std::vector<std::string>>
    executeOptimizedKeysWithCost(QueryEngine& engine,
                                  const ConjunctiveQuery& q,
                                  const Plan& plan,
                                  double estimated_cost = 0.0) const;

    Result<std::vector<BaseEntity>>
    executeOptimizedEntitiesWithCost(QueryEngine& engine,
                                      const ConjunctiveQuery& q,
                                      const Plan& plan,
                                      double estimated_cost = 0.0) const;

    // =============================
    // Hybrid Vector+Geo Cost Model
    // =============================
    struct VectorGeoCostInput {
        bool hasVectorIndex = false;
        bool hasSpatialIndex = false;
        double bboxRatio = 1.0;              // area(bbox)/area(total)
        size_t prefilterSize = 0;             // equality prefilter candidate universe
        size_t spatialIndexEntries = 0;       // number of spatial index entries (approx table size for spatial filter)
        size_t k = 10;                        // requested top-k
        size_t vectorDim = 0;                 // vector dimension (for scaling)
        size_t overfetch = 1;                 // overfetch multiplier
    };
    enum class VectorGeoPlan { SpatialThenVector, VectorThenSpatial };
    struct VectorGeoCostResult {
        VectorGeoPlan plan;
        double costSpatialFirst = 0.0;
        double costVectorFirst = 0.0;
    };
    /**
     * @brief Choose Vector Geo Plan.
     * @param[in] in Input parameter.
     * @return Return value.
     */
    static VectorGeoCostResult chooseVectorGeoPlan(const VectorGeoCostInput& in);

    // =============================
    // Content+Geo (Fulltext + Spatial) Cost Model (stub)
    // =============================
    struct ContentGeoCostInput {
        bool hasFulltextIndex = true;
        bool hasSpatialIndex = false;
        size_t fulltextHits = 0;          // estimated FT hit count
        double bboxRatio = 1.0;           // spatial selectivity
        size_t limit = 100;               // requested limit
    };
    struct ContentGeoCostResult {
        double costFulltextThenSpatial = 0.0;
        double costSpatialThenFulltext = 0.0; // for future when spatial prefilter can restrict FT search scope
        bool chooseFulltextFirst = false;       // current plan choice
    };
    /**
     * @brief Estimate Content Geo.
     * @param[in] in Input parameter.
     * @return Return value.
     */
    static ContentGeoCostResult estimateContentGeo(const ContentGeoCostInput& in);

    // =============================
    // Graph Shortest Path Cost Model (stub)
    // =============================
    struct GraphPathCostInput {
        size_t maxDepth = 5;
        size_t branchingFactor = 4;    // estimated average outgoing edges per vertex
        bool hasSpatialConstraint = false;
        double spatialSelectivity = 1.0; // fraction of vertices passing spatial filter
    };
    struct GraphPathCostResult {
        double estimatedExpandedVertices = 0.0;
        double estimatedTimeMs = 0.0; // abstract
    };
    /**
     * @brief Estimate Graph Path.
     * @param[in] in Input parameter.
     * @return Return value.
     */
    static GraphPathCostResult estimateGraphPath(const GraphPathCostInput& in);

    // =============================
    // Adaptive & Distributed Optimization (New)
    // =============================
    
    void enableAdaptiveOptimization(bool enable = true);
    
    bool isAdaptiveOptimizationEnabled() const { return adaptive_enabled_; }
    
    /**
     * @brief Record Query Execution.
     * @param[in] query_hash Input parameter.
     * @param[in] estimated_rows Input parameter.
     * @param[in] actual_rows Input parameter.
     * @param[in] execution_time_ms Input parameter.
     */
    void recordQueryExecution(
        const std::string& query_hash,
        size_t estimated_rows,
        size_t actual_rows,
        double execution_time_ms);
    
    /**
     * @brief Get Adaptive Adjustment.
     * @param[in] query_hash Input parameter.
     * @return Return value.
     */
    double getAdaptiveAdjustment(const std::string& query_hash) const;
    
    struct DistributedPlan {
        std::vector<std::string> shard_ids;
        bool use_partition_pruning = false;
        std::string join_strategy;  // "broadcast", "repartition", "semi_join"
        size_t recommended_parallelism = 1;
        bool enable_numa_awareness = false;
        std::vector<int> preferred_cpu_affinity;
    };
    
    DistributedPlan optimizeForDistribution(
        const ConjunctiveQuery& q,
        const std::vector<std::string>& available_shards,
        bool enable_partition_pruning = true) const;
    
    struct VectorWorkloadPlan {
        int recommended_ef_search = 0;
        size_t recommended_k_overfetch = 0;
        bool use_prefiltering = false;
        std::string index_type;  // "hnsw", "ivf", "flat"
    };
    
    VectorWorkloadPlan optimizeVectorWorkload(
        size_t k,
        size_t dataset_size,
        size_t dimension,
        double target_recall = 0.95) const;
    
    struct GraphWorkloadPlan {
        size_t max_expansion_depth = 0;
        bool use_bidirectional_search = false;
        bool enable_spatial_pruning = false;
        size_t recommended_parallelism = 0;
    };
    
    GraphWorkloadPlan optimizeGraphWorkload(
        size_t max_depth,
        size_t estimated_branching_factor,
        bool has_spatial_constraint = false) const;
    
    // =============================
    // Scope Validation (Phase 2 Agent 2)
    // =============================
    
    bool setScopeBounds(Plan& plan,
                       const std::string& scope_id,
                       size_t max_rows = 0,
                       size_t max_bytes = 0,
                       bool enforce_federation = false) const noexcept;
    
    /**
     * @brief Validate Result Bounds.
     * @param[in] plan Input parameter.
     * @param[in] result_rows Input parameter.
     * @param[in] result_bytes Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool validateResultBounds(const Plan& plan,
                              size_t result_rows,
                              size_t result_bytes) const noexcept;
    
    /**
     * @brief Validate Federation Scope Isolation.
     * @param[in] plan Input parameter.
     * @param[in] remote_scope_id Identifier of the remote scope.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool validateFederationScopeIsolation(const Plan& plan,
                                          const std::string& remote_scope_id) const noexcept;

private:
    SecondaryIndexManager& secIdx_;
    bool adaptive_enabled_ = false;

    // Injected collectors (non-owning pointers; callers manage lifetime)
    StatisticsCollector* stats_collector_ = nullptr;
    observability::MetricsCollector* metrics_collector_ = nullptr;

    // ========================================================================
    // THREAD-SAFETY: Per-Query Cost Model (GAP-1)
    // ========================================================================
    // Protects access to per_query_cost_model_ shared_ptr.
    // All attach/detach and execute methods must hold this lock when reading.
    // Ordering: This lock is held SECOND after advisor_cost_model_mutex_.
    mutable std::mutex per_query_cost_model_mutex_;

    // Per-query cost model (Phase 3, Issue #2419)
    // THREAD-SAFE: Protected by per_query_cost_model_mutex_
    mutable std::shared_ptr<performance::phase3::PerQueryCostModel> per_query_cost_model_;

    // ========================================================================
    // THREAD-SAFETY: Cost Model Constants (GAP-2)
    // ========================================================================
    // Protects access to advisor_cost_model_ member state.
    // All reads in chooseOrderForAndQuery() and writes via setAdvisorCostConstants()
    // must hold this lock.
    // Ordering: This lock is held FIRST. If both locks are needed, acquire this
    //           lock before per_query_cost_model_mutex_.
    mutable std::mutex advisor_cost_model_mutex_;

    // Cost model instance shared across chooseOrderForAndQuery() calls so that
    // calibrated constants (via setAdvisorCostConstants / PerQueryCostModel::calibrate)
    // are preserved between calls instead of being discarded with a local instance.
    // THREAD-SAFE: Protected by advisor_cost_model_mutex_
    OptimizerCostModel advisor_cost_model_;

    // ========================================================================
    // THREAD-SAFETY: Adaptive Optimization Initialization (GAP-3)
    // ========================================================================
    // Protects initialization of adaptive_stats_ and adaptive_selector_.
    // Used with std::call_once() for double-checked initialization.
    mutable std::once_flag adaptive_init_flag_;
    mutable std::mutex adaptive_init_mutex_;

    // Adaptive query optimization components
    // Use the full implementations from adaptive_optimizer.h
    // (No using needed - both AdaptiveQueryStats and AdaptivePlanSelector are in themis::query namespace)
    // THREAD-SAFE: Initialized once via call_once, then read-only
    
    class DistributedQueryCostModel {
    public:
        explicit DistributedQueryCostModel(
            StatisticsCollector* stats = nullptr,
            observability::MetricsCollector* metrics = nullptr)
            : stats_collector_(stats), metrics_collector_(metrics) {}

        struct ShardInfo {
            std::string shard_id;
            size_t estimated_rows = 0;
            double network_latency_ms = 0.0;
            bool is_local = false;
        };
        
        /**
         * @brief Should Prune Partition.
         * @param[in] info Input parameter.
         * @param[in] total_shards Input parameter.
         * @param[in] selectivity Input parameter.
         * @return True when the operation succeeds.
         */
        bool shouldPrunePartition(const ShardInfo& info, size_t total_shards, double selectivity) const;
        
        /**
         * @brief Get Optimal Parallelism.
         * @param[in] shards Input parameter.
         * @param[in] available_threads Input parameter.
         * @return Return value.
         */
        size_t getOptimalParallelism(const std::vector<ShardInfo>& shards, size_t available_threads) const;
        
        /**
         * @brief Get Shard Row Count.
         * @param[in] shard_id Identifier of the shard.
         * @param[in] table Input parameter.
         * @return Return value.
         */
        size_t getShardRowCount(const std::string& shard_id, const std::string& table) const;
        
        /**
         * @brief Measure Shard Latency.
         * @param[in] shard_id Identifier of the shard.
         * @return Return value.
         */
        double measureShardLatency(const std::string& shard_id) const;
        
        /**
         * @brief Calculate Predicate Selectivity.
         * @param[in] predicates Input parameter.
         * @param[in] table Input parameter.
         * @return Return value.
         */
        double calculatePredicateSelectivity(
            const std::vector<PredicateEq>& predicates,
            const std::string& table) const;

    private:
        // Non-owning pointers injected at construction time
        StatisticsCollector* stats_collector_ = nullptr;
        observability::MetricsCollector* metrics_collector_ = nullptr;
    };
    
    class MultiIndexOptimizer {
    public:
        MultiIndexOptimizer() = default;
    };
    
    // These will be initialized when adaptive optimization is enabled
    mutable std::shared_ptr<AdaptiveQueryStats> adaptive_stats_;
    mutable std::shared_ptr<AdaptivePlanSelector> adaptive_selector_;
    mutable std::shared_ptr<DistributedQueryCostModel> distributed_model_;
    mutable std::shared_ptr<MultiIndexOptimizer> multi_index_optimizer_;
};

} // namespace query
} // namespace themis
