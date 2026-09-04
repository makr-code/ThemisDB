// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rotate_completion_acceptance.cpp
 * @brief Wave B RotatE acceptance-gate coverage against a deterministic TransE baseline.
 *
 * Coverage:
 *   KGC-ACC-01  RotatE reaches the Wave B quality gates and is not worse than TransE.
 *   KGC-ACC-02  RotatE top-20 inference latency remains below the Wave B latency gate.
 *   KGC-ACC-03  KGCompletionEngine + KnowledgeGraphReasoner integration preserves public behavior.
 *
 * Notes:
 *   - Uses an in-repo deterministic FB15k-237-style fixture so the gate is reproducible in CI.
 *   - Compares against a lightweight local TransE baseline implemented in this test file.
 *   - Exercises the same public RotatEModel / KGCompletionEngine surfaces used by the AI wave tracker.
 */

#include <gtest/gtest.h>

#include "graph/knowledge_graph_reasoner.h"
#include "graph/rotate_completion.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace themis::graph;

namespace {

struct EvalMetrics {
    double mrr = 0.0;
    double hits_at_10 = 0.0;
};

RotatEConfig acceptanceCfg() {
    RotatEConfig cfg;
    cfg.embedding_dim = 16;
    cfg.neg_samples = 6;
    cfg.epochs = 40;
    cfg.learning_rate = 0.02f;
    cfg.margin = 3.0f;
    cfg.batch_size = 32;
    return cfg;
}

std::vector<KGTriple> acceptanceTrainingTriples() {
    return {
        {"alice", "mentor_of", "bob"},
        {"bob", "mentor_of", "carol"},
        {"dave", "mentor_of", "erin"},
        {"grace", "mentor_of", "heidi"},
        {"ivan", "mentor_of", "judy"},
        {"karl", "mentor_of", "lisa"},
        {"acme", "located_in", "berlin"},
        {"globex", "located_in", "paris"},
        {"initech", "located_in", "rome"},
        {"alice", "works_at", "acme"},
        {"dave", "works_at", "initech"},
        {"frank", "works_at", "globex"},
        {"alice", "lives_in", "berlin"},
        {"dave", "lives_in", "rome"},
        {"frank", "lives_in", "paris"},
    };
}

std::vector<KGTriple> acceptanceEvalTriples() {
    return {
        {"bob", "works_at", "acme"},
        {"carol", "works_at", "globex"},
        {"erin", "works_at", "initech"},
        {"bob", "lives_in", "berlin"},
        {"carol", "lives_in", "paris"},
        {"erin", "lives_in", "rome"},
    };
}

std::vector<std::string> acceptanceEntities() {
    return {
        "alice", "bob", "carol", "dave", "erin", "frank",
        "grace", "heidi", "ivan", "judy", "karl", "lisa",
        "acme", "globex", "initech",
        "berlin", "paris", "rome"
    };
}

std::vector<std::string> acceptanceRelations() {
    return {"mentor_of", "located_in", "works_at", "lives_in"};
}

class TransEBaselineModel {
public:
    explicit TransEBaselineModel(size_t embedding_dim)
        : embedding_dim_(embedding_dim) {}

    void addEntity(const std::string& id) {
        if (entity_index_.count(id) != 0) {
            return;
        }
        entity_index_[id] = entity_names_.size();
        entity_names_.push_back(id);
    }

    void addRelation(const std::string& id) {
        if (relation_index_.count(id) != 0) {
            return;
        }
        relation_index_[id] = relation_names_.size();
        relation_names_.push_back(id);
    }

    void train(const std::vector<KGTriple>& triples, size_t epochs, float learning_rate) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> init_dist(-0.2f, 0.2f);

        entity_embeddings_.assign(entity_names_.size() * embedding_dim_, 0.0f);
        relation_embeddings_.assign(relation_names_.size() * embedding_dim_, 0.0f);
        for (auto& value : entity_embeddings_) {
            value = init_dist(rng);
        }
        for (auto& value : relation_embeddings_) {
            value = init_dist(rng);
        }

        std::uniform_int_distribution<size_t> entity_dist(0, entity_names_.size() - 1);
        std::vector<size_t> order(triples.size());
        std::iota(order.begin(), order.end(), 0);

