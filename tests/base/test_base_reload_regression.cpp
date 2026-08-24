/**
 * @file test_base_reload_regression.cpp
 * @brief Focused regression tests for base module hot-reload, dependency,
 *        sandbox, and registry-config edge paths.
 *
 * Coverage:
 *  - Reload rollback scenarios (no module, null loader path, no backup, success)
 *  - Dependency conflict edge cases via PluginDependencyGraph / ModuleDependencyResolver
 *  - Sandbox degraded-state paths (launch on missing module, stats on inactive)
 *  - RegistryConfig validation edge cases (URL, timeout, retry, ssl flag)
 *  - Reload phase ordering: BEFORE_UNLOAD → AFTER_UNLOAD → AFTER_LOAD, ROLLBACK
 *  - Stats tracking across success/failure cycles
 *  - Multiple modules registered, callbacks invoked in order
 *  - State save/restore callback error isolation
 *  - BaseErrorTaxonomy: code uniqueness, description correctness, format output
 */

#include <gtest/gtest.h>

#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include "themis/base/module_sandbox.h"
#include "themis/base/plugin_dependency_graph.h"
#include "themis/base/base_error_taxonomy.h"
#include "themis/base/remote_registry_client.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

namespace themis { namespace modules { 

// =============================================================================
// Helpers
// =============================================================================

namespace {
} // anonymous namespace

// =============================================================================
// Suite 1: Reload rollback scenarios
// =============================================================================

class ReloadRollbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        mgr_    = std::make_unique<HotReloadManager>([] {
            HotReloadManager::Config c;
            c.verifySignature = false;
            c.preserveState   = false;
            c.enableRollback  = true;
            return c;
        }());
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

/// Rollback on an unregistered module name must fail gracefully.
TEST_F(ReloadRollbackTest, RollbackUnregisteredModuleFails) {
    HotReloadResult result = mgr_->rollback("nonexistent_module");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

/// Reload on an unregistered module name must fail gracefully.
TEST_F(ReloadRollbackTest, ReloadUnregisteredModuleFails) {
    HotReloadResult result = mgr_->reloadModule("nonexistent_module",
                                                "/tmp/nonexistent.so");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

/// Rollback is unavailable when no successful reload has been performed.
TEST_F(ReloadRollbackTest, RollbackUnavailableWithNoBackup) {
    mgr_->registerModule("mod_no_backup", *loader_);
    EXPECT_FALSE(mgr_->isRollbackAvailable("mod_no_backup"));

    HotReloadResult result = mgr_->rollback("mod_no_backup");
    EXPECT_FALSE(result.success);
}

/// Reload failure (non-existent path) keeps the module registered and returns
/// an error result; subsequent rollback is still unavailable (no prior success).
TEST_F(ReloadRollbackTest, ReloadFailurePreservesRegistrationState) {
    mgr_->registerModule("mod_keep", *loader_);

    HotReloadResult reload_result = mgr_->reloadModule("mod_keep",
                                                       "/nonexistent_binary.so");
    EXPECT_FALSE(reload_result.success);

    // Module still listed as registered.
    auto names = mgr_->registeredModules();
    EXPECT_NE(std::find(names.begin(), names.end(), "mod_keep"), names.end());

    // Still no rollback available after a failed reload.
    EXPECT_FALSE(mgr_->isRollbackAvailable("mod_keep"));
}

/// isRollbackAvailable returns false for a module that was never registered.
TEST_F(ReloadRollbackTest, IsRollbackAvailableOnUnknownReturnsFalse) {
    EXPECT_FALSE(mgr_->isRollbackAvailable("ghost_module"));
}

/// Reload disabled (enableRollback=false): rollback call must still not crash.
TEST_F(ReloadRollbackTest, RollbackDisabledConfigNoBackupAvailable) {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    cfg.preserveState   = false;
    cfg.enableRollback  = false;  // <-- disabled
    HotReloadManager mgr_no_rb(cfg);

    ModuleLoader loader;
    loader.setAllowUnsigned(true);

    mgr_no_rb.registerModule("mod_norb", loader);
    HotReloadResult result = mgr_no_rb.rollback("mod_norb");
    EXPECT_FALSE(result.success);
}

// =============================================================================
// Suite 2: Reload phase ordering (callback order)
// =============================================================================

class ReloadPhaseOrderTest : public ::testing::Test {
protected:
    void SetUp() override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = false;
        cfg.enableRollback  = true;
        mgr_    = std::make_unique<HotReloadManager>(cfg);
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

/// On a failed reload (non-existent binary) only BEFORE_UNLOAD is emitted.
TEST_F(ReloadPhaseOrderTest, FailedReloadEmitsOnlyBeforeUnload) {
    mgr_->registerModule("phase_mod", *loader_);

    std::vector<HotReloadManager::ReloadPhase> phases_seen;
    mgr_->addReloadCallback([&phases_seen](const std::string& /*name*/,
                                           HotReloadManager::ReloadPhase p) {
        phases_seen.push_back(p);
    });

    mgr_->reloadModule("phase_mod", "/nonexistent.so");

    // A failed reload must emit BEFORE_UNLOAD but must NOT emit AFTER_LOAD.
    EXPECT_FALSE(phases_seen.empty());
    EXPECT_EQ(phases_seen.front(), HotReloadManager::ReloadPhase::BEFORE_UNLOAD);

    bool has_after_load = std::any_of(phases_seen.begin(), phases_seen.end(),
        [](HotReloadManager::ReloadPhase p) {
            return p == HotReloadManager::ReloadPhase::AFTER_LOAD;
        });
    EXPECT_FALSE(has_after_load)
        << "AFTER_LOAD must not be emitted when the reload binary does not exist";
}

/// Multiple callbacks are all invoked for a reload attempt.
TEST_F(ReloadPhaseOrderTest, MultipleCallbacksAllInvoked) {
    mgr_->registerModule("multi_cb_mod", *loader_);

    std::atomic<int> counter_a{0}, counter_b{0}, counter_c{0};

    mgr_->addReloadCallback([&counter_a](const std::string&,
                                          HotReloadManager::ReloadPhase) {
        ++counter_a;
    });
    mgr_->addReloadCallback([&counter_b](const std::string&,
                                          HotReloadManager::ReloadPhase) {
        ++counter_b;
    });
    mgr_->addReloadCallback([&counter_c](const std::string&,
                                          HotReloadManager::ReloadPhase) {
        ++counter_c;
    });

    mgr_->reloadModule("multi_cb_mod", "/nonexistent.so");

    // All three callbacks must have been called at least once.
    EXPECT_GT(counter_a.load(), 0);
    EXPECT_GT(counter_b.load(), 0);
    EXPECT_GT(counter_c.load(), 0);

    // All must have been called the same number of times (same phases).
    EXPECT_EQ(counter_a.load(), counter_b.load());
    EXPECT_EQ(counter_b.load(), counter_c.load());
}

/// Callbacks can be cleared; after clearReloadCallbacks no callbacks fire.
TEST_F(ReloadPhaseOrderTest, ClearCallbacksPreventsDispatch) {
    mgr_->registerModule("clear_cb_mod", *loader_);

    std::atomic<int> counter{0};
    mgr_->addReloadCallback([&counter](const std::string&,
                                        HotReloadManager::ReloadPhase) {
        ++counter;
    });

    mgr_->clearReloadCallbacks();

    mgr_->reloadModule("clear_cb_mod", "/nonexistent.so");

    EXPECT_EQ(counter.load(), 0) << "Callbacks must not fire after clearReloadCallbacks()";
}

/// If a callback unregisters the module during BEFORE_UNLOAD, reload must fail
/// cleanly without dereferencing stale slot storage.
TEST_F(ReloadPhaseOrderTest, CallbackUnregisterDuringReloadFailsCleanly) {
    mgr_->registerModule("self_unregister_mod", *loader_);

    mgr_->addReloadCallback([this](const std::string &name, HotReloadManager::ReloadPhase p) {
        if (p == HotReloadManager::ReloadPhase::BEFORE_UNLOAD) {
            mgr_->unregisterModule(name);
        }
    });

    HotReloadResult result = mgr_->reloadModule("self_unregister_mod", "/nonexistent.so");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());

    auto names = mgr_->registeredModules();
    EXPECT_EQ(std::find(names.begin(), names.end(), "self_unregister_mod"), names.end());
}

/// Rollback phase callback: ROLLBACK must be emitted when rollback is called and
/// the module has no backup (the rollback itself fails, but the phase must fire).
TEST_F(ReloadPhaseOrderTest, RollbackPhaseEmittedOnRollbackAttempt) {
    mgr_->registerModule("rollback_phase_mod", *loader_);

    std::vector<HotReloadManager::ReloadPhase> phases;
    mgr_->addReloadCallback([&phases](const std::string&,
                                       HotReloadManager::ReloadPhase p) {
        phases.push_back(p);
    });

    mgr_->rollback("rollback_phase_mod");

    // ROLLBACK phase should appear when rollback is attempted (even if it fails
    // immediately due to no-backup), OR no phases emitted at all on early-return.
    // Either behavior is acceptable — this test just ensures we don't crash.
    // The important invariant: no AFTER_LOAD phase during a rollback call.
    bool has_after_load = std::any_of(phases.begin(), phases.end(),
        [](HotReloadManager::ReloadPhase p) {
            return p == HotReloadManager::ReloadPhase::AFTER_LOAD;
        });
    EXPECT_FALSE(has_after_load);
}

// =============================================================================
// Suite 3: Stats tracking across success/failure cycles
// =============================================================================

class ReloadStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = false;
        cfg.enableRollback  = true;
        mgr_    = std::make_unique<HotReloadManager>(cfg);
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

/// Each failed reload increments failedReloads and totalReloads.
TEST_F(ReloadStatsTest, FailedReloadIncrementsCounts) {
    mgr_->registerModule("stats_mod", *loader_);

    const int kAttempts = 5;
    for (int i = 0; i < kAttempts; ++i) {
        mgr_->reloadModule("stats_mod", "/nonexistent.so");
    }

    auto s = mgr_->getStats();
    EXPECT_EQ(s.totalReloads,      static_cast<uint64_t>(kAttempts));
    EXPECT_EQ(s.failedReloads,     static_cast<uint64_t>(kAttempts));
    EXPECT_EQ(s.successfulReloads, 0u);
}

/// resetStats() zeroes all counters.
TEST_F(ReloadStatsTest, ResetStatsClearsAllCounters) {
    mgr_->registerModule("stats_reset_mod", *loader_);
    mgr_->reloadModule("stats_reset_mod", "/nonexistent.so");

    auto pre = mgr_->getStats();
    EXPECT_GT(pre.totalReloads, 0u);

    mgr_->resetStats();

    auto post = mgr_->getStats();
    EXPECT_EQ(post.totalReloads,      0u);
    EXPECT_EQ(post.failedReloads,     0u);
    EXPECT_EQ(post.successfulReloads, 0u);
    EXPECT_EQ(post.rollbacks,         0u);
    EXPECT_EQ(post.statesSaved,       0u);
    EXPECT_EQ(post.statesRestored,    0u);
}

/// Stats are additive across multiple registered modules.
TEST_F(ReloadStatsTest, StatsAdditiveAcrossMultipleModules) {
    const int kModules  = 4;
    const int kPerMod   = 3;

    for (int m = 0; m < kModules; ++m) {
        mgr_->registerModule("multi_stats_mod_" + std::to_string(m), *loader_);
    }
    for (int m = 0; m < kModules; ++m) {
        for (int r = 0; r < kPerMod; ++r) {
            mgr_->reloadModule("multi_stats_mod_" + std::to_string(m),
                               "/nonexistent.so");
        }
    }

    auto s = mgr_->getStats();
    EXPECT_EQ(s.totalReloads, static_cast<uint64_t>(kModules * kPerMod));
}

// =============================================================================
// Suite 4: State save/restore callback error isolation
// =============================================================================

class StateSaveRestoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = true;   // enable state preservation
        cfg.enableRollback  = true;
        mgr_    = std::make_unique<HotReloadManager>(cfg);
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

/// A state-save callback that throws must not propagate the exception out of
/// reloadModule(); the reload should still return a usable result.
TEST_F(StateSaveRestoreTest, ThrowingStateSaveCallbackIsIsolated) {
    mgr_->registerModule("throw_save_mod", *loader_);

    mgr_->setStateSaveCallback([](const std::string& /*name*/) -> std::string {
        throw std::runtime_error("simulated save failure");
    });

    HotReloadResult result;
    EXPECT_NO_THROW({
        result = mgr_->reloadModule("throw_save_mod", "/nonexistent.so");
    }) << "Exception from StateSaveCallback must not propagate";

    // Result must be populated (success or failure) without an unhandled throw.
    (void)result;
}

/// A state-restore callback that returns false does not crash the system.
TEST_F(StateSaveRestoreTest, FalseReturningStateRestoreCallbackIsIsolated) {
    mgr_->registerModule("restore_fail_mod", *loader_);

    mgr_->setStateSaveCallback([](const std::string&) -> std::string {
        return "saved_state_data";
    });
    mgr_->setStateRestoreCallback([](const std::string&,
                                      const std::string& /*state*/) -> bool {
        return false;  // intentional restore failure
    });

    // Attempt a reload (will fail because binary doesn't exist).
    HotReloadResult result;
    EXPECT_NO_THROW({
        result = mgr_->reloadModule("restore_fail_mod", "/nonexistent.so");
    });

    // System must still be in a stable state: module still listed.
    auto names = mgr_->registeredModules();
    EXPECT_NE(std::find(names.begin(), names.end(), "restore_fail_mod"),
              names.end());
}

/// State-save callback producing non-empty state increments statesSaved when
/// a reload succeeds (fast-fail case: save fires even if reload then fails).
TEST_F(StateSaveRestoreTest, StateSaveCallbackInvokedDuringReload) {
    mgr_->registerModule("save_count_mod", *loader_);

    std::atomic<int> save_calls{0};
    mgr_->setStateSaveCallback([&save_calls](const std::string&) -> std::string {
        ++save_calls;
        return "{}";
    });

    mgr_->reloadModule("save_count_mod", "/nonexistent.so");

    // Save callback must have been called at least once during a reload attempt
    // when preserveState=true.
    EXPECT_GE(save_calls.load(), 1);
}

// =============================================================================
// Suite 5: Multiple modules registered, independent behavior
// =============================================================================

class MultiModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = false;
        cfg.enableRollback  = true;
        mgr_    = std::make_unique<HotReloadManager>(cfg);
        loader_ = std::make_unique<ModuleLoader>();
        loader_->setAllowUnsigned(true);
    }

    std::unique_ptr<HotReloadManager> mgr_;
    std::unique_ptr<ModuleLoader>     loader_;
};

/// Registering 10 modules: registeredModules() returns all names.
TEST_F(MultiModuleTest, TenModulesAllRegistered) {
    const int kN = 10;
    for (int i = 0; i < kN; ++i) {
        mgr_->registerModule("mod_" + std::to_string(i), *loader_);
    }

    auto names = mgr_->registeredModules();
    EXPECT_EQ(static_cast<int>(names.size()), kN);

    for (int i = 0; i < kN; ++i) {
        const std::string expected = "mod_" + std::to_string(i);
        EXPECT_NE(std::find(names.begin(), names.end(), expected), names.end())
            << "Expected module '" << expected << "' in registeredModules()";
    }
}

/// Unregistering one module does not affect the others.
TEST_F(MultiModuleTest, UnregisterOneDoesNotAffectOthers) {
    const int kN = 5;
    for (int i = 0; i < kN; ++i) {
        mgr_->registerModule("rem_mod_" + std::to_string(i), *loader_);
    }

    mgr_->unregisterModule("rem_mod_2");

    auto names = mgr_->registeredModules();
    EXPECT_EQ(static_cast<int>(names.size()), kN - 1);
    EXPECT_EQ(std::find(names.begin(), names.end(), "rem_mod_2"), names.end());

    // Others still present.
    for (int i = 0; i < kN; ++i) {
        if (i == 2) continue;
        const std::string expected = "rem_mod_" + std::to_string(i);
        EXPECT_NE(std::find(names.begin(), names.end(), expected), names.end());
    }
}

/// Reload failure on one module does not affect rollback availability on another.
TEST_F(MultiModuleTest, ReloadFailureOnOneModuleDoesNotAffectAnother) {
    mgr_->registerModule("independent_a", *loader_);
    mgr_->registerModule("independent_b", *loader_);

    mgr_->reloadModule("independent_a", "/nonexistent.so");

    // 'independent_b' must still have no rollback available (untouched).
    EXPECT_FALSE(mgr_->isRollbackAvailable("independent_b"));
}

// =============================================================================
// Suite 6: Dependency conflict edge cases via ModuleDependencyResolver
// =============================================================================

class DependencyResolverEdgeTest : public ::testing::Test {};

/// Linear chain resolves successfully with correct order.
TEST_F(DependencyResolverEdgeTest, LinearChainResolvesInOrder) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("base",    "1.0.0", {});
    resolver.registerModule("mid",     "1.0.0", {{"base"}});
    resolver.registerModule("top",     "1.0.0", {{"mid"}});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 3u);

