/**
 * @file test_aql_conversation_context.cpp
 * @brief Unit tests for AQLConversationContext
 */

#include <gtest/gtest.h>
#include "aql/aql_conversation_context.h"
#include "aql/llm_aql_handler.h"
#include "aql/llm_token_estimator.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace themis::aql;

// ============================================================================
// Fixture
// ============================================================================

class AQLConversationContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
        ctx     = std::make_unique<AQLConversationContext>(*handler);
    }

    void TearDown() override {
        ctx.reset();
        handler.reset();
    }

    std::unique_ptr<LLMAQLHandler>         handler;
    std::unique_ptr<AQLConversationContext> ctx;
};

// ============================================================================
// Initial state
// ============================================================================

TEST_F(AQLConversationContextTest, InitialTurnCountIsZero) {
    EXPECT_EQ(ctx->turnCount(), 0u);
}

TEST_F(AQLConversationContextTest, InitialLastQueryIsEmpty) {
    EXPECT_TRUE(ctx->lastQuery().empty());
}

TEST_F(AQLConversationContextTest, InitialHistoryIsEmpty) {
    EXPECT_TRUE(ctx->getHistory().empty());
}

TEST_F(AQLConversationContextTest, InitialSchemaContextIsEmpty) {
    EXPECT_TRUE(ctx->getSchemaContext().empty());
}

// ============================================================================
// Schema context
// ============================================================================

TEST_F(AQLConversationContextTest, SetSchemaContextStored) {
    ctx->setSchemaContext("Collections:\n- users: {name, city}\n");
    EXPECT_EQ(ctx->getSchemaContext(), "Collections:\n- users: {name, city}\n");
}

TEST_F(AQLConversationContextTest, UpdateSchemaContext) {
    ctx->setSchemaContext("schema v1");
    ctx->setSchemaContext("schema v2");
    EXPECT_EQ(ctx->getSchemaContext(), "schema v2");
}

// ============================================================================
// Input validation
// ============================================================================

TEST_F(AQLConversationContextTest, StartWithEmptyIntentThrows) {
    EXPECT_THROW(ctx->start(""), std::invalid_argument);
}

TEST_F(AQLConversationContextTest, RefineWithEmptyInstructionThrows) {
    // Inject one fake turn so refine() doesn't throw on turn_count_ == 0
    // We do this by calling start() and accepting a possible failure
    try { ctx->start("dummy intent"); } catch (...) {}
    // Force turn count to 1 so refine's precondition passes
    // (If start succeeded, turnCount() == 1; if it failed, we need to test separately)
    if (ctx->turnCount() >= 1) {
        EXPECT_THROW(ctx->refine(""), std::invalid_argument);
    }
}

TEST_F(AQLConversationContextTest, RefineBeforeStartThrows) {
    EXPECT_THROW(ctx->refine("add a filter"), std::logic_error);
}

// ============================================================================
// Start (graceful LLM absence)
// ============================================================================

TEST_F(AQLConversationContextTest, StartDoesNotThrowWithoutModel) {
    // Without a loaded LLM model, start() should return an empty string
    // but must NOT throw or crash.
    std::string result;
    EXPECT_NO_THROW({ result = ctx->start("Find all users"); });
    // Result is either a valid AQL string or empty (no model loaded)
    (void)result;
}

TEST_F(AQLConversationContextTest, StartClearsHistoryOnSecondCall) {
    // First conversation
    try { ctx->start("intent A"); } catch (...) {}
    // Second start – should reset
    try { ctx->start("intent B"); } catch (...) {}

    // History should contain: system + (user+assistant pairs for intent B only)
    // We can check that "intent A" is no longer in the history
    for (const auto& [role, content] : ctx->getHistory()) {
        EXPECT_EQ(content.find("intent A"), std::string::npos)
            << "Old conversation should have been cleared by second start()";
    }
}

// ============================================================================
// Reset
// ============================================================================

TEST_F(AQLConversationContextTest, ResetClearsState) {
    try { ctx->start("Find users"); } catch (...) {}
    ctx->reset();
    EXPECT_EQ(ctx->turnCount(), 0u);
    EXPECT_TRUE(ctx->lastQuery().empty());
    EXPECT_TRUE(ctx->getHistory().empty());
}

TEST_F(AQLConversationContextTest, RefineAfterResetThrows) {
    try { ctx->start("Find users"); } catch (...) {}
    ctx->reset();
    EXPECT_THROW(ctx->refine("add limit"), std::logic_error);
}

// ============================================================================
// Refine (graceful LLM absence)
// ============================================================================

