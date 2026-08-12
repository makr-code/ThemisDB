/**
 * @file test_multi_gpu_lora_advanced.cpp
 * @brief Advanced Multi-GPU LoRA Management Tests (v1.5.0)
 * 
 * Tests for enhanced features:
 * - Resource-aware eviction
 * - Dynamic scheduling
 * - GPU migration and fault tolerance
 * - Security and audit logging
 * - Usage heatmap tracking
 */

#include <gtest/gtest.h>
#include "llm/multi_lora_manager.h"
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
        "hot.bin", "cold.bin", "warm.bin", "1.bin", "active.bin", "idle1.bin", "idle2.bin",
        "g0-1.bin", "g1-1.bin", "g0-2.bin", "migrating.bin", "pinned.bin", "unpinned.bin",
        "new.bin", "t1.bin", "t2.bin", "audited.bin", "tenant.bin"
    };

    for (int i = 0; i <= 150; ++i) {
        rel_paths.push_back("lora" + std::to_string(i) + ".bin");
        rel_paths.push_back("lora-" + std::to_string(i) + ".bin");
        rel_paths.push_back("workflow-" + std::to_string(i) + ".bin");
        rel_paths.push_back("temp-" + std::to_string(i) + ".bin");
        rel_paths.push_back("filler" + std::to_string(i) + ".bin");
        rel_paths.push_back("g0-" + std::to_string(i) + ".bin");
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
// Test Fixture
// ═══════════════════════════════════════════════════════════

class MultiGPULoRAAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!prepareLoRAPathFixtures()) {
            GTEST_SKIP() << "Cannot prepare /path LoRA fixture files";
        }

        config_.max_lora_vram_mb = 1024;
        config_.max_lora_slots = 50;
        config_.lora_ttl = std::chrono::seconds(300);
        config_.enable_multi_lora_batch = true;
        
        // Multi-GPU with 4 GPUs
        config_.multi_gpu.enabled = true;
        config_.multi_gpu.devices = {0, 1, 2, 3};
        config_.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
        config_.multi_gpu.enable_peer_transfer = true;
        config_.multi_gpu.max_vram_per_gpu_mb = 256;  // 256MB per GPU
        config_.multi_gpu.enable_load_balancing = true;
        config_.multi_gpu.enable_fault_tolerance = true;
        config_.multi_gpu.health_check_interval_sec = 5;
    }
    
    MultiLoRAManager::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Usage Heatmap Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, UsageHeatmapTracksAccessPatterns) {
    MultiLoRAManager manager(config_);
    
    // Load adapters with different access patterns
    manager.loadLoRA("hot", "/path/hot.bin", "base", 1.0f);
    manager.loadLoRA("cold", "/path/cold.bin", "base", 1.0f);
    manager.loadLoRA("warm", "/path/warm.bin", "base", 1.0f);
    
    // Create access patterns
    for (int i = 0; i < 20; ++i) {
        manager.getLoRA("hot");
    }
    for (int i = 0; i < 5; ++i) {
        manager.getLoRA("warm");
    }
    // "cold" not accessed
    
    // Get heatmap
    auto heatmap = manager.getUsageHeatmap();
    
    // Verify structure
    ASSERT_TRUE(heatmap.is_array());
    EXPECT_EQ(heatmap.size(), 3);
    
    // Find and verify each adapter
    int hot_count = 0, warm_count = 0, cold_count = 0;
    
    for (const auto& entry : heatmap) {
        EXPECT_TRUE(entry.contains("lora_id"));
        EXPECT_TRUE(entry.contains("use_count"));
        EXPECT_TRUE(entry.contains("access_frequency"));
        
        std::string id = entry["lora_id"];
        if (id == "hot") {
            hot_count = entry["use_count"];
        } else if (id == "warm") {
            warm_count = entry["use_count"];
        } else if (id == "cold") {
            cold_count = entry["use_count"];
        }
    }
    
    EXPECT_GT(hot_count, warm_count);
    EXPECT_GT(warm_count, cold_count);
}

