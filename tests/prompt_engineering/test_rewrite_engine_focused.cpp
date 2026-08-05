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

// ============================================================================
// Test Fixture
// ============================================================================

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

// ============================================================================
// RW-P2-01: Rule Registration and Deduplication
// ============================================================================

TEST_F(RewriteEngineTest, RWP201_RegisterRuleSuccess) {
    // Register a simple regex rule
    auto rule = std::make_shared<RegexRewriteRule>(
        "rule_001",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "hello",
        "goodbye",
        "Replace hello with goodbye"
    );

    EXPECT_TRUE(engine_->register_rule(rule));

    // Verify rule is retrievable
    auto retrieved = engine_->get_rule("rule_001");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->rule_id(), "rule_001");
}

TEST_F(RewriteEngineTest, RWP201_DuplicateRuleRejected) {
    auto rule1 = std::make_shared<RegexRewriteRule>(
        "duplicate_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "foo",
        "bar",
        "First registration"
    );

    auto rule2 = std::make_shared<RegexRewriteRule>(
        "duplicate_rule",
        101,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        "baz",
        "qux",
        "Second registration (should fail)"
    );

    EXPECT_TRUE(engine_->register_rule(rule1));
    EXPECT_FALSE(engine_->register_rule(rule2));  // Should reject duplicate ID

    // Verify first rule is still registered
    auto retrieved = engine_->get_rule("duplicate_rule");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->description(), "First registration");
}

TEST_F(RewriteEngineTest, RWP201_UnregisterRule) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "rule_to_remove",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "test",
        "removed",
        "Will be removed"
    );

    EXPECT_TRUE(engine_->register_rule(rule));
    EXPECT_NE(engine_->get_rule("rule_to_remove"), nullptr);

    EXPECT_TRUE(engine_->unregister_rule("rule_to_remove"));
    EXPECT_EQ(engine_->get_rule("rule_to_remove"), nullptr);
}

TEST_F(RewriteEngineTest, RWP201_ListRules) {
    auto rule1 = std::make_shared<RegexRewriteRule>(
        "rule_a",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "a",
        "A",
        "First rule"
    );

    auto rule2 = std::make_shared<RegexRewriteRule>(
        "rule_b",
        101,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        "b",
        "B",
        "Second rule"
    );

    engine_->register_rule(rule1);
    engine_->register_rule(rule2);

    auto rules = engine_->list_rules();
    EXPECT_EQ(rules.size(), 2);
    EXPECT_TRUE(std::find(rules.begin(), rules.end(), "rule_a") != rules.end());
    EXPECT_TRUE(std::find(rules.begin(), rules.end(), "rule_b") != rules.end());
}

// ============================================================================
// RW-P2-02: Phase Ordering Enforcement
// ============================================================================

TEST_F(RewriteEngineTest, RWP202_PhaseOrderingEnforced) {
    // Register rules in different phases
    auto phase1_rule = std::make_shared<RegexRewriteRule>(
        "phase1",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "TEST",
        "test",
        "Phase 1 normalization"
    );

    auto phase2_rule = std::make_shared<RegexRewriteRule>(
        "phase2",
        100,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        "policy",
        "POLICY",
        "Phase 2 enforcement"
    );

    engine_->register_rule(phase1_rule);
    engine_->register_rule(phase2_rule);

    RewriteDocument doc;
    doc.content = "TEST policy";
    doc.document_id = "test_doc";

    RewriteContext ctx;

    // Execute rewrite - phases should execute in order 1->2->3->4
    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    // Phase 1 should have normalized TEST to test
    // Phase 2 should have transformed policy to POLICY
    EXPECT_EQ(doc.content, "test POLICY");
}

// ============================================================================
// RW-P2-03: Priority-Based Execution Within Phases
// ============================================================================

TEST_F(RewriteEngineTest, RWP203_PriorityOrdering) {
    // Register rules with different priorities in same phase
    // Lower priority value = executes first

    auto priority_10 = std::make_shared<RegexRewriteRule>(
        "rule_priority_10",
        10,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "a",
        "A",
        "Priority 10 (should execute first)"
    );

    auto priority_20 = std::make_shared<RegexRewriteRule>(
        "rule_priority_20",
        20,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "A",
        "AAA",
        "Priority 20 (should execute second)"
    );

    engine_->register_rule(priority_10);
    engine_->register_rule(priority_20);

    RewriteDocument doc;
    doc.content = "a";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    // If priority 10 executes first: a -> A, then priority 20: A -> AAA
    // Final result should be AAA
    EXPECT_EQ(doc.content, "AAA");
}

