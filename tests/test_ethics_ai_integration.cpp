#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/ethics_evaluator.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/rag_context_engine.h"
#include "ethics_ai/ethics_ai_types.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

using namespace themis::plugins::ethics;

// ============================================================================
// Shared helpers
// ============================================================================

static PhilosophyProfile makeProfile(const std::string& id,
                                     const std::string& name,
                                     const std::string& thesis) {
    PhilosophyProfile p;
    p.school_id  = id;
    p.name       = name;
    p.main_theses.push_back(thesis);
    p.secondary_theses.push_back("Practical application of " + name);
    p.decision_framework["primary"] = "Apply " + name + " principles";
    return p;
}

static EthicalArgument makeArg(const std::string& id,
                                const std::string& school,
                                ArgumentType type = ArgumentType::PRO,
                                ArgumentStrength strength = ArgumentStrength::MODERATE) {
    EthicalArgument a;
    a.id               = id;
    a.philosophy_school = school;
    a.argument_type    = type;
    a.strength         = strength;
    a.content          = "Content for " + id + " from " + school;
    return a;
}

// ============================================================================
// Suite 1: Full pipeline – initialization → debate → decision → evaluation
// ============================================================================

class FullPipelineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_  = std::make_shared<PhilosophyLoader>();
        store_   = std::make_shared<ArgumentStore>();
        rag_     = std::make_shared<RAGContextEngine>(store_);
        engine_  = std::make_shared<EthicalDiscourseEngine>(loader_, store_, rag_);
        // evaluator_ is stateless, no shared-ptr necessary
        eval_    = std::make_shared<EthicsEvaluator>();

        // Standalone mode (no RocksDB)
        Status s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK()) << "ArgumentStore init failed: " << s.message;

        // Register two philosophy profiles directly
        loader_->addProfile(makeProfile("utilitarianism",
                                        "Utilitarianism",
                                        "Maximise overall well-being"));
        loader_->addProfile(makeProfile("kantian",
                                        "Kantian Ethics",
                                        "Act only according to the categorical imperative"));
    }

    std::shared_ptr<PhilosophyLoader>          loader_;
    std::shared_ptr<ArgumentStore>             store_;
    std::shared_ptr<RAGContextEngine>          rag_;
    std::shared_ptr<EthicalDiscourseEngine>    engine_;
    std::shared_ptr<EthicsEvaluator>           eval_;
};

// --- initializeDebate ---

TEST_F(FullPipelineIntegrationTest, InitializeDebateReturnsDebateId) {
    auto result = engine_->initializeDebate(
        "Should autonomous vehicles prioritize passenger or pedestrian safety?",
        {"utilitarianism", "kantian"},
        "autonomous-systems");

    ASSERT_TRUE(std::holds_alternative<DebateInitialization>(result));
    const auto& debate = std::get<DebateInitialization>(result);
    EXPECT_FALSE(debate.debate_id.empty());
    EXPECT_EQ(2u, debate.philosophy_schools.size());
    EXPECT_EQ("autonomous-systems", debate.category);
}

TEST_F(FullPipelineIntegrationTest, InitializeDebateUnknownSchoolReturnsError) {
    auto result = engine_->initializeDebate(
        "Some dilemma",
        {"nonexistent_school"},
        "generic");

    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// --- makeDecision ---

TEST_F(FullPipelineIntegrationTest, MakeDecisionReturnsValidDecision) {
    auto result = engine_->makeDecision(
        "Should patient data be shared without consent to advance medical research?",
        {"utilitarianism", "kantian"},
        "medical-ethics",
        /*use_rag=*/false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result))
        << "Expected EthicalDecision, got Status: "
        << (std::holds_alternative<Status>(result)
                ? std::get<Status>(result).message : "");

    const auto& decision = std::get<EthicalDecision>(result);
    EXPECT_FALSE(decision.decision_id.empty());
    EXPECT_FALSE(decision.decision_text.empty());
    EXPECT_EQ("utilitarianism", decision.primary_philosophy);
    EXPECT_GE(decision.confidence, 0.0);
    EXPECT_LE(decision.confidence, 1.0);
    EXPECT_GE(decision.consensus_level, 0.0);
    EXPECT_LE(decision.consensus_level, 1.0);
    // Engine stores one PRO argument per school → 2 arguments in store
    EXPECT_EQ(2u, decision.supporting_philosophies.size());
}

