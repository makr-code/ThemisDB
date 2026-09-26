/**
 * @file multi_model_selector.cpp
 * @brief Implementation of MultiModelSelector for Phase 12
 */

#include "rag/multi_model_selector.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace themis::rag::optimization {

struct MultiModelSelector::Impl {
  std::mutex mu;
  std::unordered_map<uint32_t, ModelStats> models;
  uint32_t baseline_model = 0;
  uint32_t min_samples = 100;
  double confidence_level = 0.95;
  
  // Rolling statistics for p-value computation
  std::unordered_map<uint32_t, std::vector<double>> quality_samples;
  std::unordered_map<uint32_t, std::vector<double>> latency_samples;
};

MultiModelSelector::MultiModelSelector(uint32_t num_models, uint32_t min_samples_for_decision)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->min_samples = min_samples_for_decision;
  pimpl_->models.reserve(num_models);
}

MultiModelSelector::~MultiModelSelector() = default;

uint32_t MultiModelSelector::RegisterModel(uint32_t model_version, const std::string& model_id,
                                          bool is_baseline) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  ModelStats stats;
  stats.model_version = model_version;
  stats.model_id = model_id;
  
  pimpl_->models[model_version] = stats;
  pimpl_->quality_samples[model_version].reserve(1000);
  pimpl_->latency_samples[model_version].reserve(1000);
  
  if (is_baseline) {
    pimpl_->baseline_model = model_version;
  }
  
  return model_version;
}

void MultiModelSelector::ReportQueryMetrics(uint32_t model_version, double latency_ms,
                                           double quality_score, uint32_t cost_tokens) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto it = pimpl_->models.find(model_version);
  if (it == pimpl_->models.end()) {
    return;  // Model not registered
  }
  
  ModelStats& stats = it->second;
  
  // Update statistics
  uint64_t n = stats.total_queries;
  stats.total_queries++;
  stats.total_cost += cost_tokens * 0.00001;  // Rough token-to-cost conversion
  
  // Running mean for latency
  double old_mean_latency = stats.mean_latency_ms;
  stats.mean_latency_ms = (old_mean_latency * n + latency_ms) / (n + 1);
  
  // Running mean for quality
  double old_mean_quality = stats.mean_quality_score;
  stats.mean_quality_score = (old_mean_quality * n + quality_score) / (n + 1);
  
  // Sample tracking for confidence intervals
  pimpl_->quality_samples[model_version].push_back(quality_score);
  pimpl_->latency_samples[model_version].push_back(latency_ms);
  
  // Keep only last 1000 samples for efficiency
  if (pimpl_->quality_samples[model_version].size() > 1000) {
    pimpl_->quality_samples[model_version].erase(pimpl_->quality_samples[model_version].begin());
  }
  if (pimpl_->latency_samples[model_version].size() > 1000) {
    pimpl_->latency_samples[model_version].erase(pimpl_->latency_samples[model_version].begin());
  }
  
  // Compute confidence based on sample size
  if (stats.total_queries > 0) {
    stats.confidence = std::min(1.0, static_cast<double>(stats.total_queries) / pimpl_->min_samples);
  }
}

ModelStats MultiModelSelector::GetModelStats(uint32_t model_version) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto it = pimpl_->models.find(model_version);
  if (it != pimpl_->models.end()) {
    return it->second;
  }
  return ModelStats();
}

uint32_t MultiModelSelector::SelectBestModel(double cost_weight) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  double best_score = -1e9;
  uint32_t best_model = 0;
  
  // Normalize cost and quality for comparison
  double max_cost = 1.0;
  double max_quality = 1.0;
  for (const auto& [version, stats] : pimpl_->models) {
    max_cost = std::max(max_cost, stats.total_cost);
    max_quality = std::max(max_quality, stats.mean_quality_score);
  }
  
  for (const auto& [version, stats] : pimpl_->models) {
    if (stats.total_queries == 0) continue;
    
    double normalized_cost = stats.total_cost / max_cost;
    double normalized_quality = stats.mean_quality_score / max_quality;
    
    // Composite score: (1-weight)*quality - weight*cost
    double score = (1.0 - cost_weight) * normalized_quality - cost_weight * normalized_cost;
    
    // Boost confidence in model with more samples
    score *= stats.confidence;
    
    if (score > best_score) {
      best_score = score;
      best_model = version;
    }
  }
  
  return best_model;
}

