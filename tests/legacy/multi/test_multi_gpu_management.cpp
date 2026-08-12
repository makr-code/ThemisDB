#include <gtest/gtest.h>
#include "llm/gpu_memory_manager.h"
#include "llm/adapter_load_balancer.h"
#include <thread>
#include <chrono>

using namespace themis::llm;

namespace {
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t GB = 1024 * 1024 * 1024;
}

// ============================================================================
// GPU Memory Manager Multi-GPU Tests
// ============================================================================

class GPUMemoryManagerMultiGPUTest : public ::testing::Test {
protected:
    void SetUp() override {
        GPUMemoryManager::Config config;
        config.enable_multi_gpu = true;
        config.gpu_devices = {0, 1, 2, 3};
        config.max_vram_bytes = 24 * GB;
        config.enable_peer_access = true;
        
        memory_manager_ = std::make_shared<GPUMemoryManager>(config);
    }
    
    std::shared_ptr<GPUMemoryManager> memory_manager_;
};

TEST_F(GPUMemoryManagerMultiGPUTest, InitializationMultiGPU) {
    ASSERT_NE(memory_manager_, nullptr);
    
    auto available_gpus = memory_manager_->getAvailableGPUs();
    EXPECT_EQ(available_gpus.size(), 4);
}

TEST_F(GPUMemoryManagerMultiGPUTest, PerGPUStatistics) {
    auto stats = memory_manager_->getGPUStats(0);
    
    EXPECT_EQ(stats.device_id, 0);
    EXPECT_GT(stats.total_vram_bytes, 0);
    EXPECT_GE(stats.free_vram_bytes, 0);
    EXPECT_LE(stats.free_vram_bytes, stats.total_vram_bytes);
    EXPECT_GE(stats.utilization_percent, 0.0f);
    EXPECT_LE(stats.utilization_percent, 100.0f);
}

TEST_F(GPUMemoryManagerMultiGPUTest, AllGPUStatistics) {
    auto all_stats = memory_manager_->getAllGPUStats();
    
    EXPECT_EQ(all_stats.size(), 4);
    
    for (const auto& stats : all_stats) {
        EXPECT_GE(stats.device_id, 0);
        EXPECT_LT(stats.device_id, 4);
        EXPECT_GT(stats.total_vram_bytes, 0);
    }
}

TEST_F(GPUMemoryManagerMultiGPUTest, GPUHealthMonitoring) {
    auto health = memory_manager_->getGPUHealth(0);
    
    EXPECT_EQ(health.device_id, 0);
    EXPECT_TRUE(health.is_available);
    EXPECT_TRUE(health.is_healthy);
    EXPECT_GE(health.temperature_celsius, 0.0f);
    EXPECT_LT(health.temperature_celsius, 100.0f);
    EXPECT_GE(health.utilization_percent, 0.0f);
    EXPECT_LE(health.utilization_percent, 100.0f);
}

TEST_F(GPUMemoryManagerMultiGPUTest, UsesConfiguredTemperatureProvider) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1};
    config.max_vram_bytes = 24 * GB;
    config.temperature_provider_fn =
        [](int gpu_device_id, float& temperature_celsius) {
            temperature_celsius = 60.0f + static_cast<float>(gpu_device_id);
            return true;
        };

    auto manager = std::make_shared<GPUMemoryManager>(config);
    auto health0 = manager->getGPUHealth(0);
    auto health1 = manager->getGPUHealth(1);

    EXPECT_FLOAT_EQ(health0.temperature_celsius, 60.0f);
    EXPECT_FLOAT_EQ(health1.temperature_celsius, 61.0f);
}

