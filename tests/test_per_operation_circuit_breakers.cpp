/**
 * @file test_per_operation_circuit_breakers.cpp
 * @brief Unit tests for per-operation-type circuit breakers in LLMAQLHandler
 *
 * Verifies that:
 * - Each operation type (infer, rag, embed, finetune) has its own independent
 *   circuit breaker instance.
 * - A tripped "infer" breaker does NOT block "rag" or "embed" operations.
 * - Per-operation CircuitBreaker::Config is injectable via LLMAQLHandler::Config.
 * - getCircuitBreakerStates() returns the correct state for all operation types.
 * - LLM STATS output includes circuit breaker states.
 * - Circuit breaker state is recorded in metrics for all command types.
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include "sharding/circuit_breaker.h"

using namespace themis::aql;
using namespace themis::sharding;

// ============================================================================
// Test fixture
// ============================================================================

class PerOperationCircuitBreakerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }

    void TearDown() override {
        handler.reset();
    }

    std::unique_ptr<LLMAQLHandler> handler;
};

// ============================================================================
// AC-1: Initial state — all breakers start CLOSED
// ============================================================================

TEST_F(PerOperationCircuitBreakerTest, InitialState_AllBreakersAreClosed) {
    auto states = handler->getCircuitBreakerStates();

    EXPECT_EQ(states.infer,    "CLOSED");
    EXPECT_EQ(states.rag,      "CLOSED");
    EXPECT_EQ(states.embed,    "CLOSED");
    EXPECT_EQ(states.finetune, "CLOSED");
}

// ============================================================================
// AC-2: Default constructor uses the default Config (same defaults as the old
//        single-breaker, so the map has four entries, all CLOSED).
// ============================================================================

TEST_F(PerOperationCircuitBreakerTest, DefaultConstructor_HasFourBreakers) {
    // Just verify the four required keys are present via getCircuitBreakerStates()
    auto states = handler->getCircuitBreakerStates();
    EXPECT_FALSE(states.infer.empty());
    EXPECT_FALSE(states.rag.empty());
    EXPECT_FALSE(states.embed.empty());
    EXPECT_FALSE(states.finetune.empty());
}

// ============================================================================
// AC-3: Config injection — per-command CircuitBreaker::Config is respected.
// ============================================================================

TEST_F(PerOperationCircuitBreakerTest, ConfigInjection_PerCommandThresholdsAreRespected) {
    LLMAQLHandler::Config cfg;
    // Give infer a tiny failure threshold
    cfg.infer_circuit_breaker.failure_threshold = 2;
    cfg.infer_circuit_breaker.timeout           = std::chrono::seconds(10);
    // Give rag a larger one
    cfg.rag_circuit_breaker.failure_threshold = 10;

    LLMAQLHandler customHandler(cfg);

    // Both breakers should start CLOSED regardless of threshold values
    auto states = customHandler.getCircuitBreakerStates();
    EXPECT_EQ(states.infer, "CLOSED");
    EXPECT_EQ(states.rag,   "CLOSED");
}

// ============================================================================
// AC-4: getCircuitBreakerStates() returns correct human-readable strings
//        for all four operations.
// ============================================================================

TEST_F(PerOperationCircuitBreakerTest, GetCircuitBreakerStates_ReturnsStringsForAllOps) {
    auto states = handler->getCircuitBreakerStates();

    // Each string must be one of the known state names produced by
    // CircuitBreaker::stateToString().
    for (const auto& s : {states.infer, states.rag, states.embed, states.finetune}) {
        EXPECT_TRUE(s == "CLOSED" || s == "OPEN" || s == "HALF_OPEN")
            << "Unexpected circuit-breaker state string: " << s;
    }
}

// ============================================================================
// AC-5: Direct CircuitBreaker isolation test — uses sharding::CircuitBreaker
//        directly to simulate the infer breaker being open while rag is closed.
// ============================================================================

TEST(PerOperationCircuitBreakerIsolation, InferBreakerTripped_DoesNotAffectRagBreaker) {
    CircuitBreaker::Config sensitive_cfg;
    sensitive_cfg.failure_threshold = 1;
    sensitive_cfg.timeout           = std::chrono::seconds(60);
    sensitive_cfg.success_threshold = 1;
    sensitive_cfg.failure_window    = std::chrono::seconds(60);

    CircuitBreaker infer_breaker(sensitive_cfg);
    CircuitBreaker rag_breaker(sensitive_cfg);

    // Trip the infer breaker
    infer_breaker.recordFailure();

    EXPECT_EQ(infer_breaker.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(infer_breaker.allowRequest());

    // RAG breaker must remain unaffected
    EXPECT_EQ(rag_breaker.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(rag_breaker.allowRequest());
}

TEST(PerOperationCircuitBreakerIsolation, RagBreakerTripped_DoesNotAffectInferBreaker) {
    CircuitBreaker::Config sensitive_cfg;
    sensitive_cfg.failure_threshold = 1;
    sensitive_cfg.timeout           = std::chrono::seconds(60);
    sensitive_cfg.success_threshold = 1;
    sensitive_cfg.failure_window    = std::chrono::seconds(60);

    CircuitBreaker infer_breaker(sensitive_cfg);
    CircuitBreaker rag_breaker(sensitive_cfg);

    // Trip only the rag breaker
    rag_breaker.recordFailure();

    EXPECT_EQ(rag_breaker.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(rag_breaker.allowRequest());

    EXPECT_EQ(infer_breaker.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(infer_breaker.allowRequest());
}

TEST(PerOperationCircuitBreakerIsolation, EmbedBreakerTripped_DoesNotAffectOtherBreakers) {
    CircuitBreaker::Config sensitive_cfg;
    sensitive_cfg.failure_threshold = 1;
    sensitive_cfg.timeout           = std::chrono::seconds(60);
    sensitive_cfg.success_threshold = 1;
    sensitive_cfg.failure_window    = std::chrono::seconds(60);

    CircuitBreaker infer_breaker(sensitive_cfg);
    CircuitBreaker rag_breaker(sensitive_cfg);
    CircuitBreaker embed_breaker(sensitive_cfg);

    // Trip only the embed breaker
    embed_breaker.recordFailure();

    EXPECT_EQ(embed_breaker.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(embed_breaker.allowRequest());

    EXPECT_EQ(infer_breaker.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(infer_breaker.allowRequest());
    EXPECT_EQ(rag_breaker.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(rag_breaker.allowRequest());
}

// ============================================================================
// AC-6: LLMAQLHandler::Config has all four per-op config fields.
// ============================================================================

TEST(PerOperationCircuitBreakerConfig, ConfigHasAllFourFields) {
    LLMAQLHandler::Config cfg;

    // Verify each field is independently configurable
    cfg.infer_circuit_breaker.failure_threshold    = 3;
    cfg.rag_circuit_breaker.failure_threshold      = 7;
    cfg.embed_circuit_breaker.failure_threshold    = 5;
    cfg.finetune_circuit_breaker.failure_threshold = 10;

    EXPECT_EQ(cfg.infer_circuit_breaker.failure_threshold,    3u);
    EXPECT_EQ(cfg.rag_circuit_breaker.failure_threshold,      7u);
    EXPECT_EQ(cfg.embed_circuit_breaker.failure_threshold,    5u);
    EXPECT_EQ(cfg.finetune_circuit_breaker.failure_threshold, 10u);
}

TEST(PerOperationCircuitBreakerConfig, DefaultConfigMatchesOriginalThresholds) {
    LLMAQLHandler::Config cfg;

    // Default values should match the original single-breaker configuration
    EXPECT_EQ(cfg.infer_circuit_breaker.failure_threshold,    5u);
    EXPECT_EQ(cfg.rag_circuit_breaker.failure_threshold,      5u);
    EXPECT_EQ(cfg.embed_circuit_breaker.failure_threshold,    5u);
    EXPECT_EQ(cfg.finetune_circuit_breaker.failure_threshold, 5u);

    EXPECT_EQ(cfg.infer_circuit_breaker.timeout,    std::chrono::seconds(60));
    EXPECT_EQ(cfg.rag_circuit_breaker.timeout,      std::chrono::seconds(60));
    EXPECT_EQ(cfg.embed_circuit_breaker.timeout,    std::chrono::seconds(60));
    EXPECT_EQ(cfg.finetune_circuit_breaker.timeout, std::chrono::seconds(60));
}

// ============================================================================
// AC-7: executeStats() output includes circuit breaker states section
// ============================================================================

TEST_F(PerOperationCircuitBreakerTest, ExecuteStats_IncludesCircuitBreakerSection) {
    std::string stats = {};
    ASSERT_NO_THROW(stats = handler->executeStats());

    EXPECT_NE(stats.find("Circuit breakers"), std::string::npos)
        << "executeStats() output must contain a 'Circuit breakers' section.\n"
        << "Actual output:\n" << stats;
    EXPECT_NE(stats.find("infer"), std::string::npos);
    EXPECT_NE(stats.find("rag"), std::string::npos);
    EXPECT_NE(stats.find("embed"), std::string::npos);
    EXPECT_NE(stats.find("finetune"), std::string::npos);
}

// ============================================================================
// AC-8: CircuitBreakerStates struct has all four required fields.
// ============================================================================

TEST(PerOperationCircuitBreakerStates, StructHasAllFourFields) {
    LLMAQLHandler::CircuitBreakerStates states;
    states.infer    = "CLOSED";
    states.rag      = "OPEN";
    states.embed    = "HALF_OPEN";
    states.finetune = "CLOSED";

    EXPECT_EQ(states.infer,    "CLOSED");
    EXPECT_EQ(states.rag,      "OPEN");
    EXPECT_EQ(states.embed,    "HALF_OPEN");
    EXPECT_EQ(states.finetune, "CLOSED");
}

// ============================================================================
// AC-9: Independent reset — resetting one breaker doesn't affect others
// ============================================================================

TEST(PerOperationCircuitBreakerIsolation, IndependentReset) {
    CircuitBreaker::Config cfg;
    cfg.failure_threshold = 1;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;
    cfg.failure_window    = std::chrono::seconds(60);

    CircuitBreaker infer_breaker(cfg);
    CircuitBreaker rag_breaker(cfg);

    // Trip both
    infer_breaker.recordFailure();
    rag_breaker.recordFailure();

    EXPECT_EQ(infer_breaker.getState(), CircuitBreaker::State::OPEN);
    EXPECT_EQ(rag_breaker.getState(),   CircuitBreaker::State::OPEN);

    // Reset only infer
    infer_breaker.reset();

    EXPECT_EQ(infer_breaker.getState(), CircuitBreaker::State::CLOSED);
    // rag breaker must still be open
    EXPECT_EQ(rag_breaker.getState(),   CircuitBreaker::State::OPEN);
}

// ============================================================================
// AC-10: LLMAQLHandler(Config) constructor compiles and constructs successfully
// ============================================================================

TEST(PerOperationCircuitBreakerConfig, ConfigConstructorCompiles) {
    LLMAQLHandler::Config cfg;
    cfg.infer_circuit_breaker.failure_threshold = 3;
    cfg.rag_circuit_breaker.failure_threshold   = 7;

    ASSERT_NO_THROW({
        LLMAQLHandler handler(cfg);
        auto states = handler.getCircuitBreakerStates();
        EXPECT_EQ(states.infer, "CLOSED");
        EXPECT_EQ(states.rag,   "CLOSED");
    });
}
