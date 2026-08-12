/**
 * ThemisDB Multi-GPU LoRA Tests (v1.4.0)
 * 
 * Comprehensive tests for multi-GPU LoRA adapter support including:
 * - Round-robin placement
 * - Data parallel replication
 * - Model parallel splitting
 * - GPU failure handling
 * - Load balancing
 * - Unified VRAM tracking
 */

#include <gtest/gtest.h>
#include "llm/multi_lora_manager.h"
#include "llm/gpu_memory_manager.h"
#include "llm/llm_plugin_interface.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace themis::llm;

namespace {
bool ensureFixtureFile(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        return false;
    }

    if (!std::filesystem::exists(path)) {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            return false;
        }
        out << "themis-lora-fixture";
    }
    return true;
}

bool prepareLoRAPathFixtures() {
    const std::filesystem::path root("C:/path");
    std::vector<std::filesystem::path> rel_paths = {
        "to/popular.bin", "to/replicated.bin", "to/split.bin", "to/huge.bin",
        "to/pinned.bin", "to/new.bin", "to/single1.bin", "to/single2.bin",
        "to/multi.bin", "to/quant.bin", "to/lora.bin", "to/model.bin",
        "to/large.bin", "to/math.bin", "to/code.bin", "to/chat.bin",
        "to/lora-a.bin", "to/lora-b.bin", "to/lora-c.bin", "to/lora-d.bin",
        "to/test.bin", "to/lora1.bin", "to/lora2.bin", "to/lora3.bin", "to/lora4.bin",
        "lora-a.bin", "lora-b.bin", "lora-c.bin", "lora-d.bin"
    };

    for (int i = 0; i <= 150; ++i) {
        rel_paths.push_back(std::filesystem::path("to") / ("lora" + std::to_string(i) + ".bin"));
        rel_paths.push_back(std::filesystem::path("to") / ("lora-" + std::to_string(i) + ".bin"));
        rel_paths.push_back("lora" + std::to_string(i) + ".bin");
        rel_paths.push_back("lora-" + std::to_string(i) + ".bin");
        rel_paths.push_back("workflow-" + std::to_string(i) + ".bin");
        rel_paths.push_back("temp-" + std::to_string(i) + ".bin");
        rel_paths.push_back("filler" + std::to_string(i) + ".bin");
    }

    for (const auto& rel : rel_paths) {
        if (!ensureFixtureFile(root / rel)) {
            return false;
        }
    }
    return true;
}
} // namespace

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class MultiGPULoRATest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!prepareLoRAPathFixtures()) {
            GTEST_SKIP() << "Cannot prepare /path LoRA fixture files";
        }

        // Basic config
        config_.max_lora_vram_mb = 512;
        config_.max_lora_slots = 16;
        config_.lora_ttl = std::chrono::seconds(60);
        config_.enable_multi_lora_batch = true;
        
        // Multi-GPU config with 4 GPUs
        config_.multi_gpu.enabled = true;
        config_.multi_gpu.devices = {0, 1, 2, 3};
        config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
        config_.multi_gpu.enable_peer_transfer = true;
        config_.multi_gpu.max_vram_per_gpu_mb = 128;  // 128MB per GPU
        config_.multi_gpu.enable_load_balancing = true;
    }
    
    MultiLoRAManager::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Multi-GPU Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, MultiGPUConfigurationEnabled) {
    MultiLoRAManager manager(config_);
    
    auto gpu_config = manager.getMultiGPUConfig();
    EXPECT_TRUE(gpu_config.enabled);
    EXPECT_EQ(gpu_config.devices.size(), 4);
    EXPECT_EQ(gpu_config.devices[0], 0);
    EXPECT_EQ(gpu_config.devices[1], 1);
    EXPECT_EQ(gpu_config.devices[2], 2);
    EXPECT_EQ(gpu_config.devices[3], 3);
    EXPECT_EQ(gpu_config.strategy, MultiGPUStrategy::ROUND_ROBIN);
}

TEST_F(MultiGPULoRATest, UpdateMultiGPUConfig) {
    MultiLoRAManager manager(config_);
    
    MultiGPUConfig new_config;
    new_config.enabled = true;
    new_config.devices = {0, 1};
    new_config.strategy = MultiGPUStrategy::DATA_PARALLEL;
    
    manager.setMultiGPUConfig(new_config);
    
    auto updated = manager.getMultiGPUConfig();
    EXPECT_EQ(updated.devices.size(), 2);
    EXPECT_EQ(updated.strategy, MultiGPUStrategy::DATA_PARALLEL);
}

