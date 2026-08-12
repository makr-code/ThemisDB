/**
 * @file test_model_router.cpp
 * @brief Unit tests for ModelRouter (content-based and metadata-tag routing).
 */

#include <gtest/gtest.h>
#include "llm/model_router.h"
#include "llm/inference_engine_enhanced.h"
#include "llm/llm_plugin_interface.h"
#include <nlohmann/json.hpp>

using namespace themis::llm;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static RoutingRule makePromptRule(const std::string& id,
                                   const std::string& model_id,
                                   const std::string& pattern,
                                   int priority = 0) {
    RoutingRule r;
    r.id = id;
    r.target_model_id = model_id;
    r.prompt_patterns = { pattern };
    r.priority = priority;
    return r;
}

static RoutingRule makeTagRule(const std::string& id,
                                const std::string& model_id,
                                const std::vector<std::string>& tags,
                                int priority = 0) {
    RoutingRule r;
    r.id = id;
    r.target_model_id = model_id;
    r.metadata_tags = tags;
    r.priority = priority;
    return r;
}

static json tagsJson(const std::vector<std::string>& tags) {
    json m;
    m["tags"] = tags;
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// ModelRouter unit tests
// ─────────────────────────────────────────────────────────────────────────────

class ModelRouterTest : public ::testing::Test {
protected:
    ModelRouter router;
};

// ── Basic prompt-pattern matching ────────────────────────────────────────────

TEST_F(ModelRouterTest, PromptPatternMatchesKeyword) {
    router.addRule(makePromptRule("r1", "code-model", "python|java|c\\+\\+"));
    auto result = router.route("Write a python function", json{});
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.model_id, "code-model");
    EXPECT_EQ(result.rule_id, "r1");
}

TEST_F(ModelRouterTest, PromptPatternNoMatch) {
    router.addRule(makePromptRule("r1", "code-model", "python|java"));
    auto result = router.route("Translate this text to French", json{});
    EXPECT_FALSE(result.matched);
    EXPECT_TRUE(result.model_id.empty());
}

TEST_F(ModelRouterTest, PromptPatternIsCaseInsensitive) {
    router.addRule(makePromptRule("r1", "code-model", "PYTHON"));
    auto result = router.route("write Python code", json{});
    EXPECT_TRUE(result.matched);
}

// ── Metadata tag matching ─────────────────────────────────────────────────────

TEST_F(ModelRouterTest, MetadataTagMatchesSingleTag) {
    router.addRule(makeTagRule("r2", "legal-model", {"legal"}));
    auto result = router.route("Analyse the contract", tagsJson({"legal", "contract"}));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.model_id, "legal-model");
}

TEST_F(ModelRouterTest, MetadataTagNoMatch) {
    router.addRule(makeTagRule("r2", "legal-model", {"legal"}));
    auto result = router.route("Analyse the contract", tagsJson({"general"}));
    EXPECT_FALSE(result.matched);
}

TEST_F(ModelRouterTest, MetadataTagMissingTagsKey) {
    router.addRule(makeTagRule("r2", "legal-model", {"legal"}));
    auto result = router.route("some prompt", json{});
    EXPECT_FALSE(result.matched);
}

// ── Priority ordering ─────────────────────────────────────────────────────────

TEST_F(ModelRouterTest, HigherPriorityRuleWins) {
    router.addRule(makePromptRule("low",  "model-low",  "test", 1));
    router.addRule(makePromptRule("high", "model-high", "test", 10));
    auto result = router.route("this is a test", json{});
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.model_id, "model-high");
    EXPECT_EQ(result.rule_id, "high");
}

TEST_F(ModelRouterTest, EqualPriorityFirstInsertionWins) {
    router.addRule(makePromptRule("first",  "model-a", "hello", 5));
    router.addRule(makePromptRule("second", "model-b", "hello", 5));
    auto result = router.route("say hello", json{});
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.model_id, "model-a");
}

// ── MatchMode::ALL ────────────────────────────────────────────────────────────

TEST_F(ModelRouterTest, AllModeRequiresBothPatternAndTag) {
    RoutingRule r;
    r.id = "r-all";
    r.target_model_id = "strict-model";
    r.prompt_patterns = { "urgent" };
    r.metadata_tags   = { "premium" };
    r.match_mode      = RoutingRule::MatchMode::ALL;
    router.addRule(r);

    // Both match
    auto yes = router.route("urgent request here", tagsJson({"premium"}));
    EXPECT_TRUE(yes.matched);

    // Only prompt matches
    auto no_tag = router.route("urgent request here", tagsJson({"free"}));
    EXPECT_FALSE(no_tag.matched);

    // Only tag matches
    auto no_prompt = router.route("regular request", tagsJson({"premium"}));
    EXPECT_FALSE(no_prompt.matched);
}

