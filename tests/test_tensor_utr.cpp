/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_tensor_utr.cpp                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 7 (Q3–Q4 2028)                      ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_tensor_utr.cpp
 * @brief Phase-7 Unified Tensor Representation (UTR) tests.
 *
 * Test IDs
 * --------
 * UTR-01  fromGeospatial: result is a valid TTTrain (non-empty cores)
 * UTR-02  fromGeospatial: rejects empty grid
 * UTR-03  fromGeospatial: rejects zero cell_size_deg
 * UTR-04  fromGeospatial: rejects mismatched values size
 * UTR-05  fromGeospatial: reconstruction round-trip RMSE < 5%
 * UTR-06  fromImage: result is valid TTTrain for RGB image
 * UTR-07  fromImage: result is valid TTTrain for grayscale image
 * UTR-08  fromImage: rejects zero dimension
 * UTR-09  fromImage: rejects mismatched pixels size
 * UTR-10  fromDocument: result is valid HTTrain (non-null root)
 * UTR-11  fromDocument: rejects empty text
 * UTR-12  fromDocument: paragraph split vs. sentence split produces different shapes
 * UTR-13  fromTabular: result is valid HyperIndexTensor
 * UTR-14  fromTabular: rejects schema with < 2 columns
 * UTR-15  fromTabular: rejects empty rows
 * UTR-16  HyperIndexTensor::contract: pinned contraction returns positive count
 * UTR-17  fromDocument: injected EmbedFn is called; result still valid
 * UTR-18  fromDocument: clearEmbedFn reverts to FNV-1a fallback
 * UTR-19  fromDocument: EmbedFn returning wrong size throws runtime_error
 * UTR-20  fromImage: injected ImageEmbedFn is called; result still valid
 * UTR-21  fromImage: clearImageEmbedFn reverts to raw-pixel fallback
 */

#include "tensor/utr_converter.h"
#include "tensor/hyper_index_builder.h"
#include "storage/tensor_train_decomposer.h"

#include <atomic>
#include <gtest/gtest.h>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

// ============================================================================
// Helpers
// ============================================================================

namespace {

themis::tensor::RasterGrid makeGrid4x4() {
    themis::tensor::RasterGrid g;
    g.rows         = 4;
    g.cols         = 4;
    g.lat_min      = 48.0;
    g.lon_min       = 11.0;
    g.cell_size_deg = 0.01;
    g.values.resize(16);
    for (std::size_t i = 0; i < 16; ++i) {
        g.values[i] = static_cast<float>(i % 4) * 0.25f;
    }
    return g;
}

std::vector<float> makePixels(std::size_t h, std::size_t w, std::size_t c) {
    std::vector<float> px(h * w * c);
    for (std::size_t i = 0; i < px.size(); ++i) {
        px[i] = static_cast<float>(i % 256);
    }
    return px;
}

themis::tensor::HyperIndexTensor buildSimpleHyperIndex() {
    using namespace themis::tensor;

    ColumnSchema c0;
    c0.name      = "age";
    c0.type      = ColumnType::NUMERIC;
    c0.range_min = 0.0;
    c0.range_max = 100.0;

    ColumnSchema c1;
    c1.name = "income";
    c1.type = ColumnType::NUMERIC;
    c1.range_min = 0.0;
    c1.range_max = 200000.0;

    const std::vector<ColumnSchema> schema = {c0, c1};

    std::vector<TableRow> rows;
    for (int i = 0; i < 50; ++i) {
        TableRow r;
        r.numeric_values = {20.0 + (i % 60), 30000.0 + (i % 8) * 20000.0};
        rows.push_back(r);
    }

    HyperIndexConfig cfg;
    cfg.bucket_count = 4;
    cfg.eps          = 0.05;

    return HyperIndexBuilder::fromSchema("tenant_a", schema, rows, cfg);
}

} // namespace

// ============================================================================
// fromGeospatial
// ============================================================================

TEST(UTRConverter, FromGeospatialReturnValidTrain) {
    const auto train = themis::tensor::UTRConverter::fromGeospatial(makeGrid4x4());
    EXPECT_EQ(train.cores.size(), 2u);
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{4u, 4u}));
}

TEST(UTRConverter, FromGeospatialRejectsEmptyGrid) {
    themis::tensor::RasterGrid g;
    g.rows = 0; g.cols = 4;
    g.cell_size_deg = 0.01;
    EXPECT_THROW(themis::tensor::UTRConverter::fromGeospatial(g),
                 std::invalid_argument);
}

TEST(UTRConverter, FromGeospatialRejectsZeroCellSize) {
    auto g = makeGrid4x4();
    g.cell_size_deg = 0.0;
    EXPECT_THROW(themis::tensor::UTRConverter::fromGeospatial(g),
                 std::invalid_argument);
}

