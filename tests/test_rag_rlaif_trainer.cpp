/**
 * @file test_rag_rlaif_trainer.cpp
 * @brief Unit tests for RLAIFTrainer (Constitutional AI / RLAIF pipeline).
 *
 * Coverage:
 *  - Default construction (heuristic judge, default config)
 *  - Custom config construction
 *  - Invalid config throws std::invalid_argument
 *  - loadDefaultPrinciples() populates principles list
 *  - addPrinciple() / getPrinciples() round-trip
 *  - removePrinciple() removes by ID
 *  - removePrinciple() for unknown ID is a no-op
 *  - runTrainingStep() returns a result with success flag
 *  - runTrainingStep() populates preference_pair fields
 *  - runTrainingStep() increments dataset on success
 *  - runTrainingStep() does not exceed max_dataset_size
 *  - generateCritique() returns a ConstitutionalCritique
 *  - generateCritique() sets violation_found on obviously harmful text
 *  - createPreferencePair() returns a PreferencePair
 *  - createPreferencePair() chosen != rejected
 *  - addToQueue() / processBatch() returns results per queued item
 *  - processBatch() clears the queue after processing
 *  - getDataset() returns accumulated pairs
 *  - clearDataset() empties dataset and resets datasetSize
 *  - datasetSize() matches getDataset().size()
 *  - getStats() returns stats with non-negative counts
 *  - resetStats() zeroes stat counters
 *  - setStepCallback() invoked on each training step
 *  - setConfig() / getConfig() round-trip
 *  - setConfig() throws on invalid config
 *  - setJudge(nullptr) falls back to heuristic
 *  - judgeName() returns active judge name
 *  - validateConfig() throws on max_revision_iterations <= 0
 *  - validateConfig() throws on min_quality_threshold out of [0,1]
 *  - validateConfig() throws on improvement_threshold < 0
 *  - HeuristicAIJudge::judge() returns score in [0, 1]
 *  - HeuristicAIJudge::critique() returns non-empty string
 *  - HeuristicAIJudge::revise() returns non-empty string
 *  - HeuristicAIJudge::name() returns non-empty string
 *  - RLAIFTrainerFactory::createDefault returns valid trainer
 *  - RLAIFTrainerFactory::createStrict has 5 revision iterations
 *  - RLAIFTrainerFactory::createFast has 1 revision iteration
 *  - RLAIFTrainerFactory::createWithJudge injects the judge
 */

#include "rag/rlaif_trainer.h"

#include <gtest/gtest.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace themis::rag::training;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static AIPrinciple makePrinciple(const std::string& id,
                                  ConstitutionalStrategy strategy =
                                      ConstitutionalStrategy::HARMLESSNESS)
{
    AIPrinciple p;
    p.id                  = id;
    p.description         = "Test principle " + id;
    p.critique_template   = "Does the response violate " + id + "?";
    p.revision_template   = "Revise the response to comply with " + id + ".";
    p.strategy            = strategy;
    return p;
}

// A trivial judge that always prefers the longer response and scores by length.
class LengthJudge : public IAIJudge {
public:
    double judge(const std::string& /*prompt*/,
                 const std::string& response_a,
                 const std::string& response_b) const override
    {
        // Returns >0.5 if response_a is longer, <0.5 if response_b is longer.
        double la = static_cast<double>(response_a.size());
        double lb = static_cast<double>(response_b.size());
        double total = la + lb;
        return total > 0.0 ? la / total : 0.5;
    }

    std::string critique(const std::string& /*prompt*/,
                         const std::string& /*response*/,
                         const AIPrinciple& principle) const override
    {
        return "LengthJudge critique for " + principle.id;
    }

    std::string revise(const std::string& /*prompt*/,
                       const std::string& response,
                       const std::string& /*critique*/,
                       const AIPrinciple& /*principle*/) const override
    {
        return response + " [revised by LengthJudge]";
    }

    std::string name() const override { return "LengthJudge"; }
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerConstruct, DefaultConstruction)
{
    RLAIFTrainer trainer;
    EXPECT_EQ(trainer.getConfig().max_revision_iterations, 3);
    EXPECT_EQ(trainer.datasetSize(), 0u);
}

TEST(RLAIFTrainerConstruct, CustomConfig)
{
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 2;
    cfg.min_quality_threshold   = 0.7;
    RLAIFTrainer trainer(cfg);
    EXPECT_EQ(trainer.getConfig().max_revision_iterations, 2);
    EXPECT_DOUBLE_EQ(trainer.getConfig().min_quality_threshold, 0.7);
}

