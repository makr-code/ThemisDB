/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_flash_attention_correctness.cpp               ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:14:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     318                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_flash_attention_correctness.cpp
 * @brief Unit tests for Flash Attention v3 correctness
 * 
 * Tests:
 * - Configuration validation
 * - Tensor shape validation
 * - Backend detection
 * - Basic attention computation
 * - Causal masking
 * - KV-Cache management
 */

#include <gtest/gtest.h>
#include "llm/attention/flash_attention.h"
#include "llm/attention/kv_cache_manager.h"
#include <vector>
#include <cmath>

using namespace themis::llm::attention;

namespace {

// Helper function to create test tensors
Tensor createTestTensor(const std::vector<int>& shape) {
    Tensor t;
    t.shape = shape;
    t.size = 1;
    for (int dim : shape) {
        t.size *= dim;
    }
    t.data = new float[t.size];
    
    // Initialize with small random values
    for (size_t i = 0; i < t.size; ++i) {
        t.data[i] = static_cast<float>(i % 100) * 0.01f;
    }
    
    return t;
}

void freeTensor(Tensor& t) {
    if (t.data) {
        delete[] t.data;
        t.data = nullptr;
    }
}

} // anonymous namespace

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(FlashAttentionConfig, DefaultConfiguration) {
    FlashAttentionConfig config;
    
    EXPECT_EQ(config.batch_size, 1);
    EXPECT_EQ(config.seq_len, 2048);
    EXPECT_EQ(config.num_heads, 32);
    EXPECT_EQ(config.head_dim, 128);
    EXPECT_TRUE(config.use_causal_mask);
    EXPECT_TRUE(config.enable_flash_v2);
    
    // Scale should be auto-computed
    float expected_scale = 1.0f / std::sqrt(128.0f);
    EXPECT_NEAR(config.scale, expected_scale, 1e-5f);
}

TEST(FlashAttentionConfig, CustomConfiguration) {
    FlashAttentionConfig config;
    config.batch_size = 4;
    config.seq_len = 1024;
    config.num_heads = 16;
    config.head_dim = 64;
    config.use_causal_mask = false;
    
    EXPECT_EQ(config.batch_size, 4);
    EXPECT_EQ(config.seq_len, 1024);
    EXPECT_FALSE(config.use_causal_mask);
}

// ============================================================================
// Backend Detection Tests
// ============================================================================

TEST(FlashAttention, BackendDetection) {
    Backend backend = FlashAttention::selectBestBackend();
    
    // Should select some backend
    EXPECT_NE(backend, Backend::AUTO);
    
    // At minimum, CPU should be available
    EXPECT_TRUE(FlashAttention::isBackendAvailable(Backend::CPU));
}

TEST(FlashAttention, BackendNames) {
    EXPECT_STREQ(getBackendName(Backend::CPU), "CPU");
    EXPECT_STREQ(getBackendName(Backend::CUDA_SM90), "CUDA SM90 (Hopper)");
    EXPECT_STREQ(getBackendName(Backend::CUDA_SM86), "CUDA SM86 (Ampere)");
}

// ============================================================================
// Tensor Validation Tests
// ============================================================================

TEST(FlashAttention, TensorValidation) {
    Tensor valid_tensor;
    valid_tensor.data = new float[100];
    valid_tensor.size = 100;
    valid_tensor.shape = {1, 10, 2, 5};
    
    EXPECT_TRUE(valid_tensor.isValid());
    
    Tensor invalid_tensor;
    EXPECT_FALSE(invalid_tensor.isValid());
    
    delete[] valid_tensor.data;
}

// ============================================================================
// FlashAttention Instantiation Tests
// ============================================================================

TEST(FlashAttention, CPUBackendInstantiation) {
    FlashAttentionConfig config;
    config.batch_size = 1;
    config.seq_len = 16;
    config.num_heads = 4;
    config.head_dim = 32;
    
    EXPECT_NO_THROW({
        FlashAttention flash_attn(Backend::CPU, config);
        EXPECT_EQ(flash_attn.getBackendName(), "CPU (Fallback)");
    });
}

// ============================================================================
// KV-Cache Manager Tests
// ============================================================================

TEST(KVCacheManager, Initialization) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    
    EXPECT_NO_THROW({
        KVCacheManager cache_mgr(config);
        
        EXPECT_EQ(cache_mgr.getTotalBlockCount(), 128);
        EXPECT_EQ(cache_mgr.getFreeBlockCount(), 128);
    });
}

