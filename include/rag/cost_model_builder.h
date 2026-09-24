// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Cost model construction from historical data.
///
/// Builds predictive cost models by fitting cost functions to historical
/// data. Supports multiple model types (linear, polynomial, neural) and
/// multi-variate regression with cross-validation.
///
/// @details
/// Cost model components:
/// - Fixed costs: Infrastructure, storage, licensing
/// - Variable costs: Per-query retrieval + reranking
/// - Scaling factors: Batch size, staleness tolerance, freshness SLA
///
/// Variables (model inputs):
/// - num_retrieved: # documents retrieved per query
/// - rerank_fraction: % of queries reranked
/// - query_complexity: Query length in tokens
/// - staleness_tolerance_sec: Allowed staleness
/// - freshness_sla_p95_sec: SLA target p95
///
/// Model types:
/// - Linear: cost = w0 + w1*x1 + w2*x2 + ...
/// - Polynomial: cost = w0 + w1*x1 + w2*x1^2 + ...
/// - Ridge regression: Adds L2 penalty to prevent overfitting
/// - Kernel: RBF or polynomial kernel regression
///
/// Training workflow:
/// 1. Collect historical cost data (run_id, variables, cost)
/// 2. Split into train/validation/test (70/15/15)
/// 3. Fit model: minimize ||cost - model(variables)||^2
/// 4. Evaluate: report RMSE, R^2, residual analysis
/// 5. Deploy: use model for forecasting
///
/// @code
/// auto builder = std::make_unique<CostModelBuilder>();
/// builder->AddTrainingData(cost_records);
/// auto model = builder->BuildModel("linear", 0.1);  // L2 alpha
/// auto validation_rmse = model->Evaluate(validation_data);
/// auto forecast = model->Predict({num_retrieved: 20, rerank_fraction: 0.8});
/// @endcode
class CostModelBuilder {
 public:
  /// @brief Training data point.
  struct DataPoint {
    std::map<std::string, float> features;  ///< Variable values
    float cost_usd;                         ///< Actual cost
    int64_t timestamp_us;
  };

  /// @brief Fitted cost model.
  class CostModel {
   public:
    /// @brief Predict cost for given variables.
    /// @param features Feature map (variable_name -> value).
    /// @return Predicted cost in USD.
    float Predict(const std::map<std::string, float>& features);

    /// @brief Get model coefficients.
    /// @return Map of variable_name -> coefficient.
    std::map<std::string, float> GetCoefficients();

    /// @brief Evaluate on test data.
    /// @param test_data Test data points.
    /// @return RMSE (root mean squared error).
    float Evaluate(const std::vector<DataPoint>& test_data);

    /// @brief Get model quality metrics.
    /// @return Map of metric_name -> value (R^2, RMSE, MAE, etc).
    std::map<std::string, float> GetMetrics();

    /// @brief Compute residuals.
    /// @param data Test data.
    /// @return Prediction residuals (actual - predicted).
    std::vector<float> GetResiduals(const std::vector<DataPoint>& data);

    /// @brief Export model to JSON.
    /// @param output_path File path.
    /// @return true if exported successfully.
    bool ExportToJSON(const std::string& output_path);

    /// @brief Load model from JSON.
    /// @param input_path File path.
    /// @return true if loaded successfully.
    bool LoadFromJSON(const std::string& input_path);

   private:
    friend class CostModelBuilder;

    std::string model_type_;  ///< "linear", "polynomial", "kernel", etc
    std::map<std::string, float> coefficients_;
    std::map<std::string, float> metrics_;  ///< R^2, RMSE, MAE
    std::vector<float> residuals_;
  };

  /// @brief Constructor.
  CostModelBuilder();

  /// @brief Add training data point.
  /// @param data_point Data point (features + cost).
  void AddTrainingData(const DataPoint& data_point);

  /// @brief Add batch of training data.
  /// @param data_points Vector of data points.
  void AddTrainingData(const std::vector<DataPoint>& data_points);

  /// @brief Build cost model.
  /// @param model_type Model type ("linear", "polynomial", "ridge", "kernel").
  /// @param regularization_alpha L2 regularization coefficient.
  /// @return Fitted cost model.
  std::unique_ptr<CostModel> BuildModel(
      const std::string& model_type = "linear",
      float regularization_alpha = 0.01f);

  /// @brief Split data for cross-validation.
  /// @param num_folds Number of folds.
  /// @return Vector of (train_data, validation_data) pairs.
  std::vector<std::pair<std::vector<DataPoint>, std::vector<DataPoint>>>
  CrossValidationSplit(uint32_t num_folds = 5);

  /// @brief Get training data statistics.
  /// @return Map of stat_name -> value (min, max, mean, std).
  std::map<std::string, float> GetDatasetStats();

  /// @brief Get feature importance.
  /// @param model Fitted model.
  /// @return Map of variable_name -> importance score.
  std::map<std::string, float> GetFeatureImportance(const CostModel& model);

  /// @brief Validate training data quality.
  /// @return List of data quality issues (empty if OK).
  std::vector<std::string> ValidateData();

 private:
  std::vector<DataPoint> training_data_;
};

}  // namespace themis::rag