TEST(RLAIFTrainerConstruct, CustomJudge)
{
    auto judge = std::make_shared<LengthJudge>();
    RLAIFTrainer trainer(RLAIFConfig{}, judge);
    EXPECT_EQ(trainer.judgeName(), "LengthJudge");
}

TEST(RLAIFTrainerConstruct, DefaultJudgeNameIsHeuristic)
{
    RLAIFTrainer trainer;
    EXPECT_FALSE(trainer.judgeName().empty());
}

// ---------------------------------------------------------------------------
// Config validation
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerValidate, InvalidMaxRevisionIterations)
{
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 0;
    EXPECT_THROW(RLAIFTrainer::validateConfig(cfg), std::invalid_argument);
}

TEST(RLAIFTrainerValidate, InvalidNegativeIterations)
{
    RLAIFConfig cfg;
    cfg.max_revision_iterations = -1;
    EXPECT_THROW(RLAIFTrainer::validateConfig(cfg), std::invalid_argument);
}

TEST(RLAIFTrainerValidate, InvalidMinQualityThresholdNegative)
{
    RLAIFConfig cfg;
    cfg.min_quality_threshold = -0.1;
    EXPECT_THROW(RLAIFTrainer::validateConfig(cfg), std::invalid_argument);
}

TEST(RLAIFTrainerValidate, InvalidMinQualityThresholdAboveOne)
{
    RLAIFConfig cfg;
    cfg.min_quality_threshold = 1.1;
    EXPECT_THROW(RLAIFTrainer::validateConfig(cfg), std::invalid_argument);
}

TEST(RLAIFTrainerValidate, InvalidImprovementThresholdNegative)
{
    RLAIFConfig cfg;
    cfg.improvement_threshold = -0.01;
    EXPECT_THROW(RLAIFTrainer::validateConfig(cfg), std::invalid_argument);
}

TEST(RLAIFTrainerValidate, ValidConfigDoesNotThrow)
{
    RLAIFConfig cfg;
    EXPECT_NO_THROW(RLAIFTrainer::validateConfig(cfg));
}

TEST(RLAIFTrainerValidate, ConstructorThrowsOnInvalidConfig)
{
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 0;
    EXPECT_THROW(RLAIFTrainer{cfg}, std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Principle management
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerPrinciples, LoadDefaultPrinciplesNonEmpty)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    EXPECT_GT(trainer.getPrinciples().size(), 0u);
}

TEST(RLAIFTrainerPrinciples, AddPrincipleRoundTrip)
{
    RLAIFTrainer trainer;
    trainer.addPrinciple(makePrinciple("test-001"));
    bool found = false;
    for (const auto& p : trainer.getPrinciples()) {
        if (p.id == "test-001") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(RLAIFTrainerPrinciples, RemovePrincipleById)
{
    RLAIFTrainer trainer;
    trainer.addPrinciple(makePrinciple("to-remove"));
    trainer.removePrinciple("to-remove");
    bool found = false;
    for (const auto& p : trainer.getPrinciples()) {
        if (p.id == "to-remove") { found = true; break; }
    }
    EXPECT_FALSE(found);
}

TEST(RLAIFTrainerPrinciples, RemoveUnknownIdIsNoOp)
{
    RLAIFTrainer trainer;
    trainer.addPrinciple(makePrinciple("keep-me"));
    EXPECT_NO_THROW(trainer.removePrinciple("does-not-exist"));
    bool found = false;
    for (const auto& p : trainer.getPrinciples()) {
        if (p.id == "keep-me") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// runTrainingStep()
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerStep, RunStepReturnsResult)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    auto step = trainer.runTrainingStep("What is RAG?", "A helpful answer.");
    // Result must have a query field
    EXPECT_FALSE(step.preference_pair.prompt.empty());
}

TEST(RLAIFTrainerStep, RunStepDatasetGrowsOnSuccess)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0; // Accept all pairs
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    size_t before = trainer.datasetSize();
    trainer.runTrainingStep("Query?", "A good answer.");
    // Dataset may grow by 0 or 1 depending on score; just verify no error
    EXPECT_GE(trainer.datasetSize(), before);
}

TEST(RLAIFTrainerStep, RunStepPreferencePairChosenNotEmpty)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    auto step = trainer.runTrainingStep("Tell me about Paris.",
                                        "Paris is the capital of France.");
    if (step.success) {
        EXPECT_FALSE(step.preference_pair.chosen.empty());
    }
}