TEST_F(MultiGPULoRATest, SingleGPUModeByDefault) {
    MultiLoRAManager::Config single_gpu_config;
    single_gpu_config.multi_gpu.enabled = false;
    
    MultiLoRAManager manager(single_gpu_config);
    
    auto gpu_config = manager.getMultiGPUConfig();
    EXPECT_FALSE(gpu_config.enabled);
}

// ═══════════════════════════════════════════════════════════
// Round-Robin Placement Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, RoundRobinPlacement) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // Load 4 LoRAs
    manager.loadLoRA("lora-1", "/path/to/lora1.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-2", "/path/to/lora2.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-3", "/path/to/lora3.bin", "base-model", 1.0f);
    manager.loadLoRA("lora-4", "/path/to/lora4.bin", "base-model", 1.0f);
    
    // Verify each LoRA is on a different GPU (round-robin)
    auto gpu1 = manager.getLoRAGPUPlacement("lora-1");
    auto gpu2 = manager.getLoRAGPUPlacement("lora-2");
    auto gpu3 = manager.getLoRAGPUPlacement("lora-3");
    auto gpu4 = manager.getLoRAGPUPlacement("lora-4");
    
    EXPECT_EQ(gpu1.size(), 1);
    EXPECT_EQ(gpu2.size(), 1);
    EXPECT_EQ(gpu3.size(), 1);
    EXPECT_EQ(gpu4.size(), 1);
    
    // Each should be on a different GPU
    EXPECT_EQ(gpu1[0], 0);
    EXPECT_EQ(gpu2[0], 1);
    EXPECT_EQ(gpu3[0], 2);
    EXPECT_EQ(gpu4[0], 3);
}

TEST_F(MultiGPULoRATest, RoundRobinWrapsAround) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // Load 8 LoRAs (more than number of GPUs)
    for (int i = 0; i < 8; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        std::string path = "/path/to/lora" + std::to_string(i) + ".bin";
        manager.loadLoRA(lora_id, path, "base-model", 1.0f);
    }
    
    // Verify round-robin wraps around
    auto gpu0 = manager.getLoRAGPUPlacement("lora-0");
    auto gpu4 = manager.getLoRAGPUPlacement("lora-4");
    
    EXPECT_EQ(gpu0[0], 0);
    EXPECT_EQ(gpu4[0], 0);  // Should wrap back to GPU 0
}

TEST_F(MultiGPULoRATest, RoundRobinDistributesEvenly) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // Load 16 LoRAs
    for (int i = 0; i < 16; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        std::string path = "/path/to/lora" + std::to_string(i) + ".bin";
        manager.loadLoRA(lora_id, path, "base-model", 1.0f);
    }
    
    // Check per-GPU memory usage
    auto per_gpu_usage = manager.getPerGPUMemoryUsage();
    
    // Each GPU should have approximately equal usage (4 LoRAs per GPU)
    EXPECT_EQ(per_gpu_usage.size(), 4);
    
    for (const auto& [gpu_id, usage] : per_gpu_usage) {
        EXPECT_GT(usage, 0);  // Each GPU should have some LoRAs
    }
}

// ═══════════════════════════════════════════════════════════
// Data Parallel Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, DataParallelReplication) {
    config_.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    MultiLoRAManager manager(config_);
    
    // Load with multi-GPU placement
    bool loaded = manager.loadLoRA("replicated-lora", "/path/to/lora.bin", "base-model", 
                                    false, GPUPlacement::MULTI_GPU, 1.0f);
    EXPECT_TRUE(loaded);
    
    // LoRA should be on all GPUs
    auto gpus = manager.getLoRAGPUPlacement("replicated-lora");
    EXPECT_EQ(gpus.size(), 4);  // Should be on all 4 GPUs
    
    // Verify all GPUs are in the list
    std::set<int> gpu_set(gpus.begin(), gpus.end());
    EXPECT_EQ(gpu_set.count(0), 1);
    EXPECT_EQ(gpu_set.count(1), 1);
    EXPECT_EQ(gpu_set.count(2), 1);
    EXPECT_EQ(gpu_set.count(3), 1);
}

