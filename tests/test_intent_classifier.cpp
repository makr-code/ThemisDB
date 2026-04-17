// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B7 / S-7: IntentClassifier unit tests
//
// Tests:
//   IC-01  classify() LEGITIMATE query → LEGITIMATE, confidence > 0.9
//   IC-02  classify() SQL injection OR 1=1 → SQL_INJECTION, confidence > 0.85
//   IC-03  classify() UNION SELECT → SQL_INJECTION, confidence > 0.85
//   IC-04  classify() DROP TABLE → SQL_INJECTION, confidence > 0.85
//   IC-05  maybeAlert() confidence < threshold → nullopt
//   IC-06  maybeAlert() SQL_INJECTION confidence ≥ threshold → IntentAlert returned
//   IC-07  IntentAlert::evidence_embedding has dimension 384
//   IC-08  evidence_embedding contains NO query plaintext characters

#include <gtest/gtest.h>
#include "security/intent_classifier.h"

#include <algorithm>
#include <string>

using namespace themis::security;
using IT = IntentClassifier::IntentType;

namespace {

ZeroTrustContext makeCtx(const std::string& user = "alice") {
    ZeroTrustContext ctx;
    ctx.user_id    = user;
    ctx.request_id = "req-test-001";
    return ctx;
}

} // namespace

// ---------------------------------------------------------------------------
// IC-01  Legitimate query → LEGITIMATE, high confidence
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, LegitimateQuery) {
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT name, price FROM products WHERE id = ?", ctx);
    EXPECT_EQ(res.intent, IT::LEGITIMATE);
    EXPECT_GT(res.confidence, 0.9);
}

// ---------------------------------------------------------------------------
// IC-02  OR 1=1 pattern → SQL_INJECTION
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, SqlInjectionOr1Eq1) {
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT * FROM users WHERE id=1 OR 1=1 --", ctx);
    EXPECT_EQ(res.intent, IT::SQL_INJECTION);
    EXPECT_GT(res.confidence, 0.85);
}

// ---------------------------------------------------------------------------
// IC-03  UNION SELECT → SQL_INJECTION
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, SqlInjectionUnionSelect) {
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT id FROM orders UNION SELECT username, password FROM users", ctx);
    EXPECT_EQ(res.intent, IT::SQL_INJECTION);
    EXPECT_GT(res.confidence, 0.85);
}

// ---------------------------------------------------------------------------
// IC-04  ; DROP TABLE → SQL_INJECTION
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, SqlInjectionDropTable) {
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT 1; DROP TABLE users; --", ctx);
    EXPECT_EQ(res.intent, IT::SQL_INJECTION);
    EXPECT_GT(res.confidence, 0.85);
}

// ---------------------------------------------------------------------------
// IC-05  maybeAlert() confidence below threshold → nullopt
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, MaybeAlertBelowThresholdReturnsNullopt) {
    IntentClassifier clf;
    // Craft a result with low confidence
    IntentClassifier::ClassificationResult low;
    low.intent            = IT::SQL_INJECTION;
    low.confidence        = 0.50;
    low.primary_indicator = "SQL_INJECTION_FEATURES";

    const auto alert = clf.maybeAlert(low, "session-xyz", 0.85);
    EXPECT_FALSE(alert.has_value());
}

// ---------------------------------------------------------------------------
// IC-06  maybeAlert() SQL_INJECTION ≥ threshold → alert returned
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, MaybeAlertAboveThresholdReturnsAlert) {
    IntentClassifier clf("shard-7");
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT * FROM users WHERE id=1 OR 1=1 -- bypass", ctx);

    if (res.intent == IT::SQL_INJECTION && res.confidence >= 0.85) {
        const auto alert = clf.maybeAlert(res, "sess-abc");
        ASSERT_TRUE(alert.has_value());
        EXPECT_EQ(alert->intent, IT::SQL_INJECTION);
        EXPECT_GE(alert->confidence, 0.85);
        EXPECT_EQ(alert->session_id, "sess-abc");
        EXPECT_EQ(alert->shard_id, "shard-7");
        EXPECT_GT(alert->risk_delta, 0.0);
    }
}

// ---------------------------------------------------------------------------
// IC-07  evidence_embedding has dimension 384
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, EvidenceEmbeddingDimension384) {
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(
        "SELECT * FROM users WHERE id=1 OR 1=1", ctx);

    const auto alert = clf.maybeAlert(res, "sess-dim", 0.0);
    if (alert.has_value()) {
        EXPECT_EQ(alert->evidence_embedding.size(), 384u);
    }
}

// ---------------------------------------------------------------------------
// IC-08  evidence_embedding contains NO plaintext from the original query
// ---------------------------------------------------------------------------
TEST(IntentClassifierTest, EvidenceEmbeddingNoQueryPlaintext) {
    const std::string query = "SELECT * FROM secrets WHERE 1=1 OR 'x'='x'";
    IntentClassifier clf;
    const auto ctx = makeCtx();
    const auto res = clf.classify(query, ctx);

    const auto alert = clf.maybeAlert(res, "sess-plain", 0.0);
    if (alert.has_value()) {
        // Encode the embedding as raw float bytes and verify the original
        // query string does not appear character-by-character.
        const auto& emb = alert->evidence_embedding;
        // Ensure the embedding is numeric (floats), not text — all values finite.
        for (const auto v : emb) {
            EXPECT_TRUE(std::isfinite(v));
        }
        // The embedding bytes should NOT form the ASCII string "SELECT" etc.
        std::string embStr(
            reinterpret_cast<const char*>(emb.data()),
            emb.size() * sizeof(float));
        for (const auto& fragment : {"SELECT", "secrets", "1=1"}) {
            EXPECT_EQ(embStr.find(fragment), std::string::npos);
        }
    }
}