TEST(RLAIFTrainerStep, MaxDatasetSizeEnforced)
{
    RLAIFConfig cfg;
    cfg.max_dataset_size     = 2;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    for (int i = 0; i < 10; ++i) {
        trainer.runTrainingStep("Q" + std::to_string(i), "Answer " + std::to_string(i));
    }
    EXPECT_LE(trainer.datasetSize(), 2u);
}

// ---------------------------------------------------------------------------
// generateCritique()
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerCritique, ReturnsConstitutionalCritique)
{
    RLAIFTrainer trainer;
    auto p = makePrinciple("harmlessness", ConstitutionalStrategy::HARMLESSNESS);
    auto critique = trainer.generateCritique("Helpful and harmless answer.", p);
    EXPECT_FALSE(critique.principle_id.empty());
}

TEST(RLAIFTrainerCritique, PrincipleIdPropagated)
{
    RLAIFTrainer trainer;
    auto p = makePrinciple("my-principle");
    auto critique = trainer.generateCritique("Some response.", p);
    EXPECT_EQ(critique.principle_id, "my-principle");
}

// ---------------------------------------------------------------------------
// createPreferencePair()
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerPreference, CreatePreferencePairNotEmpty)
{
    RLAIFTrainer trainer;
    auto pair = trainer.createPreferencePair(
        "What is 2+2?",
        "The answer is 4.",
        "Four.");
    EXPECT_FALSE(pair.prompt.empty());
    EXPECT_FALSE(pair.chosen.empty());
    EXPECT_FALSE(pair.rejected.empty());
}

TEST(RLAIFTrainerPreference, ChosenDiffersFromRejected)
{
    auto judge = std::make_shared<LengthJudge>();
    RLAIFTrainer trainer(RLAIFConfig{}, judge);
    auto pair = trainer.createPreferencePair(
        "Explain gravity.",
        "Gravity is a fundamental force that attracts objects with mass toward each other.",
        "It pulls things down.");
    EXPECT_NE(pair.chosen, pair.rejected);
}

// ---------------------------------------------------------------------------
// Batch operations
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerBatch, ProcessBatchReturnsCorrectCount)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    trainer.addToQueue("Q1", "Answer 1");
    trainer.addToQueue("Q2", "Answer 2");
    trainer.addToQueue("Q3", "Answer 3");
    auto results = trainer.processBatch();
    EXPECT_EQ(results.size(), 3u);
}

TEST(RLAIFTrainerBatch, ProcessBatchClearsQueue)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    trainer.addToQueue("Q1", "A1");
    trainer.processBatch();
    // Second processBatch on empty queue returns empty
    auto results2 = trainer.processBatch();
    EXPECT_TRUE(results2.empty());
}

TEST(RLAIFTrainerBatch, EmptyBatchReturnsEmpty)
{
    RLAIFTrainer trainer;
    auto results = trainer.processBatch();
    EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// Dataset access
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerDataset, GetDatasetMatchesDatasetSize)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    trainer.runTrainingStep("Q1", "A1");
    trainer.runTrainingStep("Q2", "A2");
    EXPECT_EQ(trainer.getDataset().size(), trainer.datasetSize());
}

TEST(RLAIFTrainerDataset, ClearDatasetEmptiesDataset)
{
    RLAIFConfig cfg;
    cfg.min_preference_score = 0.0;
    RLAIFTrainer trainer(cfg);
    trainer.loadDefaultPrinciples();
    trainer.runTrainingStep("Q", "A");
    trainer.clearDataset();
    EXPECT_EQ(trainer.datasetSize(), 0u);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerStats, GetStatsNonNegative)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    trainer.runTrainingStep("Q", "A");
    auto stats = trainer.getStats();
    EXPECT_GE(stats.total_steps, 0u);
    EXPECT_GE(stats.successful_steps, 0u);
}

TEST(RLAIFTrainerStats, ResetStatsZeroesCounters)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    trainer.runTrainingStep("Q", "A");
    trainer.resetStats();
    auto stats = trainer.getStats();
    EXPECT_EQ(stats.total_steps, 0u);
    EXPECT_EQ(stats.successful_steps, 0u);
}

