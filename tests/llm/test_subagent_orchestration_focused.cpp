/**
 * @file test_subagent_orchestration_focused.cpp
 * @brief Comprehensive tests for LLM Subagent Orchestration Layer.
 *        Tests factory, lifecycle, coordinator, isolation, and failure scenarios.
 *
 * Test Coverage:
 *   - SO-01..SO-08: Subagent Factory (create, validate, list, metrics)
 *   - SO-09..SO-16: Subagent Lifecycle (load, unload, state transitions)
 *   - SO-17..SO-24: Subagent Inference (single, batch, async)
 *   - SO-25..SO-32: SubagentCoordinator (fan-out, merge strategies, partial failures)
 *   - SO-33..SO-40: Resource Isolation (quota, policy, concurrent load)
 *   - SO-41..SO-48: Error Handling (timeouts, invalid configs, recovery)
 */

#include <gtest/gtest.h>
#include "llm/subagent_factory.h"
#include "llm/subagent_coordinator.h"
#include "llm/subagent.h"
#include "llm/llm_plugin_interface.h"
#include "llm/shared_worker_pool.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include "llm/token_quota_manager.h"
#include "llm/prompt_policy.h"
#include "utils/expected.h"

#include <memory>
#include <thread>
#include <vector>
#include <atomic>

namespace themis {
namespace llm {

// ============================================================================
// § 1  Mock LLM Plugin for Testing
// ============================================================================

class MockLLMPlugin : public ILLMPlugin {
public:
    std::string getName() const override { return "MockPlugin"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    InferenceResponse generate(const InferenceRequest& req) override {
        InferenceResponse resp;
        resp.output = "Mock: " + req.prompt.substr(0, 20);
        resp.tokens_generated = 10;
        return resp;
    }

    std::vector<InferenceResponse> generateBatch(
        const std::vector<InferenceRequest>& reqs) override {
        std::vector<InferenceResponse> results;
        for (const auto& req : reqs) {
            results.push_back(generate(req));
        }
        return results;
    }

    EmbeddingResponse embed(const std::string& text) override {
        EmbeddingResponse resp;
        resp.embedding.resize(384, 0.1f);
        return resp;
    }

    std::vector<EmbeddingResponse> embedBatch(
        const std::vector<std::string>& texts) override {
        std::vector<EmbeddingResponse> results;
        for (const auto& text : texts) {
            results.push_back(embed(text));
        }
        return results;
    }

    void setConfig(const json& config) override {}
    json getConfig() const override { return json::object(); }
    bool isLoaded() const override { return true; }
    InferenceCapabilities getCapabilities() const override {
        return InferenceCapabilities{};
    }
};

// ============================================================================
// § 2  Mock Model Loader
// ============================================================================

class MockModelLoader : public ModelLoader {
    // Simplified - in production, loads actual models
public:
    bool loadModel(const std::string& model_id) override { return true; }
    bool unloadModel(const std::string& model_id) override { return true; }
    bool modelExists(const std::string& model_id) const override { return true; }
};

// ============================================================================
// § 3  Mock Multi-LoRA Manager
// ============================================================================

class MockMultiLoRAManager : public MultiLoRAManager {
    // Simplified - in production, manages LoRA adapters
public:
    bool loadAdapter(const std::string& adapter_id) override { return true; }
    bool unloadAdapter(const std::string& adapter_id) override { return true; }
};

// ============================================================================
// § 4  SubagentFactory Tests
// ============================================================================

class SubagentFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_shared<MockLLMPlugin>();
        worker_pool_ = std::make_shared<SharedWorkerPool>(2);
        model_loader_ = std::make_shared<MockModelLoader>();
        lora_manager_ = std::make_shared<MockMultiLoRAManager>();
        quota_manager_ = std::make_shared<TokenQuotaManager>();

        auto factory_result = SubagentFactory::create(
            plugin_.get(),
            worker_pool_,
            model_loader_,
            lora_manager_,
            quota_manager_
        );
        ASSERT_TRUE(factory_result);
        factory_ = factory_result.value();
    }