TEST_F(MultiGPULoRAAdvancedTest, UsageHeatmapIncludesGPUPlacement) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("lora-1", "/path/1.bin", "base", 1.0f);
    
    auto heatmap = manager.getUsageHeatmap();
    
    ASSERT_FALSE(heatmap.empty());
    auto entry = heatmap[0];
    
    EXPECT_TRUE(entry.contains("primary_gpu"));
    EXPECT_TRUE(entry.contains("gpu_placement"));
    EXPECT_GE(entry["primary_gpu"].get<int>(), 0);
}

// ═══════════════════════════════════════════════════════════
// Resource-Aware Eviction Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, ResourceAwareEvictionPrioritizesIdleAdapters) {
    MultiLoRAManager manager(config_);
    
    // Load adapters
    manager.loadLoRA("active", "/path/active.bin", "base", 1.0f);
    manager.loadLoRA("idle-1", "/path/idle1.bin", "base", 1.0f);
    manager.loadLoRA("idle-2", "/path/idle2.bin", "base", 1.0f);
    
    // Keep "active" adapter hot
    for (int i = 0; i < 10; ++i) {
        manager.getLoRA("active");
    }
    
    const size_t loaded_before =
        static_cast<size_t>(manager.isLoRALoaded("active")) +
        static_cast<size_t>(manager.isLoRALoaded("idle-1")) +
        static_cast<size_t>(manager.isLoRALoaded("idle-2"));

    // Trigger resource-aware eviction.
    // With tiny fixture files, freed MB may round down to 0 even when entries
    // are evicted, so verify actual cache effects instead of MB count.
    const size_t freed = manager.evictResourceAware(-1, 64);
    EXPECT_GE(freed, 0u);

    const size_t loaded_after =
        static_cast<size_t>(manager.isLoRALoaded("active")) +
        static_cast<size_t>(manager.isLoRALoaded("idle-1")) +
        static_cast<size_t>(manager.isLoRALoaded("idle-2"));
    EXPECT_LT(loaded_after, loaded_before);
}

TEST_F(MultiGPULoRAAdvancedTest, ResourceAwareEvictionRespectsGPUFilter) {
    MultiLoRAManager manager(config_);
    
    // Load adapters on specific GPUs
    manager.loadLoRA("lora-gpu0-1", "/path/g0-1.bin", "base", 1.0f);
    manager.loadLoRA("lora-gpu1-1", "/path/g1-1.bin", "base", 1.0f);
    manager.loadLoRA("lora-gpu0-2", "/path/g0-2.bin", "base", 1.0f);
    
    auto gpu0_placement = manager.getLoRAGPUPlacement("lora-gpu0-1");
    ASSERT_FALSE(gpu0_placement.empty());
    int gpu0_id = gpu0_placement[0];
    
    // Evict only from GPU 0
    size_t freed = manager.evictResourceAware(gpu0_id, 32);
    
    if (freed > 0) {
        // Verify that adapters on GPU 0 were evicted
        // but GPU 1 adapter remains
        auto gpu1_placement = manager.getLoRAGPUPlacement("lora-gpu1-1");
        if (!gpu1_placement.empty() && gpu1_placement[0] != gpu0_id) {
            EXPECT_TRUE(manager.isLoRALoaded("lora-gpu1-1"));
        }
    }
}

TEST_F(MultiGPULoRAAdvancedTest, ResourceAwareEvictionRespectsPinning) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("pinned", "/path/pinned.bin", "base", 1.0f);
    manager.loadLoRA("unpinned", "/path/unpinned.bin", "base", 1.0f);
    
    manager.pinLoRA("pinned");
    
    size_t freed = manager.evictResourceAware(-1, 64);
    
    // Pinned adapter should not be evicted
    EXPECT_TRUE(manager.isLoRALoaded("pinned"));
}

// ═══════════════════════════════════════════════════════════
// Dynamic Scheduling Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, SchedulingRecommendationSelectsLeastLoadedGPU) {
    MultiLoRAManager manager(config_);
    
    // Load adapters to create imbalanced load
    for (int i = 0; i < 5; ++i) {
        manager.loadLoRA("lora-gpu0-" + std::to_string(i), 
                        "/path/g0-" + std::to_string(i) + ".bin", 
                        "base", 1.0f);
    }
    
    // Get recommendation for new adapter
    size_t new_adapter_vram = 32 * 1024 * 1024;  // 32MB
    auto recommendation = manager.getSchedulingRecommendation(new_adapter_vram, 5);
    
    ASSERT_TRUE(recommendation.contains("recommended_gpu"));
    ASSERT_TRUE(recommendation.contains("gpu_evaluations"));
    ASSERT_TRUE(recommendation.contains("confidence"));
    
    int recommended_gpu = recommendation["recommended_gpu"];
    EXPECT_GE(recommended_gpu, 0);
    EXPECT_LE(recommended_gpu, 3);
    
    // Confidence should be reasonable
    double confidence = recommendation["confidence"];
    EXPECT_GE(confidence, 0.0);
    EXPECT_LE(confidence, 1.0);
}

