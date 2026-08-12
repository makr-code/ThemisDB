/**
 * @file test_tensor_storage_observer.cpp
 * @brief Unit tests for TensorNetworkStorageEngine CDC observer hooks.
 *
 * Test IDs:
 *   TNSE-OBS-01  write observer called after successful put()
 *   TNSE-OBS-02  delete observer called after successful remove()
 *   TNSE-OBS-03  graph stays in sync with storage when observers wired to fp-graph
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/tensor_network_storage_engine.h"
#include "storage/tensor_train_decomposer.h"
#include "graph/tensor_fingerprint_graph.h"

#include <atomic>
#include <memory>
#include <random>
#include <vector>

using namespace themis::storage;
using namespace themis::graph;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<float> randVec(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> d(0.0f, 1.0f);
    std::vector<float> v(n);
    for (auto& x : v) x = d(rng);
    return v;
}

static std::shared_ptr<TensorNetworkStorageEngine> makeEngine() {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    TensorStorageConfig cfg;
    cfg.tt_config.eps = 0.05;
    cfg.min_compression_ratio = 0.0;
    return std::make_shared<TensorNetworkStorageEngine>(backend, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// TNSE-OBS-01: write observer invoked after put()
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorStorageObserverTest, TNSE_OBS_01_WriteObserverCalledAfterPut) {
    auto engine = makeEngine();

    std::atomic<int> call_count{0};
    TensorFieldKey observed_key;
    std::size_t observed_core_count = 0;

    engine->setWriteObserverFn([&](const TensorFieldKey& key, const TTTrain& train) {
        ++call_count;
        observed_key = key;
        observed_core_count = train.cores.size();
    });

    auto data = randVec(8, 42);
    TensorFieldKey key{"tenant1", "col1", "field1"};
    ASSERT_TRUE(engine->put(key, data, {8, 1}));

    EXPECT_EQ(call_count.load(), 1);
    EXPECT_EQ(observed_key.tenant, "tenant1");
    EXPECT_EQ(observed_key.collection, "col1");
    EXPECT_EQ(observed_key.field, "field1");
    EXPECT_GT(observed_core_count, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TNSE-OBS-02: delete observer invoked after remove()
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorStorageObserverTest, TNSE_OBS_02_DeleteObserverCalledAfterRemove) {
    auto engine = makeEngine();

    std::atomic<int> delete_calls{0};
    TensorFieldKey deleted_key;

    engine->setDeleteObserverFn([&](const TensorFieldKey& key) {
        ++delete_calls;
        deleted_key = key;
    });

    TensorFieldKey key{"t", "c", "f_del"};
    auto data = randVec(8, 43);
    ASSERT_TRUE(engine->put(key, data, {8, 1}));

    EXPECT_TRUE(engine->remove(key));
    EXPECT_EQ(delete_calls.load(), 1);
    EXPECT_EQ(deleted_key.field, "f_del");
}

// ─────────────────────────────────────────────────────────────────────────────
// TNSE-OBS-03: graph stays in sync when observers are wired to fp-graph
//
// Uses key.field as tensor_id (deterministic from storage key).
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorStorageObserverTest, TNSE_OBS_03_GraphStaysInSyncViaObservers) {
    auto engine = makeEngine();

    FingerprintGraphConfig fp_cfg;
    fp_cfg.similarity_threshold = 0.90;
    fp_cfg.num_hash_funcs = 64;
    fp_cfg.num_bands = 16;
    auto graph = std::make_shared<TensorFingerprintGraph>(fp_cfg);

    // Wire write observer: use key.field as tensor_id.
    engine->setWriteObserverFn(
        [graph](const TensorFieldKey& key, const TTTrain& train) {
            graph->insert(key.field, train, key.tenant, key.collection, key.field);
        });

    // Wire delete observer: remove by key.field.
    engine->setDeleteObserverFn(
        [graph](const TensorFieldKey& key) {
            graph->remove(key.field);
        });

    // Put two tensors — graph should gain 2 nodes.
    auto data_a = randVec(8, 700);
    auto data_b = randVec(8, 701);
    ASSERT_TRUE(engine->put({"t", "c", "fa"}, data_a, {8, 1}));
    ASSERT_TRUE(engine->put({"t", "c", "fb"}, data_b, {8, 1}));

    EXPECT_EQ(graph->nodeCount(), 2u);

    // Remove one — graph should shrink to 1 node.
    ASSERT_TRUE(engine->remove({"t", "c", "fa"}));
    EXPECT_EQ(graph->nodeCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge case: observer NOT set — put/remove still succeed
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorStorageObserverTest, TNSE_OBS_NoObserverSetDoesNotCrash) {
    auto engine = makeEngine();

    TensorFieldKey key{"t", "c", "f"};
    auto data = randVec(8, 800);
    EXPECT_TRUE(engine->put(key, data, {8, 1}));
    EXPECT_TRUE(engine->remove(key));
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge case: throwing observer — exception swallowed; put() still returns true
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorStorageObserverTest, TNSE_OBS_ThrowingObserverDoesNotPropagateException) {
    auto engine = makeEngine();

    engine->setWriteObserverFn([](const TensorFieldKey&, const TTTrain&) {
        throw std::runtime_error("deliberate observer failure");
    });

    auto data = randVec(8, 801);
    EXPECT_NO_THROW({
        bool ok = engine->put({"t", "c", "fthrow"}, data, {8, 1});
        EXPECT_TRUE(ok);
    });
}

TEST(TensorStorageObserverTest, TNSE_OBS_ThrowingDeleteObserverDoesNotPropagateException) {
    auto engine = makeEngine();

    engine->setDeleteObserverFn([](const TensorFieldKey&) {
        throw std::runtime_error("deliberate delete observer failure");
    });

    auto data = randVec(8, 802);
    ASSERT_TRUE(engine->put({"t", "c", "fdelete_throw"}, data, {8, 1}));

    EXPECT_NO_THROW({
        bool ok = engine->remove({"t", "c", "fdelete_throw"});
        EXPECT_TRUE(ok);
    });
}
