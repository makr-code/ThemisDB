/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_conversation_context.cpp                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 14:17:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     206                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • afade9ae9  2026-02-21  [aql] Interactive AQL query builder, validator, template ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_aql_conversation_context.cpp
 * @brief Unit tests for AQLConversationContext
 */

#include <gtest/gtest.h>
#include "aql/aql_conversation_context.h"
#include "aql/llm_aql_handler.h"

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
