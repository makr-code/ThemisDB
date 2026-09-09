/**
 * @file test_llm_federated_failure_envelopes.cpp
 * @brief Focused regression tests for fan-out failure envelopes and telemetry metadata.
 */

#include <gtest/gtest.h>

#include "llm/inference_engine_enhanced.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

using namespace themis::llm;

namespace {

class FastMockPlugin final : public ILLMPlugin {
public:
    std::atomic<int> calls{0};

    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id = "local-model";
        info.name = "local-model";
        info.is_loaded = true;
        return info;
    }
    InferenceResponse generate(const InferenceRequest& req) override {
        calls.fetch_add(1, std::memory_order_relaxed);
        InferenceResponse resp;
        resp.success = true;
        resp.request_id = req.request_id;
        resp.model_id = "local-model";
        resp.text = "local";
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
};

class StubFederatedBackend final : public IFederatedInferenceBackend {
public:
    std::vector<FanOutInstanceResult> next_results;

    std::vector<FanOutInstanceResult> execute(
        const std::vector<std::string>&,
        const InferenceRequest&) override {
        return next_results;
    }
};

FanOutInstanceResult makeResult(const std::string& instance_id,
                                bool success,
                                const std::string& error_code,
                                const std::string& error,
                                int attempts,
                                int64_t dispatch_time_ms,
                                InferenceResponse response = {}) {
    FanOutInstanceResult result;
    result.instance_id = instance_id;
    result.response = std::move(response);
    result.success = success;
    result.error_code = error_code;
    result.error = error;
    result.attempts = attempts;
    result.dispatch_time_ms = dispatch_time_ms;
    return result;
}

InferenceEngineEnhanced::EnhancedInferenceRequest makeFanoutRequest(
    const std::string& request_id) {
    InferenceEngineEnhanced::EnhancedInferenceRequest req;
    req.request_id = request_id;
    req.base_request.request_id = request_id;
    req.base_request.prompt = "fanout";
    req.base_request.max_tokens = 16;
    req.target_instance_ids = {"shard-a", "shard-b"};
    req.allow_caching = false;
    return req;
}

}  // namespace

TEST(LLMFederatedFailureEnvelopes, AllFailedFanOutReturnsFailureEnvelopeMetadata) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_context_caching = false;
    cfg.enable_batch_processing = false;
    cfg.num_worker_threads = 1;

    InferenceEngineEnhanced engine(cfg);
    auto local_plugin = std::make_shared<FastMockPlugin>();
    engine.registerModel("local-model", local_plugin);

    auto backend = std::make_shared<StubFederatedBackend>();
    backend->next_results = {
        makeResult("shard-a", false, "LLM_FANOUT_TIMEOUT", "timeout", 2, 120),
        makeResult("shard-b", false, "LLM_FANOUT_RETRY_EXHAUSTED", "retry exhausted", 3, 210),
    };
    engine.setFederatedBackend(backend);
    engine.start();

    auto resp = engine.submit(makeFanoutRequest("fanout-all-fail")).get();

    EXPECT_FALSE(resp.success);
    EXPECT_NE(resp.error_message.find("LLM_FANOUT_TIMEOUT"), std::string::npos);
    EXPECT_NE(resp.error_message.find("LLM_FANOUT_RETRY_EXHAUSTED"), std::string::npos);
    ASSERT_TRUE(resp.metadata.is_object());
    EXPECT_EQ(resp.metadata.value("fan_out_total", 0), 2);
    EXPECT_EQ(resp.metadata.value("fan_out_success_count", 0), 0);
    EXPECT_EQ(resp.metadata.value("fan_out_failure_count", 0), 2);
    EXPECT_EQ(resp.metadata.value("fan_out_total_attempts", 0), 5);
    EXPECT_EQ(resp.metadata.value("fan_out_failure_class", std::string{}),
              "LLM_FANOUT_ALL_FAILED");
    ASSERT_TRUE(resp.metadata.contains("fan_out_failure_envelope"));
    EXPECT_EQ(resp.metadata["fan_out_failure_envelope"].size(), 2u);
    EXPECT_EQ(local_plugin->calls.load(std::memory_order_relaxed), 0);

    engine.shutdown();
}

TEST(LLMFederatedFailureEnvelopes, PartialFailureFanOutSuccessKeepsTelemetryMetadata) {
    InferenceEngineEnhanced::Config cfg;
    cfg.enable_context_caching = false;
    cfg.enable_batch_processing = false;
    cfg.num_worker_threads = 1;

    InferenceEngineEnhanced engine(cfg);
    auto local_plugin = std::make_shared<FastMockPlugin>();
    engine.registerModel("local-model", local_plugin);

    InferenceResponse remote_success;
    remote_success.success = true;
    remote_success.text = "remote-ok";
    remote_success.model_id = "remote-model";

    auto backend = std::make_shared<StubFederatedBackend>();
    backend->next_results = {
        makeResult("shard-a", false, "LLM_FANOUT_REMOTE_PERMANENT", "http 403", 1, 11),
        makeResult("shard-b", true, "", "", 2, 35, remote_success),
    };
    engine.setFederatedBackend(backend);
    engine.start();

    auto resp = engine.submit(makeFanoutRequest("fanout-partial")).get();

    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.text, "remote-ok");
    ASSERT_TRUE(resp.metadata.is_object());
    EXPECT_EQ(resp.metadata.value("fan_out_total", 0), 2);
    EXPECT_EQ(resp.metadata.value("fan_out_success_count", 0), 1);
    EXPECT_EQ(resp.metadata.value("fan_out_failure_count", 0), 1);
    EXPECT_EQ(resp.metadata.value("fan_out_total_attempts", 0), 3);
    EXPECT_EQ(resp.metadata.value("fan_out_partial_failure", false), true);
    ASSERT_TRUE(resp.metadata.contains("fan_out_failure_envelope"));
    EXPECT_EQ(resp.metadata["fan_out_failure_envelope"].size(), 1u);
    EXPECT_EQ(local_plugin->calls.load(std::memory_order_relaxed), 0);

    engine.shutdown();
}
