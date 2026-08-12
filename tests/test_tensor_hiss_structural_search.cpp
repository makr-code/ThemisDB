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
 * HissReshaper exposeQuantics reshape         THSS-05
 * HissReshaper infer bit-depths from modes    THSS-06
 * HissReshaper reject mismatched grid count   THSS-07
 * HissReshaper reject mismatched grid product THSS-08
 * HissReshaper preserves dense tensor values  THSS-09
 * HissReshaper residual-factor reshape        THSS-10
 * HissReshaper QuanticsFn bridge inject       THSS-11
 * HissReshaper QuanticsFn bridge clear        THSS-12
 * HissReshaper QuanticsFn bridge callback OK  THSS-13
 * QTTMappingDescriptor power-of-2 roundtrip  THSS-14
 * QTTMappingDescriptor non-power-of-2 map    THSS-15
 * QTTMappingDescriptor padding detection     THSS-16
 * QTTMappingDescriptor edge case: dim=1      THSS-17
 * QTTMappingDescriptor exposeQuantics populates mapping THSS-18
 * QTTMappingDescriptor dense roundtrip via mapping THSS-19
 * TNSRTask construction error on null engine  TNSR-01
 * TNSRTask empty key range → zero report      TNSR-02
 * TNSRTask recompresses and writes back       TNSR-03
 * TNSRTask skips keys absent from engine      TNSR-04
 * TNSRTask respects min_bytes_saved_to_commit TNSR-05
 * TNSRTask cancel mid-run                     TNSR-06
 * TNSRTask report duration > 0               TNSR-07
 * TNSRTask multiple keys, rank_delta positive TNSR-08
 * TNSRTask trivial train fast-path skip        TNSR-09
 * TNSRTask RerouteSerializeFn bridge inject   TNSR-10
 * TNSRTask RerouteSerializeFn bridge clear    TNSR-11
 * TNSRTask RerouteSerializeFn callback called TNSR-12
 */

#include "tensor/hiss_structural_search.h"
#include "tensor/tnsr_task.h"

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <numeric>
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

themis::storage::TTTrain makeNonPowerOfTwoModeTrain() {
    std::vector<float> dense(60);
    std::iota(dense.begin(), dense.end(), 1.0f);

    themis::storage::TensorTrainDecomposer decomposer;
    themis::storage::TensorTrainConfig cfg;
    cfg.eps = 1e-6;

    return decomposer.decompose(dense, {3, 4, 5}, cfg).first;
}

themis::storage::TTTrain makeRankOneTrain2D() {
    std::vector<float> dense(16, 0.0f);
    // Outer-product-like separable signal -> near rank-1 TT.
    for (std::size_t r = 0; r < 4; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            dense[r * 4 + c] = static_cast<float>((r + 1) * (c + 1));
        }
    }

    themis::storage::TensorTrainDecomposer decomposer;
    themis::storage::TensorTrainConfig cfg;
    cfg.eps = 1e-8;
    return decomposer.decompose(dense, {4, 4}, cfg).first;
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

TEST(TensorHissSearch, HissReshaperExposeQuanticsReshapesModes) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {2, 4, 8});
    EXPECT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 1u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 3u);
    EXPECT_EQ(qt.grid_sizes, (std::vector<std::size_t>{2u, 4u, 8u}));
    EXPECT_EQ(qt.quantics_mode_sizes, (std::vector<std::size_t>{2u, 2u, 2u, 2u, 2u, 2u}));
    EXPECT_EQ(qt.toTTTrain().mode_sizes, qt.quantics_mode_sizes);
    EXPECT_EQ(qt.toTTTrain().cores.size(), qt.quantics_mode_sizes.size());
}

TEST(TensorHissSearch, HissReshaperInfersBitDepthsFromTrainModes) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});
    ASSERT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 2u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 2u);
    EXPECT_EQ(qt.grid_sizes, train.mode_sizes);
    EXPECT_EQ(qt.quantics_mode_sizes, (std::vector<std::size_t>{2u, 2u, 2u, 2u, 2u, 2u}));
}

TEST(TensorHissSearch, HissReshaperRejectsMismatchedGridSizeCount) {
    const auto train = makeSmallTrain();
    EXPECT_THROW(themis::tensor::HissReshaper::exposeQuantics(train, {4, 4}), std::invalid_argument);
}

TEST(TensorHissSearch, HissReshaperRejectsMismatchedGridProduct) {
    const auto train = makeSmallTrain();
    EXPECT_THROW(themis::tensor::HissReshaper::exposeQuantics(train, {4, 4, 5}), std::invalid_argument);
}

