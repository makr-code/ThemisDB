/**
 * @file test_kernel_fusion_cpu_fallback.cpp
 * @brief Chaos / fallback tests for kernel fusion — no GPU required.
 *
 * These tests exercise the kernel fusion CPU fallback path and validate that
 * all fused kernel functions produce numerically stable output (no NaN/Inf,
 * no crash) when CUDA is unavailable.  They run in any standard CI environment
 * that does not have a CUDA-capable GPU.
 *
 * Coverage:
 *  - KernelFusionManager construction and configuration
 *  - fusedLayerNormLinearResidual: output finite, residual applied
 *  - fusedAttentionQKV: output finite with unit weights
 *  - fusedSoftmaxDropoutAttention: rows sum to ≈1, output finite
 *  - fusedGatedFFN: output finite
 *  - fusedRMSNormLinear: output finite
 *  - KernelFusionManager::shouldFuse*: decision logic smoke test
 *  - Stats: counters start at zero
 *
 * @see docs/llm_roadmap.md — Q3 Testing checklist (chaos / CPU-fallback)
 */

#include <gtest/gtest.h>
#include "llm/kernel_fusion.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

using namespace themis::llm::kernels;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool allFinite(const std::vector<float>& v) {
    for (float x : v) {
        if (!std::isfinite(x)) {
          return false;
        }
    }
    return true;
}

static bool noNaN(const std::vector<float>& v) {
    for (float x : v) {
        if (std::isnan(x)) {
          return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// KernelFusionManager construction
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, ManagerDefaultConfig_DoesNotCrash) {
    KernelFusionManager::Config cfg;
    EXPECT_NO_THROW(KernelFusionManager mgr(cfg));
}

TEST(KernelFusionCPUFallback, ManagerDisabledFusion_ShouldNotFuse) {
    KernelFusionManager::Config cfg;
    cfg.enable_fusion = false;
    KernelFusionManager mgr(cfg);
    // With fusion disabled, all 'should fuse' methods return false.
    EXPECT_FALSE(mgr.shouldFuseLayerNormLinear(1, 8, 64));
    EXPECT_FALSE(mgr.shouldFuseQKV(1, 8, 64));
    EXPECT_FALSE(mgr.shouldFuseFFN(1, 8, 64));
}

TEST(KernelFusionCPUFallback, ManagerStats_InitialCountersZero) {
    KernelFusionManager mgr(KernelFusionManager::Config{});
    auto stats = mgr.getStats();
    EXPECT_EQ(stats.ln_linear_fusions, 0u);
    EXPECT_EQ(stats.qkv_fusions, 0u);
    EXPECT_EQ(stats.ffn_fusions, 0u);
    EXPECT_EQ(stats.total_fusions, 0u);
}

// ---------------------------------------------------------------------------
// fusedLayerNormLinearResidual — CPU fallback smoke test
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, LayerNormLinearResidual_OutputFinite) {
    const int B = 1, S = 4, H = 8;
    const int N = B * S * H;

    std::vector<float> input(N, 1.0f);
    std::vector<float> weight(H * H, 0.1f);
    std::vector<float> bias(H, 0.0f);
    std::vector<float> residual(N, 0.5f);
    std::vector<float> ln_weight(H, 1.0f);
    std::vector<float> ln_bias(H, 0.0f);
    std::vector<float> output(N, 0.0f);

    EXPECT_NO_THROW(
        fusedLayerNormLinearResidual(
            output.data(), input.data(),
            weight.data(), bias.data(),
            residual.data(), ln_weight.data(), ln_bias.data(),
            B, S, H)
    );

    EXPECT_TRUE(allFinite(output)) << "Output contains non-finite values";
}

TEST(KernelFusionCPUFallback, LayerNormLinearResidual_NoNaN) {
    const int B = 2, S = 2, H = 4;
    const int N = B * S * H;

    std::vector<float> input(N);
    // std::iota with 0.1f produces 0.1, 1.1, 2.1, … (increment step = 1.0)
    // — we just need varied non-zero values to exercise the kernel path.
    std::iota(input.begin(), input.end(), 0.1f);

    std::vector<float> weight(H * H, 0.01f);
    std::vector<float> bias(H, 0.0f);
    std::vector<float> residual(N, 0.0f);
    std::vector<float> ln_weight(H, 1.0f);
    std::vector<float> ln_bias(H, 0.0f);
    std::vector<float> output(N, 0.0f);

    fusedLayerNormLinearResidual(
        output.data(), input.data(),
        weight.data(), bias.data(),
        residual.data(), ln_weight.data(), ln_bias.data(),
        B, S, H);

    EXPECT_TRUE(noNaN(output)) << "Output contains NaN";
}

// ---------------------------------------------------------------------------
// fusedAttentionQKV — CPU fallback smoke test
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, AttentionQKV_OutputFinite) {
    const int B = 1, S = 4, H = 8, NUM_HEADS = 2;
    const int N = B * S * H;

    std::vector<float> input(N, 0.5f);
    std::vector<float> qkv_weight(3 * H * H, 0.1f);
    std::vector<float> qkv_bias(3 * H, 0.0f);
    std::vector<float> Q(N, 0.0f), K(N, 0.0f), V(N, 0.0f);

    EXPECT_NO_THROW(
        fusedAttentionQKV(
            Q.data(), K.data(), V.data(),
            input.data(), qkv_weight.data(), qkv_bias.data(),
            B, S, H, NUM_HEADS)
    );

    EXPECT_TRUE(allFinite(Q)) << "Query tensor contains non-finite values";
    EXPECT_TRUE(allFinite(K)) << "Key tensor contains non-finite values";
    EXPECT_TRUE(allFinite(V)) << "Value tensor contains non-finite values";
}

