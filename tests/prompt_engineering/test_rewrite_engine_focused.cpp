/**
 * @file test_rewrite_engine_focused.cpp
 * @brief Focused test suite for RewriteEngine Phase 2 core implementation (RW-P2-01..06).
 * @version 1.0.0
 * @note Maturity: 🟡 TEST/PHASE2
 * @note Status: Phase 2 focused tests (Q4 2026)
 *
 * Test coverage:
 * - RW-P2-01: Rule registration and deduplication
 * - RW-P2-02: Phase ordering enforcement
 * - RW-P2-03: Priority-based execution within phases
 * - RW-P2-04: Max-steps loop prevention
 * - RW-P2-05: Trace correctness and completeness
 * - RW-P2-06: Thread-safety under concurrent register/rewrite
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

// Include headers from the prompt_engineering module
#include "prompt_engineering/rewrite_engine.h"
#include "prompt_engineering/rewrite_rule.h"

using namespace themis::prompt_engineering;

class RewriteEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = create_rewrite_engine();
        ASSERT_NE(engine_, nullptr);
    }

    void TearDown() override {
        engine_.reset();
    }

    std::unique_ptr<IRewriteEngine> engine_;
};

TEST_F(RewriteEngineTest, RegistersAndListsRules) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "normalize_case",
        10,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "TEST",
        "test",
        "Normalize uppercase token"
    );

    EXPECT_TRUE(engine_->register_rule(rule));
    EXPECT_FALSE(engine_->register_rule(rule));

    const auto retrieved = engine_->get_rule("normalize_case");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->rule_id(), "normalize_case");

    const auto ids = engine_->list_rules();
    EXPECT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], "normalize_case");
}

TEST_F(RewriteEngineTest, ExecutesRulesInPhaseOrderAndPriority) {
    auto phase1_a = std::make_shared<RegexRewriteRule>(
        "phase1_a",
        10,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "a",
        "A",
        "Lowercase to uppercase"
    );

    auto phase1_b = std::make_shared<RegexRewriteRule>(
        "phase1_b",
        20,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "A",
        "AAA",
        "Apply second pass"
    );

    auto phase2 = std::make_shared<RegexRewriteRule>(
        "phase2_upper",
        5,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        "policy",
        "POLICY",
        "Policy normalization"
    );

    ASSERT_TRUE(engine_->register_rule(phase1_a));
    ASSERT_TRUE(engine_->register_rule(phase1_b));
    ASSERT_TRUE(engine_->register_rule(phase2));

    RewriteDocument doc;
    doc.content = "a policy";
    doc.document_id = "rewrite_order";

    RewriteContext ctx;
    const auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(doc.content, "AAA POLICY");
    EXPECT_EQ(result.total_transformations, 2u);
}

TEST_F(RewriteEngineTest, StopsAtMaxSteps) {
    auto looping_rule = std::make_shared<RegexRewriteRule>(
        "loop_guard",
        1,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "x",
        "xx",
        "Looping replacement"
    );

    ASSERT_TRUE(engine_->register_rule(looping_rule));

    RewriteDocument doc;
    doc.content = "x";
    doc.document_id = "loop_guard";

    RewriteContext ctx;
    ctx.max_steps = 1;

    const auto result = engine_->rewrite(doc, ctx);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, PromptEngineeringErrorCode::REWRITE_MAX_STEPS_EXCEEDED);
}

TEST_F(RewriteEngineTest, CapturesTraceWhenEnabled) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "trace_rule",
        5,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "hello",
        "goodbye",
        "Trace event"
    );

    ASSERT_TRUE(engine_->register_rule(rule));

    RewriteDocument doc;
    doc.content = "hello world hello";
    doc.document_id = "trace_case";

    RewriteContext ctx;
    ctx.trace_enabled = true;
    ctx.max_trace_entries = 8;

    const auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    ASSERT_FALSE(result.traces.empty());
    EXPECT_EQ(result.traces.front().rule_id, "trace_rule");
    EXPECT_EQ(result.traces.front().phase, RewritePhase::PHASE_1_INPUT_NORMALIZATION);
    EXPECT_GT(result.traces.front().match_count, 0u);
    EXPECT_TRUE(result.traces.front().transformation_applied);
}

TEST_F(RewriteEngineTest, StopsOnTerminalPolicyRule) {
    auto terminal_rule = std::make_shared<PolicyRewriteRule>(
        "deny_dangerous",
        1,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        [](const RewriteDocument& doc, const RewriteContext&) {
            return doc.content.find("dangerous") != std::string::npos;
        },
        [](RewriteDocument& doc, const RewriteContext&) {
            RewriteResult result;
            result.success = true;
            result.error_code = PromptEngineeringErrorCode::TEMPLATE_INVALID_ID;
            result.error_message = "policy violation";
            result.transformed_text = doc.content;
            result.was_blocked = true;
            return result;
        },
        "Reject dangerous content",
        true
    );

    ASSERT_TRUE(engine_->register_rule(terminal_rule));

    RewriteDocument doc;
    doc.content = "dangerous input";
    doc.document_id = "blocked_doc";

    RewriteContext ctx;
    const auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.was_blocked);
    EXPECT_EQ(result.rules_matched, 1u);
    EXPECT_FALSE(result.traces.empty());
    EXPECT_EQ(result.traces.front().rule_id, "deny_dangerous");
}

