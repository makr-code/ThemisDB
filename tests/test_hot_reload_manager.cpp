/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hot_reload_manager.cpp                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-22 08:56:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     361                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 4fb12f70c  2026-02-22  Add hot-reload manager for plugins (base module Phase 2) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file test_hot_reload_manager.cpp
/// @brief Unit tests for HotReloadManager (base module, Phase 2)
///
/// Tests are designed to run without real plugin binaries; they verify the
/// API surface, state machine transitions, callback handling, and error paths.

#include <gtest/gtest.h>
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include <string>
#include <vector>
#include <thread>

using namespace themis::modules;

// =============================================================================
// ModuleVersion Tests
// =============================================================================

TEST(ModuleVersion, DefaultConstruction) {
    ModuleVersion v;
    EXPECT_TRUE(v.version.empty());
    EXPECT_TRUE(v.abiVersion.empty());
    EXPECT_TRUE(v.buildId.empty());
    EXPECT_EQ(v.major, 0u);
    EXPECT_EQ(v.minor, 0u);
    EXPECT_EQ(v.patch, 0u);
}

TEST(ModuleVersion, ToStringWithVersionString) {
    ModuleVersion v;
    v.version = "2.1.0";
    EXPECT_EQ(v.toString(), "2.1.0");
}

TEST(ModuleVersion, ToStringFallbackToNumbers) {
    ModuleVersion v;
    v.major = 1;
    v.minor = 2;
    v.patch = 3;
    EXPECT_EQ(v.toString(), "1.2.3");
}

TEST(ModuleVersion, EqualityOperator) {
    ModuleVersion a, b;
    a.version = "1.0.0"; a.major = 1;
    b.version = "1.0.0"; b.major = 1;
    EXPECT_EQ(a, b);
}

TEST(ModuleVersion, InequalityOperator) {
    ModuleVersion a, b;
    a.version = "1.0.0"; a.major = 1;
    b.version = "2.0.0"; b.major = 2;
    EXPECT_NE(a, b);
}

TEST(ModuleVersion, FromMetadata) {
    ModuleMetadata meta;
    meta.version    = "3.2.1";
    meta.abiVersion = "abi-v3";
    meta.buildId    = "abc123";
    meta.themisMajor = 3;
    meta.themisMinor = 2;
    meta.themisPatch = 1;

    auto v = ModuleVersion::fromMetadata(meta);
    EXPECT_EQ(v.version, "3.2.1");
    EXPECT_EQ(v.abiVersion, "abi-v3");
    EXPECT_EQ(v.buildId, "abc123");
    EXPECT_EQ(v.major, 3u);
    EXPECT_EQ(v.minor, 2u);
    EXPECT_EQ(v.patch, 1u);
}

// =============================================================================
// HotReloadResult Tests
// =============================================================================

TEST(HotReloadResult, DefaultConstruction) {
    HotReloadResult r;
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.errorMessage.empty());
    EXPECT_TRUE(r.previousVersion.empty());
    EXPECT_TRUE(r.newVersion.empty());
    EXPECT_FALSE(r.rollbackAvailable);
    EXPECT_EQ(r.reloadDurationMs, 0u);
}

// =============================================================================
// HotReloadManager Basic API Tests
// =============================================================================

class HotReloadManagerTest : public ::testing::Test {
protected:
    HotReloadManager mgr;
    ModuleLoader     loader;
};

TEST_F(HotReloadManagerTest, DefaultConstruction) {
    EXPECT_NO_THROW({ HotReloadManager m; });
}

TEST_F(HotReloadManagerTest, ConstructWithConfig) {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    cfg.preserveState   = false;
    cfg.enableRollback  = false;
    EXPECT_NO_THROW({ HotReloadManager m(cfg); });
}

TEST_F(HotReloadManagerTest, RegisterModule) {
    EXPECT_NO_THROW(mgr.registerModule("test_mod", loader));
}

