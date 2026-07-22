/**
 * @file test_nvfp4_kv_quantization.cpp
 * @brief P2-D06 Phase 2 Tests: NVFP4 KV-Cache Quantization and Accuracy Validation
 *
 * This test suite verifies NVFP4 KV-cache quantization accuracy against FP16 baseline
 * and validates compression efficiency across three quantization types (FP16, INT8, NVFP4).
 *
 * @note Phase 2 Acceptance Gates:
 * - P2-GATE-01: NVFP4 accuracy delta ≤ 1% vs FP16
 * - P2-GATE-02: Round-trip numeric consistency
 * - P2-GATE-04: VRAM footprint NVFP4 ≤ 55% vs FP16
 *
 * @author Copilot Coding Agent
 * @date 2026-07-22
 * @version 1.0.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "llm/paged_kv_cache.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace themis {
namespace llm {
namespace test {

using ::testing::FloatNear;
using ::testing::DoubleNear;

/**
 * @brief Test fixture for NVFP4 KV quantization tests
 */
class NVFP4QuantizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create dummy block manager (minimal setup for testing)
        // In practice, would use real PagedBlockManager
        config_.block_size = 16;
        config_.num_blocks = 4096;
        config_.num_layers = 32;
        config_.head_dim = 128;
        config_.num_kv_heads = 8;
        config_.enable_prefix_caching = true;
    }

    /// Generate random KV cache data with realistic distribution
    std::vector<float> generateRandomKVData(size_t size, uint32_t seed = 42) {
        std::mt19937 rng(seed);
        std::normal_distribution<float> dist(0.0f, 0.1f);  // Small scale like attention values
        
        std::vector<float> data(size);
        for (auto& v : data) {
            v = dist(rng);
        }
        return data;
    }

    /// Generate typical KV cache patterns (sparsity, correlations)
    std::vector<float> generateRealisticKVData(size_t size) {
        std::vector<float> data(size);
        
        // Simulate attention scores: mostly small values with occasional large values
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> sparse_dist(0.0f, 1.0f);
        
        for (size_t i = 0; i < size; ++i) {
            float u = sparse_dist(rng);
            if (u < 0.85f) {
                // Small values (85% of data)
                data[i] = (sparse_dist(rng) - 0.5f) * 0.05f;
            } else {
                // Occasional larger values (15% of data)
                data[i] = (sparse_dist(rng) - 0.5f) * 0.5f;
            }
        }
        return data;
    }

    /// Calculate MAPE (Mean Absolute Percentage Error)
    float calculateMAPE(const std::vector<float>& original, const std::vector<float>& reconstructed) {
        if (original.size() != reconstructed.size()) {
            return std::numeric_limits<float>::max();
        }
        
        double sum_error = 0.0;
        size_t non_zero_count = 0;
        
        for (size_t i = 0; i < original.size(); ++i) {
            float abs_original = std::abs(original[i]);
            if (abs_original > 1e-6f) {
                float error = std::abs(original[i] - reconstructed[i]) / abs_original;
                sum_error += error;
                non_zero_count++;
            }
        }
        
        if (non_zero_count == 0) return 0.0f;
        return static_cast<float>(sum_error / non_zero_count);
    }

    /// Calculate MSE (Mean Squared Error)
    float calculateMSE(const std::vector<float>& original, const std::vector<float>& reconstructed) {
        if (original.size() != reconstructed.size()) {
            return std::numeric_limits<float>::max();
        }
        
        double sum_squared_error = 0.0;
        for (size_t i = 0; i < original.size(); ++i) {
            float error = original[i] - reconstructed[i];
            sum_squared_error += error * error;
        }
        
        return static_cast<float>(sum_squared_error / original.size());
    }

    PagedKVCache::Config config_;
};

/**
 * @test NVFP4 Quantization: Round-trip accuracy with random data
 *
 * Verifies that quantizing and dequantizing KV data preserves values within tolerance.
 * Gate P2-GATE-01: Accuracy delta ≤ 1% vs FP16 baseline
 */
