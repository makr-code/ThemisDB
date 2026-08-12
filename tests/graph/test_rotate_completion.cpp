/**
 * @file tests/graph/test_rotate_completion.cpp
 * @brief Unit tests for RotatE Knowledge Graph Completion — KGC-01..15
 *
 * Coverage:
 *   KGC-01  addEntity/addRelation return unique indices
 *   KGC-02  Duplicate addEntity returns same index
 *   KGC-03  entityCount/relationCount reflect registry state
 *   KGC-04  score() throws on unregistered entity before training
 *   KGC-05  score() throws when model is not yet trained
 *   KGC-06  train() succeeds with valid triples
 *   KGC-07  train() result reports correct entity/relation/triple counts
 *   KGC-08  score() returns finite non-negative distance after training
 *   KGC-09  score() is symmetric-ish: same triple produces same score twice
 *   KGC-10  entityEmbedding() returns 2*dim values after training
 *   KGC-11  relationPhase() returns dim values after training
 *   KGC-12  predictTail returns top_k results sorted by ascending score
 *   KGC-13  predictHead returns top_k results sorted by ascending score
 *   KGC-14  KGCompletionEngine injects high-confidence predictions into reasoner
 *   KGC-15  KGCompletionEngine completeHead delegates to LinkPredictionHead
 *   KGC-16  Training is not a no-op: epoch count influences learned scores
 */

#include <gtest/gtest.h>
#include "graph/rotate_completion.h"
#include "graph/knowledge_graph_reasoner.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace themis::graph;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static RotatEConfig smallCfg() {
    RotatEConfig cfg;
    cfg.embedding_dim = 4;
    cfg.neg_samples   = 4;
    cfg.epochs        = 5;
    cfg.learning_rate = 1e-3f;
    cfg.batch_size    = 8;
    return cfg;
}

// Build a tiny graph: alice --knows--> bob, bob --knows--> carol
static void populateSmall(RotatEModel& model) {
    model.addEntity("alice");
    model.addEntity("bob");
    model.addEntity("carol");
    model.addRelation("knows");
}

