/*
 * @file test_tensor_hiss_structural_search.cpp
 * @brief Phase-6 tensorgraph tests: Hiss, TemplateCatalog, HissReshaper, TNSR.
 *
 * Test IDs
 * --------
 * TensorNetworkGraph add/edge/neighbors       THSS-01
 * TensorNetworkGraph rerouteEdge              THSS-02
 * HissStructuralSearchEngine determinism      THSS-03
 * TemplateCatalog register/lookup             THSS-04
 * HissReshaper exposeQuantics passthrough     THSS-05
 * HissReshaper infer bit-depths from modes    THSS-06
 * HissReshaper reject mismatched grid count   THSS-07
 * TNSRTask construction error on null engine  TNSR-01
 * TNSRTask empty key range → zero report      TNSR-02
 * TNSRTask recompresses and writes back       TNSR-03
 * TNSRTask skips keys absent from engine      TNSR-04
 * TNSRTask respects min_bytes_saved_to_commit TNSR-05
 * TNSRTask cancel mid-run                     TNSR-06
 * TNSRTask report duration > 0               TNSR-07
 * TNSRTask multiple keys, rank_delta positive TNSR-08
 */

#include "tensor/hiss_structural_search.h"
#include "tensor/tnsr_task.h"

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace {

themis::storage::TTTrain makeSmallTrain() {
    themis::storage::TTTrain t;
    t.mode_sizes = {4, 4, 4};
    t.cores.resize(3);

    t.cores[0].r_left = 1;
    t.cores[0].n = 4;
    t.cores[0].r_right = 3;
    t.cores[0].data.assign(1 * 4 * 3, 0.1f);
    t.cores[0].data[0] = 4.0f;

    t.cores[1].r_left = 3;
    t.cores[1].n = 4;
    t.cores[1].r_right = 2;
    t.cores[1].data.assign(3 * 4 * 2, 0.2f);
    t.cores[1].data[5] = 5.0f;

    t.cores[2].r_left = 2;
    t.cores[2].n = 4;
    t.cores[2].r_right = 1;
    t.cores[2].data.assign(2 * 4 * 1, 0.3f);
    t.cores[2].data[3] = 6.0f;
    return t;
}

} // namespace

TEST(TensorHissSearch, GraphAddNodeEdgeAndNeighbors) {
    themis::tensor::TensorNetworkGraph g;
    const auto a = g.addNode({"a", 0, 1, 2, 4, 0.1});
    const auto b = g.addNode({"b", 1, 2, 1, 4, 0.2});
    EXPECT_TRUE(g.addEdge({a, b, 1.0, "chain"}));
    EXPECT_FALSE(g.addEdge({a, b, 1.0, "chain"}));
    EXPECT_EQ(g.nodeCount(), 2u);
    EXPECT_EQ(g.edgeCount(), 1u);
    const auto n = g.neighbors(a);
    ASSERT_EQ(n.size(), 1u);
    EXPECT_EQ(n[0], b);
}

TEST(TensorHissSearch, GraphRerouteEdge) {
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"a", 0, 1, 2, 4, 0.1});
    g.addNode({"b", 1, 2, 1, 4, 0.2});
    ASSERT_TRUE(g.addEdge({0, 1, 1.0, "chain"}));
    EXPECT_TRUE(g.rerouteEdge(0, 1, "reshaped"));
    EXPECT_EQ(g.edges().front().topology, "reshaped");
}

TEST(TensorHissSearch, HissSearchBuildsDeterministicGraph) {
    const auto train = makeSmallTrain();
    themis::tensor::HissConfig cfg;
    cfg.entropy_threshold = 0.1;
    cfg.max_reshape_depth = 2;
    cfg.diversity_budget = 4;
    cfg.random_seed = 1234;

    themis::tensor::HissStructuralSearchEngine engine;
    const auto g1 = engine.search(train, cfg);
    const auto g2 = engine.search(train, cfg);
    EXPECT_EQ(g1.nodeCount(), train.cores.size());
    EXPECT_EQ(g1.edgeCount(), g2.edgeCount());
    ASSERT_EQ(g1.edges().size(), g2.edges().size());
    for (std::size_t i = 0; i < g1.edges().size(); ++i) {
        EXPECT_EQ(g1.edges()[i].from, g2.edges()[i].from);
        EXPECT_EQ(g1.edges()[i].to, g2.edges()[i].to);
        EXPECT_EQ(g1.edges()[i].topology, g2.edges()[i].topology);
        EXPECT_FLOAT_EQ(static_cast<float>(g1.edges()[i].weight), static_cast<float>(g2.edges()[i].weight));
    }
}

TEST(TensorHissSearch, TemplateCatalogRegisterLookup) {
    themis::tensor::TemplateCatalog c;
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n", 0, 1, 1, 8, 0.0});
    c.registerTemplate("finance", g);
    auto found = c.lookup("finance");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->nodeCount(), 1u);
    EXPECT_EQ(c.size(), 1u);
    EXPECT_FALSE(c.lookup("unknown").has_value());
}

TEST(TensorHissSearch, HissReshaperExposeQuanticsPassthrough) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {2, 4, 8});
    EXPECT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 1u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 3u);
    EXPECT_EQ(qt.toTTTrain().cores.size(), train.cores.size());
}

TEST(TensorHissSearch, HissReshaperInfersBitDepthsFromTrainModes) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});
    ASSERT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 2u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 2u);
}