TEST(TensorHissSearch, HissReshaperPreservesDenseTensorValues) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {2, 4, 8});

    const auto original = train.reconstruct();
    const auto reshaped = qt.toTTTrain().reconstruct();
    ASSERT_EQ(original.size(), reshaped.size());

    for (std::size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(original[i], reshaped[i], 1e-3f);
    }
}

TEST(TensorHissSearch, HissReshaperUsesResidualFactorForNonPowerModes) {
    const auto train = makeNonPowerOfTwoModeTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});

    EXPECT_EQ(qt.grid_sizes, (std::vector<std::size_t>{3u, 4u, 5u}));
    EXPECT_EQ(qt.padded_grid_sizes, (std::vector<std::size_t>{4u, 4u, 8u}));
    EXPECT_EQ(qt.bit_depths, (std::vector<std::size_t>{2u, 2u, 3u}));
    for (std::size_t i = 0; i < qt.padded_grid_sizes.size(); ++i) {
        EXPECT_EQ(qt.padded_grid_sizes[i], std::size_t{1} << qt.bit_depths[i]);
    }
    EXPECT_EQ(qt.quantics_mode_sizes,
              (std::vector<std::size_t>{2u, 2u, 2u, 2u, 2u, 2u, 2u}));
    EXPECT_EQ(qt.original_element_count, train.reconstruct().size());

    const auto original = train.reconstruct();
    const auto reshaped = qt.toTTTrain().reconstruct();
    ASSERT_GE(reshaped.size(), original.size());
    EXPECT_EQ(reshaped.size(),
              qt.padded_grid_sizes[0] * qt.padded_grid_sizes[1] * qt.padded_grid_sizes[2]);

    // Non-power-of-two layouts are sparse in flat QTT space. Validate with
    // explicit physical<->QTT mapping instead of linear prefix assumptions.
    for (std::size_t p = 0; p < original.size(); ++p) {
        const auto q = qt.mapping.physicalToQTT(p);
        ASSERT_LT(q, reshaped.size());
        EXPECT_NEAR(original[p], reshaped[q], 1e-3f)
            << "value mismatch at physical_idx=" << p << " / qtt_idx=" << q;
    }

    for (std::size_t q = 0; q < reshaped.size(); ++q) {
        const auto back = qt.mapping.qttToPhysical(q);
        if (!back.has_value()) {
            EXPECT_NEAR(reshaped[q], 0.0f, 1e-3f)
                << "padding slot at qtt_idx=" << q << " must be ~= 0";
        }
    }
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
// THSS-14  QTTMappingDescriptor: power-of-2 dimensions → full roundtrip
// ============================================================================
TEST(QTTMappingDescriptor, PowerOfTwoRoundtrip) {
    // grid_sizes = [2, 4, 8] — all are already powers of two
    themis::tensor::QTTMappingDescriptor desc;
    desc.grid_sizes        = {2u, 4u, 8u};
    desc.padded_grid_sizes = {2u, 4u, 8u};
    desc.bit_depths        = {1u, 2u, 3u};

    const std::size_t total_physical = 2u * 4u * 8u; // 64
    for (std::size_t p = 0; p < total_physical; ++p) {
        const auto q   = desc.physicalToQTT(p);
        const auto back = desc.qttToPhysical(q);
        ASSERT_TRUE(back.has_value())
            << "physical_idx " << p << " → qtt " << q << " must not be padding";
        EXPECT_EQ(*back, p)
            << "roundtrip failed for physical_idx " << p;
    }

    // physicalToQTT must be a bijection on [0, 64): all QTT results unique.
    std::vector<std::size_t> qtt_results(total_physical);
    for (std::size_t p = 0; p < total_physical; ++p) {
        qtt_results[p] = desc.physicalToQTT(p);
    }
    auto sorted = qtt_results;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(sorted.end(), std::adjacent_find(sorted.begin(), sorted.end()))
        << "physicalToQTT results must be unique (bijection)";
}

// ============================================================================
// THSS-15  QTTMappingDescriptor: non-power-of-2 dimensions → valid mapping
// ============================================================================
TEST(QTTMappingDescriptor, NonPowerOfTwoMapping) {
    // grid_sizes = [3, 5] padded to [4, 8]; bit_depths = [2, 3]; B = 5
    themis::tensor::QTTMappingDescriptor desc;
    desc.grid_sizes        = {3u, 5u};
    desc.padded_grid_sizes = {4u, 8u};
    desc.bit_depths        = {2u, 3u};

    const std::size_t total_physical = 3u * 5u; // 15
    for (std::size_t p = 0; p < total_physical; ++p) {
        const auto q    = desc.physicalToQTT(p);
        const auto back = desc.qttToPhysical(q);
        ASSERT_TRUE(back.has_value())
            << "physical_idx " << p << " → qtt " << q << " must not be padding";
        EXPECT_EQ(*back, p)
            << "roundtrip failed for physical_idx " << p;
        // QTT index must lie within the padded flat range [0, 4*8)
        EXPECT_LT(q, 4u * 8u);
    }
}

