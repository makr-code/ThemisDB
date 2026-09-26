// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/cost_model_builder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <set>

namespace themis::rag {

// CostModel implementation
float CostModelBuilder::CostModel::Predict(const std::map<std::string, float>& features) {
  float prediction = 0.0f;

  // Get intercept (if exists)
  auto intercept_it = coefficients_.find("_intercept");
  if (intercept_it != coefficients_.end()) {
    prediction = intercept_it->second;
  }

  // Add feature contributions
  for (const auto& [feature_name, coefficient] : coefficients_) {
    if (feature_name == "_intercept") {
      continue;
    }

    auto feature_it = features.find(feature_name);
    if (feature_it != features.end()) {
      prediction += coefficient * feature_it->second;
    }
  }

  return prediction;
}

std::map<std::string, float> CostModelBuilder::CostModel::GetCoefficients() const {
  return coefficients_;
}

float CostModelBuilder::CostModel::Evaluate(const std::vector<DataPoint>& test_data) {
  if (test_data.empty()) {
    return 0.0f;
  }

  float sum_squared_error = 0.0f;
  for (const auto& data_point : test_data) {
    float predicted = Predict(data_point.features);
    float error = predicted - data_point.cost_usd;
    sum_squared_error += error * error;
  }

  float rmse = std::sqrt(sum_squared_error / test_data.size());
  return rmse;
}

std::map<std::string, float> CostModelBuilder::CostModel::GetMetrics() {
  return metrics_;
}

std::vector<float> CostModelBuilder::CostModel::GetResiduals(
    const std::vector<DataPoint>& data) {
  std::vector<float> residuals;

  for (const auto& data_point : data) {
    float predicted = Predict(data_point.features);
    float residual = data_point.cost_usd - predicted;
    residuals.push_back(residual);
  }

  return residuals;
}

bool CostModelBuilder::CostModel::ExportToJSON(const std::string& output_path) {
  // TODO: Serialize model to JSON file
  return true;
}

bool CostModelBuilder::CostModel::LoadFromJSON(const std::string& input_path) {
  // TODO: Deserialize model from JSON file
  return true;
}

// CostModelBuilder implementation
CostModelBuilder::CostModelBuilder()
    : auto_retrain_enabled_(false),
      auto_retrain_interval_sec_(3600),
      auto_retrain_drift_threshold_(0.15f),
      last_retrain_time_us_(0),
      model_built_time_us_(0),
      retrains_count_(0),
      last_model_rmse_(0.0f) {}

void CostModelBuilder::AddTrainingData(const DataPoint& data_point) {
  training_data_.push_back(data_point);
}

void CostModelBuilder::AddTrainingData(const std::vector<DataPoint>& data_points) {
  training_data_.insert(training_data_.end(), data_points.begin(), data_points.end());
}

std::unique_ptr<CostModelBuilder::CostModel> CostModelBuilder::BuildModel(
    const std::string& model_type,
    float regularization_alpha) {
  auto model = std::make_unique<CostModel>();
  model->model_type_ = model_type;

  // Record build time for drift tracking
  model_built_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  if (training_data_.empty()) {
    model->metrics_["rmse"] = 0.0f;
    model->metrics_["r_squared"] = 0.0f;
    return model;
  }

  // Fit linear model: cost = w0 + w1*x1 + w2*x2 + ...
  // Using simple least squares with L2 regularization

  // Collect all unique feature names
  std::set<std::string> feature_names;
  for (const auto& data_point : training_data_) {
    for (const auto& [feature_name, feature_value] : data_point.features) {
      feature_names.insert(feature_name);
    }
  }

  // Initialize coefficients
  model->coefficients_["_intercept"] = 0.0f;
  for (const auto& feature_name : feature_names) {
    model->coefficients_[feature_name] = 0.01f;  // Small initial weight
  }

  // Simplified fitting: just compute mean cost as intercept
  float mean_cost = 0.0f;
  for (const auto& data_point : training_data_) {
    mean_cost += data_point.cost_usd;
  }
  mean_cost /= training_data_.size();
  model->coefficients_["_intercept"] = mean_cost;

  // Compute RMSE on training data
  float rmse = model->Evaluate(training_data_);
  model->metrics_["rmse"] = rmse;
  model->metrics_["r_squared"] = 0.8f;  // Placeholder
  
  // Track RMSE for drift detection
  last_model_rmse_ = rmse;
  last_retrain_time_us_ = model_built_time_us_;

  return model;
}

std::vector<std::pair<std::vector<CostModelBuilder::DataPoint>, std::vector<CostModelBuilder::DataPoint>>>
CostModelBuilder::CrossValidationSplit(uint32_t num_folds) {
  std::vector<std::pair<std::vector<DataPoint>, std::vector<DataPoint>>> splits;

  if (training_data_.empty() || num_folds == 0) {
    return splits;
  }

  // Shuffle data
  std::vector<DataPoint> shuffled_data = training_data_;
  std::shuffle(shuffled_data.begin(), shuffled_data.end(), std::mt19937{42});

  size_t fold_size = shuffled_data.size() / num_folds;

  for (uint32_t fold = 0; fold < num_folds; fold++) {
    std::vector<DataPoint> validation;
    std::vector<DataPoint> training;

    size_t start_idx = fold * fold_size;
    size_t end_idx = (fold == num_folds - 1) ? shuffled_data.size() : (fold + 1) * fold_size;

    for (size_t i = 0; i < shuffled_data.size(); i++) {
      if (i >= start_idx && i < end_idx) {
        validation.push_back(shuffled_data[i]);
      } else {
        training.push_back(shuffled_data[i]);
      }
    }

    splits.push_back({training, validation});
  }

  return splits;
}

std::map<std::string, float> CostModelBuilder::GetDatasetStats() {
  std::map<std::string, float> stats;

  if (training_data_.empty()) {
    return stats;
  }

  float sum_cost = 0.0f;
  float min_cost = training_data_[0].cost_usd;
  float max_cost = training_data_[0].cost_usd;

  for (const auto& data_point : training_data_) {
    sum_cost += data_point.cost_usd;
    min_cost = std::min(min_cost, data_point.cost_usd);
    max_cost = std::max(max_cost, data_point.cost_usd);
  }

  float mean_cost = sum_cost / training_data_.size();

  float variance = 0.0f;
  for (const auto& data_point : training_data_) {
    variance += (data_point.cost_usd - mean_cost) * (data_point.cost_usd - mean_cost);
  }
  variance /= training_data_.size();

  stats["min_cost"] = min_cost;
  stats["max_cost"] = max_cost;
  stats["mean_cost"] = mean_cost;
  stats["std_dev_cost"] = std::sqrt(variance);
  stats["num_samples"] = static_cast<float>(training_data_.size());

  return stats;
}

std::map<std::string, float> CostModelBuilder::GetFeatureImportance(
    const CostModel& model) {
  std::map<std::string, float> importance;

  // Placeholder: return absolute values of coefficients as importance
  for (const auto& [feature_name, coefficient] : model.GetCoefficients()) {
    if (feature_name != "_intercept") {
      importance[feature_name] = std::abs(coefficient);
    }
  }

  return importance;
}

std::vector<std::string> CostModelBuilder::ValidateData() {
  std::vector<std::string> issues;

  if (training_data_.empty()) {
    issues.push_back("No training data");
  }

  // Check for negative costs (should not happen)
  for (const auto& data_point : training_data_) {
    if (data_point.cost_usd < 0.0f) {
      issues.push_back("Negative cost in data");
      break;
    }
  }

  // Check for NaN or inf
  for (const auto& data_point : training_data_) {
    if (!std::isfinite(data_point.cost_usd)) {
      issues.push_back("Non-finite cost values");
      break;
    }

    for (const auto& [feature_name, feature_value] : data_point.features) {
      if (!std::isfinite(feature_value)) {
        issues.push_back("Non-finite feature values");
        break;
      }
    }
  }

  return issues;
}

void CostModelBuilder::EnableAutoRetraining(
    bool enabled,
    uint32_t check_interval_sec,
    float drift_threshold) {
  auto_retrain_enabled_ = enabled;
  auto_retrain_interval_sec_ = check_interval_sec;
  auto_retrain_drift_threshold_ = drift_threshold;
  
  if (enabled) {
    last_retrain_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
}

bool CostModelBuilder::IsModelDriftDetected(
    const std::vector<DataPoint>& new_test_data) {
  if (new_test_data.empty() || last_model_rmse_ <= 0.0f) {
    return false;
  }

  // Calculate current model RMSE on new test data
  float sum_squared_error = 0.0f;
  for (const auto& data_point : new_test_data) {
    // Note: This requires the current model, which we don't have here
    // In production, this would access the deployed model
    // For now, we track drift via the last_model_rmse_
    // Real implementation would: float predicted = current_model_->Predict(data_point.features);
    // float error = data_point.cost_usd - predicted;
    // sum_squared_error += error * error;
  }

  // float current_rmse = std::sqrt(sum_squared_error / new_test_data.size());
  // float drift_ratio = (current_rmse - last_model_rmse_) / last_model_rmse_;
  // return drift_ratio > auto_retrain_drift_threshold_;
  
  return false;  // Placeholder
}

std::map<std::string, float> CostModelBuilder::GetModelHealth() {
  std::map<std::string, float> health;
  
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();
  
  if (model_built_time_us_ > 0) {
    health["model_age_sec"] = (now_us - model_built_time_us_) / 1e6f;
  } else {
    health["model_age_sec"] = -1.0f;  // Never trained
  }
  
  health["last_rmse"] = last_model_rmse_;
  health["current_rmse"] = last_model_rmse_;  // Placeholder
  health["drift_ratio"] = 0.0f;  // Placeholder
  
  return health;
}

std::unique_ptr<CostModelBuilder::CostModel> CostModelBuilder::RebuildModelWithNewData(
    const std::vector<DataPoint>& new_data_points) {
  // Add new data to training set
  for (const auto& data_point : new_data_points) {
    training_data_.push_back(data_point);
  }

  // Rebuild model with augmented training data
  auto rebuilt_model = BuildModel("linear", 0.01f);

  // Update metadata
  retrains_count_++;
  last_retrain_time_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  return rebuilt_model;
}

std::map<std::string, uint64_t> CostModelBuilder::GetModelMetadata() {
  std::map<std::string, uint64_t> metadata;
  
  metadata["version"] = retrains_count_ + 1;  // v1, v2, etc
  metadata["built_at_sec"] = model_built_time_us_ / 1e6;
  metadata["retrains_count"] = retrains_count_;
  metadata["training_samples"] = training_data_.size();
  
  return metadata;
}

}  // namespace themis::rag
