/**
 * @file test_aql_conversation_context.cpp
 * @brief Unit tests for multi-turn AQL conversation context and iterative
 *        query refinement (AQLConversationSession + translateNLToAQLIterative).
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"

using namespace themis::aql;

// ============================================================================
// AQLConversationSession Tests
// ============================================================================

TEST(AQLConversationSessionTest, InitiallyEmpty) {
    AQLConversationSession session;
    EXPECT_TRUE(session.empty());
    EXPECT_EQ(session.size(), 0u);
    EXPECT_TRUE(session.getHistory().empty());
}

TEST(AQLConversationSessionTest, AddTurnIncreasesSize) {
    AQLConversationSession session;
    session.addTurn("Find all users", "FOR u IN users RETURN u");
    EXPECT_FALSE(session.empty());
    EXPECT_EQ(session.size(), 1u);
}

TEST(AQLConversationSessionTest, AddMultipleTurns) {
    AQLConversationSession session;
    session.addTurn("Find all users", "FOR u IN users RETURN u");
    session.addTurn("Filter to Seattle", "FOR u IN users FILTER u.city == 'Seattle' RETURN u");

    EXPECT_EQ(session.size(), 2u);

    const auto& history = session.getHistory();
    ASSERT_EQ(history.size(), 2u);

    EXPECT_EQ(history[0].nl_query,   "Find all users");
    EXPECT_EQ(history[0].aql_result, "FOR u IN users RETURN u");

    EXPECT_EQ(history[1].nl_query,   "Filter to Seattle");
    EXPECT_EQ(history[1].aql_result,
              "FOR u IN users FILTER u.city == 'Seattle' RETURN u");
}

TEST(AQLConversationSessionTest, ClearResetsSession) {
    AQLConversationSession session;
    session.addTurn("Find all users", "FOR u IN users RETURN u");
    ASSERT_EQ(session.size(), 1u);

    session.clear();
    EXPECT_TRUE(session.empty());
    EXPECT_EQ(session.size(), 0u);
    EXPECT_TRUE(session.getHistory().empty());
}

TEST(AQLConversationSessionTest, PreservesInsertionOrder) {
    AQLConversationSession session;
    for (int i = 0; i < 5; ++i) {
        session.addTurn("query " + std::to_string(i), "aql " + std::to_string(i));
    }

    ASSERT_EQ(session.size(), 5u);
    for (std::size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(session.getHistory()[i].nl_query,
                  "query " + std::to_string(i));
        EXPECT_EQ(session.getHistory()[i].aql_result,
                  "aql " + std::to_string(i));
    }
}

TEST(AQLConversationSessionTest, ClearThenAddWorks) {
    AQLConversationSession session;
    session.addTurn("first", "aql_first");
    session.clear();
    session.addTurn("second", "aql_second");

    ASSERT_EQ(session.size(), 1u);
    EXPECT_EQ(session.getHistory()[0].nl_query, "second");
}

// ============================================================================
// LLMAQLHandler – translateNLToAQLIterative Tests
// ============================================================================

class AQLIterativeTranslationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }

    void TearDown() override {
        handler.reset();
    }

    std::unique_ptr<LLMAQLHandler> handler;
};

TEST_F(AQLIterativeTranslationTest, FirstTurnPopulatesSession) {
    AQLConversationSession session;
    ASSERT_TRUE(session.empty());

    try {
        auto aql = handler->translateNLToAQLIterative("Find all users", session);

        // Session must have exactly one turn after the call
        ASSERT_EQ(session.size(), 1u);
        EXPECT_EQ(session.getHistory()[0].nl_query, "Find all users");
        // The stored AQL must match what was returned
        EXPECT_EQ(session.getHistory()[0].aql_result, aql);
        EXPECT_FALSE(aql.empty());

    } catch (const std::exception& e) {
        // Expected when no LLM model is loaded
        EXPECT_TRUE(std::string(e.what()).find("translation failed") != std::string::npos ||
                    std::string(e.what()).find("CHAT failed") != std::string::npos);
        // Session must not have been modified on failure
        EXPECT_TRUE(session.empty());
    }
}

TEST_F(AQLIterativeTranslationTest, SubsequentTurnAccumulatesHistory) {
    AQLConversationSession session;

    // Simulate a pre-existing first turn
    session.addTurn("Find all users",
                    "FOR u IN users RETURN u");

    try {
        auto aql = handler->translateNLToAQLIterative(
            "Now filter to only users in Seattle", session);

        // Session should now hold two turns
        ASSERT_EQ(session.size(), 2u);
        EXPECT_EQ(session.getHistory()[1].nl_query,
                  "Now filter to only users in Seattle");
        EXPECT_EQ(session.getHistory()[1].aql_result, aql);

    } catch (const std::exception& e) {
        // Only the original turn must remain when the call fails
        EXPECT_EQ(session.size(), 1u);
    }
}

TEST_F(AQLIterativeTranslationTest, SchemaContextIsAccepted) {
    AQLConversationSession session;
    const std::string schema = "Collections:\n- users: {name, city, age}\n";

    try {
        auto aql = handler->translateNLToAQLIterative(
            "Find users older than 30", session, schema);

        ASSERT_EQ(session.size(), 1u);
        EXPECT_FALSE(aql.empty());

    } catch (const std::exception& e) {
        EXPECT_TRUE(session.empty());
    }
}

TEST_F(AQLIterativeTranslationTest, NoMarkdownInResult) {
    AQLConversationSession session;

    try {
        auto aql = handler->translateNLToAQLIterative("List all documents", session);

        // Markdown fences must be stripped
        EXPECT_EQ(aql.find("```"), std::string::npos);

        if (!aql.empty()) {
            // Result must be trimmed
            EXPECT_FALSE(std::isspace(static_cast<unsigned char>(aql.front())));
            EXPECT_FALSE(std::isspace(static_cast<unsigned char>(aql.back())));
        }
    } catch (const std::exception& e) {
        // Expected without model; just ensure session is clean
        EXPECT_TRUE(session.empty());
    }
}

TEST_F(AQLIterativeTranslationTest, IndependentSessionsDoNotInterfere) {
    AQLConversationSession sessionA;
    AQLConversationSession sessionB;

    sessionA.addTurn("Find users", "FOR u IN users RETURN u");

    // sessionB must not be affected by sessionA
    EXPECT_TRUE(sessionB.empty());

    try {
        handler->translateNLToAQLIterative("Find products", sessionB);
        EXPECT_EQ(sessionA.size(), 1u);
        EXPECT_EQ(sessionB.size(), 1u);
    } catch (const std::exception&) {
        EXPECT_EQ(sessionA.size(), 1u);
        EXPECT_TRUE(sessionB.empty());
    }
}