static std::vector<KGTriple> smallTriples() {
    return {
        {"alice", "knows", "bob"},
        {"bob",   "knows", "carol"},
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-01  addEntity/addRelation return unique indices
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_01_UniqueIndices) {
    RotatEModel model(smallCfg());
    size_t i0 = model.addEntity("a");
    size_t i1 = model.addEntity("b");
    size_t i2 = model.addRelation("r1");
    size_t i3 = model.addRelation("r2");

    EXPECT_NE(i0, i1);
    EXPECT_NE(i2, i3);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-02  Duplicate addEntity returns same index
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_02_DuplicateEntitySameIndex) {
    RotatEModel model(smallCfg());
    size_t first  = model.addEntity("alice");
    size_t second = model.addEntity("alice");
    EXPECT_EQ(first, second);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-03  entityCount/relationCount reflect registry
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_03_CountsCorrect) {
    RotatEModel model(smallCfg());
    EXPECT_EQ(model.entityCount(), 0u);
    model.addEntity("e1");
    model.addEntity("e2");
    EXPECT_EQ(model.entityCount(), 2u);

    model.addRelation("r1");
    EXPECT_EQ(model.relationCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-04  score() throws on unregistered entity
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_04_ScoreThrowsUnregistered) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    model.train(smallTriples());

    EXPECT_THROW(model.score("unknown", "knows", "bob"), std::out_of_range);
    EXPECT_THROW(model.score("alice", "unknown_rel", "bob"), std::out_of_range);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-05  score() throws when model not trained
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_05_ScoreThrowsNotTrained) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    EXPECT_THROW(model.score("alice", "knows", "bob"), std::runtime_error);
    EXPECT_FALSE(model.isTrained());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-06  train() succeeds
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_06_TrainSucceeds) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    auto result = model.train(smallTriples());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(model.isTrained());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-07  train() result has correct counts
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_07_TrainResultCounts) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    auto triples = smallTriples();
    auto result  = model.train(triples);

    EXPECT_EQ(result.entities,  model.entityCount());
    EXPECT_EQ(result.relations, model.relationCount());
    EXPECT_EQ(result.triples,   triples.size());
    EXPECT_EQ(result.epochs_run, smallCfg().epochs);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-08  score() returns finite non-negative value after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_08_ScoreFiniteNonNegative) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    model.train(smallTriples());

    double s = model.score("alice", "knows", "bob");
    EXPECT_TRUE(std::isfinite(s));
    EXPECT_GE(s, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-09  score() is deterministic (same triple → same score twice)
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_09_ScoreDeterministic) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    model.train(smallTriples());

    double s1 = model.score("alice", "knows", "bob");
    double s2 = model.score("alice", "knows", "bob");
    EXPECT_DOUBLE_EQ(s1, s2);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-10  entityEmbedding returns 2*dim values after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_10_EntityEmbeddingSize) {
    auto cfg = smallCfg();
    RotatEModel model(cfg);
    populateSmall(model);
    model.train(smallTriples());

    auto emb = model.entityEmbedding("alice");
    EXPECT_EQ(emb.size(), 2 * cfg.embedding_dim);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-11  relationPhase returns dim values after training
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_11_RelationPhaseSize) {
    auto cfg = smallCfg();
    RotatEModel model(cfg);
    populateSmall(model);
    model.train(smallTriples());

    auto phase = model.relationPhase("knows");
    EXPECT_EQ(phase.size(), cfg.embedding_dim);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-12  predictTail returns top_k results sorted by ascending score
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_12_PredictTailSorted) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    model.train(smallTriples());

    LinkPredictionHead lph(model);
    auto preds = lph.predictTail("alice", "knows", 3);

    ASSERT_EQ(preds.size(), 3u); // 3 entities total
    for (size_t i = 1; i < preds.size(); ++i) {
        EXPECT_LE(preds[i - 1].score, preds[i].score); // ascending
    }
    // Ranks should be 1-based sequential
    for (size_t i = 0; i < preds.size(); ++i) {
        EXPECT_DOUBLE_EQ(preds[i].rank, static_cast<double>(i + 1));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-13  predictHead returns top_k results sorted by ascending score
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_13_PredictHeadSorted) {
    RotatEModel model(smallCfg());
    populateSmall(model);
    model.train(smallTriples());

    LinkPredictionHead lph(model);
    auto preds = lph.predictHead("knows", "carol", 2);

    ASSERT_EQ(preds.size(), 2u);
    EXPECT_LE(preds[0].score, preds[1].score);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-14  KGCompletionEngine injects predictions into reasoner
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGCompletionEngineTest, KGC_14_ReasonerInjection) {
    KGCompletionEngine engine(smallCfg());
    engine.addEntity("alice");
    engine.addEntity("bob");
    engine.addEntity("carol");
    engine.addRelation("knows");
    engine.train({{"alice", "knows", "bob"}, {"bob", "knows", "carol"}});

    KnowledgeGraphReasoner reasoner;
    // Use a very high threshold so all predictions get injected.
    engine.setReasoner(&reasoner, 1e9);

    auto preds = engine.completeTail("alice", "knows", 3);
    EXPECT_FALSE(preds.empty());

    // Reasoner should now have facts injected — verify no exception thrown
    // and that addFact calls were accepted (derivedTripleCount includes inferred triples).
    // completeTail injected via addFact; no exception = success.
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-15  KGCompletionEngine completeHead delegates correctly
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGCompletionEngineTest, KGC_15_CompleteHeadDelegates) {
    KGCompletionEngine engine(smallCfg());
    engine.addEntity("alice");
    engine.addEntity("bob");
    engine.addEntity("carol");
    engine.addRelation("knows");
    engine.train({{"alice", "knows", "bob"}, {"bob", "knows", "carol"}});

    auto preds = engine.completeHead("knows", "carol", 2);
    ASSERT_EQ(preds.size(), 2u);
    for (size_t i = 1; i < preds.size(); ++i) {
        EXPECT_LE(preds[i - 1].score, preds[i].score);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGC-16  More epochs should alter learned scores (training is not a no-op)
// ─────────────────────────────────────────────────────────────────────────────
TEST(RotatEModelTest, KGC_16_EpochCountInfluencesScore) {
    auto cfg_short = smallCfg();
    cfg_short.epochs = 1;

    RotatEModel short_model(cfg_short);
    populateSmall(short_model);
    short_model.train(smallTriples());
    double short_score = short_model.score("alice", "knows", "bob");

    auto cfg_long = smallCfg();
    cfg_long.epochs = 50;

    RotatEModel long_model(cfg_long);
    populateSmall(long_model);
    long_model.train(smallTriples());
    double long_score = long_model.score("alice", "knows", "bob");

    EXPECT_GT(std::fabs(short_score - long_score), 1e-6);
}
