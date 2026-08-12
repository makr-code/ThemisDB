#include <gtest/gtest.h>
#include "llm/lora_framework/quantization.h"
#include <vector>
#include <cmath>
#include <random>

using namespace themis::llm::lora;

/**
 * @file test_quantization.cpp
 * @brief Comprehensive tests for quantization (NF4, INT8, double quantization)
 */

class QuantizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize random number generator with fixed seed for reproducibility
        gen_ = std::mt19937(42);
    }
    
    // Generate random data with normal distribution
    std::vector<float> generate_random_data(size_t size, float mean = 0.0f, float stddev = 1.0f) {
        std::normal_distribution<float> dist(mean, stddev);
        std::vector<float> data(size);
        for (auto& val : data) {
            val = dist(gen_);
        }
        return data;
    }
    
    // Compute MSE between two vectors
    float compute_mse(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vector sizes must match");
        }
        float mse = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            mse += diff * diff;
        }
        return mse / a.size();
    }
    
    std::mt19937 gen_;
};

// ===== NF4 Quantization Tests =====

TEST_F(QuantizationTest, NF4_FindBin) {
    // Test finding nearest NF4 bin
    
    // Test exact matches
    EXPECT_EQ(quantization::find_nf4_bin(-1.0f), 0);
    EXPECT_EQ(quantization::find_nf4_bin(0.0f), 7);
    EXPECT_EQ(quantization::find_nf4_bin(1.0f), 15);
    
    // Test approximate matches
    EXPECT_EQ(quantization::find_nf4_bin(-0.7f), 1);  // Nearest to -0.6962
    EXPECT_EQ(quantization::find_nf4_bin(0.5f), 12);  // Nearest to 0.4407 or 0.5626
}

TEST_F(QuantizationTest, NF4_QuantizeDequantize_Small) {
    // Test NF4 quantization on small tensor
    std::vector<float> input = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    
    QuantizedTensor quantized(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, quantized);
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    // Check size
    EXPECT_EQ(output.size(), input.size());
    
    // Check approximate reconstruction (should be close but not exact)
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_NEAR(output[i], input[i], 0.2f) << "Index " << i;
    }
}

TEST_F(QuantizationTest, NF4_QuantizationError) {
    // Test quantization error is within acceptable range
    auto input = generate_random_data(256, 0.0f, 1.0f);
    
    QuantizedTensor quantized(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, quantized);
    
    float error = quantization::quantization_error(input, quantized);
    
    // NF4 should have low quantization error for normally distributed data
    // Typical MSE should be < 0.01 for normalized data
    EXPECT_LT(error, 0.05f) << "Quantization error too high: " << error;
}

TEST_F(QuantizationTest, NF4_DebugLogSinkReceivesMessages) {
    std::vector<std::string> messages;
    quantization::setDebugLogFn([&](const std::string& message) {
        messages.push_back(message);
    });

    auto input = generate_random_data(64);
    QuantizedTensor quantized(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, quantized);

    quantization::setDebugLogFn(nullptr);

    ASSERT_FALSE(messages.empty());
    EXPECT_NE(messages.front().find("Quantizing to NF4"), std::string::npos);
}

TEST_F(QuantizationTest, NF4_MemoryReduction) {
    // Test that NF4 reduces memory usage by ~8x
    size_t num_elements = 1024;
    auto input = generate_random_data(num_elements);
    
    QuantizedTensor quantized(QuantizationType::NF4, {num_elements}, 64);
    quantization::quantize_nf4(input, quantized);
    
    size_t original_bytes = num_elements * sizeof(float);  // 4096 bytes
    size_t quantized_bytes = quantized.memory_bytes();
    
    // NF4 uses 4 bits per value + block params
    // Expected: ~512 bytes for data + ~96 bytes for 16 blocks = ~608 bytes
    EXPECT_LT(quantized_bytes, original_bytes / 4) << "Memory not reduced enough";
    
    // Should be roughly 1/8 to 1/6 of original (accounting for block params)
    float reduction_ratio = static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_LT(reduction_ratio, 0.25f) << "Reduction ratio: " << reduction_ratio;
}

TEST_F(QuantizationTest, NF4_BlockWise) {
    // Test block-wise quantization with different block sizes
    auto input = generate_random_data(512, 0.0f, 1.0f);
    
    // Test with block size 64
    QuantizedTensor q64(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, q64, 64);
    EXPECT_EQ(q64.num_blocks(), 8);  // 512 / 64 = 8
    
    // Test with block size 128
    QuantizedTensor q128(QuantizationType::NF4, {input.size()}, 128);
    quantization::quantize_nf4(input, q128, 128);
    EXPECT_EQ(q128.num_blocks(), 4);  // 512 / 128 = 4
    
    // Smaller blocks should have slightly better accuracy
    float error64 = quantization::quantization_error(input, q64);
    float error128 = quantization::quantization_error(input, q128);
    
    // Both should be low, but 64 might be slightly better
    EXPECT_LT(error64, 0.05f);
    EXPECT_LT(error128, 0.05f);
}

// ===== INT8 Quantization Tests =====

