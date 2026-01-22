#include "llm/ml_model_manager.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themis::llm;

class MLModelManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        MLModelManager::Config config;
        config.enable_health_monitoring = false;  // Disable for tests
        config.enable_auto_scaling = false;       // Disable for tests
        
        manager_ = std::make_unique<MLModelManager>(config);
    }
    
    void TearDown() override {
        if (manager_) {
            manager_->shutdown();
        }
    }
    
    std::unique_ptr<MLModelManager> manager_;
};

// ═══════════════════════════════════════════════════════════
// Registration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, RegisterModel_Success) {
    MLModelConfig config;
    config.model_id = "test-classifier";
    config.model_name = "Test Classifier";
    config.version = "1.0";
    config.type = MLModelType::CLASSIFIER;
    config.file_path = "/tmp/model.onnx";
    config.format = "onnx";
    
    auto result = manager_->registerModel(config);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
    
    // Verify registration
    auto models = manager_->listModels();
    ASSERT_EQ(models.size(), 1);
    EXPECT_EQ(models[0], "test-classifier");
}

TEST_F(MLModelManagerTest, RegisterModel_Duplicate) {
    MLModelConfig config;
    config.model_id = "test-model";
    config.model_name = "Test Model";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    
    auto result1 = manager_->registerModel(config);
    ASSERT_TRUE(result1.has_value());
    
    auto result2 = manager_->registerModel(config);
    ASSERT_TRUE(result2.has_error());
    EXPECT_TRUE(result2.error().find("already registered") != std::string::npos);
}

TEST_F(MLModelManagerTest, RegisterMultipleModels) {
    for (int i = 0; i < 5; ++i) {
        MLModelConfig config;
        config.model_id = "model-" + std::to_string(i);
        config.model_name = "Model " + std::to_string(i);
        config.version = "1.0";
        config.type = (i % 2 == 0) ? MLModelType::LLM : MLModelType::CLASSIFIER;
        
        auto result = manager_->registerModel(config);
        ASSERT_TRUE(result.has_value());
    }
    
    auto models = manager_->listModels();
    EXPECT_EQ(models.size(), 5);
}

// ═══════════════════════════════════════════════════════════
// Deployment Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, DeployModel_Success) {
    MLModelConfig config;
    config.model_id = "test-llm";
    config.model_name = "Test LLM";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    config.file_path = "/tmp/model.gguf";
    
    auto reg_result = manager_->registerModel(config);
    ASSERT_TRUE(reg_result.has_value());
    
    auto deploy_result = manager_->deployModel("test-llm", 2);
    ASSERT_TRUE(deploy_result.has_value());
    EXPECT_EQ(deploy_result.value().size(), 2);
    
    // Verify status
    auto status = manager_->getModelStatus("test-llm");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), MLModelStatus::DEPLOYED);
}

TEST_F(MLModelManagerTest, DeployModel_NotRegistered) {
    auto result = manager_->deployModel("nonexistent-model", 1);
    ASSERT_TRUE(result.has_error());
    EXPECT_TRUE(result.error().find("not found") != std::string::npos);
}

TEST_F(MLModelManagerTest, DeployModel_MultipleInstances) {
    MLModelConfig config;
    config.model_id = "multi-instance";
    config.model_name = "Multi Instance Model";
    config.version = "1.0";
    config.type = MLModelType::EMBEDDING;
    
    manager_->registerModel(config);
    
    auto result = manager_->deployModel("multi-instance", 4);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 4);
    
    auto instances = manager_->listModelInstances("multi-instance");
    EXPECT_EQ(instances.size(), 4);
}

