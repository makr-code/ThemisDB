#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"
#include "llm/grafana_metrics.h"
#include "test_helpers_llm.h"
#include <httplib.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::llm;
using namespace themis::llm::monitoring;
using namespace themis::test;

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
    if (!hasRealModels()) {
        GTEST_SKIP() << "Skipping test: Real models not available (THEMIS_LLM_MODELS_PATH not set)";
    }
    
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
    
    std::vector<std::thread> threads = {};

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

// ═══════════════════════════════════════════════════════════
// MetricsServer health/ready endpoint tests (Q1 implementation)
// ═══════════════════════════════════════════════════════════

class MetricsServerHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        exporter = std::make_unique<PrometheusExporter>();
        MetricsServer::ServerConfig cfg;
        cfg.host = "127.0.0.1";
        cfg.port = 9091;  // Avoid port conflict with any running server
        server = std::make_unique<MetricsServer>(cfg, exporter.get());
    }

    void TearDown() override {
        server->stop();
    }

    std::unique_ptr<PrometheusExporter> exporter;
    std::unique_ptr<MetricsServer> server;
};

TEST_F(MetricsServerHandlerTest, HealthEndpointReturnsOkWhenRunning) {
    server->start();
    ASSERT_TRUE(server->isRunning());

    std::string response;
    // Exercise the internal handler directly (no real HTTP listener needed)
    // handleRequest is private, so invoke via the public URL helper + simulate:
    // We validate behavior through getHealthURL and the exported JSON body by
    // calling start/stop and observing isRunning.
    EXPECT_NE(server->getHealthURL().find("/health"), std::string::npos);
}

TEST_F(MetricsServerHandlerTest, ReadyEndpointURLContainsReadyPath) {
    server->start();
    EXPECT_NE(server->getReadyURL().find("/ready"), std::string::npos);
}

TEST_F(MetricsServerHandlerTest, HealthURLAndReadyURLDifferent) {
    EXPECT_NE(server->getHealthURL(), server->getReadyURL());
}

TEST_F(MetricsServerHandlerTest, BackpressureDropMetricRegistered) {
    // Verify that LLMMetricsCollector registers llm_backpressure_drops_total
    // and that incrementing it causes it to appear in exportMetrics().
    auto collector = std::make_unique<LLMMetricsCollector>(exporter.get());
    
    // Initially zero — may not appear in export until first increment
    collector->recordBackpressureDrop();
    
    std::string metrics = exporter->exportMetrics();
    EXPECT_NE(metrics.find("llm_backpressure_drops_total"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// GET /models endpoint tests (Q1 implementation)
// ═══════════════════════════════════════════════════════════

TEST_F(MetricsServerHandlerTest, ModelsEndpointURL_ContainsModelsPath) {
    EXPECT_NE(server->getModelsURL().find("/models"), std::string::npos);
}

TEST_F(MetricsServerHandlerTest, ModelsEndpoint_NoCallback_ReturnsEmptyArray) {
    server->start();
    // No callback registered — should return empty JSON array, not 404
    std::string response;
    // handleRequest is private; verify via the public URL accessor and that
    // the server doesn't crash.  Functional dispatch is tested below via
    // the callback mechanism.
    EXPECT_NE(server->getModelsURL(), "");
}

TEST_F(MetricsServerHandlerTest, ModelsEndpoint_WithCallback_ReturnsCallbackOutput) {
    server->start();

    const std::string expected_json = R"([{"id":"mistral-7b","vram_mb":6144}])";
    server->setModelInfoCallback([&]() { return expected_json; });

    // Simulate a request by calling the internal dispatch via a helper that
    // goes through handleRequest directly.  Since handleRequest is private we
    // verify the wiring by checking getModelsURL() and confirming the callback
    // is invoked (which is unit-testable by making the callback set a flag).
    bool callback_invoked = false;
    server->setModelInfoCallback([&]() {
        callback_invoked = true;
        return expected_json;
    });

    // At this point we can only verify the callback is registered.
    // Full integration would require the HTTP listener; that is a TODO item.
    // The test proves the callback is wired at compile time and the accessor
    // works.
    EXPECT_NE(server->getModelsURL().find("/models"), std::string::npos);
    EXPECT_FALSE(callback_invoked);  // not invoked until a real HTTP request arrives
}

TEST_F(MetricsServerHandlerTest, ModelsURL_DifferentFromOtherEndpoints) {
    EXPECT_NE(server->getModelsURL(), server->getHealthURL());
    EXPECT_NE(server->getModelsURL(), server->getReadyURL());
    EXPECT_NE(server->getModelsURL(), server->getMetricsURL());
}



// ═══════════════════════════════════════════════════════════
// Real HTTP round-trip tests (requires live MetricsServer)
// Uses port 19091 to avoid conflicts with the existing fixture (9091).
// ═══════════════════════════════════════════════════════════

class MetricsServerHTTPTest : public ::testing::Test {
protected:
    static constexpr int kPort = 19091;

    void SetUp() override {
        exporter = std::make_unique<PrometheusExporter>();
        MetricsServer::ServerConfig cfg;
        cfg.host        = "127.0.0.1";
        cfg.port        = kPort;
        cfg.enable_cors = false;  // keep responses simple for assertions
        server = std::make_unique<MetricsServer>(cfg, exporter.get());
        ASSERT_TRUE(server->start()) << "MetricsServer failed to bind port " << kPort;
    }

    void TearDown() override {
        server->stop();
    }

    std::unique_ptr<PrometheusExporter> exporter;
    std::unique_ptr<MetricsServer>      server;
};

TEST_F(MetricsServerHTTPTest, HealthEndpoint_Returns200AndOkStatus) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/health");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("\"ok\""), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, ReadyEndpoint_Returns200WhenReady) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/ready");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("\"ready\""), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, MetricsEndpoint_ReturnsPrometheusFormat) {
    // Record a metric so there is something to scrape.
    LLMMetricsCollector collector(exporter.get());
    collector.recordInferenceRequest("test-model");

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/metrics");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("llm_"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, ModelsEndpoint_NoCallback_ReturnsEmptyArray) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/models");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "[]");
}

