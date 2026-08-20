#include <gtest/gtest.h>

#include "llm/llm_plugin_interface.h"
#include "llm/prompt_policy.h"
#include "llm/subagent.h"
#include "llm/subagent_coordinator.h"
#include "llm/subagent_factory.h"

#include <future>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace themis::llm {

class MockLLMPlugin final : public ILLMPlugin {
public:
    bool loadModel(const std::string& model_path, const json& = {}) override {
        model_info_.emplace();
        model_info_->name = model_path;
        model_info_->path = model_path;
        model_info_->model_id = model_path;
        model_info_->is_loaded = true;
        return true;
    }

    void unloadModel() override { model_info_.reset(); }
    std::optional<ModelInfo> getModelInfo() const override { return model_info_; }
    bool isModelLoaded() const override { return model_info_.has_value(); }
    bool loadLoRA(const std::string& lora_id, const std::string& lora_path, float scale = 1.0f) override {
        LoRAInfo info;
        info.id = lora_id;
        info.path = lora_path;
        info.scale = scale;
        info.is_loaded = true;
        loras_.push_back(info);
        return true;
    }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return loras_; }
    InferenceResponse generate(const InferenceRequest& request) override {
        InferenceResponse response;
        response.success = true;
        response.model_id = request.model_id;
        response.text = request.prompt;
        response.tokens_generated = 1;
        return response;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& request) override { return generate(request); }
    std::vector<float> embed(const std::string&) override { return {0.25f, 0.5f, 0.75f}; }
    LLMCapabilities getCapabilities() const override {
        LLMCapabilities caps;
        caps.supports_completion = true;
        caps.supports_embeddings = true;
        return caps;
    }
    json getMemoryStats() const override { return json::object(); }
    json getPerformanceStats() const override { return json::object(); }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

private:
    std::optional<ModelInfo> model_info_;
    std::vector<LoRAInfo> loras_;
};

class SubagentContractTest : public ::testing::Test {
protected:
    void SetUp() override { plugin = std::make_shared<MockLLMPlugin>(); }

    std::shared_ptr<MockLLMPlugin> plugin;
};

TEST_F(SubagentContractTest, PluginModelAndLoRAContract) {
    EXPECT_TRUE(plugin->loadModel("mock-model"));
    ASSERT_TRUE(plugin->getModelInfo().has_value());
    EXPECT_EQ(plugin->getModelInfo()->model_id, "mock-model");
    EXPECT_TRUE(plugin->loadLoRA("lora-1", "path/to/lora", 0.75f));
    EXPECT_EQ(plugin->listLoRAs().size(), 1u);
    EXPECT_TRUE(plugin->unloadLoRA("lora-1"));
}

