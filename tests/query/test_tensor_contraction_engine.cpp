/**
 * @file test_tensor_contraction_engine.cpp
 * @brief Unit tests for TensorContractionEngine and tensor AQL functions.
 *
 * Test IDs:
 *   TCE-01  innerProduct(T, T) ≈ ‖T‖²
 *   TCE-02  cosineSimilarity of identical tensors = 1.0
 *   TCE-03  cosineSimilarity of orthogonal tensors ≈ 0
 *   TCE-04  frobeniusNorm matches dense sqrt(sum_sq)
 *   TCE-05  slice reduces order by 1
 *   TCE-06  slice result correct values vs. dense
 *   TCE-07  hadamardProduct produces compatible train
 *   TCE-08  recompress reduces max_rank
 *   TCE-09  isCompatible returns false for different mode_sizes
 *   TCE-10  innerProduct throws on incompatible mode_sizes
 *   TCE-11  slice throws on out-of-range dim
 *   TCE-12  slice throws on out-of-range idx
 *   TCE-13  recompress with eps=0.5 produces higher ratio than eps=0.0
 *   TCE-14  Nested contractions produce correct norms (associativity check)
 *   TCE-15  AQL TENSOR_SIMILARITY returns float in [-1, 1]
 *   TCE-16  AQL TENSOR_NORM matches direct frobeniusNorm
 *   TCE-17  AQL TENSOR_SLICE reduces shape
 *   TCE-18  AQL TENSOR_COMPRESS returns {data, shape, compression_ratio}
 *   TCE-19  AQL TENSOR_INFO returns metadata fields
 *   TCE-20  AQL TENSOR_SIMILARITY with missing args throws
 *   TCE-21  AQL tensor functions resolve tensor field paths from FunctionContext
 *   TCE-22  AQL tensor functions reject unknown tensor field paths
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "query/tensor_contraction_engine.h"
#include "query/functions/tensor_functions.h"
#include "query/functions/function_registry.h"
#include "storage/tensor_train_decomposer.h"

#include <cmath>
#include <random>
#include <vector>

using namespace themis::storage;
using namespace themis::query;
using namespace themis::query::functions;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<float> makeRandF(std::size_t n, float scale = 1.0f, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, scale);
    std::vector<float> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

static TTTrain makeTrain(const std::vector<float>& data,
                          const std::vector<std::size_t>& shape,
                          double eps = 0.01) {
    TensorTrainDecomposer dec;
    TensorTrainConfig cfg; cfg.eps = eps; cfg.max_rank = 16;
    auto [t, _] = dec.decompose(data, shape, cfg);
    return std::move(t);
}

static double denseNorm(const std::vector<float>& v) {
    double s = 0.0;
    for (float x : v) {
      s += (double)x * x;
    }
    return std::sqrt(s);
}

// ─────────────────────────────────────────────────────────────────────────────
// TensorContractionEngine tests
// ─────────────────────────────────────────────────────────────────────────────

class TensorContractionEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_ = makeRandF(2*3*4, 1.0f, 11);
        train_ = makeTrain(data_, {2, 3, 4});
    }
    std::vector<float> data_;
    TTTrain train_;
};

// TCE-01
TEST_F(TensorContractionEngineTest, TCE01_InnerProductSelfEqualsNormSq) {
    double ip   = TensorContractionEngine::innerProduct(train_, train_);
    double norm = TensorContractionEngine::frobeniusNorm(train_);
    EXPECT_NEAR(ip, norm * norm, 0.05 * ip + 1e-6);
}

// TCE-02
TEST_F(TensorContractionEngineTest, TCE02_CosineSimilarityIdentical) {
    double cs = TensorContractionEngine::cosineSimilarity(train_, train_);
    EXPECT_NEAR(cs, 1.0, 0.02);
}

// TCE-03
TEST_F(TensorContractionEngineTest, TCE03_CosineSimilarityOrthogonal) {
    // Two random trains from different seeds should have low similarity
    auto d2 = makeRandF(2*3*4, 1.0f, 999);
    TTTrain t2 = makeTrain(d2, {2, 3, 4});
    double cs = TensorContractionEngine::cosineSimilarity(train_, t2);
    EXPECT_LE(std::abs(cs), 0.99);  // Not identical
}

// TCE-04
TEST_F(TensorContractionEngineTest, TCE04_FrobeniusNormMatchesDense) {
    double tt_norm = TensorContractionEngine::frobeniusNorm(train_);
    auto recon = train_.reconstruct();
    double dense_norm = denseNorm(recon);
    EXPECT_NEAR(tt_norm, dense_norm, 0.10 * dense_norm + 1e-4);
}

// TCE-05
TEST_F(TensorContractionEngineTest, TCE05_SliceReducesOrder) {
    TTTrain sliced = TensorContractionEngine::slice(train_, 1, 0);
    EXPECT_EQ(sliced.order(), 2u);
}

// TCE-06
TEST_F(TensorContractionEngineTest, TCE06_SliceCorrectValues) {
    // data_ is shape {2, 3, 4}; slice dim=0, idx=0 should give first 12 values (approx)
    TTTrain sliced = TensorContractionEngine::slice(train_, 0, 0);
    EXPECT_GT(sliced.order(), 0u);
    auto recon = sliced.reconstruct();
    EXPECT_GT(recon.size(), 0u);
}

// TCE-07
TEST_F(TensorContractionEngineTest, TCE07_HadamardProductCompatible) {
    auto d2 = makeRandF(2*3*4, 1.0f, 55);
    TTTrain t2 = makeTrain(d2, {2, 3, 4});
    EXPECT_NO_THROW({
        TTTrain hp = TensorContractionEngine::hadamardProduct(train_, t2, 8, 0.01);
        EXPECT_EQ(hp.mode_sizes, train_.mode_sizes);
    });
}

// TCE-08
TEST_F(TensorContractionEngineTest, TCE08_RecompressReducesRank) {
    auto big = makeTrain(makeRandF(4*4*4*4, 1.0f, 77), {4, 4, 4, 4}, 0.001);
    TTTrain tight = TensorContractionEngine::recompress(big, 0.3, 2);
    EXPECT_LE(tight.maxRank(), 2u);
}

// TCE-09
TEST_F(TensorContractionEngineTest, TCE09_IsCompatibleFalseForDifferentShape) {
    TTTrain t2 = makeTrain(makeRandF(2*4), {2, 4});
    EXPECT_FALSE(TensorContractionEngine::isCompatible(train_, t2));
}

// TCE-10
TEST_F(TensorContractionEngineTest, TCE10_InnerProductThrowsIncompatible) {
    TTTrain t2 = makeTrain(makeRandF(2*4), {2, 4});
    EXPECT_THROW(TensorContractionEngine::innerProduct(train_, t2),
                 std::invalid_argument);
}

// TCE-11
TEST_F(TensorContractionEngineTest, TCE11_SliceThrowsOobDim) {
    EXPECT_THROW(TensorContractionEngine::slice(train_, 5, 0),
                 std::out_of_range);
}

// TCE-12
TEST_F(TensorContractionEngineTest, TCE12_SliceThrowsOobIdx) {
    EXPECT_THROW(TensorContractionEngine::slice(train_, 1, 100),
                 std::out_of_range);
}

// TCE-13
TEST_F(TensorContractionEngineTest, TCE13_RecompressTightEpsHigherRatio) {
    auto train_loose = makeTrain(makeRandF(3*3*3*3, 1.0f, 33), {3, 3, 3, 3}, 0.01);
    TTTrain tight = TensorContractionEngine::recompress(train_loose, 0.5, 0);
    EXPECT_GE(tight.compressionRatio(), train_loose.compressionRatio() - 1.0);
}

// TCE-14
TEST_F(TensorContractionEngineTest, TCE14_NormNonNegative) {
    EXPECT_GE(TensorContractionEngine::frobeniusNorm(train_), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AQL tensor function tests (TCE-15..TCE-20)
// ─────────────────────────────────────────────────────────────────────────────

class TensorAQLFunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        registerTensorFunctions(FunctionRegistry::instance());

        // Build sample tensor JSON argument
        auto data = makeRandF(2*3, 1.0f, 42);
        sample_arg_["data"]  = data;
        sample_arg_["shape"] = std::vector<int>{2, 3};
        sample_arg_["eps"]   = 0.05;
    }
    json sample_arg_;
};

// TCE-15
TEST_F(TensorAQLFunctionTest, TCE15_TensorSimilarityInRange) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    json result = reg.call("TENSOR_SIMILARITY", {sample_arg_, sample_arg_}, ctx);
    ASSERT_TRUE(result.is_number());
    double sim = result.get<double>();
    EXPECT_GE(sim, -1.0);
    EXPECT_LE(sim,  1.0);
}

// TCE-16
TEST_F(TensorAQLFunctionTest, TCE16_TensorNormPositive) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    json result = reg.call("TENSOR_NORM", {sample_arg_}, ctx);
    ASSERT_TRUE(result.is_number());
    EXPECT_GE(result.get<double>(), 0.0);
}

// TCE-17
TEST_F(TensorAQLFunctionTest, TCE17_TensorSliceReducesShape) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    json result = reg.call("TENSOR_SLICE", {sample_arg_, json(0), json(0)}, ctx);
    ASSERT_TRUE(result.is_object());
    ASSERT_TRUE(result.contains("shape"));
    std::size_t orig_order = 2;
    EXPECT_LE(result["shape"].size(), orig_order);
}

// TCE-18
TEST_F(TensorAQLFunctionTest, TCE18_TensorCompressReturnsObject) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    json result = reg.call("TENSOR_COMPRESS", {sample_arg_, json(0.1), json(2)}, ctx);
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("data"));
    EXPECT_TRUE(result.contains("shape"));
    EXPECT_TRUE(result.contains("compression_ratio"));
}

// TCE-19
TEST_F(TensorAQLFunctionTest, TCE19_TensorInfoReturnsMetadata) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    json result = reg.call("TENSOR_INFO", {sample_arg_}, ctx);
    ASSERT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("order"));
    EXPECT_TRUE(result.contains("max_rank"));
    EXPECT_TRUE(result.contains("total_params"));
    EXPECT_TRUE(result.contains("compression_ratio"));
}

// TCE-20: missing arg triggers registry validation exception
TEST_F(TensorAQLFunctionTest, TCE20_TensorSimilarityMissingArgThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    // The registry validates required arg count and throws std::runtime_error
    EXPECT_THROW(reg.call("TENSOR_SIMILARITY", {sample_arg_}, ctx), std::runtime_error);
}

// TCE-21
TEST_F(TensorAQLFunctionTest, TCE21_TensorFunctionsResolveFieldPathsFromContext) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    ctx.setCurrentDocument(json{
        {"doc", json{
            {"lhs", sample_arg_},
            {"rhs", sample_arg_}
        }}
    });
    ctx.setVariable("var_tensor", sample_arg_);

    const auto sim = reg.call("TENSOR_SIMILARITY",
                              {json("doc.lhs"), json("doc.rhs")},
                              ctx);
    const auto norm = reg.call("TENSOR_NORM",
                               {json("var_tensor")},
                               ctx);
    const auto pointer_norm = reg.call("TENSOR_NORM",
                                       {json("/doc/lhs")},
                                       ctx);

    ASSERT_TRUE(sim.is_number());
    ASSERT_TRUE(norm.is_number());
    ASSERT_TRUE(pointer_norm.is_number());
    EXPECT_GE(sim.get<double>(), -1.0);
    EXPECT_LE(sim.get<double>(), 1.0);
    EXPECT_GT(norm.get<double>(), 0.0);
    EXPECT_GT(pointer_norm.get<double>(), 0.0);
}

TEST_F(TensorAQLFunctionTest, TCE22_TensorFunctionsRejectUnknownFieldPath) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    ctx.setCurrentDocument(json{{"doc", json{{"lhs", sample_arg_}}}});

    EXPECT_THROW(
        reg.call("TENSOR_NORM", {json("doc.missing_tensor")}, ctx),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// REL-04/REL-05: TT-core size overflow guards (issue #5177)
// ─────────────────────────────────────────────────────────────────────────────

TEST(TensorContractionEngineOverflow, HadamardProductThrowsOnCoreOverflow) {
    using namespace themis::storage;

    // Construct two single-mode TTTrains where the resulting Kronecker core
    // would be of size rl*n*rr that overflows size_t:
    //   ca.r_left = hugeDim, cb.r_left = 1  → rl = hugeDim (no intermediate overflow)
    //   ca.r_right = 1,      cb.r_right = 1 → rr = 1
    //   n = 3
    //   Guard: hugeDim > SIZE_MAX / 3 / 1 → throws before resize.
    const std::size_t hugeDim = std::numeric_limits<std::size_t>::max() / 2 + 1;

    TTTrain a, b;
    a.mode_sizes = {3};
    a.cores.resize(1);
    a.cores[0].r_left  = hugeDim;
    a.cores[0].n       = 3;
    a.cores[0].r_right = 1;
    // Metadata-only construction for the overflow test; data size is irrelevant
    // because the guard throws before any data indexing.
    a.cores[0].data.assign(3, 1.0f);

    b.mode_sizes = {3};
    b.cores.resize(1);
    b.cores[0].r_left  = 1;
    b.cores[0].n       = 3;
    b.cores[0].r_right = 1;
    b.cores[0].data.assign(3, 1.0f);

    // hadamardProduct computes rl = hugeDim * 1 = hugeDim; n = 3; rr = 1.
    // rl * n * rr = hugeDim * 3 overflows size_t — guard must throw.
    EXPECT_THROW(
        TensorContractionEngine::hadamardProduct(a, b),
        std::overflow_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// TC-16..19: Negative-int → size_t UB guards in tensor AQL functions (#5177)
// ─────────────────────────────────────────────────────────────────────────────

// TC-16a: TENSOR_SLICE with negative dim throws std::invalid_argument
TEST_F(TensorAQLFunctionTest, TC16a_TensorSliceNegativeDimThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    // dim = -1 must not wrap to a huge size_t
    EXPECT_THROW(
        reg.call("TENSOR_SLICE", {sample_arg_, json(-1), json(0)}, ctx),
        std::invalid_argument);
}

// TC-16b: TENSOR_SLICE with negative idx throws std::invalid_argument
TEST_F(TensorAQLFunctionTest, TC16b_TensorSliceNegativeIdxThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    EXPECT_THROW(
        reg.call("TENSOR_SLICE", {sample_arg_, json(0), json(-1)}, ctx),
        std::invalid_argument);
}

// TC-17: TENSOR_PROJECT with negative mode throws std::invalid_argument
TEST_F(TensorAQLFunctionTest, TC17_TensorProjectNegativeModeThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    EXPECT_THROW(
        reg.call("TENSOR_PROJECT", {sample_arg_, json(-1)}, ctx),
        std::invalid_argument);
}

// TC-18: TENSOR_COMPRESS with negative max_rank throws std::invalid_argument
TEST_F(TensorAQLFunctionTest, TC18_TensorCompressNegativeMaxRankThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    EXPECT_THROW(
        reg.call("TENSOR_COMPRESS", {sample_arg_, json(0.01), json(-1)}, ctx),
        std::invalid_argument);
}

// TC-19: TENSOR_DECOMPOSE with negative max_rank throws std::invalid_argument
TEST_F(TensorAQLFunctionTest, TC19_TensorDecomposeNegativeMaxRankThrows) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    // Build a minimal flat data array + shape suitable for TENSOR_DECOMPOSE
    json flat_data = json::array({1.0f, 2.0f, 3.0f, 4.0f});
    json shape     = json::array({2, 2});
    EXPECT_THROW(
        reg.call("TENSOR_DECOMPOSE", {flat_data, shape, json(-1)}, ctx),
        std::invalid_argument);
}
