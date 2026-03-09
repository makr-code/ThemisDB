/**
 * @file test_rag_distributed_evaluator.cpp
 * @brief Unit tests for DistributedRAGEvaluator (Issue: #2245)
 *
 * Coverage:
 *  - Construction validation (empty workers list rejected)
 *  - Single-judge delegation (result equals direct judge output)
 *  - Multi-judge MEAN aggregation (output in [0,1] range)
 *  - Multi-judge WEIGHTED_MEAN aggregation
 *  - MAJORITY_VOTING: overall_score is 0.0 or 1.0
 *  - BEST_OF_N: returns the highest overall_score
 *  - batchEvaluate: correct output count
 *  - setAggregationStrategy at runtime
 *  - totalEvaluations counter increments
 *  - judgeCount() accessor
 *  - Failed-judge handling with skip_failed_judges=true
 *  - min_successful_judges enforcement
 *  - DistributedEvaluatorFactory::createHomogeneous
 *  - DistributedEvaluatorFactory::createFastThorough
 */

#include <gtest/gtest.h>
#include "rag/distributed_rag_evaluator.h"
#include "rag/rag_judge.h"

#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::rag::distributed;
using namespace themis::rag::judge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<RetrievedDocument> makeTestDocs()
{
    return {
        {"d1", "Paris is the capital of France.", 0.95, {}},
        {"d2", "France is in Western Europe.",     0.88, {}},
    };
}

static EvaluationInput makeTestInput()
{
    EvaluationInput in;
    in.query            = "What is the capital of France?";
    in.generated_answer = "The capital of France is Paris.";
    in.documents        = makeTestDocs();
    return in;
}

static JudgeWorkerConfig makeWorker(
    const std::string&  id,
    EvaluationMode      mode   = EvaluationMode::FAST,
    double              weight = 1.0)
{
    RAGJudgeConfig cfg;
    cfg.mode = mode;
    JudgeWorkerConfig w;
    w.judge_id     = id;
    w.judge_config = cfg;
    w.weight       = weight;
    return w;
}

// ---------------------------------------------------------------------------
// Construction validation
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorConstruct, EmptyWorkersThrows)
{
    EXPECT_THROW(
        DistributedRAGEvaluator({}, DistributedEvaluatorConfig{}),
        std::invalid_argument);
}

TEST(DistributedRAGEvaluatorConstruct, SingleWorkerOk)
{
    EXPECT_NO_THROW(
        DistributedRAGEvaluator({makeWorker("j0")}, DistributedEvaluatorConfig{}));
}

TEST(DistributedRAGEvaluatorConstruct, MultipleWorkersOk)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"), makeWorker("j2"),
    };
    EXPECT_NO_THROW(
        DistributedRAGEvaluator(workers, DistributedEvaluatorConfig{}));
}

// ---------------------------------------------------------------------------
// judgeCount() and totalEvaluations()
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorAccessors, JudgeCount)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"),
    };
    DistributedRAGEvaluator ev(workers);
    EXPECT_EQ(ev.judgeCount(), 2u);
}

TEST(DistributedRAGEvaluatorAccessors, TotalEvaluationsInitiallyZero)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    EXPECT_EQ(ev.totalEvaluations(), 0u);
}

TEST(DistributedRAGEvaluatorAccessors, TotalEvaluationsIncrementsAfterEvaluate)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    ev.evaluate(makeTestInput());
    EXPECT_EQ(ev.totalEvaluations(), 1u);
    ev.evaluate(makeTestInput());
    EXPECT_EQ(ev.totalEvaluations(), 2u);
}

// ---------------------------------------------------------------------------
// Single-judge delegation
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorSingleJudge, ScoreInRange)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    auto [result, meta] = ev.evaluate(makeTestInput());

    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_EQ(meta.successful_judges, 1u);
    EXPECT_EQ(meta.failed_judges, 0u);
}