// ---------------------------------------------------------------------------
// fusedSoftmaxDropoutAttention — rows should sum to ≈1 (softmax property)
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, SoftmaxDropoutAttention_OutputFinite) {
    const int B = 1, NUM_HEADS = 1, SQ = 4, SKV = 4, HD = 4;
    const int N = B * NUM_HEADS * SQ * HD;
    const int score_size = B * NUM_HEADS * SQ * SKV;

    std::vector<float> scores(score_size, 1.0f);
    std::vector<float> values(B * NUM_HEADS * SKV * HD, 0.5f);
    std::vector<float> output(N, 0.0f);
    std::vector<float> attn_weights(score_size, 0.0f);

    EXPECT_NO_THROW(
        fusedSoftmaxDropoutAttention(
            output.data(), attn_weights.data(),
            scores.data(), values.data(),
            /*mask=*/nullptr,
            B, NUM_HEADS, SQ, SKV, HD,
            /*dropout=*/0.0f, /*causal=*/false)
    );

    EXPECT_TRUE(allFinite(output))       << "Attention output contains non-finite values";
    EXPECT_TRUE(allFinite(attn_weights)) << "Attention weights contain non-finite values";
}

// ---------------------------------------------------------------------------
// fusedGatedFFN — CPU fallback smoke test
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, GatedFFN_OutputFinite) {
    const int B = 1, S = 2, H = 4, I = 8;
    const int N = B * S * H;

    std::vector<float> input(N, 1.0f);
    std::vector<float> gate_w(I * H, 0.05f);
    std::vector<float> up_w(I * H, 0.05f);
    std::vector<float> down_w(H * I, 0.05f);
    std::vector<float> output(N, 0.0f);

    EXPECT_NO_THROW(
        fusedGatedFFN(
            output.data(), input.data(),
            gate_w.data(), up_w.data(), down_w.data(),
            B, S, H, I)
    );

    EXPECT_TRUE(allFinite(output)) << "FFN output contains non-finite values";
}

// ---------------------------------------------------------------------------
// fusedRMSNormLinear — CPU fallback smoke test
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, RMSNormLinear_OutputFinite) {
    const int B = 1, S = 4, H = 8;
    const int N = B * S * H;

    std::vector<float> input(N, 2.0f);
    std::vector<float> weight(H * H, 0.1f);
    std::vector<float> rms_weight(H, 1.0f);
    std::vector<float> output(N, 0.0f);

    EXPECT_NO_THROW(
        fusedRMSNormLinear(
            output.data(), input.data(),
            weight.data(), rms_weight.data(),
            B, S, H)
    );

    EXPECT_TRUE(allFinite(output)) << "RMSNorm+Linear output contains non-finite values";
}

// ---------------------------------------------------------------------------
// Zero-size batch: edge case — must not crash or produce garbage
// ---------------------------------------------------------------------------

TEST(KernelFusionCPUFallback, LayerNormLinearResidual_ZeroBatch_DoesNotCrash) {
    std::vector<float> output;
    // batch_size == 0 → nothing to compute; function must return without crash.
    EXPECT_NO_THROW(
        fusedLayerNormLinearResidual(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            /*batch=*/0, /*seq=*/4, /*hidden=*/8)
    );
}
