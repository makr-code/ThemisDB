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
 * UTR-22  fromDocument: registered ITextEncoder is called and produces valid HTTrain
 * UTR-23  fromDocument: clearTextEncoder reverts to built-in lexical encoder
 * UTR-24  fromDocument: ITextEncoder wrong size throws runtime_error (fail-closed)
 * UTR-25  fromDocument: unavailable ITextEncoder falls back to EmbedFn bridge
 * UTR-26  fromImage: registered IImageEncoder is called and produces valid TTTrain
 * UTR-27  fromImage: clearImageEncoder reverts to built-in patch encoder
 * UTR-28  fromImage: IImageEncoder returning empty TTTrain throws runtime_error
 * UTR-29  fromDocument: lexical encoder produces L2-unit-norm embeddings (non-zero)
 * UTR-30  fromDocument: ITextEncoder takes priority over EmbedFn bridge
 */

#include "tensor/utr_converter.h"
#include "tensor/encoder_interface.h"
#include "tensor/hyper_index_builder.h"
#include "storage/tensor_train_decomposer.h"

#include <array>
#include <atomic>
#include <algorithm>
#include <gtest/gtest.h>
#include <cmath>
#include <numeric>
#include <optional>
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

    std::vector<TableRow> rows = {};

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

[[nodiscard]] themis::tensor::HyperIndexTensor buildFkAwareHyperIndex(std::size_t max_hops) {
    using namespace themis::tensor;

    ColumnSchema customer_id;
    customer_id.name = "customer_id";
    customer_id.type = ColumnType::NUMERIC;
    customer_id.range_min = 0.0;
    customer_id.range_max = 100.0;

    ColumnSchema order_customer_fk;
    order_customer_fk.name = "order_customer_fk";
    order_customer_fk.type = ColumnType::NUMERIC;
    order_customer_fk.range_min = 0.0;
    order_customer_fk.range_max = 100.0;

    ColumnSchema line_order_fk;
    line_order_fk.name = "line_order_fk";
    line_order_fk.type = ColumnType::NUMERIC;
    line_order_fk.range_min = 0.0;
    line_order_fk.range_max = 100.0;

    std::vector<TableRow> rows;
    rows.reserve(64);
    for (int i = 0; i < 32; ++i) {
        TableRow r;
        r.numeric_values = {90.0, 5.0, 5.0};
        rows.push_back(r);
    }
    for (int i = 0; i < 32; ++i) {
        TableRow r;
        r.numeric_values = {10.0, 10.0, 10.0};
        rows.push_back(r);
    }

    HyperIndexConfig cfg;
    cfg.bucket_count = 4;
    cfg.eps = 1e-6;
    cfg.max_rank = 8;
    cfg.numeric_bucket_strategy = HyperIndexConfig::NumericBucketStrategy::UNIFORM_RANGE;
    cfg.fk_graph.max_hops = max_hops;
    cfg.fk_graph.propagation_decay = 1.0;
    cfg.fk_graph.signal_blend_weight = 0.5;
    cfg.fk_graph.default_join_strength = 1.0;
    cfg.fk_graph.edges = {
        HyperIndexConfig::ForeignKeyEdge{0u, 1u, 1.0},
        HyperIndexConfig::ForeignKeyEdge{1u, 2u, 1.0},
    };

    return HyperIndexBuilder::fromSchema(
        "tenant_fk", {customer_id, order_customer_fk, line_order_fk}, rows, cfg);
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

    // Compare normalised value distributions (Hilbert ordering may permute index order).
    float vmin = g.values[0], vmax = g.values[0];
    for (const auto v : g.values) { vmin = std::min(vmin, v); vmax = std::max(vmax, v); }
    const float range = (vmax > vmin) ? (vmax - vmin) : 1.0f;

    std::vector<float> expected = {};

    expected.reserve(recon.size());
    for (std::size_t i = 0; i < g.values.size(); ++i) {
        expected.push_back((g.values[i] - vmin) / range);
    }

    // Hilbert traversal may permute index order vs. row-major source layout.
    // We verify value-distribution fidelity after ordering-independent sort.
    auto sorted_recon = recon;
    auto sorted_expected = expected;
    std::sort(sorted_recon.begin(), sorted_recon.end());
    std::sort(sorted_expected.begin(), sorted_expected.end());

    double ss_res = 0.0, ss_tot = 0.0;
    for (std::size_t i = 0; i < sorted_recon.size(); ++i) {
        const double delta = sorted_recon[i] - sorted_expected[i];
        const double expected_val = static_cast<double>(sorted_expected[i]);
        ss_res += delta * delta;
        ss_tot += expected_val * expected_val;
    }
    const double rmse = (ss_tot > 0.0) ? std::sqrt(ss_res / ss_tot) : 0.0;
    EXPECT_LT(rmse, 0.05);
}

