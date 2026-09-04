/**
 * @file tests/rag/test_self_rag.cpp
 * @brief Unit tests for SelfRAGController — SELF_RAG-01..12
 *
 * Coverage:
 *   SELF_RAG-01  Default-constructed controller never crashes on shouldRetrieve()
 *   SELF_RAG-02  shouldRetrieve() returns true when confidence < threshold
 *   SELF_RAG-03  shouldRetrieve() returns false when confidence >= threshold
 *   SELF_RAG-04  runRefinementLoop() throws when no retrieval callback is set
 *   SELF_RAG-05  runRefinementLoop() returns empty result when shouldRetrieve() is false
 *   SELF_RAG-06  criticDocuments() uses retrieval score as proxy without critic cb
 *   SELF_RAG-07  criticDocuments() uses injected critic callback when set
 *   SELF_RAG-08  criticDocuments() respects Relevant / Partial / Irrelevant thresholds
 *   SELF_RAG-09  runRefinementLoop() stops early when target_relevant_docs reached
 *   SELF_RAG-10  runRefinementLoop() runs up to max_rounds when target not met
 *   SELF_RAG-11  deduplication: same document id is not scored twice across rounds
 *   SELF_RAG-12  reset() clears seen-ids so repeated calls work independently
 */

#include <gtest/gtest.h>
#include "rag/self_rag.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

using namespace themis::rag;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static SelfRAGDocument makeDoc(const std::string& id, double score) {
    return {id, "content of " + id, score};
}

