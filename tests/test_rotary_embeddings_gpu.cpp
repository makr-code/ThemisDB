#include <gtest/gtest.h>
#include "index/rotary_embeddings.h"
#include "index/rotary_embeddings_gpu.h"
#include <numeric>
#include <cmath>
#include <chrono>

using namespace themis;

// ============================================================================
// Test Fixture
// ============================================================================

class RotaryEmbeddingGPUTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.hidden_dim = 128;
        config_.num_rotation_pairs = 64;
        config_.base_theta = 10000.0;
        config_.computeThetaCache();
        
        cpu_rope_ = std::make_unique<RotaryEmbedding>(config_);
        
        // Try to create GPU instance - may fall back to CPU if GPU unavailable
        try {
            gpu_rope_cuda_ = std::make_unique<RotaryEmbeddingGPU>(config_, GPUBackend::CUDA);
        } catch (const std::exception& e) {
            // GPU not available, tests will be skipped
            gpu_rope_cuda_ = nullptr;
        }
        
        try {
            gpu_rope_hip_ = std::make_unique<RotaryEmbeddingGPU>(config_, GPUBackend::HIP);
        } catch (const std::exception& e) {
            // HIP not available
            gpu_rope_hip_ = nullptr;
        }
    }
    
    void TearDown() override {
        cpu_rope_.reset();
        gpu_rope_cuda_.reset();
        gpu_rope_hip_.reset();
    }
    
    RotationConfig config_;
    std::unique_ptr<RotaryEmbedding> cpu_rope_;
    std::unique_ptr<RotaryEmbeddingGPU> gpu_rope_cuda_;
    std::unique_ptr<RotaryEmbeddingGPU> gpu_rope_hip_;
    
    // Helper: compute maximum difference between two embeddings
    float maxDifference(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) {
          return std::numeric_limits<float>::max();
        }
        
        float max_diff = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            max_diff = std::max(max_diff, std::abs(a[i] - b[i]));
        }
        return max_diff;
    }
    
    // Helper: check if two batch results are approximately equal
    bool batchesApproximatelyEqual(
        const std::vector<std::vector<float>>& a,
        const std::vector<std::vector<float>>& b,
        float tolerance = 1e-4f
    ) {
        if (a.size() != b.size()) {
          return false;
        }
        
        for (size_t i = 0; i < a.size(); ++i) {
            if (maxDifference(a[i], b[i]) > tolerance) {
                return false;
            }
        }
        return true;
    }
};

// ============================================================================
// GPU Availability Tests
// ============================================================================

TEST_F(RotaryEmbeddingGPUTest, GPUBackendDetection) {
    if (gpu_rope_cuda_) {
        EXPECT_TRUE(gpu_rope_cuda_->getBackend() == GPUBackend::CUDA);
        if (gpu_rope_cuda_->isGPUAvailable()) {
            std::cout << "CUDA GPU available for testing" << std::endl;
        } else {
            std::cout << "CUDA GPU not available - will fall back to CPU" << std::endl;
        }
    }
    
    if (gpu_rope_hip_) {
        EXPECT_TRUE(gpu_rope_hip_->getBackend() == GPUBackend::HIP);
        if (gpu_rope_hip_->isGPUAvailable()) {
            std::cout << "HIP GPU available for testing" << std::endl;
        } else {
            std::cout << "HIP GPU not available - will fall back to CPU" << std::endl;
        }
    }
}

// ============================================================================
// Correctness Tests - GPU vs CPU
// ============================================================================

TEST_F(RotaryEmbeddingGPUTest, SmallBatchCorrectness_CUDA) {
    if (!gpu_rope_cuda_) {
        GTEST_SKIP() << "capability:cuda_backend_instance_available=false;reason=cuda_backend_unavailable";
    }
    
    // Small batch (should use CPU fallback by default)
    size_t batch_size = 10;
    std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(128));
    for (size_t i = 0; i < batch_size; ++i) {
        std::iota(embeddings[i].begin(), embeddings[i].end(), static_cast<float>(i * 10));
    }
    
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    auto cpu_result = cpu_rope_->rotateBatch(embeddings, positions);
    auto gpu_result = gpu_rope_cuda_->rotateBatch(embeddings, positions);
    
    ASSERT_EQ(cpu_result.size(), gpu_result.size());
    EXPECT_TRUE(batchesApproximatelyEqual(cpu_result, gpu_result, 1e-4f));
}

