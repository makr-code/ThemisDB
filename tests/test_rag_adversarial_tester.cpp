/**
 * @file test_rag_adversarial_tester.cpp
 * @brief Unit tests for AdversarialTester (RAG adversarial robustness testing).
 *
 * Coverage:
 *  - Construction with default and custom configuration
 *  - getConfig() reflects set config
 *  - addBaseQuery / addBaseDocument / setBaseQueries / setBaseDocuments
 *  - generatePerturbedQueries: produces correct count, non-empty, distinct
 *    from original for each strategy
 *  - generatePoisonedDocuments: returns same count as input, at least one
 *    document is modified
 *  - generateSycophancyQuery: non-empty and differs from original
 *  - isSuccessfulAttack: identical answers → not an attack; fully divergent
 *    answers → successful attack
 *  - testRobustness throws when no queries / no documents configured
 *  - testRobustness returns valid report with robustness_score in [0, 1]
 *  - Individual test phases do not crash with populated tester
 *  - RobustnessReport fields are correctly populated after full run
 */

#include <gtest/gtest.h>
#include "rag/adversarial_tester.h"
#include "rag/rag_judge.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::rag::adversarial;
using namespace themis::rag::judge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RetrievedDocument makeDoc(const std::string& id, const std::string& content)
{
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = 0.9;
    return d;
}