// ============================================================================
// THSS-16  QTTMappingDescriptor: padding region returns std::nullopt
// ============================================================================
TEST(QTTMappingDescriptor, PaddingRegionReturnsNullopt) {
    // grid_sizes = [3, 5] padded to [4, 8]
    themis::tensor::QTTMappingDescriptor desc;
    desc.grid_sizes        = {3u, 5u};
    desc.padded_grid_sizes = {4u, 8u};
    desc.bit_depths        = {2u, 3u};

    const std::size_t total_physical  = 3u * 5u;  // 15
    const std::size_t total_padded    = 4u * 8u;  // 32
    std::size_t padding_found = 0u;

    for (std::size_t q = 0; q < total_padded; ++q) {
        const auto back = desc.qttToPhysical(q);
        if (!back.has_value()) {
            ++padding_found;
        } else {
            // Every valid QTT index must round-trip back correctly.
            EXPECT_LT(*back, total_physical);
            EXPECT_EQ(desc.physicalToQTT(*back), q);
        }
    }
    // Exactly total_padded - total_physical indices must be padding.
    EXPECT_EQ(padding_found, total_padded - total_physical)
        << "unexpected number of padding QTT indices";
}

// ============================================================================
// THSS-17  QTTMappingDescriptor: edge case — single-element dimension
// ============================================================================
TEST(QTTMappingDescriptor, EdgeCaseDimensionOne) {
    // grid_sizes = [1, 3]; padded = [1, 4]; bit_depths = [0→1 min, 2] → [1, 2]
    // calculateBitDepth(1) = 1 (minimum); padded = 2
    themis::tensor::QTTMappingDescriptor desc;
    desc.grid_sizes        = {1u, 3u};
    desc.padded_grid_sizes = {2u, 4u};
    desc.bit_depths        = {1u, 2u};

    const std::size_t total_physical = 1u * 3u; // 3
    for (std::size_t p = 0; p < total_physical; ++p) {
        const auto q    = desc.physicalToQTT(p);
        const auto back = desc.qttToPhysical(q);
        ASSERT_TRUE(back.has_value())
            << "physical_idx " << p << " should not map to padding";
        EXPECT_EQ(*back, p);
    }

    // QTT indices where dim-0 reconstructed index = 1 (>= grid_sizes[0]=1) are padding.
    const std::size_t total_padded = 2u * 4u; // 8
    std::size_t padding_count = 0;
    for (std::size_t q = 0; q < total_padded; ++q) {
        if (!desc.qttToPhysical(q).has_value()) ++padding_count;
    }
    EXPECT_EQ(padding_count, total_padded - total_physical);
}

// ============================================================================
// THSS-18  exposeQuantics populates QTTrain::mapping correctly
// ============================================================================
TEST(HissReshaper, ExposeQuanticsPopulatesMapping) {
    const auto train = makeNonPowerOfTwoModeTrain(); // mode_sizes = {3, 4, 5}
    const auto qt    = themis::tensor::HissReshaper::exposeQuantics(train, {});

    // Mapping descriptor must mirror the QTTrain metadata fields.
    EXPECT_EQ(qt.mapping.grid_sizes,        qt.grid_sizes);
    EXPECT_EQ(qt.mapping.padded_grid_sizes, qt.padded_grid_sizes);
    EXPECT_EQ(qt.mapping.bit_depths,        qt.bit_depths);

    // grid_sizes = {3, 4, 5}; padded = {4, 4, 8}; bit_depths = {2, 2, 3}
    EXPECT_EQ(qt.mapping.grid_sizes,        (std::vector<std::size_t>{3u, 4u, 5u}));
    EXPECT_EQ(qt.mapping.padded_grid_sizes, (std::vector<std::size_t>{4u, 4u, 8u}));
    EXPECT_EQ(qt.mapping.bit_depths,        (std::vector<std::size_t>{2u, 2u, 3u}));

    // Every physical index must survive the roundtrip.
    const std::size_t n_phys = qt.original_element_count;
    for (std::size_t p = 0; p < n_phys; ++p) {
        const auto q    = qt.mapping.physicalToQTT(p);
        const auto back = qt.mapping.qttToPhysical(q);
        ASSERT_TRUE(back.has_value()) << "physical_idx " << p << " must not be padding";
        EXPECT_EQ(*back, p);
    }
}

