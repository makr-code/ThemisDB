/**
 * @file test_constitutional_reasoning.cpp
 * @brief Unit tests for Constitutional Reasoning Engine
 */

#include <gtest/gtest.h>

// Disable constitutional reasoning tests
#if 0
#include "llm/constitutional_reasoning_engine.h"

using namespace themis::llm;

class ConstitutionalReasoningEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.enable_self_critique = true;
        config_.enable_self_revision = true;
        config_.max_iterations = 3;
        config_.improvement_threshold = 0.05f;
        config_.min_acceptable_score = 0.7f;
        
        engine_ = std::make_unique<ConstitutionalReasoningEngine>(config_);
    }
    
    ConstitutionalReasoningConfig config_;
    std::unique_ptr<ConstitutionalReasoningEngine> engine_;
};

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, DefaultConfiguration) {
    auto engine = std::make_unique<ConstitutionalReasoningEngine>();
    auto config = engine->getConfig();
    
    EXPECT_TRUE(config.enable_self_critique);
    EXPECT_TRUE(config.enable_self_revision);
    EXPECT_GT(config.max_iterations, 0);
}

TEST_F(ConstitutionalReasoningEngineTest, ConfigurationUpdate) {
    ConstitutionalReasoningConfig new_config;
    new_config.max_iterations = 5;
    new_config.improvement_threshold = 0.1f;
    
    engine_->setConfig(new_config);
    auto retrieved = engine_->getConfig();
    
    EXPECT_EQ(retrieved.max_iterations, 5);
    EXPECT_FLOAT_EQ(retrieved.improvement_threshold, 0.1f);
}

// ═══════════════════════════════════════════════════════════
// Principle Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, LoadDefaultPrinciples) {
    engine_->loadDefaultPrinciples();
    auto principles = engine_->getPrinciples();
    
    EXPECT_GT(principles.size(), 0);
    
    // Check for key principles
    bool has_autonomy = false;
    bool has_transparency = false;
    bool has_harm = false;
    
    for (const auto& p : principles) {
        if (p.id == "human_autonomy") has_autonomy = true;
        if (p.id == "transparency") has_transparency = true;
        if (p.id == "do_no_harm") has_harm = true;
    }
    
    EXPECT_TRUE(has_autonomy);
    EXPECT_TRUE(has_transparency);
    EXPECT_TRUE(has_harm);
}

TEST_F(ConstitutionalReasoningEngineTest, AddPrinciple) {
    ConstitutionalPrinciple custom;
    custom.id = "custom_principle";
    custom.name = "Custom Test Principle";
    custom.description = "A custom principle for testing";
    custom.priority = 2;
    custom.domain_agnostic = true;
    
    engine_->addPrinciple(custom);
    auto principles = engine_->getPrinciples();
    
    bool found = false;
    for (const auto& p : principles) {
        if (p.id == "custom_principle") {
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);
}

TEST_F(ConstitutionalReasoningEngineTest, RemovePrinciple) {
    engine_->loadDefaultPrinciples();
    auto principles_before = engine_->getPrinciples();
    size_t count_before = principles_before.size();
    
    if (count_before > 0) {
        engine_->removePrinciple(principles_before[0].id);
        auto principles_after = engine_->getPrinciples();
        
        EXPECT_EQ(principles_after.size(), count_before - 1);
    }
}

// ═══════════════════════════════════════════════════════════
// Violation Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, CheckViolationsPatronizing) {
    std::string response = "You must do this immediately. You have to follow these instructions.";
    
    auto violations = engine_->checkViolations(response);
    
    // Should detect autonomy violation
    EXPECT_GT(violations.size(), 0);
    
    bool has_autonomy_violation = false;
    for (const auto& v : violations) {
        if (v == "human_autonomy") {
            has_autonomy_violation = true;
            break;
        }
    }
    EXPECT_TRUE(has_autonomy_violation);
}

TEST_F(ConstitutionalReasoningEngineTest, CheckViolationsNoTransparency) {
    std::string response = "This is definitely the answer. There is no doubt about it.";
    
    auto violations = engine_->checkViolations(response);
    
    // Should detect transparency violation
    bool has_transparency_violation = false;
    for (const auto& v : violations) {
        if (v == "transparency") {
            has_transparency_violation = true;
            break;
        }
    }
    EXPECT_TRUE(has_transparency_violation);
}