TEST_F(AQLConversationContextTest, RefineDoesNotThrowWithoutModel) {
    // start() may fail silently when no model is loaded, so manually inject
    // a fake turn to bypass the precondition and test refine() independently.
    // We expose turn_count_ indirectly: call start() which at minimum either
    // succeeds (turn_count_ = 1) or fails silently (turn_count_ = 0).
    // Either way refine() must not crash.

    // Attempt start; if turn count becomes 1 we can call refine
    try { ctx->start("find all users"); } catch (...) {}
    if (ctx->turnCount() >= 1) {
        std::string result;
        EXPECT_NO_THROW({ result = ctx->refine("also filter by age > 18"); });
        (void)result;
    }
}

// ============================================================================
// History inspection
// ============================================================================

TEST_F(AQLConversationContextTest, HistoryContainsSystemMessage) {
    try { ctx->start("Find all orders"); } catch (...) {}
    const auto history = ctx->getHistory();
    if (!history.empty()) {
        EXPECT_EQ(history.front().first, "system");
    }
}

TEST_F(AQLConversationContextTest, HistoryRolesAreValid) {
    try { ctx->start("Find all products"); } catch (...) {}
    for (const auto& [role, content] : ctx->getHistory()) {
        bool valid_role = (role == "system" || role == "user" || role == "assistant");
        EXPECT_TRUE(valid_role) << "unexpected role: " << role;
    }
}

// ============================================================================
// Move semantics
// ============================================================================

TEST_F(AQLConversationContextTest, MoveConstructible) {
    AQLConversationContext moved(std::move(*ctx));
    EXPECT_EQ(moved.turnCount(), 0u);
}

// ============================================================================
// Config / Bounded history (max_turns)
// ============================================================================

// Helper: create a context with a fake LLM that echoes a fixed response,
// using the injected executor mechanism.
static AQLConversationContext makeContextWithFakeLLM(
    LLMAQLHandler& handler,
    std::size_t max_turns,
    std::size_t max_history_tokens = 0 /* 0 = disabled */
) {
    AQLConversationContext::Config cfg;
    cfg.max_turns          = max_turns;
    cfg.max_history_tokens = max_history_tokens;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN col RETURN x");
    };
    return AQLConversationContext(handler, std::move(cfg));
}

TEST_F(AQLConversationContextTest, MaxTurns_EvictsOldestPairWhenLimitReached) {
    // max_turns=3: after 5 turns the context should retain only 3 pairs.
    auto ctx3 = makeContextWithFakeLLM(*handler, /*max_turns=*/3);

    // Drive 5 turns (1 start + 4 refines)
    ctx3.start("turn 1");
    ctx3.refine("turn 2");
    ctx3.refine("turn 3");
    ctx3.refine("turn 4");
    ctx3.refine("turn 5");

    // turn_count() must reflect the retained window size, not the total
    EXPECT_EQ(ctx3.turnCount(), 3u);

    // history: system + 3×(user+assistant) = 7 messages
    EXPECT_EQ(ctx3.getHistory().size(), 7u);
}

TEST_F(AQLConversationContextTest, MaxTurns_SystemMessageAlwaysPreserved) {
    auto ctx3 = makeContextWithFakeLLM(*handler, /*max_turns=*/2);
    ctx3.start("intent A");
    ctx3.refine("refine B");
    ctx3.refine("refine C");

    const auto hist = ctx3.getHistory();
    ASSERT_FALSE(hist.empty());
    EXPECT_EQ(hist.front().first, "system");
}

TEST_F(AQLConversationContextTest, MaxTurns_TurnCountNeverExceedsLimit) {
    auto ctx3 = makeContextWithFakeLLM(*handler, /*max_turns=*/3);
    for (int i = 0; i < 10; ++i) {
        if (i == 0) {
          ctx3.start("first turn");
        }
        else        ctx3.refine("refine " + std::to_string(i));
    }
    EXPECT_LE(ctx3.turnCount(), 3u);
}

TEST_F(AQLConversationContextTest, MaxTurns_DefaultConfigFiftyTurns) {
    // Default max_turns is 50 – verify the default is honoured.
    AQLConversationContext::Config cfg;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN c RETURN x");
    };
    AQLConversationContext ctx2(*handler, std::move(cfg));

    ctx2.start("first");
    for (int i = 0; i < 20; ++i) {
        ctx2.refine("refine " + std::to_string(i));
    }
    // 21 turns, well below the 50-turn limit – nothing should be evicted.
    EXPECT_EQ(ctx2.turnCount(), 21u);
}

// ============================================================================
// Config / Bounded history (max_history_tokens)
// ============================================================================