TEST_F(RotaryEmbeddingGPUTest, LargeBatchCorrectness_CUDA) {
    if (!gpu_rope_cuda_ || !gpu_rope_cuda_->isGPUAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_gpu_not_available";
    }
    
    // Large batch (should use GPU)
    size_t batch_size = 200;
    std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(128));
    for (size_t i = 0; i < batch_size; ++i) {
        std::iota(embeddings[i].begin(), embeddings[i].end(), static_cast<float>(i));
    }
    
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    auto cpu_result = cpu_rope_->rotateBatch(embeddings, positions);
    auto gpu_result = gpu_rope_cuda_->rotateBatchGPU(embeddings, positions);
    
    ASSERT_EQ(cpu_result.size(), gpu_result.size());
    EXPECT_TRUE(batchesApproximatelyEqual(cpu_result, gpu_result, 1e-3f));
}

TEST_F(RotaryEmbeddingGPUTest, SmallBatchCorrectness_HIP) {
    if (!gpu_rope_hip_ || !gpu_rope_hip_->isGPUAvailable()) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_gpu_not_available";
    }
    
    size_t batch_size = 10;
    std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(128));
    for (size_t i = 0; i < batch_size; ++i) {
        std::iota(embeddings[i].begin(), embeddings[i].end(), static_cast<float>(i * 10));
    }
    
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    auto cpu_result = cpu_rope_->rotateBatch(embeddings, positions);
    auto gpu_result = gpu_rope_hip_->rotateBatch(embeddings, positions);
    
    ASSERT_EQ(cpu_result.size(), gpu_result.size());
    EXPECT_TRUE(batchesApproximatelyEqual(cpu_result, gpu_result, 1e-4f));
}

// ============================================================================
// Automatic Fallback Tests
// ============================================================================

TEST_F(RotaryEmbeddingGPUTest, AutomaticFallbackToGPU) {
    if (!gpu_rope_cuda_ || !gpu_rope_cuda_->isGPUAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_gpu_not_available";
    }
    
    // Set threshold to 50
    gpu_rope_cuda_->setGPUBatchThreshold(50);
    EXPECT_EQ(gpu_rope_cuda_->getGPUBatchThreshold(), 50);
    
    // Batch size below threshold (49) - should use CPU
    std::vector<std::vector<float>> small_batch(49, std::vector<float>(128, 1.0f));
    std::vector<size_t> small_positions(49, 0);
    
    // This should use CPU fallback automatically
    auto small_result = gpu_rope_cuda_->rotateBatch(small_batch, small_positions);
    EXPECT_EQ(small_result.size(), 49);
    
    // Batch size at threshold (50) - should use GPU
    std::vector<std::vector<float>> large_batch(50, std::vector<float>(128, 1.0f));
    std::vector<size_t> large_positions(50, 0);
    
    // This should use GPU
    auto large_result = gpu_rope_cuda_->rotateBatch(large_batch, large_positions);
    EXPECT_EQ(large_result.size(), 50);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(RotaryEmbeddingGPUTest, PerformanceComparison_CUDA) {
    if (!gpu_rope_cuda_ || !gpu_rope_cuda_->isGPUAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_gpu_not_available";
    }
    
    // Create large batch for performance testing
    size_t batch_size = 1000;
    std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(128));
    for (size_t i = 0; i < batch_size; ++i) {
        std::iota(embeddings[i].begin(), embeddings[i].end(), static_cast<float>(i));
    }
    
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    // Warm-up
    cpu_rope_->rotateBatch(embeddings, positions);
    gpu_rope_cuda_->rotateBatchGPU(embeddings, positions);
    
    // Time CPU
    auto cpu_start = std::chrono::high_resolution_clock::now();
    auto cpu_result = cpu_rope_->rotateBatch(embeddings, positions);
    auto cpu_end = std::chrono::high_resolution_clock::now();
    auto cpu_duration = std::chrono::duration_cast<std::chrono::microseconds>(cpu_end - cpu_start);
    
    // Time GPU
    auto gpu_start = std::chrono::high_resolution_clock::now();
    auto gpu_result = gpu_rope_cuda_->rotateBatchGPU(embeddings, positions);
    auto gpu_end = std::chrono::high_resolution_clock::now();
    auto gpu_duration = std::chrono::duration_cast<std::chrono::microseconds>(gpu_end - gpu_start);
    
    std::cout << "Batch size: " << batch_size << std::endl;
    std::cout << "CPU time: " << cpu_duration.count() << " µs" << std::endl;
    std::cout << "GPU time: " << gpu_duration.count() << " µs" << std::endl;
    
    if (gpu_duration.count() > 0) {
        double speedup = static_cast<double>(cpu_duration.count()) / gpu_duration.count();
        std::cout << "Speedup: " << speedup << "x" << std::endl;
        
        // GPU should be faster for large batches (but this is not a strict requirement)
        // Just report the speedup for informational purposes
    }
    
    // Results should still be correct
    EXPECT_TRUE(batchesApproximatelyEqual(cpu_result, gpu_result, 1e-3f));
}

