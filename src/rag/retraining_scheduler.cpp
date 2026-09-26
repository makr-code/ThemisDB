/**
 * @file retraining_scheduler.cpp
 * @brief Retraining Scheduler implementation
 *
 * Monitors cost model drift, quality regression, and time-based schedules
 * to trigger automated retraining.
 */

#include "rag/retraining_scheduler.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace themis::rag::lifecycle {

std::string TriggerToString(RetariningTrigger trigger) {
  switch (trigger) {
    case RetariningTrigger::kScheduledTime:
      return "scheduled_time";
    case RetariningTrigger::kDriftDetected:
      return "drift_detected";
    case RetariningTrigger::kQualityRegression:
      return "quality_regression";
    case RetariningTrigger::kManualRequest:
      return "manual_request";
    case RetariningTrigger::kNone:
      return "none";
  }
  return "unknown";
}

struct RetariningScheduler::Impl {
  ModelRegistry* model_registry = nullptr;
  RetariningCallback callback = nullptr;
  
  uint32_t retraining_interval_hours = 24;
  double drift_threshold_rmse = 0.15;
  double quality_regression_threshold_pct = 0.05;

  std::atomic<bool> running{false};
  std::atomic<bool> retraining_in_progress{false};
  std::unique_ptr<std::thread> monitor_thread;
  
  mutable std::mutex mu;
  
  // Last trigger info
  RetariningTrigger last_trigger = RetariningTrigger::kNone;
  std::string last_trigger_reason;
  uint64_t last_request_time_us = 0;

  // Drift tracking
  double last_baseline_rmse = 0.0;

  // Quality tracking
  std::string last_regressed_metric;
  double last_baseline_quality = 1.0;

  uint64_t GetCurrentTimestampUs() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return duration_cast<microseconds>(duration).count();
  }

  void MonitoringLoop() {
    // Placeholder: periodic monitoring would check:
    // 1. Time elapsed since last retraining
    // 2. Cost model drift signals
    // 3. Quality regression signals
    // 4. Manual requests
    while (running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      // Check conditions and invoke callback if needed
    }
  }
};

RetariningScheduler::RetariningScheduler(ModelRegistry& model_registry,
                                         uint32_t retraining_interval_hours,
                                         double drift_threshold_rmse,
                                         double quality_regression_threshold_pct)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->model_registry = &model_registry;
  pimpl_->retraining_interval_hours = retraining_interval_hours;
  pimpl_->drift_threshold_rmse = drift_threshold_rmse;
  pimpl_->quality_regression_threshold_pct = quality_regression_threshold_pct;
}

RetariningScheduler::~RetariningScheduler() {
  Stop();
}

void RetariningScheduler::SetRetariningCallback(const RetariningCallback& callback) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  pimpl_->callback = callback;
}

void RetariningScheduler::Start() {
  if (pimpl_->running.exchange(true)) {
    // Already running
    return;
  }

  pimpl_->monitor_thread = std::make_unique<std::thread>([this] {
    pimpl_->MonitoringLoop();
  });
}

void RetariningScheduler::Stop() {
  pimpl_->running = false;
  if (pimpl_->monitor_thread && pimpl_->monitor_thread->joinable()) {
    pimpl_->monitor_thread->join();
  }
}

bool RetariningScheduler::IsRetariningInProgress() const {
  return pimpl_->retraining_in_progress;
}

bool RetariningScheduler::RequestRetraining() {
  std::unique_lock<std::mutex> lock(pimpl_->mu);

  if (pimpl_->retraining_in_progress) {
    // Already retraining, queue the request but don't trigger again
    return true;
  }

  pimpl_->last_trigger = RetariningTrigger::kManualRequest;
  pimpl_->last_trigger_reason = "Manual retraining request";
  pimpl_->last_request_time_us = pimpl_->GetCurrentTimestampUs();
  pimpl_->retraining_in_progress = true;

  if (pimpl_->callback) {
    // Unlock before invoking callback to avoid deadlock
    lock.unlock();
    pimpl_->callback(RetariningTrigger::kManualRequest, "Manual retraining request");
    lock.lock();
  }

  return true;
}

void RetariningScheduler::ReportCostModelDrift(double current_rmse,
                                                double baseline_rmse) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);

  if (baseline_rmse == 0.0) {
    // Can't compute drift ratio if baseline is zero
    return;
  }

  double drift_ratio = (current_rmse - baseline_rmse) / baseline_rmse;

  if (drift_ratio > pimpl_->drift_threshold_rmse && !pimpl_->retraining_in_progress) {
    pimpl_->last_trigger = RetariningTrigger::kDriftDetected;
    pimpl_->last_trigger_reason = "Cost model RMSE drift: " + std::to_string(drift_ratio);
    pimpl_->last_request_time_us = pimpl_->GetCurrentTimestampUs();
    pimpl_->retraining_in_progress = true;
    pimpl_->last_baseline_rmse = baseline_rmse;

    if (pimpl_->callback) {
      lock.unlock();
      pimpl_->callback(RetariningTrigger::kDriftDetected, pimpl_->last_trigger_reason);
      lock.lock();
    }
  }
}

void RetariningScheduler::ReportQualityRegression(const std::string& metric_name,
                                                  double current_value,
                                                  double baseline_value) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);

  if (baseline_value == 0.0) {
    return;
  }

  double regression_pct = (baseline_value - current_value) / baseline_value;

  if (regression_pct > pimpl_->quality_regression_threshold_pct &&
      !pimpl_->retraining_in_progress) {
    pimpl_->last_trigger = RetariningTrigger::kQualityRegression;
    pimpl_->last_trigger_reason =
        "Quality regression in " + metric_name + ": " + std::to_string(regression_pct);
    pimpl_->last_request_time_us = pimpl_->GetCurrentTimestampUs();
    pimpl_->retraining_in_progress = true;
    pimpl_->last_regressed_metric = metric_name;
    pimpl_->last_baseline_quality = baseline_value;

    if (pimpl_->callback) {
      lock.unlock();
      pimpl_->callback(RetariningTrigger::kQualityRegression, pimpl_->last_trigger_reason);
      lock.lock();
    }
  }
}

std::pair<RetariningTrigger, std::string> RetariningScheduler::GetLastTriggerReason()
    const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return {pimpl_->last_trigger, pimpl_->last_trigger_reason};
}

uint64_t RetariningScheduler::GetLastRetariningRequestTimeUs() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->last_request_time_us;
}

}  // namespace themis::rag::lifecycle