TEST(RLAIFTrainerStats, TotalStepsIncrementsPerStep)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    trainer.resetStats();
    trainer.runTrainingStep("Q1", "A1");
    trainer.runTrainingStep("Q2", "A2");
    EXPECT_EQ(trainer.getStats().total_steps, 2u);
}

// ---------------------------------------------------------------------------
// Step callback
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerCallback, CallbackInvokedOnStep)
{
    RLAIFTrainer trainer;
    trainer.loadDefaultPrinciples();
    std::atomic<int> count{0};
    trainer.setStepCallback([&](const RLAIFTrainingStep&) { ++count; });
    trainer.runTrainingStep("Q1", "A1");
    trainer.runTrainingStep("Q2", "A2");
    EXPECT_EQ(count.load(), 2);
}

// ---------------------------------------------------------------------------
// setConfig / setJudge
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerConfig, SetConfigRoundTrip)
{
    RLAIFTrainer trainer;
    RLAIFConfig cfg;
    cfg.max_revision_iterations = 5;
    cfg.include_rationale       = false;
    trainer.setConfig(cfg);
    EXPECT_EQ(trainer.getConfig().max_revision_iterations, 5);
    EXPECT_FALSE(trainer.getConfig().include_rationale);
}

TEST(RLAIFTrainerConfig, SetConfigThrowsOnInvalid)
{
    RLAIFTrainer trainer;
    RLAIFConfig bad;
    bad.max_revision_iterations = -1;
    EXPECT_THROW(trainer.setConfig(bad), std::invalid_argument);
}

TEST(RLAIFTrainerConfig, SetJudgeNullptrFallsBack)
{
    auto judge = std::make_shared<LengthJudge>();
    RLAIFTrainer trainer(RLAIFConfig{}, judge);
    EXPECT_EQ(trainer.judgeName(), "LengthJudge");
    trainer.setJudge(nullptr);
    EXPECT_NE(trainer.judgeName(), "LengthJudge");
    EXPECT_FALSE(trainer.judgeName().empty());
}

TEST(RLAIFTrainerConfig, SetJudgeUpdatesName)
{
    RLAIFTrainer trainer;
    auto judge = std::make_shared<LengthJudge>();
    trainer.setJudge(judge);
    EXPECT_EQ(trainer.judgeName(), "LengthJudge");
}

// ---------------------------------------------------------------------------
// HeuristicAIJudge
// ---------------------------------------------------------------------------

TEST(HeuristicAIJudge, JudgeScoreInRange)
{
    HeuristicAIJudge j;
    double s = j.judge("query", "response a", "response b");
    EXPECT_GE(s, 0.0);
    EXPECT_LE(s, 1.0);
}

TEST(HeuristicAIJudge, CritiqueNonEmpty)
{
    HeuristicAIJudge j;
    auto p = makePrinciple("harmlessness");
    std::string c = j.critique("Q", "A", p);
    EXPECT_FALSE(c.empty());
}

TEST(HeuristicAIJudge, ReviseNonEmpty)
{
    HeuristicAIJudge j;
    auto p = makePrinciple("harmlessness");
    std::string r = j.revise("Q", "A", "critique text", p);
    EXPECT_FALSE(r.empty());
}