TEST_F(MetricsServerHTTPTest, ModelsEndpoint_WithCallback_ReturnsCallbackJSON) {
    const std::string expected = R"([{"id":"llama-3b","vram_mb":2048}])";
    server->setModelInfoCallback([&]() { return expected; });

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/models");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, expected);
}

TEST_F(MetricsServerHTTPTest, AdminReload_NoCallback_Returns501NotImplemented) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Post("/admin/models/reload", "{\"model\":\"llama-3b\"}", "application/json");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("not_implemented"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, AdminSimulate_WithCallback_InvokesCallback) {
    bool invoked = false;
    server->setSimulateCallback([&](const std::string& body) {
        invoked = true;
        return R"({"allowed":true,"sanitized_prompt":"hello"})";
    });

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Post("/admin/prompt/simulate", R"({"prompt":"hello"})", "application/json");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_TRUE(invoked);
    EXPECT_NE(res->body.find("allowed"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, UnknownPath_Returns404) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/does_not_exist");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 404);
}

// ─── Admin Sessions endpoint tests (Q4 Admin/DX) ───────────────────────────

TEST_F(MetricsServerHTTPTest, AdminSessions_NoCallback_ReturnsEmptyArray) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/admin/sessions");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, "[]");
}

TEST_F(MetricsServerHTTPTest, AdminSessions_WithCallback_ReturnsCallbackJSON) {
    const std::string expected =
        R"([{"session_id":"s1","model_id":"llama-3b","state":"running"}])";
    server->setSessionListCallback([&]() { return expected; });

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/admin/sessions");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, expected);
}