bool MultiModelSelector::IsStatisticallySignificantWinner(double confidence_threshold) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  if (pimpl_->baseline_model == 0) {
    return false;  // No baseline to compare against
  }
  
  auto baseline_it = pimpl_->models.find(pimpl_->baseline_model);
  if (baseline_it == pimpl_->models.end() || baseline_it->second.total_queries < pimpl_->min_samples) {
    return false;  // Insufficient baseline data
  }
  
  uint32_t best_model = SelectBestModel(0.5);  // Balanced weight for significance test
  if (best_model == pimpl_->baseline_model || best_model == 0) {
    return false;  // Baseline is already best
  }
  
  auto best_it = pimpl_->models.find(best_model);
  if (best_it == pimpl_->models.end() || best_it->second.total_queries < pimpl_->min_samples) {
    return false;  // Insufficient data on challenger
  }
  
  // Simple t-test: compare mean quality scores
  const auto& baseline_samples = pimpl_->quality_samples[pimpl_->baseline_model];
  const auto& best_samples = pimpl_->quality_samples[best_model];
  
  if (baseline_samples.size() < 10 || best_samples.size() < 10) {
    return false;  // Need at least 10 samples each
  }
  
  // Compute means and variances
  double baseline_mean = baseline_it->second.mean_quality_score;
  double best_mean = best_it->second.mean_quality_score;
  
  // Compute standard deviations
  double baseline_var = 0.0, best_var = 0.0;
  for (double x : baseline_samples) {
    baseline_var += (x - baseline_mean) * (x - baseline_mean);
  }
  for (double x : best_samples) {
    best_var += (x - best_mean) * (x - best_mean);
  }
  baseline_var /= baseline_samples.size();
  best_var /= best_samples.size();
  
  // Welch's t-test statistic
  double se = std::sqrt(baseline_var / baseline_samples.size() + best_var / best_samples.size());
  if (se < 1e-9) {
    return false;  // Zero variance, can't test
  }
  double t_stat = (best_mean - baseline_mean) / se;
  
  // Rough p-value approximation: t > 1.96 ≈ p < 0.05 for large N
  // For even stronger evidence, require t > 2.58 (p < 0.01)
  return t_stat > 2.58;  // p < 0.01 confidence
}

std::vector<ParetoPoint> MultiModelSelector::ComputeParetoFrontier() const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  std::vector<ParetoPoint> frontier;
  
  // Convert model stats to Pareto points
  std::vector<ParetoPoint> points;
  for (const auto& [version, stats] : pimpl_->models) {
    if (stats.total_queries == 0) continue;
    ParetoPoint p;
    p.model_version = version;
    p.cost = stats.total_cost / (stats.total_queries + 1);  // Cost per query
    p.quality = stats.mean_quality_score;
    points.push_back(p);
  }
  
  // Find Pareto frontier (no model dominates another on both cost and quality)
  for (size_t i = 0; i < points.size(); ++i) {
    bool dominated = false;
    for (size_t j = 0; j < points.size(); ++j) {
      if (i != j) {
        // Model j dominates i if j has lower cost AND higher quality
        if (points[j].cost < points[i].cost && points[j].quality > points[i].quality) {
          dominated = true;
          break;
        }
      }
    }
    if (!dominated) {
      points[i].is_dominated = false;
      frontier.push_back(points[i]);
    }
  }
  
  return frontier;
}

std::vector<uint32_t> MultiModelSelector::GetFallbackChain() const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  std::vector<uint32_t> chain;
  
  // Primary: current best model
  uint32_t best = SelectBestModel(0.5);
  if (best > 0) {
    chain.push_back(best);
  }
  
  // Secondary: previous best (baseline if it's not the same)
  if (pimpl_->baseline_model > 0 && pimpl_->baseline_model != best) {
    chain.push_back(pimpl_->baseline_model);
  }
  
  // Tertiary: any model with sufficient data (stability over optimization)
  for (const auto& [version, stats] : pimpl_->models) {
    if (stats.total_queries >= pimpl_->min_samples) {
      if (std::find(chain.begin(), chain.end(), version) == chain.end()) {
        chain.push_back(version);
      }
    }
  }
  
  return chain;
}

void MultiModelSelector::SetConfidenceLevel(double confidence_level) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  pimpl_->confidence_level = confidence_level;
}

}  // namespace themis::rag::optimization