// ============================================================================
// RW-P2-04: Max-Steps Loop Prevention
// ============================================================================

TEST_F(RewriteEngineTest, RWP204_MaxStepsEnforced) {
    // Create a rule that could potentially loop (but we limit steps)
    auto looping_rule = std::make_shared<RegexRewriteRule>(
        "potentially_looping",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "x",
        "xx",  // This could expand unboundedly
        "Rule that doubles x's"
    );

    engine_->register_rule(looping_rule);

    RewriteDocument doc;
    doc.content = "x";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    ctx.max_steps = 10;  // Set low limit

    auto result = engine_->rewrite(doc, ctx);

    // Should either succeed with limited expansions or stop at max steps
    // For this regex rule with max_replacements not set, it should keep expanding
    // until max_steps is hit
    EXPECT_TRUE(result.success || result.error_code == PromptEngineeringErrorCode::REWRITE_MAX_STEPS_EXCEEDED);
}

TEST_F(RewriteEngineTest, RWP204_MaxStepsDefault) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "simple_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "test",
        "result",
        "Simple transformation"
    );

    engine_->register_rule(rule);

    RewriteDocument doc;
    doc.content = "test";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    // Don't set max_steps - should use default (1000)

    auto result = engine_->rewrite(doc, ctx);
    EXPECT_TRUE(result.success);
}

// ============================================================================
// RW-P2-05: Trace Correctness and Completeness
// ============================================================================

TEST_F(RewriteEngineTest, RWP205_TraceRecordsMatches) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "traced_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "hello",
        "goodbye",
        "Should be traced"
    );

    engine_->register_rule(rule);

    RewriteDocument doc;
    doc.content = "hello world hello";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    ctx.trace_enabled = true;

    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.traces.size(), 0);
    EXPECT_EQ(result.traces[0].rule_id, "traced_rule");
    EXPECT_EQ(result.traces[0].phase, RewritePhase::PHASE_1_INPUT_NORMALIZATION);
    EXPECT_GT(result.traces[0].match_count, 0);
}

TEST_F(RewriteEngineTest, RWP205_TraceDisabled) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "no_trace_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "x",
        "y",
        "Should not be traced"
    );

    engine_->register_rule(rule);

    RewriteDocument doc;
    doc.content = "x";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    ctx.trace_enabled = false;

    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    // Traces should be empty when disabled
    EXPECT_EQ(result.traces.size(), 0);
}

TEST_F(RewriteEngineTest, RWP205_TraceCapturesTransformation) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "capture_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "foo",
        "bar",
        "Captures transformation"
    );

    engine_->register_rule(rule);

    RewriteDocument doc;
    doc.content = "foo";
    doc.document_id = "test_doc";

    RewriteContext ctx;
    ctx.trace_enabled = true;

    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    ASSERT_GT(result.traces.size(), 0);
    
    const auto& trace = result.traces[0];
    EXPECT_EQ(trace.rule_id, "capture_rule");
    EXPECT_EQ(trace.matched_text, "foo");
    EXPECT_TRUE(trace.transformation_applied);
    EXPECT_GT(trace.rule_latency_micros, 0);
}

// ============================================================================
// RW-P2-06: Thread-Safety Under Concurrent Register/Rewrite
// ============================================================================

TEST_F(RewriteEngineTest, RWP206_ConcurrentRuleRegistration) {
    std::vector<std::thread> threads;
    std::atomic<int> successful_registrations(0);

    // Register 10 different rules concurrently
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &successful_registrations, i]() {
            auto rule = std::make_shared<RegexRewriteRule>(
                "thread_rule_" + std::to_string(i),
                static_cast<uint8_t>(i),
                RewritePhase::PHASE_1_INPUT_NORMALIZATION,
                "pattern_" + std::to_string(i),
                "replacement_" + std::to_string(i),
                "Thread-registered rule"
            );

            if (engine_->register_rule(rule)) {
                successful_registrations++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successful_registrations, 10);
    auto rules = engine_->list_rules();
    EXPECT_EQ(rules.size(), 10);
}