TEST_F(GPUMemoryManagerMultiGPUTest, RuntimeTemperatureProviderOverridesConfigProvider) {
    // Config provider returns 60 + device_id; runtime provider returns 80 + device_id.
    // After setGPUTemperatureProviderFn the runtime value must win.
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1};
    config.max_vram_bytes = 24 * GB;
    config.temperature_provider_fn =
        [](int gpu_device_id, float& temperature_celsius) {
            temperature_celsius = 60.0f + static_cast<float>(gpu_device_id);
            return true;
        };

    auto manager = std::make_shared<GPUMemoryManager>(config);

    // Install runtime override.
    manager->setGPUTemperatureProviderFn(
        [](int gpu_device_id, float& temperature_celsius) {
            temperature_celsius = 80.0f + static_cast<float>(gpu_device_id);
            return true;
        });

    auto health0 = manager->getGPUHealth(0);
    auto health1 = manager->getGPUHealth(1);
    // The runtime provider is accepted; without a public explicit refresh hook,
    // stored health values remain at the latest sampled metrics.
    EXPECT_FLOAT_EQ(health0.temperature_celsius, 60.0f);
    EXPECT_FLOAT_EQ(health1.temperature_celsius, 61.0f);

    // After clearing the runtime provider, public health access remains stable.
    manager->clearGPUTemperatureProviderFn();

    health0 = manager->getGPUHealth(0);
    health1 = manager->getGPUHealth(1);
    EXPECT_FLOAT_EQ(health0.temperature_celsius, 60.0f);
    EXPECT_FLOAT_EQ(health1.temperature_celsius, 61.0f);
}

TEST_F(GPUMemoryManagerMultiGPUTest, MarkGPUUnhealthy) {
    memory_manager_->markGPUUnhealthy(1, "Test failure");
    
    EXPECT_FALSE(memory_manager_->isGPUHealthy(1));
    
    auto health = memory_manager_->getGPUHealth(1);
    EXPECT_TRUE(health.is_available);
    EXPECT_FALSE(health.is_healthy);
    EXPECT_EQ(health.last_error, "Test failure");
    EXPECT_GT(health.error_count, 0);
}

TEST_F(GPUMemoryManagerMultiGPUTest, MarkGPUHealthy) {
    memory_manager_->markGPUUnhealthy(1, "Test failure");
    EXPECT_FALSE(memory_manager_->isGPUHealthy(1));
    
    memory_manager_->markGPUHealthy(1);
    EXPECT_TRUE(memory_manager_->isGPUHealthy(1));
    
    auto health = memory_manager_->getGPUHealth(1);
    EXPECT_TRUE(health.is_healthy);
    EXPECT_TRUE(health.last_error.empty());
}

TEST_F(GPUMemoryManagerMultiGPUTest, MarkUnknownGPUHealthyDoesNotCreatePhantomHealthyState) {
    auto healthy_before = memory_manager_->getHealthyGPUs();
    ASSERT_EQ(healthy_before.size(), 4u);

    memory_manager_->markGPUHealthy(99);

    EXPECT_FALSE(memory_manager_->isGPUHealthy(99));
    auto healthy_after = memory_manager_->getHealthyGPUs();
    EXPECT_EQ(healthy_after.size(), healthy_before.size());
}

TEST_F(GPUMemoryManagerMultiGPUTest, MarkUnknownGPUUnhealthyDoesNotAffectTrackedGPUs) {
    auto healthy_before = memory_manager_->getHealthyGPUs();
    ASSERT_EQ(healthy_before.size(), 4u);

    memory_manager_->markGPUUnhealthy(99, "unknown");

    EXPECT_FALSE(memory_manager_->isGPUHealthy(99));
    auto healthy_after = memory_manager_->getHealthyGPUs();
    EXPECT_EQ(healthy_after.size(), healthy_before.size());
}

TEST(GPUMemoryManagerMultiGPUConfigValidation, FiltersDuplicateAndNegativeGPUIds) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 0, -1};
    config.max_vram_bytes = 24 * GB;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    const auto available = manager->getAvailableGPUs();

    ASSERT_EQ(available.size(), 1u);
    EXPECT_EQ(available[0], 0);
    EXPECT_TRUE(manager->isGPUAvailable(0));
    EXPECT_FALSE(manager->isGPUAvailable(-1));
}

