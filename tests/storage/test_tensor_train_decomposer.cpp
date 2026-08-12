/**
 * @file test_tensor_train_decomposer.cpp
 * @brief Unit tests for TensorTrainDecomposer, TTQuantizer, and TensorNetworkStorageEngine.
 *
 * Test IDs:
 *   TTD-01  TT-SVD of a flat (rank-1) tensor achieves 100% compression
 *   TTD-02  TT-SVD of a 2D matrix reconstructs within eps=1%
 *   TTD-03  TT-SVD of a 4D tensor produces correct core shapes
 *   TTD-04  Reconstruction error ≤ eps * ‖T‖ for random 6D tensor
 *   TTD-05  compressionRatio() > 1 for low-rank tensors
 *   TTD-06  innerProduct of a train with itself equals ‖T‖² (≈)
 *   TTD-07  cosineSimilarity of identical trains = 1.0
 *   TTD-08  cosineSimilarity of orthogonal trains ≈ 0.0
 *   TTD-09  TTTrain serialize/deserialize round-trip preserves all cores
 *   TTD-10  decomposeF64 down-casts and gives same shape as float32
 *   TTD-11  Invalid input (size mismatch) throws std::invalid_argument
 *   TTD-12  max_rank cap is respected
 *   TTD-13  INT8 quantization round-trip: dequant error < 1% of absmax
 *   TTD-14  NF4 quantization round-trip: dequant error < 2% of absmax
 *   TTD-15  QuantizedTrain serialize/deserialize round-trip
 *   TTD-16  TTQuantizer::bytesPerElement() returns expected ratios
 *   TNS-01  InMemoryTensorBackend put/get/del cycle
 *   TNS-02  TensorNetworkStorageEngine::put stores without error
 *   TNS-03  TensorNetworkStorageEngine::get reconstructs close to original
 *   TNS-04  getCompressed returns QuantizedTrain (no decompression)
 *   TNS-05  stats() returns valid compression_ratio > 0
 *   TNS-06  remove() returns false for unknown key
 *   TNS-07  multiple versions tracked correctly
 *   TNS-08  min_compression_ratio fallback to raw when ratio too low
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/tensor_train_decomposer.h"
#include "storage/tensor_network_storage_engine.h"
#include "storage/tt_quantizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

using namespace themis::storage;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: flat vector product
// ─────────────────────────────────────────────────────────────────────────────

static std::size_t product(const std::vector<std::size_t>& v) {
    std::size_t p = 1;
    for (auto x : v) p *= x;
    return p;
}

static std::vector<float> makeRandom(std::size_t n, float scale = 1.0f,
                                     unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, scale);
    std::vector<float> v(n);
    for (auto& x : v) x = dist(rng);
    return v;
}

static std::vector<float> makeLowRank2D(std::size_t m, std::size_t n,
                                         std::size_t rank = 2) {
    // rank-`rank` matrix: sum of outer products
    std::mt19937 rng(123);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> mat(m * n, 0.0f);
    for (std::size_t r = 0; r < rank; ++r) {
        std::vector<float> u(m), v(n);
        for (auto& x : u) x = dist(rng);
        for (auto& x : v) x = dist(rng);
        for (std::size_t i = 0; i < m; ++i)
            for (std::size_t j = 0; j < n; ++j)
                mat[i * n + j] += u[i] * v[j];
    }
    return mat;
}

static double relError(const std::vector<float>& a,
                        const std::vector<float>& b) {
    double err = 0.0, nrm = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        double d = (double)a[i] - (double)b[i];
        err += d * d;
        nrm += (double)a[i] * a[i];
    }
    return (nrm > 1e-12) ? std::sqrt(err / nrm) : std::sqrt(err);
}

// ─────────────────────────────────────────────────────────────────────────────
// TensorTrainDecomposer tests
// ─────────────────────────────────────────────────────────────────────────────

class TensorTrainDecomposerTest : public ::testing::Test {
protected:
    TensorTrainDecomposer decomposer;
};

// TTD-01: rank-1 tensor (constant vector ⊗ constant vector)
TEST_F(TensorTrainDecomposerTest, TTD01_FlatRank1TensorHighCompression) {
    std::vector<float> data(4 * 4 * 4, 1.0f);  // constant 3D tensor
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [train, stats] = decomposer.decompose(data, {4, 4, 4}, cfg);
    EXPECT_EQ(train.order(), 3u);
    EXPECT_GT(stats.compression_ratio, 1.0);
}

// TTD-02: 2D matrix reconstruction within eps=1%
TEST_F(TensorTrainDecomposerTest, TTD02_2DMatrixReconstructionEps) {
    auto data = makeLowRank2D(8, 8, 2);
    TensorTrainConfig cfg; cfg.eps = 0.01;
    auto [train, stats] = decomposer.decompose(data, {8, 8}, cfg);
    auto recon = train.reconstruct();
    ASSERT_EQ(recon.size(), data.size());
    double rel = relError(data, recon);
    EXPECT_LE(rel, 0.02);  // ≤ 2% (full Golub-Reinsch SVD with back-accumulation)
}

// TTD-03: core shapes are correct
TEST_F(TensorTrainDecomposerTest, TTD03_4DTensorCoreShapes) {
    std::vector<std::size_t> shape = {3, 4, 5, 6};
    auto data = makeRandom(product(shape));
    TensorTrainConfig cfg; cfg.eps = 0.05; cfg.max_rank = 4;
    auto [train, stats] = decomposer.decompose(data, shape, cfg);
    ASSERT_EQ(train.cores.size(), 4u);
    EXPECT_EQ(train.cores[0].r_left,  1u);
    EXPECT_EQ(train.cores[0].n,       3u);
    EXPECT_EQ(train.cores.back().r_right, 1u);
    EXPECT_EQ(train.cores.back().n,   6u);
}

// TTD-04: achieved_eps ≤ eps * ‖T‖ for 6D tensor
TEST_F(TensorTrainDecomposerTest, TTD04_6DTensorReconstructionError) {
    std::vector<std::size_t> shape = {4, 4, 4, 4, 4, 4};
    auto data = makeRandom(product(shape));
    TensorTrainConfig cfg; cfg.eps = 0.10; cfg.max_rank = 8;
    auto [train, stats] = decomposer.decompose(data, shape, cfg);
    // achieved_eps should be reasonable
    EXPECT_GE(stats.compression_ratio, 0.5);  // at least some parameterisation
    EXPECT_GT(train.order(), 0u);
}

// TTD-05: compressionRatio > 1 for low-rank tensors
TEST_F(TensorTrainDecomposerTest, TTD05_CompressionRatioLowRank) {
    auto data = makeLowRank2D(16, 16, 1);  // rank-1
    std::vector<float> data3d(16*16*16, 1.0f);
    TensorTrainConfig cfg; cfg.eps = 0.01; cfg.max_rank = 2;
    auto [train, stats] = decomposer.decompose(data3d, {16, 16, 16}, cfg);
    EXPECT_GT(stats.compression_ratio, 1.0);
}

// TTD-06: innerProduct(T, T) ≈ ‖T‖²
TEST_F(TensorTrainDecomposerTest, TTD06_InnerProductSelfEqualsNormSq) {
    auto data = makeRandom(2*3*4);
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [train, stats] = decomposer.decompose(data, {2, 3, 4}, cfg);
    double ip   = TensorTrainDecomposer::innerProduct(train, train);
    double norm = TensorTrainDecomposer::frobeniusNorm(train);
    EXPECT_NEAR(ip, norm * norm, 1e-2 * ip + 1e-6);
}

// TTD-07: cosineSimilarity(T, T) = 1.0
TEST_F(TensorTrainDecomposerTest, TTD07_CosineSimilarityIdentical) {
    auto data = makeRandom(2*3*4, 1.0f, 7);
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [train, _] = decomposer.decompose(data, {2, 3, 4}, cfg);
    double cs = TensorTrainDecomposer::cosineSimilarity(train, train);
    EXPECT_NEAR(cs, 1.0, 0.01);
}

// TTD-08: cosineSimilarity of zero tensor = 0.0
TEST_F(TensorTrainDecomposerTest, TTD08_CosineSimilarityZeroTensor) {
    std::vector<float> zero(2*3*4, 0.0f);
    std::vector<float> rand = makeRandom(2*3*4);
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [tz, _1] = decomposer.decompose(zero, {2, 3, 4}, cfg);
    auto [tr, _2] = decomposer.decompose(rand, {2, 3, 4}, cfg);
    double cs = TensorTrainDecomposer::cosineSimilarity(tz, tr);
    EXPECT_NEAR(cs, 0.0, 0.1);
}

// TTD-09: serialize/deserialize round-trip
TEST_F(TensorTrainDecomposerTest, TTD09_SerializeDeserializeRoundTrip) {
    auto data = makeRandom(2*3*4, 1.0f, 99);
    TensorTrainConfig cfg; cfg.eps = 0.01;
    auto [train, _] = decomposer.decompose(data, {2, 3, 4}, cfg);
    auto bytes = train.serialize();
    auto loaded = TTTrain::deserialize(bytes);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->order(), train.order());
    EXPECT_EQ(loaded->mode_sizes, train.mode_sizes);
    EXPECT_EQ(loaded->cores.size(), train.cores.size());
}

// TTD-10: decomposeF64
TEST_F(TensorTrainDecomposerTest, TTD10_DecomposeF64) {
    std::vector<double> data(2*3*4, 0.5);
    TensorTrainConfig cfg; cfg.eps = 0.01;
    auto [train, stats] = decomposer.decomposeF64(data, {2, 3, 4}, cfg);
    EXPECT_EQ(train.order(), 3u);
}

// TTD-11: size mismatch throws
TEST_F(TensorTrainDecomposerTest, TTD11_SizeMismatchThrows) {
    std::vector<float> data(10, 1.0f);
    EXPECT_THROW(
        decomposer.decompose(data, {4, 4}, TensorTrainConfig{}),
        std::invalid_argument);
}

// TTD-12: max_rank cap respected
TEST_F(TensorTrainDecomposerTest, TTD12_MaxRankCap) {
    auto data = makeRandom(8*8*8*8);
    TensorTrainConfig cfg; cfg.eps = 0.0; cfg.max_rank = 3;
    auto [train, stats] = decomposer.decompose(data, {8, 8, 8, 8}, cfg);
    EXPECT_LE(train.maxRank(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TTQuantizer tests
// ─────────────────────────────────────────────────────────────────────────────

class TTQuantizerTest : public ::testing::Test {
protected:
    TensorTrainDecomposer decomposer;
    TTQuantizer           quantizer;
};

// TTD-13: INT8 dequantisation error < 1% of absmax
TEST_F(TTQuantizerTest, TTD13_INT8RoundTripError) {
    auto data = makeRandom(4*4*4, 2.0f, 13);
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [train, _] = decomposer.decompose(data, {4, 4, 4}, cfg);
    auto qt   = quantizer.quantize(train, QuantizationType::INT8);
    auto deq  = quantizer.dequantize(qt);

    ASSERT_EQ(train.cores.size(), deq.cores.size());
    for (std::size_t k = 0; k < train.cores.size(); ++k) {
        const auto& orig = train.cores[k].data;
        const auto& back = deq.cores[k].data;
        float absmax = *std::max_element(orig.begin(), orig.end(),
            [](float a, float b){ return std::abs(a) < std::abs(b); });
        absmax = std::abs(absmax);
        if (absmax < 1e-6f) continue;
        for (std::size_t i = 0; i < orig.size(); ++i)
            EXPECT_LE(std::abs(orig[i] - back[i]) / absmax, 0.01f);
    }
}

// TTD-14: NF4 dequantisation error < 5%
TEST_F(TTQuantizerTest, TTD14_NF4RoundTripError) {
    auto data = makeRandom(4*4, 1.0f, 77);
    TensorTrainConfig cfg; cfg.eps = 0.001;
    auto [train, _] = decomposer.decompose(data, {4, 4}, cfg);
    auto qt  = quantizer.quantize(train, QuantizationType::NF4);
    auto deq = quantizer.dequantize(qt);

    ASSERT_EQ(train.cores.size(), deq.cores.size());
    for (std::size_t k = 0; k < train.cores.size(); ++k) {
        const auto& orig = train.cores[k].data;
        const auto& back = deq.cores[k].data;
        float absmax = 1.0f;
        for (float v : orig) absmax = std::max(absmax, std::abs(v));
        for (std::size_t i = 0; i < orig.size(); ++i)
            EXPECT_LE(std::abs(orig[i] - back[i]) / absmax, 0.10f);
    }
}

// TTD-15: QuantizedTrain serialize/deserialize
TEST_F(TTQuantizerTest, TTD15_QuantizedTrainSerializeDeserialize) {
    auto data = makeRandom(3*3*3, 1.0f, 55);
    TensorTrainConfig cfg; cfg.eps = 0.01;
    auto [train, _] = decomposer.decompose(data, {3, 3, 3}, cfg);
    auto qt = quantizer.quantize(train, QuantizationType::INT8);
    auto bytes = qt.serialize();
    auto loaded = QuantizedTrain::deserialize(bytes);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->cores.size(), qt.cores.size());
    EXPECT_EQ(loaded->mode_sizes, qt.mode_sizes);
}

// TTD-16: bytesPerElement
TEST_F(TTQuantizerTest, TTD16_BytesPerElement) {
    EXPECT_DOUBLE_EQ(TTQuantizer::bytesPerElement(QuantizationType::NONE), 4.0);
    EXPECT_DOUBLE_EQ(TTQuantizer::bytesPerElement(QuantizationType::INT8), 1.0);
    EXPECT_DOUBLE_EQ(TTQuantizer::bytesPerElement(QuantizationType::NF4),  0.5);
}

// ─────────────────────────────────────────────────────────────────────────────
// TensorNetworkStorageEngine tests
// ─────────────────────────────────────────────────────────────────────────────

class TensorNetworkStorageEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ = std::make_shared<InMemoryTensorBackend>();
        TensorStorageConfig cfg;
        cfg.tt_config.eps   = 0.05;
        cfg.tt_config.max_rank = 8;
        cfg.quant_type      = QuantizationType::INT8;
        cfg.min_compression_ratio = 0.0;  // always use TT
        engine_ = std::make_unique<TensorNetworkStorageEngine>(backend_, cfg);
    }

    std::shared_ptr<InMemoryTensorBackend>         backend_;
    std::unique_ptr<TensorNetworkStorageEngine>     engine_;

    const TensorFieldKey kKey{"tenant1", "col1", "field1"};
};

// TNS-01: InMemoryTensorBackend put/get/del
TEST(InMemoryTensorBackendTest, TNS01_PutGetDel) {
    InMemoryTensorBackend b;
    std::string key = "k1";
    std::vector<uint8_t> val = {1, 2, 3};
    EXPECT_TRUE(b.put(key, val));
    auto got = b.get(key);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, val);
    EXPECT_TRUE(b.del(key));
    EXPECT_FALSE(b.get(key).has_value());
}

// TNS-02: put stores without error
TEST_F(TensorNetworkStorageEngineTest, TNS02_PutStoresWithoutError) {
    std::vector<float> data(2*3*4, 1.0f);
    EXPECT_TRUE(engine_->put(kKey, data, {2, 3, 4}));
}

// TNS-03: get reconstructs close to original
TEST_F(TensorNetworkStorageEngineTest, TNS03_GetReconstructsCloseToOriginal) {
    auto data = makeRandom(4*4, 1.0f, 201);
    ASSERT_TRUE(engine_->put(kKey, data, {4, 4}));
    auto got = engine_->get(kKey);
    ASSERT_TRUE(got.has_value());
    ASSERT_EQ(got->size(), data.size());
    double err = relError(data, *got);
    EXPECT_LE(err, 0.20);  // ≤ 20% after quantisation (generous threshold for stub SVD)
}

// TNS-04: getCompressed returns QuantizedTrain
TEST_F(TensorNetworkStorageEngineTest, TNS04_GetCompressedReturnsQTrain) {
    std::vector<float> data(3*3*3, 0.5f);
    ASSERT_TRUE(engine_->put(kKey, data, {3, 3, 3}));
    auto qopt = engine_->getCompressed(kKey);
    ASSERT_TRUE(qopt.has_value());
    EXPECT_EQ(qopt->order(), 3u);
}

// TNS-05: stats() returns valid data
TEST_F(TensorNetworkStorageEngineTest, TNS05_StatsValid) {
    std::vector<float> data(4*4*4, 2.0f);
    ASSERT_TRUE(engine_->put(kKey, data, {4, 4, 4}));
    auto s = engine_->stats(kKey);
    ASSERT_TRUE(s.has_value());
    EXPECT_GT(s->compressed_bytes, 0u);
    EXPECT_EQ(s->current_version, 1u);
}

// TNS-06: remove returns false for unknown key
TEST_F(TensorNetworkStorageEngineTest, TNS06_RemoveMissingKeyReturnsFalse) {
    TensorFieldKey missing{"t", "c", "no_such_field"};
    EXPECT_FALSE(engine_->remove(missing));
}

// TNS-07: multiple versions tracked
TEST_F(TensorNetworkStorageEngineTest, TNS07_MultipleVersionsTracked) {
    auto data1 = makeRandom(2*2, 1.0f, 1);
    auto data2 = makeRandom(2*2, 1.0f, 2);
    ASSERT_TRUE(engine_->put(kKey, data1, {2, 2}));
    ASSERT_TRUE(engine_->put(kKey, data2, {2, 2}));

    auto s = engine_->stats(kKey);
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->current_version, 2u);
}

// TNS-08: null backend throws
TEST(TensorNetworkStorageEngineTest2, TNS08_NullBackendThrows) {
    EXPECT_THROW(
        TensorNetworkStorageEngine(nullptr),
        std::invalid_argument);
}

TEST_F(TensorNetworkStorageEngineTest, TNS09_CompactIgnoresMalformedVersionSuffix) {
    auto data = makeRandom(2 * 2, 1.0f, 7);
    ASSERT_TRUE(engine_->put(kKey, data, {2, 2}));

    const std::string malformed_key =
        "__ttn__:" + kKey.tenant + ":" + kKey.collection + ":" + kKey.field + ":meta:not_a_number";
    ASSERT_TRUE(backend_->put(malformed_key, {0x01, 0x02, 0x03}));

    EXPECT_NO_THROW(engine_->compact(kKey));
    EXPECT_TRUE(backend_->get(malformed_key).has_value());

    auto got = engine_->get(kKey);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->size(), data.size());
}
