/**
 * @file tests/test_rotate_completion.cpp
 * @brief Unit tests for RotatE Knowledge Graph Completion — KGC-01..15
 *
 * Coverage:
 *   KGC-01  Default RotatEModel is not trained and has zero entities/relations
 *   KGC-02  addEntity/addRelation register unique IDs; duplicates are ignored
 *   KGC-03  train() with no registered entities returns empty (no throw)
 *   KGC-04  train() succeeds with minimal entities/relations/triples
 *   KGC-05  train() result has correct entity, relation, and epoch counts
 *   KGC-06  score() throws when model is not trained
 *   KGC-07  score() does not throw after training for known (h, r, t)
 *   KGC-08  rankTail() throws when model is not trained
 *   KGC-09  rankTail() returns ascending-score list after training
 *   KGC-10  rankHead() returns ascending-score list after training
 *   KGC-11  top_k limits prediction count in rankTail/rankHead
 *   KGC-12  LinkPredictionHead::predictTail/predictHead delegate to model
 *   KGC-13  KGCompletionEngine::completeTail injects facts into wired reasoner
 *   KGC-14  KGCompletionEngine::completeHead works without a wired reasoner
 *   KGC-15  KGCompletionEngine::setReasoner threshold controls injection
 */

#include <gtest/gtest.h>
#include "graph/rotate_completion.h"
#include "graph/knowledge_graph_reasoner.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::graph;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a small config suitable for fast unit-test training (few epochs).
RotatEConfig smallCfg() {
    RotatEConfig cfg;
    cfg.embedding_dim = 8;
    cfg.neg_samples   = 4;
    cfg.epochs        = 10;
    cfg.learning_rate = 1e-2f;
    cfg.margin        = 2.0f;
    return cfg;
}

/// Populate a model with three entities and one relation.
void addBaseEntitiesAndRelation(RotatEModel& m) {
    m.addEntity("alice");
    m.addEntity("bob");
    m.addEntity("carol");
    m.addRelation("knows");
}

/// Build a minimal training triple set.
std::vector<KGTriple> baseTriples() {
    return {
        {"alice", "knows", "bob"},
        {"bob",   "knows", "carol"},
    };
}