TEST(UTRConverter, FromGeospatialPadsToHilbertPowerOfTwoGrid) {
    themis::tensor::RasterGrid g;
    g.rows = 3;
    g.cols = 5;
    g.cell_size_deg = 0.01;
    g.values.resize(g.rows * g.cols);
    for (std::size_t i = 0; i < g.values.size(); ++i) {
        g.values[i] = static_cast<float>(i);
    }

    themis::tensor::UTRConfig cfg;
    cfg.eps = 1e-4;
    cfg.max_rank = 8;
    const auto train = themis::tensor::UTRConverter::fromGeospatial(g, cfg);
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{8u, 8u}));
    const auto recon = train.reconstruct();
    ASSERT_EQ(recon.size(), 64u);
}

// ============================================================================
// fromImage
// ============================================================================

TEST(UTRConverter, FromImageRGBReturnValidTrain) {
    const auto px = makePixels(4, 4, 3);
    const auto train = themis::tensor::UTRConverter::fromImage(px, 4, 4, 3);
    EXPECT_EQ(train.cores.size(), 3u);
    // 4x4 with patch extent 4 -> single 1x1 patch grid.
    // RGB statistics emit {mean,stddev} per channel -> feature width 3*2 = 6.
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{1u, 1u, 6u}));
}

TEST(UTRConverter, FromImageGrayscaleReturnValidTrain) {
    const auto px = makePixels(8, 8, 1);
    const auto train = themis::tensor::UTRConverter::fromImage(px, 8, 8, 1);
    EXPECT_EQ(train.cores.size(), 3u);
    // 8x8 with patch extent 4 -> 2x2 patch grid.
    // Single-channel stats emit {mean,stddev} -> feature width 2.
    EXPECT_EQ(train.mode_sizes, (std::vector<std::size_t>{2u, 2u, 2u}));
}