TEST(UTRConverter, FromGeospatialRejectsMismatchedValues) {
    auto g = makeGrid4x4();
    g.values.resize(3);
    EXPECT_THROW(themis::tensor::UTRConverter::fromGeospatial(g),
                 std::invalid_argument);
}

TEST(UTRConverter, FromGeospatialRoundTripRMSE) {
    auto g = makeGrid4x4();
    // Use low eps so reconstruction is accurate
    themis::tensor::UTRConfig cfg;
    cfg.eps      = 0.001;
    cfg.max_rank = 4;

    const auto train = themis::tensor::UTRConverter::fromGeospatial(g, cfg);
    const auto recon = train.reconstruct();
    ASSERT_EQ(recon.size(), g.values.size());

    // Compute normalised RMSE against normalised input
    float vmin = g.values[0], vmax = g.values[0];
    for (const auto v : g.values) { vmin = std::min(vmin, v); vmax = std::max(vmax, v); }
    const float range = (vmax > vmin) ? (vmax - vmin) : 1.0f;

    double ss_res = 0.0, ss_tot = 0.0;
    for (std::size_t i = 0; i < recon.size(); ++i) {
        const double normed = (g.values[i] - vmin) / range;
        ss_res += (recon[i] - normed) * (recon[i] - normed);
        ss_tot += normed * normed;
    }
    const double rmse = (ss_tot > 0.0) ? std::sqrt(ss_res / ss_tot) : 0.0;
    EXPECT_LT(rmse, 0.05);
}

// ============================================================================
// fromImage
// ============================================================================

TEST(UTRConverter, FromImageRGBReturnValidTrain) {
    const auto px = makePixels(4, 4, 3);
    const auto train = themis::tensor::UTRConverter::fromImage(px, 4, 4, 3);
    EXPECT_EQ(train.cores.size(), 3u);
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{4u, 4u, 3u}));
}

TEST(UTRConverter, FromImageGrayscaleReturnValidTrain) {
    const auto px = makePixels(8, 8, 1);
    const auto train = themis::tensor::UTRConverter::fromImage(px, 8, 8, 1);
    EXPECT_EQ(train.cores.size(), 2u);
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{8u, 8u}));
}

TEST(UTRConverter, FromImageRejectsZeroDimension) {
    EXPECT_THROW(themis::tensor::UTRConverter::fromImage({}, 0, 4, 3),
                 std::invalid_argument);
    EXPECT_THROW(themis::tensor::UTRConverter::fromImage({}, 4, 0, 3),
                 std::invalid_argument);
    EXPECT_THROW(themis::tensor::UTRConverter::fromImage({}, 4, 4, 0),
                 std::invalid_argument);
}

TEST(UTRConverter, FromImageRejectsMismatchedPixelCount) {
    const std::vector<float> px(3, 0.0f);
    EXPECT_THROW(themis::tensor::UTRConverter::fromImage(px, 4, 4, 3),
                 std::invalid_argument);
}

// ============================================================================
// fromDocument
// ============================================================================

TEST(UTRConverter, FromDocumentReturnValidHTTrain) {
    const std::string doc =
        "First paragraph covers topic A.\n\nSecond paragraph discusses B.\n\n"
        "Third paragraph concludes with C.";
    themis::tensor::UTRConfig cfg;
    cfg.embed_dim    = 16;
    cfg.max_segments = 8;
    const auto ht = themis::tensor::UTRConverter::fromDocument(
        doc, themis::tensor::DocumentStructureHint::PARAGRAPHS, cfg);
    EXPECT_NE(ht.root, nullptr);
    EXPECT_GE(ht.shape.size(), 2u);
}

TEST(UTRConverter, FromDocumentRejectsEmptyText) {
    EXPECT_THROW(themis::tensor::UTRConverter::fromDocument(""),
                 std::invalid_argument);
}

TEST(UTRConverter, FromDocumentParagraphVsSentenceProduceDifferentShapes) {
    const std::string doc =
        "This is sentence one. This is sentence two. And sentence three.\n\n"
        "Second paragraph. More content here.";

    themis::tensor::UTRConfig cfg;
    cfg.embed_dim    = 16;
    cfg.max_segments = 16;

    const auto ht_para = themis::tensor::UTRConverter::fromDocument(
        doc, themis::tensor::DocumentStructureHint::PARAGRAPHS, cfg);
    const auto ht_sent = themis::tensor::UTRConverter::fromDocument(
        doc, themis::tensor::DocumentStructureHint::SENTENCES, cfg);

    EXPECT_NE(ht_para.root, nullptr);
    EXPECT_NE(ht_sent.root, nullptr);
    // Sentence mode produces more segments → different shape[0]
    EXPECT_NE(ht_para.shape[0], ht_sent.shape[0]);
}