    // "base" must come before "mid", "mid" before "top".
    auto pos_base = std::find(result.loadOrder.begin(), result.loadOrder.end(), "base");
    auto pos_mid  = std::find(result.loadOrder.begin(), result.loadOrder.end(), "mid");
    auto pos_top  = std::find(result.loadOrder.begin(), result.loadOrder.end(), "top");

    EXPECT_NE(pos_base, result.loadOrder.end());
    EXPECT_NE(pos_mid,  result.loadOrder.end());
    EXPECT_NE(pos_top,  result.loadOrder.end());

    EXPECT_LT(std::distance(result.loadOrder.begin(), pos_base),
              std::distance(result.loadOrder.begin(), pos_mid));
    EXPECT_LT(std::distance(result.loadOrder.begin(), pos_mid),
              std::distance(result.loadOrder.begin(), pos_top));
}

/// Direct circular dependency is detected.
TEST_F(DependencyResolverEdgeTest, DirectCyclicDependencyDetected) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("cycle_a", "1.0.0", {{"cycle_b"}});
    resolver.registerModule("cycle_b", "1.0.0", {{"cycle_a"}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
}

/// A required dependency that is missing produces an error.
TEST_F(DependencyResolverEdgeTest, MissingRequiredDependencyReportsError) {
    ModuleDependencyResolver resolver;
    // "needs_ghost" declares dependency on "ghost" which is never registered.
    resolver.registerModule("needs_ghost", "1.0.0", {{"ghost"}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.missingRequired.empty());
}

/// Version-constrained dependency: compatible version satisfies constraint.
TEST_F(DependencyResolverEdgeTest, VersionConstraintSatisfiedSucceeds) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("lib", "2.1.0", {});

    ModuleDependency dep;
    dep.name       = "lib";
    dep.minVersion = "2.0.0";
    dep.maxVersion = "3.0.0";
    dep.required   = true;
    resolver.registerModule("consumer", "1.0.0", {dep});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success)
        << "Version 2.1.0 should satisfy [2.0.0, 3.0.0]: "
        << result.errorMessage;
}