        for (size_t epoch = 0; epoch < epochs; ++epoch) {
            std::shuffle(order.begin(), order.end(), rng);
            for (size_t triple_index : order) {
                const auto& triple = triples[triple_index];
                const size_t h = entity_index_.at(triple.head);
                const size_t r = relation_index_.at(triple.relation);
                const size_t t = entity_index_.at(triple.tail);

                const size_t negative = entity_dist(rng);
                const bool corrupt_tail = (triple_index % 2 == 0);
                const size_t neg_h = corrupt_tail ? h : negative;
                const size_t neg_t = corrupt_tail ? negative : t;

                const double pos_score = scoreIndices(h, r, t);
                const double neg_score = scoreIndices(neg_h, r, neg_t);
                if (pos_score + 1.0 <= neg_score) {
                    continue;
                }

                for (size_t dim = 0; dim < embedding_dim_; ++dim) {
                    const size_t h_off = h * embedding_dim_ + dim;
                    const size_t t_off = t * embedding_dim_ + dim;
                    const size_t r_off = r * embedding_dim_ + dim;
                    const size_t nh_off = neg_h * embedding_dim_ + dim;
                    const size_t nt_off = neg_t * embedding_dim_ + dim;

                    const float pos_delta = entity_embeddings_[h_off] +
                        relation_embeddings_[r_off] - entity_embeddings_[t_off];
                    const float neg_delta = entity_embeddings_[nh_off] +
                        relation_embeddings_[r_off] - entity_embeddings_[nt_off];

                    const float pos_sign = (pos_delta >= 0.0f) ? 1.0f : -1.0f;
                    const float neg_sign = (neg_delta >= 0.0f) ? 1.0f : -1.0f;

                    entity_embeddings_[h_off] -= learning_rate * pos_sign;
                    relation_embeddings_[r_off] -= learning_rate * pos_sign;
                    entity_embeddings_[t_off] += learning_rate * pos_sign;

                    entity_embeddings_[nh_off] += learning_rate * neg_sign;
                    relation_embeddings_[r_off] += learning_rate * neg_sign;
                    entity_embeddings_[nt_off] -= learning_rate * neg_sign;
                }
            }
        }
    }

    std::vector<LinkPrediction> rankTail(const std::string& head,
                                         const std::string& relation,
                                         size_t top_k) const {
        const size_t head_index = entity_index_.at(head);
        const size_t relation_index = relation_index_.at(relation);
        std::vector<LinkPrediction> predictions = {};

        predictions.reserve(entity_names_.size());
        for (size_t entity_index = 0; entity_index < entity_names_.size(); ++entity_index) {
            predictions.push_back({
                entity_names_[entity_index],
                scoreIndices(head_index, relation_index, entity_index),
                0.0
            });
        }
        return finalizePredictions(std::move(predictions), top_k);
    }

    std::vector<LinkPrediction> rankHead(const std::string& relation,
                                         const std::string& tail,
                                         size_t top_k) const {
        const size_t relation_index = relation_index_.at(relation);
        const size_t tail_index = entity_index_.at(tail);
        std::vector<LinkPrediction> predictions = {};

        predictions.reserve(entity_names_.size());
        for (size_t entity_index = 0; entity_index < entity_names_.size(); ++entity_index) {
            predictions.push_back({
                entity_names_[entity_index],
                scoreIndices(entity_index, relation_index, tail_index),
                0.0
            });
        }
        return finalizePredictions(std::move(predictions), top_k);
    }

private:
    double scoreIndices(size_t head_index, size_t relation_index, size_t tail_index) const {
        double total = 0.0;
        for (size_t dim = 0; dim < embedding_dim_; ++dim) {
            const float delta =
                entity_embeddings_[head_index * embedding_dim_ + dim] +
                relation_embeddings_[relation_index * embedding_dim_ + dim] -
                entity_embeddings_[tail_index * embedding_dim_ + dim];
            total += std::abs(delta);
        }
        return total;
    }

    static std::vector<LinkPrediction> finalizePredictions(std::vector<LinkPrediction> predictions,
                                                           size_t top_k) {
        std::sort(predictions.begin(), predictions.end(),
                  [](const LinkPrediction& lhs, const LinkPrediction& rhs) {
                      return lhs.score < rhs.score;
                  });
        const size_t keep = std::min(top_k, predictions.size());
        predictions.resize(keep);
        for (size_t index = 0; index < predictions.size(); ++index) {
            predictions[index].rank = static_cast<double>(index + 1);
        }
        return predictions;
    }

    size_t embedding_dim_;
    std::unordered_map<std::string, size_t> entity_index_;
    std::unordered_map<std::string, size_t> relation_index_;
    std::vector<std::string> entity_names_;
    std::vector<std::string> relation_names_;
    std::vector<float> entity_embeddings_;
    std::vector<float> relation_embeddings_;
};

template <typename TailFn, typename HeadFn>
EvalMetrics evaluatePredictions(const std::vector<KGTriple>& triples,
                                TailFn&& rank_tail,
                                HeadFn&& rank_head) {
    double reciprocal_rank_sum = 0.0;
    size_t hits_at_10 = 0;
    size_t query_count = 0;

    auto accumulate_ranks = [&](const std::vector<LinkPrediction>& predictions,
                                const std::string& expected_entity) {
        auto it = std::find_if(predictions.begin(), predictions.end(),
                               [&](const LinkPrediction& prediction) {
                                   return prediction.entity == expected_entity;
                               });
        if (it == predictions.end()) {
            return;
        }
        reciprocal_rank_sum += 1.0 / it->rank;
        if (it->rank <= 10.0) {
            ++hits_at_10;
        }
        ++query_count;
    };

    for (const auto& triple : triples) {
        accumulate_ranks(rank_tail(triple.head, triple.relation), triple.tail);
        accumulate_ranks(rank_head(triple.relation, triple.tail), triple.head);
    }

    EvalMetrics metrics = {};
    if (query_count > 0) {
        metrics.mrr = reciprocal_rank_sum / static_cast<double>(query_count);
        metrics.hits_at_10 = static_cast<double>(hits_at_10) /
            static_cast<double>(query_count);
    }
    return metrics;
}