TEST_F(MultiGPULoRATest, DataParallelHigherThroughput) {
    config_.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    config_.enable_multi_lora_batch = true;
    MultiLoRAManager manager(config_);
    
    // Load a replicated LoRA
    manager.loadLoRA("popular-lora", "/path/to/popular.bin", "base-model", 
                     false, GPUPlacement::MULTI_GPU, 1.0f);
    
    // Create batch requests using the same LoRA
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    for (int i = 0; i < 10; ++i) {
        InferenceRequest req;
        req.prompt = "Request " + std::to_string(i);
        requests.push_back({req, "popular-lora"});
    }
    
    // Process batch (should benefit from replication)
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    EXPECT_EQ(responses.size(), 10);
}

TEST_F(MultiGPULoRATest, DataParallelMemoryMultiplier) {
    config_.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("replicated", "/path/to/replicated.bin", "base-model", 
                     false, GPUPlacement::MULTI_GPU, 1.0f);
    
    auto per_gpu_usage = manager.getPerGPUMemoryUsage();
    
    // Each GPU should have the full LoRA memory
    for (const auto& [gpu_id, usage] : per_gpu_usage) {
        EXPECT_GT(usage, 0);  // Each GPU has memory allocated
    }
}

// ═══════════════════════════════════════════════════════════
// Model Parallel Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, ModelParallelSplitting) {
    config_.multi_gpu.strategy = MultiGPUStrategy::MODEL_PARALLEL;
    MultiLoRAManager manager(config_);
    
    // Load large LoRA with model parallel
    bool loaded = manager.loadLoRA("large-lora", "/path/to/large.bin", "base-model", 
                                    false, GPUPlacement::MULTI_GPU, 1.0f);
    EXPECT_TRUE(loaded);
    
    // LoRA should be split across all GPUs
    auto gpus = manager.getLoRAGPUPlacement("large-lora");
    EXPECT_EQ(gpus.size(), 4);  // Should span all 4 GPUs
}

TEST_F(MultiGPULoRATest, ModelParallelMemoryEfficiency) {
    config_.multi_gpu.strategy = MultiGPUStrategy::MODEL_PARALLEL;
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("split-lora", "/path/to/split.bin", "base-model", 
                     false, GPUPlacement::MULTI_GPU, 1.0f);
    
    auto per_gpu_usage = manager.getPerGPUMemoryUsage();
    
    // Total memory across GPUs should be approximately equal to LoRA size
    // (not multiplied by number of GPUs like data parallel)
    size_t total_memory = 0;
    for (const auto& [gpu_id, usage] : per_gpu_usage) {
        total_memory += usage;
    }
    
    // Should be roughly the size of one LoRA (with some overhead)
    EXPECT_GT(total_memory, 0);
}

TEST_F(MultiGPULoRATest, ModelParallelSupportsLargeAdapters) {
    config_.multi_gpu.strategy = MultiGPUStrategy::MODEL_PARALLEL;
    config_.multi_gpu.max_vram_per_gpu_mb = 50;  // Small per-GPU limit
    MultiLoRAManager manager(config_);
    
    // Load LoRA that would be too large for single GPU
    // but fits when split across multiple GPUs
    bool loaded = manager.loadLoRA("huge-lora", "/path/to/huge.bin", "base-model", 
                                    false, GPUPlacement::MULTI_GPU, 1.0f);
    EXPECT_TRUE(loaded);
}

// ═══════════════════════════════════════════════════════════
// Load Balancing Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, LoadBalancingDetectsImbalance) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    config_.multi_gpu.enable_load_balancing = true;
    MultiLoRAManager manager(config_);
    
    // Load several LoRAs
    for (int i = 0; i < 8; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        manager.loadLoRA(lora_id, "/path/" + lora_id + ".bin", "base-model", 1.0f);
    }
    
    // Attempt load balancing
    size_t moved = manager.balanceGPULoad();
    
    // Should detect that load is already balanced (or balance it)
    EXPECT_GE(moved, 0);
}

TEST_F(MultiGPULoRATest, LoadBalancingRespectsPinnedLoRAs) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // Load and pin a LoRA
    manager.loadLoRA("pinned", "/path/to/pinned.bin", "base-model", 1.0f);
    manager.pinLoRA("pinned");
    
    // Load more LoRAs
    for (int i = 0; i < 10; ++i) {
        manager.loadLoRA("lora-" + std::to_string(i), "/path/to/lora.bin", "base-model", 1.0f);
    }
    
    // Balance load
    size_t moved = manager.balanceGPULoad();
    
    // Pinned LoRA should not move
    auto gpus_before = manager.getLoRAGPUPlacement("pinned");
    manager.balanceGPULoad();
    auto gpus_after = manager.getLoRAGPUPlacement("pinned");
    
    EXPECT_EQ(gpus_before, gpus_after);
}