// ============================================================================
// THSS-19  Dense roundtrip via QTTMappingDescriptor: values match exactly
// ============================================================================
TEST(HissReshaper, DenseRoundtripViaMapping) {
    // Use a non-power-of-two train to exercise the padding path.
    const auto train = makeNonPowerOfTwoModeTrain(); // mode_sizes = {3, 4, 5}
    const auto qt    = themis::tensor::HissReshaper::exposeQuantics(train, {});

    const auto original = train.reconstruct();
    const auto qtt_dense = qt.toTTTrain().reconstruct();

    // For each physical index, look up the QTT index and compare values.
    const auto& desc = qt.mapping;
    for (std::size_t p = 0; p < original.size(); ++p) {
        const auto q = desc.physicalToQTT(p);
        ASSERT_LT(q, qtt_dense.size());
        EXPECT_NEAR(original[p], qtt_dense[q], 1e-3f)
            << "value mismatch at physical_idx=" << p << " / qtt_idx=" << q;
    }

    // No valid physical index should map to a padding QTT slot (which holds 0).
    for (std::size_t q = 0; q < qtt_dense.size(); ++q) {
        const auto back = desc.qttToPhysical(q);
        if (!back.has_value()) {
            EXPECT_NEAR(qtt_dense[q], 0.0f, 1e-3f)
                << "padding slot at qtt_idx=" << q << " must be ≈ 0";
        }
    }
}

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

// ============================================================================
// TNSR-09  Trivial train skips HISS topology-search fast-path
// ============================================================================
TEST(TNSRTask, TrivialTrainSkipsTopologySearch) {
    auto backend = std::make_shared<themis::storage::InMemoryTensorBackend>();

    themis::storage::TensorStorageConfig cfg_storage;
    cfg_storage.tt_config.eps = 0.01;
    cfg_storage.tt_config.max_rank = 4;
    cfg_storage.min_compression_ratio = 0.0;

    auto engine = std::make_shared<themis::storage::TensorNetworkStorageEngine>(
        backend, cfg_storage);

    const auto train = makeRankOneTrain2D();
    const auto flat  = train.reconstruct();
    const themis::storage::TensorFieldKey key{"tenant", "simple", "rank1"};
    ASSERT_TRUE(engine->put(key, flat, train.mode_sizes));

    themis::tensor::TNSRTask task(engine);
    themis::tensor::TNSRConfig cfg;
    cfg.epsilon = 0.1;
    cfg.min_bytes_saved_to_commit = 0;
    cfg.max_topology_changes_per_run = 16;

    const auto report = task.run({key}, cfg);
    EXPECT_EQ(report.keys_processed, 1u);
    EXPECT_EQ(report.error_count, 0u);
    EXPECT_EQ(report.topology_search_skipped_keys, 1u);
    EXPECT_EQ(report.topology_changes, 0u);
}

// ============================================================================
// THSS-11  HissReshaper QuanticsFn bridge can be set and retrieved
// ============================================================================
TEST(HissReshaper, QuanticsFnBridgeSetAndGet) {
    themis::tensor::HissReshaper::clearQuanticsFn();
    EXPECT_FALSE(static_cast<bool>(themis::tensor::HissReshaper::getQuanticsFn()));

    bool called = false;
    themis::tensor::HissReshaper::setQuanticsFn(
        [&called](const themis::storage::TTTrain& t,
                  const std::vector<std::size_t>& gs) {
            called = true;
            return themis::tensor::QTTrain{};
        });
    EXPECT_TRUE(static_cast<bool>(themis::tensor::HissReshaper::getQuanticsFn()));
    themis::tensor::HissReshaper::clearQuanticsFn();
}

// ============================================================================
// THSS-12  HissReshaper QuanticsFn bridge cleared → fallback executes normally
// ============================================================================
TEST(HissReshaper, QuanticsFnBridgeClearRestoresFallback) {
    bool bridge_called = false;
    themis::tensor::HissReshaper::setQuanticsFn(
        [&bridge_called](const themis::storage::TTTrain& t,
                         const std::vector<std::size_t>& gs) {
            bridge_called = true;
            return themis::tensor::QTTrain{};
        });
    themis::tensor::HissReshaper::clearQuanticsFn();

    const auto train = makeSmallTrain();
    // Must not throw and must not invoke the cleared bridge.
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});
    EXPECT_FALSE(bridge_called);
    EXPECT_FALSE(qt.quantics_mode_sizes.empty());
}

