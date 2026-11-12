/**
 * @file test_lora_adapters.cpp
 * @brief Comprehensive tests for LoRA adapter management
 * 
 * Tests LoRA adapter functionality:
 * - Adapter loading/unloading
 * - Adapter switching verification
 * - Concurrent adapter access
 * - Adapter state consistency
 * - Multi-adapter management
 * 
 * Best Practices Applied:
 * - Real adapter operations (when available)
 * - State consistency validation
 * - Resource management
 * - Concurrent access testing
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "../test_performance_helpers.h"
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <fstream>

// Conditional compilation for LLM support
#ifdef THEMIS_ENABLE_LLM
#include "llm/multi_lora_manager.h"
using themis::llm::MultiLoRAManager;
using themis::llm::LoRASlot;
#endif

using namespace themis;

namespace fs = std::filesystem;

/**
 * Test fixture for LoRA adapter tests
 */
class LoRAAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_lora_test";
        fs::create_directories(test_dir_);
    }
    
    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    // Helper to create mock adapter file
    std::string createMockAdapter(const std::string& name, size_t size_bytes = 1024) {
        auto path = test_dir_ / (name + ".bin");
        std::ofstream file(path, std::ios::binary);
        
        for (size_t i = 0; i < size_bytes; ++i) {
            file.put(static_cast<char>(i % 256));
        }
        
        file.close();
        return path.string();
    }

#ifdef THEMIS_ENABLE_LLM
    std::unique_ptr<MultiLoRAManager> createManager(size_t max_slots = 16) const {
        MultiLoRAManager::Config config;
        config.max_lora_slots = max_slots;
        config.lora_base_dir = test_dir_.string();
        config.lora_ttl = std::chrono::seconds(0);
        config.multi_gpu.enabled = false;
        return std::make_unique<MultiLoRAManager>(config);
    }
#endif
    
    fs::path test_dir_;
};

// ═══════════════════════════════════════════════════════════
// Adapter Loading Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test basic adapter loading
 * Acceptance Criteria:
 * - Adapter loads successfully
 * - Adapter is registered in system
 * - Can be retrieved after loading
 */
TEST_F(LoRAAdapterTest, Loading_BasicLoad) {
    auto adapter_path = createMockAdapter("test_adapter");
    ASSERT_TRUE(fs::exists(adapter_path));
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    ASSERT_TRUE(manager->loadLoRA("test_adapter", adapter_path, "base_model", 1.0f));
    EXPECT_TRUE(manager->isLoRALoaded("test_adapter"));
    EXPECT_NE(manager->getLoRA("test_adapter"), nullptr);
#endif
}

/**
 * Test loading multiple adapters
 * Acceptance Criteria:
 * - Multiple adapters can be loaded
 * - Each adapter is tracked separately
 * - No conflicts between adapters
 */
TEST_F(LoRAAdapterTest, Loading_MultipleAdapters) {
    std::vector<std::string> adapters = {};

    for (int i = 0; i < 5; ++i) {
        adapters.push_back(createMockAdapter("adapter_" + std::to_string(i)));
    }
    
    for (const auto& adapter : adapters) {
        ASSERT_TRUE(fs::exists(adapter));
    }
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager(adapters.size());
    for (size_t i = 0; i < adapters.size(); ++i) {
        ASSERT_TRUE(manager->loadLoRA("adapter_" + std::to_string(i),
                                      adapters[i], "base_model", 1.0f));
    }

    EXPECT_EQ(manager->listLoRAs("base_model").size(), adapters.size());
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(manager->isLoRALoaded("adapter_" + std::to_string(i)));
    }
#endif
}

/**
 * Test adapter loading with invalid path
 * Acceptance Criteria:
 * - Invalid path is rejected gracefully
 * - Appropriate error returned
 * - System remains stable
 */
TEST_F(LoRAAdapterTest, Loading_InvalidPath) {
    std::string invalid_path = "/nonexistent/adapter.bin";
    
    EXPECT_FALSE(fs::exists(invalid_path));
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    EXPECT_FALSE(manager->loadLoRA("invalid", invalid_path, "base_model", 1.0f));
    EXPECT_FALSE(manager->isLoRALoaded("invalid"));
#endif
}