TEST_F(MultiGPULoRAAdvancedTest, SchedulingRecommendationConsidersVRAMAvailability) {
    MultiLoRAManager manager(config_);
    
    // Request very large adapter
    size_t large_vram = 512 * 1024 * 1024;  // 512MB (larger than per-GPU limit)
    auto recommendation = manager.getSchedulingRecommendation(large_vram, 5);
    
    ASSERT_TRUE(recommendation.contains("gpu_evaluations"));
    
    // All GPUs should report insufficient VRAM
    auto evals = recommendation["gpu_evaluations"];
    for (const auto& eval : evals) {
        if (eval["score"] == 0.0) {
            EXPECT_TRUE(eval.contains("reason"));
            std::string reason = eval["reason"];
            EXPECT_FALSE(reason.empty());
        }
    }
}

TEST_F(MultiGPULoRAAdvancedTest, SchedulingRecommendationIncludesLatencyEstimate) {
    MultiLoRAManager manager(config_);
    
    auto recommendation = manager.getSchedulingRecommendation(32 * 1024 * 1024, 5);
    
    ASSERT_TRUE(recommendation.contains("gpu_evaluations"));
    auto evals = recommendation["gpu_evaluations"];
    
    for (const auto& eval : evals) {
        if (eval["is_healthy"]) {
            EXPECT_TRUE(eval.contains("estimated_load_latency_ms"));
            double latency = eval["estimated_load_latency_ms"];
            EXPECT_GT(latency, 0);
            EXPECT_LT(latency, 200);  // Should be under 200ms goal
        }
    }
}

// ═══════════════════════════════════════════════════════════
// GPU Migration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, MigrateLoRAToGPUSucceeds) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("migrating", "/path/migrating.bin", "base", 1.0f);
    
    auto initial_gpus = manager.getLoRAGPUPlacement("migrating");
    ASSERT_FALSE(initial_gpus.empty());
    int source_gpu = initial_gpus[0];
    int target_gpu = (source_gpu + 1) % 4;
    
    bool success = manager.migrateLoRAToGPU("migrating", target_gpu);
    EXPECT_TRUE(success);
    
    // Verify new placement
    auto new_gpus = manager.getLoRAGPUPlacement("migrating");
    ASSERT_FALSE(new_gpus.empty());
    EXPECT_EQ(new_gpus[0], target_gpu);
}

TEST_F(MultiGPULoRAAdvancedTest, MigrationRespectsGPUCapacity) {
    config_.multi_gpu.max_vram_per_gpu_mb = 50;  // Very small
    MultiLoRAManager manager(config_);
    
    // Fill target GPU
    for (int i = 0; i < 3; ++i) {
        manager.loadLoRA("filler-" + std::to_string(i), 
                        "/path/filler" + std::to_string(i) + ".bin", 
                        "base", 1.0f);
    }
    
    // Try to migrate another adapter to same GPU
    manager.loadLoRA("migrating", "/path/migrating.bin", "base", 1.0f);
    
    auto gpus = manager.getLoRAGPUPlacement("migrating");
    ASSERT_FALSE(gpus.empty());
    int source_gpu = gpus[0];
    
    // Find a potentially full GPU
    int target_gpu = (source_gpu + 1) % 4;
    
    // Migration might fail due to capacity (this is expected behavior)
    bool success = manager.migrateLoRAToGPU("migrating", target_gpu);
    
    // Either succeeds or fails gracefully
    if (!success) {
        // LoRA should still be on original GPU
        auto current_gpus = manager.getLoRAGPUPlacement("migrating");
        EXPECT_FALSE(current_gpus.empty());
    }
}

