#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"
#include "llm/grafana_metrics.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm;
using namespace themis::llm::monitoring;

class LLMGrafanaMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Prometheus exporter
        exporter_ = std::make_unique<PrometheusExporter>();
        
        // Initialize metrics collector
        metrics_collector_ = std::make_unique<LLMMetricsCollector>(exporter_.get());
        
        // Configure LlamaWrapper with COMPLETE config initialization
        LlamaWrapper::Config config;
        config.n_gpu_layers = 0;  // CPU only for testing
        config.n_ctx = 512;
        config.n_batch = 32;
        config.n_threads = 4;
        
        // Disable features that might cause crashes in test environment
        config.use_kv_cache_reuse = false;
        config.enable_response_cache = false;
        config.enable_output_validation = false;
        
        // Create wrapper
        try {
            wrapper_ = std::make_unique<LlamaWrapper>(config);
            
            // Set metrics collector
            if (wrapper_) {
                wrapper_->setMetricsCollector(metrics_collector_.get());
            }
        } catch (const std::exception& e) {
            spdlog::warn("LlamaWrapper initialization failed (expected in test): {}", e.what());
            // Tests should still run with stubbed wrapper
        }
    }
    
    void TearDown() override {
        wrapper_.reset();
        metrics_collector_.reset();
        exporter_.reset();
    }
    
    std::unique_ptr<PrometheusExporter> exporter_;
    std::unique_ptr<LLMMetricsCollector> metrics_collector_;
    std::unique_ptr<LlamaWrapper> wrapper_;
};

TEST_F(LLMGrafanaMetricsTest, MetricsRecordInferenceRequest) {
    // Load a stub model
    wrapper_->loadModel("test_model.gguf");
    
    // Create inference request
    InferenceRequest request;
    request.request_id = "test-001";
    request.prompt = "Hello, world!";
    request.max_tokens = 10;
    
    // Generate response (this will use stub since no real model is loaded)
    auto response = wrapper_->generate(request);
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify metrics contain expected entries
    EXPECT_TRUE(metrics.find("llm_inference_requests_total") != std::string::npos);
    EXPECT_TRUE(metrics.find("llm_inference_duration_ms") != std::string::npos);
    EXPECT_TRUE(metrics.find("llm_tokens_generated_total") != std::string::npos);
    EXPECT_TRUE(metrics.find("model_id") != std::string::npos);
    
    // Verify response is valid
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GE(response.inference_time_ms, 0.0f);
}

TEST_F(LLMGrafanaMetricsTest, MetricsRecordModelLoading) {
    // Load model
    bool loaded = wrapper_->loadModel("test_model_v2.gguf");
    EXPECT_TRUE(loaded);
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify model loaded metrics
    EXPECT_TRUE(metrics.find("llm_models_loaded") != std::string::npos);
    EXPECT_TRUE(metrics.find("llm_model_memory_mb") != std::string::npos);
    
    // Unload model
    wrapper_->unloadModel();
    
    // Export metrics again
    metrics = exporter_->exportMetrics();
    
    // Model should still be in metrics (with 0 memory after unload)
    EXPECT_TRUE(metrics.find("llm_model_memory_mb") != std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, MetricsRecordMultipleInferences) {
    // Load model
    wrapper_->loadModel("multi_test_model.gguf");
    
    // Perform multiple inferences
    const int num_requests = 5;
    for (int i = 0; i < num_requests; ++i) {
        InferenceRequest request;
        request.request_id = "test-" + std::to_string(i);
        request.prompt = "Inference test " + std::to_string(i);
        request.max_tokens = 10;
        
        auto response = wrapper_->generate(request);
        EXPECT_FALSE(response.text.empty());
    }
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify multiple requests were recorded
    // The counter should show accumulation
    EXPECT_TRUE(metrics.find("llm_inference_requests_total") != std::string::npos);
    EXPECT_TRUE(metrics.find("llm_tokens_generated_total") != std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, MetricsRecordErrors) {
    // Try to generate without loading a model (should fail)
    InferenceRequest request;
    request.prompt = "This should fail";
    request.max_tokens = 10;
    
    EXPECT_THROW({
        wrapper_->generate(request);
    }, std::runtime_error);
    
    // Note: Error metrics would be recorded if we had proper error handling
    // For now, just verify the test doesn't crash
}

TEST_F(LLMGrafanaMetricsTest, MetricsExportFormat) {
    // Record some test metrics manually
    metrics_collector_->recordInferenceRequest("test_model");
    metrics_collector_->recordInferenceSuccess("test_model", 125.5);
    metrics_collector_->recordTokensGenerated("test_model", 42);
    metrics_collector_->recordFirstTokenLatency("test_model", 35.2);
    
    // Export metrics
    std::string metrics = exporter_->exportMetrics();
    
    // Verify Prometheus format
    EXPECT_TRUE(metrics.find("# HELP") != std::string::npos);
    EXPECT_TRUE(metrics.find("# TYPE") != std::string::npos);
    EXPECT_TRUE(metrics.find("counter") != std::string::npos || 
                metrics.find("histogram") != std::string::npos);
    
    // Verify metrics have labels
    EXPECT_TRUE(metrics.find("{model_id=\"test_model\"}") != std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, MetricsThreadSafety) {
    // Load model
    wrapper_->loadModel("concurrent_test_model.gguf");
    
    // Launch multiple threads performing inferences
    const int num_threads = 4;
    const int requests_per_thread = 3;
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, requests_per_thread]() {
            for (int i = 0; i < requests_per_thread; ++i) {
                InferenceRequest request;
                request.request_id = "thread-" + std::to_string(t) + "-" + std::to_string(i);
                request.prompt = "Concurrent test";
                request.max_tokens = 5;
                
                try {
                    auto response = wrapper_->generate(request);
                } catch (...) {
                    // Ignore errors in concurrent test
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Export metrics - should not crash
    std::string metrics = exporter_->exportMetrics();
    EXPECT_FALSE(metrics.empty());
}

TEST_F(LLMGrafanaMetricsTest, GrafanaDashboardGeneration) {
    // Test dashboard generation
    GrafanaDashboardGenerator::DashboardConfig config;
    config.title = "Test LLM Dashboard";
    config.refresh_interval_sec = 5;
    
    GrafanaDashboardGenerator generator(config);
    
    // Generate dashboard JSON
    std::string dashboard = generator.generateDashboard();
    
    // Verify JSON structure
    EXPECT_TRUE(dashboard.find("\"title\"") != std::string::npos);
    EXPECT_TRUE(dashboard.find("\"panels\"") != std::string::npos);
    EXPECT_TRUE(dashboard.find("Inference") != std::string::npos);
    EXPECT_TRUE(dashboard.find("Latency") != std::string::npos);
    EXPECT_TRUE(dashboard.find("GPU") != std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, MetricsReset) {
    // Record some metrics
    metrics_collector_->recordInferenceRequest("test_model");
    metrics_collector_->recordTokensGenerated("test_model", 100);
    
    // Verify metrics exist
    std::string metrics_before = exporter_->exportMetrics();
    EXPECT_FALSE(metrics_before.empty());
    
    // Reset metrics
    exporter_->reset();
    
    // Verify metrics are cleared
    std::string metrics_after = exporter_->exportMetrics();
    
    // After reset, output should be minimal (only HELP and TYPE headers if any)
    // or empty depending on implementation
    EXPECT_TRUE(metrics_after.empty() || 
                metrics_after.size() < metrics_before.size());
}