TEST_F(QuantizationTest, INT8_QuantizeDequantize_Small) {
    // Test INT8 quantization on small tensor
    std::vector<float> input = {-127.0f, -64.0f, 0.0f, 64.0f, 127.0f};
    
    QuantizedTensor quantized(QuantizationType::INT8, {input.size()}, 64);
    quantization::quantize_int8(input, quantized);
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    // Check size
    EXPECT_EQ(output.size(), input.size());
    
    // INT8 should be more accurate than NF4
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_NEAR(output[i], input[i], 2.0f) << "Index " << i;
    }
}

TEST_F(QuantizationTest, INT8_QuantizationError) {
    // Test INT8 quantization error
    auto input = generate_random_data(256, 0.0f, 1.0f);
    
    QuantizedTensor quantized(QuantizationType::INT8, {input.size()}, 64);
    quantization::quantize_int8(input, quantized);
    
    float error = quantization::quantization_error(input, quantized);
    
    // INT8 should have very low error
    EXPECT_LT(error, 0.001f) << "Quantization error too high: " << error;
}

TEST_F(QuantizationTest, INT8_MemoryReduction) {
    // Test that INT8 reduces memory usage by ~4x
    size_t num_elements = 1024;
    auto input = generate_random_data(num_elements);
    
    QuantizedTensor quantized(QuantizationType::INT8, {num_elements}, 64);
    quantization::quantize_int8(input, quantized);
    
    size_t original_bytes = num_elements * sizeof(float);  // 4096 bytes
    size_t quantized_bytes = quantized.memory_bytes();
    
    // INT8 uses 8 bits per value + block params
    // Expected: ~1024 bytes for data + ~96 bytes for blocks = ~1120 bytes
    EXPECT_LT(quantized_bytes, original_bytes / 2) << "Memory not reduced enough";
    
    // Should be roughly 1/4 to 1/3 of original
    float reduction_ratio = static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_LT(reduction_ratio, 0.5f) << "Reduction ratio: " << reduction_ratio;
}

TEST_F(QuantizationTest, INT8_SymmetricQuantization) {
    // Test symmetric quantization (zero-centered)
    std::vector<float> input = {-10.0f, -5.0f, 0.0f, 5.0f, 10.0f};
    
    QuantizedTensor quantized(QuantizationType::INT8, {input.size()}, 64);
    quantization::quantize_int8(input, quantized);
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    // Check symmetric property: dequant(-x) ≈ -dequant(x)
    EXPECT_NEAR(output[0], -output[4], 0.5f);  // -10 vs 10
    EXPECT_NEAR(output[1], -output[3], 0.5f);  // -5 vs 5
    EXPECT_NEAR(output[2], 0.0f, 0.5f);        // 0
}

// ===== Comparison Tests =====

TEST_F(QuantizationTest, NF4_vs_INT8_Accuracy) {
    // Compare NF4 and INT8 accuracy
    auto input = generate_random_data(512, 0.0f, 1.0f);
    
    QuantizedTensor nf4(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, nf4);
    float nf4_error = quantization::quantization_error(input, nf4);
    
    QuantizedTensor int8(QuantizationType::INT8, {input.size()}, 64);
    quantization::quantize_int8(input, int8);
    float int8_error = quantization::quantization_error(input, int8);
    
    // INT8 should be more accurate
    EXPECT_LT(int8_error, nf4_error) << "NF4: " << nf4_error << ", INT8: " << int8_error;
    
    // But NF4 should still be reasonably accurate
    EXPECT_LT(nf4_error, 0.1f);
}

TEST_F(QuantizationTest, NF4_vs_INT8_MemoryUsage) {
    // Compare memory usage
    size_t num_elements = 1024;
    auto input = generate_random_data(num_elements);
    
    QuantizedTensor nf4(QuantizationType::NF4, {num_elements}, 64);
    quantization::quantize_nf4(input, nf4);
    
    QuantizedTensor int8(QuantizationType::INT8, {num_elements}, 64);
    quantization::quantize_int8(input, int8);
    
    // NF4 should use less memory than INT8
    EXPECT_LT(nf4.memory_bytes(), int8.memory_bytes());
    
    // Roughly 2x less (4 bits vs 8 bits)
    float ratio = static_cast<float>(nf4.memory_bytes()) / int8.memory_bytes();
    EXPECT_LT(ratio, 0.7f) << "Memory ratio: " << ratio;
}

// ===== Double Quantization Tests =====

TEST_F(QuantizationTest, DoubleQuantization_RoundTrip) {
    // Test double quantization round trip
    std::vector<QuantizationBlock> blocks = {
        {1.0f, 0.0f, 64},
        {2.0f, 0.5f, 64},
        {0.5f, -0.5f, 64},
        {1.5f, 0.2f, 64}
    };
    
    std::vector<uint8_t> q_scales, q_zeros;
    float global_scale, global_zero;
    
    double_quantization::quantize_block_params(blocks, q_scales, q_zeros, 
                                                global_scale, global_zero);
    
    std::vector<QuantizationBlock> reconstructed;
    double_quantization::dequantize_block_params(q_scales, q_zeros,
                                                  global_scale, global_zero,
                                                  reconstructed);
    
    // Check size
    EXPECT_EQ(reconstructed.size(), blocks.size());
    
    // Check approximate reconstruction
    for (size_t i = 0; i < blocks.size(); ++i) {
        EXPECT_NEAR(reconstructed[i].scale, blocks[i].scale, 0.1f) << "Block " << i;
        EXPECT_NEAR(reconstructed[i].zero_point, blocks[i].zero_point, 0.1f) << "Block " << i;
    }
}