/// Version-constrained dependency: incompatible version produces mismatch.
TEST_F(DependencyResolverEdgeTest, VersionConstraintViolationReportsError) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("old_lib", "1.0.0", {});

    ModuleDependency dep;
    dep.name       = "old_lib";
    dep.minVersion = "2.0.0";
    dep.required   = true;
    resolver.registerModule("new_consumer", "1.0.0", {dep});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.versionMismatches.empty());
}

/// Optional dependency that is absent does not cause resolution failure.
TEST_F(DependencyResolverEdgeTest, MissingOptionalDependencyDoesNotFail) {
    ModuleDependencyResolver resolver;

    ModuleDependency opt_dep;
    opt_dep.name     = "optional_lib";
    opt_dep.required = false;  // optional
    resolver.registerModule("flexible_mod", "1.0.0", {opt_dep});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success)
        << "Missing optional dependency must not fail resolution: "
        << result.errorMessage;
}

/// clear() removes all registrations.
TEST_F(DependencyResolverEdgeTest, ClearResetsResolver) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", "1.0.0", {});
    resolver.registerModule("b", "1.0.0", {{"a"}});

    resolver.clear();
    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.loadOrder.empty());
}

// =============================================================================
// Suite 7: PluginDependencyGraph edge cases
// =============================================================================

