/// @file hot_reload_manager_integration_test.cpp
/// @brief Integration tests for HotReloadManager with ModuleLoader
///
/// These tests verify that HotReloadManager integrates correctly with
/// ModuleLoader, ModuleRegistry, and the callback system across a complete
/// reload lifecycle.  Tests use only in-process logic (no real .so files)
/// to stay self-contained and fast.

#include "test_fixture.h"
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::modules;
using namespace themis::test;

// =============================================================================
// Integration Test Fixture
// =============================================================================

class HotReloadManagerIntegrationTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();

        HotReloadManager::Config cfg;
        cfg.verifySignature = false;   // disable for in-process testing
        cfg.preserveState   = true;
        cfg.enableRollback  = true;
        mgr_  = std::make_unique<HotReloadManager>(cfg);
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    void TearDown() override {
        mgr_.reset();
        loader_.reset();
        IntegrationTestFixture::TearDown();
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

// =============================================================================
// Lifecycle integration: register → attempt reload → error path
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, RegisterAndReloadNonExistentPath) {
    // Registers a module with the loader, attempts to reload from a path that
    // does not exist.  The manager must return a failure and leave the
    // previously registered state intact.

    mgr_->registerModule("mod_a", *loader_);

    auto result = mgr_->reloadModule("mod_a", "/nonexistent/path/mod_a.so");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());

    // Stats must reflect exactly one failed reload attempt.
    auto stats = mgr_->getStats();
    EXPECT_EQ(stats.totalReloads,  1u);
    EXPECT_EQ(stats.failedReloads, 1u);
    EXPECT_EQ(stats.successfulReloads, 0u);
}

TEST_F(HotReloadManagerIntegrationTest, RollbackAfterNoSuccessfulReload) {
    // Without a prior successful reload, there is nothing to roll back.
    mgr_->registerModule("mod_b", *loader_);

    auto result = mgr_->rollback("mod_b");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_FALSE(mgr_->isRollbackAvailable("mod_b"));
}

// =============================================================================
// Callback integration: verify all phases fire in order
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, ReloadCallbacksFiredInOrder) {
    mgr_->registerModule("mod_c", *loader_);

    std::vector<HotReloadManager::ReloadPhase> phases_seen;
    mgr_->addReloadCallback(
        [&phases_seen](const std::string&, HotReloadManager::ReloadPhase p) {
            phases_seen.push_back(p);
        });

    // Reload will fail (nonexistent binary), but BEFORE_UNLOAD should still fire.
    mgr_->reloadModule("mod_c", "/nonexistent.so");

    // At least BEFORE_UNLOAD must have been emitted before the failure.
    ASSERT_FALSE(phases_seen.empty());
    EXPECT_EQ(phases_seen.front(), HotReloadManager::ReloadPhase::BEFORE_UNLOAD);
}

TEST_F(HotReloadManagerIntegrationTest, CallbackClearBetweenReloads) {
    mgr_->registerModule("mod_d", *loader_);

    int first_call_count  = 0;
    int second_call_count = 0;

    mgr_->addReloadCallback([&first_call_count](const std::string&, HotReloadManager::ReloadPhase) {
        ++first_call_count;
    });

    mgr_->reloadModule("mod_d", "/nonexistent.so");
    int first_count_snapshot = first_call_count;

    // Replace listeners.
    mgr_->clearReloadCallbacks();
    mgr_->addReloadCallback([&second_call_count](const std::string&, HotReloadManager::ReloadPhase) {
        ++second_call_count;
    });

    mgr_->reloadModule("mod_d", "/nonexistent.so");

    // Old listener must not have been called in the second reload.
    EXPECT_EQ(first_call_count, first_count_snapshot);
    // New listener must have been called at least once.
    EXPECT_GT(second_call_count, 0);
}

// =============================================================================
// State preservation integration
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, StateSaveCallbackInvokedOnReloadAttempt) {
    mgr_->registerModule("stateful_mod", *loader_);

    bool save_called = false;
    mgr_->setStateSaveCallback([&save_called](const std::string& name) -> std::string {
        save_called = true;
        return R"({"counter":42})";
    });

    // Even a failed reload should trigger state-save before the attempt.
    mgr_->reloadModule("stateful_mod", "/nonexistent.so");

    EXPECT_TRUE(save_called);
}