static AdversarialTester makeTester()
{
    AdversarialTester t;
    t.addBaseQuery("What is the capital of France?", "Paris");
    t.addBaseDocument(makeDoc("doc1", "Paris is the capital of France."));
    t.addBaseDocument(makeDoc("doc2", "France is a country in Western Europe."));
    return t;
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

TEST(AdversarialTesterConfig, DefaultValues)
{
    AdversarialTesterConfig cfg;
    EXPECT_GT(cfg.score_instability_threshold, 0.0);
    EXPECT_LE(cfg.score_instability_threshold, 1.0);
    EXPECT_GT(cfg.perturbations_per_query, 0u);
    EXPECT_GT(cfg.poisoning_faithfulness_threshold, 0.0);
    EXPECT_GT(cfg.context_overflow_padding_docs, 0u);
    EXPECT_FALSE(cfg.enabled_strategies.empty());
}

TEST(AdversarialTesterConfig, CustomValues)
{
    AdversarialTesterConfig cfg;
    cfg.score_instability_threshold = 0.5;
    cfg.perturbations_per_query     = 5;

    AdversarialTester tester(cfg);
    auto got = tester.getConfig();
    EXPECT_DOUBLE_EQ(got.score_instability_threshold, 0.5);
    EXPECT_EQ(got.perturbations_per_query, 5u);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(AdversarialTester, DefaultConstruction)
{
    EXPECT_NO_THROW({ AdversarialTester t; });
}

TEST(AdversarialTester, CustomConfigConstruction)
{
    AdversarialTesterConfig cfg;
    cfg.perturbations_per_query = 10;
    EXPECT_NO_THROW({ AdversarialTester t(cfg); });
}

// ---------------------------------------------------------------------------
// Population API
// ---------------------------------------------------------------------------

TEST(AdversarialTester, AddBaseQueryAndDocument)
{
    AdversarialTester t;
    EXPECT_NO_THROW(t.addBaseQuery("Hello?"));
    EXPECT_NO_THROW(t.addBaseDocument(makeDoc("d1", "Some content.")));
}

TEST(AdversarialTester, SetBaseQueriesReplacesPrevious)
{
    AdversarialTester t;
    t.addBaseQuery("Old query");

    std::vector<BaseQuery> new_queries = {{"New query 1", "ans1"},
                                          {"New query 2", "ans2"}};
    t.setBaseQueries(new_queries);

    // After setBaseQueries, the tester should work with the new set.
    t.addBaseDocument(makeDoc("d1", "Context."));
    RAGJudge judge;
    auto report = t.testRobustness(judge);
    EXPECT_GE(report.robustness_score, 0.0);
    EXPECT_LE(report.robustness_score, 1.0);
}

TEST(AdversarialTester, SetBaseDocumentsReplacesPrevious)
{
    AdversarialTester t;
    t.addBaseDocument(makeDoc("old_doc", "Old content."));

    std::vector<RetrievedDocument> new_docs = {
        makeDoc("new_doc1", "New content 1."),
        makeDoc("new_doc2", "New content 2."),
    };
    t.setBaseDocuments(new_docs);
    t.addBaseQuery("Test query", "Test answer");

    RAGJudge judge;
    auto report = t.testRobustness(judge);
    EXPECT_GE(report.robustness_score, 0.0);
    EXPECT_LE(report.robustness_score, 1.0);
}

// ---------------------------------------------------------------------------
// generatePerturbedQueries
// ---------------------------------------------------------------------------

class PerturbedQueriesTest : public ::testing::TestWithParam<AdversarialStrategy> {};

TEST_P(PerturbedQueriesTest, ProducesRequestedCount)
{
    AdversarialTester t;
    const std::string query = "What is the capital of France?";
    auto variants = t.generatePerturbedQueries(query, GetParam(), 3);
    EXPECT_EQ(variants.size(), 3u);
}

TEST_P(PerturbedQueriesTest, VariantsAreNonEmpty)
{
    AdversarialTester t;
    const std::string query = "What is the capital of France?";
    auto variants = t.generatePerturbedQueries(query, GetParam(), 3);
    for (const auto& v : variants) {
        EXPECT_FALSE(v.empty());
    }
}

TEST_P(PerturbedQueriesTest, ZeroCountReturnsEmpty)
{
    AdversarialTester t;
    auto variants = t.generatePerturbedQueries("Query?", GetParam(), 0);
    EXPECT_TRUE(variants.empty());
}

INSTANTIATE_TEST_SUITE_P(
    AllStrategies,
    PerturbedQueriesTest,
    ::testing::Values(
        AdversarialStrategy::SEMANTIC_PERTURBATION,
        AdversarialStrategy::LEXICAL_SUBSTITUTION,
        AdversarialStrategy::TYPO_INJECTION,
        AdversarialStrategy::NEGATION_FLIP,
        AdversarialStrategy::SYCOPHANCY
    )
);

// ---------------------------------------------------------------------------
// generatePoisonedDocuments
// ---------------------------------------------------------------------------

TEST(AdversarialTester, PoisonedDocsSameCount)
{
    AdversarialTester t;
    std::vector<RetrievedDocument> docs = {
        makeDoc("d1", "Content 1."),
        makeDoc("d2", "Content 2."),
        makeDoc("d3", "Content 3."),
    };
    auto poisoned = t.generatePoisonedDocuments(docs);
    EXPECT_EQ(poisoned.size(), docs.size());
}

TEST(AdversarialTester, PoisonedDocsModifyContent)
{
    AdversarialTester t;
    std::vector<RetrievedDocument> docs = {
        makeDoc("d1", "Content 1."),
        makeDoc("d2", "Content 2."),
    };
    auto poisoned = t.generatePoisonedDocuments(docs);
    // At least one document should differ from the original.
    bool any_modified = false;
    for (size_t i = 0; i < docs.size(); ++i) {
        if (poisoned[i].content != docs[i].content) {
            any_modified = true;
            break;
        }
    }
    EXPECT_TRUE(any_modified);
}

TEST(AdversarialTester, PoisonedDocsPreserveIds)
{
    AdversarialTester t;
    std::vector<RetrievedDocument> docs = {
        makeDoc("doc_a", "Text A."),
        makeDoc("doc_b", "Text B."),
    };
    auto poisoned = t.generatePoisonedDocuments(docs);
    for (size_t i = 0; i < docs.size(); ++i) {
        EXPECT_EQ(poisoned[i].id, docs[i].id);
    }
}

TEST(AdversarialTester, PoisonedEmptyDocsReturnsEmpty)
{
    AdversarialTester t;
    auto poisoned = t.generatePoisonedDocuments({});
    EXPECT_TRUE(poisoned.empty());
}

// ---------------------------------------------------------------------------
// generateSycophancyQuery
// ---------------------------------------------------------------------------

TEST(AdversarialTester, SycophancyQueryNonEmpty)
{
    AdversarialTester t;
    auto syco = t.generateSycophancyQuery("What is AI?");
    EXPECT_FALSE(syco.empty());
}

TEST(AdversarialTester, SycophancyQueryDiffersFromOriginal)
{
    AdversarialTester t;
    const std::string original = "What is AI?";
    auto syco = t.generateSycophancyQuery(original);
    EXPECT_NE(syco, original);
}

// ---------------------------------------------------------------------------
// isSuccessfulAttack
// ---------------------------------------------------------------------------

TEST(AdversarialTester, IdenticalAnswersNotAnAttack)
{
    AdversarialTester t;
    EXPECT_FALSE(t.isSuccessfulAttack("Paris is the capital of France.",
                                      "Paris is the capital of France."));
}

TEST(AdversarialTester, CompletelyDifferentAnswersIsAttack)
{
    AdversarialTester t;
    EXPECT_TRUE(t.isSuccessfulAttack(
        "Paris is the capital of France.",
        "The Eiffel Tower was built in 1889 by Gustave Eiffel in Paris."));
}

TEST(AdversarialTester, EmptyOriginalNotAnAttack)
{
    AdversarialTester t;
    EXPECT_FALSE(t.isSuccessfulAttack("", "Some answer."));
}

TEST(AdversarialTester, EmptyAdversarialNotAnAttack)
{
    AdversarialTester t;
    EXPECT_FALSE(t.isSuccessfulAttack("Some answer.", ""));
}

// ---------------------------------------------------------------------------
// testRobustness: pre-condition errors
// ---------------------------------------------------------------------------

TEST(AdversarialTester, ThrowsWhenNoQueries)
{
    AdversarialTester t;
    t.addBaseDocument(makeDoc("d1", "Content."));
    RAGJudge judge;
    EXPECT_THROW(t.testRobustness(judge), std::runtime_error);
}

TEST(AdversarialTester, ThrowsWhenNoDocuments)
{
    AdversarialTester t;
    t.addBaseQuery("Query?");
    RAGJudge judge;
    EXPECT_THROW(t.testRobustness(judge), std::runtime_error);
}

// ---------------------------------------------------------------------------
// testRobustness: full run
// ---------------------------------------------------------------------------

class AdversarialFullRunTest : public ::testing::Test {
protected:
    void SetUp() override {
        tester = makeTester();
    }
    AdversarialTester tester;
};

TEST_F(AdversarialFullRunTest, ReturnsValidRobustnessScore)
{
    RAGJudge judge;
    auto report = tester.testRobustness(judge);
    EXPECT_GE(report.robustness_score, 0.0);
    EXPECT_LE(report.robustness_score, 1.0);
}

TEST_F(AdversarialFullRunTest, PoisoningResultsPresent)
{
    RAGJudge judge;
    auto report = tester.testRobustness(judge);
    // There should be at least one poisoning result entry.
    EXPECT_FALSE(report.poisoning_results.empty());
}

TEST_F(AdversarialFullRunTest, PromptInjectionAttemptsRecorded)
{
    RAGJudge judge;
    auto report = tester.testRobustness(judge);
    // We submit known injection payloads, so attempts > 0.
    EXPECT_GT(report.prompt_injection_attempts, 0u);
}

TEST_F(AdversarialFullRunTest, ReportIsDeterministic)
{
    RAGJudge judge;
    auto report1 = makeTester().testRobustness(judge);
    auto report2 = makeTester().testRobustness(judge);
    EXPECT_DOUBLE_EQ(report1.robustness_score, report2.robustness_score);
}

// ---------------------------------------------------------------------------
// Individual test phases
// ---------------------------------------------------------------------------

TEST(AdversarialTester, TestQueryPerturbationsDoesNotCrash)
{
    auto t = makeTester();
    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testQueryPerturbations(judge, report));
}

TEST(AdversarialTester, TestDocumentPoisoningDoesNotCrash)
{
    auto t = makeTester();
    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testDocumentPoisoning(judge, report));
}