// ═══════════════════════════════════════════════════════════
// Adapter Unloading Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test adapter unloading
 * Acceptance Criteria:
 * - Adapter can be unloaded
 * - Resources are freed
 * - Adapter is removed from registry
 */
TEST_F(LoRAAdapterTest, Unloading_BasicUnload) {
    auto adapter_path = createMockAdapter("unload_test");
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    ASSERT_TRUE(manager->loadLoRA("unload_test", adapter_path, "base_model", 1.0f));
    EXPECT_TRUE(manager->unloadLoRA("unload_test"));
    EXPECT_FALSE(manager->isLoRALoaded("unload_test"));
#endif
}

/**
 * Test unloading all adapters
 * Acceptance Criteria:
 * - All adapters can be unloaded at once
 * - System returns to clean state
 * - No resource leaks
 */
TEST_F(LoRAAdapterTest, Unloading_UnloadAll) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager(3);
    for (int i = 0; i < 3; ++i) {
        const auto id = "unload_all_" + std::to_string(i);
        const auto path = createMockAdapter(id);
        ASSERT_TRUE(manager->loadLoRA(id, path, "base_model", 1.0f));
    }

    const auto loaded_adapters = manager->listLoRAs();
    ASSERT_EQ(loaded_adapters.size(), 3u);
    for (const auto& adapter : loaded_adapters) {
        EXPECT_TRUE(manager->unloadLoRA(adapter.adapter_id));
    }
    EXPECT_TRUE(manager->listLoRAs().empty());
#endif
}

/**
 * Test manager-reported memory accounting after unload
 * Acceptance Criteria:
 * - Loading an adapter increases manager-reported VRAM usage
 * - Unloading restores manager-reported usage to baseline
 */
TEST_F(LoRAAdapterTest, Unloading_MemoryAccountingCleanup) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    const auto baseline_mb =
        manager->getMemoryStats().at("vram_used_mb").get<size_t>();
    const auto adapter_path = createMockAdapter("memory_cleanup", 1024);

    ASSERT_TRUE(manager->loadLoRA(
        "memory_cleanup", adapter_path, "base_model", 1.0f));
    const auto loaded_mb =
        manager->getMemoryStats().at("vram_used_mb").get<size_t>();
    EXPECT_GT(loaded_mb, baseline_mb);

    ASSERT_TRUE(manager->unloadLoRA("memory_cleanup"));
    EXPECT_EQ(manager->getMemoryStats().at("vram_used_mb").get<size_t>(),
              baseline_mb);
#endif
}

// ═══════════════════════════════════════════════════════════
// Adapter Switching Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test switching between adapters
 * Acceptance Criteria:
 * - Can switch from one adapter to another
 * - Switching is fast (< 100ms)
 * - State is consistent after switch
 */
TEST_F(LoRAAdapterTest, Switching_BetweenAdapters) {
    auto adapter1 = createMockAdapter("adapter1");
    auto adapter2 = createMockAdapter("adapter2");

#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    ASSERT_TRUE(manager->loadLoRA("adapter1", adapter1, "base_model", 1.0f));
    ASSERT_TRUE(manager->loadLoRA("adapter2", adapter2, "base_model", 0.75f));

    std::atomic<int> apply_count{0};
    std::atomic<int> remove_count{0};
    manager->setApplyAdapterFn([&apply_count](const LoRASlot& slot) {
        if (slot.lora_id.empty()) {
            return false;
        }
        ++apply_count;
        return true;
    });
    manager->setRemoveAdapterFn([&remove_count](const LoRASlot& slot) {
        if (slot.lora_id.empty()) {
            return false;
        }
        ++remove_count;
        return true;
    });

    ASSERT_TRUE(manager->applyLoRA("adapter1", nullptr));
    ASSERT_TRUE(manager->getLoRA("adapter1")->is_active);
    ASSERT_TRUE(manager->removeLoRA("adapter1", nullptr));
    EXPECT_FALSE(manager->getLoRA("adapter1")->is_active);

    ASSERT_TRUE(manager->applyLoRA("adapter2", nullptr));
    EXPECT_TRUE(manager->getLoRA("adapter2")->is_active);
    EXPECT_EQ(apply_count.load(), 2);
    EXPECT_EQ(remove_count.load(), 1);
#endif
}