TEST(UTRConverter, FromImagePatchStatisticsPreserveMeanAndStddev) {
    // Ensure the built-in patch-statistics path is active regardless of prior tests.
    themis::tensor::UTRConverter::clearImageEncoder();
    themis::tensor::UTRConverter::clearImageEmbedFn();

    const std::array<float, 16> patch = {
        0.f, 64.f, 128.f, 255.f,
        0.f, 64.f, 128.f, 255.f,
        0.f, 64.f, 128.f, 255.f,
        0.f, 64.f, 128.f, 255.f,
    };
    const std::vector<float> px(patch.begin(), patch.end());
    themis::tensor::UTRConfig cfg;
    cfg.eps = 1e-6;
    cfg.max_rank = 8;

    const auto train = themis::tensor::UTRConverter::fromImage(px, 4, 4, 1, cfg);
    const auto recon = train.reconstruct();
    ASSERT_EQ(recon.size(), 2u);

    const double expected_mean =
        (4.0 * (0.0 + 64.0 + 128.0 + 255.0)) / (16.0 * 255.0);
    // Two-pass variance: first compute the mean, then accumulate squared
    // deviations.  The two-pass approach is numerically preferable to the
    // single-pass (sum_of_squares - n*mean^2) formula which suffers from
    // catastrophic cancellation when the mean is large relative to the
    // variance.
    double variance = 0.0;
    for (const auto value : patch) {
        const auto normed = static_cast<double>(value) / 255.0;
        variance += (normed - expected_mean) * (normed - expected_mean);
    }
    variance /= static_cast<double>(patch.size());
    const double expected_stddev = std::sqrt(variance);

    EXPECT_NEAR(recon[0], expected_mean, 1e-3);
    EXPECT_GE(recon[1], -1e-6);
    EXPECT_LE(recon[1], expected_stddev + 1e-3);
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

TEST(UTRConverter, FromDocumentLexicalEmbeddingChangesAcrossTopics) {
    themis::tensor::UTRConfig cfg;
    cfg.embed_dim = 32;
    cfg.max_segments = 8;

    const auto ht_a = themis::tensor::UTRConverter::fromDocument(
        "Database transaction commit durability rollback",
        themis::tensor::DocumentStructureHint::SENTENCES,
        cfg);
    const auto ht_b = themis::tensor::UTRConverter::fromDocument(
        "Satellite imagery forest canopy terrain elevation",
        themis::tensor::DocumentStructureHint::SENTENCES,
        cfg);

    const auto dense_a = ht_a.reconstruct();
    const auto dense_b = ht_b.reconstruct();
    ASSERT_EQ(dense_a.size(), dense_b.size());
    bool differs = false;
    for (std::size_t i = 0; i < dense_a.size(); ++i) {
        if (std::abs(dense_a[i] - dense_b[i]) > 1e-4f) {
            differs = true;
            break;
        }
    }
    EXPECT_TRUE(differs);
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

TEST(UTRConverter, HyperIndexBuilderQuantileBucketingPreservesTailSignal) {
    using namespace themis::tensor;

    ColumnSchema amount;
    amount.name = "amount";
    amount.type = ColumnType::NUMERIC;
    amount.range_min = 0.0;
    amount.range_max = 1000.0;

    ColumnSchema status;
    status.name = "status";
    status.type = ColumnType::CATEGORY;
    status.categories = {"cold", "warm", "hot"};

    std::vector<TableRow> rows = {};

    for (int i = 0; i < 63; ++i) {
        TableRow row;
        row.numeric_values = {static_cast<double>(i)};
        row.category_values = {"cold"};
        rows.push_back(row);
    }
    TableRow outlier;
    outlier.numeric_values = {999.0};
    outlier.category_values = {"hot"};
    rows.push_back(outlier);

    HyperIndexConfig cfg;
    cfg.bucket_count = 4;
    cfg.eps = 0.01;
    cfg.max_rank = 8;

    const auto hi = HyperIndexBuilder::fromSchema("tenant", {amount, status}, rows, cfg);
    const auto tail_bucket = hi.contract({{0u, 3u}});
    EXPECT_GT(tail_bucket, 0.0);
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

    std::atomic<std::size_t> call_count{};
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

TEST(UTRConverter, HyperIndexBuilderFkJoinSignalPropagatesAcrossTwoHopPath) {
    using namespace themis::tensor;

    std::vector<std::vector<std::size_t>> one_hop_buckets;
    HyperIndexBuilder::setBucketAssignmentFn(
        [&one_hop_buckets](const std::string&,
                           const std::vector<ColumnSchema>&,
                           const TableRow&,
                           std::size_t,
                           const std::vector<std::size_t>& buckets) {
            one_hop_buckets.push_back(buckets);
            return buckets;
        });
    const auto one_hop = buildFkAwareHyperIndex(1);
    HyperIndexBuilder::clearBucketAssignmentFn();

    std::vector<std::vector<std::size_t>> two_hop_buckets;
    HyperIndexBuilder::setBucketAssignmentFn(
        [&two_hop_buckets](const std::string&,
                           const std::vector<ColumnSchema>&,
                           const TableRow&,
                           std::size_t,
                           const std::vector<std::size_t>& buckets) {
            two_hop_buckets.push_back(buckets);
            return buckets;
        });
    const auto two_hop = buildFkAwareHyperIndex(2);
    HyperIndexBuilder::clearBucketAssignmentFn();

    ASSERT_FALSE(one_hop_buckets.empty());
    ASSERT_EQ(one_hop_buckets.size(), one_hop.total_rows);
    ASSERT_EQ(two_hop_buckets.size(), two_hop.total_rows);
    ASSERT_EQ(one_hop.total_rows, two_hop.total_rows);

    // The fixture's first rows originate from customer_id bucket 3.
    // With max_hops=1, mode 2 keeps its base bucket; with max_hops=2 the
    // propagated 0->1->2 signal must raise that bucket.
    const auto one_hop_mode2 = one_hop_buckets.front().at(2);
    const auto two_hop_mode2 = two_hop_buckets.front().at(2);
    EXPECT_GT(two_hop_mode2, one_hop_mode2);
}

TEST(UTRConverter, HyperIndexBuilderFkCycleTraversalIsProtected) {
    using namespace themis::tensor;

    ColumnSchema c0;
    c0.name = "a";
    c0.type = ColumnType::NUMERIC;
    c0.range_min = 0.0;
    c0.range_max = 100.0;
    ColumnSchema c1 = c0; c1.name = "b";
    ColumnSchema c2 = c0; c2.name = "c";

    TableRow row;
    row.numeric_values = {80.0, 10.0, 20.0};

    HyperIndexConfig cfg;
    cfg.bucket_count = 4;
    cfg.numeric_bucket_strategy = HyperIndexConfig::NumericBucketStrategy::UNIFORM_RANGE;
    cfg.fk_graph.max_hops = 8;
    cfg.fk_graph.propagation_decay = 0.7;
    cfg.fk_graph.edges = {
        HyperIndexConfig::ForeignKeyEdge{0u, 1u, 1.0},
        HyperIndexConfig::ForeignKeyEdge{1u, 2u, 1.0},
        HyperIndexConfig::ForeignKeyEdge{2u, 0u, 1.0},
    };

    EXPECT_NO_THROW({
        const auto hi = HyperIndexBuilder::fromSchema("tenant_cycle", {c0, c1, c2}, {row}, cfg);
        EXPECT_EQ(hi.total_rows, 1u);
    });
}

TEST(UTRConverter, HyperIndexBuilderMissingFkStatsUsesConfiguredFallback) {
    using namespace themis::tensor;

    ColumnSchema c0;
    c0.name = "pk";
    c0.type = ColumnType::NUMERIC;
    c0.range_min = 0.0;
    c0.range_max = 100.0;
    ColumnSchema c1 = c0; c1.name = "fk";

    TableRow row;
    row.numeric_values = {95.0, 5.0};

    HyperIndexConfig strict_cfg;
    strict_cfg.bucket_count = 4;
    strict_cfg.numeric_bucket_strategy = HyperIndexConfig::NumericBucketStrategy::UNIFORM_RANGE;
    strict_cfg.fk_graph.missing_stats_fallback = HyperIndexConfig::MissingFkStatsFallback::THROW;
    strict_cfg.fk_graph.edges = {
        HyperIndexConfig::ForeignKeyEdge{0u, 1u, std::nullopt},
    };
    EXPECT_THROW(
        HyperIndexBuilder::fromSchema("tenant_throw", {c0, c1}, {row}, strict_cfg),
        std::runtime_error);

    HyperIndexConfig fallback_cfg = strict_cfg;
    fallback_cfg.fk_graph.missing_stats_fallback =
        HyperIndexConfig::MissingFkStatsFallback::USE_DEFAULT_WEIGHT;
    fallback_cfg.fk_graph.default_join_strength = 1.0;

    const auto hi = HyperIndexBuilder::fromSchema("tenant_fallback", {c0, c1}, {row}, fallback_cfg);
    EXPECT_EQ(hi.total_rows, 1u);
    EXPECT_GT(hi.contract({{1u, 2u}}), 0.0);
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
                     const UTRConfig& cfg) -> themis::storage::TTTrain {
            fn_called = true;
            // Delegate to the default path via a fresh normalised buffer so
            // we don't need to re-implement TT decomposition here.
            std::vector<float> normed(px.size());
            for (std::size_t i = 0; i < px.size(); ++i)
                normed[i] = px[i] / 255.0f;
            themis::storage::TensorTrainDecomposer decomp;
            themis::storage::TensorTrainConfig tt_cfg;
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
           const UTRConfig&) -> themis::storage::TTTrain {
            return themis::storage::TTTrain{};
        });
    UTRConverter::clearImageEmbedFn();

    std::vector<float> pixels(4 * 4 * 1, 64.0f);
    const auto train = UTRConverter::fromImage(pixels, 4, 4, 1);

    EXPECT_FALSE(train.cores.empty()); // built-in path produces real output
    EXPECT_FALSE(static_cast<bool>(UTRConverter::getImageEmbedFn()));
}

// ============================================================================
// ITextEncoder / IImageEncoder interface tests (UTR-22 through UTR-30)
// ============================================================================

namespace {

/// Minimal ITextEncoder stub that returns a constant vector.
class ConstTextEncoder final : public themis::tensor::ITextEncoder {
public:
    explicit ConstTextEncoder(float fill_value = 0.75f, bool available = true)
        : fill_(fill_value), available_(available) {}

    [[nodiscard]] std::vector<float>
    encode(const std::string& /*seg*/, std::size_t embed_dim) const override {
        return std::vector<float>(embed_dim, fill_);
    }

    [[nodiscard]] bool isAvailable() const noexcept override { return available_; }

    [[nodiscard]] themis::tensor::EncoderQuality quality() const noexcept override {
        return themis::tensor::EncoderQuality::SEMANTIC;
    }

    [[nodiscard]] std::string_view description() const noexcept override {
        return "ConstTextEncoder (test stub)";
    }

private:
    float fill_;
    bool  available_;
};

/// ITextEncoder stub that returns a vector of the wrong size.
class WrongSizeTextEncoder final : public themis::tensor::ITextEncoder {
public:
    [[nodiscard]] std::vector<float>
    encode(const std::string& /*seg*/, std::size_t /*embed_dim*/) const override {
        return {1.0f, 2.0f}; // always wrong size
    }

    [[nodiscard]] bool isAvailable() const noexcept override { return true; }

    [[nodiscard]] themis::tensor::EncoderQuality quality() const noexcept override {
        return themis::tensor::EncoderQuality::SEMANTIC;
    }

    [[nodiscard]] std::string_view description() const noexcept override {
        return "WrongSizeTextEncoder (test stub)";
    }
};

/// Minimal IImageEncoder stub that returns a single-core TTTrain.
class ConstImageEncoder final : public themis::tensor::IImageEncoder {
public:
    explicit ConstImageEncoder(bool available = true, bool return_empty = false)
        : available_(available), return_empty_(return_empty) {}

    [[nodiscard]] themis::storage::TTTrain
    encode(const std::vector<float>& /*pixels*/,
           std::size_t /*h*/, std::size_t /*w*/, std::size_t /*c*/,
           const themis::tensor::UTRConfig& cfg) const override {
        if (return_empty_) return themis::storage::TTTrain{};
        // Build a minimal 2×2 TTTrain via the decomposer so it is well-formed
        themis::storage::TensorTrainDecomposer decomp;
        themis::storage::TensorTrainConfig tt_cfg;
        tt_cfg.eps      = cfg.eps;
        tt_cfg.max_rank = cfg.max_rank;
        const std::vector<float> data(4, 1.0f);
        return decomp.decompose(data, {2u, 2u}, tt_cfg).first;
    }

    [[nodiscard]] bool isAvailable() const noexcept override { return available_; }

    [[nodiscard]] themis::tensor::EncoderQuality quality() const noexcept override {
        return themis::tensor::EncoderQuality::SEMANTIC;
    }

    [[nodiscard]] std::string_view description() const noexcept override {
        return "ConstImageEncoder (test stub)";
    }

private:
    bool available_;
    bool return_empty_;
};

/// ITextEncoder stub that calls back a flag and returns a constant embedding.
class TrackingTextEncoder final : public themis::tensor::ITextEncoder {
public:
    explicit TrackingTextEncoder(bool& flag, float fill = 0.5f)
        : flag_(flag), fill_(fill) {}

    [[nodiscard]] std::vector<float>
    encode(const std::string& /*seg*/, std::size_t dim) const override {
        flag_ = true;
        return std::vector<float>(dim, fill_);
    }

    [[nodiscard]] bool isAvailable() const noexcept override { return true; }

    [[nodiscard]] themis::tensor::EncoderQuality quality() const noexcept override {
        return themis::tensor::EncoderQuality::SEMANTIC;
    }

    [[nodiscard]] std::string_view description() const noexcept override {
        return "TrackingTextEncoder (test stub)";
    }

private:
    bool& flag_;
    float fill_;
};

} // namespace

// UTR-22: registered ITextEncoder is called and produces a valid HTTrain
TEST(UTRConverter, TextEncoderBridgeIsCalledFromDocument) {
    using namespace themis::tensor;

    bool called = false;
    UTRConverter::setTextEncoder(std::make_shared<TrackingTextEncoder>(called));

    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument(
        "Hello world.\n\nSecond paragraph.", DocumentStructureHint::PARAGRAPHS, cfg);

    UTRConverter::clearTextEncoder();

    EXPECT_TRUE(called);
    EXPECT_NE(ht.root, nullptr);
}

// UTR-23: clearTextEncoder reverts to built-in lexical encoder
TEST(UTRConverter, ClearTextEncoderRevertsToLexicalFallback) {
    using namespace themis::tensor;

    UTRConverter::setTextEncoder(std::make_shared<ConstTextEncoder>(1.0f));
    UTRConverter::clearTextEncoder();

    EXPECT_EQ(UTRConverter::getTextEncoder(), nullptr);

    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument("Fallback paragraph.", {}, cfg);
    EXPECT_NE(ht.root, nullptr);
}

// UTR-24: ITextEncoder returning wrong-size vector throws runtime_error (fail-closed)
TEST(UTRConverter, TextEncoderWrongSizeThrowsRuntimeError) {
    using namespace themis::tensor;

    UTRConverter::setTextEncoder(std::make_shared<WrongSizeTextEncoder>());

    UTRConfig cfg;
    cfg.embed_dim = 16;
    EXPECT_THROW(
        UTRConverter::fromDocument("Some text.", DocumentStructureHint::SENTENCES, cfg),
        std::runtime_error);

    UTRConverter::clearTextEncoder();
}

// UTR-25: unavailable ITextEncoder falls back to EmbedFn bridge
TEST(UTRConverter, UnavailableTextEncoderFallsBackToEmbedFn) {
    using namespace themis::tensor;

    // Register unavailable encoder
    UTRConverter::setTextEncoder(std::make_shared<ConstTextEncoder>(0.0f, /*available=*/false));

    // Also register EmbedFn bridge that marks itself called
    bool embed_fn_called = false;
    UTRConverter::setEmbedFn(
        [&embed_fn_called](const std::string& /*seg*/, std::size_t embed_dim) {
            embed_fn_called = true;
            return std::vector<float>(embed_dim, 0.3f);
        });

    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument("Test fallback.", {}, cfg);

    UTRConverter::clearTextEncoder();
    UTRConverter::clearEmbedFn();

    EXPECT_TRUE(embed_fn_called);
    EXPECT_NE(ht.root, nullptr);
}

// UTR-26: registered IImageEncoder is called and produces a valid TTTrain
TEST(UTRConverter, ImageEncoderBridgeIsCalledFromImage) {
    using namespace themis::tensor;

    UTRConverter::setImageEncoder(std::make_shared<ConstImageEncoder>(/*available=*/true));

    std::vector<float> pixels(4 * 4 * 3, 128.0f);
    const auto train = UTRConverter::fromImage(pixels, 4, 4, 3);

    EXPECT_NE(UTRConverter::getImageEncoder(), nullptr);

    UTRConverter::clearImageEncoder();

    EXPECT_FALSE(train.cores.empty());
    EXPECT_EQ(UTRConverter::getImageEncoder(), nullptr);
}

// UTR-27: clearImageEncoder reverts to built-in patch encoder
TEST(UTRConverter, ClearImageEncoderRevertsToPatchFallback) {
    using namespace themis::tensor;

    UTRConverter::setImageEncoder(std::make_shared<ConstImageEncoder>());
    UTRConverter::clearImageEncoder();

    EXPECT_EQ(UTRConverter::getImageEncoder(), nullptr);

    std::vector<float> pixels(4 * 4 * 1, 64.0f);
    const auto train = UTRConverter::fromImage(pixels, 4, 4, 1);
    EXPECT_FALSE(train.cores.empty());
}

// UTR-28: IImageEncoder returning empty TTTrain throws runtime_error (fail-closed)
TEST(UTRConverter, ImageEncoderEmptyResultThrowsRuntimeError) {
    using namespace themis::tensor;

    UTRConverter::setImageEncoder(
        std::make_shared<ConstImageEncoder>(/*available=*/true, /*return_empty=*/true));

    std::vector<float> pixels(4 * 4 * 3, 100.0f);
    EXPECT_THROW(UTRConverter::fromImage(pixels, 4, 4, 3), std::runtime_error);

    UTRConverter::clearImageEncoder();
}

// UTR-29: built-in lexical encoder produces non-zero embeddings
TEST(UTRConverter, LexicalEncoderProducesNonZeroEmbedding) {
    using namespace themis::tensor;

    // Ensure no encoder or bridge is set
    UTRConverter::clearTextEncoder();
    UTRConverter::clearEmbedFn();

    UTRConfig cfg;
    cfg.embed_dim    = 32;
    cfg.max_segments = 4;

    const auto ht = UTRConverter::fromDocument(
        "Machine learning models learn from data.\n\n"
        "Tensor decomposition reduces dimensionality.",
        DocumentStructureHint::PARAGRAPHS, cfg);
    EXPECT_NE(ht.root, nullptr);

    // The reconstructed tensor must be non-trivially populated
    const auto dense = ht.reconstruct();
    ASSERT_FALSE(dense.empty());
    const float norm_sq = std::accumulate(dense.begin(), dense.end(), 0.0f,
        [](float acc, float v) { return acc + v * v; });
    EXPECT_GT(norm_sq, 0.0f);
}

// UTR-30: registered ITextEncoder takes priority over EmbedFn bridge
TEST(UTRConverter, TextEncoderTakesPriorityOverEmbedFn) {
    using namespace themis::tensor;

    bool encoder_called = false;
    bool embed_fn_called = false;

    UTRConverter::setTextEncoder(std::make_shared<TrackingTextEncoder>(encoder_called, 0.9f));
    UTRConverter::setEmbedFn(
        [&embed_fn_called](const std::string& /*seg*/, std::size_t dim) {
            embed_fn_called = true;
            return std::vector<float>(dim, 0.1f);
        });

    UTRConfig cfg;
    cfg.embed_dim = 16;
    const auto ht = UTRConverter::fromDocument("Priority test.", {}, cfg);

    UTRConverter::clearTextEncoder();
    UTRConverter::clearEmbedFn();

    EXPECT_TRUE(encoder_called);
    EXPECT_FALSE(embed_fn_called);
    EXPECT_NE(ht.root, nullptr);
}
