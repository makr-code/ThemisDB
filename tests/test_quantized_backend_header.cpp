/**
 * @file test_quantized_backend_header.cpp
 * @brief Unit tests for the IQuantizedBackend header interface.
 *
 * Uses an inline mock implementation — no hardware quantisation is performed.
 * All tests validate the public contract defined in quantized_backend.h.
 */

#include <gtest/gtest.h>
#include "acceleration/quantized_backend.h"

#include <cmath>
#include <numeric>

using namespace themis::acceleration;

// ============================================================================
// Minimal mock implementation of IQuantizedBackend
// ============================================================================

class MockQuantizedBackend final : public IQuantizedBackend {
public:
    // IComputeBackend interface
    const char* name() const noexcept override { return "mock_quantized"; }
    BackendType type() const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.deviceName = "MockQuantizedCPU";
        caps.supportsMatrixOps = true;
        caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::INT8 | PrecisionMode::INT4;
        return caps;
    }
    bool initialize() override { return true; }
    void shutdown() override {}

    // IQuantizedBackend interface
    QuantizedTensor quantize(
        const std::vector<float>& tensor,
        const std::vector<size_t>& shape,
        const QuantizationConfig& config) override
    {
        QuantizedTensor qt;
        qt.shape = shape;
        qt.scheme = config.scheme;
        qt.original_dtype_bytes = 4;

        // Compute a simple symmetric scale from the max absolute value
        float max_abs = 0.0f;
        for (float v : tensor) {
            if (std::abs(v) > max_abs) max_abs = std::abs(v);
        }
        const float scale = (max_abs > 0.0f) ? max_abs / 127.0f : 1.0f;
        qt.scales.push_back(scale);

        qt.data.reserve(tensor.size());
        for (float v : tensor) {
            qt.data.push_back(static_cast<int8_t>(std::round(v / scale)));
        }
        return qt;
    }

    std::vector<float> dequantize(const QuantizedTensor& qtensor) override {
        std::vector<float> out;
        const float scale = qtensor.scales.empty() ? 1.0f : qtensor.scales[0];
        out.reserve(qtensor.data.size());
        for (int8_t v : qtensor.data) {
            out.push_back(static_cast<float>(v) * scale);
        }
        return out;
    }

    std::vector<float> quantizedMatmul(
        const QuantizedTensor& a,
        const QuantizedTensor& b,
        size_t M, size_t K, size_t N) override
    {
        // Naive reference implementation for testing purposes
        const float sa = a.scales.empty() ? 1.0f : a.scales[0];
        const float sb = b.scales.empty() ? 1.0f : b.scales[0];
        std::vector<float> result(M * N, 0.0f);
        for (size_t m = 0; m < M; ++m) {
            for (size_t n = 0; n < N; ++n) {
                float acc = 0.0f;
                for (size_t k = 0; k < K; ++k) {
                    acc += static_cast<float>(a.data[m * K + k]) * sa
                         * static_cast<float>(b.data[k * N + n]) * sb;
                }
                result[m * N + n] = acc;
            }
        }
        return result;
    }

    QuantizationStats calibrate(
        const std::vector<std::vector<float>>& samples,
        const QuantizationConfig& /*config*/) override
    {
        QuantizationStats stats;
        stats.calibration_samples = samples.size();
        stats.compression_ratio = 0.25; // INT8 vs FP32 = 4x compression
        stats.peak_snr_db = 40.0;
        stats.quantization_error_l2 = 0.01;
        return stats;
    }

    std::vector<QuantizationScheme> supportedSchemes() const override {
        return {
            QuantizationScheme::INT8_SYMMETRIC,
            QuantizationScheme::INT8_ASYMMETRIC,
            QuantizationScheme::INT4_SYMMETRIC,
            QuantizationScheme::INT4_GROUPED,
        };
    }

    bool hasNativeInt4Support() const override { return false; }
};

// ============================================================================
// Tests
// ============================================================================

class QuantizedBackendHeaderTest : public ::testing::Test {
protected:
    MockQuantizedBackend backend;
};

TEST_F(QuantizedBackendHeaderTest, QuantizationConfigDefaults) {
    QuantizationConfig cfg;
    EXPECT_EQ(QuantizationScheme::INT8_SYMMETRIC, cfg.scheme);
    EXPECT_EQ(128, cfg.group_size);
    EXPECT_FALSE(cfg.per_channel);
    EXPECT_FALSE(cfg.enable_calibration);
    EXPECT_FLOAT_EQ(99.9f, cfg.calibration_percentile);
}