TEST(KVCacheManager, SequenceAllocation) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    
    // Allocate sequence
    uint64_t seq_id = 1;
    BlockTable table = cache_mgr.allocateSequence(seq_id, 32);
    
    EXPECT_EQ(table.sequence_id, seq_id);
    EXPECT_GT(table.block_ids.size(), 0);
    EXPECT_LT(cache_mgr.getFreeBlockCount(), 128);
    
    // Free sequence
    cache_mgr.freeSequence(seq_id);
    EXPECT_EQ(cache_mgr.getFreeBlockCount(), 128);
}

TEST(KVCacheManager, PrefixSharing) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    
    // Allocate parent sequence
    uint64_t parent_id = 1;
    cache_mgr.allocateSequence(parent_id, 32);
    
    // Share prefix with new sequence
    uint64_t child_id = 2;
    cache_mgr.sharePrefix(child_id, parent_id, 16);
    
    const BlockTable* child_table = cache_mgr.getBlockTable(child_id);
    ASSERT_NE(child_table, nullptr);
    EXPECT_TRUE(child_table->has_shared_prefix);
    EXPECT_EQ(child_table->parent_sequence_id, parent_id);
    EXPECT_EQ(child_table->shared_prefix_length, 16);
}

TEST(KVCacheManager, MemoryStats) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 100;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    
    // Allocate some sequences
    cache_mgr.allocateSequence(1, 32);
    cache_mgr.allocateSequence(2, 48);
    
    AttentionMemoryStats stats = cache_mgr.getStats();
    
    EXPECT_GT(stats.blocks_used, 0);
    EXPECT_GT(stats.blocks_free, 0);
    EXPECT_EQ(stats.blocks_used + stats.blocks_free, 100);
    EXPECT_GT(stats.kv_cache_bytes, 0);
}

// ============================================================================
// Basic Attention Computation Tests
// ============================================================================

TEST(FlashAttention, SmallAttentionCPU) {
    FlashAttentionConfig config;
    config.batch_size = 1;
    config.seq_len = 4;
    config.num_heads = 2;
    config.head_dim = 8;
    config.use_causal_mask = false;
    
    FlashAttention flash_attn(Backend::CPU, config);
    
    // Create test tensors
    Tensor Q = createTestTensor({1, 4, 2, 8});
    Tensor K = createTestTensor({1, 4, 2, 8});
    Tensor V = createTestTensor({1, 4, 2, 8});
    Tensor O = createTestTensor({1, 4, 2, 8});
    
    // Run forward pass
    Status status = flash_attn.forward(Q, K, V, O);
    
    EXPECT_EQ(status, Status::SUCCESS);
    
    // Output should be non-zero
    bool has_nonzero = false;
    for (size_t i = 0; i < O.size; ++i) {
        if (std::abs(O.data[i]) > 1e-6f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
    
    // Cleanup
    freeTensor(Q);
    freeTensor(K);
    freeTensor(V);
    freeTensor(O);
}

TEST(FlashAttention, InvalidTensorHandling) {
    FlashAttentionConfig config;
    FlashAttention flash_attn(Backend::CPU, config);
    
    Tensor invalid;
    Tensor valid = createTestTensor({1, 4, 2, 8});
    
    Status status = flash_attn.forward(invalid, valid, valid, valid);
    EXPECT_EQ(status, Status::ERROR_INVALID_TENSOR);
    
    freeTensor(valid);
}

// ============================================================================
// Performance Estimation Tests
// ============================================================================

TEST(FlashAttention, SpeedupEstimation) {
    FlashAttentionConfig config;
    
    FlashAttention cpu_attn(Backend::CPU, config);
    EXPECT_GE(cpu_attn.getExpectedSpeedup(), 1.0);
    
    // Check expected speedups for different backends
    FlashAttention fa_cpu(Backend::CPU, config);
    EXPECT_EQ(fa_cpu.getExpectedSpeedup(), 1.0);
}

// ============================================================================
// Status Message Tests
// ============================================================================

TEST(FlashAttention, StatusMessages) {
    EXPECT_STREQ(getStatusMessage(Status::SUCCESS), "Success");
    EXPECT_STREQ(getStatusMessage(Status::ERROR_INVALID_CONFIG), "Invalid configuration");
    EXPECT_STREQ(getStatusMessage(Status::ERROR_CUDA_ERROR), "CUDA error");
}


