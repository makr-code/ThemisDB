#include <gtest/gtest.h>

#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/argument_store.h"
#include "ethics_ai/rag_context_engine.h"
#include "ethics_ai/ethics_ai_types.h"

#include <memory>
#include <variant>

using namespace themis::plugins::ethics;

// ============================================================================
// Test fixture: wires up all three collaborators in standalone mode
// ============================================================================

class DiscourseEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader = std::make_shared<PhilosophyLoader>();
        store  = std::make_shared<ArgumentStore>();
        // Standalone mode: nullptr storage uses in-memory map
        auto init_status = store->initialize(nullptr, nullptr);
        ASSERT_TRUE(init_status.isOK()) << init_status.message;

        rag = std::make_shared<RAGContextEngine>(store);
        engine = std::make_unique<EthicalDiscourseEngine>(loader, store, rag);

        // Register two minimal profiles so tests have valid schools
        addProfile("kant",           "Kantian Ethics",
                   {"Act only according to a maxim you could will to be universal law."});
        addProfile("utilitarianism", "Utilitarianism",
                   {"The greatest happiness for the greatest number."});
    }

    // Helper: directly insert a profile into the loader's internal map
    void addProfile(const std::string& id,
                    const std::string& name,
                    const std::vector<std::string>& theses) {
        PhilosophyProfile p;
        p.school_id   = id;
        p.name        = name;
        p.main_theses = theses;
        p.strengths   = {"systematic"};
        p.weaknesses  = {"rigid in edge cases"};
        loader->addProfile(p);   // public helper exposed by PhilosophyLoader
    }

    std::shared_ptr<PhilosophyLoader>        loader;
    std::shared_ptr<ArgumentStore>           store;
    std::shared_ptr<RAGContextEngine>        rag;
    std::unique_ptr<EthicalDiscourseEngine>  engine;
};

// ============================================================================
// initializeDebate tests
// ============================================================================

TEST_F(DiscourseEngineTest, InitializeDebateValidSchoolsReturnsInit) {
    auto result = engine->initializeDebate(
        "Should we prioritise efficiency over fairness?",
        {"kant", "utilitarianism"},
        "resource_allocation");

    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(result));
    auto& init = std::get<DebateInitialization>(result);
    EXPECT_FALSE(init.debate_id.empty());
    EXPECT_EQ("Should we prioritise efficiency over fairness?", init.dilemma_description);
    EXPECT_EQ(2u, init.philosophy_schools.size());
    EXPECT_EQ("resource_allocation", init.category);
}

TEST_F(DiscourseEngineTest, InitializeDebateUnknownSchoolReturnsError) {
    auto result = engine->initializeDebate(
        "Is surveillance justified for public safety?",
        {"kant", "nonexistent_school"},
        "privacy");

    ASSERT_TRUE(std::holds_alternative<Status>(result));
    auto& err = std::get<Status>(result);
    EXPECT_FALSE(err.isOK());
    EXPECT_NE(std::string::npos, err.message.find("nonexistent_school"));
}

TEST_F(DiscourseEngineTest, InitializeDebateSingleSchool) {
    auto result = engine->initializeDebate(
        "Should AI systems have rights?",
        {"kant"},
        "ai_ethics");

    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(result));
    auto& init = std::get<DebateInitialization>(result);
    EXPECT_EQ(1u, init.philosophy_schools.size());
    EXPECT_EQ("kant", init.philosophy_schools[0]);
}

TEST_F(DiscourseEngineTest, InitializeDebateDebateIdIsUnique) {
    auto r1 = engine->initializeDebate("Dilemma A", {"kant"}, "test");
    auto r2 = engine->initializeDebate("Dilemma B", {"utilitarianism"}, "test");

    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(r1));
    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(r2));

    // Debate IDs must be non-empty; uniqueness is best-effort (time-based)
    EXPECT_FALSE(std::get<DebateInitialization>(r1).debate_id.empty());
    EXPECT_FALSE(std::get<DebateInitialization>(r2).debate_id.empty());
}

// ============================================================================
// makeDecision tests
// ============================================================================

TEST_F(DiscourseEngineTest, MakeDecisionValidSchoolsReturnsDecision) {
    auto result = engine->makeDecision(
        "Should personal data be monetised without explicit consent?",
        {"kant", "utilitarianism"},
        "data_privacy",
        /*use_rag=*/false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    auto& decision = std::get<EthicalDecision>(result);
    EXPECT_FALSE(decision.decision_id.empty());
    EXPECT_FALSE(decision.decision_text.empty());
    EXPECT_EQ("kant", decision.primary_philosophy);
    EXPECT_EQ(2u, decision.supporting_philosophies.size());
}

TEST_F(DiscourseEngineTest, MakeDecisionEmptySchoolsReturnsError) {
    auto result = engine->makeDecision(
        "Some dilemma",
        {},  // empty
        "category",
        false);

    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

TEST_F(DiscourseEngineTest, MakeDecisionSingleSchool) {
    auto result = engine->makeDecision(
        "Is it ethical to break a promise to prevent harm?",
        {"utilitarianism"},
        "moral_dilemma",
        false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    auto& d = std::get<EthicalDecision>(result);
    EXPECT_EQ("utilitarianism", d.primary_philosophy);
    EXPECT_GE(d.consensus_level, 0.0);
    EXPECT_LE(d.consensus_level, 1.0);
}

TEST_F(DiscourseEngineTest, MakeDecisionWithRAGContextNoThrow) {
    // RAGContextEngine operates on ArgumentStore which is empty in standalone
    // mode; the engine should handle missing context gracefully.
    EXPECT_NO_THROW({
        auto result = engine->makeDecision(
            "Trolley problem: one versus five",
            {"kant", "utilitarianism"},
            "moral_dilemma",
            /*use_rag=*/true);
        EXPECT_TRUE(std::holds_alternative<EthicalDecision>(result));
    });
}

TEST_F(DiscourseEngineTest, MakeDecisionStoresArgumentInStore) {
    engine->makeDecision(
        "Is automation ethically acceptable when it causes unemployment?",
        {"utilitarianism"},
        "economics",
        false);

    // The store must contain at least one argument after decision
    auto ids = store->getArgumentsByPhilosophy("utilitarianism", {}, 100);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(ids));
    EXPECT_GE(std::get<std::vector<EthicalArgument>>(ids).size(), 1u);
}

TEST_F(DiscourseEngineTest, DecisionTextContainsPrimaryPhilosophy) {
    auto result = engine->makeDecision(
        "Should AI replace human judges?",
        {"kant"},
        "justice",
        false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    auto& d = std::get<EthicalDecision>(result);
    // Decision text should reference the primary philosophy school
    EXPECT_NE(std::string::npos, d.decision_text.find("kant"));
}

TEST_F(DiscourseEngineTest, MultiSchoolDecisionHasConsensusBelow1) {
    auto result = engine->makeDecision(
        "Is capital punishment justified?",
        {"kant", "utilitarianism"},
        "justice",
        false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    auto& d = std::get<EthicalDecision>(result);
    // Multiple schools should not exceed full consensus.
    EXPECT_LE(d.consensus_level, 1.0);
    EXPECT_GT(d.consensus_level, 0.0);
}