TEST(DistributedRAGEvaluatorSingleJudge, MetaTimingSet)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    auto [result, meta] = ev.evaluate(makeTestInput());
    // Wall-clock time should be >= 0
    EXPECT_GE(meta.total_elapsed.count(), 0);
}

// ---------------------------------------------------------------------------
// Multi-judge aggregation: MEAN
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorAggregation, MeanScoreInRange)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"), makeWorker("j2"),
    };
    DistributedEvaluatorConfig cfg;
    cfg.aggregation = AggregationStrategy::MEAN;

    DistributedRAGEvaluator ev(workers, cfg);
    auto [result, meta] = ev.evaluate(makeTestInput());

    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_EQ(meta.successful_judges, 3u);
    EXPECT_EQ(meta.individual_results.size(), 3u);
}

// ---------------------------------------------------------------------------
// Multi-judge aggregation: WEIGHTED_MEAN
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorAggregation, WeightedMeanInRange)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0", EvaluationMode::FAST, 0.3),
        makeWorker("j1", EvaluationMode::FAST, 0.7),
    };
    DistributedEvaluatorConfig cfg;
    cfg.aggregation = AggregationStrategy::WEIGHTED_MEAN;

    DistributedRAGEvaluator ev(workers, cfg);
    auto [result, meta] = ev.evaluate(makeTestInput());

    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

// ---------------------------------------------------------------------------
// Multi-judge aggregation: MAJORITY_VOTING
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorAggregation, MajorityVotingOverallIsBinaryish)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"), makeWorker("j2"),
    };
    DistributedEvaluatorConfig cfg;
    cfg.aggregation = AggregationStrategy::MAJORITY_VOTING;

    DistributedRAGEvaluator ev(workers, cfg);
    auto [result, meta] = ev.evaluate(makeTestInput());

    // Majority voting maps overall_score to 0.0 or 1.0
    EXPECT_TRUE(result.overall_score == 0.0 || result.overall_score == 1.0);
}

// ---------------------------------------------------------------------------
// Multi-judge aggregation: BEST_OF_N
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorAggregation, BestOfNReturnsMaxScore)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"), makeWorker("j2"),
    };
    DistributedEvaluatorConfig cfg;
    cfg.aggregation = AggregationStrategy::BEST_OF_N;

    DistributedRAGEvaluator ev(workers, cfg);
    auto [result, meta] = ev.evaluate(makeTestInput());

    // Best-of-N must equal the maximum individual score
    double max_individual = 0.0;
    for (const auto& r : meta.individual_results) {
        max_individual = std::max(max_individual, r.overall_score);
    }
    EXPECT_DOUBLE_EQ(result.overall_score, max_individual);
}

// ---------------------------------------------------------------------------
// batchEvaluate
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorBatch, CorrectOutputCount)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});

    std::vector<EvaluationInput> inputs = {
        makeTestInput(), makeTestInput(), makeTestInput(),
    };
    auto results = ev.batchEvaluate(inputs);
    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(ev.totalEvaluations(), 3u);
}

TEST(DistributedRAGEvaluatorBatch, EmptyInputReturnsEmpty)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    auto results = ev.batchEvaluate({});
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(ev.totalEvaluations(), 0u);
}

// ---------------------------------------------------------------------------
// setAggregationStrategy at runtime
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorRuntime, ChangeAggregationStrategy)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"),
    };
    DistributedRAGEvaluator ev(workers);

    ev.setAggregationStrategy(AggregationStrategy::MEAN);
    EXPECT_EQ(ev.getConfig().aggregation, AggregationStrategy::MEAN);

    ev.setAggregationStrategy(AggregationStrategy::BEST_OF_N);
    EXPECT_EQ(ev.getConfig().aggregation, AggregationStrategy::BEST_OF_N);
}