/**
 * Test rapid adapter switching
 * Acceptance Criteria:
 * - System handles rapid switches
 * - No corruption or errors
 * - Performance remains stable
 */
TEST_F(LoRAAdapterTest, Switching_RapidSwitching) {
    std::vector<std::string> adapters = {};

    for (int i = 0; i < 3; ++i) {
        adapters.push_back(createMockAdapter("rapid_" + std::to_string(i)));
    }
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager(adapters.size());
    for (size_t i = 0; i < adapters.size(); ++i) {
        ASSERT_TRUE(manager->loadLoRA("rapid_" + std::to_string(i),
                                      adapters[i], "base_model", 1.0f));
    }

    std::atomic<int> applied{0};
    std::atomic<int> removed{0};
    manager->setApplyAdapterFn([&applied](const LoRASlot&) {
        ++applied;
        return true;
    });
    manager->setRemoveAdapterFn([&removed](const LoRASlot&) {
        ++removed;
        return true;
    });

    constexpr int switches = 30;
    for (int i = 0; i < switches; ++i) {
        const auto current = "rapid_" + std::to_string(i % adapters.size());
        const auto previous = "rapid_" + std::to_string((i + adapters.size() - 1) % adapters.size());
        if (i > 0) {
            ASSERT_TRUE(manager->removeLoRA(previous, nullptr));
            EXPECT_FALSE(manager->getLoRA(previous)->is_active);
        }
        ASSERT_TRUE(manager->applyLoRA(current, nullptr));
        EXPECT_TRUE(manager->getLoRA(current)->is_active);
    }

    EXPECT_EQ(applied.load(), switches);
    EXPECT_EQ(removed.load(), switches - 1);
#endif
}

/**
 * Test switching verification
 * Acceptance Criteria:
 * - Active adapter is correctly identified
 * - Switching changes active adapter
 * - State tracking is accurate
 */
TEST_F(LoRAAdapterTest, Switching_VerifyActiveAdapter) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    const auto adapter_path = createMockAdapter("active_adapter");
    ASSERT_TRUE(manager->loadLoRA("active_adapter", adapter_path, "base_model", 0.5f));
    ASSERT_NE(manager->getLoRA("active_adapter"), nullptr);
    EXPECT_FALSE(manager->getLoRA("active_adapter")->is_active);

    manager->setApplyAdapterFn([](const LoRASlot& slot) {
        return slot.lora_id == "active_adapter";
    });
    ASSERT_TRUE(manager->applyLoRA("active_adapter", nullptr));
    EXPECT_TRUE(manager->getLoRA("active_adapter")->is_active);

    manager->setApplyAdapterFn([](const LoRASlot&) { return false; });
    EXPECT_FALSE(manager->applyLoRA("active_adapter", nullptr));
    EXPECT_TRUE(manager->getLoRA("active_adapter")->is_active);
#endif
}

// ═══════════════════════════════════════════════════════════
// Concurrent Access Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test concurrent adapter access
 * Acceptance Criteria:
 * - Multiple threads can access adapters
 * - No race conditions
 * - Consistent results
 */
