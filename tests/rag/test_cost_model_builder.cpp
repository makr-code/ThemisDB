// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <vector>

#include "rag/cost_model_builder.h"

namespace themis::rag::testing {

class CostModelBuilderTest : public ::testing::Test {
 protected:
  CostModelBuilder builder_;
};

TEST_F(CostModelBuilderTest, AddTrainingData) {
  CostModelBuilder::DataPoint data;
  data.features["num_retrieved"] = 50.0f;
  data.cost_usd = 0.10f;
  
  builder_.AddTrainingData(data);
  auto stats = builder_.GetDatasetStats();
  
  EXPECT_EQ(stats["num_samples"], 1.0f);
}

TEST_F(CostModelBuilderTest, AddMultipleDataPoints) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 5; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f + i * 10;
    point.cost_usd = 0.10f + i * 0.02f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto stats = builder_.GetDatasetStats();
  
  EXPECT_EQ(stats["num_samples"], 5.0f);
}

TEST_F(CostModelBuilderTest, BuildLinearModel) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f + i * 10;
    point.cost_usd = 0.10f + i * 0.02f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto model = builder_.BuildModel("linear", 0.1f);
  
  EXPECT_NE(model, nullptr);
  auto coeffs = model->GetCoefficients();
  EXPECT_GT(coeffs.size(), 0);
}

TEST_F(CostModelBuilderTest, ModelPrediction) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f;
    point.cost_usd = 0.10f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto model = builder_.BuildModel("linear", 0.1f);
  
  float prediction = model->Predict({{"num_retrieved", 50.0f}});
  EXPECT_GT(prediction, 0.0f);
}

TEST_F(CostModelBuilderTest, ModelEvaluation) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f;
    point.cost_usd = 0.10f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto model = builder_.BuildModel("linear", 0.1f);
  
  float rmse = model->Evaluate(data);
  EXPECT_GE(rmse, 0.0f);
}

TEST_F(CostModelBuilderTest, CrossValidationSplit) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f;
    point.cost_usd = 0.10f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto splits = builder_.CrossValidationSplit(5);
  
  EXPECT_EQ(splits.size(), 5);
}

TEST_F(CostModelBuilderTest, DatasetStats) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 5; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f + i * 10;
    point.cost_usd = 0.10f + i * 0.02f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto stats = builder_.GetDatasetStats();
  
  EXPECT_EQ(stats["num_samples"], 5.0f);
  EXPECT_GT(stats["mean_cost"], 0.0f);
  EXPECT_GT(stats["std_dev_cost"], 0.0f);
}

TEST_F(CostModelBuilderTest, FeatureImportance) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f;
    point.cost_usd = 0.10f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto model = builder_.BuildModel("linear", 0.1f);
  
  auto importance = builder_.GetFeatureImportance(*model);
  EXPECT_GE(importance.size(), 0);
}

TEST_F(CostModelBuilderTest, ValidateData) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 5; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f + i * 10;
    point.cost_usd = 0.10f + i * 0.02f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto issues = builder_.ValidateData();
  
  EXPECT_EQ(issues.size(), 0);
}

TEST_F(CostModelBuilderTest, MultipleFeatures) {
  std::vector<CostModelBuilder::DataPoint> data;
  for (int i = 0; i < 10; i++) {
    CostModelBuilder::DataPoint point;
    point.features["num_retrieved"] = 50.0f + i * 10;
    point.features["rerank_fraction"] = 0.5f + i * 0.05f;
    point.cost_usd = 0.10f + i * 0.02f;
    data.push_back(point);
  }
  
  builder_.AddTrainingData(data);
  auto model = builder_.BuildModel("linear", 0.1f);
  
  auto coeffs = model->GetCoefficients();
  EXPECT_GT(coeffs.size(), 1);
}

}  // namespace themis::rag::testing