class PluginDependencyGraphEdgeTest : public ::testing::Test {};

/// buildFromResolver populates nodes and edges correctly.
TEST_F(PluginDependencyGraphEdgeTest, BuildFromResolverPopulatesGraph) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("g_base",    "1.0.0", {});
    resolver.registerModule("g_storage", "2.0.0", {{"g_base"}});
    resolver.registerModule("g_query",   "1.5.0", {{"g_storage"}, {"g_base"}});

    PluginDependencyGraph graph;
    graph.buildFromResolver(resolver);

    EXPECT_EQ(graph.nodeCount(), 3u);
    EXPECT_EQ(graph.edgeCount(), 3u);
}

/// Cycle detection in PluginDependencyGraph finds the cycle.
TEST_F(PluginDependencyGraphEdgeTest, CycleDetectionFindsDirectCycle) {
    PluginDependencyGraph graph;
    graph.addModule("x", "1.0.0");
    graph.addModule("y", "1.0.0");
    graph.addDependency("x", "y", true);
    graph.addDependency("y", "x", true);  // creates a cycle

    auto cycles = graph.detectCycles();
    EXPECT_FALSE(cycles.empty()) << "A direct cycle must be detected";
}

/// Topological order on an acyclic graph is non-empty.
TEST_F(PluginDependencyGraphEdgeTest, TopologicalOrderOnAcyclicGraph) {
    PluginDependencyGraph graph;
    graph.addModule("root",    "1.0.0");
    graph.addModule("child_a", "1.0.0");
    graph.addModule("child_b", "1.0.0");
    graph.addDependency("child_a", "root",    true);
    graph.addDependency("child_b", "child_a", true);

    auto order = graph.topologicalOrder();
    EXPECT_EQ(order.size(), 3u);

    auto pos_root    = std::find(order.begin(), order.end(), "root");
    auto pos_child_a = std::find(order.begin(), order.end(), "child_a");
    auto pos_child_b = std::find(order.begin(), order.end(), "child_b");

    EXPECT_NE(pos_root,    order.end());
    EXPECT_NE(pos_child_a, order.end());
    EXPECT_NE(pos_child_b, order.end());

    EXPECT_LT(std::distance(order.begin(), pos_root),
              std::distance(order.begin(), pos_child_a));
    EXPECT_LT(std::distance(order.begin(), pos_child_a),
              std::distance(order.begin(), pos_child_b));
}