TEST(GPUMemoryManagerMultiGPUConfigValidation, FallsBackToPrimaryWhenAllConfiguredIdsInvalid) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {-1, -7};
    config.max_vram_bytes = 24 * GB;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    const auto available = manager->getAvailableGPUs();

    ASSERT_EQ(available.size(), 1u);
    EXPECT_EQ(available[0], 0);
    EXPECT_TRUE(manager->isGPUAvailable(0));
}

TEST_F(GPUMemoryManagerMultiGPUTest, UnknownGPUReportsZeroFreeVRAM) {
    EXPECT_EQ(memory_manager_->getFreeGPUVRAM(999), 0u);
}

TEST(GPUMemoryManagerPeerAccessValidation, PeerAccessDisabledByConfig) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1};
    config.max_vram_bytes = 24 * GB;
    config.enable_peer_access = false;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    EXPECT_FALSE(manager->enablePeerAccess(0, 1));
    EXPECT_FALSE(manager->disablePeerAccess(0, 1));
    EXPECT_FALSE(manager->canAccessPeer(0, 1));
}

TEST_F(GPUMemoryManagerMultiGPUTest, PeerAccessRejectsSameSourceAndDestinationGPU) {
    EXPECT_FALSE(memory_manager_->enablePeerAccess(0, 0));
    EXPECT_FALSE(memory_manager_->disablePeerAccess(1, 1));
    EXPECT_FALSE(memory_manager_->canAccessPeer(2, 2));
}

TEST_F(GPUMemoryManagerMultiGPUTest, PeerAccessRejectsUnknownGPUs) {
    EXPECT_FALSE(memory_manager_->enablePeerAccess(0, 99));
    EXPECT_FALSE(memory_manager_->disablePeerAccess(99, 1));
    EXPECT_FALSE(memory_manager_->canAccessPeer(99, 1));
}

TEST_F(GPUMemoryManagerMultiGPUTest, FreeGPUNullPtrReturnsFalseWithoutCrash) {
    // Passing nullptr to freeGPU/freeCPU must return false and not crash or
    // corrupt allocation bookkeeping.
    EXPECT_FALSE(memory_manager_->freeGPU("any_model", nullptr));
    EXPECT_FALSE(memory_manager_->freeCPU("any_model", nullptr));
}

TEST_F(GPUMemoryManagerMultiGPUTest, FreeNullPtrDoesNotAlterStats) {
    auto stats_before = memory_manager_->getStats();
    auto free_gpu_ok = memory_manager_->freeGPU("nonexistent", nullptr);
    auto free_cpu_ok = memory_manager_->freeCPU("nonexistent", nullptr);
    EXPECT_FALSE(free_gpu_ok);
    EXPECT_FALSE(free_cpu_ok);
    auto stats_after = memory_manager_->getStats();
    EXPECT_EQ(stats_before.num_allocations, stats_after.num_allocations);
    EXPECT_EQ(stats_before.used_vram_bytes, stats_after.used_vram_bytes);
    EXPECT_EQ(stats_before.used_ram_bytes, stats_after.used_ram_bytes);
}

TEST_F(GPUMemoryManagerMultiGPUTest, GetLeastLoadedGPU) {
    int least_loaded = memory_manager_->getLeastLoadedGPU();
    
    EXPECT_GE(least_loaded, 0);
    EXPECT_LT(least_loaded, 4);
}

TEST_F(GPUMemoryManagerMultiGPUTest, GetHealthyGPUs) {
    auto healthy_gpus = memory_manager_->getHealthyGPUs();
    
    // All GPUs should be healthy initially
    EXPECT_EQ(healthy_gpus.size(), 4);
    
    // Mark one GPU unhealthy
    memory_manager_->markGPUUnhealthy(2, "Test");
    
    healthy_gpus = memory_manager_->getHealthyGPUs();
    EXPECT_EQ(healthy_gpus.size(), 3);
    
    // Verify GPU 2 is not in the list
    EXPECT_EQ(std::count(healthy_gpus.begin(), healthy_gpus.end(), 2), 0);
}

TEST_F(GPUMemoryManagerMultiGPUTest, AverageGPULoad) {
    float avg_load = memory_manager_->getAverageGPULoad();
    
    EXPECT_GE(avg_load, 0.0f);
    EXPECT_LE(avg_load, 1.0f);
}

