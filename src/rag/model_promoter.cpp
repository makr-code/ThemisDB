/**
 * @file model_promoter.cpp
 * @brief Model Promoter implementation
 *
 * Canary deployment orchestration with progressive traffic shifting and
 * automatic rollback on quality regression.
 */

#include "rag/model_promoter.h"

#include <iostream>
#include <mutex>
#include <thread>

#include "rag/model_evaluator.h"
#include "rag/model_registry.h"

namespace themis::rag::lifecycle {

std::string PhaseToString(CanaryPhase phase) {
  switch (phase) {
    case CanaryPhase::kNone:
      return "none";
    case CanaryPhase::kShadow:
      return "shadow";
    case CanaryPhase::kCanary5:
      return "canary_5pct";
    case CanaryPhase::kCanary10:
      return "canary_10pct";
    case CanaryPhase::kCanary25:
      return "canary_25pct";
    case CanaryPhase::kCanary50:
      return "canary_50pct";
    case CanaryPhase::kDeployed:
      return "deployed";
  }
  return "unknown";
}

struct ModelPromoter::Impl {
  ModelRegistry* model_registry = nullptr;
  ModelEvaluator* model_evaluator = nullptr;
  
  double quality_regression_threshold_pct = 0.05;
  uint32_t canary_phase_duration_hours = 1;
  
  QualityRegressionCallback regression_callback = nullptr;

  // Current canary state
  uint32_t baseline_version = 0;
  uint32_t canary_version = 0;
  CanaryPhase current_phase = CanaryPhase::kNone;
  uint64_t canary_start_time_us = 0;
  uint64_t phase_start_time_us = 0;
  
  mutable std::mutex mu;

  uint64_t GetCurrentTimestampUs() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<microseconds>(duration).count();
  }

  CanaryPhase GetNextPhase(CanaryPhase current) {
    switch (current) {
      case CanaryPhase::kNone:
        return CanaryPhase::kShadow;
      case CanaryPhase::kShadow:
        return CanaryPhase::kCanary5;
      case CanaryPhase::kCanary5:
        return CanaryPhase::kCanary10;
      case CanaryPhase::kCanary10:
        return CanaryPhase::kCanary25;
      case CanaryPhase::kCanary25:
        return CanaryPhase::kCanary50;
      case CanaryPhase::kCanary50:
        return CanaryPhase::kDeployed;
      case CanaryPhase::kDeployed:
        return CanaryPhase::kDeployed;  // Terminal state
    }
    return CanaryPhase::kNone;
  }

  double GetTrafficPercentage(CanaryPhase phase) {
    switch (phase) {
      case CanaryPhase::kShadow:
        return 0.0;
      case CanaryPhase::kCanary5:
        return 5.0;
      case CanaryPhase::kCanary10:
        return 10.0;
      case CanaryPhase::kCanary25:
        return 25.0;
      case CanaryPhase::kCanary50:
        return 50.0;
      case CanaryPhase::kDeployed:
        return 100.0;
      default:
        return 0.0;
    }
  }
};

ModelPromoter::ModelPromoter(ModelRegistry& model_registry,
                             ModelEvaluator& model_evaluator,
                             double quality_regression_threshold_pct,
                             uint32_t canary_phase_duration_hours)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->model_registry = &model_registry;
  pimpl_->model_evaluator = &model_evaluator;
  pimpl_->quality_regression_threshold_pct = quality_regression_threshold_pct;
  pimpl_->canary_phase_duration_hours = canary_phase_duration_hours;
}

ModelPromoter::~ModelPromoter() = default;

bool ModelPromoter::StartCanary(uint32_t candidate_version) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->current_phase != CanaryPhase::kNone) {
    // Canary already running
    return false;
  }

  if (!pimpl_->model_registry->Exists(candidate_version)) {
    return false;
  }

  auto deployed = pimpl_->model_registry->GetDeployedModel();
  pimpl_->baseline_version = deployed ? deployed->version : 0;
  pimpl_->canary_version = candidate_version;
  pimpl_->current_phase = CanaryPhase::kShadow;
  pimpl_->canary_start_time_us = pimpl_->GetCurrentTimestampUs();
  pimpl_->phase_start_time_us = pimpl_->canary_start_time_us;

  return true;
}