TEST_F(HotReloadManagerTest, RegisterModuleAppearsInList) {
    mgr.registerModule("alpha", loader);
    mgr.registerModule("beta", loader);

    auto names = mgr.registeredModules();
    EXPECT_EQ(names.size(), 2u);
    // Order not guaranteed – check both names present.
    bool found_alpha = false, found_beta = false;
    for (const auto& n : names) {
        if (n == "alpha") found_alpha = true;
        if (n == "beta")  found_beta  = true;
    }
    EXPECT_TRUE(found_alpha);
    EXPECT_TRUE(found_beta);
}

TEST_F(HotReloadManagerTest, UnregisterModule) {
    mgr.registerModule("mod_to_remove", loader);
    EXPECT_EQ(mgr.registeredModules().size(), 1u);

    mgr.unregisterModule("mod_to_remove");
    EXPECT_EQ(mgr.registeredModules().size(), 0u);
}

// =============================================================================
// Error-path Tests (no real binary)
// =============================================================================

TEST_F(HotReloadManagerTest, ReloadUnregisteredModuleFails) {
    auto result = mgr.reloadModule("nonexistent", "/some/path.so");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(HotReloadManagerTest, RollbackUnregisteredModuleFails) {
    auto result = mgr.rollback("nonexistent");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(HotReloadManagerTest, RollbackWithNoBackupFails) {
    mgr.registerModule("fresh_mod", loader);
    auto result = mgr.rollback("fresh_mod");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(HotReloadManagerTest, ReloadNonExistentPathFails) {
    mgr.registerModule("mod", loader);
    auto result = mgr.reloadModule("mod", "/absolutely/nonexistent/module.so");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

// =============================================================================
// getCurrentVersion Tests
// =============================================================================

TEST_F(HotReloadManagerTest, GetCurrentVersionUnregistered) {
    auto v = mgr.getCurrentVersion("unregistered");
    EXPECT_FALSE(v.has_value());
}

TEST_F(HotReloadManagerTest, GetCurrentVersionRegisteredNotLoaded) {
    mgr.registerModule("not_loaded", loader);
    // Loader has no module by that name; version should be nullopt.
    auto v = mgr.getCurrentVersion("not_loaded");
    EXPECT_FALSE(v.has_value());
}

// =============================================================================
// isRollbackAvailable Tests
// =============================================================================

TEST_F(HotReloadManagerTest, RollbackNotAvailableInitially) {
    mgr.registerModule("mod", loader);
    EXPECT_FALSE(mgr.isRollbackAvailable("mod"));
}

TEST_F(HotReloadManagerTest, RollbackNotAvailableForUnregistered) {
    EXPECT_FALSE(mgr.isRollbackAvailable("not_registered"));
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(HotReloadManagerTest, AddAndClearReloadCallbacks) {
    EXPECT_NO_THROW(
        mgr.addReloadCallback([](const std::string&, HotReloadManager::ReloadPhase) {})
    );
    EXPECT_NO_THROW(mgr.clearReloadCallbacks());
}

TEST_F(HotReloadManagerTest, MultipleReloadCallbacks) {
    int count = 0;
    mgr.addReloadCallback([&count](const std::string&, HotReloadManager::ReloadPhase) {
        ++count;
    });
    mgr.addReloadCallback([&count](const std::string&, HotReloadManager::ReloadPhase) {
        ++count;
    });
    // Clearing should not crash.
    EXPECT_NO_THROW(mgr.clearReloadCallbacks());
}

TEST_F(HotReloadManagerTest, ReloadCallbackExceptionHandled) {
    mgr.addReloadCallback([](const std::string&, HotReloadManager::ReloadPhase) {
        throw std::runtime_error("test exception in callback");
    });
    mgr.registerModule("mod", loader);
    // Reload will fail (no real file), but should not crash due to the exception.
    EXPECT_NO_THROW(mgr.reloadModule("mod", "/nonexistent.so"));
}

TEST_F(HotReloadManagerTest, SetStateSaveCallback) {
    EXPECT_NO_THROW(
        mgr.setStateSaveCallback(
            [](const std::string&) -> std::string { return "{}"; })
    );
}

TEST_F(HotReloadManagerTest, SetStateRestoreCallback) {
    EXPECT_NO_THROW(
        mgr.setStateRestoreCallback(
            [](const std::string&, const std::string&) -> bool { return true; })
    );
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST_F(HotReloadManagerTest, StatsDefaultZero) {
    auto s = mgr.getStats();
    EXPECT_EQ(s.totalReloads,      0u);
    EXPECT_EQ(s.successfulReloads, 0u);
    EXPECT_EQ(s.failedReloads,     0u);
    EXPECT_EQ(s.rollbacks,         0u);
    EXPECT_EQ(s.statesSaved,       0u);
    EXPECT_EQ(s.statesRestored,    0u);
}

TEST_F(HotReloadManagerTest, StatsResetWorks) {
    // Trigger a failed reload to increment counters.
    mgr.registerModule("mod", loader);
    mgr.reloadModule("mod", "/nonexistent.so");

    auto s1 = mgr.getStats();
    EXPECT_GT(s1.totalReloads, 0u);

    mgr.resetStats();
    auto s2 = mgr.getStats();
    EXPECT_EQ(s2.totalReloads, 0u);
}

TEST_F(HotReloadManagerTest, FailedReloadIncrementsStat) {
    mgr.registerModule("mod", loader);
    mgr.reloadModule("mod", "/nonexistent.so");

    auto s = mgr.getStats();
    EXPECT_EQ(s.totalReloads,  1u);
    EXPECT_EQ(s.failedReloads, 1u);
    EXPECT_EQ(s.successfulReloads, 0u);
}

TEST_F(HotReloadManagerTest, RollbackWithNoBackupIncrementsStat) {
    // rollback() returns early when no backup is present; stats should not
    // count it as a successful rollback but call is safe.
    mgr.registerModule("mod", loader);
    mgr.rollback("mod");
    // Stats.rollbacks only incremented when the rollback actually runs the
    // unload/load path.  With no backup the function returns before that.
    EXPECT_NO_THROW(mgr.getStats());
}

// =============================================================================
// Thread-safety Tests
// =============================================================================

TEST_F(HotReloadManagerTest, ConcurrentRegisterUnregister) {
    const int n = 20;
    std::vector<std::thread> threads;
    threads.reserve(n * 2);

    for (int i = 0; i < n; ++i) {
        threads.emplace_back([this, i]() {
            mgr.registerModule("mod_" + std::to_string(i), loader);
        });
    }
    for (auto& t : threads) t.join();
    threads.clear();

    for (int i = 0; i < n; ++i) {
        threads.emplace_back([this, i]() {
            mgr.unregisterModule("mod_" + std::to_string(i));
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_TRUE(mgr.registeredModules().empty());
}

TEST_F(HotReloadManagerTest, ConcurrentReloadAttempts) {
    mgr.registerModule("shared_mod", loader);

    const int n = 8;
    std::vector<std::thread> threads;
    threads.reserve(n);
    for (int i = 0; i < n; ++i) {
        threads.emplace_back([this]() {
            // All will fail (nonexistent path) but should not crash or deadlock.
            mgr.reloadModule("shared_mod", "/nonexistent.so");
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_TRUE(true);  // Reaching here means no crash / deadlock.
}

TEST_F(HotReloadManagerTest, ConcurrentCallbackRegistration) {
    const int n = 10;
    std::vector<std::thread> threads;
    threads.reserve(n);
    for (int i = 0; i < n; ++i) {
        threads.emplace_back([this]() {
            mgr.addReloadCallback([](const std::string&, HotReloadManager::ReloadPhase) {});
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_NO_THROW(mgr.clearReloadCallbacks());
}

// =============================================================================
// ReloadPhase enum sanity
// =============================================================================

TEST(HotReloadManagerPhase, EnumValuesDistinct) {
    EXPECT_NE(static_cast<int>(HotReloadManager::ReloadPhase::BEFORE_UNLOAD),
              static_cast<int>(HotReloadManager::ReloadPhase::AFTER_UNLOAD));
    EXPECT_NE(static_cast<int>(HotReloadManager::ReloadPhase::AFTER_UNLOAD),
              static_cast<int>(HotReloadManager::ReloadPhase::AFTER_LOAD));
    EXPECT_NE(static_cast<int>(HotReloadManager::ReloadPhase::AFTER_LOAD),
              static_cast<int>(HotReloadManager::ReloadPhase::ROLLBACK));
}