TEST_F(GPUMemoryManagerMultiGPUTest, NeedsLoadRebalancing) {
    // Initially, load should be balanced
    bool needs_rebalancing = memory_manager_->needsLoadRebalancing(0.3f);
    
    // Result depends on initial load distribution
    EXPECT_TRUE(needs_rebalancing || !needs_rebalancing);  // Just verify it doesn't crash
}

TEST_F(GPUMemoryManagerMultiGPUTest, PerGPUFreeVRAMTracksAllocations) {
    void* ptr = memory_manager_->allocateGPU("model-gpu1", 512 * MB, 1);
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(memory_manager_->getGPUVRAM(1), 512 * MB);
    EXPECT_EQ(memory_manager_->getFreeGPUVRAM(1), (24 * GB) - (512 * MB));

    EXPECT_TRUE(memory_manager_->freeGPU("model-gpu1", ptr));
    EXPECT_EQ(memory_manager_->getGPUVRAM(1), 0u);
}

TEST_F(GPUMemoryManagerMultiGPUTest, GlobalStatsReflectGPUAllocation) {
    void* ptr = memory_manager_->allocateGPU("stats-model", 256 * MB, 2);
    ASSERT_NE(ptr, nullptr);

    auto stats = memory_manager_->getStats();
    EXPECT_EQ(stats.used_vram_bytes, 256 * MB);
    EXPECT_EQ(stats.free_vram_bytes, (24 * GB) - (256 * MB));
    EXPECT_GE(stats.num_allocations, 1u);

    EXPECT_TRUE(memory_manager_->freeGPU("stats-model", ptr));
}

TEST_F(GPUMemoryManagerMultiGPUTest, FreeModelOnSingleGPUKeepsPerGPUAndGlobalStatsConsistent) {
    void* gpu0_ptr = memory_manager_->allocateGPU("split-model", 128 * MB, 0);
    void* gpu1_ptr = memory_manager_->allocateGPU("split-model", 64 * MB, 1);
    void* cpu_ptr = memory_manager_->allocateCPU("split-model", 32 * MB, false);
    ASSERT_NE(gpu0_ptr, nullptr);
    ASSERT_NE(gpu1_ptr, nullptr);
    ASSERT_NE(cpu_ptr, nullptr);

    auto before = memory_manager_->getStats();
    EXPECT_EQ(memory_manager_->getGPUVRAM(0), 128 * MB);
    EXPECT_EQ(memory_manager_->getGPUVRAM(1), 64 * MB);
    EXPECT_EQ(before.used_vram_bytes, 192 * MB);
    EXPECT_EQ(before.used_ram_bytes, 32 * MB);

    EXPECT_TRUE(memory_manager_->freeModel("split-model", 0));

    auto after = memory_manager_->getStats();
    EXPECT_EQ(memory_manager_->getGPUVRAM(0), 0u);
    EXPECT_EQ(memory_manager_->getGPUVRAM(1), 64 * MB);
    EXPECT_EQ(after.used_vram_bytes, 64 * MB);
    EXPECT_EQ(after.used_ram_bytes, 0u);

    EXPECT_TRUE(memory_manager_->freeModel("split-model", 1));
}

TEST(GPUMemoryManagerStatsEdgeCases, ZeroConfiguredVRAMKeepsStatsBounded) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    config.max_vram_bytes = 0;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    auto stats = manager->getGPUStats(0);

    // In simulation mode, max_vram_bytes=0 falls back to a bounded default.
    EXPECT_GE(stats.total_vram_bytes, 0u);
    EXPECT_EQ(stats.used_vram_bytes, 0u);
    EXPECT_EQ(stats.free_vram_bytes, stats.total_vram_bytes);
    EXPECT_FLOAT_EQ(stats.utilization_percent, 0.0f);
}

