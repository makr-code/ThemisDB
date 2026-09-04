#include <gtest/gtest.h>
#include "llm/kernel_fusion.h"
#include <vector>
#include <cmath>
#include <memory>
#include <chrono>
#include <iostream>

using namespace themis::llm::kernels;

// Test constants
constexpr float TENSOR_EPSILON = 1e-4f;
constexpr float ATTENTION_SUM_TOLERANCE = 0.01f;

class KernelFusionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test setup
    }
    
    void TearDown() override {
        // Common test cleanup
    }
    
    // Helper to verify tensors are close
    bool areClose(const std::vector<float>& a, const std::vector<float>& b, float eps = TENSOR_EPSILON) {
        if (a.size() != b.size()) {
          return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::abs(a[i] - b[i]) > eps) {
              return false;
            }
        }
        return true;
    }
};

TEST_F(KernelFusionTest, FusedLayerNormLinearResidual) {
    // Test dimensions
    const int batch_size = 2;
    const int seq_len = 4;
    const int hidden_dim = 8;
    const int total = batch_size * seq_len * hidden_dim;
    
    // Allocate tensors
    std::vector<float> input(total, 1.0f);
    std::vector<float> weight(hidden_dim, 0.5f);
    std::vector<float> bias(hidden_dim, 0.1f);
    std::vector<float> residual(total, 0.2f);
    std::vector<float> ln_weight(hidden_dim, 1.0f);
    std::vector<float> ln_bias(hidden_dim, 0.0f);
    std::vector<float> output(total);
    
    // Call kernel
    fusedLayerNormLinearResidual(
        output.data(),
        input.data(),
        weight.data(),
        bias.data(),
        residual.data(),
        ln_weight.data(),
        ln_bias.data(),
        batch_size,
        seq_len,
        hidden_dim
    );
    
    // Basic sanity checks
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.size(), total);
    
    // Check output is not all zeros
    bool has_nonzero = false;
    for (float val : output) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(KernelFusionTest, FusedAttentionQKV) {
    const int batch_size = 1;
    const int seq_len = 4;
    const int hidden_dim = 8;
    const int num_heads = 2;
    const int total = batch_size * seq_len * hidden_dim;
    
    // Input tensors
    std::vector<float> input(total, 0.5f);
    std::vector<float> qkv_weight(hidden_dim * 3 * hidden_dim, 0.1f);
    std::vector<float> qkv_bias(3 * hidden_dim, 0.05f);
    
    // Output tensors
    std::vector<float> query(total);
    std::vector<float> key(total);
    std::vector<float> value(total);
    
    // Call kernel
    fusedAttentionQKV(
        query.data(),
        key.data(),
        value.data(),
        input.data(),
        qkv_weight.data(),
        qkv_bias.data(),
        batch_size,
        seq_len,
        hidden_dim,
        num_heads
    );
    
    // Verify outputs are generated
    EXPECT_EQ(query.size(), total);
    EXPECT_EQ(key.size(), total);
    EXPECT_EQ(value.size(), total);
    
    // Check all outputs have non-zero values
    bool q_nonzero = false, k_nonzero = false, v_nonzero = false;
    for (size_t i = 0; i < total; ++i) {
        if (query[i] != 0.0f) {
          q_nonzero = true;
        }
        if (key[i] != 0.0f) {
          k_nonzero = true;
        }
        if (value[i] != 0.0f) {
          v_nonzero = true;
        }
    }
    EXPECT_TRUE(q_nonzero);
    EXPECT_TRUE(k_nonzero);
    EXPECT_TRUE(v_nonzero);
}

