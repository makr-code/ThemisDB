// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B9 / S-8: ExplainabilityReasonBuilder unit tests
//
// Tests:
//   ERB-01  build() HNSW_PARAMS_UPDATED → non-empty signal + decision
//   ERB-02  build() BAO_PLAN_SELECTED → non-empty signal + decision
//   ERB-03  build() TX_SEMANTIC_HINT → non-empty signal + decision
//   ERB-04  build() INTENT_ALERT confidence > 0.9 → dba_action_required = true
//   ERB-05  build() HNSW_PARAMS_UPDATED guardrail_passed=true → dba_action_required = false
//   ERB-06  build() unknown decision_type → fallback template, no crash
//   ERB-07  toNaturalLanguage() returns text ≥ 50 words
//   ERB-08  toNaturalLanguage() returns text ≤ 500 words
//   ERB-09  enrichAuditor() returns count of enriched records
//   ERB-10  build() FEDERATED_ROUND → non-empty signal + decision

#include <gtest/gtest.h>
#include "rag/explainability_reason_builder.h"

#include <chrono>
#include <sstream>
#include <string>

using namespace themis::rag;

namespace {

/// Count words in a string (split on whitespace).
static size_t wordCount(const std::string& text)
{
    std::istringstream iss(text);
    size_t n = 0;
    std::string tok = {};
    while (iss >> tok) ++n;
    return n;
}

/// Build a minimal AIDecisionRecord for a given decision_type.
static AIDecisionRecord makeRecord(
    const std::string& decision_type,
    double confidence   = 0.8,
    bool guardrail      = true,
    const std::string& shard = "shard-0")
{
    AIDecisionRecord rec;
    rec.decision_type    = decision_type;
    rec.decision_id      = "test-id-001";
    rec.confidence       = confidence;
    rec.guardrail_passed = guardrail;
    rec.shard_id         = shard;
    return rec;
}

} // namespace

// ---------------------------------------------------------------------------
// ERB-01  HNSW_PARAMS_UPDATED → known template, non-empty signal + decision
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, HnswParamsUpdated)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("HNSW_PARAMS_UPDATED", 0.91, true);
    auto chain = erb.build(rec);

    EXPECT_EQ(chain.decision_type, "HNSW_PARAMS_UPDATED");
    EXPECT_FALSE(chain.signal.empty());
    EXPECT_FALSE(chain.decision.empty());
    EXPECT_DOUBLE_EQ(chain.confidence, 0.91);
}

// ---------------------------------------------------------------------------
// ERB-02  BAO_PLAN_SELECTED → non-empty signal + decision
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, BaoPlanSelected)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("BAO_PLAN_SELECTED", 0.78);
    auto chain = erb.build(rec);

    EXPECT_FALSE(chain.signal.empty());
    EXPECT_FALSE(chain.decision.empty());
    EXPECT_EQ(chain.decision_type, "BAO_PLAN_SELECTED");
}

// ---------------------------------------------------------------------------
// ERB-03  TX_SEMANTIC_HINT → non-empty signal + decision
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, TxSemanticHint)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("TX_SEMANTIC_HINT", 0.75);
    auto chain = erb.build(rec);

    EXPECT_FALSE(chain.signal.empty());
    EXPECT_FALSE(chain.decision.empty());
}

// ---------------------------------------------------------------------------
// ERB-04  INTENT_ALERT confidence > 0.9 → dba_action_required = true
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, IntentAlertHighConfidenceRequiresDba)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("INTENT_ALERT", 0.95, true);
    auto chain = erb.build(rec);

    EXPECT_TRUE(chain.dba_action_required);
}

// ---------------------------------------------------------------------------
// ERB-05  HNSW_PARAMS_UPDATED with guardrail_passed=true → dba_action_required = false
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, HnswGuardrailPassedNoDba)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("HNSW_PARAMS_UPDATED", 0.88, true /*guardrail passed*/);
    auto chain = erb.build(rec);

    EXPECT_FALSE(chain.dba_action_required);
}

// ---------------------------------------------------------------------------
// ERB-06  Unknown decision_type → fallback template, no crash, non-empty output
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, UnknownDecisionTypeFallback)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("COMPLETELY_UNKNOWN_TYPE_XYZ", 0.5);
    ASSERT_NO_THROW({
        auto chain = erb.build(rec);
        EXPECT_FALSE(chain.signal.empty());
        EXPECT_FALSE(chain.decision.empty());
    });
}