TEST_F(NVFP4QuantizationTest, RoundTripAccuracyRandomData) {
    // Create cache instance (nullptr for block manager is acceptable for quantization testing)
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    // Generate test data
    std::vector<float> original = generateRandomKVData(1024);
    
    // Quantize to NVFP4
    auto quantized_data = cache.quantizeKVData(original, PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_FALSE(quantized_data.empty());
    
    // Verify compression: NVFP4 should achieve ~50% of original size (2 values per byte)
    float compression_ratio = static_cast<float>(quantized_data.size()) / (original.size() * sizeof(float));
    EXPECT_LT(compression_ratio, 0.1f);  // Should be under 10% (highly compressed)
    
    // Dequantize back to float
    auto reconstructed = cache.dequantizeKVData(quantized_data, PagedKVCache::KVQuantizationType::NVFP4);
    
    // Verify size match
    EXPECT_EQ(original.size(), reconstructed.size());
    
    // Verify accuracy: MAPE should be ≤ 5% (NVFP4 expected ~5% error)
    float mape = calculateMAPE(original, reconstructed);
    EXPECT_LT(mape, 0.05f) << "MAPE " << mape << " exceeds 5% tolerance";
}

/**
 * @test NVFP4 vs FP16 accuracy comparison
 *
 * Compares accuracy of NVFP4 quantization against FP16 baseline.
 * Gate P2-GATE-01: NVFP4 accuracy delta ≤ 1% vs FP16
 */
TEST_F(NVFP4QuantizationTest, NVFP4VsFP16AccuracyDelta) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    // Test with realistic attention data
    std::vector<float> original = generateRealisticKVData(512);
    
    // Quantize to FP16
    auto fp16_quantized = cache.quantizeKVData(original, PagedKVCache::KVQuantizationType::FP16);
    auto fp16_reconstructed = cache.dequantizeKVData(fp16_quantized, PagedKVCache::KVQuantizationType::FP16);
    float fp16_mape = calculateMAPE(original, fp16_reconstructed);
    
    // Quantize to NVFP4
    auto nvfp4_quantized = cache.quantizeKVData(original, PagedKVCache::KVQuantizationType::NVFP4);
    auto nvfp4_reconstructed = cache.dequantizeKVData(nvfp4_quantized, PagedKVCache::KVQuantizationType::NVFP4);
    float nvfp4_mape = calculateMAPE(original, nvfp4_reconstructed);
    
    // Verify delta ≤ 1% (gate requirement)
    float accuracy_delta = nvfp4_mape - fp16_mape;
    EXPECT_LT(accuracy_delta, 0.01f) << "NVFP4 delta " << accuracy_delta << " exceeds 1% vs FP16";
    
    // Also verify NVFP4 MAPE is reasonable (~5% expected vs ~0.1% for FP16)
    EXPECT_LT(nvfp4_mape, 0.06f);
}

/**
 * @test INT8 quantization: Per-channel quantization accuracy
 *
 * Verifies INT8 quantization with scale/zero-point parameters.
 */
TEST_F(NVFP4QuantizationTest, INT8QuantizationAccuracy) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    std::vector<float> original = generateRandomKVData(256);
    
    // Quantize to INT8
    auto int8_quantized = cache.quantizeKVData(original, PagedKVCache::KVQuantizationType::INT8);
    EXPECT_FALSE(int8_quantized.empty());
    
    // INT8 should be more efficient than NVFP4 (1 byte per value + 8 bytes metadata)
    float compression_ratio = static_cast<float>(int8_quantized.size()) / (original.size() * sizeof(float));
    EXPECT_LT(compression_ratio, 0.35f);  // Should be < 35% (highly compressed)
    
    // Dequantize
    auto int8_reconstructed = cache.dequantizeKVData(int8_quantized, PagedKVCache::KVQuantizationType::INT8);
    EXPECT_EQ(original.size(), int8_reconstructed.size());
    
    // Verify accuracy: INT8 should achieve ~2% MAPE
    float mape = calculateMAPE(original, int8_reconstructed);
    EXPECT_LT(mape, 0.03f) << "INT8 MAPE " << mape << " exceeds 3% tolerance";
}

/**
 * @test Compression factor verification
 *
 * Verifies that compression factors match theoretical expectations.
 */