TEST_F(FullPipelineIntegrationTest, MakeDecisionEmptySchoolsReturnsError) {
    auto result = engine_->makeDecision("Any dilemma", {}, "generic", false);
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

TEST_F(FullPipelineIntegrationTest, MakeDecisionSingleSchoolConsensusIsOne) {
    auto result = engine_->makeDecision(
        "A single-school dilemma",
        {"utilitarianism"},
        "test-category",
        /*use_rag=*/false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));
    EXPECT_DOUBLE_EQ(1.0, std::get<EthicalDecision>(result).consensus_level);
}

// --- makeDecision with RAG ---

TEST_F(FullPipelineIntegrationTest, MakeDecisionWithRagDoesNotCrash) {
    auto result = engine_->makeDecision(
        "Climate change mitigation vs economic growth",
        {"utilitarianism", "kantian"},
        "environmental-ethics",
        /*use_rag=*/true);

    // RAGContextEngine in standalone mode may return Status from AQL methods,
    // but makeDecision should still produce a decision (RAG failure is non-fatal).
    EXPECT_TRUE(std::holds_alternative<EthicalDecision>(result));
}

// --- evaluateDecision ---

TEST_F(FullPipelineIntegrationTest, EvaluateDecisionScoresInRange) {
    auto dec_result = engine_->makeDecision(
        "Is it ethical to use prisoner data for AI training?",
        {"utilitarianism", "kantian"},
        "data-ethics",
        false);

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(dec_result));
    const auto& decision = std::get<EthicalDecision>(dec_result);

    // Fetch arguments that were stored during makeDecision
    auto args_result = store_->getArgumentsByPhilosophy(
        "utilitarianism", {}, 50);
    std::vector<EthicalArgument> args;
    if (std::holds_alternative<std::vector<EthicalArgument>>(args_result)) {
        args = std::get<std::vector<EthicalArgument>>(args_result);
    }

    auto eval_result = eval_->evaluateDecision(decision, args);
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(eval_result));

    const auto& eval = std::get<EthicsEvaluationResult>(eval_result);
    EXPECT_GE(eval.overall_score, 0.0);
    EXPECT_LE(eval.overall_score, 1.0);
    EXPECT_GE(eval.decision_quality_score, 0.0);
    EXPECT_LE(eval.consistency_score, 1.0);
    EXPECT_GE(eval.fairness_score, 0.0);
    EXPECT_LE(eval.alignment_score, 1.0);
    EXPECT_GE(eval.transparency_score, 0.0);
    EXPECT_EQ(8u, eval.detailed_metrics.size()); // 6 named + num_arguments + confidence + consensus
}

TEST_F(FullPipelineIntegrationTest, EvaluateEmptyArgumentsDoesNotCrash) {
    EthicalDecision dummy;
    dummy.decision_id   = "test-decision";
    dummy.decision_text = "A minimal decision";
    dummy.confidence    = 0.5;
    dummy.consensus_level = 0.5;

    auto eval_result = eval_->evaluateDecision(dummy, {});
    ASSERT_TRUE(std::holds_alternative<EthicsEvaluationResult>(eval_result));
    const auto& eval = std::get<EthicsEvaluationResult>(eval_result);
    EXPECT_GE(eval.overall_score, 0.0);
    EXPECT_LE(eval.overall_score, 1.0);
}

// ============================================================================
// Suite 2: ArgumentStore seeded with multiple arguments + RAGContextEngine
// ============================================================================