// ---------------------------------------------------------------------------
// ERB-07  toNaturalLanguage() returns text ≥ 50 words
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, ToNaturalLanguageMinWords)
{
    ExplainabilityReasonBuilder erb;
    auto rec   = makeRecord("HNSW_PARAMS_UPDATED", 0.85, true);
    auto chain = erb.build(rec);
    auto text  = erb.toNaturalLanguage(chain);

    EXPECT_GE(wordCount(text), 50u);
}

// ---------------------------------------------------------------------------
// ERB-08  toNaturalLanguage() returns text ≤ 500 words
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, ToNaturalLanguageMaxWords)
{
    ExplainabilityReasonBuilder erb;
    auto rec   = makeRecord("FEDERATED_ROUND", 0.90, true);
    auto chain = erb.build(rec);
    auto text  = erb.toNaturalLanguage(chain);

    EXPECT_LE(wordCount(text), 500u);
}

// ---------------------------------------------------------------------------
// ERB-09  enrichAuditor() returns count == number of records in the vector
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, EnrichAuditorReturnsCount)
{
    ExplainabilityReasonBuilder erb;

    std::vector<AIDecisionRecord> records = {
        makeRecord("HNSW_PARAMS_UPDATED", 0.85),
        makeRecord("BAO_PLAN_SELECTED",   0.72),
        makeRecord("TX_SEMANTIC_HINT",    0.68),
    };

    const size_t enriched = erb.enrichAuditor(records);

    EXPECT_EQ(enriched, records.size());

    // Each record should now have an "_explanation" parameter
    for (const auto& rec : records) {
        EXPECT_FALSE(rec.parameters.at("_explanation").empty());
    }
}

// ---------------------------------------------------------------------------
// ERB-10  FEDERATED_ROUND → non-empty signal + decision
// ---------------------------------------------------------------------------
TEST(ExplainabilityReasonBuilderTest, FederatedRound)
{
    ExplainabilityReasonBuilder erb;
    auto rec = makeRecord("FEDERATED_ROUND", 0.82, true, "shard-3");
    auto chain = erb.build(rec);

    EXPECT_FALSE(chain.signal.empty());
    EXPECT_FALSE(chain.decision.empty());
    // shard_id should appear in the signal
    EXPECT_NE(chain.signal.find("shard-3"), std::string::npos);
}

// ---------------------------------------------------------------------------
// FADA-01  FederatedAIDecisionAuditor: mergeTimeline without fetcher
// ---------------------------------------------------------------------------
TEST(FederatedAIDecisionAuditorTest, FADA_01_MergeTimelineLocalOnly)
{
    using namespace themis::rag;
    using namespace std::chrono;
    FederatedAIDecisionAuditor auditor;

    AIDecisionRecord r1;
    r1.decision_type = "HNSW_PARAMS_UPDATED";
    r1.timestamp     = system_clock::time_point{milliseconds{100}};

    AIDecisionRecord r2;
    r2.decision_type = "BAO_PLAN_SELECTED";
    r2.timestamp     = system_clock::time_point{milliseconds{50}};

    auditor.addShard("shard-A", {r1});
    auditor.addShard("shard-B", {r2});

    auto timeline = auditor.mergeTimeline();
    ASSERT_EQ(timeline.size(), 2u);
    // Oldest first (timestamp=50 before timestamp=100)
    EXPECT_EQ(timeline[0].timestamp, system_clock::time_point{milliseconds{50}});
    EXPECT_EQ(timeline[1].timestamp, system_clock::time_point{milliseconds{100}});
    EXPECT_EQ(timeline[0].shard_id, "shard-B");
    EXPECT_EQ(timeline[1].shard_id, "shard-A");
}

// ---------------------------------------------------------------------------
// FADA-02  setShardRecordFetcher injects remote records into timeline
// ---------------------------------------------------------------------------
TEST(FederatedAIDecisionAuditorTest, FADA_02_FetcherAugmentsTimeline)
{
    using namespace themis::rag;
    using namespace std::chrono;
    FederatedAIDecisionAuditor auditor;

    AIDecisionRecord local;
    local.decision_type = "LOCAL_DECISION";
    local.timestamp     = system_clock::time_point{milliseconds{200}};
    auditor.addShard("shard-X", {local});

    auditor.setShardRecordFetcher([](const std::string& shard_id) {
        AIDecisionRecord remote;
        remote.decision_type = "REMOTE_DECISION_" + shard_id;
        remote.timestamp     = system_clock::time_point{milliseconds{100}};
        return std::vector<AIDecisionRecord>{remote};
    });

    auto timeline = auditor.mergeTimeline();
    ASSERT_EQ(timeline.size(), 2u);
    // Remote record (ts=100ms) should come first
    EXPECT_LT(timeline[0].timestamp, timeline[1].timestamp);
    EXPECT_EQ(timeline[0].decision_type, "REMOTE_DECISION_shard-X");
    EXPECT_EQ(timeline[0].shard_id, "shard-X");
    EXPECT_EQ(timeline[1].decision_type, "LOCAL_DECISION");
}