// ── MatchMode::ANY ────────────────────────────────────────────────────────────

TEST_F(ModelRouterTest, AnyModeMatchesEitherCriterion) {
    RoutingRule r;
    r.id = "r-any";
    r.target_model_id = "any-model";
    r.prompt_patterns = { "translate" };
    r.metadata_tags   = { "multilingual" };
    r.match_mode      = RoutingRule::MatchMode::ANY;
    router.addRule(r);

    // Only prompt matches
    EXPECT_TRUE(router.route("please translate this", json{}).matched);

    // Only tag matches
    EXPECT_TRUE(router.route("summarise this doc", tagsJson({"multilingual"})).matched);

    // Neither matches
    EXPECT_FALSE(router.route("calculate 2+2", tagsJson({"math"})).matched);
}

// ── Rule management ───────────────────────────────────────────────────────────

TEST_F(ModelRouterTest, AddRuleIncreasesCount) {
    EXPECT_EQ(router.ruleCount(), 0u);
    router.addRule(makePromptRule("r1", "m1", "foo"));
    EXPECT_EQ(router.ruleCount(), 1u);
    router.addRule(makePromptRule("r2", "m2", "bar"));
    EXPECT_EQ(router.ruleCount(), 2u);
}

TEST_F(ModelRouterTest, RemoveRuleDecreasesCount) {
    router.addRule(makePromptRule("r1", "m1", "foo"));
    EXPECT_TRUE(router.removeRule("r1"));
    EXPECT_EQ(router.ruleCount(), 0u);
}

TEST_F(ModelRouterTest, RemoveNonExistentRuleReturnsFalse) {
    EXPECT_FALSE(router.removeRule("nonexistent"));
}

TEST_F(ModelRouterTest, ClearRulesRemovesAll) {
    router.addRule(makePromptRule("r1", "m1", "foo"));
    router.addRule(makePromptRule("r2", "m2", "bar"));
    router.clearRules();
    EXPECT_EQ(router.ruleCount(), 0u);
    EXPECT_FALSE(router.route("foo", json{}).matched);
}

TEST_F(ModelRouterTest, GetRulesReturnsSortedByPriority) {
    router.addRule(makePromptRule("r1", "m1", "a", 5));
    router.addRule(makePromptRule("r2", "m2", "b", 10));
    router.addRule(makePromptRule("r3", "m3", "c", 1));
    auto rules = router.getRules();
    ASSERT_EQ(rules.size(), 3u);
    EXPECT_EQ(rules[0].priority, 10);
    EXPECT_EQ(rules[1].priority, 5);
    EXPECT_EQ(rules[2].priority, 1);
}

TEST_F(ModelRouterTest, ReplaceExistingRule) {
    router.addRule(makePromptRule("r1", "model-a", "hello"));
    // Replace with a different target
    router.addRule(makePromptRule("r1", "model-b", "hello"));
    EXPECT_EQ(router.ruleCount(), 1u);
    auto result = router.route("say hello", json{});
    EXPECT_EQ(result.model_id, "model-b");
}

// ── Validation ────────────────────────────────────────────────────────────────

TEST_F(ModelRouterTest, EmptyIdThrows) {
    RoutingRule r;
    r.id = "";
    r.target_model_id = "m1";
    r.prompt_patterns = { "x" };
    EXPECT_THROW(router.addRule(r), std::invalid_argument);
}

TEST_F(ModelRouterTest, EmptyTargetModelThrows) {
    RoutingRule r;
    r.id = "r1";
    r.target_model_id = "";
    r.prompt_patterns = { "x" };
    EXPECT_THROW(router.addRule(r), std::invalid_argument);
}

TEST_F(ModelRouterTest, NoCriteriaThrows) {
    RoutingRule r;
    r.id = "r1";
    r.target_model_id = "m1";
    // No patterns, no tags
    EXPECT_THROW(router.addRule(r), std::invalid_argument);
}

