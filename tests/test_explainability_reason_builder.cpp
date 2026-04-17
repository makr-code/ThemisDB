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
    std::string tok;
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