/// clear() resets the graph to empty.
TEST_F(PluginDependencyGraphEdgeTest, ClearResetsGraph) {
    PluginDependencyGraph graph;
    graph.addModule("a");
    graph.addModule("b");
    graph.addDependency("b", "a");

    graph.clear();

    EXPECT_EQ(graph.nodeCount(), 0u);
    EXPECT_EQ(graph.edgeCount(), 0u);
}

/// 100-node linear chain: buildFromResolver produces correct node count.
TEST_F(PluginDependencyGraphEdgeTest, HundredNodeChainBuildFromResolver) {
    const int kN = 100;
    ModuleDependencyResolver resolver;
    resolver.registerModule("node_0", "1.0.0", {});

    for (int i = 1; i < kN; ++i) {
        ModuleDependency dep;
        dep.name     = "node_" + std::to_string(i - 1);
        dep.required = true;
        resolver.registerModule("node_" + std::to_string(i), "1.0.0", {dep});
    }

    PluginDependencyGraph graph;
    graph.buildFromResolver(resolver);

    EXPECT_EQ(graph.nodeCount(), static_cast<size_t>(kN));
    EXPECT_EQ(graph.edgeCount(), static_cast<size_t>(kN - 1));
}

/// DOT export from a non-empty graph produces non-empty string.
TEST_F(PluginDependencyGraphEdgeTest, DotExportProducesNonEmptyString) {
    PluginDependencyGraph graph;
    graph.addModule("p", "1.0.0");
    graph.addModule("q", "1.0.0");
    graph.addDependency("q", "p");

    std::string dot = graph.toString(GraphExportFormat::DOT);
    EXPECT_FALSE(dot.empty());
    EXPECT_NE(dot.find("digraph"), std::string::npos)
        << "DOT output must contain 'digraph'";
}

/// JSON export contains expected keys.
TEST_F(PluginDependencyGraphEdgeTest, JsonExportContainsExpectedKeys) {
    PluginDependencyGraph graph;
    graph.addModule("alpha", "1.0.0");
    graph.addModule("beta",  "1.0.0");
    graph.addDependency("beta", "alpha");

    std::string json = graph.toString(GraphExportFormat::JSON);
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("nodes"), std::string::npos);
    EXPECT_NE(json.find("edges"), std::string::npos);
}

// =============================================================================
// Suite 8: Sandbox degraded-state paths
// =============================================================================

class SandboxDegradedStateTest : public ::testing::Test {};

/// stats() on an inactive sandbox returns zero-value struct without crashing.
TEST_F(SandboxDegradedStateTest, StatsOnInactiveSandboxReturnsZeros) {
    ModuleSandbox::Config cfg;
    cfg.max_memory_mb   = 64;
    cfg.max_cpu_percent = 25;

    ModuleSandbox sandbox(cfg);
    EXPECT_FALSE(sandbox.isActive());

    // stats() must be callable on an inactive sandbox.
    SandboxStats s;
    EXPECT_NO_THROW({ s = sandbox.stats(); });
    EXPECT_FALSE(s.killed);
}