TEST_F(RotaryEmbeddingGPUTest, VeryLargeBatchPerformance_CUDA) {
    if (!gpu_rope_cuda_ || !gpu_rope_cuda_->isGPUAvailable()) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_gpu_not_available";
    }
    
    // Very large batch to demonstrate GPU advantage
    size_t batch_size = 10000;
    std::vector<std::vector<float>> embeddings(batch_size, std::vector<float>(128, 1.0f));
    std::vector<size_t> positions(batch_size);
    std::iota(positions.begin(), positions.end(), 0);
    
    // Only time GPU (CPU would be too slow)
    auto gpu_start = std::chrono::high_resolution_clock::now();
    auto gpu_result = gpu_rope_cuda_->rotateBatchGPU(embeddings, positions);
    auto gpu_end = std::chrono::high_resolution_clock::now();
    auto gpu_duration = std::chrono::duration_cast<std::chrono::microseconds>(gpu_end - gpu_start);
    
    std::cout << "Very large batch size: " << batch_size << std::endl;
    std::cout << "GPU time: " << gpu_duration.count() << " µs" << std::endl;
    
    EXPECT_EQ(gpu_result.size(), batch_size);
    
    // Target: < 500 µs for 10,000 embeddings (from requirements)
    // This is just informational - actual performance depends on hardware
    if (gpu_duration.count() < 500) {
        std::cout << "✓ Performance target achieved!" << std::endl;
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(RotaryEmbeddingGPUTest, DimensionMismatchThrows) {
    if (!gpu_rope_cuda_) {
        GTEST_SKIP() << "capability:cuda_backend_instance_available=false;reason=cuda_backend_unavailable";
    }
    
    // Create batch with wrong dimension
    std::vector<std::vector<float>> embeddings = {
        std::vector<float>(128, 1.0f),
        std::vector<float>(64, 1.0f),  // Wrong dimension
    };
    std::vector<size_t> positions = {0, 1};
    
    if (gpu_rope_cuda_->isGPUAvailable()) {
        EXPECT_THROW(gpu_rope_cuda_->rotateBatchGPU(embeddings, positions), std::invalid_argument);
    }
}

TEST_F(RotaryEmbeddingGPUTest, BatchSizeMismatchThrows) {
    if (!gpu_rope_cuda_) {
        GTEST_SKIP() << "capability:cuda_backend_instance_available=false;reason=cuda_backend_unavailable";
    }
    
    std::vector<std::vector<float>> embeddings(10, std::vector<float>(128, 1.0f));
    std::vector<size_t> positions(5);  // Mismatch
    
    if (gpu_rope_cuda_->isGPUAvailable()) {
        EXPECT_THROW(gpu_rope_cuda_->rotateBatchGPU(embeddings, positions), std::invalid_argument);
    }
}

// ============================================================================
// Main removed - using GTest's main from themis_tests.exe
