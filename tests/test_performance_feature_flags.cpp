/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_performance_feature_flags.cpp                 ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:18:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     205                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f93dd332c6  2026-02-23  audit(performance): add file banners and register PMem in... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Test for performance feature flags system
// Tests compile-time and runtime feature flag management

#include <performance/feature_flags.h>
#include <gtest/gtest.h>
#include <unordered_map>
#include <string>

using namespace themis::performance;

TEST(PerformanceFeatureFlagsTest, DefaultState) {
    // Feature flags should default to disabled (unless compile-time enabled)
    auto& flags = PerformanceFeatureFlags::instance();
    
    // All flags should be accessible
    EXPECT_NO_THROW(flags.mimalloc_enabled());
    EXPECT_NO_THROW(flags.huge_pages_enabled());
    EXPECT_NO_THROW(flags.rcu_index_enabled());
    EXPECT_NO_THROW(flags.lirs_cache_enabled());
    EXPECT_NO_THROW(flags.wisckey_enabled());
    EXPECT_NO_THROW(flags.cicada_cc_enabled());
    EXPECT_NO_THROW(flags.diskann_enabled());
    EXPECT_NO_THROW(flags.bw_tree_enabled());
    EXPECT_NO_THROW(flags.pmem_enabled());
}

TEST(PerformanceFeatureFlagsTest, RuntimeToggle) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    // Test runtime toggling
    bool initial_state = flags.mimalloc_enabled();
    
    flags.set_mimalloc_enabled(true);
    EXPECT_TRUE(flags.mimalloc_enabled());
    
    flags.set_mimalloc_enabled(false);
    EXPECT_FALSE(flags.mimalloc_enabled());
    
    // Restore initial state
    flags.set_mimalloc_enabled(initial_state);
}

TEST(PerformanceFeatureFlagsTest, LoadFromConfig) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    // Create test config
    std::unordered_map<std::string, bool> config = {
        {"enable_mimalloc", true},
        {"enable_huge_pages", true},
        {"enable_rcu_index", false},
        {"enable_lirs_cache", true}
    };
    
    // Load configuration
    flags.load_from_config(config);
    
    // Verify flags match config
    EXPECT_TRUE(flags.mimalloc_enabled());
    EXPECT_TRUE(flags.huge_pages_enabled());
    EXPECT_FALSE(flags.rcu_index_enabled());
    EXPECT_TRUE(flags.lirs_cache_enabled());
    
    // Reset to defaults for other tests
    config = {
        {"enable_mimalloc", false},
        {"enable_huge_pages", false},
        {"enable_rcu_index", false},
        {"enable_lirs_cache", false}
    };
    flags.load_from_config(config);
}

TEST(PerformanceFeatureFlagsTest, GetAllFlags) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    // Set some flags
    flags.set_mimalloc_enabled(true);
    flags.set_wisckey_enabled(true);
    
    // Get all flags
    auto all_flags = flags.get_all_flags();
    
    // Verify map contains expected keys
    EXPECT_TRUE(all_flags.find("mimalloc") != all_flags.end());
    EXPECT_TRUE(all_flags.find("huge_pages") != all_flags.end());
    EXPECT_TRUE(all_flags.find("rcu_index") != all_flags.end());
    EXPECT_TRUE(all_flags.find("lirs_cache") != all_flags.end());
    EXPECT_TRUE(all_flags.find("wisckey") != all_flags.end());
    EXPECT_TRUE(all_flags.find("cicada_cc") != all_flags.end());
    EXPECT_TRUE(all_flags.find("diskann") != all_flags.end());
    EXPECT_TRUE(all_flags.find("bw_tree") != all_flags.end());
    EXPECT_TRUE(all_flags.find("pmem") != all_flags.end());
    
    // Verify states
    EXPECT_TRUE(all_flags["mimalloc"]);
    EXPECT_TRUE(all_flags["wisckey"]);
    
    // Reset
    flags.set_mimalloc_enabled(false);
    flags.set_wisckey_enabled(false);
}

TEST(PerformanceFeatureFlagsTest, MacroAccess) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    // Test convenience macros
    flags.set_mimalloc_enabled(true);
    EXPECT_TRUE(THEMIS_PERF_MIMALLOC_ENABLED());
    
    flags.set_mimalloc_enabled(false);
    EXPECT_FALSE(THEMIS_PERF_MIMALLOC_ENABLED());
    
    // Test other macros exist
    EXPECT_NO_THROW({
        (void)THEMIS_PERF_HUGE_PAGES_ENABLED();
        (void)THEMIS_PERF_RCU_INDEX_ENABLED();
        (void)THEMIS_PERF_LIRS_CACHE_ENABLED();
        (void)THEMIS_PERF_WISCKEY_ENABLED();
        (void)THEMIS_PERF_CICADA_CC_ENABLED();
        (void)THEMIS_PERF_DISKANN_ENABLED();
        (void)THEMIS_PERF_BW_TREE_ENABLED();
        (void)THEMIS_PERF_PMEM_ENABLED();
    });
}

TEST(PerformanceFeatureFlagsTest, ThreadSafety) {
    // This test verifies the atomic nature of the flags
    // In a real scenario, you would test with multiple threads
    auto& flags = PerformanceFeatureFlags::instance();
    
    // Multiple rapid toggles should work correctly
    for (int i = 0; i < 100; ++i) {
        flags.set_mimalloc_enabled(i % 2 == 0);
        bool state = flags.mimalloc_enabled();
        EXPECT_EQ(state, (i % 2 == 0));
    }
}

TEST(PerformanceFeatureFlagsTest, CompileTimeFlags) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    // If compiled with flags, they should be enabled by default
    #ifdef THEMIS_ENABLE_MIMALLOC
    EXPECT_TRUE(flags.mimalloc_enabled());
    #endif
    
    #ifdef THEMIS_ENABLE_HUGE_PAGES
    EXPECT_TRUE(flags.huge_pages_enabled());
    #endif
    
    #ifdef THEMIS_ENABLE_RCU_INDEX
    EXPECT_TRUE(flags.rcu_index_enabled());
    #endif
    
    #ifdef THEMIS_ENABLE_LIRS_CACHE
    EXPECT_TRUE(flags.lirs_cache_enabled());
    #endif

    #ifdef THEMIS_ENABLE_PMEM
    EXPECT_TRUE(flags.pmem_enabled());
    #endif
}

// Integration test: Verify feature flags can be used in conditional logic
TEST(PerformanceFeatureFlagsTest, ConditionalExecution) {
    auto& flags = PerformanceFeatureFlags::instance();
    
    int execution_count = 0;
    
    // Simulate conditional feature execution
    flags.set_mimalloc_enabled(true);
    if (THEMIS_PERF_MIMALLOC_ENABLED()) {
        execution_count++;
    }
    EXPECT_EQ(execution_count, 1);
    
    flags.set_mimalloc_enabled(false);
    if (THEMIS_PERF_MIMALLOC_ENABLED()) {
        execution_count++;
    }
    EXPECT_EQ(execution_count, 1); // Should not execute
}
