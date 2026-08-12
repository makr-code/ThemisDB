#include <gtest/gtest.h>
#include "llm/adaptive_vram_allocator.h"
#include "llm/multi_gpu_memory_coordinator.h"
#include "llm/paged_kv_cache_manager.h"
#include "llm/mixed_precision_inference.h"

using namespace themis::llm;

// Test fixture
class GPUVRAMAllocationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    // Helper: Create Llama-2-7B config
    AdaptiveVRAMAllocator::ModelConfig createLlama7BConfig() {
        AdaptiveVRAMAllocator::ModelConfig model;
        model.model_name = "Llama-2-7B";
        model.num_parameters = 7'000'000'000;
        model.num_layers = 32;
        model.hidden_dim = 4096;
        model.num_heads = 32;
        model.num_kv_heads = 8;  // GQA
        model.head_dim = 128;
        model.precision_bytes = 2;  // FP16
        return model;
    }
    
    // Helper: Create RTX 4090 hardware
    AdaptiveVRAMAllocator::HardwareInfo createRTX4090Hardware() {
        AdaptiveVRAMAllocator::HardwareInfo hw;
        hw.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24 GB
        hw.available_vram_bytes = 22ULL * 1024 * 1024 * 1024;  // 22 GB available
        hw.compute_capability_major = 8;
        hw.compute_capability_minor = 9;
        hw.has_tensor_cores = true;
        hw.memory_bandwidth_gbps = 1008;
        return hw;
    }
    
    // Helper: Create A100 hardware
    AdaptiveVRAMAllocator::HardwareInfo createA100Hardware() {
        AdaptiveVRAMAllocator::HardwareInfo hw;
        hw.total_vram_bytes = 80ULL * 1024 * 1024 * 1024;  // 80 GB
        hw.available_vram_bytes = 76ULL * 1024 * 1024 * 1024;  // 76 GB available
        hw.compute_capability_major = 8;
        hw.compute_capability_minor = 0;
        hw.has_tensor_cores = true;
        hw.memory_bandwidth_gbps = 2039;
        return hw;
    }
};

// ============================================================================
// AdaptiveVRAMAllocator Tests
// ============================================================================

TEST_F(GPUVRAMAllocationTest, CalculateOptimalAllocation_RTX4090_Llama7B) {
    AdaptiveVRAMAllocator allocator;
    
    auto model = createLlama7BConfig();
    auto hw = createRTX4090Hardware();
    
    AdaptiveVRAMAllocator::InferenceConfig config;
    // Keep this scenario in the "fits" region for 22 GB available VRAM
    // under the current allocator model (weights + KV + activations + overhead).
    config.batch_size = 2;
    config.max_seq_length = 4096;
    config.enable_prefix_caching = true;
    
    auto plan = allocator.calculateOptimalAllocation(model, hw, config);
    
    // Model should fit
    EXPECT_TRUE(plan.fits_in_vram);
    
    // Model weights should be ~14 GB (7B × 2 bytes)
    EXPECT_GE(plan.model_weights, 13ULL * 1024 * 1024 * 1024);
    EXPECT_LE(plan.model_weights, 15ULL * 1024 * 1024 * 1024);
    
    // Total should be within VRAM
    EXPECT_LE(plan.total, hw.available_vram_bytes);
    
    // Should have reasonable KV cache
    EXPECT_GT(plan.kv_cache_static, 0);
    
    // Fragmentation should be low with prefix caching
    EXPECT_LT(plan.expected_fragmentation, 0.05f);  // <5%
}

TEST_F(GPUVRAMAllocationTest, CalculateOptimalAllocation_Llama70B_TooLarge) {
    AdaptiveVRAMAllocator allocator;
    
    AdaptiveVRAMAllocator::ModelConfig model;
    model.model_name = "Llama-2-70B";
    model.num_parameters = 70'000'000'000;
    model.num_layers = 80;
    model.hidden_dim = 8192;
    model.num_heads = 64;
    model.num_kv_heads = 8;
    model.head_dim = 128;
    model.precision_bytes = 2;  // FP16
    
    auto hw = createRTX4090Hardware();
    
    AdaptiveVRAMAllocator::InferenceConfig config;
    config.batch_size = 4;
    config.max_seq_length = 4096;
    
    auto plan = allocator.calculateOptimalAllocation(model, hw, config);
    
    // Model should NOT fit
    EXPECT_FALSE(plan.fits_in_vram);
    
    // Should have recommendation
    EXPECT_FALSE(plan.recommendation.empty());
    EXPECT_NE(plan.recommendation.find("Consider"), std::string::npos);
}