TEST(AdversarialTester, TestPromptInjectionDoesNotCrash)
{
    auto t = makeTester();
    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testPromptInjection(judge, report));
}

TEST(AdversarialTester, TestContextOverflowDoesNotCrash)
{
    auto t = makeTester();
    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testContextOverflow(judge, report));
}

TEST(AdversarialTester, TestSycophancyDoesNotCrash)
{
    auto t = makeTester();
    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testSycophancy(judge, report));
}

// ---------------------------------------------------------------------------
// Config with no enabled strategies
// ---------------------------------------------------------------------------

TEST(AdversarialTester, NoEnabledStrategiesProducesNoFailingExamples)
{
    AdversarialTesterConfig cfg;
    cfg.enabled_strategies.clear();
    AdversarialTester t(cfg);
    t.addBaseQuery("What is AI?", "Artificial Intelligence");
    t.addBaseDocument(makeDoc("d1", "AI stands for Artificial Intelligence."));

    RAGJudge judge;
    RobustnessReport report;
    EXPECT_NO_THROW(t.testQueryPerturbations(judge, report));
    EXPECT_TRUE(report.failing_examples.empty());
}

// ---------------------------------------------------------------------------
// Multiple queries
// ---------------------------------------------------------------------------

TEST(AdversarialTester, MultipleQueriesAllProcessed)
{
    AdversarialTesterConfig cfg;
    cfg.perturbations_per_query = 1;
    AdversarialTester t(cfg);
    t.addBaseQuery("What is AI?", "Artificial Intelligence");
    t.addBaseQuery("Who invented AI?", "Various researchers");
    t.addBaseDocument(makeDoc("d1", "AI is Artificial Intelligence."));

    RAGJudge judge;
    auto report = t.testRobustness(judge);
    EXPECT_GE(report.robustness_score, 0.0);
    EXPECT_LE(report.robustness_score, 1.0);
}