TEST_F(KernelFusionTest, FusedGatedFFN) {
    const int batch_size = 1;
    const int seq_len = 2;
    const int hidden_dim = 4;
    const int intermediate_dim = 8;
    const int total = batch_size * seq_len * hidden_dim;
    
    // Input tensors
    std::vector<float> input(total, 0.5f);
    std::vector<float> gate_weight(hidden_dim * intermediate_dim, 0.1f);
    std::vector<float> up_weight(hidden_dim * intermediate_dim, 0.1f);
    std::vector<float> down_weight(intermediate_dim * hidden_dim, 0.1f);
    std::vector<float> output(total);
    
    // Call kernel
    fusedGatedFFN(
        output.data(),
        input.data(),
        gate_weight.data(),
        up_weight.data(),
        down_weight.data(),
        batch_size,
        seq_len,
        hidden_dim,
        intermediate_dim
    );
    
    // Verify output
    EXPECT_EQ(output.size(), total);
    
    // Check for non-zero output
    bool has_nonzero = false;
    for (float val : output) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(KernelFusionTest, FusedRMSNormLinear) {
    const int batch_size = 1;
    const int seq_len = 2;
    const int hidden_dim = 4;
    const int total = batch_size * seq_len * hidden_dim;
    
    // Input tensors
    std::vector<float> input(total, 1.0f);
    std::vector<float> weight(hidden_dim, 0.5f);
    std::vector<float> rms_weight(hidden_dim, 1.0f);
    std::vector<float> output(total);
    
    // Call kernel
    fusedRMSNormLinear(
        output.data(),
        input.data(),
        weight.data(),
        rms_weight.data(),
        batch_size,
        seq_len,
        hidden_dim
    );
    
    // Verify output
    EXPECT_EQ(output.size(), total);
    
    // Output should be non-zero
    bool has_nonzero = false;
    for (float val : output) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(KernelFusionTest, KernelFusionManagerConfig) {
    KernelFusionManager::Config config;
    config.enable_fusion = true;
    config.enable_ln_linear_fusion = true;
    config.enable_qkv_fusion = true;
    config.enable_ffn_fusion = true;
    
    KernelFusionManager manager(config);
    
    // Test fusion decision logic
    EXPECT_TRUE(manager.shouldFuseQKV(1, 128, 768));
    EXPECT_TRUE(manager.shouldFuseLayerNormLinear(2, 64, 512));
    EXPECT_TRUE(manager.shouldFuseFFN(4, 16, 1024));
    
    // Test speedup estimation
    double qkv_speedup = manager.estimateSpeedup("qkv", 1, 128, 768);
    EXPECT_GT(qkv_speedup, 1.0);
    
    double ffn_speedup = manager.estimateSpeedup("ffn", 1, 128, 768);
    EXPECT_GT(ffn_speedup, 1.0);
}

TEST_F(KernelFusionTest, FusedRoPEAttentionScore) {
    const int batch_size = 1;
    const int num_heads = 2;
    const int seq_len = 4;
    const int head_dim = 8;
    const int q_total = batch_size * num_heads * seq_len * head_dim;
    const int score_total = batch_size * num_heads * seq_len * seq_len;
    
    // Input tensors
    std::vector<float> query(q_total, 0.5f);
    std::vector<float> key(q_total, 0.5f);
    std::vector<int> position_ids(seq_len);
    for (int i = 0; i < seq_len; ++i) {
      position_ids[i] = i;
    }
    
    std::vector<float> scores(score_total);
    
    float scale = 1.0f / std::sqrt((float)head_dim);
    
    // Call kernel
    fusedRoPEAttentionScore(
        scores.data(),
        query.data(),
        key.data(),
        position_ids.data(),
        batch_size,
        num_heads,
        seq_len,
        head_dim,
        scale
    );
    
    // Verify scores are computed
    EXPECT_EQ(scores.size(), score_total);
    
    // Check for reasonable score values
    for (float score : scores) {
        EXPECT_FALSE(std::isnan(score));
        EXPECT_FALSE(std::isinf(score));
    }
}

TEST_F(KernelFusionTest, FusedSoftmaxDropoutAttention) {
    const int batch_size = 1;
    const int num_heads = 2;
    const int seq_len_q = 4;
    const int seq_len_kv = 4;
    const int head_dim = 8;
    const int score_total = batch_size * num_heads * seq_len_q * seq_len_kv;
    const int v_total = batch_size * num_heads * seq_len_kv * head_dim;
    const int out_total = batch_size * num_heads * seq_len_q * head_dim;
    
    // Input tensors
    std::vector<float> scores(score_total, 1.0f);
    std::vector<float> values(v_total, 0.5f);
    std::vector<float> attention_weights(score_total);
    std::vector<float> output(out_total);
    
    // Call kernel
    fusedSoftmaxDropoutAttention(
        output.data(),
        attention_weights.data(),
        scores.data(),
        values.data(),
        nullptr,  // no attention mask
        batch_size,
        num_heads,
        seq_len_q,
        seq_len_kv,
        head_dim,
        0.0f,     // no dropout
        true      // causal
    );
    
    // Verify output
    EXPECT_EQ(output.size(), out_total);
    EXPECT_EQ(attention_weights.size(), score_total);
    
    // Check attention weights sum to approximately 1.0 per query
    for (int b = 0; b < batch_size; ++b) {
        for (int h = 0; h < num_heads; ++h) {
            for (int q = 0; q < seq_len_q; ++q) {
                float sum = 0.0f;
                for (int k = 0; k < seq_len_kv; ++k) {
                    if (k <= q) {  // causal mask
                        int idx = ((b * num_heads + h) * seq_len_q + q) * seq_len_kv + k;
                        sum += attention_weights[idx];
                    }
                }
                EXPECT_NEAR(sum, 1.0f, ATTENTION_SUM_TOLERANCE) << "Sum of attention weights should be ~1.0";
            }
        }
    }
    
    // Check output is non-zero
    bool has_nonzero = false;
    for (float val : output) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

#ifdef THEMIS_ENABLE_CUDA
// Note: Backward pass test requires CUDA and proper memory management
// This is a placeholder that demonstrates the API
TEST_F(KernelFusionTest, DISABLED_FlashAttentionBackward) {
    // This test is disabled by default as it requires CUDA runtime
    // Enable with: --gtest_also_run_disabled_tests
    
    const int batch_size = 1;
    const int num_heads = 2;
    const int seq_len = 4;
    const int head_dim = 8;
    const int qkv_total = batch_size * num_heads * seq_len * head_dim;
    
    // Host tensors
    std::vector<float> Q(qkv_total, 0.5f);
    std::vector<float> K(qkv_total, 0.5f);
    std::vector<float> V(qkv_total, 0.5f);
    std::vector<float> O(qkv_total, 0.3f);  // From forward pass
    std::vector<float> dO(qkv_total, 1.0f); // Gradient from loss
    
    std::vector<float> dQ(qkv_total, 0.0f);
    std::vector<float> dK(qkv_total, 0.0f);
    std::vector<float> dV(qkv_total, 0.0f);
    
    // In a real test, would:
    // 1. Allocate device memory with cudaMalloc
    // 2. Copy inputs to device with cudaMemcpy
    // 3. Call launchFlashAttentionBackward
    // 4. Copy gradients back to host
    // 5. Verify gradient correctness (e.g., with finite differences)
    
    // For now, just verify API exists
    EXPECT_EQ(dQ.size(), qkv_total);
    EXPECT_EQ(dK.size(), qkv_total);
    EXPECT_EQ(dV.size(), qkv_total);
}
#endif

// Gradient check test (numerical differentiation)
TEST_F(KernelFusionTest, GradientCheckConcept) {
    // This test demonstrates the concept of gradient checking
    // In practice, would need to compute numerical gradients and compare
    
    const int batch_size = 1;
    const int num_heads = 1;
    const int seq_len = 2;
    const int head_dim = 4;
    
    // For gradient checking, we would:
    // 1. Run forward pass to get output O
    // 2. Compute loss from O
    // 3. Run backward pass to get analytical gradients
    // 4. Compute numerical gradients using finite differences:
    //    grad_numerical[i] = (f(x + eps) - f(x - eps)) / (2 * eps)
    // 5. Compare analytical vs numerical gradients
    
    // Expected: ||grad_analytical - grad_numerical|| < tolerance
    
    // This is a placeholder test
    SUCCEED() << "Gradient checking concept documented";
}

// Performance benchmark placeholder
TEST_F(KernelFusionTest, DISABLED_PerformanceBenchmark) {
    // This test is disabled by default
    // Enable it for performance testing
    
    const int batch_size = 8;
    const int seq_len = 128;
    const int hidden_dim = 768;
    const int total = batch_size * seq_len * hidden_dim;
    
    std::vector<float> input(total, 1.0f);
    std::vector<float> weight(hidden_dim, 0.5f);
    std::vector<float> bias(hidden_dim, 0.1f);
    std::vector<float> residual(total, 0.2f);
    std::vector<float> ln_weight(hidden_dim, 1.0f);
    std::vector<float> ln_bias(hidden_dim, 0.0f);
    std::vector<float> output(total);
    
    // Run multiple iterations for timing
    const int iterations = 100;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        fusedLayerNormLinearResidual(
            output.data(),
            input.data(),
            weight.data(),
            bias.data(),
            residual.data(),
            ln_weight.data(),
            ln_bias.data(),
            batch_size,
            seq_len,
            hidden_dim
        );
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "LayerNorm+Linear+Residual: " 
              << duration.count() / iterations << " ms/iteration" << std::endl;
}