TEST_F(QuantizationTest, DoubleQuantization_MemorySavings) {
    // Test memory savings from double quantization
    size_t num_blocks = 16;
    std::vector<QuantizationBlock> blocks(num_blocks);
    
    // Without double quantization: 2 * FP32 * num_blocks = 128 bytes
    size_t original_bytes = num_blocks * 2 * sizeof(float);
    
    std::vector<uint8_t> q_scales, q_zeros;
    float global_scale, global_zero;
    
    double_quantization::quantize_block_params(blocks, q_scales, q_zeros,
                                                global_scale, global_zero);
    
    // With double quantization: 2 * uint8 * num_blocks + 2 * FP32 = 40 bytes
    size_t quantized_bytes = q_scales.size() + q_zeros.size() + 2 * sizeof(float);
    
    EXPECT_LT(quantized_bytes, original_bytes / 2) << "Not enough savings";
    
    // Should save roughly 70-75%
    float reduction_ratio = static_cast<float>(quantized_bytes) / original_bytes;
    EXPECT_LT(reduction_ratio, 0.4f) << "Reduction ratio: " << reduction_ratio;
}

// ===== Edge Cases =====

TEST_F(QuantizationTest, EdgeCase_EmptyInput) {
    // Test empty input
    std::vector<float> input;
    QuantizedTensor quantized(QuantizationType::NF4, {0}, 64);
    
    // Should not crash
    EXPECT_NO_THROW(quantization::quantize_nf4(input, quantized));
}

TEST_F(QuantizationTest, EdgeCase_SingleValue) {
    // Test single value
    std::vector<float> input = {0.5f};
    
    QuantizedTensor quantized(QuantizationType::NF4, {1}, 64);
    quantization::quantize_nf4(input, quantized);
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    EXPECT_EQ(output.size(), 1);
    EXPECT_NEAR(output[0], input[0], 0.2f);
}

TEST_F(QuantizationTest, EdgeCase_UniformValues) {
    // Test all same values (edge case for scale computation)
    std::vector<float> input(100, 1.0f);
    
    QuantizedTensor quantized(QuantizationType::NF4, {input.size()}, 64);
    quantization::quantize_nf4(input, quantized);
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    // Should reconstruct accurately
    for (size_t i = 0; i < output.size(); ++i) {
        EXPECT_NEAR(output[i], 1.0f, 0.2f);
    }
}

TEST_F(QuantizationTest, EdgeCase_LargeBlockSize) {
    // Test block size larger than input
    std::vector<float> input = {1.0f, 2.0f, 3.0f};
    
    QuantizedTensor quantized(QuantizationType::NF4, {input.size()}, 1000);
    quantization::quantize_nf4(input, quantized, 1000);
    
    EXPECT_EQ(quantized.num_blocks(), 1);  // Single block
    
    std::vector<float> output;
    quantization::dequantize(quantized, output);
    
    EXPECT_EQ(output.size(), input.size());
}

// ===== Performance Metrics =====

TEST_F(QuantizationTest, Performance_NF4_LargeData) {
    // Test NF4 on large data (performance test)
    size_t num_elements = 1024 * 1024;  // 1M elements = 4MB FP32
    auto input = generate_random_data(num_elements, 0.0f, 1.0f);
    
    QuantizedTensor quantized(QuantizationType::NF4, {num_elements}, 64);
    quantization::quantize_nf4(input, quantized);
    
    // Check memory reduction
    size_t original_bytes = num_elements * sizeof(float);
    size_t quantized_bytes = quantized.memory_bytes();
    
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    
    // Should achieve >75% memory reduction
    EXPECT_GT(reduction, 0.75f) << "Reduction: " << (reduction * 100) << "%";
}

TEST_F(QuantizationTest, Performance_INT8_LargeData) {
    // Test INT8 on large data
    size_t num_elements = 1024 * 1024;  // 1M elements = 4MB FP32
    auto input = generate_random_data(num_elements, 0.0f, 1.0f);
    
    QuantizedTensor quantized(QuantizationType::INT8, {num_elements}, 64);
    quantization::quantize_int8(input, quantized);
    
    // Check memory reduction
    size_t original_bytes = num_elements * sizeof(float);
    size_t quantized_bytes = quantized.memory_bytes();
    
    float reduction = 1.0f - static_cast<float>(quantized_bytes) / original_bytes;
    
    // Should achieve >70% memory reduction
    EXPECT_GT(reduction, 0.70f) << "Reduction: " << (reduction * 100) << "%";
}