TEST_F(NVFP4QuantizationTest, CompressionFactorVerification) {
    // FP16: 50% compression (4 bytes -> 2 bytes)
    float fp16_factor = PagedKVCache::getCompressionFactor(PagedKVCache::KVQuantizationType::FP16);
    EXPECT_FLOAT_EQ(fp16_factor, 0.5f);
    
    // INT8: 75% compression (4 bytes -> 1 byte, but metadata overhead)
    float int8_factor = PagedKVCache::getCompressionFactor(PagedKVCache::KVQuantizationType::INT8);
    EXPECT_LT(int8_factor, 0.75f + 0.01f);  // Allow small margin for implementation
    EXPECT_GT(int8_factor, 0.2f);
    
    // NVFP4: 87.5% compression (4 bytes -> 0.5 bytes, gate P2-GATE-04 requires ≤ 55% vs FP16)
    float nvfp4_factor = PagedKVCache::getCompressionFactor(PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_LT(nvfp4_factor, 0.5f * 0.55f);  // Should be ≤ 55% of FP16 footprint
}

/**
 * @test Expected accuracy values match phase gates
 *
 * Verifies that expected accuracy targets align with Phase 2 requirements.
 */
TEST_F(NVFP4QuantizationTest, ExpectedAccuracyTargets) {
    // FP16: 99.9% accuracy
    float fp16_acc = PagedKVCache::getExpectedAccuracy(PagedKVCache::KVQuantizationType::FP16);
    EXPECT_NEAR(fp16_acc, 0.999f, 0.001f);
    
    // INT8: 98% accuracy
    float int8_acc = PagedKVCache::getExpectedAccuracy(PagedKVCache::KVQuantizationType::INT8);
    EXPECT_NEAR(int8_acc, 0.98f, 0.01f);
    
    // NVFP4: 99% accuracy (~99% per requirements)
    float nvfp4_acc = PagedKVCache::getExpectedAccuracy(PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_NEAR(nvfp4_acc, 0.99f, 0.01f);
}

/**
 * @test NVFP4 quantization with zero and extreme values
 *
 * Edge case testing: verifies quantization handles zero, negative, and extreme values.
 */
TEST_F(NVFP4QuantizationTest, NVFP4EdgeCases) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    // Test data with edge cases
    std::vector<float> edge_cases = {
        0.0f,
        1.0f, -1.0f,
        0.1f, -0.1f,
        100.0f, -100.0f,
        0.001f, -0.001f,
        std::numeric_limits<float>::epsilon(),
        -std::numeric_limits<float>::epsilon()
    };
    
    // Quantize
    auto quantized = cache.quantizeKVData(edge_cases, PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_FALSE(quantized.empty());
    
    // Dequantize
    auto reconstructed = cache.dequantizeKVData(quantized, PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_EQ(edge_cases.size(), reconstructed.size());
    
    // Verify zeros are preserved
    EXPECT_FLOAT_EQ(reconstructed[0], 0.0f);
    
    // Verify rough magnitude preservation for other values
    for (size_t i = 1; i < edge_cases.size(); ++i) {
        float original = edge_cases[i];
        float recon = reconstructed[i];
        
        if (std::abs(original) > 0.01f) {
            float error_ratio = std::abs(original - recon) / std::abs(original);
            EXPECT_LT(error_ratio, 0.1f) << "Large error at index " << i << ": " << original << " vs " << recon;
        }
    }
}

/**
 * @test Batch quantization with multiple sequences
 *
 * Tests quantization performance on batched KV data (multiple sequences in parallel).
 */
TEST_F(NVFP4QuantizationTest, BatchQuantizationPerformance) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    // Simulate batch of 8 sequences, each with 256 tokens worth of KV data
    const size_t batch_size = 8;
    const size_t tokens_per_sequence = 256;
    
    std::vector<std::vector<float>> batches;
    std::vector<std::vector<uint8_t>> quantized_batches;
    
    // Quantize all sequences
    for (size_t i = 0; i < batch_size; ++i) {
        auto batch_data = generateRandomKVData(tokens_per_sequence * 128);  // 128 = 2*kv_heads*head_dim
        batches.push_back(batch_data);
        
        auto quantized = cache.quantizeKVData(batch_data, PagedKVCache::KVQuantizationType::NVFP4);
        quantized_batches.push_back(quantized);
    }
    
    // Verify compression across all batches
    size_t total_original = batch_size * tokens_per_sequence * 128 * sizeof(float);
    size_t total_quantized = 0;
    for (const auto& q : quantized_batches) {
        total_quantized += q.size();
    }
    
    float overall_compression = static_cast<float>(total_quantized) / total_original;
    EXPECT_LT(overall_compression, 0.1f) << "Batch compression " << overall_compression << " too high";
}

/**
 * @test NVFP4 quantization numeric stability
 *
 * Verifies that repeated quantize/dequantize cycles maintain stability.
 */
TEST_F(NVFP4QuantizationTest, NumericStabilityMultipleCycles) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    std::vector<float> original = generateRandomKVData(512);
    std::vector<float> current = original;
    
    // Run 3 quantize/dequantize cycles
    for (int cycle = 0; cycle < 3; ++cycle) {
        auto quantized = cache.quantizeKVData(current, PagedKVCache::KVQuantizationType::NVFP4);
        current = cache.dequantizeKVData(quantized, PagedKVCache::KVQuantizationType::NVFP4);
    }
    
    // Final reconstruction should still be reasonable (MAPE < 15% after 3 cycles)
    float final_mape = calculateMAPE(original, current);
    EXPECT_LT(final_mape, 0.15f) << "Numeric stability degraded after cycles: " << final_mape;
}

/**
 * @test Empty data handling
 *
 * Verifies quantization gracefully handles empty inputs.
 */
TEST_F(NVFP4QuantizationTest, EmptyDataHandling) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    std::vector<float> empty;
    
    // All quantization types should handle empty data
    auto nvfp4_result = cache.quantizeKVData(empty, PagedKVCache::KVQuantizationType::NVFP4);
    EXPECT_TRUE(nvfp4_result.empty());
    
    auto fp16_result = cache.quantizeKVData(empty, PagedKVCache::KVQuantizationType::FP16);
    EXPECT_TRUE(fp16_result.empty());
    
    auto int8_result = cache.quantizeKVData(empty, PagedKVCache::KVQuantizationType::INT8);
    EXPECT_TRUE(int8_result.empty());
}

