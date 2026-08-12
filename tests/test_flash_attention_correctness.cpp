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

// ============================================================================
// Attention Correctness Tests (CPU)
//
// These tests verify actual scaled dot-product attention numerics rather than
// just checking that the output is non-zero.  The reference is computed inline
// with the standard formula:
//   S[i,j] = scale * dot(Q[i,:], K[j,:])
//   A[i,j] = softmax_j(S[i,:])   (with optional causal mask)
//   O[i,:] = sum_j A[i,j] * V[j,:]
// ============================================================================

namespace {

// Compute reference output for a single (batch=1, head=1) slice.
// Q, K, V: shape [seq_len, head_dim]
// Returns O: shape [seq_len, head_dim]
std::vector<float> referenceAttention(
    const std::vector<float>& Q,
    const std::vector<float>& K,
    const std::vector<float>& V,
    int seq_len, int head_dim,
    float scale, bool causal)
{
    std::vector<float> O(static_cast<size_t>(seq_len * head_dim), 0.0f);
    std::vector<float> scores(static_cast<size_t>(seq_len));

    for (int i = 0; i < seq_len; ++i) {
        // Dot-products
        float max_s = -std::numeric_limits<float>::infinity();
        for (int j = 0; j < seq_len; ++j) {
            if (causal && j > i) {
                scores[j] = -std::numeric_limits<float>::infinity();
                continue;
            }
            float dot = 0.0f;
            for (int d = 0; d < head_dim; ++d) {
                dot += Q[i * head_dim + d] * K[j * head_dim + d];
            }
            scores[j] = scale * dot;
            if (scores[j] > max_s) max_s = scores[j];
        }
        // Softmax
        float sum = 0.0f;
        for (int j = 0; j < seq_len; ++j) {
            scores[j] = std::exp(scores[j] - max_s);
            sum += scores[j];
        }
        if (sum > 0.0f) {
            for (int j = 0; j < seq_len; ++j) scores[j] /= sum;
        }
        // Weighted sum of V
        for (int d = 0; d < head_dim; ++d) {
            float out = 0.0f;
            for (int j = 0; j < seq_len; ++j) {
                out += scores[j] * V[j * head_dim + d];
            }
            O[i * head_dim + d] = out;
        }
    }
    return O;
}

// Helper: fill a Tensor from a flat vector (caller owns data pointer).
void fillTensor(Tensor& t, std::vector<float>& buf, const std::vector<int>& shape) {
    t.shape = shape;
    t.size  = buf.size();
    t.data  = buf.data();
}

} // anonymous namespace

TEST(FlashAttentionCPU, ForwardMatchesReference_NoCausal) {
    // batch=1, seq=4, heads=1, head_dim=4; no causal mask
    const int B = 1, S = 4, H = 1, D = 4;
    FlashAttentionConfig cfg;
    cfg.batch_size    = B;
    cfg.seq_len       = S;
    cfg.num_heads     = H;
    cfg.head_dim      = D;
    cfg.scale         = 1.0f / std::sqrt(static_cast<float>(D));
    cfg.use_causal_mask = false;

    // Deterministic input values
    std::vector<float> qbuf = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    std::vector<float> kbuf = qbuf;
    std::vector<float> vbuf = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9,10,11,12,
       13,14,15,16
    };
    std::vector<float> obuf(static_cast<size_t>(B * S * H * D), 0.0f);

    Tensor Q, K, V, O;
    fillTensor(Q, qbuf, {B, S, H, D});
    fillTensor(K, kbuf, {B, S, H, D});
    fillTensor(V, vbuf, {B, S, H, D});
    fillTensor(O, obuf, {B, S, H, D});

    FlashAttention fa(Backend::CPU, cfg);
    ASSERT_EQ(fa.forward(Q, K, V, O), Status::SUCCESS);

    // Compute reference
    std::vector<float> ref = referenceAttention(qbuf, kbuf, vbuf, S, D, cfg.scale, false);

    for (int i = 0; i < S * D; ++i) {
        EXPECT_NEAR(O.data[i], ref[i], 1e-5f)
            << "Mismatch at position " << i;
    }
}