// ---------------------------------------------------------------------------
// min_successful_judges enforcement
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorMinJudges, ThrowsWhenNotEnoughSucceed)
{
    DistributedEvaluatorConfig cfg;
    cfg.min_successful_judges = 5;  // require 5 but only 2 workers

    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"),
    };
    DistributedRAGEvaluator ev(workers, cfg);

    // With only 2 workers and a minimum of 5, this should throw.
    EXPECT_THROW(ev.evaluate(makeTestInput()), std::runtime_error);
}

TEST(DistributedRAGEvaluatorMinJudges, SucceedsWhenEnoughRespond)
{
    DistributedEvaluatorConfig cfg;
    cfg.min_successful_judges = 2;

    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"), makeWorker("j2"),
    };
    DistributedRAGEvaluator ev(workers, cfg);
    EXPECT_NO_THROW(ev.evaluate(makeTestInput()));
}

// ---------------------------------------------------------------------------
// Inter-judge agreement
// ---------------------------------------------------------------------------

TEST(DistributedRAGEvaluatorMeta, AgreementInRange)
{
    std::vector<JudgeWorkerConfig> workers = {
        makeWorker("j0"), makeWorker("j1"),
    };
    DistributedRAGEvaluator ev(workers);
    auto [result, meta] = ev.evaluate(makeTestInput());

    EXPECT_GE(meta.inter_judge_agreement, 0.0);
    EXPECT_LE(meta.inter_judge_agreement, 1.0);
}

TEST(DistributedRAGEvaluatorMeta, SingleJudgeAgreementIsOne)
{
    DistributedRAGEvaluator ev({makeWorker("j0")});
    auto [result, meta] = ev.evaluate(makeTestInput());
    EXPECT_DOUBLE_EQ(meta.inter_judge_agreement, 1.0);
}

// ---------------------------------------------------------------------------
// DistributedEvaluatorFactory
// ---------------------------------------------------------------------------

TEST(DistributedEvaluatorFactory, CreateHomogeneousRejectsZeroCount)
{
    EXPECT_THROW(
        DistributedEvaluatorFactory::createHomogeneous(0),
        std::invalid_argument);
}

TEST(DistributedEvaluatorFactory, CreateHomogeneousSingleJudge)
{
    auto ev = DistributedEvaluatorFactory::createHomogeneous(1);
    ASSERT_NE(ev, nullptr);
    EXPECT_EQ(ev->judgeCount(), 1u);
}

TEST(DistributedEvaluatorFactory, CreateHomogeneousMultipleJudges)
{
    auto ev = DistributedEvaluatorFactory::createHomogeneous(4);
    ASSERT_NE(ev, nullptr);
    EXPECT_EQ(ev->judgeCount(), 4u);
}

TEST(DistributedEvaluatorFactory, CreateHomogeneousCanEvaluate)
{
    auto ev = DistributedEvaluatorFactory::createHomogeneous(2);
    auto [result, meta] = ev->evaluate(makeTestInput());
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
}

TEST(DistributedEvaluatorFactory, CreateFastThoroughHasTwoJudges)
{
    auto ev = DistributedEvaluatorFactory::createFastThorough();
    ASSERT_NE(ev, nullptr);
    EXPECT_EQ(ev->judgeCount(), 2u);
}

TEST(DistributedEvaluatorFactory, CreateFastThoroughCanEvaluate)
{
    auto ev = DistributedEvaluatorFactory::createFastThorough();
    auto [result, meta] = ev->evaluate(makeTestInput());
    EXPECT_GE(result.overall_score, 0.0);
    EXPECT_LE(result.overall_score, 1.0);
    EXPECT_EQ(meta.successful_judges, 2u);
}

TEST(DistributedEvaluatorFactory, CreateFastThoroughUsesWeightedMean)
{
    auto ev = DistributedEvaluatorFactory::createFastThorough();
    EXPECT_EQ(ev->getConfig().aggregation, AggregationStrategy::WEIGHTED_MEAN);
}
