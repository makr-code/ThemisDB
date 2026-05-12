/**
 * @file test_agentic_rag_relay.cpp
 * @brief Unit tests for the AgenticRAG ↔ RoundTripSimulator relay guard.
 *
 * Tests the DELEGATE-52 pre-production safety net that fires a
 * RoundTripSimulator relay after an AgenticRAG::run() session completes.
 *
 * Test IDs: ARR-01 … ARR-04
 *
 *   ARR-01  No relay_guard configured → delegate_relay is std::nullopt.
 *   ARR-02  Relay guard with identity EditFn → delegate_relay populated,
 *           RS@1 = 1.0, stop_reason == COMPLETED_NORMALLY.
 *   ARR-03  Relay guard with destructive EditFn → delegate_relay populated,
 *           RS@1 = 0.0 and fully_catastrophic == true (threshold default 0.80).
 *   ARR-04  Relay guard configured but edit_pairs empty → guard not ready
 *           (relay_guard stays nullopt because guard is not executable).
 *
 * Scientific basis: Laban et al., "LLMs Corrupt Your Documents When You
 * Delegate" (arXiv:2604.15597).
 */

#include "rag/agentic_rag.h"
#include "rag/delegate_evaluator.h"

#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

using namespace themis::rag::agentic;
using namespace themis::rag::delegate_eval;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static themis::rag::judge::RetrievedDocument makeDoc(const std::string& id,
                                                     const std::string& content) {
    themis::rag::judge::RetrievedDocument d;
    d.id = id;
    d.content = content;
    d.similarity_score = 1.0;
    return d;
}

static EditFn identityFn() {
    return [](const std::string& doc, const std::string& /*instr*/) {
        return doc;
    };
}

static EditFn destructiveFn() {
    return [](const std::string& /*doc*/, const std::string& /*instr*/) {
        return std::string{};
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// ARR-01  No relay guard → delegate_relay is nullopt
// ─────────────────────────────────────────────────────────────────────────────

TEST(AgenticRAGRelayTest, ARR01_NoGuard_RelayIsNullopt) {
    AgenticRAGConfig cfg;
    cfg.max_iterations = 1;
    AgenticRAG agent(cfg);

    auto result = agent.run("query", {makeDoc("d1", "hello world")});

    EXPECT_FALSE(result.delegate_relay.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ARR-02  Identity EditFn → RS@1 = 1.0, stop_reason == COMPLETED_NORMALLY
// ─────────────────────────────────────────────────────────────────────────────

TEST(AgenticRAGRelayTest, ARR02_IdentityFn_RSIsOne) {
    PlainTextEvaluator evaluator;
    DelegateEvaluatorConfig sim_cfg;
    sim_cfg.num_round_trips = 1;
    RoundTripSimulator simulator(sim_cfg);

    RoundTripEditPair pair{"change something", "undo the change",
                           "seed", DomainType::PLAIN_TEXT};

    AgenticRAGConfig cfg;
    cfg.max_iterations = 1;
    AgenticRAGConfig::RelayGuardConfig guard;
    guard.simulator = &simulator;
    guard.evaluator = &evaluator;
    guard.edit_pairs = {pair};
    guard.edit_fn = identityFn();
    cfg.relay_guard = std::move(guard);

    AgenticRAG agent(cfg);
    auto result = agent.run("query", {makeDoc("d1", "hello world")});

    ASSERT_TRUE(result.delegate_relay.has_value());
    const auto& relay = *result.delegate_relay;
    EXPECT_EQ(relay.stop_reason, themis::rag::delegate_eval::StopReason::COMPLETED_NORMALLY);
    ASSERT_FALSE(relay.scores.rs_per_interaction.empty());
    EXPECT_NEAR(relay.scores.rs_per_interaction.front(), 1.0, 1e-9);
    EXPECT_EQ(relay.catastrophic_corruption_count, 0u);
    EXPECT_FALSE(relay.fully_catastrophic);
}

// ─────────────────────────────────────────────────────────────────────────────
// ARR-03  Destructive EditFn → RS@1 = 0.0, fully_catastrophic == true
// ─────────────────────────────────────────────────────────────────────────────

TEST(AgenticRAGRelayTest, ARR03_DestructiveFn_FullyCatastrophic) {
    PlainTextEvaluator evaluator;
    DelegateEvaluatorConfig sim_cfg;
    sim_cfg.num_round_trips = 1;
    RoundTripSimulator simulator(sim_cfg);

    RoundTripEditPair pair{"delete all", "restore all",
                           "seed", DomainType::PLAIN_TEXT};

    AgenticRAGConfig cfg;
    cfg.max_iterations = 1;
    AgenticRAGConfig::RelayGuardConfig guard;
    guard.simulator = &simulator;
    guard.evaluator = &evaluator;
    guard.edit_pairs = {pair};
    guard.edit_fn = destructiveFn();
    cfg.relay_guard = std::move(guard);

    AgenticRAG agent(cfg);
    auto result = agent.run("query", {makeDoc("d1", "important content")});

    ASSERT_TRUE(result.delegate_relay.has_value());
    const auto& relay = *result.delegate_relay;
    ASSERT_FALSE(relay.scores.rs_per_interaction.empty());
    EXPECT_NEAR(relay.scores.rs_per_interaction.front(), 0.0, 1e-9);
    EXPECT_TRUE(relay.fully_catastrophic);
}

// ─────────────────────────────────────────────────────────────────────────────
// ARR-04  Guard with empty edit_pairs → relay guard is not ready, nullopt
// ─────────────────────────────────────────────────────────────────────────────

TEST(AgenticRAGRelayTest, ARR04_EmptyEditPairs_GuardNotReady) {
    PlainTextEvaluator evaluator;
    DelegateEvaluatorConfig sim_cfg;
    sim_cfg.num_round_trips = 1;
    RoundTripSimulator simulator(sim_cfg);

    AgenticRAGConfig cfg;
    cfg.max_iterations = 1;
    AgenticRAGConfig::RelayGuardConfig guard;
    guard.simulator = &simulator;
    guard.evaluator = &evaluator;
    guard.edit_pairs = {}; // empty — guard must NOT fire
    guard.edit_fn = identityFn();
    cfg.relay_guard = std::move(guard);

    AgenticRAG agent(cfg);
    auto result = agent.run("query", {makeDoc("d1", "content")});

    // Guard is configured but not ready (empty edit_pairs) → nullopt.
    EXPECT_FALSE(result.delegate_relay.has_value());
}