// ---------------------------------------------------------------------------
// FADA-03  Clearing the fetcher reverts to local-only behaviour
// ---------------------------------------------------------------------------
TEST(FederatedAIDecisionAuditorTest, FADA_03_ClearFetcherRevertsToLocal)
{
    using namespace themis::rag;
    using namespace std::chrono;
    FederatedAIDecisionAuditor auditor;

    AIDecisionRecord local;
    local.decision_type = "LOCAL";
    local.timestamp     = system_clock::time_point{milliseconds{1}};
    auditor.addShard("shard-Y", {local});

    auditor.setShardRecordFetcher([](const std::string&) {
        AIDecisionRecord r;
        r.decision_type = "REMOTE";
        r.timestamp     = system_clock::time_point{milliseconds{0}};
        return std::vector<AIDecisionRecord>{r};
    });

    // Clear the fetcher
    auditor.setShardRecordFetcher({});

    auto timeline = auditor.mergeTimeline();
    ASSERT_EQ(timeline.size(), 1u);
    EXPECT_EQ(timeline[0].decision_type, "LOCAL");
}

// ── ERB-NL-01: default template path still works after adding injection API
TEST(ExplainabilityReasonBuilderNlGen, ERB_NL_01_DefaultTemplateUnchanged) {
    using namespace themis::rag;
    ExplainabilityReasonBuilder erb;
    AIDecisionRecord rec;
    rec.decision_type = "HNSW_PARAMS_UPDATED";
    rec.confidence    = 0.9;
    auto chain = erb.build(rec);
    auto nl    = erb.toNaturalLanguage(chain);
    EXPECT_FALSE(nl.empty());
    EXPECT_NE(nl.find("HNSW_PARAMS_UPDATED"), std::string::npos);
    EXPECT_NE(nl.find("Confidence"), std::string::npos);
}

// ── ERB-NL-02: injected fn is called and its result returned
TEST(ExplainabilityReasonBuilderNlGen, ERB_NL_02_InjectedFnCalled) {
    using namespace themis::rag;
    ExplainabilityReasonBuilder erb;
    bool called = false;
    erb.setNlGeneratorFn([&called](const ExplainabilityReasonBuilder::CausalChain& c) {
        called = true;
        return std::string("custom: ") + c.decision_type;
    });
    AIDecisionRecord rec;
    rec.decision_type = "BAO_PLAN_SELECTED";
    rec.confidence    = 0.75;
    auto chain = erb.build(rec);
    auto nl    = erb.toNaturalLanguage(chain);
    EXPECT_TRUE(called);
    EXPECT_EQ(nl, "custom: BAO_PLAN_SELECTED");
}

// ── ERB-NL-03: clearing the fn reverts to template path
TEST(ExplainabilityReasonBuilderNlGen, ERB_NL_03_ClearFnRevertsToTemplate) {
    using namespace themis::rag;
    ExplainabilityReasonBuilder erb;
    erb.setNlGeneratorFn([](const ExplainabilityReasonBuilder::CausalChain&) {
        return std::string("custom");
    });
    // Clear the fn
    erb.setNlGeneratorFn({});
    AIDecisionRecord rec;
    rec.decision_type = "LOOP_TRIGGER";
    rec.confidence    = 0.5;
    auto chain = erb.build(rec);
    auto nl    = erb.toNaturalLanguage(chain);
    EXPECT_NE(nl.find("LOOP_TRIGGER"), std::string::npos);
    EXPECT_EQ(nl.find("custom"), std::string::npos);
}

// ── ERB-NL-04: fn returning empty string falls back to template
TEST(ExplainabilityReasonBuilderNlGen, ERB_NL_04_EmptyReturnFallsBackToTemplate) {
    using namespace themis::rag;
    ExplainabilityReasonBuilder erb;
    erb.setNlGeneratorFn([](const ExplainabilityReasonBuilder::CausalChain&) {
        return std::string{};  // intentionally empty
    });
    AIDecisionRecord rec;
    rec.decision_type = "INTENT_ALERT";
    rec.confidence    = 0.95;
    auto chain = erb.build(rec);
    auto nl    = erb.toNaturalLanguage(chain);
    // Should fall back to template since fn returned empty
    EXPECT_FALSE(nl.empty());
    EXPECT_NE(nl.find("INTENT_ALERT"), std::string::npos);
}