TEST_F(RewriteEngineTest, RWP206_ConcurrentRewrite) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "concurrent_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "a",
        "b",
        "Used for concurrent rewrite"
    );

    engine_->register_rule(rule);

    std::vector<std::thread> threads;
    std::atomic<int> successful_rewrites(0);

    // Execute 5 concurrent rewrites
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &successful_rewrites]() {
            RewriteDocument doc;
            doc.content = "a";
            doc.document_id = "concurrent_doc";

            RewriteContext ctx;
            auto result = engine_->rewrite(doc, ctx);

            if (result.success) {
                successful_rewrites++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successful_rewrites, 5);
}

TEST_F(RewriteEngineTest, RWP206_ConcurrentRegisterAndRewrite) {
    // Mix concurrent registration and rewrite operations
    std::atomic<int> operations_completed(0);
    std::vector<std::thread> threads;

    // Thread 0: Rewrite
    // Thread 1: Register rule
    // Thread 2: Rewrite
    // etc.

    for (int i = 0; i < 6; ++i) {
        if (i % 2 == 0) {
            // Register thread
            threads.emplace_back([this, &operations_completed, i]() {
                auto rule = std::make_shared<RegexRewriteRule>(
                    "mixed_rule_" + std::to_string(i),
                    static_cast<uint8_t>(i),
                    RewritePhase::PHASE_1_INPUT_NORMALIZATION,
                    "test",
                    "done",
                    "Mixed concurrent rule"
                );

                if (engine_->register_rule(rule)) {
                    operations_completed++;
                }
            });
        } else {
            // Rewrite thread
            threads.emplace_back([this, &operations_completed]() {
                RewriteDocument doc;
                doc.content = "test";
                doc.document_id = "mixed_doc";

                RewriteContext ctx;
                auto result = engine_->rewrite(doc, ctx);

                if (result.success) {
                    operations_completed++;
                }
            });
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(operations_completed, 6);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(RewriteEngineTest, IntegrationMultiPhaseExecution) {
    // Register rules in all phases
    auto rule1 = std::make_shared<RegexRewriteRule>(
        "phase1_normalize",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "TEST",
        "test",
        "Normalize to lowercase"
    );

    auto rule2 = std::make_shared<RegexRewriteRule>(
        "phase2_policy",
        100,
        RewritePhase::PHASE_2_POLICY_ENFORCEMENT,
        "dangerous",
        "safe",
        "Apply safety policy"
    );

    auto rule3 = std::make_shared<RegexRewriteRule>(
        "phase4_cleanup",
        100,
        RewritePhase::PHASE_4_POST_GENERATION,
        "  ",
        " ",
        "Normalize whitespace"
    );

    engine_->register_rule(rule1);
    engine_->register_rule(rule2);
    engine_->register_rule(rule3);

    RewriteDocument doc;
    doc.content = "TEST dangerous  code";
    doc.document_id = "integration_test";

    RewriteContext ctx;
    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    // After phase 1: "test dangerous  code"
    // After phase 2: "test safe  code"
    // After phase 4: "test safe code"
    EXPECT_EQ(doc.content, "test safe code");
}

TEST_F(RewriteEngineTest, IntegrationDictionaryRule) {
    std::unordered_map<std::string, std::string> mappings{
        {"hello", "hola"},
        {"world", "mundo"}
    };

    auto dict_rule = std::make_shared<DictionaryRewriteRule>(
        "translation_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        mappings,
        "Translate to Spanish"
    );

    engine_->register_rule(dict_rule);

    RewriteDocument doc;
    doc.content = "hello world";
    doc.document_id = "dict_test";

    RewriteContext ctx;
    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(doc.content, "hola mundo");
}

// ============================================================================
// Statistics
// ============================================================================

TEST_F(RewriteEngineTest, StatsTracking) {
    auto rule = std::make_shared<RegexRewriteRule>(
        "stats_rule",
        100,
        RewritePhase::PHASE_1_INPUT_NORMALIZATION,
        "x",
        "y",
        "For stats"
    );

    engine_->register_rule(rule);

    RewriteDocument doc;
    doc.content = "x";
    doc.document_id = "stats_doc";

    RewriteContext ctx;
    auto result = engine_->rewrite(doc, ctx);

    EXPECT_TRUE(result.success);

    // Get stats as JSON
    auto stats_json = engine_->get_stats_json();
    EXPECT_NE(stats_json, "");
    EXPECT_TRUE(stats_json.find("total_rules_registered") != std::string::npos);
}

} // namespace test