// ============================================================================
// fromTabular / HyperIndexBuilder
// ============================================================================

TEST(UTRConverter, FromTabularReturnValidHyperIndex) {
    const auto hi = buildSimpleHyperIndex();
    EXPECT_EQ(hi.tenant_id, "tenant_a");
    EXPECT_FALSE(hi.tt_train.cores.empty());
    EXPECT_EQ(hi.schema.size(), 2u);
    EXPECT_EQ(hi.total_rows, 50u);
}

TEST(UTRConverter, FromTabularRejectsSchemaWithOneColumn) {
    using namespace themis::tensor;
    ColumnSchema c0; c0.name = "x"; c0.type = ColumnType::NUMERIC;
    c0.range_min = 0.0; c0.range_max = 1.0;
    TableRow r; r.numeric_values = {0.5};
    EXPECT_THROW(
        HyperIndexBuilder::fromSchema("t", {c0}, {r}),
        std::invalid_argument);
}

TEST(UTRConverter, FromTabularRejectsEmptyRows) {
    using namespace themis::tensor;
    ColumnSchema c0; c0.name = "a"; c0.type = ColumnType::NUMERIC;
    c0.range_min = 0.0; c0.range_max = 1.0;
    ColumnSchema c1 = c0; c1.name = "b";
    EXPECT_THROW(
        HyperIndexBuilder::fromSchema("t", {c0, c1}, {}),
        std::invalid_argument);
}

TEST(UTRConverter, HyperIndexTensorContractReturnPositive) {
    const auto hi = buildSimpleHyperIndex();
    // Contract over no pinned modes → sum of all entries = total_rows
    const auto total = hi.contract({});
    EXPECT_GT(total, 0.0);
    // Rough check: sum should be within an order of magnitude of total_rows
    EXPECT_GT(total, hi.total_rows * 0.1);
    EXPECT_LT(total, hi.total_rows * 10.0);
}

TEST(UTRConverter, HyperIndexTensorContractPinnedModeReturnsSubset) {
    const auto hi = buildSimpleHyperIndex();
    const auto full    = hi.contract({});
    const auto pinned  = hi.contract({{0u, 0u}});  // pin dimension-0 to bucket 0
    EXPECT_GE(full, pinned);
    EXPECT_GE(pinned, 0.0);
}

TEST(UTRConverter, HyperIndexBuilderBucketAssignmentBridgeAccessor) {
    using namespace themis::tensor;

    HyperIndexBuilder::clearBucketAssignmentFn();
    EXPECT_FALSE(static_cast<bool>(HyperIndexBuilder::getBucketAssignmentFn()));

    HyperIndexBuilder::setBucketAssignmentFn(
        [](const std::string&,
           const std::vector<ColumnSchema>&,
           const TableRow&,
           std::size_t,
           const std::vector<std::size_t>& buckets) {
            return buckets;
        });
    EXPECT_TRUE(static_cast<bool>(HyperIndexBuilder::getBucketAssignmentFn()));

    HyperIndexBuilder::clearBucketAssignmentFn();
    EXPECT_FALSE(static_cast<bool>(HyperIndexBuilder::getBucketAssignmentFn()));
}

TEST(UTRConverter, HyperIndexBuilderBucketAssignmentBridgeIsInvoked) {
    using namespace themis::tensor;

    std::atomic<std::size_t> call_count{0};
    HyperIndexBuilder::setBucketAssignmentFn(
        [&call_count](const std::string& tenant_id,
                      const std::vector<ColumnSchema>& schema,
                      const TableRow&,
                      std::size_t,
                      const std::vector<std::size_t>& buckets) {
            call_count.fetch_add(1, std::memory_order_relaxed);
            EXPECT_EQ(tenant_id, "tenant_a");
            EXPECT_EQ(schema.size(), 2u);
            return buckets;
        });

    const auto hi = buildSimpleHyperIndex();
    HyperIndexBuilder::clearBucketAssignmentFn();

    EXPECT_EQ(hi.total_rows, 50u);
    EXPECT_EQ(call_count.load(std::memory_order_relaxed), hi.total_rows);
}

TEST(UTRConverter, HyperIndexBuilderBucketAssignmentBridgeRejectsInvalidSize) {
    using namespace themis::tensor;

    HyperIndexBuilder::setBucketAssignmentFn(
        [](const std::string&,
           const std::vector<ColumnSchema>&,
           const TableRow&,
           std::size_t,
           const std::vector<std::size_t>&) {
            return std::vector<std::size_t>{0u};
        });

    EXPECT_THROW(buildSimpleHyperIndex(), std::runtime_error);
    HyperIndexBuilder::clearBucketAssignmentFn();
}