// Returns a retrieval callback that always delivers `docs`.
static SelfRAGController::RetrievalCallback makeConstRetriever(
        std::vector<SelfRAGDocument> docs)
{
    return [docs](const std::string& /*query*/, size_t /*top_k*/) {
        return docs;
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-01  Default controller is safe to call
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_01_DefaultConstructSafe) {
    SelfRAGController ctrl;
    // shouldRetrieve with confidence=0 should not crash
    EXPECT_NO_THROW(ctrl.shouldRetrieve("what is RotatE?", 0.0));
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-02  shouldRetrieve returns true when confidence < threshold
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_02_ShouldRetrieveTrue) {
    SelfRAGConfig cfg;
    cfg.retrieval_confidence_threshold = 0.6;
    SelfRAGController ctrl(cfg);

    EXPECT_TRUE(ctrl.shouldRetrieve("query", 0.3));
    EXPECT_TRUE(ctrl.shouldRetrieve("query", 0.0));
    EXPECT_TRUE(ctrl.shouldRetrieve("query", 0.59));
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-03  shouldRetrieve returns false when confidence >= threshold
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_03_ShouldRetrieveFalse) {
    SelfRAGConfig cfg;
    cfg.retrieval_confidence_threshold = 0.6;
    SelfRAGController ctrl(cfg);

    EXPECT_FALSE(ctrl.shouldRetrieve("query", 0.6));
    EXPECT_FALSE(ctrl.shouldRetrieve("query", 0.9));
    EXPECT_FALSE(ctrl.shouldRetrieve("query", 1.0));
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-03b  evidence-seeking queries can trigger retrieval near threshold
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_03b_EvidenceQueryTriggersRetrieve) {
    SelfRAGConfig cfg;
    cfg.retrieval_confidence_threshold = 0.6;
    SelfRAGController ctrl(cfg);

    EXPECT_TRUE(ctrl.shouldRetrieve(
        "How do benchmark metrics compare according to cited sources?", 0.75));
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-04  runRefinementLoop throws without retrieval callback
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_04_ThrowsWithoutCallback) {
    SelfRAGConfig cfg;
    cfg.retrieval_confidence_threshold = 1.0; // always retrieve
    SelfRAGController ctrl(cfg);

    EXPECT_THROW(ctrl.runRefinementLoop("query", 0.0), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-05  runRefinementLoop returns empty result when retrieval not triggered
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_05_NoRetrievalReturnsEmpty) {
    SelfRAGConfig cfg;
    cfg.retrieval_confidence_threshold = 0.0; // never retrieve (threshold at 0)
    SelfRAGController ctrl(cfg);
    ctrl.setRetrievalCallback(makeConstRetriever({makeDoc("d1", 0.9)}));

    auto result = ctrl.runRefinementLoop("query", 1.0); // confidence=1.0 >= 0.0 threshold
    EXPECT_FALSE(result.retrieval_triggered);
    EXPECT_TRUE(result.relevant_docs.empty());
    EXPECT_EQ(result.total_rounds_used, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-06  criticDocuments uses doc.score as proxy (no critic callback)
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_06_CriticUsesDocScore) {
    SelfRAGConfig cfg;
    cfg.relevant_threshold = 0.7;
    cfg.partial_threshold  = 0.4;
    SelfRAGController ctrl(cfg);

    std::vector<SelfRAGDocument> docs = {
        makeDoc("d1", 0.9),  // Relevant
        makeDoc("d2", 0.5),  // Partial
        makeDoc("d3", 0.2),  // Irrelevant
    };

    auto rated = ctrl.criticDocuments("q", docs);
    ASSERT_EQ(rated.size(), 3u);
    EXPECT_EQ(rated[0].verdict, CriticVerdict::Relevant);
    EXPECT_EQ(rated[1].verdict, CriticVerdict::Partial);
    EXPECT_EQ(rated[2].verdict, CriticVerdict::Irrelevant);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-06b  fallback critic includes lexical overlap with query
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_06b_CriticUsesQueryDocumentOverlap) {
    SelfRAGConfig cfg;
    cfg.relevant_threshold = 0.7;
    cfg.partial_threshold  = 0.4;
    SelfRAGController ctrl(cfg);

    std::vector<SelfRAGDocument> docs = {
        {"overlap", "rotate embedding model improves knowledge graph completion", 0.2},
        {"unrelated", "kafka broker retention settings and partition layout", 0.2},
    };

    auto rated = ctrl.criticDocuments("what is rotate embedding model", docs);
    ASSERT_EQ(rated.size(), 2u);
    EXPECT_GT(rated[0].critic_score, rated[1].critic_score);
    EXPECT_EQ(rated[0].verdict, CriticVerdict::Partial);
    EXPECT_EQ(rated[1].verdict, CriticVerdict::Irrelevant);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-07  criticDocuments uses injected CriticCallback
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_07_CriticCallbackUsed) {
    SelfRAGConfig cfg;
    cfg.relevant_threshold = 0.5;
    cfg.partial_threshold  = 0.25;
    SelfRAGController ctrl(cfg);

    // Critic always returns 1.0 regardless of doc.score
    ctrl.setCriticCallback([](const std::string& /*q*/,
                               const SelfRAGDocument& /*d*/) -> double {
        return 1.0;
    });

    std::vector<SelfRAGDocument> docs = {makeDoc("d1", 0.0)}; // score 0 would be Irrelevant
    auto rated = ctrl.criticDocuments("q", docs);

    ASSERT_EQ(rated.size(), 1u);
    EXPECT_EQ(rated[0].verdict, CriticVerdict::Relevant); // critic overrides score
    EXPECT_DOUBLE_EQ(rated[0].critic_score, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-08  criticDocuments thresholds are correct
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_08_CriticThresholdBoundaries) {
    SelfRAGConfig cfg;
    cfg.relevant_threshold = 0.7;
    cfg.partial_threshold  = 0.4;
    SelfRAGController ctrl(cfg);

    // Exactly at relevant threshold
    auto r1 = ctrl.criticDocuments("q", {makeDoc("d1", 0.7)});
    EXPECT_EQ(r1[0].verdict, CriticVerdict::Relevant);

    // Exactly at partial threshold
    auto r2 = ctrl.criticDocuments("q", {makeDoc("d2", 0.4)});
    EXPECT_EQ(r2[0].verdict, CriticVerdict::Partial);

    // Just below partial threshold
    auto r3 = ctrl.criticDocuments("q", {makeDoc("d3", 0.39)});
    EXPECT_EQ(r3[0].verdict, CriticVerdict::Irrelevant);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-09  runRefinementLoop stops early when target_relevant_docs met
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_09_EarlyStop) {
    SelfRAGConfig cfg;
    cfg.max_rounds                     = 5;
    cfg.top_k                          = 2;
    cfg.target_relevant_docs           = 2;
    cfg.relevant_threshold             = 0.7;
    cfg.partial_threshold              = 0.4;
    cfg.retrieval_confidence_threshold = 1.0; // always retrieve

    SelfRAGController ctrl(cfg);

    // Supply two relevant docs per round → target met on round 1.
    ctrl.setRetrievalCallback([](const std::string&, size_t top_k) {
        std::vector<SelfRAGDocument> docs = {};

        for (size_t i = 0; i < top_k; ++i) {
            docs.push_back({"id-" + std::to_string(i), "text", 0.9});
        }
        return docs;
    });

    auto result = ctrl.runRefinementLoop("query", 0.0);
    EXPECT_TRUE(result.retrieval_triggered);
    EXPECT_EQ(result.total_rounds_used, 1u);
    EXPECT_TRUE(result.round_stats[0].stop_early);
    EXPECT_GE(result.relevant_docs.size(), cfg.target_relevant_docs);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-10  runRefinementLoop runs up to max_rounds
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_10_RunsMaxRounds) {
    SelfRAGConfig cfg;
    cfg.max_rounds                     = 3;
    cfg.top_k                          = 2;
    cfg.target_relevant_docs           = 100; // impossible target
    cfg.relevant_threshold             = 0.9;
    cfg.partial_threshold              = 0.8;
    cfg.retrieval_confidence_threshold = 1.0; // always retrieve

    SelfRAGController ctrl(cfg);

    // Each round returns unique IDs with low score (Irrelevant).
    std::atomic<size_t> call_count{0};
    ctrl.setRetrievalCallback([&](const std::string&, size_t top_k) {
        size_t base = call_count.fetch_add(top_k);
        std::vector<SelfRAGDocument> docs = {};

        for (size_t i = 0; i < top_k; ++i) {
            docs.push_back({"id-" + std::to_string(base + i), "text", 0.1});
        }
        return docs;
    });

    auto result = ctrl.runRefinementLoop("query", 0.0);
    EXPECT_EQ(result.total_rounds_used, cfg.max_rounds);
    EXPECT_EQ(result.round_stats.size(), cfg.max_rounds);
    EXPECT_FALSE(result.round_stats.back().stop_early);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-11  Deduplication: same doc id not scored twice
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_11_Deduplication) {
    SelfRAGConfig cfg;
    cfg.max_rounds                     = 3;
    cfg.top_k                          = 2;
    cfg.target_relevant_docs           = 100; // never stop early
    cfg.relevant_threshold             = 0.7;
    cfg.partial_threshold              = 0.4;
    cfg.retrieval_confidence_threshold = 1.0;

    SelfRAGController ctrl(cfg);

    // Always return the same two docs.
    ctrl.setRetrievalCallback([](const std::string&, size_t) {
        return std::vector<SelfRAGDocument>{
            {"dup-a", "text-a", 0.9},
            {"dup-b", "text-b", 0.9},
        };
    });

    auto result = ctrl.runRefinementLoop("query", 0.0);

    // Despite 3 rounds, each doc should appear only once.
    size_t total = result.relevant_docs.size() + result.partial_docs.size();
    EXPECT_EQ(total, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// SELF_RAG-12  reset() clears seen-ids for independent reuse
// ─────────────────────────────────────────────────────────────────────────────
TEST(SelfRAGControllerTest, SELF_RAG_12_ResetClearsState) {
    SelfRAGConfig cfg;
    cfg.max_rounds                     = 1;
    cfg.top_k                          = 1;
    cfg.target_relevant_docs           = 1;
    cfg.relevant_threshold             = 0.7;
    cfg.partial_threshold              = 0.4;
    cfg.retrieval_confidence_threshold = 1.0;

    SelfRAGController ctrl(cfg);
    ctrl.setRetrievalCallback([](const std::string&, size_t) {
        return std::vector<SelfRAGDocument>{{"doc1", "text", 0.9}};
    });

    auto r1 = ctrl.runRefinementLoop("q1", 0.0);
    EXPECT_EQ(r1.relevant_docs.size(), 1u);

    // Without reset, doc1 would be deduped away.
    ctrl.reset();

    auto r2 = ctrl.runRefinementLoop("q2", 0.0);
    EXPECT_EQ(r2.relevant_docs.size(), 1u); // doc1 visible again after reset
}