std::vector<double> latencySamplesMs(RotatEModel& model,
                                     const std::string& head,
                                     const std::string& relation,
                                     size_t iterations) {
    std::vector<double> samples;
    samples.reserve(iterations);
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        const auto start = std::chrono::steady_clock::now();
        auto predictions = model.rankTail(head, relation, 20);
        const auto end = std::chrono::steady_clock::now();
        EXPECT_FALSE(predictions.empty());
        samples.push_back(
            std::chrono::duration<double, std::milli>(end - start).count());
    }
    std::sort(samples.begin(), samples.end());
    return samples;
}

} // namespace

TEST(RotateCompletionAcceptanceTest, KGC_ACC_01_RotatEBeatsTransEAndMeetsQualityGates) {
    auto config = acceptanceCfg();
    const auto training_triples = acceptanceTrainingTriples();
    const auto eval_triples = acceptanceEvalTriples();
    const auto entities = acceptanceEntities();
    const auto relations = acceptanceRelations();

    RotatEModel rotate_model(config);
    for (const auto& entity : entities) {
        rotate_model.addEntity(entity);
    }
    for (const auto& relation : relations) {
        rotate_model.addRelation(relation);
    }
    const auto rotate_result = rotate_model.train(training_triples);
    ASSERT_TRUE(rotate_result.success);

    TransEBaselineModel transe_model(config.embedding_dim);
    for (const auto& entity : entities) {
        transe_model.addEntity(entity);
    }
    for (const auto& relation : relations) {
        transe_model.addRelation(relation);
    }
    transe_model.train(training_triples, config.epochs, config.learning_rate);

    const auto rotate_metrics = evaluatePredictions(
        eval_triples,
        [&](const std::string& head, const std::string& relation) {
            return rotate_model.rankTail(head, relation, entities.size());
        },
        [&](const std::string& relation, const std::string& tail) {
            return rotate_model.rankHead(relation, tail, entities.size());
        });
    const auto transe_metrics = evaluatePredictions(
        eval_triples,
        [&](const std::string& head, const std::string& relation) {
            return transe_model.rankTail(head, relation, entities.size());
        },
        [&](const std::string& relation, const std::string& tail) {
            return transe_model.rankHead(relation, tail, entities.size());
        });

    EXPECT_GE(rotate_metrics.mrr, 0.35);
    EXPECT_GE(rotate_metrics.hits_at_10, 0.55);
    EXPECT_GE(rotate_metrics.mrr + 1e-9, transe_metrics.mrr);
    EXPECT_GE(rotate_metrics.hits_at_10 + 1e-9, transe_metrics.hits_at_10);
}

TEST(RotateCompletionAcceptanceTest, KGC_ACC_02_Top20InferenceLatencyWithinGate) {
    auto config = acceptanceCfg();
    const auto triples = acceptanceTrainingTriples();

    RotatEModel model(config);
    for (const auto& entity : acceptanceEntities()) {
        model.addEntity(entity);
    }
    for (const auto& relation : acceptanceRelations()) {
        model.addRelation(relation);
    }
    ASSERT_TRUE(model.train(triples).success);

    const auto samples_ms = latencySamplesMs(model, "alice", "works_at", 200);
    ASSERT_FALSE(samples_ms.empty());

    const size_t p99_index = std::min(samples_ms.size() - 1,
        static_cast<size_t>(std::ceil(samples_ms.size() * 0.99)) - 1);
    const double p99_ms = samples_ms[p99_index];

    EXPECT_LT(p99_ms, 50.0);
}

TEST(RotateCompletionAcceptanceTest, KGC_ACC_03_KGCompletionEnginePreservesPublicBehavior) {
    KGCompletionEngine engine(acceptanceCfg());
    for (const auto& entity : acceptanceEntities()) {
        engine.addEntity(entity);
    }
    for (const auto& relation : acceptanceRelations()) {
        engine.addRelation(relation);
    }
    ASSERT_TRUE(engine.train(acceptanceTrainingTriples()).success);

    KnowledgeGraphReasoner reasoner;
    engine.setReasoner(&reasoner, 1000.0);

    auto tail_predictions = engine.completeTail("alice", "works_at", 20);
    auto head_predictions = engine.completeHead("lives_in", "rome", 20);

    EXPECT_FALSE(tail_predictions.empty());
    EXPECT_FALSE(head_predictions.empty());
    EXPECT_EQ(engine.model().entityCount(), acceptanceEntities().size());
    EXPECT_EQ(engine.model().relationCount(), acceptanceRelations().size());
    EXPECT_GE(reasoner.factCount(), 1u);
}