/**
 * @test Memory footprint validation for P2-GATE-04
 *
 * Validates VRAM footprint reduction: NVFP4 ≤ 55% of FP16 footprint.
 * This is the gate requirement from the Phase 2 specification.
 */
TEST_F(NVFP4QuantizationTest, MemoryFootprintGateP2GATE04) {
    std::shared_ptr<PagedBlockManager> dummy_manager;
    PagedKVCache cache(config_, dummy_manager);
    
    // Simulate large KV cache: 32 layers, 8 heads, 128 dims, 2048 tokens
    const size_t num_layers = 32;
    const size_t num_tokens = 2048;
    const size_t kv_size = 2 * 8 * 128;  // 2*kv_heads*head_dim per token
    const size_t total_values = num_layers * num_tokens * kv_size;
    
    // Generate data
    std::vector<float> kv_data = generateRandomKVData(total_values);
    
    // Get footprints
    auto fp16_quantized = cache.quantizeKVData(kv_data, PagedKVCache::KVQuantizationType::FP16);
    auto nvfp4_quantized = cache.quantizeKVData(kv_data, PagedKVCache::KVQuantizationType::NVFP4);
    
    size_t fp16_footprint = fp16_quantized.size();
    size_t nvfp4_footprint = nvfp4_quantized.size();
    
    float nvfp4_to_fp16_ratio = static_cast<float>(nvfp4_footprint) / fp16_footprint;
    
    // Gate P2-GATE-04: VRAM footprint NVFP4 ≤ 55% of FP16
    EXPECT_LT(nvfp4_to_fp16_ratio, 0.55f) << "NVFP4 " << nvfp4_footprint 
        << " is " << nvfp4_to_fp16_ratio * 100 << "% of FP16 " << fp16_footprint;
}

} // namespace test
} // namespace llm
} // namespace themis