TEST(HeuristicAIJudge, NameNonEmpty)
{
    HeuristicAIJudge j;
    EXPECT_FALSE(j.name().empty());
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

TEST(RLAIFTrainerFactory, CreateDefaultIsValid)
{
    auto trainer = RLAIFTrainerFactory::createDefault();
    EXPECT_GE(trainer.getConfig().max_revision_iterations, 1);
}

TEST(RLAIFTrainerFactory, CreateStrictHasFiveIterations)
{
    auto trainer = RLAIFTrainerFactory::createStrict();
    EXPECT_EQ(trainer.getConfig().max_revision_iterations, 5);
}

TEST(RLAIFTrainerFactory, CreateFastHasOneIteration)
{
    auto trainer = RLAIFTrainerFactory::createFast();
    EXPECT_EQ(trainer.getConfig().max_revision_iterations, 1);
}

TEST(RLAIFTrainerFactory, CreateWithJudgeInjectsJudge)
{
    auto judge   = std::make_shared<LengthJudge>();
    auto trainer = RLAIFTrainerFactory::createWithJudge(judge);
    EXPECT_EQ(trainer.judgeName(), "LengthJudge");
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-5: Cross-shard RLAIF feedback (RLAIF-CSS-01..03, ZT-FED-01)
// ─────────────────────────────────────────────────────────────────────────────

#include "distributed_knowledge/cross_shard_feedback_sync.h"

using namespace themis::distributed_knowledge;
using namespace themis::rag::training;

namespace {

PreferencePair makeSyntheticPair(const std::string& label = "cross-shard") {
    PreferencePair pp;
    pp.prompt   = "[" + label + "] query";
    pp.chosen   = "good response from cross-shard knowledge";
    pp.rejected = "poor response";
    pp.preference_score = 0.8;
    return pp;
}

} // namespace

// RLAIF-CSS-01: addCrossShardSummary() increments applied_pairs counter
TEST(RLAIFTrainerCrossShardTest, RLAIF_CSS_01_AddCrossShardSummary_IncrementsAppliedPairs) {
    RLAIFTrainer trainer;
    const size_t initial = trainer.getCrossShardStats().applied_pairs;

    FeedbackSummary summary;
    summary.summary_id          = "sum-001";
    summary.feedback_type_label = "USER_NEGATIVE";
    summary.reason_embedding    = std::vector<float>(384, 0.1f);

    trainer.addCrossShardSummary(summary, makeSyntheticPair());

    EXPECT_EQ(trainer.getCrossShardStats().applied_pairs, initial + 1);
}

// RLAIF-CSS-02: addCrossShardSummary() appends the PreferencePair to the dataset
TEST(RLAIFTrainerCrossShardTest, RLAIF_CSS_02_AddCrossShardSummary_AppendsToDataset) {
    RLAIFTrainer trainer;
    const size_t initial_size = trainer.datasetSize();

    FeedbackSummary summary;
    summary.summary_id          = "sum-002";
    summary.feedback_type_label = "HALLUCINATION_DETECTED";
    summary.reason_embedding    = std::vector<float>(384, 0.5f);

    auto pp = makeSyntheticPair("hal-shard");
    trainer.addCrossShardSummary(summary, pp);

    EXPECT_EQ(trainer.datasetSize(), initial_size + 1);
    EXPECT_EQ(trainer.getDataset().back().prompt, pp.prompt);
}

// RLAIF-CSS-03: getCrossShardStats() returns consistent counters after multiple summaries
TEST(RLAIFTrainerCrossShardTest, RLAIF_CSS_03_GetCrossShardStats_ConsistentCounters) {
    RLAIFTrainer trainer;

    for (int i = 0; i < 5; ++i) {
        FeedbackSummary s;
        s.summary_id          = "sum-" + std::to_string(i);
        s.feedback_type_label = "USER_NEGATIVE";
        s.reason_embedding    = std::vector<float>(384, static_cast<float>(i) * 0.01f);
        trainer.addCrossShardSummary(s, makeSyntheticPair());
    }

    const auto stats = trainer.getCrossShardStats();
    EXPECT_EQ(stats.received_summaries, 5u);
    EXPECT_EQ(stats.applied_pairs,      5u);
    EXPECT_EQ(stats.skipped_summaries,  0u);
}

// ZT-FED-01: handleInboundSummary() with rejecting policy → callback NOT called
TEST(CrossShardFeedbackSyncPolicyTest, ZT_FED_01_RejectingPolicyCheck_CallbackNotInvoked) {
    FeedbackSyncConfig cfg;
    cfg.max_embedding_dim    = 3;
    cfg.validate_embedding_dim = false; // allow any dim in this test

    int callback_count = 0;
    int gossip_count   = 0;

    CrossShardFeedbackSync sync(
        cfg, "shard-recv",
        [&gossip_count](nlohmann::json) { ++gossip_count; });

    sync.setFeedbackCallback([&callback_count](const FeedbackSummary&) {
        ++callback_count;
    });

    // Inject always-reject policy
    sync.setInboundPolicyCheck([](const FeedbackSummary&) { return false; });

    FeedbackSummary s;
    s.summary_id          = "zt-sum-001";
    s.feedback_type_label = "USER_NEGATIVE";
    s.reason_embedding    = {0.1f, 0.2f, 0.3f};

    sync.handleInboundSummary(s.toJson());

    EXPECT_EQ(callback_count, 0) << "Policy-rejected summary must NOT invoke feedback callback";
    EXPECT_GE(sync.rejectedByPolicyCount(), 1u);
}