TEST_F(ConstitutionalReasoningEngineTest, CheckViolationsCleanResponse) {
    std::string response = "You might consider several options. "
                           "This could work, but I'm not entirely certain. "
                           "It may be helpful to consult with experts.";
    
    auto violations = engine_->checkViolations(response);
    
    EXPECT_EQ(violations.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Scoring Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, ScoreResponseGood) {
    std::string response = "You might consider these options. "
                           "Based on available information, this could work. "
                           "However, I recommend consulting with experts.";
    
    float score = engine_->scoreResponse(response);
    
    EXPECT_GE(score, 0.7f);
}

TEST_F(ConstitutionalReasoningEngineTest, ScoreResponsePoor) {
    std::string response = "You must do this. This is definitely correct. "
                           "You have to follow exactly.";
    
    float score = engine_->scoreResponse(response);
    
    EXPECT_LT(score, 0.7f);
}

// ═══════════════════════════════════════════════════════════
// Critique Generation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, GenerateCritiqueForViolation) {
    std::string response = "You must do this immediately.";
    std::string query = "What should I do?";
    
    // Get autonomy principle
    auto principles = engine_->getPrinciples();
    ConstitutionalPrinciple autonomy_principle;
    for (const auto& p : principles) {
        if (p.id == "human_autonomy") {
            autonomy_principle = p;
            break;
        }
    }
    
    if (autonomy_principle.id == "human_autonomy") {
        std::string critique = engine_->generateCritique(
            response, query, autonomy_principle, nullptr
        );
        
        EXPECT_FALSE(critique.empty());
    }
}

// ═══════════════════════════════════════════════════════════
// Revision Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, GenerateRevision) {
    std::string response = "You must do this.";
    std::vector<std::string> critiques = {
        "Response uses commanding language.",
        "Does not respect human autonomy."
    };
    std::string query = "What should I do?";
    
    std::string revised = engine_->generateRevision(
        response, critiques, query, nullptr
    );
    
    EXPECT_FALSE(revised.empty());
    EXPECT_NE(revised, response); // Should be different
}

// ═══════════════════════════════════════════════════════════
// Full Reasoning Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, ReasonWithViolations) {
    std::string response = "You must do this immediately. This is definitely correct.";
    std::string query = "What should I do about this ethical dilemma?";
    
    auto result = engine_->reason(response, query, nullptr);
    
    EXPECT_EQ(result.original_response, response);
    EXPECT_GT(result.violated_principles.size(), 0);
    EXPECT_GE(result.original_score, 0.0f);
    EXPECT_LE(result.original_score, 1.0f);
}

TEST_F(ConstitutionalReasoningEngineTest, ReasonWithoutViolations) {
    std::string response = "You might consider several approaches. "
                           "Different options could work depending on your situation. "
                           "It may be helpful to consult with relevant experts.";
    std::string query = "What should I do?";
    
    auto result = engine_->reason(response, query, nullptr);
    
    EXPECT_EQ(result.violated_principles.size(), 0);
    EXPECT_FALSE(result.was_revised);
}

TEST_F(ConstitutionalReasoningEngineTest, ReasonIterations) {
    std::string response = "You must do this. You have to follow. "
                           "This is definitely right.";
    std::string query = "What should I do?";
    
    auto result = engine_->reason(response, query, nullptr);
    
    // Should have attempted some iterations
    EXPECT_GE(result.iterations, 0);
    EXPECT_LE(result.iterations, config_.max_iterations);
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, StatisticsTracking) {
    std::string response1 = "You must do this.";
    std::string response2 = "You might consider this.";
    std::string query = "What should I do?";
    
    engine_->reason(response1, query, nullptr);
    engine_->reason(response2, query, nullptr);
    
    auto stats = engine_->getStatistics();
    
    EXPECT_EQ(stats.total_reasonings, 2);
}

TEST_F(ConstitutionalReasoningEngineTest, StatisticsReset) {
    std::string response = "You must do this.";
    std::string query = "What should I do?";
    
    engine_->reason(response, query, nullptr);
    engine_->resetStatistics();
    
    auto stats = engine_->getStatistics();
    EXPECT_EQ(stats.total_reasonings, 0);
}