// ============================================================================
// STUB #257 — EmbedFn bridge tests
// ============================================================================

// UTR-17: injected EmbedFn is called and fromDocument still produces a valid HTTrain
TEST(UTRConverter, EmbedFnBridgeIsCalledFromDocument) {
    using namespace themis::tensor;

    bool fn_called = false;
    UTRConverter::setEmbedFn(
        [&fn_called](const std::string& /*seg*/, std::size_t embed_dim) {
            fn_called = true;
            return std::vector<float>(embed_dim, 0.5f); // uniform embedding
        });

    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument(
        "Hello world.\n\nSecond paragraph.", DocumentStructureHint::PARAGRAPHS, cfg);

    UTRConverter::clearEmbedFn();

    EXPECT_TRUE(fn_called);
    EXPECT_NE(ht.root, nullptr);
}

// UTR-18: clearEmbedFn reverts to FNV-1a fallback (both produce valid HTTrain)
TEST(UTRConverter, ClearEmbedFnRevertsToFnv1aFallback) {
    using namespace themis::tensor;

    UTRConverter::setEmbedFn(
        [](const std::string& /*seg*/, std::size_t embed_dim) {
            return std::vector<float>(embed_dim, 1.0f);
        });
    UTRConverter::clearEmbedFn();

    // After clearing, the built-in fallback must still work
    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument("Fallback paragraph.", {}, cfg);
    EXPECT_NE(ht.root, nullptr);

    // Bridge slot must be empty
    EXPECT_FALSE(static_cast<bool>(UTRConverter::getEmbedFn()));
}

// UTR-19: EmbedFn returning wrong-size vector throws runtime_error
TEST(UTRConverter, EmbedFnWrongSizeThrows) {
    using namespace themis::tensor;

    UTRConverter::setEmbedFn(
        [](const std::string& /*seg*/, std::size_t /*embed_dim*/) {
            return std::vector<float>{1.0f, 2.0f}; // always wrong size
        });

    UTRConfig cfg;
    cfg.embed_dim = 16;
    EXPECT_THROW(
        UTRConverter::fromDocument("Some text.", {}, cfg),
        std::runtime_error);

    UTRConverter::clearEmbedFn();
}

// ============================================================================
// STUB #258 — ImageEmbedFn bridge tests
// ============================================================================

// UTR-20: injected ImageEmbedFn is called and fromImage returns its result
TEST(UTRConverter, ImageEmbedFnBridgeIsCalledFromImage) {
    using namespace themis::tensor;

    bool fn_called = false;
    UTRConverter::setImageEmbedFn(
        [&fn_called](const std::vector<float>& px,
                     std::size_t h, std::size_t w, std::size_t c,
                     const UTRConfig& cfg) -> storage::TTTrain {
            fn_called = true;
            // Delegate to the default path via a fresh normalised buffer so
            // we don't need to re-implement TT decomposition here.
            std::vector<float> normed(px.size());
            for (std::size_t i = 0; i < px.size(); ++i)
                normed[i] = px[i] / 255.0f;
            storage::TensorTrainDecomposer decomp;
            storage::TensorTrainConfig tt_cfg;
            tt_cfg.eps      = cfg.eps;
            tt_cfg.max_rank = cfg.max_rank;
            std::vector<std::size_t> shape = (c == 1) ? std::vector<std::size_t>{h, w}
                                                        : std::vector<std::size_t>{h, w, c};
            return decomp.decompose(normed, shape, tt_cfg).first;
        });

    std::vector<float> pixels(4 * 4 * 3, 128.0f);
    const auto train = UTRConverter::fromImage(pixels, 4, 4, 3);

    UTRConverter::clearImageEmbedFn();

    EXPECT_TRUE(fn_called);
    EXPECT_FALSE(train.cores.empty());
}

// UTR-21: clearImageEmbedFn reverts to raw-pixel fallback
TEST(UTRConverter, ClearImageEmbedFnRevertsToRawPixelFallback) {
    using namespace themis::tensor;

    UTRConverter::setImageEmbedFn(
        [](const std::vector<float>&, std::size_t, std::size_t, std::size_t,
           const UTRConfig&) -> storage::TTTrain {
            return storage::TTTrain{};
        });
    UTRConverter::clearImageEmbedFn();

    std::vector<float> pixels(4 * 4 * 1, 64.0f);
    const auto train = UTRConverter::fromImage(pixels, 4, 4, 1);

    EXPECT_FALSE(train.cores.empty()); // built-in path produces real output
    EXPECT_FALSE(static_cast<bool>(UTRConverter::getImageEmbedFn()));
}