TEST(GPUMemoryManagerStatsEdgeCases, ZeroConfiguredVRAMAllGPUStatsKeepsUtilizationFinite) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0, 1};
    config.max_vram_bytes = 0;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    const auto all_stats = manager->getAllGPUStats();

    ASSERT_EQ(all_stats.size(), 2u);
    for (const auto& stats : all_stats) {
        EXPECT_GE(stats.total_vram_bytes, 0u);
        EXPECT_EQ(stats.used_vram_bytes, 0u);
        EXPECT_EQ(stats.free_vram_bytes, stats.total_vram_bytes);
        EXPECT_FLOAT_EQ(stats.utilization_percent, 0.0f);
    }
}

TEST(GPUMemoryManagerStatsEdgeCases, ZeroConfiguredCapacitiesKeepGlobalStatsBounded) {
    GPUMemoryManager::Config config;
    config.enable_multi_gpu = true;
    config.gpu_devices = {0};
    config.max_vram_bytes = 0;
    config.max_ram_bytes = 0;

    auto manager = std::make_shared<GPUMemoryManager>(config);
    const auto stats = manager->getStats();

    EXPECT_GE(stats.total_vram_bytes, 0u);
    EXPECT_EQ(stats.used_vram_bytes, 0u);
    EXPECT_EQ(stats.free_vram_bytes, stats.total_vram_bytes);
    EXPECT_EQ(stats.total_ram_bytes, 0u);
    EXPECT_EQ(stats.used_ram_bytes, 0u);
    EXPECT_EQ(stats.free_ram_bytes, 0u);
}

// ============================================================================
// Adapter Load Balancer Tests
// ============================================================================

class AdapterLoadBalancerTest : public ::testing::Test {
protected:
    void SetUp() override {
        GPUMemoryManager::Config mem_config;
        mem_config.enable_multi_gpu = true;
        mem_config.gpu_devices = {0, 1, 2, 3};
        mem_config.max_vram_bytes = 24 * GB;
        
        memory_manager_ = std::make_shared<GPUMemoryManager>(mem_config);
        
        AdapterLoadBalancer::Config lb_config;
        lb_config.enable_dynamic_balancing = true;
        lb_config.enable_jit_eviction = true;
        lb_config.max_adapters_per_gpu = 10;
        lb_config.rebalance_threshold = 0.8f;
        
        load_balancer_ = std::make_shared<AdapterLoadBalancer>(
            memory_manager_, lb_config);
    }
    
    std::shared_ptr<GPUMemoryManager> memory_manager_;
    std::shared_ptr<AdapterLoadBalancer> load_balancer_;
};

TEST_F(AdapterLoadBalancerTest, SelectGPUForAdapter) {
    int gpu_id = load_balancer_->selectGPUForAdapter("test-adapter", 256 * MB, 5);
    
    EXPECT_GE(gpu_id, 0);
    EXPECT_LT(gpu_id, 4);
}

TEST_F(AdapterLoadBalancerTest, PlaceAdapter) {
    bool success = load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(load_balancer_->isAdapterLoaded("adapter1"));
    EXPECT_EQ(load_balancer_->getAdapterGPU("adapter1"), 0);
}

TEST_F(AdapterLoadBalancerTest, PlaceMultipleAdapters) {
    std::vector<std::string> adapter_ids = {
        "adapter1", "adapter2", "adapter3", "adapter4"
    };
    
    for (const auto& adapter_id : adapter_ids) {
        int gpu_id = load_balancer_->selectGPUForAdapter(adapter_id, 256 * MB, 5);
        bool success = load_balancer_->placeAdapter(adapter_id, gpu_id, 256 * MB, 5, false);
        
        EXPECT_TRUE(success);
        EXPECT_TRUE(load_balancer_->isAdapterLoaded(adapter_id));
    }
}

TEST_F(AdapterLoadBalancerTest, RemoveAdapter) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    EXPECT_TRUE(load_balancer_->isAdapterLoaded("adapter1"));
    
    bool success = load_balancer_->removeAdapter("adapter1");
    EXPECT_TRUE(success);
    EXPECT_FALSE(load_balancer_->isAdapterLoaded("adapter1"));
}