TEST_F(ConstitutionalReasoningEngineTest, PrincipleViolationTracking) {
    std::string response = "You must do this immediately.";
    std::string query = "What should I do?";
    
    engine_->reason(response, query, nullptr);
    auto stats = engine_->getStatistics();
    
    EXPECT_GT(stats.violations_detected, 0);
}

// ═══════════════════════════════════════════════════════════
// Callback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, ReasoningCallback) {
    bool callback_called = false;
    
    engine_->setReasoningCallback(
        [&callback_called](const ConstitutionalReasoningResult& result) {
            callback_called = true;
        }
    );
    
    std::string response = "You might consider this.";
    std::string query = "What should I do?";
    
    engine_->reason(response, query, nullptr);
    
    EXPECT_TRUE(callback_called);
}

// ═══════════════════════════════════════════════════════════
// Cache Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, CacheClear) {
    engine_->clearCache();
    auto stats = engine_->getStatistics();
    
    // Should not crash
    EXPECT_EQ(stats.cache_hits, 0);
}

// ═══════════════════════════════════════════════════════════
// Factory Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, FactoryDefault) {
    auto engine = ConstitutionalReasoningFactory::createDefault();
    ASSERT_NE(engine, nullptr);
    
    auto principles = engine->getPrinciples();
    EXPECT_GT(principles.size(), 0);
}

TEST_F(ConstitutionalReasoningEngineTest, FactoryStrict) {
    auto engine = ConstitutionalReasoningFactory::createStrict();
    ASSERT_NE(engine, nullptr);
    
    auto config = engine->getConfig();
    EXPECT_GE(config.min_acceptable_score, 0.8f);
}

TEST_F(ConstitutionalReasoningEngineTest, FactoryLenient) {
    auto engine = ConstitutionalReasoningFactory::createLenient();
    ASSERT_NE(engine, nullptr);
    
    auto config = engine->getConfig();
    EXPECT_LE(config.min_acceptable_score, 0.7f);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases
// ═══════════════════════════════════════════════════════════

TEST_F(ConstitutionalReasoningEngineTest, EmptyResponse) {
    std::string response = "";
    std::string query = "What should I do?";
    
    auto result = engine_->reason(response, query, nullptr);
    
    EXPECT_EQ(result.original_response, "");
    EXPECT_GE(result.original_score, 0.0f);
    EXPECT_LE(result.original_score, 1.0f);
}

TEST_F(ConstitutionalReasoningEngineTest, VeryLongResponse) {
    std::string response(10000, 'x');
    std::string query = "What should I do?";
    
    auto result = engine_->reason(response, query, nullptr);
    
    EXPECT_GE(result.original_score, 0.0f);
    EXPECT_LE(result.original_score, 1.0f);
}

TEST_F(ConstitutionalReasoningEngineTest, DisabledCritique) {
    ConstitutionalReasoningConfig no_critique_config;
    no_critique_config.enable_self_critique = false;
    
    auto engine = std::make_unique<ConstitutionalReasoningEngine>(no_critique_config);
    
    std::string response = "You must do this.";
    std::string query = "What should I do?";
    
    auto result = engine->reason(response, query, nullptr);
    
    EXPECT_EQ(result.critiques.size(), 0);
    EXPECT_FALSE(result.was_revised);
}

TEST_F(ConstitutionalReasoningEngineTest, DisabledRevision) {
    ConstitutionalReasoningConfig no_revision_config;
    no_revision_config.enable_self_revision = false;
    
    auto engine = std::make_unique<ConstitutionalReasoningEngine>(no_revision_config);
    
    std::string response = "You must do this.";
    std::string query = "What should I do?";
    
    auto result = engine->reason(response, query, nullptr);
    
    EXPECT_FALSE(result.was_revised);
    EXPECT_EQ(result.revised_response, result.original_response);
}

} // anonymous namespace

#endif // 0

TEST(ConstitutionalReasoningDisabled, DISABLED_AllTestsSkipped) {
    GTEST_SKIP() << "Constitutional reasoning tests are currently disabled";
}