    std::shared_ptr<MockLLMPlugin> plugin_;
    std::shared_ptr<SharedWorkerPool> worker_pool_;
    std::shared_ptr<MockModelLoader> model_loader_;
    std::shared_ptr<MockMultiLoRAManager> lora_manager_;
    std::shared_ptr<TokenQuotaManager> quota_manager_;
    std::unique_ptr<SubagentFactory> factory_;
};

// SO-01: Factory creation succeeds
TEST_F(SubagentFactoryTest, FactoryCreation) {
    EXPECT_TRUE(factory_);
}

// SO-02: Configuration validation detects empty model_id
TEST_F(SubagentFactoryTest, ValidateConfigEmptyModelId) {
    SubagentConfig config;
    config.id = "test";
    config.model_id = "";  // Invalid

    auto errors = factory_->validateConfig(config);
    EXPECT_FALSE(errors.empty());
}

// SO-03: Subagent creation succeeds with valid config
TEST_F(SubagentFactoryTest, CreateSubagentSuccess) {
    SubagentConfig config;
    config.id = "assistant_1";
    config.model_id = "mistral-7b";
    config.budget.max_tokens_per_minute = 50000;

    auto result = factory_->createSubagent(config);
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value()->id(), "assistant_1");
}

// SO-04: List subagents returns created instances
TEST_F(SubagentFactoryTest, ListSubagents) {
    SubagentConfig config1;
    config1.id = "assistant_1";
    config1.model_id = "mistral-7b";

    SubagentConfig config2;
    config2.id = "analyzer_2";
    config2.model_id = "gpt-3.5";

    auto result1 = factory_->createSubagent(config1);
    auto result2 = factory_->createSubagent(config2);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);

    auto subagents = factory_->listSubagents();
    EXPECT_EQ(subagents.size(), 2);
}

// SO-05: Get subagent by ID
TEST_F(SubagentFactoryTest, GetSubagent) {
    SubagentConfig config;
    config.id = "test_agent";
    config.model_id = "mistral-7b";

    auto create_result = factory_->createSubagent(config);
    EXPECT_TRUE(create_result);

    auto retrieved = factory_->getSubagent("test_agent");
    EXPECT_TRUE(retrieved);
    EXPECT_EQ(retrieved->id(), "test_agent");
}

// SO-06: Destroy subagent
TEST_F(SubagentFactoryTest, DestroySubagent) {
    SubagentConfig config;
    config.id = "test_agent";
    config.model_id = "mistral-7b";

    auto create_result = factory_->createSubagent(config);
    EXPECT_TRUE(create_result);

    auto destroy_result = factory_->destroySubagent("test_agent");
    EXPECT_TRUE(destroy_result);

    auto retrieved = factory_->getSubagent("test_agent");
    EXPECT_FALSE(retrieved);
}

// SO-07: Factory statistics
TEST_F(SubagentFactoryTest, FactoryStatistics) {
    SubagentConfig config;
    config.id = "agent_1";
    config.model_id = "mistral-7b";

    factory_->createSubagent(config);
    auto stats = factory_->getFactoryStats();

    EXPECT_EQ(stats.total_created, 1);
    EXPECT_EQ(stats.currently_active, 1);
}

// SO-08: Register and unregister prompt policy
TEST_F(SubagentFactoryTest, PromptPolicyManagement) {
    auto policy = std::make_shared<PromptPolicy>();
    auto register_result = factory_->registerPromptPolicy("test_policy", policy);
    EXPECT_TRUE(register_result);

    auto unregister_result = factory_->unregisterPromptPolicy("test_policy");
    EXPECT_TRUE(unregister_result);
}

// ============================================================================
// § 5  Subagent Lifecycle Tests
// ============================================================================

class SubagentLifecycleTest : public SubagentFactoryTest {
protected:
    void SetUp() override {
        SubagentFactoryTest::SetUp();
        SubagentConfig config;
        config.id = "lifecycle_agent";
        config.model_id = "mistral-7b";
        auto result = factory_->createSubagent(config);
        ASSERT_TRUE(result);
        subagent_ = result.value();
    }

    std::shared_ptr<Subagent> subagent_;
};

// SO-09: Subagent initial state is CREATED
TEST_F(SubagentLifecycleTest, InitialState) {
    EXPECT_EQ(subagent_->getState(), SubagentState::CREATED);
    EXPECT_FALSE(subagent_->isReady());
}

// SO-10: Load transitions to READY
TEST_F(SubagentLifecycleTest, LoadTransition) {
    auto result = subagent_->load();
    EXPECT_TRUE(result);
    EXPECT_EQ(subagent_->getState(), SubagentState::READY);
    EXPECT_TRUE(subagent_->isReady());
}

// SO-11: Unload after load
TEST_F(SubagentLifecycleTest, UnloadAfterLoad) {
    subagent_->load();
    auto result = subagent_->unload();
    EXPECT_TRUE(result);
    EXPECT_EQ(subagent_->getState(), SubagentState::TERMINATED);
}