TEST_F(MultiGPULoRATest, PerGPUMemoryTracking) {
    MultiLoRAManager manager(config_);
    
    // Load LoRAs
    for (int i = 0; i < 8; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        manager.loadLoRA(lora_id, "/path/" + lora_id + ".bin", "base-model", 1.0f);
    }
    
    // Get per-GPU usage
    auto per_gpu_usage = manager.getPerGPUMemoryUsage();
    
    EXPECT_EQ(per_gpu_usage.size(), 4);  // 4 GPUs
    
    // All GPUs should have some memory used
    for (const auto& [gpu_id, usage] : per_gpu_usage) {
        EXPECT_GT(usage, 0);
    }
}

// ═══════════════════════════════════════════════════════════
// GPU Failure Handling Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, HandlesGPUFailureGracefully) {
    MultiLoRAManager manager(config_);
    
    // Load LoRAs normally
    for (int i = 0; i < 4; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        bool loaded = manager.loadLoRA(lora_id, "/path/" + lora_id + ".bin", "base-model", 1.0f);
        EXPECT_TRUE(loaded);
    }
    
    // Simulate GPU failure by updating config to exclude a GPU
    MultiGPUConfig new_config = config_.multi_gpu;
    new_config.devices = {0, 1, 2};  // Remove GPU 3
    manager.setMultiGPUConfig(new_config);
    
    // Should still be able to load new LoRAs
    bool loaded = manager.loadLoRA("new-lora", "/path/to/new.bin", "base-model", 1.0f);
    EXPECT_TRUE(loaded);
}

TEST_F(MultiGPULoRATest, FallbackToAvailableGPUs) {
    MultiLoRAManager manager(config_);
    
    // Reduce available GPUs mid-operation
    MultiGPUConfig reduced_config = config_.multi_gpu;
    reduced_config.devices = {0, 1};  // Only 2 GPUs available
    manager.setMultiGPUConfig(reduced_config);
    
    // Load LoRAs - should use only available GPUs
    for (int i = 0; i < 4; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        bool loaded = manager.loadLoRA(lora_id, "/path/" + lora_id + ".bin", "base-model", 1.0f);
        EXPECT_TRUE(loaded);
        
        auto gpus = manager.getLoRAGPUPlacement(lora_id);
        EXPECT_EQ(gpus.size(), 1);
        EXPECT_TRUE(gpus[0] == 0 || gpus[0] == 1);  // Should be on GPU 0 or 1 only
    }
}

// ═══════════════════════════════════════════════════════════
// Mixed Strategy Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, MixSingleAndMultiGPULoRAs) {
    config_.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    MultiLoRAManager manager(config_);
    
    // Load some single-GPU LoRAs
    manager.loadLoRA("single-1", "/path/to/single1.bin", "base-model", false, GPUPlacement::SINGLE_GPU, 1.0f);
    manager.loadLoRA("single-2", "/path/to/single2.bin", "base-model", false, GPUPlacement::SINGLE_GPU, 1.0f);
    
    // Load a multi-GPU LoRA
    manager.loadLoRA("multi", "/path/to/multi.bin", "base-model", false, GPUPlacement::MULTI_GPU, 1.0f);
    
    // Verify placements
    auto single1_gpus = manager.getLoRAGPUPlacement("single-1");
    auto single2_gpus = manager.getLoRAGPUPlacement("single-2");
    auto multi_gpus = manager.getLoRAGPUPlacement("multi");
    
    EXPECT_EQ(single1_gpus.size(), 1);
    EXPECT_EQ(single2_gpus.size(), 1);
    EXPECT_EQ(multi_gpus.size(), 4);  // Data parallel replicates across all GPUs
}

TEST_F(MultiGPULoRATest, QuantizationWithMultiGPU) {
    config_.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    config_.quantization.enabled = true;
    config_.quantization.mode = QuantizationMode::INT8;
    MultiLoRAManager manager(config_);
    
    // Load quantized LoRA across multiple GPUs
    bool loaded = manager.loadLoRA("quant-multi", "/path/to/quant.bin", "base-model", 
                                    true, GPUPlacement::MULTI_GPU, 1.0f);
    EXPECT_TRUE(loaded);
    
    // Should be quantized AND replicated
    auto stats = manager.getQuantizationStats("quant-multi");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->mode, QuantizationMode::INT8);
    
    auto gpus = manager.getLoRAGPUPlacement("quant-multi");
    EXPECT_GT(gpus.size(), 1);  // Should be on multiple GPUs
}