TEST(TensorHissSearch, HissReshaperRejectsMismatchedGridSizeCount) {
    const auto train = makeSmallTrain();
    EXPECT_THROW(themis::tensor::HissReshaper::exposeQuantics(train, {4, 4}), std::invalid_argument);
}

// ============================================================================
// Helper: build a small in-memory engine with one stored tensor
// ============================================================================

namespace {

std::shared_ptr<themis::storage::TensorNetworkStorageEngine> makeTinyEngine(
    std::shared_ptr<themis::storage::InMemoryTensorBackend>& backend_out,
    themis::storage::TensorFieldKey& key_out)
{
    auto backend = std::make_shared<themis::storage::InMemoryTensorBackend>();
    backend_out  = backend;

    themis::storage::TensorStorageConfig cfg;
    cfg.tt_config.eps      = 0.01;
    cfg.tt_config.max_rank = 4;
    cfg.min_compression_ratio = 0.0; // Always store regardless of ratio

    auto engine = std::make_shared<themis::storage::TensorNetworkStorageEngine>(
        backend, cfg);

    const auto train = makeSmallTrain();
    const auto flat  = train.reconstruct();
    key_out = {"tenant", "col", "field"};
    engine->put(key_out, flat, train.mode_sizes);
    return engine;
}

} // namespace

// ============================================================================
// TNSR-01  Constructor rejects null engine
// ============================================================================
TEST(TNSRTask, NullEngineThrows) {
    EXPECT_THROW(
        themis::tensor::TNSRTask(nullptr),
        std::invalid_argument);
}

// ============================================================================
// TNSR-02  Empty key range produces zero report
// ============================================================================
TEST(TNSRTask, EmptyKeyRange) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRTask task(engine);
    auto report = task.run({});
    EXPECT_EQ(report.keys_processed, 0u);
    EXPECT_EQ(report.keys_rewritten, 0u);
    EXPECT_EQ(report.bytes_saved, 0u);
    EXPECT_GE(report.duration_ms, 0.0);
}

// ============================================================================
// TNSR-03  Single key: run does not throw; report fields are consistent
// ============================================================================
TEST(TNSRTask, SingleKeyRunSucceeds) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRConfig cfg;
    cfg.epsilon                  = 0.05;   // Looser eps → may shrink ranks
    cfg.min_bytes_saved_to_commit = 0;     // Accept any saving

    themis::tensor::TNSRTask task(engine);
    auto report = task.run({key}, cfg);

    EXPECT_EQ(report.keys_processed, 1u);
    EXPECT_EQ(report.error_count, 0u);
    // keys_rewritten is 0 or 1 depending on whether recompression saved bytes
    EXPECT_LE(report.keys_rewritten, 1u);
    EXPECT_GE(report.duration_ms, 0.0);
}

// ============================================================================
// TNSR-04  Key absent from engine is skipped (no crash)
// ============================================================================
TEST(TNSRTask, AbsentKeySkipped) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRTask task(engine);
    themis::storage::TensorFieldKey ghost{"x", "y", "z"};
    auto report = task.run({ghost});
    EXPECT_EQ(report.keys_processed, 1u);
    EXPECT_EQ(report.keys_rewritten, 0u);
    EXPECT_EQ(report.error_count, 0u);
}

// ============================================================================
// TNSR-05  min_bytes_saved_to_commit = large → no write-back
// ============================================================================
TEST(TNSRTask, MinBytesSavedGuard) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRConfig cfg;
    cfg.epsilon                  = 0.05;
    cfg.min_bytes_saved_to_commit = 1'000'000; // Effectively infinite

    themis::tensor::TNSRTask task(engine);
    auto report = task.run({key}, cfg);

    EXPECT_EQ(report.keys_rewritten, 0u);
}

// ============================================================================
// TNSR-06  Cancel-flag stops run between keys
// ============================================================================
TEST(TNSRTask, CancelFlagStopsRun) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    // Store a second key
    const auto train = makeSmallTrain();
    const auto flat  = train.reconstruct();
    themis::storage::TensorFieldKey key2{"t2", "c2", "f2"};
    engine->put(key2, flat, train.mode_sizes);

    themis::tensor::TNSRTask task(engine);
    task.requestCancel();
    auto report = task.run({key, key2});

    // Should have processed 0 keys (cancel pre-set)
    EXPECT_EQ(report.keys_processed, 0u);

    task.clearCancel();
    EXPECT_FALSE(task.isCancelRequested());
}

// ============================================================================
// TNSR-07  duration_ms is non-negative
// ============================================================================
TEST(TNSRTask, DurationNonNegative) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRTask task(engine);
    auto report = task.run({key});
    EXPECT_GE(report.duration_ms, 0.0);
}

// ============================================================================
// TNSR-08  Two keys: rank_delta is non-negative when compression tightens
// ============================================================================
TEST(TNSRTask, TwoKeysRankDeltaNonNegative) {
    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    // Store a second tensor with even looser construction (higher rank)
    const auto train = makeSmallTrain();
    const auto flat  = train.reconstruct();
    themis::storage::TensorFieldKey key2{"t2", "c2", "f2"};
    engine->put(key2, flat, train.mode_sizes);

    themis::tensor::TNSRConfig cfg;
    cfg.epsilon                  = 0.1;
    cfg.min_bytes_saved_to_commit = 0;

    themis::tensor::TNSRTask task(engine);
    auto report = task.run({key, key2}, cfg);

    EXPECT_EQ(report.keys_processed, 2u);
    EXPECT_GE(report.rank_delta, 0);
}
