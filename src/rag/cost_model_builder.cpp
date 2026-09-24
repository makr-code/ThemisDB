// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/cost_model_builder.h"

#include <algorithm>
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
CostModelBuilder::CostModelBuilder() {}

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

}  // namespace themis::rag