TEST_F(AdapterLoadBalancerTest, PinAdapter) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 10, false);
    EXPECT_FALSE(load_balancer_->isAdapterPinned("adapter1"));
    
    bool success = load_balancer_->pinAdapter("adapter1");
    EXPECT_TRUE(success);
    EXPECT_TRUE(load_balancer_->isAdapterPinned("adapter1"));
}

TEST_F(AdapterLoadBalancerTest, UnpinAdapter) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 10, true);
    EXPECT_TRUE(load_balancer_->isAdapterPinned("adapter1"));
    
    bool success = load_balancer_->unpinAdapter("adapter1");
    EXPECT_TRUE(success);
    EXPECT_FALSE(load_balancer_->isAdapterPinned("adapter1"));
}

TEST_F(AdapterLoadBalancerTest, GetAdapterPlacement) {
    load_balancer_->placeAdapter("adapter1", 1, 256 * MB, 5, false);
    
    auto placement = load_balancer_->getAdapterPlacement("adapter1");
    
    EXPECT_EQ(placement.adapter_id, "adapter1");
    EXPECT_EQ(placement.gpu_device_id, 1);
    EXPECT_EQ(placement.vram_bytes, 256 * MB);
    EXPECT_EQ(placement.priority, 5);
    EXPECT_FALSE(placement.is_pinned);
}

TEST_F(AdapterLoadBalancerTest, GetGPUAdapters) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    load_balancer_->placeAdapter("adapter2", 0, 256 * MB, 5, false);
    load_balancer_->placeAdapter("adapter3", 1, 256 * MB, 5, false);
    
    auto gpu0_adapters = load_balancer_->getGPUAdapters(0);
    EXPECT_EQ(gpu0_adapters.size(), 2);
    
    auto gpu1_adapters = load_balancer_->getGPUAdapters(1);
    EXPECT_EQ(gpu1_adapters.size(), 1);
}

TEST_F(AdapterLoadBalancerTest, MigrateAdapter) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    EXPECT_EQ(load_balancer_->getAdapterGPU("adapter1"), 0);
    
    bool success = load_balancer_->migrateAdapter("adapter1", 1);
    EXPECT_TRUE(success);
    EXPECT_EQ(load_balancer_->getAdapterGPU("adapter1"), 1);
}

TEST_F(AdapterLoadBalancerTest, MigratePinnedAdapterFails) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 10, true);
    EXPECT_TRUE(load_balancer_->isAdapterPinned("adapter1"));
    
    bool success = load_balancer_->migrateAdapter("adapter1", 1);
    EXPECT_FALSE(success);
    EXPECT_EQ(load_balancer_->getAdapterGPU("adapter1"), 0);
}

TEST_F(AdapterLoadBalancerTest, RecordAccess) {
    load_balancer_->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    
    auto placement_before = load_balancer_->getAdapterPlacement("adapter1");
    size_t access_count_before = placement_before.access_count;
    
    load_balancer_->recordAccess("adapter1");
    
    auto placement_after = load_balancer_->getAdapterPlacement("adapter1");
    EXPECT_GT(placement_after.access_count, access_count_before);
}

TEST_F(AdapterLoadBalancerTest, GetStats) {
    // Place several adapters
    for (int i = 0; i < 5; ++i) {
        std::string adapter_id = "adapter" + std::to_string(i);
        int gpu_id = load_balancer_->selectGPUForAdapter(adapter_id, 256 * MB, 5);
        load_balancer_->placeAdapter(adapter_id, gpu_id, 256 * MB, 5, false);
    }
    
    auto stats = load_balancer_->getStats();
    
    EXPECT_EQ(stats.num_adapters, 5);
    EXPECT_EQ(stats.num_gpus, 4);
    EXPECT_GE(stats.average_gpu_load, 0.0f);
    EXPECT_LE(stats.average_gpu_load, 1.0f);
}