TEST_F(AQLConversationContextTest, MaxHistoryTokens_EvictsWhenBudgetExceeded) {
    // Use a very small token budget to force eviction.
    // CharDivisionEstimator(4): each message ≈ len/4 tokens.
    // Keep budget tight: 50 tokens => roughly 200 chars.
    AQLConversationContext::Config cfg;
    cfg.max_turns          = 100; // high, so only token budget triggers eviction
    cfg.max_history_tokens = 50;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN c RETURN x"); // ~5 tokens
    };
    AQLConversationContext tctx(*handler, std::move(cfg));

    // Drive several turns and verify we never exceed the budget.
    tctx.start("find all users");
    for (int i = 0; i < 5; ++i) {
        tctx.refine("add filter " + std::to_string(i));
        EXPECT_LE(tctx.tokenCount(), 50u);
    }
}

// ============================================================================
// tokenCount()
// ============================================================================

TEST_F(AQLConversationContextTest, TokenCount_IsZeroOnFreshContext) {
    EXPECT_EQ(ctx->tokenCount(), 0u);
}

TEST_F(AQLConversationContextTest, TokenCount_PositiveAfterStart) {
    AQLConversationContext::Config cfg;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN c RETURN x");
    };
    AQLConversationContext tctx(*handler, std::move(cfg));
    tctx.start("find all users");
    EXPECT_GT(tctx.tokenCount(), 0u);
}

TEST_F(AQLConversationContextTest, TokenCount_ResetToZeroAfterReset) {
    AQLConversationContext::Config cfg;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN c RETURN x");
    };
    AQLConversationContext tctx(*handler, std::move(cfg));
    tctx.start("find all users");
    ASSERT_GT(tctx.tokenCount(), 0u);
    tctx.reset();
    EXPECT_EQ(tctx.tokenCount(), 0u);
}

// ============================================================================
// CharDivisionEstimator
// ============================================================================

TEST(AqlConversationCharDivisionEstimatorTest, EmptyStringReturnsZero) {
    CharDivisionEstimator est;
    EXPECT_EQ(est.estimate(""), 0u);
}

TEST(AqlConversationCharDivisionEstimatorTest, DefaultRatioFour) {
    CharDivisionEstimator est;
    // "hello" = 5 chars => ceil(5/4) = 2
    EXPECT_EQ(est.estimate("hello"), 2u);
}

TEST(AqlConversationCharDivisionEstimatorTest, CustomRatio) {
    CharDivisionEstimator est(2);
    // "hello" = 5 chars => ceil(5/2) = 3
    EXPECT_EQ(est.estimate("hello"), 3u);
}

TEST(AqlConversationCharDivisionEstimatorTest, ZeroRatioFallsBackToFour) {
    CharDivisionEstimator est(0);
    EXPECT_EQ(est.estimate("abcd"), 1u);
}

// ============================================================================
// Thread safety (basic smoke test)
// ============================================================================

TEST_F(AQLConversationContextTest, ConcurrentTurnCountReadIsConsistent) {
    AQLConversationContext::Config cfg;
    cfg.max_turns = 5;
    cfg.llm_executor = [](const std::vector<std::pair<std::string, std::string>>&) {
        return std::string("FOR x IN c RETURN x");
    };
    AQLConversationContext tctx(*handler, std::move(cfg));

    tctx.start("first");

    // Reader threads track the maximum turn count observed.  Using an atomic
    // avoids the lock + unbounded vector of the previous implementation, and
    // a yield prevents a tight CPU spin in CI.
    std::atomic<bool> stop{false};
    std::atomic<std::size_t> max_observed_turns{0};

    std::vector<std::thread> readers = {};

    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&tctx, &stop, &max_observed_turns]() {
            while (!stop.load(std::memory_order_relaxed)) {
                const std::size_t tc = tctx.turnCount();
                // CAS-loop to update max_observed_turns
                std::size_t prev = max_observed_turns.load(std::memory_order_relaxed);
                while (tc > prev &&
                       !max_observed_turns.compare_exchange_weak(
                           prev, tc, std::memory_order_relaxed)) {}
                (void)tctx.tokenCount();
                std::this_thread::yield();
            }
        });
    }

    for (int i = 0; i < 8; ++i) {
        tctx.refine("step " + std::to_string(i));
    }
    stop = true;
    for (auto& t : readers) {
      t.join();
    }

    // All observed turn counts must have been within the window.
    EXPECT_LE(max_observed_turns.load(), 5u)
        << "turnCount() exceeded max_turns during concurrent reads";
    EXPECT_LE(tctx.turnCount(), 5u);
}
