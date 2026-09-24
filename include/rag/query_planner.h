/**
 * @file query_planner.h
 * @brief Query-aware routing optimization for RAG Phase 12
 *
 * Analyzes query complexity and selects optimal retrieval strategies
 * (lexical vs dense vs hybrid) with adaptive budget allocation.
 *
 * @version 0.1.0
 * @note Phase: 12 (Advanced Cost Optimization)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::optimization {

/**
 * @brief Query complexity assessment
 */
struct QueryComplexity {
  enum Level { kSimple, kModerate, kComplex };
  
  Level level = kSimple;
  double estimated_cost = 0.0;      ///< Estimated cost in latency/tokens
  uint32_t suggested_retrieval_k = 10;  ///< Number of docs to retrieve
  uint32_t reranker_budget = 5;     ///< Number of docs to re-rank
  std::string optimal_strategy;      ///< "lexical", "dense", "hybrid"
};

/**
 * @brief Query Planner — Cost-aware query routing
 *
 * Analyzes query intent and complexity to select optimal retrieval strategy
 * and allocate budgets for retrieval, re-ranking, and generation stages.
 * Integrates with cost model to predict end-to-end latency and cost.
 *
 * Thread-safe for concurrent query planning.
 */
class QueryPlanner {
 public:
  /**
   * @brief Constructor
   *
   * @param cost_model_predictor Callback to get cost predictions from Phase 10
   * @param enable_adaptive_budget true to enable dynamic budget adjustment
   */
  QueryPlanner(bool enable_adaptive_budget = true);
  ~QueryPlanner();

  /**
   * @brief Analyze query and recommend retrieval strategy
   *
   * Determines query complexity and selects optimal retrieval approach:
   * - Lexical: BM25 for factual questions ("Who is X?")
   * - Dense: Embeddings for semantic similarity ("Similar to X")
   * - Hybrid: Fusion for complex queries requiring multiple signals
   *
   * @param query User query text
   * @param available_strategies List of enabled strategies (e.g., {"lexical", "dense", "hybrid"})
   * @return Complexity assessment with strategy recommendation
   */
  QueryComplexity AnalyzeQuery(const std::string& query,
                               const std::vector<std::string>& available_strategies);

  /**
   * @brief Estimate end-to-end latency for query
   *
   * Predicts latency across retrieval → re-ranking → generation pipeline.
   * Uses cost model from Phase 10 for per-stage estimates.
   *
   * @param query Query text
   * @param strategy Selected retrieval strategy
   * @param generation_model LLM model size ("small", "medium", "large")
   * @return Estimated latency in milliseconds
   */
  double EstimateLatency(const std::string& query, const std::string& strategy,
                        const std::string& generation_model = "medium");

  /**
   * @brief Allocate re-ranking budget based on cost model
   *
   * Determines how many candidate documents to re-rank based on:
   * - Available latency budget
   * - Re-ranker cost (from Phase 10)
   * - Query importance (tenant SLO)
   *
   * @param available_latency_ms Latency budget in milliseconds
   * @param query Query text for complexity assessment
   * @param default_k Default number of docs to retrieve
   * @return Recommended number of docs to re-rank
   */
  uint32_t AllocateRerankerBudget(double available_latency_ms, const std::string& query,
                                  uint32_t default_k = 10);

  /**
   * @brief Configure cost model predictor callback
   *
   * Allows QueryPlanner to query Phase 10 (CostModelBuilder) for
   * per-operation cost estimates.
   *
   * @param predictor Callback: (operation, params) -> estimated_ms
   */
  using CostPredictor = std::function<double(const std::string&, const std::string&)>;
  void SetCostPredictor(const CostPredictor& predictor);

  /**
   * @brief Set complexity thresholds
   *
   * Configures where kSimple/kModerate/kComplex boundaries are.
   *
   * @param simple_threshold Max complexity for "simple" queries (default 0.3)
   * @param moderate_threshold Max complexity for "moderate" queries (default 0.7)
   */
  void SetComplexityThresholds(double simple_threshold, double moderate_threshold);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::optimization