TEST(FlashAttentionCPU, ForwardMatchesReference_CausalMask) {
    const int B = 1, S = 4, H = 1, D = 4;
    FlashAttentionConfig cfg;
    cfg.batch_size    = B;
    cfg.seq_len       = S;
    cfg.num_heads     = H;
    cfg.head_dim      = D;
    cfg.scale         = 1.0f / std::sqrt(static_cast<float>(D));
    cfg.use_causal_mask = true;

    std::vector<float> qbuf = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    std::vector<float> kbuf = qbuf;
    std::vector<float> vbuf = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9,10,11,12,
       13,14,15,16
    };
    std::vector<float> obuf(static_cast<size_t>(B * S * H * D), 0.0f);

    Tensor Q, K, V, O;
    fillTensor(Q, qbuf, {B, S, H, D});
    fillTensor(K, kbuf, {B, S, H, D});
    fillTensor(V, vbuf, {B, S, H, D});
    fillTensor(O, obuf, {B, S, H, D});

    FlashAttention fa(Backend::CPU, cfg);
    ASSERT_EQ(fa.forward(Q, K, V, O), Status::SUCCESS);

    std::vector<float> ref = referenceAttention(qbuf, kbuf, vbuf, S, D, cfg.scale, true);

    for (int i = 0; i < S * D; ++i) {
        EXPECT_NEAR(O.data[i], ref[i], 1e-5f)
            << "Mismatch at position " << i;
    }

    // With causal mask, position 0 attends only to itself => O[0] == V[0]
    for (int d = 0; d < D; ++d) {
        EXPECT_NEAR(O.data[d], vbuf[d], 1e-5f)
            << "Causal: position 0 should equal V[0] at dim " << d;
    }
}

TEST(FlashAttentionCPU, ForwardUniformQKGivesUniformAttention) {
    // When all Q and K rows are identical, softmax scores are uniform,
    // so O[i] should equal the mean of all V rows.
    const int B = 1, S = 3, H = 1, D = 2;
    FlashAttentionConfig cfg;
    cfg.batch_size    = B;
    cfg.seq_len       = S;
    cfg.num_heads     = H;
    cfg.head_dim      = D;
    cfg.scale         = 1.0f;
    cfg.use_causal_mask = false;

    // All query/key rows identical
    std::vector<float> qbuf = {1, 0,  1, 0,  1, 0};
    std::vector<float> kbuf = qbuf;
    std::vector<float> vbuf = {1, 4,  2, 5,  3, 6};  // rows: [1,4],[2,5],[3,6]
    std::vector<float> obuf(static_cast<size_t>(B * S * H * D), 0.0f);

    Tensor Q, K, V, O;
    fillTensor(Q, qbuf, {B, S, H, D});
    fillTensor(K, kbuf, {B, S, H, D});
    fillTensor(V, vbuf, {B, S, H, D});
    fillTensor(O, obuf, {B, S, H, D});

    FlashAttention fa(Backend::CPU, cfg);
    ASSERT_EQ(fa.forward(Q, K, V, O), Status::SUCCESS);

    // Expected output: mean of V rows = [2, 5] (for each query position)
    const float expected_d0 = (1.0f + 2.0f + 3.0f) / 3.0f;
    const float expected_d1 = (4.0f + 5.0f + 6.0f) / 3.0f;

    for (int i = 0; i < S; ++i) {
        EXPECT_NEAR(O.data[i * D + 0], expected_d0, 1e-5f)
            << "Uniform attention: dim 0 mismatch at query " << i;
        EXPECT_NEAR(O.data[i * D + 1], expected_d1, 1e-5f)
            << "Uniform attention: dim 1 mismatch at query " << i;
    }
}

TEST(FlashAttentionCPU, BackwardReturnsSuccessAndWritesGradients) {
    const int B = 1, S = 4, H = 1, D = 4;
    FlashAttentionConfig cfg;
    cfg.batch_size    = B;
    cfg.seq_len       = S;
    cfg.num_heads     = H;
    cfg.head_dim      = D;
    cfg.scale         = 1.0f;
    cfg.use_causal_mask = false;

    const size_t total = static_cast<size_t>(B * S * H * D);
    std::vector<float> do_buf(total, 1.0f);
    std::vector<float> dq_buf(total, 0.0f);
    std::vector<float> dk_buf(total, 0.0f);
    std::vector<float> dv_buf(total, 0.0f);

    Tensor dO, dQ, dK, dV;
    fillTensor(dO, do_buf, {B, S, H, D});
    fillTensor(dQ, dq_buf, {B, S, H, D});
    fillTensor(dK, dk_buf, {B, S, H, D});
    fillTensor(dV, dv_buf, {B, S, H, D});

    FlashAttention fa(Backend::CPU, cfg);
    ASSERT_EQ(fa.backward(dO, dQ, dK, dV), Status::SUCCESS);

    // All gradient tensors must have received non-zero updates.
    bool dq_nonzero = false, dk_nonzero = false, dv_nonzero = false;
    for (size_t i = 0; i < total; ++i) {
        if (std::abs(dQ.data[i]) > 1e-7f) dq_nonzero = true;
        if (std::abs(dK.data[i]) > 1e-7f) dk_nonzero = true;
        if (std::abs(dV.data[i]) > 1e-7f) dv_nonzero = true;
    }
    EXPECT_TRUE(dq_nonzero) << "dQ should have non-zero gradients";
    EXPECT_TRUE(dk_nonzero) << "dK should have non-zero gradients";
    EXPECT_TRUE(dv_nonzero) << "dV should have non-zero gradients";
}

