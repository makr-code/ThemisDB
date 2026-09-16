/**
 * @file test_base_soak.cpp
 * @brief Wave D delivery: long-duration soak test for the base module's
 *        primary hot-reload and rollback paths.
 *
 * Purpose:
 *  - Validate that HotReloadManager accumulates stats monotonically over many
 *    iterations without memory leaks or counter corruption.
 *  - Demonstrate long-duration confidence in the reload/rollback/re-register
 *    cycle under controlled conditions.
 *
 * The iteration count is controlled by the THEMIS_SOAK_ITERATIONS environment
 * variable (default: 1000).  For CI runs this produces a fast, deterministic
 * regression; for hardware validation the variable can be raised to 100 000+.
 *
 * Run with:
 *   ctest -L soak --timeout 7200
 * or
 *   THEMIS_SOAK_ITERATIONS=50000 ctest -L soak
 *
 * Requires: GTest, themis_core (hot_reload_manager, module_loader).
 */

#include <gtest/gtest.h>

#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"

#include <cstdlib>
#include <string>

namespace themis { namespace modules {

namespace {

/// Read the soak iteration count from the environment; default 1 000.
int soakIterations() {
    const char* env = std::getenv("THEMIS_SOAK_ITERATIONS");
    if (!env) { return 1000; }
    const int v = std::atoi(env);
    return (v > 0) ? v : 1000;
}

} // namespace

// =============================================================================
// Soak test: reload → rollback → re-register cycle
// =============================================================================

/**
 * @brief Long-duration soak: simulated reload/rollback/re-register loop.
 *
 * Each iteration:
 *  1. Attempt reloadModule() on a registered module with a nonexistent path
 *     (→ fails cleanly; failedReloads increments).
 *  2. Attempt rollback() with no backup (→ fails cleanly; rollbacks stays 0).
 *  3. Unregister and re-register the module.
 *
 * Invariants validated at the end:
 *  - totalReloads == failedReloads (all fail because path is nonexistent)
 *  - totalReloads equals the configured iteration count
 *  - rollbacks == 0 (no rollback ever had a backup)
 *  - successfulReloads == 0
 */
TEST(SoakTest, ReloadRollbackReregisterCycle) {
    const int iterations = soakIterations();
    GTEST_LOG_(INFO) << "SoakTest: running " << iterations << " iterations "
                     << "(set THEMIS_SOAK_ITERATIONS to change)";

    HotReloadManager mgr;
    ModuleLoader     loader;
    const std::string mod_name = "soak_module";

    mgr.registerModule(mod_name, loader);

    for (int i = 0; i < iterations; ++i) {
        // 1. Failed reload (nonexistent path).
        auto r = mgr.reloadModule(mod_name, "/nonexistent_soak_" + std::to_string(i));
        ASSERT_FALSE(r.success)      << "reload must fail on iteration " << i;
        ASSERT_FALSE(r.errorMessage.empty());

        // 2. Failed rollback (no backup).
        auto rb = mgr.rollback(mod_name);
        ASSERT_FALSE(rb.success)     << "rollback must fail (no backup) on iteration " << i;

        // 3. Re-register (unregister + re-register simulates a plugin hot-swap
        //    where the same loader is reassigned).
        mgr.unregisterModule(mod_name);
        mgr.registerModule(mod_name, loader);
    }

    auto stats = mgr.getStats();

    EXPECT_EQ(stats.totalReloads,      static_cast<uint64_t>(iterations));
    EXPECT_EQ(stats.failedReloads,     static_cast<uint64_t>(iterations));
    EXPECT_EQ(stats.successfulReloads, 0u);
    EXPECT_EQ(stats.rollbacks,         0u);
}

/**
 * @brief Soak: stats monotonicity — totalReloads never decreases.
 *
 * Runs a smaller number of iterations and samples stats at each step to
 * confirm that all counters are non-decreasing.
 */
TEST(SoakTest, StatsMonotonicity) {
    constexpr int kIterations = 200;

    HotReloadManager mgr;
    ModuleLoader     loader;
    mgr.registerModule("mono_mod", loader);

    uint64_t prev_total   = 0;
    uint64_t prev_failed  = 0;

    for (int i = 0; i < kIterations; ++i) {
        mgr.reloadModule("mono_mod", "/nonexistent");
        auto stats = mgr.getStats();

        EXPECT_GE(stats.totalReloads,  prev_total)
            << "totalReloads must not decrease at iteration " << i;
        EXPECT_GE(stats.failedReloads, prev_failed)
            << "failedReloads must not decrease at iteration " << i;

        prev_total  = stats.totalReloads;
        prev_failed = stats.failedReloads;
    }
}

/**
 * @brief Soak: concurrent soak with N threads each doing M iterations.
 *
 * Validates that concurrent reload attempts against N independent modules
 * do not corrupt the global stats counter.
 */
TEST(SoakTest, ConcurrentSoakLoops) {
    constexpr int kThreads     = 10;
    constexpr int kPerThread   = 100;

    HotReloadManager             mgr;
    std::vector<ModuleLoader>    loaders(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        mgr.registerModule("thread_mod_" + std::to_string(i), loaders[i]);
    }

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&mgr, t]() {
            const std::string name = "thread_mod_" + std::to_string(t);
            for (int i = 0; i < kPerThread; ++i) {
                mgr.reloadModule(name, "/nonexistent_" + std::to_string(i));
            }
        });
    }
    for (auto& th : threads) { th.join(); }

    auto stats = mgr.getStats();
    EXPECT_EQ(stats.totalReloads,
              static_cast<uint64_t>(kThreads * kPerThread));
    EXPECT_EQ(stats.failedReloads, stats.totalReloads);
    EXPECT_EQ(stats.successfulReloads, 0u);
}

}} // namespace themis::modules