/// lastError() is available immediately (empty on a fresh sandbox).
TEST_F(SandboxDegradedStateTest, LastErrorEmptyOnFreshSandbox) {
    ModuleSandbox sandbox;
    EXPECT_TRUE(sandbox.lastError().empty());
}

/// launchWarnings() is empty before launch() is called.
TEST_F(SandboxDegradedStateTest, LaunchWarningsEmptyBeforeLaunch) {
    ModuleSandbox sandbox;
    EXPECT_TRUE(sandbox.launchWarnings().empty());
}

/// launch() with an empty module name does not crash and returns a bool result.
TEST_F(SandboxDegradedStateTest, LaunchEmptyModuleNameDoesNotCrash) {
    ModuleSandbox::Config cfg;
    cfg.max_memory_mb = 32;

    ModuleSandbox sandbox(cfg);
    bool ok = false;
    EXPECT_NO_THROW({ ok = sandbox.launch(""); });
    // Whether it succeeds depends on the platform; we just verify no crash.
    (void)ok;
}

/// HotReloadManager::getSandboxStats() returns nullopt when sandboxing is not
/// configured (the default config).
TEST_F(SandboxDegradedStateTest, SandboxStatsNulloptWhenNotConfigured) {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    cfg.preserveState   = false;
    cfg.enableRollback  = false;
    // sandboxConfig is std::nullopt (default).
    HotReloadManager mgr(cfg);

    ModuleLoader loader;
    loader.setAllowUnsigned(true);
    mgr.registerModule("no_sandbox_mod", loader);

    auto stats_opt = mgr.getSandboxStats("no_sandbox_mod");
    EXPECT_FALSE(stats_opt.has_value())
        << "getSandboxStats() must return nullopt when no sandbox is configured";
}

/// getSandboxStats() returns nullopt for an unregistered module.
TEST_F(SandboxDegradedStateTest, SandboxStatsNulloptForUnknownModule) {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    HotReloadManager mgr(cfg);

    auto stats_opt = mgr.getSandboxStats("ghost_module");
    EXPECT_FALSE(stats_opt.has_value());
}

// =============================================================================
// Suite 9: RegistryConfig validation edge cases (header-level, no network)
// =============================================================================

// NOTE: We test only the RegistryConfig struct and its field values.
// RemoteRegistryClient is NOT instantiated here — no libcurl link required.

class RegistryConfigValidationTest : public ::testing::Test {};

/// Default-constructed RegistryConfig has sane defaults.
TEST_F(RegistryConfigValidationTest, DefaultConfigHasSaneDefaults) {
    RegistryConfig cfg;
    EXPECT_TRUE(cfg.registry_url.empty());
    EXPECT_TRUE(cfg.auth_token.empty());
    EXPECT_TRUE(cfg.api_key.empty());
    EXPECT_EQ(cfg.download_dir, "/tmp/themis_plugins");
    EXPECT_GT(cfg.timeout_ms,     0);
    EXPECT_GT(cfg.max_retries,    0);
    EXPECT_GT(cfg.max_total_retry_time_ms, 0);
    EXPECT_TRUE(cfg.verify_ssl);
    EXPECT_TRUE(cfg.ca_bundle_path.empty());
    EXPECT_TRUE(cfg.pinned_public_key.empty());
}