TrafficSplitDecision ModelPromoter::GetTrafficSplit() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  TrafficSplitDecision decision;
  decision.baseline_version = pimpl_->baseline_version;
  decision.canary_version = pimpl_->canary_version;
  decision.phase = pimpl_->current_phase;
  decision.canary_traffic_pct = pimpl_->GetTrafficPercentage(pimpl_->current_phase);

  return decision;
}

bool ModelPromoter::AdvancePhase() {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->current_phase == CanaryPhase::kNone) {
    return false;
  }

  CanaryPhase next_phase = pimpl_->GetNextPhase(pimpl_->current_phase);
  if (next_phase == pimpl_->current_phase) {
    // Already at terminal phase
    return false;
  }

  pimpl_->current_phase = next_phase;
  pimpl_->phase_start_time_us = pimpl_->GetCurrentTimestampUs();

  return true;
}

bool ModelPromoter::Rollback(const std::string& reason) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->current_phase == CanaryPhase::kNone) {
    return false;
  }

  // Revert to baseline model
  pimpl_->current_phase = CanaryPhase::kNone;
  pimpl_->canary_version = 0;

  // Could mark canary version as failed in registry here
  if (pimpl_->model_registry && pimpl_->canary_version > 0) {
    pimpl_->model_registry->UpdateModelStatus(pimpl_->canary_version,
                                               ModelStatus::kFailed,
                                               "Canary rollback: " + reason);
  }

  return true;
}

bool ModelPromoter::ReportMetric(const std::string& metric_name, double current_value,
                                 double baseline_value) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);

  if (pimpl_->current_phase == CanaryPhase::kNone) {
    return true;
  }

  if (baseline_value == 0.0) {
    return true;
  }

  double regression = (baseline_value - current_value) / baseline_value;

  if (regression > pimpl_->quality_regression_threshold_pct) {
    // Regression detected, trigger rollback
    if (pimpl_->regression_callback) {
      lock.unlock();
      pimpl_->regression_callback(metric_name, baseline_value, current_value,
                                  pimpl_->quality_regression_threshold_pct);
      lock.lock();
    }

    Rollback("Quality regression in " + metric_name);
    return false;
  }

  return true;
}

void ModelPromoter::SetQualityRegressionCallback(const QualityRegressionCallback& callback) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  pimpl_->regression_callback = callback;
}

CanaryPhase ModelPromoter::GetCurrentPhase() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->current_phase;
}

bool ModelPromoter::IsCanaryActive() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->current_phase != CanaryPhase::kNone;
}

uint64_t ModelPromoter::GetCanaryStartTimeUs() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->canary_start_time_us;
}

bool ModelPromoter::FinalizeDeployment() {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  if (pimpl_->current_phase != CanaryPhase::kDeployed) {
    return false;
  }

  // Update model registry: mark canary as deployed, archive baseline
  if (pimpl_->baseline_version > 0 && pimpl_->model_registry) {
    pimpl_->model_registry->UpdateModelStatus(pimpl_->baseline_version,
                                               ModelStatus::kRetired,
                                               "Replaced by v" +
                                                   std::to_string(pimpl_->canary_version));
  }

  if (pimpl_->canary_version > 0 && pimpl_->model_registry) {
    pimpl_->model_registry->UpdateModelStatus(pimpl_->canary_version,
                                               ModelStatus::kDeployed,
                                               "Canary promotion complete");
  }

  // Clear canary state
  pimpl_->current_phase = CanaryPhase::kNone;
  pimpl_->baseline_version = pimpl_->canary_version;
  pimpl_->canary_version = 0;

  return true;
}

}  // namespace themis::rag::lifecycle