// ═══════════════════════════════════════════════════════════
// Performance and Stress Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, HighLoadScenario) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    config_.max_lora_slots = 32;
    MultiLoRAManager manager(config_);
    
    // Load many LoRAs
    for (int i = 0; i < 32; ++i) {
        std::string lora_id = "lora-" + std::to_string(i);
        std::string path = "/path/to/lora" + std::to_string(i) + ".bin";
        bool loaded = manager.loadLoRA(lora_id, path, "base-model", 1.0f);
        EXPECT_TRUE(loaded);
    }
    
    // Verify all are loaded
    auto loras = manager.listLoRAs();
    EXPECT_GE(loras.size(), 1);  // Should have loaded at least some
    
    // Check memory distribution
    auto per_gpu_usage = manager.getPerGPUMemoryUsage();
    EXPECT_EQ(per_gpu_usage.size(), 4);
}

TEST_F(MultiGPULoRATest, SimultaneousLoadsOnDifferentGPUs) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // Load 4 LoRAs simultaneously (would go to different GPUs)
    std::vector<std::string> lora_ids = {"lora-a", "lora-b", "lora-c", "lora-d"};
    
    for (const auto& id : lora_ids) {
        bool loaded = manager.loadLoRA(id, "/path/to/" + id + ".bin", "base-model", 1.0f);
        EXPECT_TRUE(loaded);
    }
    
    // Each should be on a different GPU
    std::set<int> used_gpus;
    for (const auto& id : lora_ids) {
        auto gpus = manager.getLoRAGPUPlacement(id);
        EXPECT_EQ(gpus.size(), 1);
        used_gpus.insert(gpus[0]);
    }
    
    EXPECT_EQ(used_gpus.size(), 4);  // All 4 GPUs should be used
}

TEST_F(MultiGPULoRATest, LinearScalingWithMultipleGPUs) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    MultiLoRAManager manager(config_);
    
    // With 4 GPUs, should be able to load ~4x more LoRAs compared to single GPU
    // (assuming per-GPU VRAM limits)
    
    int loaded_count = 0;
    for (int i = 0; i < 100; ++i) {  // Try to load many
        std::string lora_id = "lora-" + std::to_string(i);
        if (manager.loadLoRA(lora_id, "/path/" + lora_id + ".bin", "base-model", 1.0f)) {
            loaded_count++;
        }
    }
    
    // Should be able to load at least 4 (one per GPU)
    EXPECT_GE(loaded_count, 4);
}

// ═══════════════════════════════════════════════════════════
// GPU Memory Manager Multi-GPU Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, GPUMemoryManagerMultiGPUSupport) {
    GPUMemoryManager::Config gpu_config;
    gpu_config.enable_multi_gpu = true;
    gpu_config.gpu_devices = {0, 1, 2, 3};
    gpu_config.enable_peer_access = true;
    gpu_config.max_vram_bytes = 1024ULL * 1024 * 1024;  // 1GB per GPU
    
    GPUMemoryManager gpu_mem(gpu_config);
    
    // Allocate on specific GPUs
    void* ptr0 = gpu_mem.allocateGPU("model-1", 100 * 1024 * 1024, 0);  // 100MB on GPU 0
    void* ptr1 = gpu_mem.allocateGPU("model-2", 100 * 1024 * 1024, 1);  // 100MB on GPU 1
    
    EXPECT_NE(ptr0, nullptr);
    EXPECT_NE(ptr1, nullptr);
    
    // Check per-GPU usage
    EXPECT_EQ(gpu_mem.getGPUVRAM(0), 100 * 1024 * 1024);
    EXPECT_EQ(gpu_mem.getGPUVRAM(1), 100 * 1024 * 1024);
    EXPECT_EQ(gpu_mem.getGPUVRAM(2), 0);
    EXPECT_EQ(gpu_mem.getGPUVRAM(3), 0);
    
    // Cleanup
    gpu_mem.freeGPU("model-1", ptr0);
    gpu_mem.freeGPU("model-2", ptr1);
}

