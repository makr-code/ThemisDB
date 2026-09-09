#include <gtest/gtest.h>

#include "llm/llm_plugin_interface.h"
#include "llm/multi_lora_manager.h"
#include "llm/prompt_policy.h"
#include "llm/shared_worker_pool.h"
#include "llm/subagent.h"
#include "llm/subagent_coordinator.h"
#include "llm/subagent_factory.h"

#include <future>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace themis::llm {

class ModelLoader {};

class MockLLMPlugin final : public ILLMPlugin {
public:
    bool loadModel(const std::string& model_path, const json& = {}) override {
        if (fail_load_model) {
            return false;
        }
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
        if (fail_load_lora) {
            return false;
        }
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
        const bool model_marked_failure =
            fail_model_ids.find(request.model_id) != fail_model_ids.end();
        response.success = !fail_generate && !model_marked_failure;
        response.model_id = request.model_id;
        auto output_it = per_model_output.find(request.model_id);
        if (echo_model_id_in_output) {
            response.text = response.success
                ? (request.model_id + ":" +
                   (output_it != per_model_output.end() ? output_it->second : request.prompt))
                : std::string();
        } else {
            response.text = response.success
                ? (output_it != per_model_output.end() ? output_it->second : request.prompt)
                : std::string();
        }
        response.error_message = response.success ? "" : "mock_generate_failed";
        response.tokens_generated = 1;
        response.tokens_prompt = 1;
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

    bool fail_load_model = false;
    bool fail_load_lora = false;
    bool fail_generate = false;
    bool echo_model_id_in_output = false;
    std::unordered_set<std::string> fail_model_ids;
    std::unordered_map<std::string, std::string> per_model_output;

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

TEST(SubagentContract, RuntimeLifecycleAndInference) {
    auto plugin = std::make_shared<MockLLMPlugin>();
    auto worker_pool = std::make_shared<SharedWorkerPool>();
    auto model_loader = std::make_shared<ModelLoader>();
    auto lora_manager = std::make_shared<MultiLoRAManager>(MultiLoRAManager::Config{});
    auto quota_manager = std::make_shared<TokenQuotaManager>();

    SubagentFactory::Config factory_cfg;
    auto factory_result = SubagentFactory::create(
        plugin.get(), worker_pool, model_loader, lora_manager, quota_manager, factory_cfg);
    ASSERT_TRUE(factory_result);
    auto factory = std::shared_ptr<SubagentFactory>(std::move(factory_result.value()));

    SubagentConfig cfg;
    cfg.id = "runtime-subagent";
    cfg.model_id = "runtime-model";
    cfg.budget.max_tokens_per_request = 16;
    cfg.budget.timeout_ms = 2000;

    auto create_result = factory->createSubagent(cfg);
    ASSERT_TRUE(create_result);
    auto subagent = create_result.value();

    EXPECT_TRUE(subagent->load(1000));
    EXPECT_EQ(subagent->getState(), SubagentState::READY);
    EXPECT_TRUE(subagent->warm(1000));

    InferenceRequest req;
    req.prompt = "hello-runtime";
    req.max_tokens = 32;
    auto infer_result = subagent->infer(req);
    EXPECT_TRUE(infer_result.success);
    EXPECT_EQ(infer_result.output, "hello-runtime");
    EXPECT_GT(infer_result.tokens_consumed, 0u);

    subagent->resetQuota();
    auto quota = subagent->checkQuota(8);
    EXPECT_TRUE(quota.allowed);

    EXPECT_TRUE(subagent->unload(1000));
    EXPECT_EQ(subagent->getState(), SubagentState::TERMINATED);
}

TEST(SubagentContract, LoadAndInferFailurePathsAreDeterministic) {
    auto plugin = std::make_shared<MockLLMPlugin>();
    auto worker_pool = std::make_shared<SharedWorkerPool>();
    auto model_loader = std::make_shared<ModelLoader>();
    auto lora_manager = std::make_shared<MultiLoRAManager>(MultiLoRAManager::Config{});
    auto quota_manager = std::make_shared<TokenQuotaManager>();

    auto factory_result = SubagentFactory::create(
        plugin.get(), worker_pool, model_loader, lora_manager, quota_manager, {});
    ASSERT_TRUE(factory_result);
    auto factory = std::shared_ptr<SubagentFactory>(std::move(factory_result.value()));

    SubagentConfig cfg;
    cfg.id = "failure-subagent";
    cfg.model_id = "runtime-model";

    auto create_result = factory->createSubagent(cfg);
    ASSERT_TRUE(create_result);
    auto subagent = create_result.value();

    plugin->fail_load_model = true;
    auto load_result = subagent->load(1000);
    ASSERT_FALSE(load_result);
    EXPECT_NE(load_result.error().find("subagent_load_model_failed"), std::string::npos);

    InferenceRequest infer_before_ready_request;
    infer_before_ready_request.prompt = "test";
    auto infer_before_ready = subagent->infer(infer_before_ready_request);
    EXPECT_FALSE(infer_before_ready.success);
    EXPECT_NE(infer_before_ready.error.find("subagent_infer_state_not_ready"), std::string::npos);

    SubagentConfig cfg2;
    cfg2.id = "infer-failure-subagent";
    cfg2.model_id = "runtime-model";
    auto create_result2 = factory->createSubagent(cfg2);
    ASSERT_TRUE(create_result2);
    auto subagent2 = create_result2.value();

    plugin->fail_load_model = false;
    ASSERT_TRUE(subagent2->load(1000));
    plugin->fail_generate = true;
    InferenceRequest infer_fail_request;
    infer_fail_request.prompt = "test";
    auto infer_fail = subagent2->infer(infer_fail_request);
    EXPECT_FALSE(infer_fail.success);
    EXPECT_NE(infer_fail.error.find("subagent_infer_runtime_failed"), std::string::npos);
}

TEST(SubagentCoordinatorContract, MajorityVoteUsesSemanticAggregation) {
    auto plugin = std::make_shared<MockLLMPlugin>();
    plugin->per_model_output["m1"] = R"({"answer":"Paris"})";
    plugin->per_model_output["m2"] = R"({ "answer" : "Paris" })";
    plugin->per_model_output["m3"] = R"({"answer":"Berlin"})";

    auto worker_pool = std::make_shared<SharedWorkerPool>();
    auto model_loader = std::make_shared<ModelLoader>();
    auto lora_manager = std::make_shared<MultiLoRAManager>(MultiLoRAManager::Config{});
    auto quota_manager = std::make_shared<TokenQuotaManager>();

    auto factory_result = SubagentFactory::create(
        plugin.get(), worker_pool, model_loader, lora_manager, quota_manager, {});
    ASSERT_TRUE(factory_result);
    auto factory = std::shared_ptr<SubagentFactory>(std::move(factory_result.value()));

    for (const auto& entry : {std::pair{"s1", "m1"}, std::pair{"s2", "m2"}, std::pair{"s3", "m3"}}) {
        SubagentConfig cfg;
        cfg.id = entry.first;
        cfg.model_id = entry.second;
        auto created = factory->createSubagent(cfg);
        ASSERT_TRUE(created);
        ASSERT_TRUE(created.value()->load(1000));
    }

    auto coordinator_result = SubagentCoordinator::create(factory);
    ASSERT_TRUE(coordinator_result);
    auto coordinator = std::move(coordinator_result.value());

    SubagentCoordinatorConfig coord_cfg;
    coord_cfg.strategy = SubagentMergeStrategy::MAJORITY_VOTE;
    InferenceRequest request;
    request.prompt = "capital";
    auto aggregate = coordinator->inferMultiple({"s1", "s2", "s3"}, request, coord_cfg);

    ASSERT_TRUE(aggregate.success);
    EXPECT_EQ(aggregate.num_successful, 3u);
    EXPECT_EQ(nlohmann::json::parse(aggregate.merged_output).value("answer", ""), "Paris");
    EXPECT_NE(aggregate.summary.find("merge=majority_vote"), std::string::npos);
}

TEST(SubagentCoordinatorContract, BestScoreHandlesPartialFailures) {
    auto plugin = std::make_shared<MockLLMPlugin>();
    plugin->per_model_output["m1"] = "short";
    plugin->per_model_output["m2"] = "this is a significantly longer answer for scoring";
    plugin->per_model_output["m3"] = "ignored";
    plugin->fail_model_ids.insert("m3");

    auto worker_pool = std::make_shared<SharedWorkerPool>();
    auto model_loader = std::make_shared<ModelLoader>();
    auto lora_manager = std::make_shared<MultiLoRAManager>(MultiLoRAManager::Config{});
    auto quota_manager = std::make_shared<TokenQuotaManager>();

    auto factory_result = SubagentFactory::create(
        plugin.get(), worker_pool, model_loader, lora_manager, quota_manager, {});
    ASSERT_TRUE(factory_result);
    auto factory = std::shared_ptr<SubagentFactory>(std::move(factory_result.value()));

    for (const auto& entry : {std::pair{"s1", "m1"}, std::pair{"s2", "m2"}, std::pair{"s3", "m3"}}) {
        SubagentConfig cfg;
        cfg.id = entry.first;
        cfg.model_id = entry.second;
        auto created = factory->createSubagent(cfg);
        ASSERT_TRUE(created);
        ASSERT_TRUE(created.value()->load(1000));
    }

    auto coordinator_result = SubagentCoordinator::create(factory);
    ASSERT_TRUE(coordinator_result);
    auto coordinator = std::move(coordinator_result.value());

    SubagentCoordinatorConfig coord_cfg;
    coord_cfg.strategy = SubagentMergeStrategy::BEST_SCORE;
    InferenceRequest request;
    request.prompt = "score";
    auto aggregate = coordinator->inferMultiple({"s1", "s2", "s3"}, request, coord_cfg);

    ASSERT_TRUE(aggregate.success);
    EXPECT_EQ(aggregate.num_failed, 1u);
    EXPECT_EQ(aggregate.merged_output, "this is a significantly longer answer for scoring");
    EXPECT_NE(aggregate.summary.find("merge=best_score"), std::string::npos);
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