TEST(FlashAttentionCPU, BackwardInvalidTensorReturnsError) {
    FlashAttentionConfig cfg;
    FlashAttention fa(Backend::CPU, cfg);

    Tensor invalid;
    const size_t sz = 64;
    std::vector<float> buf(sz, 0.0f);
    Tensor valid;
    valid.data = buf.data(); valid.size = sz;
    valid.shape = {1, 1, 1, static_cast<int>(sz)};

    EXPECT_EQ(fa.backward(invalid, valid, valid, valid), Status::ERROR_INVALID_TENSOR);
}

// ─── Batch 36 input_validation regression tests ─────────────────────────────

// forward() with batch_size == 0 must return ERROR_INVALID_CONFIG (not crash or
// silently allocate a zero-element vector that later confuses callers).
TEST(FlashAttentionCPU, ForwardZeroBatchReturnsInvalidConfig) {
    FlashAttentionConfig cfg;
    cfg.batch_size  = 0;  // invalid
    cfg.seq_len     = 8;
    cfg.num_heads   = 2;
    cfg.head_dim    = 16;
    FlashAttention fa(Backend::CPU, cfg);

    const size_t sz = 8 * 2 * 16;
    std::vector<float> buf(sz, 1.0f);
    Tensor q, k, v, out;
    q.data = buf.data(); q.size = sz; q.shape = {0, 8, 2, 16};
    k = q; v = q; out = q;

    EXPECT_EQ(fa.forward(q, k, v, out), Status::ERROR_INVALID_CONFIG);
}

// forward() with seq_len == 0 must also return ERROR_INVALID_CONFIG.
TEST(FlashAttentionCPU, ForwardZeroSeqLenReturnsInvalidConfig) {
    FlashAttentionConfig cfg;
    cfg.batch_size  = 1;
    cfg.seq_len     = 0;  // invalid
    cfg.num_heads   = 2;
    cfg.head_dim    = 16;
    FlashAttention fa(Backend::CPU, cfg);

    const size_t sz = 64;
    std::vector<float> buf(sz, 1.0f);
    Tensor q, k, v, out;
    q.data = buf.data(); q.size = sz; q.shape = {1, 0, 2, 16};
    k = q; v = q; out = q;

    EXPECT_EQ(fa.forward(q, k, v, out), Status::ERROR_INVALID_CONFIG);
}

// backward() with num_heads == 0 must return ERROR_INVALID_CONFIG.
TEST(FlashAttentionCPU, BackwardZeroNumHeadsReturnsInvalidConfig) {
    FlashAttentionConfig cfg;
    cfg.batch_size  = 1;
    cfg.seq_len     = 8;
    cfg.num_heads   = 0;  // invalid
    cfg.head_dim    = 16;
    FlashAttention fa(Backend::CPU, cfg);

    const size_t sz = 8 * 16;
    std::vector<float> buf(sz, 1.0f);
    Tensor t;
    t.data = buf.data(); t.size = sz; t.shape = {1, 8, 0, 16};

    EXPECT_EQ(fa.backward(t, t, t, t), Status::ERROR_INVALID_CONFIG);
}

// ============================================================================
// KVCacheManager Input Validation Tests (batch 39)
// ============================================================================

TEST(KVCacheManager, ZeroKVBlockSizeThrows) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 0;  // invalid
    EXPECT_THROW(KVCacheManager{config}, std::invalid_argument);
}

TEST(KVCacheManager, AllocateSequenceRejectsZeroTokens) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    KVCacheManager mgr(config);
    EXPECT_THROW(mgr.allocateSequence(1, 0), std::invalid_argument);
}

TEST(KVCacheManager, AllocateSequenceRejectsNegativeTokens) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    KVCacheManager mgr(config);
    EXPECT_THROW(mgr.allocateSequence(1, -5), std::invalid_argument);
}

TEST(KVCacheManager, SharePrefixRejectsNonPositivePrefixLength) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 128;
    config.kv_block_size = 16;
    KVCacheManager mgr(config);
    mgr.allocateSequence(1, 32);
    EXPECT_THROW(mgr.sharePrefix(2, 1, 0),  std::invalid_argument);
    EXPECT_THROW(mgr.sharePrefix(3, 1, -1), std::invalid_argument);
}

TEST(KVCacheManager, CalculateBlockSizeRejectsZeroDimensions) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 0;  // avoid actual block allocation
    config.kv_block_size = 16;
    config.num_layers    = 0;  // invalid — should throw inside calculateBlockSize
    // The constructor calls calculateBlockSize; invalid_argument expected.
    EXPECT_THROW(KVCacheManager{config}, std::invalid_argument);
}