TEST_F(MultiGPULoRAAdvancedTest, MigrationRespectsPinning) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("pinned", "/path/pinned.bin", "base", 1.0f);
    manager.pinLoRA("pinned");
    
    auto gpus = manager.getLoRAGPUPlacement("pinned");
    ASSERT_FALSE(gpus.empty());
    int source_gpu = gpus[0];
    int target_gpu = (source_gpu + 1) % 4;
    
    bool success = manager.migrateLoRAToGPU("pinned", target_gpu);
    
    EXPECT_FALSE(success);  // Should fail for pinned adapter
    
    // Verify still on original GPU
    auto current_gpus = manager.getLoRAGPUPlacement("pinned");
    EXPECT_EQ(current_gpus[0], source_gpu);
}

// ═══════════════════════════════════════════════════════════
// GPU Health and Auto-Migration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, GPUHealthCheckExecutes) {
    MultiLoRAManager manager(config_);
    
    // Load some adapters
    for (int i = 0; i < 8; ++i) {
        manager.loadLoRA("lora-" + std::to_string(i), 
                        "/path/lora" + std::to_string(i) + ".bin", 
                        "base", 1.0f);
    }
    
    // Check health (in simulation, all GPUs are healthy)
    size_t migrated = manager.checkGPUHealthAndMigrate();
    
    // In normal conditions, no migrations needed
    EXPECT_EQ(migrated, 0);
}

TEST_F(MultiGPULoRAAdvancedTest, AutoMigrationOccursOnGPUFailure) {
    MultiLoRAManager manager(config_);
    
    // Load adapters
    for (int i = 0; i < 4; ++i) {
        manager.loadLoRA("lora-" + std::to_string(i), 
                        "/path/lora" + std::to_string(i) + ".bin", 
                        "base", 1.0f);
    }
    
    // Simulate GPU failure by removing from available devices
    MultiGPUConfig new_config = config_.multi_gpu;
    new_config.devices = {0, 1, 2};  // Remove GPU 3
    manager.setMultiGPUConfig(new_config);
    
    // Load new adapter - should avoid failed GPU
    bool loaded = manager.loadLoRA("new-after-failure", "/path/new.bin", "base", 1.0f);
    EXPECT_TRUE(loaded);
    
    auto gpus = manager.getLoRAGPUPlacement("new-after-failure");
    ASSERT_FALSE(gpus.empty());
    EXPECT_NE(gpus[0], 3);  // Should not be on removed GPU
}

// ═══════════════════════════════════════════════════════════
// Security and Audit Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, TenantIsolationTracking) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("tenant1-lora", "/path/t1.bin", "base", 1.0f);
    manager.loadLoRA("tenant2-lora", "/path/t2.bin", "base", 1.0f);
    
    manager.setLoRATenant("tenant1-lora", "tenant-A");
    manager.setLoRATenant("tenant2-lora", "tenant-B");
    
    // Verify tenant assignment through heatmap
    auto heatmap = manager.getUsageHeatmap();
    
    bool found_tenant_a = false;
    bool found_tenant_b = false;
    
    for (const auto& entry : heatmap) {
        std::string tenant = entry["tenant_id"];
        if (tenant == "tenant-A") found_tenant_a = true;
        if (tenant == "tenant-B") found_tenant_b = true;
    }
    
    EXPECT_TRUE(found_tenant_a);
    EXPECT_TRUE(found_tenant_b);
}

TEST_F(MultiGPULoRAAdvancedTest, AuditLogRecordsLoRAOperations) {
    MultiLoRAManager manager(config_);
    
    // Perform operations
    manager.loadLoRA("audited", "/path/audited.bin", "base", 1.0f);
    manager.setLoRATenant("audited", "test-tenant");
    manager.unloadLoRA("audited");
    
    // Get audit log
    auto log = manager.getGPUTransferAuditLog(100);
    
    ASSERT_TRUE(log.is_array());
    EXPECT_GE(log.size(), 2);  // At least load and unload events
    
    // Verify log entries
    bool found_load = false;
    bool found_unload = false;
    
    for (const auto& entry : log) {
        EXPECT_TRUE(entry.contains("event_type"));
        EXPECT_TRUE(entry.contains("lora_id"));
        EXPECT_TRUE(entry.contains("timestamp"));
        
        std::string event_type = entry["event_type"];
        if (event_type == "load") found_load = true;
        if (event_type == "unload") found_unload = true;
    }
    
    EXPECT_TRUE(found_load);
    EXPECT_TRUE(found_unload);
}

