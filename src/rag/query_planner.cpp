/**
 * @file query_planner.cpp
 * @brief Implementation of QueryPlanner for Phase 12
 */

#include "rag/query_planner.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace themis::rag::optimization {

struct QueryPlanner::Impl {
  std::mutex mu;
  CostPredictor predictor;
  bool enable_adaptive_budget = true;
  
  double simple_threshold = 0.3;
  double moderate_threshold = 0.7;
  
  // Cached complexity scores for common queries
  std::unordered_map<std::string, double> complexity_cache;
};

QueryPlanner::QueryPlanner(bool enable_adaptive_budget)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->enable_adaptive_budget = enable_adaptive_budget;
}

QueryPlanner::~QueryPlanner() = default;

QueryComplexity QueryPlanner::AnalyzeQuery(const std::string& query,
                                          const std::vector<std::string>& available_strategies) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  QueryComplexity result;
  
  // Analyze query characteristics
  double complexity = 0.0;
  
  // Token length heuristic (longer = more complex)
  uint32_t token_count = query.length() / 4;  // Rough estimate: 4 chars per token
  if (token_count > 50) {
    complexity += 0.3;
  } else if (token_count > 20) {
    complexity += 0.15;
  }
  
  // Query type detection
  if (query.find("?") != std::string::npos) {
    complexity += 0.2;  // Questions often more complex
  }
  if (query.find(" AND ") != std::string::npos || 
      query.find(" OR ") != std::string::npos) {
    complexity += 0.25;  // Boolean operators indicate complexity
  }
  if (query.find("compare") != std::string::npos ||
      query.find("difference") != std::string::npos) {
    complexity += 0.15;  // Comparative queries
  }
  
  // Clamp complexity to [0, 1]
  complexity = std::min(1.0, complexity);
  
  // Determine complexity level
  if (complexity <= pimpl_->simple_threshold) {
    result.level = QueryComplexity::kSimple;
    result.suggested_retrieval_k = 5;
    result.reranker_budget = 3;
  } else if (complexity <= pimpl_->moderate_threshold) {
    result.level = QueryComplexity::kModerate;
    result.suggested_retrieval_k = 10;
    result.reranker_budget = 5;
  } else {
    result.level = QueryComplexity::kComplex;
    result.suggested_retrieval_k = 20;
    result.reranker_budget = 10;
  }
  
  // Strategy selection
  if (!available_strategies.empty()) {
    // Simple queries: lexical (fast, low-cost)
    if (result.level == QueryComplexity::kSimple) {
      auto it = std::find(available_strategies.begin(), available_strategies.end(), "lexical");
      if (it != available_strategies.end()) {
        result.optimal_strategy = "lexical";
        result.estimated_cost = 5.0;  // ms
      } else {
        result.optimal_strategy = available_strategies[0];
        result.estimated_cost = 10.0;
      }
    }
    // Complex queries: hybrid (balanced)
    else if (result.level == QueryComplexity::kComplex) {
      auto it = std::find(available_strategies.begin(), available_strategies.end(), "hybrid");
      if (it != available_strategies.end()) {
        result.optimal_strategy = "hybrid";
        result.estimated_cost = 20.0;  // ms
      } else {
        result.optimal_strategy = available_strategies[0];
        result.estimated_cost = 25.0;
      }
    }
    // Moderate: dense (good quality-cost balance)
    else {
      auto it = std::find(available_strategies.begin(), available_strategies.end(), "dense");
      if (it != available_strategies.end()) {
        result.optimal_strategy = "dense";
        result.estimated_cost = 15.0;  // ms
      } else {
        result.optimal_strategy = available_strategies[0];
        result.estimated_cost = 15.0;
      }
    }
  }
  
  return result;
}

double QueryPlanner::EstimateLatency(const std::string& query, const std::string& strategy,
                                    const std::string& generation_model) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  double latency = 0.0;
  
  // Base retrieval latency
  if (strategy == "lexical") {
    latency += 5.0;  // BM25 is fast
  } else if (strategy == "dense") {
    latency += 15.0;  // Embedding lookup
  } else if (strategy == "hybrid") {
    latency += 25.0;  // Both paths
  } else {
    latency += 10.0;  // Default
  }
  
  // Re-ranking latency (typically 5-10ms per doc)
  uint32_t rerank_budget = AllocateRerankerBudget(5000.0, query);
  latency += rerank_budget * 2.0;  // ~2ms per doc
  
  // Generation latency depends on model size
  if (generation_model == "small") {
    latency += 100.0;
  } else if (generation_model == "medium") {
    latency += 200.0;
  } else if (generation_model == "large") {
    latency += 400.0;
  }
  
  return latency;
}

uint32_t QueryPlanner::AllocateRerankerBudget(double available_latency_ms, const std::string& query,
                                              uint32_t default_k) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  QueryComplexity complexity = AnalyzeQuery(query, {"lexical", "dense", "hybrid"});
  
  // Re-ranker cost ~2ms per doc (from Phase 10 cost model)
  // Leave 10% overhead for query planning/other operations
  double available_for_reranking = available_latency_ms * 0.9;
  
  // Estimate cost of retrieval + generation (leave remainder for re-ranking)
  double retrieval_cost = 10.0;  // Typical: 10ms
  double generation_cost = 150.0;  // Typical: 150ms
  double available_for_budget = available_for_reranking - retrieval_cost - generation_cost;
  
  if (available_for_budget < 10.0) {
    // If tight budget, skip re-ranking
    return 0;
  }
  
  // How many docs can we re-rank in available time?
  // Assume 2ms per doc, but complexity affects this
  double cost_per_doc = 2.0;
  if (complexity.level == QueryComplexity::kComplex) {
    cost_per_doc = 3.0;  // Complex queries need more sophisticated re-ranking
  }
  
  uint32_t budget = static_cast<uint32_t>(available_for_budget / cost_per_doc);
  
  // Clamp to reasonable range [0, 20]
  return std::min(20u, std::max(0u, budget));
}

void QueryPlanner::SetCostPredictor(const CostPredictor& predictor) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  pimpl_->predictor = predictor;
}

void QueryPlanner::SetComplexityThresholds(double simple_threshold, double moderate_threshold) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  pimpl_->simple_threshold = simple_threshold;
  pimpl_->moderate_threshold = moderate_threshold;
}

}  // namespace themis::rag::optimization