TEST_F(ModelRouterTest, InvalidRegexThrows) {
    RoutingRule r;
    r.id = "r1";
    r.target_model_id = "m1";
    r.prompt_patterns = { "[invalid" };  // unclosed bracket
    EXPECT_THROW(router.addRule(r), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// InferenceEngineEnhanced integration tests
// ─────────────────────────────────────────────────────────────────────────────

class MockLLMPlugin : public ILLMPlugin {
public:
    explicit MockLLMPlugin(const std::string& model_id) : model_id_(model_id) {}
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    bool isModelLoaded() const override { return true; }
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info;
        info.model_id = model_id_;
        info.is_loaded = true;
        return info;
    }
    InferenceResponse generate(const InferenceRequest& req) override {
        InferenceResponse r;
        r.request_id = req.request_id;
        r.text = "response from " + model_id_;
        r.model_id = model_id_;
        r.tokens_generated = 5;
        r.inference_time_ms = 10.0f;
        return r;
    }
    InferenceResponse generateRAG(const RAGContext&, const InferenceRequest& req) override {
        return generate(req);
    }
    std::vector<float> embed(const std::string&) override { return {}; }
    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return {}; }
    json getPerformanceStats() const override { return {}; }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
private:
    std::string model_id_;
};

class EngineRoutingTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceEngineEnhanced::Config cfg;
        cfg.enable_context_caching  = false;
        cfg.enable_batch_processing = false;
        cfg.enable_load_balancing   = true;
        cfg.num_worker_threads      = 1;
        cfg.batch_timeout_ms        = 10;
        engine_ = std::make_unique<InferenceEngineEnhanced>(cfg);

        engine_->registerModel("general-model",  std::make_shared<MockLLMPlugin>("general-model"));
        engine_->registerModel("code-model",     std::make_shared<MockLLMPlugin>("code-model"));
        engine_->registerModel("legal-model",    std::make_shared<MockLLMPlugin>("legal-model"));
        engine_->start();
    }
    void TearDown() override { engine_->shutdown(); }

    InferenceEngineEnhanced::EnhancedInferenceRequest makeRequest(
        const std::string& prompt,
        const json& metadata = json{}) const
    {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        req.base_request.prompt = prompt;
        req.base_request.metadata = metadata;
        return req;
    }

    std::unique_ptr<InferenceEngineEnhanced> engine_;
};

TEST_F(EngineRoutingTest, PromptRoutingSelectsCodeModel) {
    engine_->addRoutingRule(makePromptRule("code-rule", "code-model", "python|function|algorithm"));

    auto req = makeRequest("Write a python function to sort a list");
    auto handle = engine_->submit(req);
    auto response = handle.get();

    EXPECT_EQ(response.model_id, "code-model");
}

TEST_F(EngineRoutingTest, TagRoutingSelectsLegalModel) {
    engine_->addRoutingRule(makeTagRule("legal-rule", "legal-model", {"legal"}));

    auto req = makeRequest("Analyse this agreement", tagsJson({"legal", "contract"}));
    auto handle = engine_->submit(req);
    auto response = handle.get();

    EXPECT_EQ(response.model_id, "legal-model");
}

TEST_F(EngineRoutingTest, NoRuleUsesLoadBalancer) {
    // With no routing rules the engine falls back to load balancing.
    auto req = makeRequest("How is the weather?");
    auto handle = engine_->submit(req);
    auto response = handle.get();
    // We just verify a valid model was chosen
    EXPECT_FALSE(response.model_id.empty());
}

TEST_F(EngineRoutingTest, RemoveRuleFallsBackToLoadBalancer) {
    engine_->addRoutingRule(makePromptRule("code-rule", "code-model", "algorithm"));
    engine_->removeRoutingRule("code-rule");

    // Now no rule fires; load balancer picks any available model
    auto req = makeRequest("implement an algorithm");
    auto handle = engine_->submit(req);
    auto response = handle.get();
    EXPECT_FALSE(response.model_id.empty());
}

TEST_F(EngineRoutingTest, GetRoutingRulesReturnsAddedRules) {
    engine_->addRoutingRule(makePromptRule("r1", "code-model",  "code",  5));
    engine_->addRoutingRule(makePromptRule("r2", "legal-model", "legal", 10));

    auto rules = engine_->getRoutingRules();
    ASSERT_EQ(rules.size(), 2u);
    EXPECT_EQ(rules[0].id, "r2");  // higher priority first
    EXPECT_EQ(rules[1].id, "r1");
}

TEST_F(EngineRoutingTest, ClearRoutingRulesRemovesAll) {
    engine_->addRoutingRule(makePromptRule("r1", "code-model", "code"));
    engine_->clearRoutingRules();
    EXPECT_TRUE(engine_->getRoutingRules().empty());
}