TEST_F(HotReloadManagerIntegrationTest, StateRestoreCallbackNotInvokedOnFailedReload) {
    mgr_->registerModule("stateful_mod2", *loader_);

    bool restore_called = false;
    mgr_->setStateSaveCallback([](const std::string&) -> std::string {
        return R"({"counter":7})";
    });
    mgr_->setStateRestoreCallback([&restore_called](const std::string&, const std::string&) -> bool {
        restore_called = true;
        return true;
    });

    // Reload fails → restore must NOT be called.
    mgr_->reloadModule("stateful_mod2", "/nonexistent.so");

    EXPECT_FALSE(restore_called);
}

// =============================================================================
// Multiple-module isolation
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, MultipleModulesAreIndependent) {
    mgr_->registerModule("alpha", *loader_);
    mgr_->registerModule("beta",  *loader_);
    mgr_->registerModule("gamma", *loader_);

    auto names = mgr_->registeredModules();
    ASSERT_EQ(names.size(), 3u);

    // Reload attempt on one module must not affect the others.
    mgr_->reloadModule("alpha", "/nonexistent.so");

    auto stats = mgr_->getStats();
    EXPECT_EQ(stats.totalReloads, 1u);

    // beta and gamma are still registered and intact.
    EXPECT_FALSE(mgr_->isRollbackAvailable("beta"));
    EXPECT_FALSE(mgr_->isRollbackAvailable("gamma"));

    // Unregister one and verify count.
    mgr_->unregisterModule("beta");
    EXPECT_EQ(mgr_->registeredModules().size(), 2u);
}

// =============================================================================
// Stats reset integration
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, StatsResetBetweenTestPhases) {
    mgr_->registerModule("mod_stats", *loader_);

    // Phase 1: cause failures.
    for (int i = 0; i < 3; ++i) {
        mgr_->reloadModule("mod_stats", "/nonexistent.so");
    }

    auto stats1 = mgr_->getStats();
    EXPECT_EQ(stats1.failedReloads, 3u);

    // Reset.
    mgr_->resetStats();

    auto stats2 = mgr_->getStats();
    EXPECT_EQ(stats2.totalReloads,  0u);
    EXPECT_EQ(stats2.failedReloads, 0u);

    // Phase 2: cause more failures.
    mgr_->reloadModule("mod_stats", "/another_nonexistent.so");
    auto stats3 = mgr_->getStats();
    EXPECT_EQ(stats3.failedReloads, 1u);
}

// =============================================================================
// Version query integration
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, GetCurrentVersionUnloadedModuleReturnsNullopt) {
    mgr_->registerModule("mod_version", *loader_);

    // Module is registered but loader has no entry for it → no version.
    auto ver = mgr_->getCurrentVersion("mod_version");
    EXPECT_FALSE(ver.has_value());
}

TEST_F(HotReloadManagerIntegrationTest, GetCurrentVersionUnregisteredModuleReturnsNullopt) {
    auto ver = mgr_->getCurrentVersion("totally_unknown");
    EXPECT_FALSE(ver.has_value());
}

// =============================================================================
// Concurrent reload integration
// =============================================================================

TEST_F(HotReloadManagerIntegrationTest, ConcurrentReloadsDoNotCorruptState) {
    mgr_->registerModule("concurrent_mod", *loader_);

    const int N = 8;
    std::vector<std::thread> threads;
    std::atomic<int>         fail_count{0};
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, &fail_count]() {
            auto r = mgr_->reloadModule("concurrent_mod", "/nonexistent.so");
            if (!r.success) {
              ++fail_count;
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    // All reloads must have failed (no real binary) without any crash.
    EXPECT_EQ(fail_count.load(), N);

    auto stats = mgr_->getStats();
    EXPECT_EQ(stats.totalReloads,  static_cast<uint64_t>(N));
    EXPECT_EQ(stats.failedReloads, static_cast<uint64_t>(N));
}
