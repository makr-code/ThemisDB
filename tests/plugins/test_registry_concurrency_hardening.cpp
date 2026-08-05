// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_registry_concurrency_hardening.cpp
 * @brief Phase 2B focused tests for plugin registry concurrency.
 *
 * Test IDs: PLG-17 through PLG-22
 * Validates atomic registry operations and concurrency safety.
 *
 * @see include/plugins/plugin_registry.h
 * @see src/plugins/ROADMAP.md — Phase 2B implementation
 */

#include "gtest/gtest.h"
#include "plugins/plugin_registry.h"
#include "plugins/plugin_interface.h"

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

namespace themis {
namespace plugins {
namespace test {

// Mock plugin interface for testing
class IMockPlugin {
public:
    virtual ~IMockPlugin() = default;
    virtual std::string getName() const = 0;
};

class MockPluginImpl : public IMockPlugin {
private:
    std::string name_;
public:
    MockPluginImpl(const std::string& name) : name_(name) {}
    std::string getName() const override { return name_; }
};

// ============================================================================
// PLG-17 — Registry single registration
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG17_SingleRegistration) {
    PluginRegistry::clearRegistry();
    
    PluginRegistry::registerFactory<IMockPlugin>(
        "test_plugin_1",
        []() { return std::make_unique<MockPluginImpl>("test_plugin_1"); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<IMockPlugin>("test_plugin_1"));
    
    auto plugin = PluginRegistry::create<IMockPlugin>("test_plugin_1");
    EXPECT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->getName(), "test_plugin_1");
    
    PluginRegistry::clearRegistry();
}

// ============================================================================
// PLG-18 — Registry multiple registrations (different types)
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG18_MultipleRegistrationsDifferentTypes) {
    PluginRegistry::clearRegistry();
    
    // Register multiple plugins of the same type with different names
    for (int i = 0; i < 5; ++i) {
        std::string name = "plugin_" + std::to_string(i);
        PluginRegistry::registerFactory<IMockPlugin>(
            name,
            [name]() { return std::make_unique<MockPluginImpl>(name); }
        );
    }
    
    // Verify all were registered
    auto plugins = PluginRegistry::listPlugins<IMockPlugin>();
    EXPECT_EQ(plugins.size(), 5u);
    
    // Verify we can create each one
    for (int i = 0; i < 5; ++i) {
        std::string name = "plugin_" + std::to_string(i);
        EXPECT_TRUE(PluginRegistry::hasPlugin<IMockPlugin>(name));
        auto plugin = PluginRegistry::create<IMockPlugin>(name);
        EXPECT_NE(plugin, nullptr);
        EXPECT_EQ(plugin->getName(), name);
    }
    
    PluginRegistry::clearRegistry();
}

// ============================================================================
// PLG-19 — Registry unregistration
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG19_RegistrationUnregistration) {
    PluginRegistry::clearRegistry();
    
    PluginRegistry::registerFactory<IMockPlugin>(
        "test_plugin_2",
        []() { return std::make_unique<MockPluginImpl>("test_plugin_2"); }
    );
    
    EXPECT_TRUE(PluginRegistry::hasPlugin<IMockPlugin>("test_plugin_2"));
    
    bool removed = PluginRegistry::unregisterFactory<IMockPlugin>("test_plugin_2");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(PluginRegistry::hasPlugin<IMockPlugin>("test_plugin_2"));
    
    // Attempting to unregister again should return false
    removed = PluginRegistry::unregisterFactory<IMockPlugin>("test_plugin_2");
    EXPECT_FALSE(removed);
    
    PluginRegistry::clearRegistry();
}

// ============================================================================
// PLG-20 — Registry create on non-existent plugin
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG20_CreateNonExistentPlugin) {
    PluginRegistry::clearRegistry();
    
    // Attempting to create a plugin that wasn't registered should throw
    EXPECT_THROW(
        PluginRegistry::create<IMockPlugin>("non_existent_plugin"),
        std::runtime_error
    );
    
    PluginRegistry::clearRegistry();
}

// ============================================================================
// PLG-21 — Registry atomicity (overwrite on re-registration)
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG21_AtomicReRegistration) {
    PluginRegistry::clearRegistry();
    
    int call_count = 0;
    
    // Register first version
    PluginRegistry::registerFactory<IMockPlugin>(
        "plugin_version_1",
        [&call_count]() {
            ++call_count;
            return std::make_unique<MockPluginImpl>("v1");
        }
    );
    
    // Overwrite with second version (simulating a reload scenario)
    call_count = 0;
    PluginRegistry::registerFactory<IMockPlugin>(
        "plugin_version_1",
        [&call_count]() {
            ++call_count;
            return std::make_unique<MockPluginImpl>("v2");
        }
    );
    
    // Create and verify we get the new version
    auto plugin = PluginRegistry::create<IMockPlugin>("plugin_version_1");
    EXPECT_EQ(plugin->getName(), "v2");
    EXPECT_EQ(call_count, 1); // Factory was called once
    
    PluginRegistry::clearRegistry();
}

// ============================================================================
// PLG-22 — Registry concurrent reads (stress test)
// ============================================================================

TEST(RegistryConcurrencyHardening, PLG22_ConcurrentReads) {
    PluginRegistry::clearRegistry();
    
    // Register plugins
    for (int i = 0; i < 10; ++i) {
        std::string name = "concurrent_plugin_" + std::to_string(i);
        PluginRegistry::registerFactory<IMockPlugin>(
            name,
            [name]() { return std::make_unique<MockPluginImpl>(name); }
        );
    }
    
    // Spawn multiple threads trying to read and create plugins concurrently
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 10; ++i) {
                try {
                    std::string name = "concurrent_plugin_" + std::to_string(i);
                    if (PluginRegistry::hasPlugin<IMockPlugin>(name)) {
                        auto plugin = PluginRegistry::create<IMockPlugin>(name);
                        if (plugin && plugin->getName() == name) {
                            ++success_count;
                        }
                    }
                } catch (...) {
                    ++error_count;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify no errors and all threads succeeded
    EXPECT_EQ(error_count, 0);
    EXPECT_EQ(success_count, 50); // 5 threads * 10 plugins
    
    PluginRegistry::clearRegistry();
}

} // namespace test
} // namespace plugins
} // namespace themis