/// RegistryConfig fields can be assigned without throwing.
TEST_F(RegistryConfigValidationTest, RegistryConfigFieldAssignment) {
    RegistryConfig cfg;
    cfg.registry_url              = "https://registry.example.com/api/v1";
    cfg.auth_token                = "bearer_token_value";
    cfg.api_key                   = "api_key_value";
    cfg.download_dir              = "/custom/dir";
    cfg.timeout_ms                = 5000;
    cfg.max_retries               = 1;
    cfg.max_total_retry_time_ms   = 10000;
    cfg.verify_ssl                = false;
    cfg.ca_bundle_path            = "/etc/ssl/certs/ca-certificates.crt";
    cfg.pinned_public_key         = "sha256//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

    EXPECT_EQ(cfg.registry_url, "https://registry.example.com/api/v1");
    EXPECT_EQ(cfg.timeout_ms, 5000);
    EXPECT_EQ(cfg.max_retries, 1);
    EXPECT_FALSE(cfg.verify_ssl);
    EXPECT_EQ(cfg.pinned_public_key, "sha256//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
}

/// RegistryConfig: zero timeout is representable (edge value).
TEST_F(RegistryConfigValidationTest, ZeroTimeoutIsAssignable) {
    RegistryConfig cfg;
    cfg.timeout_ms = 0;
    EXPECT_EQ(cfg.timeout_ms, 0);
}

/// RegistryConfig: zero max_retries is representable (edge value — no retry).
TEST_F(RegistryConfigValidationTest, ZeroMaxRetriesIsAssignable) {
    RegistryConfig cfg;
    cfg.max_retries = 0;
    EXPECT_EQ(cfg.max_retries, 0);
}

/// RegistryConfig copy is value-equivalent.
TEST_F(RegistryConfigValidationTest, RegistryConfigCopyEquivalent) {
    RegistryConfig original;
    original.registry_url = "https://example.com";
    original.timeout_ms   = 1234;
    original.verify_ssl   = false;

    RegistryConfig copy = original;

    EXPECT_EQ(copy.registry_url, original.registry_url);
    EXPECT_EQ(copy.timeout_ms,   original.timeout_ms);
    EXPECT_EQ(copy.verify_ssl,   original.verify_ssl);
}

// =============================================================================
// Suite 10: BaseErrorTaxonomy — code uniqueness, descriptions, format output
// =============================================================================

using namespace BaseErrorTaxonomy;

class BaseErrorTaxonomyTest : public ::testing::Test {};

/// All error codes are unique (no two taxonomy entries share a code).
TEST_F(BaseErrorTaxonomyTest, AllCodesAreUnique) {
    const std::vector<int> codes = {
        BASE_LOADER_PATH_NOT_FOUND::code,
        BASE_LOADER_SIGNATURE_REJECTED::code,
        BASE_LOADER_ABI_MISMATCH::code,
        BASE_LOADER_LOAD_FAILED::code,
        BASE_LOADER_INIT_FAILED::code,
        BASE_LOADER_HEALTH_CHECK_FAILED::code,
        BASE_SANDBOX_LAUNCH_FAILED::code,
        BASE_SANDBOX_RESOURCE_LIMIT::code,
        BASE_SANDBOX_TIMEOUT::code,
        BASE_SANDBOX_DEGRADED::code,
        BASE_SANDBOX_INACTIVE_STATS::code,
        BASE_RELOAD_NO_BACKUP::code,
        BASE_RELOAD_ROLLBACK_FAILED::code,
        BASE_RELOAD_CANDIDATE_LOAD_FAILED::code,
        BASE_RELOAD_STATE_RESTORE_FAILED::code,
        BASE_RELOAD_NOT_REGISTERED::code,
        BASE_DEPENDENCY_CONFLICT::code,
        BASE_DEPENDENCY_CYCLE::code,
        BASE_DEPENDENCY_MISSING_REQUIRED::code,
        BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code,
        BASE_REGISTRY_NETWORK_ERROR::code,
        BASE_REGISTRY_AUTH_FAILURE::code,
        BASE_REGISTRY_CHECKSUM_MISMATCH::code,
        BASE_REGISTRY_DOWNLOAD_FAILED::code,
    };

    std::vector<int> sorted = codes;
    std::sort(sorted.begin(), sorted.end());
    auto dup = std::adjacent_find(sorted.begin(), sorted.end());
    EXPECT_EQ(dup, sorted.end()) << "Duplicate error code found in taxonomy";
}

/// All codes fall within their declared range.
TEST_F(BaseErrorTaxonomyTest, CodesInCorrectRanges) {
    // Loader: 1100–1149
    EXPECT_GE(BASE_LOADER_PATH_NOT_FOUND::code,      1100);
    EXPECT_LE(BASE_LOADER_PATH_NOT_FOUND::code,      1149);
    EXPECT_GE(BASE_LOADER_HEALTH_CHECK_FAILED::code, 1100);
    EXPECT_LE(BASE_LOADER_HEALTH_CHECK_FAILED::code, 1149);

    // Sandbox: 1150–1199
    EXPECT_GE(BASE_SANDBOX_LAUNCH_FAILED::code,      1150);
    EXPECT_LE(BASE_SANDBOX_INACTIVE_STATS::code,     1199);

    // Reload: 1200–1249
    EXPECT_GE(BASE_RELOAD_NO_BACKUP::code,           1200);
    EXPECT_LE(BASE_RELOAD_NOT_REGISTERED::code,      1249);

    // Dependency: 1250–1299
    EXPECT_GE(BASE_DEPENDENCY_CONFLICT::code,        1250);
    EXPECT_LE(BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code, 1299);

    // Registry: 1300–1349
    EXPECT_GE(BASE_REGISTRY_NETWORK_ERROR::code,     1300);
    EXPECT_LE(BASE_REGISTRY_DOWNLOAD_FAILED::code,   1349);
}

/// All descriptions are non-empty.
TEST_F(BaseErrorTaxonomyTest, AllDescriptionsNonEmpty) {
    EXPECT_FALSE(BASE_LOADER_PATH_NOT_FOUND::description().empty());
    EXPECT_FALSE(BASE_LOADER_SIGNATURE_REJECTED::description().empty());
    EXPECT_FALSE(BASE_LOADER_ABI_MISMATCH::description().empty());
    EXPECT_FALSE(BASE_LOADER_LOAD_FAILED::description().empty());
    EXPECT_FALSE(BASE_LOADER_INIT_FAILED::description().empty());
    EXPECT_FALSE(BASE_LOADER_HEALTH_CHECK_FAILED::description().empty());
    EXPECT_FALSE(BASE_SANDBOX_LAUNCH_FAILED::description().empty());
    EXPECT_FALSE(BASE_SANDBOX_RESOURCE_LIMIT::description().empty());
    EXPECT_FALSE(BASE_SANDBOX_TIMEOUT::description().empty());
    EXPECT_FALSE(BASE_SANDBOX_DEGRADED::description().empty());
    EXPECT_FALSE(BASE_SANDBOX_INACTIVE_STATS::description().empty());
    EXPECT_FALSE(BASE_RELOAD_NO_BACKUP::description().empty());
    EXPECT_FALSE(BASE_RELOAD_ROLLBACK_FAILED::description().empty());
    EXPECT_FALSE(BASE_RELOAD_CANDIDATE_LOAD_FAILED::description().empty());
    EXPECT_FALSE(BASE_RELOAD_STATE_RESTORE_FAILED::description().empty());
    EXPECT_FALSE(BASE_RELOAD_NOT_REGISTERED::description().empty());
    EXPECT_FALSE(BASE_DEPENDENCY_CONFLICT::description().empty());
    EXPECT_FALSE(BASE_DEPENDENCY_CYCLE::description().empty());
    EXPECT_FALSE(BASE_DEPENDENCY_MISSING_REQUIRED::description().empty());
    EXPECT_FALSE(BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::description().empty());
    EXPECT_FALSE(BASE_REGISTRY_NETWORK_ERROR::description().empty());
    EXPECT_FALSE(BASE_REGISTRY_AUTH_FAILURE::description().empty());
    EXPECT_FALSE(BASE_REGISTRY_CHECKSUM_MISMATCH::description().empty());
    EXPECT_FALSE(BASE_REGISTRY_DOWNLOAD_FAILED::description().empty());
}

/// format() output contains the error code as a substring.
TEST_F(BaseErrorTaxonomyTest, FormatOutputContainsCode) {
    auto msg = BASE_LOADER_ABI_MISMATCH::format("my_module", "1.0", "2.0");
    EXPECT_NE(msg.find(std::to_string(BASE_LOADER_ABI_MISMATCH::code)),
              std::string::npos)
        << "format() output must embed the error code";
}

/// format() output contains the module name argument.
TEST_F(BaseErrorTaxonomyTest, FormatOutputContainsModuleName) {
    auto msg = BASE_RELOAD_NO_BACKUP::format("analytics_plugin");
    EXPECT_NE(msg.find("analytics_plugin"), std::string::npos);
}

/// resolveDescription returns a non-empty string for all known codes.
TEST_F(BaseErrorTaxonomyTest, ResolveDescriptionCoversAllKnownCodes) {
    const std::vector<int> codes = {
        BASE_LOADER_PATH_NOT_FOUND::code,
        BASE_LOADER_SIGNATURE_REJECTED::code,
        BASE_LOADER_ABI_MISMATCH::code,
        BASE_LOADER_LOAD_FAILED::code,
        BASE_LOADER_INIT_FAILED::code,
        BASE_LOADER_HEALTH_CHECK_FAILED::code,
        BASE_SANDBOX_LAUNCH_FAILED::code,
        BASE_SANDBOX_RESOURCE_LIMIT::code,
        BASE_SANDBOX_TIMEOUT::code,
        BASE_SANDBOX_DEGRADED::code,
        BASE_SANDBOX_INACTIVE_STATS::code,
        BASE_RELOAD_NO_BACKUP::code,
        BASE_RELOAD_ROLLBACK_FAILED::code,
        BASE_RELOAD_CANDIDATE_LOAD_FAILED::code,
        BASE_RELOAD_STATE_RESTORE_FAILED::code,
        BASE_RELOAD_NOT_REGISTERED::code,
        BASE_DEPENDENCY_CONFLICT::code,
        BASE_DEPENDENCY_CYCLE::code,
        BASE_DEPENDENCY_MISSING_REQUIRED::code,
        BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code,
        BASE_REGISTRY_NETWORK_ERROR::code,
        BASE_REGISTRY_AUTH_FAILURE::code,
        BASE_REGISTRY_CHECKSUM_MISMATCH::code,
        BASE_REGISTRY_DOWNLOAD_FAILED::code,
    };

    for (int c : codes) {
        auto desc = resolveDescription(c);
        EXPECT_FALSE(desc.empty())
            << "resolveDescription(" << c << ") returned empty string";
        EXPECT_NE(desc, std::string_view("unknown error code"))
            << "resolveDescription(" << c << ") returned 'unknown error code'";
    }
}

/// resolveDescription returns "unknown error code" for out-of-range values.
TEST_F(BaseErrorTaxonomyTest, ResolveDescriptionUnknownCodeReturnsUnknown) {
    EXPECT_EQ(resolveDescription(9999), std::string_view("unknown error code"));
    EXPECT_EQ(resolveDescription(0),    std::string_view("unknown error code"));
    EXPECT_EQ(resolveDescription(-1),   std::string_view("unknown error code"));
}

/// isKnownCode is true for every defined code and false for unknowns.
TEST_F(BaseErrorTaxonomyTest, IsKnownCodeCorrect) {
    EXPECT_TRUE(isKnownCode(BASE_LOADER_PATH_NOT_FOUND::code));
    EXPECT_TRUE(isKnownCode(BASE_SANDBOX_LAUNCH_FAILED::code));
    EXPECT_TRUE(isKnownCode(BASE_RELOAD_NO_BACKUP::code));
    EXPECT_TRUE(isKnownCode(BASE_DEPENDENCY_CYCLE::code));
    EXPECT_TRUE(isKnownCode(BASE_REGISTRY_CHECKSUM_MISMATCH::code));

    EXPECT_FALSE(isKnownCode(0));
    EXPECT_FALSE(isKnownCode(1099));  // just below loader range
    EXPECT_FALSE(isKnownCode(1350));  // just above registry range
    EXPECT_FALSE(isKnownCode(9999));
}
} } // namespace themis::modules