/// Build and train a small KGCompletionEngine with three entities.
void trainEngine(KGCompletionEngine& eng) {
    eng.addEntity("alice");
    eng.addEntity("bob");
    eng.addEntity("carol");
    eng.addRelation("knows");
    eng.train(baseTriples());
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// KGC-01  Default RotatEModel is not trained and has zero entities/relations
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_01_DefaultModelNotTrained) {
    RotatEModel m;
    EXPECT_FALSE(m.isTrained());
    EXPECT_EQ(m.entityCount(),   0u);
    EXPECT_EQ(m.relationCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-02  addEntity/addRelation register unique IDs; duplicates are ignored
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_02_AddEntityRelationDeduplicates) {
    RotatEModel m;
    m.addEntity("a");
    m.addEntity("b");
    m.addEntity("a"); // duplicate
    EXPECT_EQ(m.entityCount(), 2u);

    m.addRelation("rel1");
    m.addRelation("rel1"); // duplicate
    EXPECT_EQ(m.relationCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-03  train() with no registered entities returns empty result (no throw)
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_03_TrainNoEntitiesReturnsEmpty) {
    RotatEModel m(smallCfg());
    // No entities or relations registered.
    std::vector<KGTriple> triples; // empty
    auto res = m.train(triples);
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.epochs_run, 0u);
    EXPECT_FALSE(m.isTrained());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-04  train() succeeds with minimal entities/relations/triples
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_04_TrainSucceedsMinimal) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);

    auto res = m.train(baseTriples());

    EXPECT_TRUE(res.success);
    EXPECT_TRUE(m.isTrained());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-05  train() result has correct entity, relation, and epoch counts
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_05_TrainResultCounts) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);

    auto res = m.train(baseTriples());

    EXPECT_EQ(res.entities,   3u);
    EXPECT_EQ(res.relations,  1u);
    EXPECT_EQ(res.triples,    baseTriples().size());
    EXPECT_EQ(res.epochs_run, smallCfg().epochs);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-06  score() throws when model is not trained
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_06_ScoreThrowsUntrained) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    EXPECT_THROW(m.score("alice", "knows", "bob"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-07  score() does not throw after training for known (h, r, t)
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_07_ScoreNoThrowAfterTraining) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    m.train(baseTriples());

    EXPECT_NO_THROW({
        double s = m.score("alice", "knows", "bob");
        EXPECT_GE(s, 0.0);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-08  rankTail() throws when model is not trained
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_08_RankTailThrowsUntrained) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    EXPECT_THROW(m.rankTail("alice", "knows", 3), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-09  rankTail() returns ascending-score list after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_09_RankTailSortedAscending) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    m.train(baseTriples());

    auto preds = m.rankTail("alice", "knows", 3);
    ASSERT_FALSE(preds.empty());

    for (size_t i = 1; i < preds.size(); ++i) {
        EXPECT_LE(preds[i - 1].score, preds[i].score)
            << "Predictions are not sorted ascending at index " << i;
    }

    // Ranks should be 1-based and increasing.
    for (size_t i = 0; i < preds.size(); ++i) {
        EXPECT_DOUBLE_EQ(preds[i].rank, static_cast<double>(i + 1));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-10  rankHead() returns ascending-score list after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_10_RankHeadSortedAscending) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    m.train(baseTriples());

    auto preds = m.rankHead("knows", "carol", 3);
    ASSERT_FALSE(preds.empty());

    for (size_t i = 1; i < preds.size(); ++i) {
        EXPECT_LE(preds[i - 1].score, preds[i].score)
            << "Predictions are not sorted ascending at index " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-11  top_k limits prediction count in rankTail/rankHead
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_11_TopKLimitsResults) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    m.train(baseTriples());

    // 3 entities registered; top_k=1 → only 1 result
    auto preds1 = m.rankTail("alice", "knows", 1);
    EXPECT_EQ(preds1.size(), 1u);

    // top_k > entity count → clamped to entity count
    auto preds_all = m.rankTail("alice", "knows", 100);
    EXPECT_EQ(preds_all.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-12  LinkPredictionHead::predictTail/predictHead delegate to model
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_12_LinkPredictionHeadDelegates) {
    RotatEModel m(smallCfg());
    addBaseEntitiesAndRelation(m);
    m.train(baseTriples());

    LinkPredictionHead head(m);

    auto tail_preds = head.predictTail("alice", "knows", 3);
    EXPECT_EQ(tail_preds.size(), 3u);

    auto head_preds = head.predictHead("knows", "carol", 2);
    EXPECT_EQ(head_preds.size(), 2u);

    // predictTail and rankTail should give same ordering
    auto direct = m.rankTail("alice", "knows", 3);
    ASSERT_EQ(tail_preds.size(), direct.size());
    for (size_t i = 0; i < tail_preds.size(); ++i) {
        EXPECT_EQ(tail_preds[i].entity, direct[i].entity);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-13  KGCompletionEngine::completeTail injects facts into wired reasoner
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_13_CompleteTailInjectsIntoReasoner) {
    KGCompletionEngine eng(smallCfg());
    trainEngine(eng);

    KnowledgeGraphReasoner reasoner;
    // Set threshold high so all predictions are injected.
    eng.setReasoner(&reasoner, 1e9);

    auto preds = eng.completeTail("alice", "knows", 3);
    EXPECT_FALSE(preds.empty());

    // factCount must be >= 1 (at least one fact injected)
    EXPECT_GE(reasoner.factCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-14  KGCompletionEngine::completeHead works without a wired reasoner
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_14_CompleteHeadNoReasoner) {
    KGCompletionEngine eng(smallCfg());
    trainEngine(eng);
    // No reasoner wired.
    EXPECT_NO_THROW({
        auto preds = eng.completeHead("knows", "carol", 2);
        EXPECT_EQ(preds.size(), 2u);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-15  KGCompletionEngine::setReasoner threshold controls injection
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotateCompletionTest, KGC_15_ThresholdControlsInjection) {
    // Use a threshold of 0.0 so no prediction is considered confident enough.
    KGCompletionEngine eng(smallCfg());
    trainEngine(eng);
    KnowledgeGraphReasoner reasoner;
    eng.setReasoner(&reasoner, 0.0);

    // completeTail should still return predictions …
    auto preds = eng.completeTail("alice", "knows", 3);
    EXPECT_FALSE(preds.empty());

    // … but nothing should be injected (all scores > 0.0 threshold).
    EXPECT_EQ(reasoner.factCount(), 0u);
}