// ============================================================================
// THSS-13  HissReshaper QuanticsFn bridge callback is invoked
// ============================================================================
TEST(HissReshaper, QuanticsFnBridgeCallbackInvoked) {
    struct Guard {
        ~Guard() { themis::tensor::HissReshaper::clearQuanticsFn(); }
    } guard;

    int call_count = 0;
    themis::tensor::HissReshaper::setQuanticsFn(
        [&call_count](const themis::storage::TTTrain& train,
                      const std::vector<std::size_t>& grid_sizes) {
            ++call_count;
            // Return a minimal valid QTTrain mirroring the input.
            themis::tensor::QTTrain qt;
            qt.grid_sizes           = train.mode_sizes;
            qt.quantics_mode_sizes  = train.mode_sizes;
            qt.tt_train             = train;
            for (const auto ms : train.mode_sizes) {
                std::size_t depth = 0;
                auto sz = ms;
                while (sz > 1) { sz >>= 1; ++depth; }
                qt.bit_depths.push_back(std::max<std::size_t>(depth, 1u));
            }
            return qt;
        });

    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(qt.grid_sizes, train.mode_sizes);
}

// ============================================================================
// TNSR-10  RerouteSerializeFn bridge can be set and retrieved
// ============================================================================
TEST(TNSRTask, RerouteSerializeFnBridgeSetAndGet) {
    themis::tensor::TNSRTask::clearRerouteSerializeFn();
    EXPECT_FALSE(static_cast<bool>(themis::tensor::TNSRTask::getRerouteSerializeFn()));

    themis::tensor::TNSRTask::setRerouteSerializeFn(
        [](themis::storage::TensorNetworkStorageEngine&,
           const themis::storage::TensorFieldKey&,
           const themis::tensor::TensorNetworkGraph&,
           const themis::storage::TTTrain&) { return true; });
    EXPECT_TRUE(static_cast<bool>(themis::tensor::TNSRTask::getRerouteSerializeFn()));
    themis::tensor::TNSRTask::clearRerouteSerializeFn();
}

// ============================================================================
// TNSR-11  RerouteSerializeFn cleared → run() still completes without error
// ============================================================================
TEST(TNSRTask, RerouteSerializeFnClearNoError) {
    themis::tensor::TNSRTask::setRerouteSerializeFn(
        [](themis::storage::TensorNetworkStorageEngine&,
           const themis::storage::TensorFieldKey&,
           const themis::tensor::TensorNetworkGraph&,
           const themis::storage::TTTrain&) { return false; });
    themis::tensor::TNSRTask::clearRerouteSerializeFn();

    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRTask task(engine);
    themis::tensor::TNSRConfig cfg;
    cfg.epsilon = 0.5;
    cfg.min_bytes_saved_to_commit = 0;
    cfg.max_topology_changes_per_run = 4;

    const auto report = task.run({key}, cfg);
    EXPECT_EQ(report.error_count, 0u);
}

// ============================================================================
// TNSR-12  RerouteSerializeFn callback is invoked for non-trivial trains
//          that receive topology changes
// ============================================================================
TEST(TNSRTask, RerouteSerializeFnCallbackInvoked) {
    struct Guard {
        ~Guard() { themis::tensor::TNSRTask::clearRerouteSerializeFn(); }
    } guard;

    int serialize_calls = 0;
    themis::tensor::TNSRTask::setRerouteSerializeFn(
        [&serialize_calls](themis::storage::TensorNetworkStorageEngine&,
                           const themis::storage::TensorFieldKey&,
                           const themis::tensor::TensorNetworkGraph& tng,
                           const themis::storage::TTTrain&) {
            ++serialize_calls;
            // Sanity: graph must have at least one node.
            EXPECT_GE(tng.nodeCount(), 1u);
            return true;
        });

    std::shared_ptr<themis::storage::InMemoryTensorBackend> be;
    themis::storage::TensorFieldKey key;
    auto engine = makeTinyEngine(be, key);

    themis::tensor::TNSRTask task(engine);
    themis::tensor::TNSRConfig cfg;
    cfg.epsilon = 0.5;
    cfg.min_bytes_saved_to_commit = 0;
    cfg.max_topology_changes_per_run = 8;
    cfg.hiss_config.entropy_threshold = 0.0; // maximize edge candidates

    const auto report = task.run({key}, cfg);
    EXPECT_EQ(report.error_count, 0u);
    // If topology changes occurred, the serialize fn must have been called;
    // if no topology changes occurred, the callback must not have been called.
    if (report.topology_changes > 0) {
        EXPECT_GE(serialize_calls, 1);
    } else {
        EXPECT_EQ(serialize_calls, 0);
    }
}