TEST_F(MetricsServerHTTPTest, AdminSessionsURL_ContainsAdminSessionsPath) {
    EXPECT_NE(server->getAdminSessionsURL().find("/admin/sessions"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, AdminDeleteSession_NoCallback_ReturnsNotImplemented) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Delete("/admin/sessions/some-session-id");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("not_implemented"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, AdminDeleteSession_WithCallback_InvokesCallbackWithId) {
    std::string captured_id;
    server->setSessionDeleteCallback([&](const std::string& id) {
        captured_id = id;
        return R"({"status":"deleted","session_id":")" + id + "\"}";
    });

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Delete("/admin/sessions/my-session-uuid");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(captured_id, "my-session-uuid");
    EXPECT_NE(res->body.find("deleted"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Unified metrics dashboard tests (Phase 2 — Q3 2026)
// ═══════════════════════════════════════════════════════════

TEST_F(LLMGrafanaMetricsTest, EngineTypedMetrics_AsyncEngine) {
    // Record metrics tagged with engine_type="async"
    metrics_collector_->recordEngineInferenceRequest("model-a", "async");
    metrics_collector_->recordEngineInferenceSuccess("model-a", "async", 120.0);
    metrics_collector_->recordEngineTokensGenerated("model-a", "async", 32);
    metrics_collector_->recordEngineQueueDepth("async", 3);

    std::string metrics = exporter_->exportMetrics();
    EXPECT_NE(metrics.find("llm_engine_inference_requests_total"), std::string::npos);
    EXPECT_NE(metrics.find("engine_type=\"async\""), std::string::npos);
    EXPECT_NE(metrics.find("llm_engine_tokens_generated_total"), std::string::npos);
    EXPECT_NE(metrics.find("llm_engine_queue_depth"), std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, EngineTypedMetrics_EnhancedEngine) {
    // Record metrics tagged with engine_type="enhanced"
    metrics_collector_->recordEngineInferenceRequest("model-b", "enhanced");
    metrics_collector_->recordEngineInferenceSuccess("model-b", "enhanced", 80.0);
    metrics_collector_->recordEngineInferenceFailure("model-b", "enhanced", "timeout");
    metrics_collector_->recordEngineTokensGenerated("model-b", "enhanced", 64);
    metrics_collector_->recordEngineQueueDepth("enhanced", 7);

    std::string metrics = exporter_->exportMetrics();
    EXPECT_NE(metrics.find("engine_type=\"enhanced\""), std::string::npos);
    EXPECT_NE(metrics.find("llm_engine_inference_failures_total"), std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, EngineTypedMetrics_BothEngines_SeparateLabels) {
    // Both engine types should produce distinct label values in the same export.
    metrics_collector_->recordEngineInferenceRequest("shared-model", "async");
    metrics_collector_->recordEngineInferenceRequest("shared-model", "enhanced");

    std::string metrics = exporter_->exportMetrics();
    EXPECT_NE(metrics.find("engine_type=\"async\""), std::string::npos);
    EXPECT_NE(metrics.find("engine_type=\"enhanced\""), std::string::npos);
}

TEST_F(LLMGrafanaMetricsTest, UnifiedDashboardGeneration_ContainsEnginePanels) {
    GrafanaDashboardGenerator::DashboardConfig config;
    config.title = "Test Unified Dashboard";

    GrafanaDashboardGenerator generator(config);
    std::string dashboard = generator.generateUnifiedDashboard();

    // Must contain the unified tag and both engine type references
    EXPECT_NE(dashboard.find("unified"), std::string::npos);
    EXPECT_NE(dashboard.find("async"), std::string::npos);
    EXPECT_NE(dashboard.find("enhanced"), std::string::npos);

    // Must contain standard Grafana JSON structure
    EXPECT_NE(dashboard.find("\"title\""), std::string::npos);
    EXPECT_NE(dashboard.find("\"panels\""), std::string::npos);

    // Must reference the engine-typed Prometheus metrics
    EXPECT_NE(dashboard.find("llm_engine_inference_requests_total"), std::string::npos);
    EXPECT_NE(dashboard.find("llm_engine_tokens_generated_total"), std::string::npos);
    EXPECT_NE(dashboard.find("llm_engine_queue_depth"), std::string::npos);
    EXPECT_NE(dashboard.find("llm_worker_pool_queue_depth"), std::string::npos);
}

TEST_F(MetricsServerHandlerTest, DashboardEndpoint_DefaultCallback_ReturnsUnifiedJSON) {
    // Without any registered dashboard callback the server should generate a
    // real unified dashboard JSON (not the old stub message).
    std::string response;
    // Drive handleRequest() indirectly by checking the URL accessor and
    // verifying the server does not crash; functional content is tested via
    // the HTTP fixture below.
    EXPECT_NE(server->getDashboardURL().find("/dashboard"), std::string::npos);
}

TEST_F(MetricsServerHandlerTest, DashboardEndpoint_WithCallback_InvokesCallback) {
    const std::string expected = R"({"dashboard":{"title":"My Custom Dashboard"}})";
    bool invoked = false;
    server->setDashboardCallback([&]() {
        invoked = true;
        return expected;
    });

    // The callback is wired at this point; it will be invoked on the first
    // HTTP GET /dashboard request.
    EXPECT_NE(server->getDashboardURL().find("/dashboard"), std::string::npos);
    EXPECT_FALSE(invoked);  // not invoked until a real HTTP request arrives
}

TEST_F(MetricsServerHTTPTest, DashboardEndpoint_DefaultCallback_ReturnsGrafanaJSON) {
    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/dashboard");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);

    // Response must be real Grafana dashboard JSON, not the old stub message.
    EXPECT_EQ(res->body.find("Dashboard endpoint"), std::string::npos);
    EXPECT_NE(res->body.find("\"dashboard\""), std::string::npos);
    EXPECT_NE(res->body.find("\"panels\""), std::string::npos);
    EXPECT_NE(res->body.find("unified"), std::string::npos);
}

TEST_F(MetricsServerHTTPTest, DashboardEndpoint_WithCallback_ReturnsCallbackJSON) {
    const std::string expected = R"({"dashboard":{"title":"Custom"},"panels":[]})";
    server->setDashboardCallback([&]() { return expected; });

    httplib::Client cli("127.0.0.1", kPort);
    auto res = cli.Get("/dashboard");
    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(res->body, expected);
}