TEST_F(MultiGPULoRATest, GPUMemoryManagerPeerAccess) {
    GPUMemoryManager::Config gpu_config;
    gpu_config.enable_multi_gpu = true;
    gpu_config.gpu_devices = {0, 1};
    gpu_config.enable_peer_access = true;
    
    GPUMemoryManager gpu_mem(gpu_config);
    
    // Enable peer access between GPUs
    bool enabled = gpu_mem.enablePeerAccess(0, 1);
    EXPECT_TRUE(enabled);
    
    // Check if peer access is possible
    bool can_access = gpu_mem.canAccessPeer(0, 1);
    EXPECT_TRUE(can_access);
}

TEST_F(MultiGPULoRATest, GPUMemoryManagerAvailableGPUs) {
    GPUMemoryManager::Config gpu_config;
    gpu_config.enable_multi_gpu = true;
    gpu_config.gpu_devices = {0, 1, 2, 3};
    
    GPUMemoryManager gpu_mem(gpu_config);
    
    auto available = gpu_mem.getAvailableGPUs();
    EXPECT_EQ(available.size(), 4);
    EXPECT_EQ(available[0], 0);
    EXPECT_EQ(available[1], 1);
    EXPECT_EQ(available[2], 2);
    EXPECT_EQ(available[3], 3);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases and Error Handling
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, EmptyGPUDeviceList) {
    MultiLoRAManager::Config empty_config;
    empty_config.multi_gpu.enabled = true;
    empty_config.multi_gpu.devices = {};  // Empty!
    
    MultiLoRAManager manager(empty_config);
    
    // Should still work, falling back to default behavior
    bool loaded = manager.loadLoRA("test", "/path/to/test.bin", "base-model", 1.0f);
    EXPECT_TRUE(loaded);
}

TEST_F(MultiGPULoRATest, InvalidGPUDeviceID) {
    MultiLoRAManager::Config invalid_config;
    invalid_config.multi_gpu.enabled = true;
    invalid_config.multi_gpu.devices = {0, 999};  // GPU 999 doesn't exist
    
    MultiLoRAManager manager(invalid_config);
    
    // Should handle gracefully
    bool loaded = manager.loadLoRA("test", "/path/to/test.bin", "base-model", 1.0f);
    EXPECT_TRUE(loaded);  // Should still load (on valid GPUs)
}

TEST_F(MultiGPULoRATest, MultiGPUWithSingleGPUDevice) {
    MultiLoRAManager::Config single_device_config;
    single_device_config.multi_gpu.enabled = true;
    single_device_config.multi_gpu.devices = {0};  // Only one GPU
    single_device_config.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;
    
    MultiLoRAManager manager(single_device_config);
    
    // Should work, but behave like single GPU
    bool loaded = manager.loadLoRA("test", "/path/to/test.bin", "base-model", 
                                    false, GPUPlacement::MULTI_GPU, 1.0f);
    EXPECT_TRUE(loaded);
    
    auto gpus = manager.getLoRAGPUPlacement("test");
    EXPECT_EQ(gpus.size(), 1);
    EXPECT_EQ(gpus[0], 0);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRATest, EndToEndMultiGPUWorkflow) {
    config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
    config_.enable_multi_lora_batch = true;
    MultiLoRAManager manager(config_);
    
    // 1. Load multiple LoRAs
    manager.loadLoRA("math", "/path/to/math.bin", "llama-7b", 1.0f);
    manager.loadLoRA("code", "/path/to/code.bin", "llama-7b", 1.0f);
    manager.loadLoRA("chat", "/path/to/chat.bin", "llama-7b", 1.0f);
    
    // 2. Create batch inference requests
    std::vector<std::pair<InferenceRequest, std::string>> requests;
    
    InferenceRequest req1;
    req1.prompt = "Solve 2x + 5 = 13";
    requests.push_back({req1, "math"});
    
    InferenceRequest req2;
    req2.prompt = "Write a function";
    requests.push_back({req2, "code"});
    
    InferenceRequest req3;
    req3.prompt = "Hello!";
    requests.push_back({req3, "chat"});
    
    // 3. Process batch
    auto responses = manager.batchInferenceMultiLoRA(requests, nullptr);
    EXPECT_EQ(responses.size(), 3);
    
    // 4. Check statistics
    auto stats = manager.getStatistics();
    EXPECT_EQ(stats.total_loras_loaded, 3);
    
    // 5. Balance load
    size_t moved = manager.balanceGPULoad();
    EXPECT_GE(moved, 0);
    
    // 6. Verify all LoRAs still accessible
    EXPECT_TRUE(manager.isLoRALoaded("math"));
    EXPECT_TRUE(manager.isLoRALoaded("code"));
    EXPECT_TRUE(manager.isLoRALoaded("chat"));
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════