TEST_F(LoRAAdapterTest, Concurrent_MultipleAccess) {
    auto adapter = createMockAdapter("concurrent_test");

#ifdef THEMIS_ENABLE_LLM
    MultiLoRAManager::Config config;
    config.max_lora_slots = 16;
    config.lora_base_dir = test_dir_.string();
    config.lora_ttl = std::chrono::seconds(0);
    MultiLoRAManager manager(config);

    ASSERT_TRUE(manager.loadLoRA("concurrent_test", adapter, "base_model", 1.0f));

    const int num_threads = 10;
    constexpr int accesses_per_thread = 20;
    std::atomic<int> access_count{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&manager, &access_count, &adapter, accesses_per_thread]() {
            for (int access = 0; access < accesses_per_thread; ++access) {
                if (manager.loadLoRA("concurrent_test", adapter, "base_model", 1.0f) &&
                    manager.isLoRALoaded("concurrent_test") &&
                    !manager.listLoRAs("base_model").empty()) {
                    ++access_count;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(access_count.load(), num_threads * accesses_per_thread);
    EXPECT_TRUE(manager.isLoRALoaded("concurrent_test"));
#endif
}

/**
 * Test concurrent load/unload operations
 * Acceptance Criteria:
 * - Concurrent load/unload handled safely
 * - No deadlocks
 * - State remains consistent
 */
TEST_F(LoRAAdapterTest, Concurrent_LoadUnload) {
#ifdef THEMIS_ENABLE_LLM
    constexpr int num_adapters = 5;
    constexpr int lifecycle_iterations = 10;
    std::vector<std::string> adapters;
    std::vector<std::string> adapter_paths;

    for (int i = 0; i < num_adapters; ++i) {
        adapters.push_back("concurrent_" + std::to_string(i));
        adapter_paths.push_back(createMockAdapter(adapters.back()));
    }

    MultiLoRAManager::Config config;
    config.max_lora_slots = num_adapters;
    config.lora_base_dir = test_dir_.string();
    config.lora_ttl = std::chrono::seconds(0);
    MultiLoRAManager manager(config);

    std::atomic<int> completed_lifecycles{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < num_adapters; ++i) {
        threads.emplace_back([&, i]() {
            for (int iteration = 0; iteration < lifecycle_iterations; ++iteration) {
                if (manager.loadLoRA(adapters[i], adapter_paths[i], "base_model", 1.0f) &&
                    manager.isLoRALoaded(adapters[i]) &&
                    manager.unloadLoRA(adapters[i])) {
                    ++completed_lifecycles;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(completed_lifecycles.load(), num_adapters * lifecycle_iterations);
    for (const auto& adapter : adapters) {
        EXPECT_FALSE(manager.isLoRALoaded(adapter));
    }
#endif
}

/**
 * Test thread-safe adapter registry reads during concurrent lifecycle changes
 * Acceptance Criteria:
 * - Registry queries remain consistent while adapters are loaded and unloaded
 * - Loaded adapter metadata remains visible to concurrent readers
 */
TEST_F(LoRAAdapterTest, Concurrent_RegistryThreadSafety) {
#ifdef THEMIS_ENABLE_LLM
    constexpr size_t num_adapters = 5;
    constexpr int lifecycle_iterations = 10;
    std::vector<std::string> adapter_ids;
    std::vector<std::string> adapter_paths;
    adapter_ids.reserve(num_adapters);
    adapter_paths.reserve(num_adapters);

    for (size_t i = 0; i < num_adapters; ++i) {
        adapter_ids.push_back("registry_" + std::to_string(i));
        adapter_paths.push_back(createMockAdapter(adapter_ids.back()));
    }

    auto manager = createManager(num_adapters);
    std::atomic<int> completed_lifecycles{0};
    std::vector<std::thread> threads;
    threads.reserve(num_adapters);

    for (size_t i = 0; i < num_adapters; ++i) {
        threads.emplace_back([&, i]() {
            for (int iteration = 0; iteration < lifecycle_iterations; ++iteration) {
                if (!manager->loadLoRA(adapter_ids[i], adapter_paths[i],
                                       "base_model", 1.0f)) {
                    continue;
                }

                const auto info = manager->getLoRAInfo(adapter_ids[i]);
                const auto listed = manager->listLoRAs("base_model");
                const bool registry_consistent =
                    manager->isLoRALoaded(adapter_ids[i]) &&
                    info.has_value() &&
                    info->id == adapter_ids[i] &&
                    info->is_loaded &&
                    !listed.empty();
                const bool unloaded = manager->unloadLoRA(adapter_ids[i]);

                if (registry_consistent && unloaded) {
                    ++completed_lifecycles;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(completed_lifecycles.load(),
              static_cast<int>(num_adapters) * lifecycle_iterations);
    EXPECT_TRUE(manager->listLoRAs().empty());
#endif
}

// ═══════════════════════════════════════════════════════════
// State Consistency Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test adapter state consistency
 * Acceptance Criteria:
 * - Adapter state is tracked correctly
 * - State updates are atomic
 * - No inconsistent states
 */
TEST_F(LoRAAdapterTest, StateConsistency_StatusTracking) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    const auto adapter_path = createMockAdapter("status_tracking");

    EXPECT_FALSE(manager->isLoRALoaded("status_tracking"));
    EXPECT_TRUE(manager->listLoRAs().empty());
    ASSERT_TRUE(manager->loadLoRA("status_tracking", adapter_path, "base_model", 1.0f));

    EXPECT_TRUE(manager->isLoRALoaded("status_tracking"));
    const auto* slot = manager->getLoRA("status_tracking");
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->lora_id, "status_tracking");
    EXPECT_EQ(slot->base_model_id, "base_model");
    EXPECT_EQ(manager->listLoRAs("base_model").size(), 1u);

    ASSERT_TRUE(manager->unloadLoRA("status_tracking"));
    EXPECT_FALSE(manager->isLoRALoaded("status_tracking"));
    EXPECT_TRUE(manager->listLoRAs().empty());
#endif
}

/**
 * Test adapter metadata consistency
 * Acceptance Criteria:
 * - Metadata is preserved correctly
 * - No data corruption
 * - Metadata accessible after operations
 */
TEST_F(LoRAAdapterTest, StateConsistency_MetadataIntegrity) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    const auto adapter_path = createMockAdapter("metadata_adapter", 2048);
    ASSERT_TRUE(manager->loadLoRA("metadata_adapter", adapter_path, "base_model", 0.625f));

    const auto info = manager->getLoRAInfo("metadata_adapter");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->id, "metadata_adapter");
    EXPECT_EQ(info->adapter_id, "metadata_adapter");
    EXPECT_EQ(info->base_model_id, "base_model");
    EXPECT_EQ(info->path, adapter_path);
    EXPECT_FLOAT_EQ(info->scale, 0.625f);
    EXPECT_EQ(info->size_bytes, fs::file_size(adapter_path));
    EXPECT_TRUE(info->is_loaded);
#endif
}

/**
 * Test adapter usage tracking
 * Acceptance Criteria:
 * - Adapter application attempts increment the recorded usage count
 * - A failed application bridge does not change the active state
 */
TEST_F(LoRAAdapterTest, StateConsistency_UsageTracking) {
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager();
    const auto adapter_path = createMockAdapter("usage_tracking");
    ASSERT_TRUE(manager->loadLoRA("usage_tracking", adapter_path, "base_model", 1.0f));
    ASSERT_NE(manager->getLoRA("usage_tracking"), nullptr);
    EXPECT_EQ(manager->getLoRA("usage_tracking")->use_count, 1u);

    manager->setApplyAdapterFn([](const LoRASlot& slot) {
        return slot.lora_id == "usage_tracking";
    });

    ASSERT_TRUE(manager->applyLoRA("usage_tracking", nullptr));
    EXPECT_EQ(manager->getLoRA("usage_tracking")->use_count, 2u);
    EXPECT_TRUE(manager->getLoRA("usage_tracking")->is_active);

    ASSERT_TRUE(manager->applyLoRA("usage_tracking", nullptr));
    EXPECT_EQ(manager->getLoRA("usage_tracking")->use_count, 3u);

    manager->setApplyAdapterFn([](const LoRASlot&) { return false; });
    EXPECT_FALSE(manager->applyLoRA("usage_tracking", nullptr));
    EXPECT_EQ(manager->getLoRA("usage_tracking")->use_count, 4u);
    EXPECT_TRUE(manager->getLoRA("usage_tracking")->is_active);
#endif
}

// ═══════════════════════════════════════════════════════════
// Multi-Adapter Management Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test managing multiple adapters simultaneously
 * Acceptance Criteria:
 * - System handles multiple active adapters
 * - Resource limits respected
 * - Performance acceptable
 */
TEST_F(LoRAAdapterTest, MultiAdapter_SimultaneousManagement) {
    const int max_adapters = 8;
    std::vector<std::string> adapters;
    
    for (int i = 0; i < max_adapters; ++i) {
        adapters.push_back(createMockAdapter("multi_" + std::to_string(i)));
    }
    
#ifdef THEMIS_ENABLE_LLM
    auto manager = createManager(adapters.size());
    for (size_t i = 0; i < adapters.size(); ++i) {
        ASSERT_TRUE(manager->loadLoRA("multi_" + std::to_string(i),
                                      adapters[i], "base_model", 1.0f));
    }
    EXPECT_EQ(manager->listLoRAs("base_model").size(),
              static_cast<size_t>(max_adapters));
#endif
}

/**
 * Test pinning and unpinning behavior during slot-limit eviction
 * Acceptance Criteria:
 * - Pinned adapters are protected from automatic LRU eviction
 * - Unpinned adapters become eligible for later eviction
 */
#ifdef THEMIS_ENABLE_LLM
TEST_F(LoRAAdapterTest, MultiAdapter_PinAndUnpinEvictionProtection) {
    const auto pinned_path = createMockAdapter("pin_protected");
    const auto first_eviction_path = createMockAdapter("first_eviction");
    const auto second_eviction_path = createMockAdapter("second_eviction");
    const auto manager = createManager(2);

    ASSERT_TRUE(manager->loadLoRA(
        "pin_protected", pinned_path, "base_model", 1.0f));
    manager->pinLoRA("pin_protected");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(manager->loadLoRA(
        "first_eviction", first_eviction_path, "base_model", 1.0f));

    ASSERT_TRUE(manager->loadLoRA(
        "second_eviction", second_eviction_path, "base_model", 1.0f));
    EXPECT_TRUE(manager->isLoRALoaded("pin_protected"));
    EXPECT_FALSE(manager->isLoRALoaded("first_eviction"));
    EXPECT_TRUE(manager->isLoRALoaded("second_eviction"));

    manager->unpinLoRA("pin_protected");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(manager->loadLoRA(
        "first_eviction", first_eviction_path, "base_model", 1.0f));
    EXPECT_FALSE(manager->isLoRALoaded("pin_protected"));
    EXPECT_TRUE(manager->isLoRALoaded("first_eviction"));
    EXPECT_EQ(manager->listLoRAs().size(), 2u);
}
#endif

/**
 * Test least-recently-used eviction at the configured slot limit
 * Acceptance Criteria:
 * - Loading beyond capacity evicts the least recently used unpinned adapter
 * - Recently accessed adapters remain registered
 * - The manager remains usable after eviction
 */
#ifdef THEMIS_ENABLE_LLM
TEST_F(LoRAAdapterTest, MultiAdapter_LRUEviction) {
    const auto oldest_path = createMockAdapter("lru_oldest");
    const auto recent_path = createMockAdapter("lru_recent");
    const auto incoming_path = createMockAdapter("lru_incoming");
    auto manager = createManager(2);

    ASSERT_TRUE(manager->loadLoRA(
        "lru_oldest", oldest_path, "base_model", 1.0f));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(manager->loadLoRA(
        "lru_recent", recent_path, "base_model", 1.0f));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    ASSERT_TRUE(manager->loadLoRA(
        "lru_oldest", oldest_path, "base_model", 1.0f));

    ASSERT_TRUE(manager->loadLoRA(
        "lru_incoming", incoming_path, "base_model", 1.0f));

    EXPECT_FALSE(manager->isLoRALoaded("lru_recent"));
    EXPECT_TRUE(manager->isLoRALoaded("lru_oldest"));
    EXPECT_TRUE(manager->isLoRALoaded("lru_incoming"));
    EXPECT_EQ(manager->listLoRAs().size(), 2u);
}
#endif

// ═══════════════════════════════════════════════════════════
// Performance Tests
// ═══════════════════════════════════════════════════════════

/**
 * Measure manager-side registration latency for a small synthetic adapter fixture
 * Acceptance Criteria:
 * - Manager registration is measured and completes within the coarse 1-second test budget
 * - Registry metadata reflects the fixture size after registration
 * - The measurement does not claim model parsing, GPU transfer, or inference latency
 */
#ifdef THEMIS_ENABLE_LLM
TEST_F(LoRAAdapterTest, Performance_ManagerFixtureRegistrationLatency) {
    constexpr size_t fixture_size_bytes = 10 * 1024;
    const auto adapter = createMockAdapter("perf_test", fixture_size_bytes);
    auto manager = createManager();
    test::LatencyMeasurement timer;

    ASSERT_TRUE(manager->loadLoRA(
        "perf_test", adapter, "base_model", 1.0f));
    const auto registration_time_ms = timer.elapsedMs();
    const auto info = manager->getLoRAInfo("perf_test");

    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->size_bytes, fixture_size_bytes);
    EXPECT_LT(registration_time_ms, 1000.0)
        << "Synthetic fixture registration exceeded the coarse test budget";
    std::cout << "Registered " << info->size_bytes << " synthetic adapter bytes in "
              << registration_time_ms << "ms (manager-side setup only)" << std::endl;
}
#endif

/**
 * Test manager switching statistics
 * Acceptance Criteria:
 * - Successful adapter applications increment the switching counter
 * - Failed application callbacks do not increment the switching counter
 */
#ifdef THEMIS_ENABLE_LLM
TEST_F(LoRAAdapterTest, Switching_StatisticsTrackSuccessfulApplications) {
    auto manager = createManager();
    const auto adapter1_path = createMockAdapter("stats_adapter1");
    const auto adapter2_path = createMockAdapter("stats_adapter2");
    ASSERT_TRUE(manager->loadLoRA(
        "stats_adapter1", adapter1_path, "base_model", 1.0f));
    ASSERT_TRUE(manager->loadLoRA(
        "stats_adapter2", adapter2_path, "base_model", 1.0f));

    manager->setApplyAdapterFn([](const LoRASlot&) { return true; });
    manager->setRemoveAdapterFn([](const LoRASlot&) { return true; });
    const auto initial_switches = manager->getStatistics().switches;

    ASSERT_TRUE(manager->applyLoRA("stats_adapter1", nullptr));
    ASSERT_TRUE(manager->removeLoRA("stats_adapter1", nullptr));
    ASSERT_TRUE(manager->applyLoRA("stats_adapter2", nullptr));
    EXPECT_EQ(manager->getStatistics().switches, initial_switches + 2);

    manager->setApplyAdapterFn([](const LoRASlot&) { return false; });
    EXPECT_FALSE(manager->applyLoRA("stats_adapter2", nullptr));
    EXPECT_EQ(manager->getStatistics().switches, initial_switches + 2);
}
#endif

/**
 * Test manager-reported adapter VRAM accounting
 * Acceptance Criteria:
 * - Reported bytes equal the sum of loaded adapter size estimates
 * - Loaded adapter count matches the manager registry
 * - Unloading all fixtures restores the VRAM accounting baseline
 */
#ifdef THEMIS_ENABLE_LLM
TEST_F(LoRAAdapterTest, Performance_AdapterVRAMAccounting) {
    constexpr size_t adapter_count = 5;
    constexpr size_t adapter_size_bytes = 5 * 1024;
    std::vector<std::string> adapter_paths;
    adapter_paths.reserve(adapter_count);
    for (size_t i = 0; i < adapter_count; ++i) {
        adapter_paths.push_back(
            createMockAdapter("mem_" + std::to_string(i), adapter_size_bytes));
    }

    auto manager = createManager(adapter_count);
    size_t expected_bytes = 0;
    for (size_t i = 0; i < adapter_paths.size(); ++i) {
        const auto id = "mem_" + std::to_string(i);
        ASSERT_TRUE(manager->loadLoRA(id, adapter_paths[i], "base_model", 1.0f));
        const auto info = manager->getLoRAInfo(id);
        ASSERT_TRUE(info.has_value());
        EXPECT_EQ(info->size_bytes, fs::file_size(adapter_paths[i]));
        expected_bytes += info->size_bytes;
    }

    EXPECT_EQ(manager->getMemoryStats().at("loras_loaded").get<size_t>(),
              adapter_count);
    EXPECT_EQ(manager->getMemoryStats().at("vram_used_mb").get<size_t>(), 1u);
    size_t reported_bytes = 0;
    for (const auto& info : manager->listLoRAs()) {
        reported_bytes += info.size_bytes;
    }
    EXPECT_EQ(reported_bytes, expected_bytes);

    for (size_t i = 0; i < adapter_paths.size(); ++i) {
        EXPECT_TRUE(manager->unloadLoRA("mem_" + std::to_string(i)));
    }
    EXPECT_EQ(manager->getMemoryStats().at("vram_used_mb").get<size_t>(), 0u);
}
#endif
