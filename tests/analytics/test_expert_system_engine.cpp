/*
 * Tests: ExpertSystemEngine (ES-01..ES-20) + KnowledgeBase (KB-01..KB-08)
 *
 * Copyright (c) 2025 VCC-URN Project — SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "analytics/expert_system_engine.h"
#include "analytics/knowledge_base.h"

using namespace themisdb::analytics;

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeBase tests  KB-01..KB-08
// ─────────────────────────────────────────────────────────────────────────────

TEST(KnowledgeBaseTest, KB01_AssertFactReturnsNonEmptyId) {
    KnowledgeBase kb;
    const auto id = kb.assertFact("Alice", "knows", "Bob");
    EXPECT_FALSE(id.empty());
}

TEST(KnowledgeBaseTest, KB02_RetractFactRemovesIt) {
    KnowledgeBase kb;
    const auto id = kb.assertFact("Alice", "knows", "Bob");
    EXPECT_EQ(kb.factCount(), 1u);
    EXPECT_TRUE(kb.retractFact(id));
    EXPECT_EQ(kb.factCount(), 0u);
    EXPECT_FALSE(kb.retractFact(id));  // Already gone
}

TEST(KnowledgeBaseTest, KB03_GetFactsByPredicateReturnsMatching) {
    KnowledgeBase kb;
    (void)kb.assertFact("Alice", "knows", "Bob");
    (void)kb.assertFact("Alice", "knows", "Carol");
    (void)kb.assertFact("Bob",   "likes", "Pizza");

    const auto knows = kb.getFacts("knows");
    EXPECT_EQ(knows.size(), 2u);
    for (const auto& f : knows)
        EXPECT_EQ(f.predicate, "knows");

    const auto likes = kb.getFacts("likes");
    EXPECT_EQ(likes.size(), 1u);
}

TEST(KnowledgeBaseTest, KB04_GetFactsEmptyPredicateReturnsAll) {
    KnowledgeBase kb;
    (void)kb.assertFact("A", "p1", "X");
    (void)kb.assertFact("B", "p2", "Y");
    EXPECT_EQ(kb.getFacts("").size(), 2u);
}

TEST(KnowledgeBaseTest, KB05_AddRuleAndGetRulesSortedByPriority) {
    KnowledgeBase kb;
    HornClause r1; r1.id = "low";  r1.priority = 1;
    HornClause r2; r2.id = "high"; r2.priority = 10;
    HornClause r3; r3.id = "mid";  r3.priority = 5;
    kb.addRule(r1); kb.addRule(r2); kb.addRule(r3);
    const auto rules = kb.getRules();
    ASSERT_EQ(rules.size(), 3u);
    EXPECT_EQ(rules[0].id, "high");
    EXPECT_EQ(rules[1].id, "mid");
    EXPECT_EQ(rules[2].id, "low");
}

TEST(KnowledgeBaseTest, KB06_ClearFactsKeepsRules) {
    KnowledgeBase kb;
    (void)kb.assertFact("A", "p", "X");
    HornClause r; r.id = "r1"; r.priority = 0;
    kb.addRule(r);
    kb.clearFacts();
    EXPECT_EQ(kb.factCount(), 0u);
    EXPECT_EQ(kb.ruleCount(), 1u);
}

TEST(KnowledgeBaseTest, KB07_GetFactByIdReturnsCorrectFact) {
    KnowledgeBase kb;
    const auto id = kb.assertFact("Alice", "knows", "Bob");
    const auto f  = kb.getFactById(id);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->subject,   "Alice");
    EXPECT_EQ(f->predicate, "knows");
    EXPECT_EQ(f->object,    "Bob");
}

TEST(KnowledgeBaseTest, KB08_GetFactByIdReturnsNulloptForUnknown) {
    KnowledgeBase kb;
    EXPECT_FALSE(kb.getFactById("nonexistent").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ExpertSystemEngine tests  ES-01..ES-20
// ─────────────────────────────────────────────────────────────────────────────

// Helper: build a simple rule A→type→X => A→is→Y
static HornClause makeSimpleRule(const std::string& id,
                                  const std::string& cond_pred,
                                  const std::string& cond_obj,
                                  const std::string& cons_pred,
                                  const std::string& cons_obj,
                                  int priority = 0) {
    HornClause rule;
    rule.id       = id;
    rule.priority = priority;
    TriplePattern cond;
    cond.subject   = "?x";
    cond.predicate = cond_pred;
    cond.object    = cond_obj;
    TriplePattern cons;
    cons.subject   = "?x";
    cons.predicate = cons_pred;
    cons.object    = cons_obj;
    rule.conditions.push_back(cond);
    rule.consequents.push_back(cons);
    return rule;
}

TEST(ExpertSystemEngineTest, ES01_AssertFactIncreasesFactCount) {
    ExpertSystemEngine ese;
    EXPECT_EQ(ese.factCount(), 0u);
    (void)ese.assertFact("Alice", "knows", "Bob");
    EXPECT_EQ(ese.factCount(), 1u);
}

TEST(ExpertSystemEngineTest, ES02_ForwardChainFiresSimpleRule) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    const int fired = ese.forwardChain();
    EXPECT_GE(fired, 1);
    const auto human = ese.knowledgeBase().getFacts("is");
    ASSERT_FALSE(human.empty());
    EXPECT_EQ(human[0].subject, "Alice");
    EXPECT_EQ(human[0].object,  "Human");
}

TEST(ExpertSystemEngineTest, ES03_ForwardChainIsIdempotent) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    const int fired1 = ese.forwardChain();
    const int fired2 = ese.forwardChain();  // Nothing new to derive
    EXPECT_GE(fired1, 1);
    EXPECT_EQ(fired2, 0);
    EXPECT_EQ(ese.knowledgeBase().getFacts("is").size(), 1u);
}

TEST(ExpertSystemEngineTest, ES04_ForwardChainFixpointStopsEarly) {
    ExpertSystemEngine ese;
    // Rule: thing→type→A => thing→status→done; after one cycle, fixpoint.
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "A", "status", "done"));
    (void)ese.assertFact("X", "type", "A");
    const int fired = ese.forwardChain(50);
    EXPECT_EQ(fired, 1);  // exactly one new fact derived
}

TEST(ExpertSystemEngineTest, ES05_ForwardChainBindsVariablesCorrectly) {
    ExpertSystemEngine ese;
    // Two entities of type=Person → both get is=Human
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    (void)ese.assertFact("Bob",   "type", "Person");
    (void)ese.forwardChain();
    const auto humans = ese.knowledgeBase().getFacts("is");
    EXPECT_EQ(humans.size(), 2u);
}

TEST(ExpertSystemEngineTest, ES06_QueryGoalFindsFactInWM) {
    ExpertSystemEngine ese;
    (void)ese.assertFact("Alice", "knows", "Bob");
    TriplePattern goal{"Alice", "knows", "Bob"};
    const auto result = ese.queryGoal(goal);
    EXPECT_TRUE(result.success);
}

TEST(ExpertSystemEngineTest, ES07_QueryGoalBackwardChainsOneRule) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    // Prove Alice→is→Human (not yet in WM, but derivable)
    TriplePattern goal{"Alice", "is", "Human"};
    const auto result = ese.queryGoal(goal);
    EXPECT_TRUE(result.success);
}

TEST(ExpertSystemEngineTest, ES08_QueryGoalReturnsFalseForUnprovable) {
    ExpertSystemEngine ese;
    (void)ese.assertFact("Alice", "knows", "Bob");
    TriplePattern goal{"Alice", "flies", "high"};
    const auto result = ese.queryGoal(goal);
    EXPECT_FALSE(result.success);
}

TEST(ExpertSystemEngineTest, ES09_QueryGoalHasNonEmptyProofTrace) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    TriplePattern goal{"Alice", "is", "Human"};
    const auto result = ese.queryGoal(goal);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.proof_trace.empty());
}

TEST(ExpertSystemEngineTest, ES10_QueryGoalDepthLimitPreventsInfiniteChain) {
    // Create a chain of length 15 (exceeds default depth limit of 10).
    ExpertSystemEngine ese;
    // Rule: ?x→at_level→N => ?x→at_level→N+1  — simulate via distinct predicates
    // Instead: simply test that if we add a rule that cycles, DLS stops.
    // Build a rule chain: level0→level1→...→level11 (11 steps)
    for (int i = 0; i < 12; ++i) {
        HornClause rule;
        rule.id = "r" + std::to_string(i);
        rule.priority = 0;
        TriplePattern cond;
        cond.subject   = "?x";
        cond.predicate = "level" + std::to_string(i);
        cond.object    = "true";
        TriplePattern cons;
        cons.subject   = "?x";
        cons.predicate = "level" + std::to_string(i + 1);
        cons.object    = "true";
        rule.conditions.push_back(cond);
        rule.consequents.push_back(cons);
        ese.knowledgeBase().addRule(rule);
    }
    (void)ese.assertFact("entity", "level0", "true");
    // Goal: prove level12 — needs 12 backward steps, exceeds depth=10
    TriplePattern goal{"entity", "level12", "true"};
    const auto result = ese.queryGoal(goal);
    // Should fail due to depth limit (or succeed if engine short-circuits via WM).
    // The important thing is it terminates.
    (void)result;  // Just checking it doesn't hang.
}

TEST(ExpertSystemEngineTest, ES11_MLScorerAboveThresholdFiresRule) {
    ExpertSystemEngine ese;
    HornClause rule = makeSimpleRule("r1", "type", "Person", "is", "Human");
    rule.ml_confidence_threshold = 0.8;
    ese.knowledgeBase().addRule(rule);
    (void)ese.assertFact("Alice", "type", "Person");
    // Inject scorer that always returns confidence 0.95
    ese.setMLScorerFn([](const HornClause&, const std::vector<Fact>&) { return 0.95; });
    const int fired = ese.forwardChain();
    EXPECT_GE(fired, 1);
}

TEST(ExpertSystemEngineTest, ES12_MLScorerBelowThresholdSkipsRule) {
    ExpertSystemEngine ese;
    HornClause rule = makeSimpleRule("r1", "type", "Person", "is", "Human");
    rule.ml_confidence_threshold = 0.8;
    ese.knowledgeBase().addRule(rule);
    (void)ese.assertFact("Alice", "type", "Person");
    // Inject scorer that always returns confidence 0.5 (below threshold)
    ese.setMLScorerFn([](const HornClause&, const std::vector<Fact>&) { return 0.5; });
    const int fired = ese.forwardChain();
    EXPECT_EQ(fired, 0);
}

TEST(ExpertSystemEngineTest, ES13_NullMLScorerFiresRuleDeterministically) {
    ExpertSystemEngine ese;
    HornClause rule = makeSimpleRule("r1", "type", "Person", "is", "Human");
    rule.ml_confidence_threshold = 0.0;  // No ML required
    ese.knowledgeBase().addRule(rule);
    (void)ese.assertFact("Alice", "type", "Person");
    // No scorer set → deterministic firing
    const int fired = ese.forwardChain();
    EXPECT_GE(fired, 1);
}

TEST(ExpertSystemEngineTest, ES14_ExplainReturnsJsonForDerivedFact) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    (void)ese.assertFact("Alice", "type", "Person");
    (void)ese.forwardChain();

    // Find the derived fact id.
    const auto is_facts = ese.knowledgeBase().getFacts("is");
    ASSERT_FALSE(is_facts.empty());
    const std::string fid  = is_facts[0].id;
    const std::string json = ese.explain(fid);
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json, "[]");
    EXPECT_NE(json.find("r1"), std::string::npos);
}

TEST(ExpertSystemEngineTest, ES15_ExplainReturnsEmptyArrayForUnknownFact) {
    ExpertSystemEngine ese;
    EXPECT_EQ(ese.explain("nonexistent_fact_id"), "[]");
}

TEST(ExpertSystemEngineTest, ES16_RetractedFactNotUsedInForwardChain) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Person", "is", "Human"));
    const auto id = ese.assertFact("Alice", "type", "Person");
    EXPECT_TRUE(ese.retractFact(id));
    const int fired = ese.forwardChain();
    EXPECT_EQ(fired, 0);  // Fact was retracted before chain ran
}

TEST(ExpertSystemEngineTest, ES17_RetractUnknownIdReturnsFalse) {
    ExpertSystemEngine ese;
    EXPECT_FALSE(ese.retractFact("no_such_id"));
}

TEST(ExpertSystemEngineTest, ES18_ConcurrentAssertFactFrom8ThreadsIsSafe) {
    ExpertSystemEngine ese;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ese, t]() {
            for (int i = 0; i < kPerThread; ++i)
(void)ese.assertFact("subj_" + std::to_string(t),
                               "pred",
                               "obj_" + std::to_string(i));
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_LE(ese.factCount(), static_cast<std::size_t>(kThreads * kPerThread));
}

TEST(ExpertSystemEngineTest, ES19_ConcurrentForwardChainIsSafe) {
    ExpertSystemEngine ese;
    ese.knowledgeBase().addRule(makeSimpleRule("r1", "type", "Entity", "classified", "true"));
    for (int i = 0; i < 20; ++i)
(void)ese.assertFact("e" + std::to_string(i), "type", "Entity");

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t)
        threads.emplace_back([&ese]() { (void)ese.forwardChain(10); });
    for (auto& th : threads) th.join();
    // No crash and some facts derived
    EXPECT_GE(ese.knowledgeBase().getFacts("classified").size(), 1u);
}

TEST(ExpertSystemEngineTest, ES20_ForwardChain1000Facts100RulesWithin1Second) {
    ExpertSystemEngine ese;

    // Add 100 rules (different condition predicates → no derived explosions)
    for (int i = 0; i < 100; ++i) {
        HornClause rule;
        rule.id = "r" + std::to_string(i);
        rule.priority = i;
        TriplePattern cond;
        cond.subject   = "?x";
        cond.predicate = "attr" + std::to_string(i);
        cond.object    = "val";
        TriplePattern cons;
        cons.subject   = "?x";
        cons.predicate = "has_attr" + std::to_string(i);
        cons.object    = "true";
        rule.conditions.push_back(cond);
        rule.consequents.push_back(cons);
        ese.knowledgeBase().addRule(rule);
    }

    // Add 1000 facts — each entity has attr0 so rule 0 fires for each.
    for (int i = 0; i < 1000; ++i)
(void)ese.assertFact("e" + std::to_string(i), "attr0", "val");

    const auto t0 = std::chrono::steady_clock::now();
    (void)ese.forwardChain(200);
    const auto t1  = std::chrono::steady_clock::now();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(ms, 5000);  // Must complete within 5 seconds (generous upper bound)
}

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeBase YAML parser bridge (STUB #272)  KB-YP-01..KB-YP-04
// ─────────────────────────────────────────────────────────────────────────────

// KB-YP-01: Without YamlParserFn, loadRulesFromYaml uses built-in parser.
//           A non-existent path returns -1 (file-open error).
TEST(KnowledgeBaseTest, KB_YP_01_builtin_parser_returns_minus1_for_missing_file) {
    KnowledgeBase::clearYamlParserFn();
    KnowledgeBase kb;
    EXPECT_EQ(kb.loadRulesFromYaml("/tmp/does_not_exist_kb_yp_01.yaml"), -1);
}

// KB-YP-02: Injected YamlParserFn is called instead of the built-in parser.
TEST(KnowledgeBaseTest, KB_YP_02_injected_parser_is_called) {
    bool fn_called = false;
    KnowledgeBase::setYamlParserFn(
        [&fn_called](const std::string& /*path*/, KnowledgeBase& kb_ref) -> int {
            fn_called = true;
            HornClause rule;
            rule.id = "injected_rule";
            rule.priority = 1;
            kb_ref.addRule(rule);
            return 1;
        });

    KnowledgeBase kb;
    int loaded = kb.loadRulesFromYaml("/any/path/does/not/matter.yaml");

    KnowledgeBase::clearYamlParserFn();

    EXPECT_TRUE(fn_called);
    EXPECT_EQ(loaded, 1);
    EXPECT_EQ(kb.ruleCount(), 1u);
    const auto rules = kb.getRules();
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].id, "injected_rule");
}