TEST(SubagentContract, FactoryAndSubagentContractsCompile) {
    using FactoryCreateResult = decltype(SubagentFactory::create(
        std::declval<ILLMPlugin*>(),
        std::declval<std::shared_ptr<SharedWorkerPool>>(),
        std::declval<std::shared_ptr<ModelLoader>>(),
        std::declval<std::shared_ptr<MultiLoRAManager>>(),
        std::declval<std::shared_ptr<TokenQuotaManager>>(),
        std::declval<const SubagentFactory::Config&>()));

    using CreateSubagentResult = decltype(std::declval<SubagentFactory&>().createSubagent(std::declval<const SubagentConfig&>()));
    using DestroySubagentResult = decltype(std::declval<SubagentFactory&>().destroySubagent(std::declval<const std::string&>(), 30000));
    using StateResult = decltype(std::declval<SubagentFactory&>().getSubagentState(std::declval<const std::string&>()));
    using MetricsResult = decltype(std::declval<SubagentFactory&>().getSubagentMetrics(std::declval<const std::string&>()));
    using RegisterPolicyResult = decltype(std::declval<SubagentFactory&>().registerPromptPolicy(std::declval<const std::string&>(), std::declval<std::shared_ptr<PromptPolicy>>()));
    using UnregisterPolicyResult = decltype(std::declval<SubagentFactory&>().unregisterPromptPolicy(std::declval<const std::string&>()));
    using CoordinatorCreateResult = decltype(SubagentCoordinator::create(std::declval<std::shared_ptr<SubagentFactory>>()));
    using InferMultipleResult = decltype(std::declval<SubagentCoordinator&>().inferMultiple(
        std::declval<const std::vector<std::string>&>(),
        std::declval<const InferenceRequest&>(),
        std::declval<const SubagentCoordinatorConfig&>()));

    static_assert(std::is_same_v<FactoryCreateResult, SubagentResult<std::unique_ptr<SubagentFactory>>>);
    static_assert(std::is_same_v<CreateSubagentResult, SubagentResult<std::shared_ptr<Subagent>>>);
    static_assert(std::is_same_v<DestroySubagentResult, SubagentResult<void>>);
    static_assert(std::is_same_v<StateResult, SubagentResult<SubagentState>>);
    static_assert(std::is_same_v<MetricsResult, SubagentResult<SubagentMetrics>>);
    static_assert(std::is_same_v<RegisterPolicyResult, SubagentResult<void>>);
    static_assert(std::is_same_v<UnregisterPolicyResult, SubagentResult<void>>);
    static_assert(std::is_same_v<CoordinatorCreateResult, SubagentResult<std::unique_ptr<SubagentCoordinator>>>);
    static_assert(std::is_same_v<InferMultipleResult, SubagentCoordinatorAggregateResult>);
}

TEST(SubagentContract, StateAndResultTypes) {
    EXPECT_STREQ(subagentStateToString(SubagentState::CREATED), "CREATED");
    EXPECT_STREQ(subagentStateToString(SubagentState::READY), "READY");
    EXPECT_STREQ(subagentStateToString(SubagentState::TERMINATED), "TERMINATED");

    SubagentMetrics metrics;
    EXPECT_EQ(metrics.total_requests, 0u);
    EXPECT_EQ(metrics.successful_inferences, 0u);
}

} // namespace themis::llm

#if 0
    auto check = subagent_->checkQuota(1);
    EXPECT_FALSE(check.allowed);
}

// SO-35: Concurrent requests from different subagents
TEST_F(SubagentIsolationTest, ConcurrentRequests) {
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &success_count]() {
            InferenceRequest req;
            req.prompt = "Test";
            auto result = subagent_->infer(req);
            if (result.success) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GT(success_count, 0);
}

// SO-36: Policy isolation
TEST_F(SubagentIsolationTest, PolicyIsolation) {
    auto config = subagent_->config();
    EXPECT_EQ(config.isolation_level, SubagentIsolationLevel::STRICT);
}

// SO-37: Resource limits per subagent
TEST_F(SubagentIsolationTest, ResourceLimits) {
    auto config = subagent_->config();
    EXPECT_LE(config.budget.max_tokens_per_minute, 1000000);  // Reasonable limit
}

// SO-38: Metrics are per-subagent
TEST_F(SubagentIsolationTest, PerSubagentMetrics) {
    InferenceRequest req;
    req.prompt = "Test";
    subagent_->infer(req);

    auto metrics = subagent_->getMetrics();
    EXPECT_EQ(metrics.total_requests, 1);
}

// SO-39: Quota window reset
TEST_F(SubagentIsolationTest, QuotaWindowReset) {
    subagent_->resetQuota();
    auto check = subagent_->checkQuota(50);
    EXPECT_TRUE(check.allowed);
}

// SO-40: Correlation context propagation
TEST_F(SubagentIsolationTest, CorrelationContext) {
    InferenceRequest req;
    req.prompt = "Test";

    LLMCorrelationContext ctx;
    ctx.trace_id = "test-trace-12345678901234567890ab";
    ctx.span_id = "test-span-1234567890abcd";

    auto result = subagent_->infer(req, ctx);
    EXPECT_EQ(result.trace_id, ctx.trace_id);
}

#endif