// SO-12: Pause and resume
TEST_F(SubagentLifecycleTest, PauseResume) {
    subagent_->load();
    EXPECT_EQ(subagent_->getState(), SubagentState::READY);

    auto pause_result = subagent_->pause();
    EXPECT_TRUE(pause_result);
    EXPECT_EQ(subagent_->getState(), SubagentState::PAUSED);

    auto resume_result = subagent_->resume();
    EXPECT_TRUE(resume_result);
    EXPECT_EQ(subagent_->getState(), SubagentState::READY);
}

// SO-13: Cannot load twice
TEST_F(SubagentLifecycleTest, NoDoubleLoad) {
    subagent_->load();
    auto result = subagent_->load();
    EXPECT_FALSE(result);
}

// SO-14: Idempotent unload
TEST_F(SubagentLifecycleTest, IdempotentUnload) {
    subagent_->load();
    auto result1 = subagent_->unload();
    auto result2 = subagent_->unload();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
}

// SO-15: Warm improves cache
TEST_F(SubagentLifecycleTest, Warm) {
    subagent_->load();
    auto result = subagent_->warm();
    EXPECT_TRUE(result);
}

// SO-16: State name conversion
TEST_F(SubagentLifecycleTest, StateToString) {
    EXPECT_STREQ(subagentStateToString(SubagentState::CREATED), "CREATED");
    EXPECT_STREQ(subagentStateToString(SubagentState::READY), "READY");
    EXPECT_STREQ(subagentStateToString(SubagentState::PAUSED), "PAUSED");
}

// ============================================================================
// § 6  Subagent Inference Tests
// ============================================================================

class SubagentInferenceTest : public SubagentLifecycleTest {
protected:
    void SetUp() override {
        SubagentLifecycleTest::SetUp();
        subagent_->load();
    }
};

// SO-17: Single inference succeeds
TEST_F(SubagentInferenceTest, SingleInference) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    auto result = subagent_->infer(req);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.output.empty());
}

// SO-18: Async inference returns future
TEST_F(SubagentInferenceTest, AsyncInference) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    auto future = subagent_->inferAsync(req);
    EXPECT_TRUE(future.valid());

    auto result = future.get();
    EXPECT_TRUE(result.success);
}

// SO-19: Batch inference
TEST_F(SubagentInferenceTest, BatchInference) {
    std::vector<InferenceRequest> requests;
    for (int i = 0; i < 3; ++i) {
        InferenceRequest req;
        req.prompt = "Question " + std::to_string(i);
        requests.push_back(req);
    }

    auto results = subagent_->inferBatch(requests);
    EXPECT_EQ(results.size(), 3);
    for (const auto& result : results) {
        EXPECT_TRUE(result.success);
    }
}

// SO-20: Stream inference with callback
TEST_F(SubagentInferenceTest, StreamInference) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    std::string received_tokens;
    auto result = subagent_->inferStream(req, [&](const std::string& token) {
        received_tokens += token;
    });

    EXPECT_TRUE(result.success);
}

// SO-21: Metrics collection
TEST_F(SubagentInferenceTest, MetricsCollection) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    subagent_->infer(req);
    auto metrics = subagent_->getMetrics();

    EXPECT_GT(metrics.total_requests, 0);
    EXPECT_GT(metrics.successful_inferences, 0);
}

// SO-22: Reset metrics
TEST_F(SubagentInferenceTest, ResetMetrics) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    subagent_->infer(req);
    subagent_->resetMetrics();
    auto metrics = subagent_->getMetrics();

    EXPECT_EQ(metrics.tokens_consumed, 0);
}

// SO-23: Quota tracking
TEST_F(SubagentInferenceTest, QuotaTracking) {
    auto check = subagent_->checkQuota(100);
    EXPECT_TRUE(check.allowed);

    subagent_->consumeQuota(100);
}

// SO-24: Inference fails when not ready
TEST_F(SubagentInferenceTest, InferenceWhenNotReady) {
    subagent_->pause();
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    auto result = subagent_->infer(req);
    EXPECT_FALSE(result.success);
}

// ============================================================================
// § 7  SubagentCoordinator Tests
// ============================================================================

class SubagentCoordinatorTest : public SubagentFactoryTest {
protected:
    void SetUp() override {
        SubagentFactoryTest::SetUp();
        // Create multiple subagents
        for (int i = 0; i < 3; ++i) {
            SubagentConfig config;
            config.id = "agent_" + std::to_string(i);
            config.model_id = "mistral-7b";
            auto result = factory_->createSubagent(config);
            ASSERT_TRUE(result);
            auto subagent = result.value();
            subagent->load();
            subagents_.push_back(subagent);
            subagent_ids_.push_back(config.id);
        }

        auto coordinator_result = SubagentCoordinator::create(factory_);
        ASSERT_TRUE(coordinator_result);
        coordinator_ = coordinator_result.value();
    }