TEST_F(QuantizedBackendHeaderTest, QuantizationSchemeEnumValuesDistinct) {
    EXPECT_NE(QuantizationScheme::INT8_SYMMETRIC,  QuantizationScheme::INT8_ASYMMETRIC);
    EXPECT_NE(QuantizationScheme::INT4_SYMMETRIC,  QuantizationScheme::INT4_GROUPED);
    EXPECT_NE(QuantizationScheme::FP8_E4M3,        QuantizationScheme::FP8_E5M2);
    EXPECT_NE(QuantizationScheme::INT8_SYMMETRIC,  QuantizationScheme::INT4_GROUPED);
}

TEST_F(QuantizedBackendHeaderTest, QuantizedTensorDefaultOriginalDtypeBytes) {
    QuantizedTensor qt;
    EXPECT_EQ(4u, qt.original_dtype_bytes);
}

TEST_F(QuantizedBackendHeaderTest, QuantizeProducesCorrectShape) {
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f};
    QuantizationConfig cfg;
    auto qt = backend.quantize(data, {2, 2}, cfg);

    ASSERT_EQ(2u, qt.shape.size());
    EXPECT_EQ(2u, qt.shape[0]);
    EXPECT_EQ(2u, qt.shape[1]);
    EXPECT_EQ(4u, qt.data.size());
}

TEST_F(QuantizedBackendHeaderTest, DequantizeApproximatesOriginal) {
    std::vector<float> original = {1.0f, -2.0f, 0.5f, -0.5f};
    QuantizationConfig cfg;
    auto qt = backend.quantize(original, {4}, cfg);
    auto recovered = backend.dequantize(qt);

    ASSERT_EQ(original.size(), recovered.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(original[i], recovered[i], 0.05f)
            << "Dequantisation error too large at index " << i;
    }
}

TEST_F(QuantizedBackendHeaderTest, QuantizedMatmulIdentityScale) {
    // 2×2 identity-like multiply: A = [[1,0],[0,1]], B = [[1,0],[0,1]]
    std::vector<float> a_data = {1.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> b_data = {1.0f, 0.0f, 0.0f, 1.0f};
    QuantizationConfig cfg;

    auto qa = backend.quantize(a_data, {2, 2}, cfg);
    auto qb = backend.quantize(b_data, {2, 2}, cfg);

    auto result = backend.quantizedMatmul(qa, qb, 2, 2, 2);
    ASSERT_EQ(4u, result.size());
    // Diagonal elements should be close to 1, off-diagonal close to 0
    EXPECT_GT(result[0], 0.8f);
    EXPECT_LT(std::abs(result[1]), 0.2f);
    EXPECT_LT(std::abs(result[2]), 0.2f);
    EXPECT_GT(result[3], 0.8f);
}

TEST_F(QuantizedBackendHeaderTest, CalibrationStats_CompressionRatio) {
    std::vector<std::vector<float>> samples = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f}
    };
    QuantizationConfig cfg;
    auto stats = backend.calibrate(samples, cfg);

    EXPECT_EQ(2u, stats.calibration_samples);
    EXPECT_GT(stats.compression_ratio, 0.0);
    EXPECT_LE(stats.compression_ratio, 1.0);
}

TEST_F(QuantizedBackendHeaderTest, SupportedSchemesNonEmpty) {
    auto schemes = backend.supportedSchemes();
    EXPECT_FALSE(schemes.empty());
}

TEST_F(QuantizedBackendHeaderTest, HasNativeInt4SupportReturnsBool) {
    // The mock reports false; we simply verify the method is callable and returns bool.
    const bool result = backend.hasNativeInt4Support();
    EXPECT_FALSE(result);
}

TEST_F(QuantizedBackendHeaderTest, BackendCapabilitiesReflectQuantization) {
    auto caps = backend.getCapabilities();
    const bool supportsInt8 = hasPrecision(caps.supportedPrecisions, PrecisionMode::INT8);
    const bool supportsInt4 = hasPrecision(caps.supportedPrecisions, PrecisionMode::INT4);
    EXPECT_TRUE(supportsInt8);
    EXPECT_TRUE(supportsInt4);
}

TEST_F(QuantizedBackendHeaderTest, IsAvailableAndInitialize) {
    EXPECT_TRUE(backend.isAvailable());
    EXPECT_TRUE(backend.initialize());
    EXPECT_STREQ("mock_quantized", backend.name());
}