TEST_F(GPUVRAMAllocationTest, CalculateKVCacheSizePerToken) {
    auto model = createLlama7BConfig();
    
    size_t kv_size = AdaptiveVRAMAllocator::calculateKVCacheSizePerToken(model);
    
    // Formula: 2 × 32 layers × 8 heads × 128 dim × 2 bytes
    size_t expected = 2 * 32 * 8 * 128 * 2;
    EXPECT_EQ(kv_size, expected);
    
    // Should be ~128 KB per token
    EXPECT_NEAR(kv_size, 128 * 1024, 1024);
}

TEST_F(GPUVRAMAllocationTest, CalculateModelSize) {
    size_t num_params = 7'000'000'000;
    
    // FP16
    size_t size_fp16 = AdaptiveVRAMAllocator::calculateModelSize(num_params, 2.0f);
    EXPECT_NEAR(static_cast<double>(size_fp16), 14'000'000'000.0, 1e8);
    
    // INT8
    size_t size_int8 = AdaptiveVRAMAllocator::calculateModelSize(num_params, 1.0f);
    EXPECT_NEAR(static_cast<double>(size_int8), 7'000'000'000.0, 1e8);
    
    // Q4
    size_t size_q4 = AdaptiveVRAMAllocator::calculateModelSize(num_params, 0.5f);
    EXPECT_NEAR(static_cast<double>(size_q4), 3'500'000'000.0, 1e8);
}

// ============================================================================
// PagedKVCacheManager Tests
// ============================================================================

TEST_F(GPUVRAMAllocationTest, PagedKVCache_BlockAllocation) {
    PagedKVCacheManager::Config config;
    config.num_blocks = 1024;
    config.block_size = 16;
    config.num_layers = 32;
    config.head_dim = 128;
    config.num_kv_heads = 8;
    
    PagedKVCacheManager cache_mgr(config);
    
    // Allocate 10 blocks
    auto blocks = cache_mgr.allocateBlocks(10);
    
    EXPECT_EQ(blocks.size(), 10);
    
    // All blocks should be valid
    for (int block_id : blocks) {
        EXPECT_GE(block_id, 0);
        EXPECT_LT(block_id, 1024);
    }
    
    // Free blocks
    cache_mgr.freeBlocks(blocks);
}

TEST_F(GPUVRAMAllocationTest, PagedKVCache_PrefixCaching) {
    PagedKVCacheManager::Config config;
    config.num_blocks = 1024;
    config.enable_prefix_caching = true;
    
    PagedKVCacheManager cache_mgr(config);
    
    // Create parent sequence
    uint64_t parent_seq = 1;
    auto parent_table = cache_mgr.addSequence(parent_seq, 512);  // 512 tokens
    
    EXPECT_EQ(parent_table.sequence_id, parent_seq);
    EXPECT_EQ(parent_table.num_tokens, 512);
    
    // Create child sequence with shared prefix
    uint64_t child_seq = 2;
    bool success = cache_mgr.enablePrefixCaching(child_seq, parent_seq, 256);  // Share 256 tokens
    
    EXPECT_TRUE(success);
    
    // Check memory savings
    double savings = cache_mgr.calculatePrefixSavings();
    EXPECT_GT(savings, 0.0);
}

TEST_F(GPUVRAMAllocationTest, PagedKVCache_MemoryStats) {
    PagedKVCacheManager::Config config;
    config.num_blocks = 100;
    config.block_size = 16;
    
    PagedKVCacheManager cache_mgr(config);
    
    auto stats = cache_mgr.getMemoryStats();
    
    // Initially all blocks should be free
    EXPECT_EQ(stats.total_blocks, 100);
    EXPECT_EQ(stats.free_blocks, 100);
    EXPECT_EQ(stats.used_blocks, 0);
    EXPECT_EQ(stats.num_sequences, 0);
    
    // Allocate sequence
    cache_mgr.addSequence(1, 64);  // 64 tokens = 4 blocks (16 tokens/block)
    
    stats = cache_mgr.getMemoryStats();
    EXPECT_EQ(stats.num_sequences, 1);
    EXPECT_GT(stats.used_blocks, 0);
    EXPECT_LT(stats.free_blocks, 100);
}

// ============================================================================
// MultiGPUMemoryCoordinator Tests
// ============================================================================

TEST_F(GPUVRAMAllocationTest, MultiGPU_TensorParallelism) {
    MultiGPUMemoryCoordinator coordinator;
    coordinator.initialize({0, 1, 2, 3});
    
    size_t model_size = 140ULL * 1024 * 1024 * 1024;  // 140 GB
    auto plan = coordinator.distributeModelWeights({0, 1, 2, 3}, model_size);
    
    EXPECT_EQ(plan.strategy, MultiGPUMemoryCoordinator::DistributionStrategy::TENSOR_PARALLEL);
    EXPECT_EQ(plan.tensor_parallel_size, 4);
    EXPECT_EQ(plan.shard_sizes.size(), 4);
    
    // Each GPU should get ~35 GB
    for (size_t shard_size : plan.shard_sizes) {
        EXPECT_NEAR(shard_size, model_size / 4, 1e9);
    }
    
    // Should enable P2P
    EXPECT_TRUE(plan.enable_p2p);
    EXPECT_GT(plan.p2p_pairs.size(), 0);
}

TEST_F(GPUVRAMAllocationTest, MultiGPU_PipelineParallelism) {
    MultiGPUMemoryCoordinator coordinator;
    coordinator.initialize({0, 1, 2, 3});
    
    size_t num_layers = 80;
    size_t layer_size = 1750ULL * 1024 * 1024;  // 1.75 GB
    
    auto plan = coordinator.distributeLayers({0, 1, 2, 3}, num_layers, layer_size);
    
    EXPECT_EQ(plan.strategy, MultiGPUMemoryCoordinator::DistributionStrategy::PIPELINE_PARALLEL);
    EXPECT_EQ(plan.pipeline_parallel_size, 4);
    EXPECT_EQ(plan.layer_assignments.size(), 4);
    
    // Check layer distribution
    size_t total_layers = 0;
    for (const auto& gpu_layers : plan.layer_assignments) {
        total_layers += gpu_layers.size();
    }
    EXPECT_EQ(total_layers, num_layers);
}

TEST_F(GPUVRAMAllocationTest, MultiGPU_LoadBalancing) {
    MultiGPUMemoryCoordinator coordinator;
    coordinator.initialize({0, 1, 2, 3});
    
    size_t batch_size = 64;
    auto plan = coordinator.balanceInferenceLoad({0, 1, 2, 3}, batch_size);
    
    EXPECT_EQ(plan.strategy, MultiGPUMemoryCoordinator::DistributionStrategy::DATA_PARALLEL);
    EXPECT_EQ(plan.batch_assignments.size(), 4);
    
    // Total batch should match
    int total_batch = 0;
    for (int gpu_batch : plan.batch_assignments) {
        total_batch += gpu_batch;
        EXPECT_GT(gpu_batch, 0);
    }
    EXPECT_EQ(total_batch, batch_size);
}

// ============================================================================
// MixedPrecisionInference Tests
// ============================================================================

TEST_F(GPUVRAMAllocationTest, MixedPrecision_SelectOptimalPrecision) {
    MixedPrecisionInference mpi;
    
    size_t available_vram = 24ULL * 1024 * 1024 * 1024;  // 24 GB
    size_t model_size_fp32 = 28ULL * 1024 * 1024 * 1024;  // 28 GB
    
    // Should select FP16 (14 GB)
    auto precision = mpi.selectOptimalPrecision(available_vram, model_size_fp32, 0.01f);
    EXPECT_EQ(precision, PrecisionMode::FP16);
    
    // With smaller VRAM, should select INT8
    available_vram = 10ULL * 1024 * 1024 * 1024;  // 10 GB
    precision = mpi.selectOptimalPrecision(available_vram, model_size_fp32, 0.02f);
    EXPECT_EQ(precision, PrecisionMode::INT8);
}

TEST_F(GPUVRAMAllocationTest, MixedPrecision_PrecisionInfo) {
    auto fp16_info = MixedPrecisionInference::getPrecisionInfo(PrecisionMode::FP16);
    EXPECT_EQ(fp16_info.bytes_per_param, 2);
    EXPECT_NEAR(fp16_info.accuracy_retention, 0.999f, 0.001f);
    EXPECT_NEAR(fp16_info.memory_reduction, 0.5f, 0.01f);
    
    auto int8_info = MixedPrecisionInference::getPrecisionInfo(PrecisionMode::INT8);
    EXPECT_EQ(int8_info.bytes_per_param, 1);
    EXPECT_NEAR(int8_info.accuracy_retention, 0.98f, 0.01f);
    EXPECT_NEAR(int8_info.memory_reduction, 0.75f, 0.01f);
}

TEST_F(GPUVRAMAllocationTest, MixedPrecision_CalculateModelSize) {
    size_t num_params = 7'000'000'000;
    
    // FP16
    size_t size_fp16 = MixedPrecisionInference::calculateModelSize(num_params, PrecisionMode::FP16);
    EXPECT_NEAR(static_cast<double>(size_fp16), 14'000'000'000.0, 1e8);
    
    // INT8
    size_t size_int8 = MixedPrecisionInference::calculateModelSize(num_params, PrecisionMode::INT8);
    EXPECT_NEAR(static_cast<double>(size_int8), 7'000'000'000.0, 1e8);
}

TEST_F(GPUVRAMAllocationTest, MixedPrecision_StringConversion) {
    EXPECT_EQ(MixedPrecisionInference::fromString("FP16"), PrecisionMode::FP16);
    EXPECT_EQ(MixedPrecisionInference::fromString("INT8"), PrecisionMode::INT8);
    EXPECT_EQ(MixedPrecisionInference::fromString("Q4"), PrecisionMode::Q4);
    
    EXPECT_EQ(MixedPrecisionInference::toString(PrecisionMode::FP16), "FP16");
    EXPECT_EQ(MixedPrecisionInference::toString(PrecisionMode::INT8), "INT8");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GPUVRAMAllocationTest, Integration_CompleteWorkflow) {
    // 1. Calculate allocation plan
    AdaptiveVRAMAllocator allocator;
    auto model = createLlama7BConfig();
    auto hw = createRTX4090Hardware();
    
    AdaptiveVRAMAllocator::InferenceConfig config;
    config.batch_size = 2;
    config.max_seq_length = 4096;
    config.enable_prefix_caching = true;
    
    auto plan = allocator.calculateOptimalAllocation(model, hw, config);
    ASSERT_TRUE(plan.fits_in_vram);
    
    // 2. Setup paged KV cache
    PagedKVCacheManager::Config cache_config;
    cache_config.num_blocks = 4096;
    cache_config.block_size = 16;
    cache_config.enable_prefix_caching = true;
    
    PagedKVCacheManager cache_mgr(cache_config);
    
    // 3. Add sequences
    cache_mgr.addSequence(1, 2048);
    cache_mgr.addSequence(2, 2048);
    
    // 4. Check stats
    auto stats = cache_mgr.getMemoryStats();
    EXPECT_GT(stats.used_blocks, 0);
    EXPECT_EQ(stats.num_sequences, 2);
}

// ============================================================================
// PagedKVCacheManager Input Validation Tests (batch 39)
// ============================================================================

TEST(PagedKVCacheManagerTest, ZeroBlockSizeThrows) {
    PagedKVCacheManager::Config config;
    config.num_blocks  = 64;
    config.block_size  = 0;  // invalid
    EXPECT_THROW(PagedKVCacheManager{config}, std::invalid_argument);
}

TEST(PagedKVCacheManagerTest, ZeroNumTokensAllocatesNoBlocks) {
    PagedKVCacheManager::Config config;
    config.num_blocks  = 64;
    config.block_size  = 16;
    PagedKVCacheManager mgr(config);
    auto table = mgr.addSequence(1, 0);
    EXPECT_EQ(table.block_ids.size(), 0u);
}