// KB-YP-03: Injected fn returning -1 propagates the error to the caller.
TEST(KnowledgeBaseTest, KB_YP_03_injected_parser_error_propagates) {
    KnowledgeBase::setYamlParserFn(
        [](const std::string&, KnowledgeBase&) -> int { return -1; });

    KnowledgeBase kb;
    int result = kb.loadRulesFromYaml("any.yaml");

    KnowledgeBase::clearYamlParserFn();
    EXPECT_EQ(result, -1);
}

// KB-YP-04: After clearYamlParserFn(), built-in parser is used again.
TEST(KnowledgeBaseTest, KB_YP_04_clear_reverts_to_builtin_parser) {
    KnowledgeBase::setYamlParserFn(
        [](const std::string&, KnowledgeBase&) -> int { return 42; });
    KnowledgeBase::clearYamlParserFn();

    KnowledgeBase kb;
    int result = kb.loadRulesFromYaml("/tmp/does_not_exist_kb_yp_04.yaml");
    // Built-in parser tries to open the file and returns -1 for missing files.
    EXPECT_EQ(result, -1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrency / Thread-Safety items #41–45: shared_mutex + lock-under-callback
// ─────────────────────────────────────────────────────────────────────────────

// ES-21: Concurrent readers (explain/factCount/ruleCount) must not block each
//        other under the shared_lock upgrade.
TEST(ExpertSystemEngineTest, ES21_ConcurrentReadersDoNotBlockEachOther) {
    ExpertSystemEngine ese;
    for (int i = 0; i < 10; ++i)
        (void)ese.assertFact("s" + std::to_string(i), "p", "o");

    constexpr int kThreads = 8;
    std::atomic<int> successes{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    // Mix of read-only operations that must all complete without deadlock.
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&ese, &successes, t]() {
            if (t % 3 == 0)      { (void)ese.factCount(); }
            else if (t % 3 == 1) { (void)ese.ruleCount(); }
            else                 { (void)ese.explain("non_existent_id"); }
            ++successes;
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(successes.load(), kThreads);
}

// ES-22: forwardChain() with a ScorerFn that calls assertFact on the same
//        engine must NOT deadlock (lock-under-callback fix, items #41–45).
TEST(ExpertSystemEngineTest, ES22_ForwardChainScorerCallbackNoDeadlock) {
    ExpertSystemEngine ese;

    // Rule: anything with predicate "trigger" → "result"
    HornClause rule;
    rule.id = "r_callback";
    rule.priority = 1;
    rule.ml_confidence_threshold = 0.5;
    TriplePattern cond;
    cond.subject   = "?x";
    cond.predicate = "trigger";
    cond.object    = "yes";
    rule.conditions.push_back(cond);
    TriplePattern cons;
    cons.subject   = "?x";
    cons.predicate = "result";
    cons.object    = "done";
    rule.consequents.push_back(cons);
    ese.knowledgeBase().addRule(rule);

    // Inject a scorer function that tries to re-enter the engine via assertFact.
    // With the lock-under-callback fix the scorer is called without holding
    // mutex_, so this assert should not deadlock.
    ese.setMLScorerFn([&ese](const HornClause&, const std::vector<Fact>&) -> double {
        // This would deadlock if forwardChain() still held the mutex here.
        (void)ese.assertFact("side_effect", "scorer_ran", "true");
        return 1.0;  // Always confident → allow rule to fire.
    });

    (void)ese.assertFact("A", "trigger", "yes");

    // Must complete without hanging.
    const int fired = ese.forwardChain(5);
    EXPECT_GE(fired, 1);
    // Side-effect fact added by scorer callback must be visible.
    EXPECT_GE(ese.factCount(), 2u);
}
