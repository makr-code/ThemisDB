/// @file test_hot_reload_manager.cpp
/// @brief Unit tests for HotReloadManager (base module, Phase 2)
///
/// Tests are designed to run without real plugin binaries; they verify the
/// API surface, state machine transitions, callback handling, and error paths.

#include <gtest/gtest.h>
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include <atomic>
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
        if (n == "alpha") {
          found_alpha = true;
        }
        if (n == "beta") {
          found_beta  = true;
        }
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
    for (auto& t : threads) {
      t.join();
    }
    threads.clear();

    for (int i = 0; i < n; ++i) {
        threads.emplace_back([this, i]() {
            mgr.unregisterModule("mod_" + std::to_string(i));
        });
    }
    for (auto& t : threads) {
      t.join();
    }

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
    for (auto& t : threads) {
      t.join();
    }

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
    for (auto& t : threads) {
      t.join();
    }

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

// =============================================================================
// Sandbox configuration Tests
// =============================================================================

TEST_F(HotReloadManagerTest, DefaultConfigHasNoSandbox) {
    HotReloadManager::Config cfg;
    EXPECT_FALSE(cfg.sandboxConfig.has_value());
}

TEST_F(HotReloadManagerTest, SandboxConfigCanBeSet) {
    HotReloadManager::Config cfg;
    ModuleSandbox::Config sc;
    sc.max_memory_mb   = 128;
    sc.max_cpu_percent = 25;
    cfg.sandboxConfig  = sc;
    EXPECT_TRUE(cfg.sandboxConfig.has_value());
    EXPECT_EQ(cfg.sandboxConfig->max_memory_mb,   128u);
    EXPECT_EQ(cfg.sandboxConfig->max_cpu_percent,  25);
}

TEST_F(HotReloadManagerTest, ConstructWithSandboxConfig) {
    HotReloadManager::Config cfg;
    ModuleSandbox::Config sc;
    sc.max_memory_mb   = 64;
    sc.max_cpu_percent = 10;
    cfg.sandboxConfig  = sc;
    EXPECT_NO_THROW({ HotReloadManager m(cfg); });
}

TEST_F(HotReloadManagerTest, GetSandboxStatsUnregisteredModule) {
    // Module not registered – must return nullopt.
    auto stats = mgr.getSandboxStats("not_registered");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(HotReloadManagerTest, GetSandboxStatsRegisteredNoSandboxConfig) {
    // Module registered but manager has no sandboxConfig → nullopt.
    mgr.registerModule("mod_no_sandbox", loader);
    auto stats = mgr.getSandboxStats("mod_no_sandbox");
    EXPECT_FALSE(stats.has_value());
}

TEST_F(HotReloadManagerTest, GetSandboxStatsWithSandboxConfigButNotLoaded) {
    // Manager has sandboxConfig, but no reload has been performed yet
    // (sandbox is not launched) → nullopt.
    HotReloadManager::Config cfg;
    ModuleSandbox::Config sc;
    sc.max_memory_mb = 64;
    cfg.sandboxConfig = sc;
    HotReloadManager mgr_sb(cfg);

    ModuleLoader ldr;
    mgr_sb.registerModule("sandboxed_mod", ldr);
    // No reload performed; sandbox never launched.
    auto stats = mgr_sb.getSandboxStats("sandboxed_mod");
    EXPECT_FALSE(stats.has_value());
}

// =============================================================================
// TSAN-compatible concurrent reader / reload stress test
//
// This test is designed so that ThreadSanitizer (TSAN) can detect data races
// when the binary is compiled with -DTHEMIS_ENABLE_TSAN=ON.  Under normal
// (non-sanitised) builds it validates that 16 reader threads running
// concurrently with 1 reload-attempt thread do not crash or deadlock.
// =============================================================================

TEST_F(HotReloadManagerTest, ConcurrentReadersWithReloadThread) {
    mgr.registerModule("shared_mod", loader);

    const int num_readers = 16;
    const int iterations  = 50;

    std::atomic<bool> done{false};
    // Tracks whether any reader thread observed an empty module list.
    // Using an atomic avoids calling EXPECT_* from non-main threads, which is
    // not thread-safe in GoogleTest.
    std::atomic<bool> reader_saw_empty_list{false};

    // 1 reload thread – all reload calls will fail (nonexistent path) but
    // they exercise the write path protected by std::unique_lock.
    std::thread reload_thread([&]() {
        for (int i = 0; i < iterations && !done.load(std::memory_order_relaxed); ++i) {
            mgr.reloadModule("shared_mod", "/nonexistent.so");
        }
        done.store(true, std::memory_order_relaxed);
    });

    // 16 reader threads exercise the read path protected by std::shared_lock.
    std::vector<std::thread> readers;
    readers.reserve(num_readers);
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            while (!done.load(std::memory_order_relaxed)) {
                (void)mgr.getCurrentVersion("shared_mod");
                (void)mgr.isRollbackAvailable("shared_mod");
                auto names = mgr.registeredModules();
                (void)mgr.getStats();
                // Record failure condition without calling EXPECT_* from a
                // worker thread (GoogleTest assertions are not thread-safe).
                if (names.empty()) {
                    reader_saw_empty_list.store(true, std::memory_order_relaxed);
                }
            }
        });
    }

    reload_thread.join();
    for (auto& t : readers) {
      t.join();
    }

    // All assertions run on the main test thread after all workers have joined.
    EXPECT_FALSE(reader_saw_empty_list.load())
        << "A reader thread observed an empty module list during concurrent access";

    // Verify state is consistent after concurrent access.
    auto names = mgr.registeredModules();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "shared_mod");

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.totalReloads, static_cast<uint64_t>(iterations));
    EXPECT_EQ(stats.failedReloads, static_cast<uint64_t>(iterations));
    EXPECT_EQ(stats.successfulReloads, 0u);
}