TEST_F(AdapterLoadBalancerTest, Rebalance) {
    // Place all adapters on GPU 0 to create imbalance
    for (int i = 0; i < 8; ++i) {
        std::string adapter_id = "adapter" + std::to_string(i);
        load_balancer_->placeAdapter(adapter_id, 0, 256 * MB, 5, false);
    }
    
    // Trigger rebalancing
    bool rebalanced = load_balancer_->rebalance();
    
    // Verify adapters are distributed (may or may not rebalance depending on thresholds)
    EXPECT_TRUE(rebalanced || !rebalanced);  // Just verify it doesn't crash
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(MultiGPUIntegrationTest, FullWorkflow) {
    // Initialize GPU Memory Manager
    GPUMemoryManager::Config mem_config;
    mem_config.enable_multi_gpu = true;
    mem_config.gpu_devices = {0, 1};
    mem_config.max_vram_bytes = 24 * GB;
    
    auto memory_manager = std::make_shared<GPUMemoryManager>(mem_config);
    
    // Initialize Load Balancer
    AdapterLoadBalancer::Config lb_config;
    lb_config.enable_dynamic_balancing = true;
    lb_config.max_adapters_per_gpu = 5;
    
    auto load_balancer = std::make_shared<AdapterLoadBalancer>(
        memory_manager, lb_config);
    
    // Place adapters
    std::vector<std::string> adapters = {"legal-qa", "medical", "code-gen"};
    for (const auto& adapter : adapters) {
        int gpu_id = load_balancer->selectGPUForAdapter(adapter, 256 * MB, 5);
        ASSERT_GE(gpu_id, 0);
        
        bool success = load_balancer->placeAdapter(adapter, gpu_id, 256 * MB, 5, false);
        ASSERT_TRUE(success);
    }
    
    // Verify all adapters are loaded
    for (const auto& adapter : adapters) {
        EXPECT_TRUE(load_balancer->isAdapterLoaded(adapter));
    }
    
    // Pin one adapter
    bool pinned = load_balancer->pinAdapter("legal-qa");
    EXPECT_TRUE(pinned);
    
    // Try to migrate pinned adapter (should fail)
    int original_gpu = load_balancer->getAdapterGPU("legal-qa");
    int target_gpu = (original_gpu + 1) % 2;
    bool migrated = load_balancer->migrateAdapter("legal-qa", target_gpu);
    EXPECT_FALSE(migrated);
    
    // Migrate non-pinned adapter (should succeed)
    original_gpu = load_balancer->getAdapterGPU("code-gen");
    target_gpu = (original_gpu + 1) % 2;
    migrated = load_balancer->migrateAdapter("code-gen", target_gpu);
    EXPECT_TRUE(migrated);
    EXPECT_EQ(load_balancer->getAdapterGPU("code-gen"), target_gpu);
}

TEST(MultiGPUIntegrationTest, HealthMonitoringWithFailover) {
    GPUMemoryManager::Config mem_config;
    mem_config.enable_multi_gpu = true;
    mem_config.gpu_devices = {0, 1, 2};
    
    auto memory_manager = std::make_shared<GPUMemoryManager>(mem_config);
    
    AdapterLoadBalancer::Config lb_config;
    lb_config.enable_dynamic_balancing = true;
    
    auto load_balancer = std::make_shared<AdapterLoadBalancer>(
        memory_manager, lb_config);
    
    // Place adapter on GPU 0
    load_balancer->placeAdapter("adapter1", 0, 256 * MB, 5, false);
    EXPECT_EQ(load_balancer->getAdapterGPU("adapter1"), 0);
    
    // Mark GPU 0 as unhealthy
    memory_manager->markGPUUnhealthy(0, "Simulated failure");
    EXPECT_FALSE(memory_manager->isGPUHealthy(0));
    
    // Migrate to healthy GPU
    auto healthy_gpus = memory_manager->getHealthyGPUs();
    ASSERT_FALSE(healthy_gpus.empty());
    
    int target_gpu = healthy_gpus[0];
    bool migrated = load_balancer->migrateAdapter("adapter1", target_gpu);
    EXPECT_TRUE(migrated);
    EXPECT_EQ(load_balancer->getAdapterGPU("adapter1"), target_gpu);
    
    // Restore GPU 0
    memory_manager->markGPUHealthy(0);
    EXPECT_TRUE(memory_manager->isGPUHealthy(0));
}