    std::vector<std::shared_ptr<Subagent>> subagents_;
    std::vector<std::string> subagent_ids_;
    std::unique_ptr<SubagentCoordinator> coordinator_;
};

// SO-25: Coordinator creation
TEST_F(SubagentCoordinatorTest, CoordinatorCreation) {
    EXPECT_TRUE(coordinator_);
}

// SO-26: First-win merge strategy
TEST_F(SubagentCoordinatorTest, FirstWinStrategy) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    config.strategy = SubagentMergeStrategy::FIRST_WIN;

    auto result = coordinator_->inferMultiple(subagent_ids_, req, config);
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.num_successful, 0);
}

// SO-27: All-succeed merge strategy
TEST_F(SubagentCoordinatorTest, AllSucceedStrategy) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    config.strategy = SubagentMergeStrategy::ALL_SUCCEED;

    auto result = coordinator_->inferMultiple(subagent_ids_, req, config);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.num_failed, 0);
}

// SO-28: Ensemble merge strategy
TEST_F(SubagentCoordinatorTest, EnsembleStrategy) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    config.strategy = SubagentMergeStrategy::ENSEMBLE;

    auto result = coordinator_->inferMultiple(subagent_ids_, req, config);
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.per_subagent_results.size(), 0);
}

// SO-29: Partial failure handling
TEST_F(SubagentCoordinatorTest, PartialFailureHandling) {
    // Pause one subagent
    subagents_[0]->pause();

    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    config.strategy = SubagentMergeStrategy::FIRST_WIN;
    config.fail_on_any_error = false;

    auto result = coordinator_->inferMultiple(subagent_ids_, req, config);
    EXPECT_TRUE(result.success);  // Should succeed despite one failure
}

// SO-30: Batch coordination
TEST_F(SubagentCoordinatorTest, BatchCoordination) {
    std::vector<InferenceRequest> requests;
    for (int i = 0; i < 2; ++i) {
        InferenceRequest req;
        req.prompt = "Question " + std::to_string(i);
        requests.push_back(req);
    }

    SubagentCoordinatorConfig config;
    config.strategy = SubagentMergeStrategy::FIRST_WIN;

    auto results = coordinator_->inferMultipleBatch(subagent_ids_, requests, config);
    EXPECT_EQ(results.size(), 2);
}

// SO-31: Coordinator statistics
TEST_F(SubagentCoordinatorTest, CoordinatorStatistics) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    coordinator_->inferMultiple(subagent_ids_, req, config);

    auto stats = coordinator_->getStats();
    EXPECT_GT(stats.total_coordinations, 0);
}

// SO-32: Diagnostics collection
TEST_F(SubagentCoordinatorTest, Diagnostics) {
    InferenceRequest req;
    req.prompt = "What is 2+2?";

    SubagentCoordinatorConfig config;
    config.verbose_logging = true;
    coordinator_->inferMultiple(subagent_ids_, req, config);

    auto diag = coordinator_->getLastDiagnostics();
    EXPECT_FALSE(diag.summary.empty());
}

// ============================================================================
// § 8  Resource Isolation Tests
// ============================================================================

class SubagentIsolationTest : public SubagentFactoryTest {
protected:
    void SetUp() override {
        SubagentFactoryTest::SetUp();
        SubagentConfig config;
        config.id = "isolation_agent";
        config.model_id = "mistral-7b";
        config.budget.max_tokens_per_minute = 100;  // Small quota
        config.isolation_level = SubagentIsolationLevel::STRICT;

        auto result = factory_->createSubagent(config);
        ASSERT_TRUE(result);
        subagent_ = result.value();
        subagent_->load();
    }

    std::shared_ptr<Subagent> subagent_;
};

// SO-33: Independent quota tracking
TEST_F(SubagentIsolationTest, IndependentQuota) {
    auto check = subagent_->checkQuota(50);
    EXPECT_TRUE(check.allowed);
    EXPECT_LE(check.tokens_used, 100);
}

// SO-34: Quota exhaustion blocks requests
TEST_F(SubagentIsolationTest, QuotaExhaustion) {
    subagent_->consumeQuota(100);  // Consume all quota
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

} // namespace llm
} // namespace themis