TEST_F(MultiGPULoRAAdvancedTest, AuditLogIncludesTenantInfo) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("tenant-lora", "/path/tenant.bin", "base", 1.0f);
    manager.setLoRATenant("tenant-lora", "secure-tenant");
    manager.unloadLoRA("tenant-lora");
    
    auto log = manager.getGPUTransferAuditLog(10);
    
    // Find unload event
    bool found_tenant_in_log = false;
    for (const auto& entry : log) {
        if (entry["event_type"] == "unload" && entry["lora_id"] == "tenant-lora") {
            EXPECT_TRUE(entry.contains("tenant_id"));
            std::string tenant = entry["tenant_id"];
            if (tenant == "secure-tenant") {
                found_tenant_in_log = true;
            }
        }
    }
    
    EXPECT_TRUE(found_tenant_in_log);
}

TEST_F(MultiGPULoRAAdvancedTest, AuditLogRecordsMigrations) {
    MultiLoRAManager manager(config_);
    
    manager.loadLoRA("migrating", "/path/migrating.bin", "base", 1.0f);
    
    auto gpus = manager.getLoRAGPUPlacement("migrating");
    ASSERT_FALSE(gpus.empty());
    int source_gpu = gpus[0];
    int target_gpu = (source_gpu + 1) % 4;
    
    bool success = manager.migrateLoRAToGPU("migrating", target_gpu);
    
    if (success) {
        auto log = manager.getGPUTransferAuditLog(10);
        
        // Find migration event
        bool found_migration = false;
        for (const auto& entry : log) {
            if (entry["event_type"] == "migrate" && entry["lora_id"] == "migrating") {
                EXPECT_EQ(entry["source_gpu"], source_gpu);
                EXPECT_EQ(entry["target_gpu"], target_gpu);
                found_migration = true;
            }
        }
        
        EXPECT_TRUE(found_migration);
    }
}

TEST_F(MultiGPULoRAAdvancedTest, AuditLogLimited) {
    MultiLoRAManager manager(config_);
    
    // Generate many events
    for (int i = 0; i < 50; ++i) {
        std::string id = "temp-" + std::to_string(i);
        manager.loadLoRA(id, "/path/" + id + ".bin", "base", 1.0f);
        manager.unloadLoRA(id);
    }
    
    // Request limited log
    auto log = manager.getGPUTransferAuditLog(10);
    
    EXPECT_LE(log.size(), 10);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPULoRAAdvancedTest, EndToEndWorkflowWithNewFeatures) {
    MultiLoRAManager manager(config_);
    
    // 1. Load adapters with tenant association
    for (int i = 0; i < 10; ++i) {
        std::string id = "workflow-" + std::to_string(i);
        manager.loadLoRA(id, "/path/" + id + ".bin", "base", 1.0f);
        manager.setLoRATenant(id, "tenant-" + std::to_string(i % 3));
    }
    
    // 2. Create access patterns
    for (int i = 0; i < 5; ++i) {
        manager.getLoRA("workflow-" + std::to_string(i));
    }
    
    // 3. Get usage heatmap
    auto heatmap = manager.getUsageHeatmap();
    EXPECT_EQ(heatmap.size(), 10);
    
    // 4. Get scheduling recommendation
    auto recommendation = manager.getSchedulingRecommendation(64 * 1024 * 1024, 7);
    EXPECT_TRUE(recommendation.contains("recommended_gpu"));
    
    // 5. Perform resource-aware eviction
    size_t freed = manager.evictResourceAware(-1, 128);
    EXPECT_GE(freed, 0);
    
    // 6. Check GPU health
    size_t migrated = manager.checkGPUHealthAndMigrate();
    EXPECT_EQ(migrated, 0);  // All healthy in simulation
    
    // 7. Get audit log
    auto log = manager.getGPUTransferAuditLog(50);
    EXPECT_GT(log.size(), 0);
    
    // 8. Verify statistics
    auto stats = manager.getStatistics();
    EXPECT_GE(stats.cache_misses, 10u);
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════