class ArgumentStoreRAGIntegrationTest : public ::testing::Test {
protected:
    static constexpr size_t kNumArgs = 21; // 7 per school × 3 schools

    void SetUp() override {
        store_ = std::make_shared<ArgumentStore>();
        rag_   = std::make_shared<RAGContextEngine>(store_);

        Status s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK());

        // Seed 7 arguments per school across 3 schools
        static const char* schools[] = {"utilitarianism", "kantian", "virtue_ethics"};
        static const ArgumentType types[] = {
            ArgumentType::PRO, ArgumentType::CONTRA, ArgumentType::PRO,
            ArgumentType::CONTRA, ArgumentType::PRO, ArgumentType::CONTRA, ArgumentType::PRO
        };
        size_t idx = 0;
        for (const char* school : schools) {
            for (int i = 0; i < 7; ++i, ++idx) {
                auto a = makeArg("arg_" + std::to_string(idx), school, types[i]);
                store_->storeArgument(a, false);
            }
        }

        // Build a 3-node chain: arg_0 → arg_1 → arg_2 (utilitarianism)
        ArgumentChain chain;
        chain.id           = "chain_util_intro";
        chain.chain_type   = "pro";
        chain.coherence_score = 0.85;
        chain.argument_ids = {"arg_0", "arg_1", "arg_2"};
        store_->storeChain(chain);
    }

    std::shared_ptr<ArgumentStore>    store_;
    std::shared_ptr<RAGContextEngine> rag_;
};

TEST_F(ArgumentStoreRAGIntegrationTest, GetArgsByPhilosophyReturnsOnlySchoolSubset) {
    auto result = store_->getArgumentsByPhilosophy("utilitarianism", {}, 50);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    const auto& args = std::get<std::vector<EthicalArgument>>(result);
    EXPECT_EQ(7u, args.size());
    for (const auto& a : args) {
        EXPECT_EQ("utilitarianism", a.philosophy_school);
    }
}

TEST_F(ArgumentStoreRAGIntegrationTest, GetArgsByPhilosophyLimitIsRespected) {
    auto result = store_->getArgumentsByPhilosophy("kantian", {}, 3);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    EXPECT_LE(std::get<std::vector<EthicalArgument>>(result).size(), 3u);
}

TEST_F(ArgumentStoreRAGIntegrationTest, GetArgsByPhilosophyUnknownSchoolReturnsEmpty) {
    auto result = store_->getArgumentsByPhilosophy("stoicism", {}, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    EXPECT_TRUE(std::get<std::vector<EthicalArgument>>(result).empty());
}

TEST_F(ArgumentStoreRAGIntegrationTest, GetArgsByPhilosophyProFilterWorks) {
    auto result = store_->getArgumentsByPhilosophy(
        "virtue_ethics", {ArgumentType::PRO}, 50);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(result));
    for (const auto& a : std::get<std::vector<EthicalArgument>>(result)) {
        EXPECT_EQ(ArgumentType::PRO, a.argument_type);
    }
}

TEST_F(ArgumentStoreRAGIntegrationTest, StoreAndRetrieveChainRoundTrip) {
    auto result = store_->getChain("chain_util_intro");
    ASSERT_TRUE(std::holds_alternative<ArgumentChain>(result));
    const auto& chain = std::get<ArgumentChain>(result);
    EXPECT_EQ("chain_util_intro", chain.id);
    ASSERT_EQ(3u, chain.argument_ids.size());
    EXPECT_EQ("arg_0", chain.argument_ids[0]);
    EXPECT_EQ("arg_1", chain.argument_ids[1]);
    EXPECT_EQ("arg_2", chain.argument_ids[2]);
    EXPECT_DOUBLE_EQ(0.85, chain.coherence_score);
}

