/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ethics_ai_benchmark.cpp                       ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:53:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     245                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 87778519a4  2026-04-12  feat(ethics_ai): remove stubs — computed scoring, YAML fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/discourse_engine.h"
#include "ethics_ai/ethics_evaluator.h"
#include "ethics_ai/philosophy_loader.h"
#include "ethics_ai/rag_context_engine.h"
#include "plugins/ethics_ai/ethics_ai_types.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace themis::plugins::ethics;
using Clock = std::chrono::steady_clock;

namespace {

double ms(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

static PhilosophyProfile makeProfile(const std::string& id,
                                     const std::string& name) {
    PhilosophyProfile p;
    p.school_id = id;
    p.name      = name;
    p.main_theses    = {"Maximise overall well-being",
                        "Act for the greatest good for the greatest number"};
    p.secondary_theses = {"Consider long-term consequences",
                          "Utility is the ultimate criterion"};
    p.decision_framework["primary"] = "Apply " + name + " utility calculus";
    return p;
}

} // anonymous namespace

// =============================================================================
// Fixture
// =============================================================================

class EthicsAIBenchmarkTests : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_shared<PhilosophyLoader>();
        store_  = std::make_shared<ArgumentStore>();
        rag_    = std::make_shared<RAGContextEngine>(store_);
        engine_ = std::make_shared<EthicalDiscourseEngine>(loader_, store_, rag_);

        Status s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK()) << s.message;

        loader_->addProfile(makeProfile("utilitarianism", "Utilitarianism"));
        loader_->addProfile(makeProfile("kantian",        "Kantian Ethics"));
    }

    std::shared_ptr<PhilosophyLoader>       loader_;
    std::shared_ptr<ArgumentStore>          store_;
    std::shared_ptr<RAGContextEngine>       rag_;
    std::shared_ptr<EthicalDiscourseEngine> engine_;
};

// =============================================================================
// PB-01  makeDecision() single-school latency
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB01_MakeDecisionSingleSchoolUnder500ms) {
    auto t0 = Clock::now();
    auto result = engine_->makeDecision(
        "Should patient data be shared without consent for medical research?",
        {"utilitarianism"},
        "medical-ethics",
        /*use_rag=*/false);
    auto t1 = Clock::now();

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result))
        << "makeDecision returned Status: "
        << (std::holds_alternative<Status>(result)
                ? std::get<Status>(result).message : "");

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 500.0)
        << "makeDecision (single school) took " << elapsed << " ms, expected < 500 ms";
}

// =============================================================================
// PB-02  makeDecision() two-school latency
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB02_MakeDecisionTwoSchoolsUnder500ms) {
    auto t0 = Clock::now();
    auto result = engine_->makeDecision(
        "Is it ethical to use surveillance for public safety?",
        {"utilitarianism", "kantian"},
        "surveillance-ethics",
        /*use_rag=*/false);
    auto t1 = Clock::now();

    ASSERT_TRUE(std::holds_alternative<EthicalDecision>(result));

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 500.0)
        << "makeDecision (two schools) took " << elapsed << " ms, expected < 500 ms";
}

// =============================================================================
// PB-03  computeConfidence() O(n) latency for 100 arguments
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB03_ComputeConfidence100ArgsUnder1ms) {
    std::vector<EthicalArgument> args;
    args.reserve(100);
    static const ArgumentStrength strengths[] = {
        ArgumentStrength::WEAK, ArgumentStrength::MODERATE,
        ArgumentStrength::STRONG, ArgumentStrength::DECISIVE
    };
    for (int i = 0; i < 100; ++i) {
        EthicalArgument a;
        a.id               = "a" + std::to_string(i);
        a.philosophy_school = "utilitarianism";
        a.argument_type    = ArgumentType::PRO;
        a.strength         = strengths[i % 4];
        args.push_back(a);
    }

    auto t0 = Clock::now();
    double conf = EthicsEvaluator::computeConfidence(args);
    auto t1 = Clock::now();

    EXPECT_GE(conf, 0.0);
    EXPECT_LE(conf, 1.0);

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 1.0)
        << "computeConfidence(100 args) took " << elapsed << " ms, expected < 1 ms";
}

// =============================================================================
// PB-04  computeConsensus() O(n) latency for 100 arguments
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB04_ComputeConsensus100ArgsUnder1ms) {
    std::vector<EthicalArgument> args;
    args.reserve(100);
    static const std::string schools[] = {
        "utilitarianism", "kantian", "virtue_ethics", "rawls"
    };
    for (int i = 0; i < 100; ++i) {
        EthicalArgument a;
        a.id                = "a" + std::to_string(i);
        a.philosophy_school = schools[i % 4];
        a.argument_type     = (i % 3 == 0) ? ArgumentType::CONTRA : ArgumentType::PRO;
        a.strength          = ArgumentStrength::MODERATE;
        args.push_back(a);
    }

    auto t0 = Clock::now();
    double cons = EthicsEvaluator::computeConsensus(args);
    auto t1 = Clock::now();

    EXPECT_GE(cons, 0.0);
    EXPECT_LE(cons, 1.0);

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 1.0)
        << "computeConsensus(100 args) took " << elapsed << " ms, expected < 1 ms";
}

// =============================================================================
// PB-05  vectorSemanticSearch() (uses generateEmbedding() internally) < 5 ms
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB05_VectorSemanticSearchUnder5ms) {
    // vectorSemanticSearch() calls generateEmbedding() on every stored argument
    // as part of cosine similarity ranking.  With an empty store the timing
    // reflects the overhead of a single query-embedding call plus the search
    // dispatch path.  A real embedding benchmark requires injecting a populated
    // store; this threshold covers the query-path overhead only.
    std::vector<float> query(768, 0.0f);
    query[0] = 1.0f;
    auto result = rag_->vectorSemanticSearch(query, "utilitarianism", 1);
    auto t1 = Clock::now();

    // Result is valid (either empty results or Status — both are fine in standalone)
    (void)result;

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 5.0)
        << "vectorSemanticSearch (empty store) took " << elapsed
        << " ms, expected < 5 ms";
}

// =============================================================================
// PB-06  buildContext() standalone mode < 1 s
// =============================================================================

TEST_F(EthicsAIBenchmarkTests, PB06_BuildContextStandaloneUnder1s) {
    // Seed the store with some arguments so the code paths are exercised.
    for (int i = 0; i < 10; ++i) {
        EthicalArgument a;
        a.id                = "seed_" + std::to_string(i);
        a.philosophy_school = (i % 2 == 0) ? "utilitarianism" : "kantian";
        a.argument_type     = ArgumentType::PRO;
        a.strength          = ArgumentStrength::STRONG;
        a.content           = "Ethics seed argument " + std::to_string(i) +
                              " covering privacy, autonomy and dignity.";
        store_->storeArgument(a, false);
    }

    auto t0 = Clock::now();
    auto result = rag_->buildContext(
        "Should AI systems make life-or-death decisions autonomously?",
        {"utilitarianism", "kantian"},
        "ai-autonomy");
    auto t1 = Clock::now();

    (void)result; // may succeed or return Status in standalone mode

    double elapsed = ms(t0, t1);
    EXPECT_LT(elapsed, 1000.0)
        << "buildContext (standalone) took " << elapsed << " ms, expected < 1000 ms";
}