// ═══════════════════════════════════════════════════════════
// Inference Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, Inference_Success) {
    // Setup
    MLModelConfig config;
    config.model_id = "inference-test";
    config.model_name = "Inference Test Model";
    config.version = "1.0";
    config.type = MLModelType::CLASSIFIER;
    
    manager_->registerModel(config);
    manager_->deployModel("inference-test", 1);
    
    // Inference
    MLInferenceRequest request;
    request.model_id = "inference-test";
    request.input_data = json{{"text", "test input"}};
    
    auto result = manager_->infer(request);
    ASSERT_TRUE(result.has_value());
    
    auto response = result.value();
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.model_id, "inference-test");
    EXPECT_GT(response.total_time_ms, 0.0f);
}

TEST_F(MLModelManagerTest, Inference_ModelNotDeployed) {
    MLModelConfig config;
    config.model_id = "not-deployed";
    config.model_name = "Not Deployed Model";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    
    manager_->registerModel(config);
    // Don't deploy
    
    MLInferenceRequest request;
    request.model_id = "not-deployed";
    request.input_data = json{{"prompt", "test"}};
    
    auto result = manager_->infer(request);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().success);
}

TEST_F(MLModelManagerTest, InferenceAsync_Success) {
    // Setup
    MLModelConfig config;
    config.model_id = "async-test";
    config.model_name = "Async Test Model";
    config.version = "1.0";
    config.type = MLModelType::REGRESSOR;
    
    manager_->registerModel(config);
    manager_->deployModel("async-test", 1);
    
    // Async inference
    bool callback_called = false;
    MLInferenceResponse received_response;
    
    MLInferenceRequest request;
    request.model_id = "async-test";
    request.input_data = json{{"features", {1.0, 2.0, 3.0}}};
    
    auto request_id = manager_->inferAsync(request, 
        [&callback_called, &received_response](const MLInferenceResponse& response) {
            callback_called = true;
            received_response = response;
        }
    );
    
    EXPECT_FALSE(request_id.empty());
    
    // Wait for callback
    for (int i = 0; i < 50 && !callback_called; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(received_response.success);
}

TEST_F(MLModelManagerTest, InferenceConcurrent_LoadBalancing) {
    // Setup with multiple instances
    MLModelConfig config;
    config.model_id = "load-balanced";
    config.model_name = "Load Balanced Model";
    config.version = "1.0";
    config.type = MLModelType::VISION;
    
    manager_->registerModel(config);
    manager_->deployModel("load-balanced", 3);
    
    // Concurrent inference
    const int num_requests = 30;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < num_requests; ++i) {
        threads.emplace_back([this, &success_count]() {
            MLInferenceRequest request;
            request.model_id = "load-balanced";
            request.input_data = json{{"image", "base64..."}};
            
            auto result = manager_->infer(request);
            if (result.has_value() && result.value().success) {
                success_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count.load(), num_requests);
    
    // Verify load distribution
    auto instances = manager_->listModelInstances("load-balanced");
    for (const auto& inst : instances) {
        EXPECT_GT(inst.total_requests, 0);
    }
}

// ═══════════════════════════════════════════════════════════
// Lifecycle Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, UpdateModel_Success) {
    // Deploy v1
    MLModelConfig config_v1;
    config_v1.model_id = "updateable";
    config_v1.model_name = "Updateable Model";
    config_v1.version = "1.0";
    config_v1.type = MLModelType::LLM;
    config_v1.max_batch_size = 32;
    
    manager_->registerModel(config_v1);
    manager_->deployModel("updateable", 2);
    
    // Update to v2
    MLModelConfig config_v2 = config_v1;
    config_v2.version = "2.0";
    config_v2.max_batch_size = 64;
    
    auto result = manager_->updateModel("updateable", config_v2);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
    
    // Verify update
    auto updated_config = manager_->getModelConfig("updateable");
    ASSERT_TRUE(updated_config.has_value());
    EXPECT_EQ(updated_config.value().version, "2.0");
    EXPECT_EQ(updated_config.value().max_batch_size, 64);
}

TEST_F(MLModelManagerTest, RetireModel_Success) {
    MLModelConfig config;
    config.model_id = "retireable";
    config.model_name = "Retireable Model";
    config.version = "1.0";
    config.type = MLModelType::CLASSIFIER;
    
    manager_->registerModel(config);
    manager_->deployModel("retireable", 1);
    
    auto result = manager_->retireModel("retireable", 1000);
    ASSERT_TRUE(result.has_value());
    
    auto status = manager_->getModelStatus("retireable");
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status.value(), MLModelStatus::RETIRED);
}

TEST_F(MLModelManagerTest, UnregisterModel_Success) {
    MLModelConfig config;
    config.model_id = "removable";
    config.model_name = "Removable Model";
    config.version = "1.0";
    config.type = MLModelType::EMBEDDING;
    
    manager_->registerModel(config);
    manager_->deployModel("removable", 1);
    manager_->retireModel("removable", 1000);
    
    auto result = manager_->unregisterModel("removable");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
    
    auto models = manager_->listModels();
    EXPECT_TRUE(std::find(models.begin(), models.end(), "removable") == models.end());
}

TEST_F(MLModelManagerTest, UnregisterModel_NotRetired) {
    MLModelConfig config;
    config.model_id = "active-model";
    config.model_name = "Active Model";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    
    manager_->registerModel(config);
    manager_->deployModel("active-model", 1);
    
    auto result = manager_->unregisterModel("active-model");
    ASSERT_TRUE(result.has_error());
    EXPECT_TRUE(result.error().find("must be retired") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Scaling Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, ScaleModel_Up) {
    MLModelConfig config;
    config.model_id = "scalable";
    config.model_name = "Scalable Model";
    config.version = "1.0";
    config.type = MLModelType::MULTIMODAL;
    
    manager_->registerModel(config);
    manager_->deployModel("scalable", 2);
    
    auto result = manager_->scaleModel("scalable", 5);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
    
    auto instances = manager_->listModelInstances("scalable");
    EXPECT_EQ(instances.size(), 5);
}

TEST_F(MLModelManagerTest, ScaleModel_Down) {
    MLModelConfig config;
    config.model_id = "scalable-down";
    config.model_name = "Scalable Down Model";
    config.version = "1.0";
    config.type = MLModelType::SPEECH;
    
    manager_->registerModel(config);
    manager_->deployModel("scalable-down", 5);
    
    auto result = manager_->scaleModel("scalable-down", 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
    
    auto instances = manager_->listModelInstances("scalable-down");
    EXPECT_EQ(instances.size(), 2);
}

// ═══════════════════════════════════════════════════════════
// Query and Discovery Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, ListModels_Filter) {
    // Register models of different types
    for (int i = 0; i < 3; ++i) {
        MLModelConfig config;
        config.model_id = "llm-" + std::to_string(i);
        config.model_name = "LLM " + std::to_string(i);
        config.version = "1.0";
        config.type = MLModelType::LLM;
        manager_->registerModel(config);
    }
    
    for (int i = 0; i < 2; ++i) {
        MLModelConfig config;
        config.model_id = "classifier-" + std::to_string(i);
        config.model_name = "Classifier " + std::to_string(i);
        config.version = "1.0";
        config.type = MLModelType::CLASSIFIER;
        manager_->registerModel(config);
    }
    
    // Filter by type
    json filter;
    filter["type"] = static_cast<int>(MLModelType::LLM);
    
    auto llm_models = manager_->listModels(filter);
    EXPECT_EQ(llm_models.size(), 3);
}

TEST_F(MLModelManagerTest, GetModelMetrics) {
    MLModelConfig config;
    config.model_id = "metrics-test";
    config.model_name = "Metrics Test Model";
    config.version = "1.0";
    config.type = MLModelType::CLASSIFIER;
    
    manager_->registerModel(config);
    manager_->deployModel("metrics-test", 2);
    
    // Perform some inferences
    for (int i = 0; i < 10; ++i) {
        MLInferenceRequest request;
        request.model_id = "metrics-test";
        request.input_data = json{{"data", i}};
        manager_->infer(request);
    }
    
    auto metrics = manager_->getModelMetrics("metrics-test");
    EXPECT_TRUE(metrics.contains("model_id"));
    EXPECT_TRUE(metrics.contains("total_requests"));
    EXPECT_TRUE(metrics.contains("successful_requests"));
    EXPECT_EQ(metrics["model_id"], "metrics-test");
    EXPECT_GT(metrics["total_requests"].get<int>(), 0);
}

TEST_F(MLModelManagerTest, GetSystemStats) {
    // Register and deploy multiple models
    for (int i = 0; i < 3; ++i) {
        MLModelConfig config;
        config.model_id = "sys-model-" + std::to_string(i);
        config.model_name = "System Model " + std::to_string(i);
        config.version = "1.0";
        config.type = MLModelType::LLM;
        
        manager_->registerModel(config);
        manager_->deployModel("sys-model-" + std::to_string(i), 2);
    }
    
    auto stats = manager_->getSystemStats();
    EXPECT_TRUE(stats.contains("total_models"));
    EXPECT_TRUE(stats.contains("total_instances"));
    EXPECT_EQ(stats["total_models"].get<int>(), 3);
    EXPECT_EQ(stats["total_instances"].get<int>(), 6);
}

// ═══════════════════════════════════════════════════════════
// Health Check Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, HealthCheck_Success) {
    MLModelConfig config;
    config.model_id = "health-test";
    config.model_name = "Health Test Model";
    config.version = "1.0";
    config.type = MLModelType::VISION;
    config.enable_health_check = true;
    
    manager_->registerModel(config);
    auto deploy_result = manager_->deployModel("health-test", 1);
    ASSERT_TRUE(deploy_result.has_value());
    
    std::string instance_id = deploy_result.value()[0];
    
    bool healthy = manager_->healthCheck(instance_id);
    EXPECT_TRUE(healthy);
}

TEST_F(MLModelManagerTest, RestartInstance_Success) {
    MLModelConfig config;
    config.model_id = "restart-test";
    config.model_name = "Restart Test Model";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    
    manager_->registerModel(config);
    auto deploy_result = manager_->deployModel("restart-test", 1);
    ASSERT_TRUE(deploy_result.has_value());
    
    std::string instance_id = deploy_result.value()[0];
    
    auto result = manager_->restartInstance(instance_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

// ═══════════════════════════════════════════════════════════
// Lifecycle Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, StartStop_Success) {
    manager_->start();
    
    // Manager should be running
    // (We can't directly check running_ but we can verify operations work)
    
    MLModelConfig config;
    config.model_id = "lifecycle-test";
    config.model_name = "Lifecycle Test Model";
    config.version = "1.0";
    config.type = MLModelType::CUSTOM;
    
    auto result = manager_->registerModel(config);
    EXPECT_TRUE(result.has_value());
    
    manager_->shutdown();
    
    // After shutdown, operations should still work but background threads stop
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(MLModelManagerTest, EmptyModelId) {
    MLModelConfig config;
    config.model_id = "";
    config.model_name = "Empty ID Model";
    config.version = "1.0";
    config.type = MLModelType::LLM;
    
    auto result = manager_->registerModel(config);
    // Should succeed but might not be best practice
    EXPECT_TRUE(result.has_value());
}

TEST_F(MLModelManagerTest, ScaleToZero) {
    MLModelConfig config;
    config.model_id = "scale-zero";
    config.model_name = "Scale Zero Model";
    config.version = "1.0";
    config.type = MLModelType::EMBEDDING;
    
    manager_->registerModel(config);
    manager_->deployModel("scale-zero", 3);
    
    auto result = manager_->scaleModel("scale-zero", 0);
    ASSERT_TRUE(result.has_value());
    
    auto instances = manager_->listModelInstances("scale-zero");
    EXPECT_EQ(instances.size(), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