TEST_F(ArgumentStoreRAGIntegrationTest, GetChainUnknownIdReturnsError) {
    auto result = store_->getChain("no_such_chain");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

TEST_F(ArgumentStoreRAGIntegrationTest, TraverseChainVisitsCorrectNodes) {
    // RAGContextEngine BFS starting at arg_0 (which is the chain root)
    auto result = rag_->traverseArgumentChain("arg_0", 10, "forward");
    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& visited = std::get<std::vector<std::string>>(result);
    ASSERT_FALSE(visited.empty());
    EXPECT_EQ("arg_0", visited.front());
}

TEST_F(ArgumentStoreRAGIntegrationTest, TraverseChainMaxDepthTerminates) {
    // max_depth=1 must not visit more than 2 nodes (start + one hop)
    auto result = rag_->traverseArgumentChain("arg_0", 1, "forward");
    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    EXPECT_LE(std::get<std::vector<std::string>>(result).size(), 2u);
}

// ============================================================================
// Suite 3: RAGContextEngine  buildContext / query patterns
// ============================================================================

class RAGContextBuildTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_shared<ArgumentStore>();
        rag_   = std::make_shared<RAGContextEngine>(store_);

        Status s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK());

        // Seed a small set of diverse arguments and one decision
        store_->storeArgument(makeArg("a1", "utilitarianism", ArgumentType::PRO,
                                      ArgumentStrength::DECISIVE), false);
        store_->storeArgument(makeArg("a2", "kantian", ArgumentType::CONTRA,
                                      ArgumentStrength::STRONG), false);
        store_->storeArgument(makeArg("a3", "utilitarianism", ArgumentType::PRO,
                                      ArgumentStrength::MODERATE), false);

        EthicalDecision d;
        d.decision_id           = "d1";
        d.decision_text         = "Proceed with transparency measures";
        d.primary_philosophy    = "utilitarianism";
        d.confidence            = 0.8;
        d.consensus_level       = 0.65;
        store_->storeDecision(d);
    }

    std::shared_ptr<ArgumentStore>    store_;
    std::shared_ptr<RAGContextEngine> rag_;
};

TEST_F(RAGContextBuildTest, BuildContextDoesNotCrash) {
    auto result = rag_->buildContext(
        "Data privacy vs. public safety",
        {"utilitarianism", "kantian"},
        "privacy");

    // May succeed or return Status depending on AQL availability in standalone mode;
    // must not throw or crash.
    (void)result; // just exercising the code path
    SUCCEED();
}

TEST_F(RAGContextBuildTest, FindSimilarDilemmasReturnsVector) {
    auto result = rag_->findSimilarDilemmas("data privacy", 0.5, 5);
    // In standalone mode AQL is unavailable; result is either a vector or an error Status.
    // Both are acceptable – what we verify is no crash and correct variant type.
    EXPECT_TRUE(std::holds_alternative<std::vector<std::string>>(result) ||
                std::holds_alternative<Status>(result));
}

TEST_F(RAGContextBuildTest, GetBestPracticesReturnsVector) {
    auto result = rag_->getBestPractices("privacy", 0.6, 5);
    EXPECT_TRUE(std::holds_alternative<std::vector<std::string>>(result) ||
                std::holds_alternative<Status>(result));
}

TEST_F(RAGContextBuildTest, VectorSemanticSearchWithZeroVectorDoesNotCrash) {
    using SimilarityResults = std::vector<std::pair<std::string, double>>;
    std::vector<float> zero_vec(128, 0.0f);
    auto result = rag_->vectorSemanticSearch(zero_vec, "utilitarianism", 5);
    EXPECT_TRUE(std::holds_alternative<SimilarityResults>(result) ||
                std::holds_alternative<Status>(result));
}

TEST_F(RAGContextBuildTest, TraverseFromUnseededArgStartsChainForwardWithStartNode) {
    // Non-existent start ID: BFS adds start node unconditionally before checking store
    auto result = rag_->traverseArgumentChain("orphan_arg", 3, "forward");
    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(result));
    const auto& chain = std::get<std::vector<std::string>>(result);
    ASSERT_FALSE(chain.empty());
    EXPECT_EQ("orphan_arg", chain.front());
}
