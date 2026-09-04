// Unit tests for Cold/Warm Tier Index Migration (Issue #2407)
// Index module — Phase 3
//
// Covers:
//   - Index registration (HOT tier, explicit tier)
//   - Access recording and metadata retrieval
//   - Manual demotion: HOT → WARM, WARM → COLD
//   - Manual promotion: COLD → WARM, WARM → HOT
//   - Already-in-target-tier is a no-op success
//   - Export callback is invoked on demotion
//   - Import callback is invoked on promotion
//   - Automatic migration pass (age-based demotion)
//   - Automatic migration pass (access-based promotion)
//   - Export/import failure propagation
//   - listIndexesByTier and listIndexes helpers
//   - Tier path helpers (warmPath / coldPath)
//   - Edge cases: empty name, unknown index, unregister

#include "index/tiered_index_manager.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::index;
using Tier = IndexTierMeta::Tier;

// ---------------------------------------------------------------------------
// Helper: build a manager with no-op callbacks (default behaviour)
// ---------------------------------------------------------------------------

static TieredIndexManager makeManager() {
    return TieredIndexManager("/tmp/themis_test_warm", "/tmp/themis_test_cold");
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, RegisterDefaultIsHot) {
    auto mgr = makeManager();
    ASSERT_TRUE(mgr.registerIndex("idx_a", "/data/idx_a"));
    EXPECT_TRUE(mgr.hasIndex("idx_a"));
    auto meta = mgr.getMetadata("idx_a");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->tier, Tier::HOT);
    EXPECT_EQ(meta->data_path, "/data/idx_a");
    EXPECT_EQ(meta->access_count, 0u);
}

TEST(TieredIndexManager, RegisterExplicitWarmTier) {
    auto mgr = makeManager();
    ASSERT_TRUE(mgr.registerIndex("idx_b", Tier::WARM, "/warm/idx_b"));
    auto meta = mgr.getMetadata("idx_b");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->tier, Tier::WARM);
}

TEST(TieredIndexManager, RegisterDuplicateReturnsFalse) {
    auto mgr = makeManager();
    ASSERT_TRUE(mgr.registerIndex("idx_dup", "/data/idx_dup"));
    EXPECT_FALSE(mgr.registerIndex("idx_dup", "/data/idx_dup"));
}

TEST(TieredIndexManager, RegisterEmptyNameReturnsFalse) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.registerIndex("", "/data/empty"));
}

TEST(TieredIndexManager, UnregisterRemovesIndex) {
    auto mgr = makeManager();
    ASSERT_TRUE(mgr.registerIndex("idx_rm", "/data/idx_rm"));
    EXPECT_TRUE(mgr.unregisterIndex("idx_rm"));
    EXPECT_FALSE(mgr.hasIndex("idx_rm"));
}

TEST(TieredIndexManager, UnregisterUnknownReturnsFalse) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.unregisterIndex("does_not_exist"));
}

// ---------------------------------------------------------------------------
// Access tracking
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, RecordAccessIncreasesCounter) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_acc", "/data/idx_acc");
    EXPECT_TRUE(mgr.recordAccess("idx_acc"));
    EXPECT_TRUE(mgr.recordAccess("idx_acc"));
    auto meta = mgr.getMetadata("idx_acc");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->access_count, 2u);
}

TEST(TieredIndexManager, RecordAccessUnknownReturnsFalse) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.recordAccess("no_such_index"));
}

TEST(TieredIndexManager, ResetAccessCount) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_rst", "/data/idx_rst");
    mgr.recordAccess("idx_rst");
    mgr.recordAccess("idx_rst");
    EXPECT_TRUE(mgr.resetAccessCount("idx_rst"));
    EXPECT_EQ(mgr.getMetadata("idx_rst")->access_count, 0u);
}

// ---------------------------------------------------------------------------
// Manual migration – demotion
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, DemoteHotToWarm) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_hw", "/data/idx_hw");
    mgr.recordAccess("idx_hw");

    auto res = mgr.demoteToWarm("idx_hw");
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.code, MigrationDiagnosticCode::NONE);
    EXPECT_EQ(res.from_tier, Tier::HOT);
    EXPECT_EQ(res.to_tier,   Tier::WARM);
    EXPECT_EQ(res.source_path, "/data/idx_hw");
    EXPECT_EQ(res.target_path, mgr.warmPath("idx_hw"));

    auto meta = mgr.getMetadata("idx_hw");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->tier, Tier::WARM);
    EXPECT_EQ(meta->access_count, 0u);
}

TEST(TieredIndexManager, DemoteWarmToCold) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_wc", Tier::WARM, "/warm/idx_wc");

    auto res = mgr.demoteToCold("idx_wc");
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.to_tier, Tier::COLD);

    EXPECT_EQ(mgr.getMetadata("idx_wc")->tier, Tier::COLD);
}

TEST(TieredIndexManager, DemoteHotToColdInTwoSteps) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_hc", "/data/idx_hc");

    EXPECT_TRUE(mgr.demoteToWarm("idx_hc").ok);
    EXPECT_TRUE(mgr.demoteToCold("idx_hc").ok);
    EXPECT_EQ(mgr.getMetadata("idx_hc")->tier, Tier::COLD);
}

// ---------------------------------------------------------------------------
// Manual migration – promotion
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, PromoteColdToWarm) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_cw", Tier::COLD, "/cold/idx_cw");

    auto res = mgr.promoteToHot("idx_cw");
    // Direct cold→hot promotion is allowed by migrateTo.
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(mgr.getMetadata("idx_cw")->tier, Tier::HOT);
}

TEST(TieredIndexManager, PromoteWarmToHot) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_wh", Tier::WARM, "/warm/idx_wh");
    mgr.recordAccess("idx_wh");
    mgr.recordAccess("idx_wh");
    const auto before = mgr.getMetadata("idx_wh")->last_access;

    auto res = mgr.promoteToHot("idx_wh");
    EXPECT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.code, MigrationDiagnosticCode::NONE);
    EXPECT_EQ(res.to_tier, Tier::HOT);
    EXPECT_EQ(res.source_path, "/warm/idx_wh");
    EXPECT_EQ(res.target_path, "/warm/idx_wh");

    auto meta = mgr.getMetadata("idx_wh");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->tier, Tier::HOT);
    EXPECT_EQ(meta->access_count, 0u);
    EXPECT_GE(meta->last_access, before);
}

// ---------------------------------------------------------------------------
// Already-in-target-tier
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, MigrateToSameTierIsNoOp) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_noop", "/data/idx_noop");  // HOT

    auto res = mgr.migrateTo("idx_noop", Tier::HOT);
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.from_tier, Tier::HOT);
    EXPECT_EQ(res.to_tier,   Tier::HOT);
}

// ---------------------------------------------------------------------------
// Migration on unknown index
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, MigrateUnknownFails) {
    auto mgr = makeManager();
    auto res = mgr.migrateTo("ghost", Tier::WARM);
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(res.code, MigrationDiagnosticCode::INDEX_NOT_FOUND);
    EXPECT_FALSE(res.message.empty());
    EXPECT_EQ(res.from_tier, Tier::WARM);
    EXPECT_EQ(res.to_tier, Tier::WARM);
}

// ---------------------------------------------------------------------------
// Export / Import callbacks
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, ExportCallbackCalledOnDemotion) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_exp", "/data/idx_exp");

    std::string exported_name, exported_path;
    mgr.setExportFn([&](const std::string& n, const std::string& p) {
        exported_name = n;
        exported_path = p;
        return true;
    });

    auto res = mgr.demoteToWarm("idx_exp");
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(exported_name, "idx_exp");
    EXPECT_FALSE(exported_path.empty());
}

TEST(TieredIndexManager, ImportCallbackCalledOnPromotion) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_imp", Tier::WARM, "/warm/idx_imp");

    std::string imported_name = {};
    mgr.setImportFn([&](const std::string& n, const std::string&) {
        imported_name = n;
        return true;
    });

    auto res = mgr.promoteToHot("idx_imp");
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(imported_name, "idx_imp");
}

TEST(TieredIndexManager, ExportFailurePropagated) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_fail", "/data/idx_fail");
    mgr.setExportFn([](const std::string&, const std::string&) { return false; });

    auto res = mgr.demoteToWarm("idx_fail");
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(res.code, MigrationDiagnosticCode::EXPORT_FAILED);
    EXPECT_FALSE(res.message.empty());
    EXPECT_EQ(res.source_path, "/data/idx_fail");
    EXPECT_EQ(res.target_path, mgr.warmPath("idx_fail"));
    // Tier must NOT have changed on failure.
    EXPECT_EQ(mgr.getMetadata("idx_fail")->tier, Tier::HOT);
}

TEST(TieredIndexManager, ExportExceptionPropagatedAsDiagnostic) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_ex", "/data/idx_ex");
    mgr.setExportFn([](const std::string&, const std::string&) -> bool {
        throw std::runtime_error("simulated export exception");
    });

    EXPECT_NO_THROW({
        auto res = mgr.demoteToWarm("idx_ex");
        EXPECT_FALSE(res.ok);
        EXPECT_EQ(res.code, MigrationDiagnosticCode::EXPORT_FAILED);
        EXPECT_NE(res.message.find("simulated export exception"), std::string::npos);
        EXPECT_EQ(res.source_path, "/data/idx_ex");
        EXPECT_EQ(res.target_path, mgr.warmPath("idx_ex"));
        EXPECT_EQ(mgr.getMetadata("idx_ex")->tier, Tier::HOT);
    });
}

TEST(TieredIndexManager, ImportFailurePropagated) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_ifail", Tier::WARM, "/warm/idx_ifail");
    mgr.setImportFn([](const std::string&, const std::string&) { return false; });

    auto res = mgr.promoteToHot("idx_ifail");
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(res.code, MigrationDiagnosticCode::IMPORT_FAILED);
    EXPECT_EQ(res.source_path, "/warm/idx_ifail");
    EXPECT_EQ(res.target_path, "/warm/idx_ifail");
    // Tier must NOT have changed on failure.
    EXPECT_EQ(mgr.getMetadata("idx_ifail")->tier, Tier::WARM);
}

TEST(TieredIndexManager, ImportExceptionPropagatedAsDiagnostic) {
    auto mgr = makeManager();
    mgr.registerIndex("idx_iex", Tier::WARM, "/warm/idx_iex");
    mgr.setImportFn([](const std::string&, const std::string&) -> bool {
        throw std::runtime_error("simulated import exception");
    });

    EXPECT_NO_THROW({
        auto res = mgr.promoteToHot("idx_iex");
        EXPECT_FALSE(res.ok);
        EXPECT_EQ(res.code, MigrationDiagnosticCode::IMPORT_FAILED);
        EXPECT_NE(res.message.find("simulated import exception"), std::string::npos);
        EXPECT_EQ(res.source_path, "/warm/idx_iex");
        EXPECT_EQ(res.target_path, "/warm/idx_iex");
        EXPECT_EQ(mgr.getMetadata("idx_iex")->tier, Tier::WARM);
    });
}

// ---------------------------------------------------------------------------
// Automatic migration pass — age-based demotion
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, AutoPassDemotesIdleHotToWarm) {
    auto mgr = makeManager();

    // Use a very short idle threshold (0 seconds) so the index is immediately stale.
    TierMigrationPolicy pol;
    pol.hot_to_warm_idle_secs  = 0;
    pol.warm_to_cold_idle_secs = 9999;
    pol.promotion_window_secs  = 0;   // disable promotions
    mgr.setPolicy(pol);

    mgr.registerIndex("idx_auto_hw", "/data/idx_auto_hw");

    // Sleep briefly to ensure > 0 s of idle time.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto results = mgr.runMigrationPass();
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(results[0].ok);
    EXPECT_EQ(results[0].to_tier, Tier::WARM);
    EXPECT_EQ(mgr.getMetadata("idx_auto_hw")->tier, Tier::WARM);
}

TEST(TieredIndexManager, AutoPassDemotesIdleWarmToCold) {
    auto mgr = makeManager();

    TierMigrationPolicy pol;
    pol.hot_to_warm_idle_secs  = 9999;
    pol.warm_to_cold_idle_secs = 0;
    pol.promotion_window_secs  = 0;
    mgr.setPolicy(pol);

    mgr.registerIndex("idx_auto_wc", Tier::WARM, "/warm/idx_auto_wc");

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto results = mgr.runMigrationPass();
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(results[0].ok);
    EXPECT_EQ(results[0].to_tier, Tier::COLD);
}

// ---------------------------------------------------------------------------
// Automatic migration pass — access-based promotion
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, AutoPassPromotesWarmToHotOnHighAccess) {
    auto mgr = makeManager();

    TierMigrationPolicy pol;
    pol.hot_to_warm_idle_secs    = 9999;  // no demotion
    pol.warm_to_cold_idle_secs   = 9999;
    pol.warm_to_hot_access_threshold = 3;
    pol.promotion_window_secs    = 9999;  // always in window
    mgr.setPolicy(pol);

    mgr.registerIndex("idx_promo", Tier::WARM, "/warm/idx_promo");
    mgr.recordAccess("idx_promo");
    mgr.recordAccess("idx_promo");
    mgr.recordAccess("idx_promo");  // 3 accesses, threshold met

    auto results = mgr.runMigrationPass();
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(results[0].ok);
    EXPECT_EQ(results[0].to_tier, Tier::HOT);
    EXPECT_EQ(mgr.getMetadata("idx_promo")->tier, Tier::HOT);
}

TEST(TieredIndexManager, AutoPassPromotesColdToWarmOnAccess) {
    auto mgr = makeManager();

    TierMigrationPolicy pol;
    pol.hot_to_warm_idle_secs    = 9999;
    pol.warm_to_cold_idle_secs   = 9999;
    pol.cold_to_warm_access_threshold = 2;
    pol.promotion_window_secs    = 9999;
    mgr.setPolicy(pol);

    mgr.registerIndex("idx_cold_up", Tier::COLD, "/cold/idx_cold_up");
    mgr.recordAccess("idx_cold_up");
    mgr.recordAccess("idx_cold_up");

    auto results = mgr.runMigrationPass();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].to_tier, Tier::WARM);
}

// ---------------------------------------------------------------------------
// listIndexes helpers
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, ListIndexes) {
    auto mgr = makeManager();
    mgr.registerIndex("a", "/data/a");
    mgr.registerIndex("b", Tier::WARM, "/warm/b");
    mgr.registerIndex("c", Tier::COLD, "/cold/c");

    auto all = mgr.listIndexes();
    EXPECT_EQ(all.size(), 3u);

    auto hot  = mgr.listIndexesByTier(Tier::HOT);
    auto warm = mgr.listIndexesByTier(Tier::WARM);
    auto cold = mgr.listIndexesByTier(Tier::COLD);
    EXPECT_EQ(hot.size(),  1u);
    EXPECT_EQ(warm.size(), 1u);
    EXPECT_EQ(cold.size(), 1u);
}

// ---------------------------------------------------------------------------
// Tier path helpers
// ---------------------------------------------------------------------------

TEST(TieredIndexManager, WarmAndColdPathsContainIndexName) {
    TieredIndexManager mgr("/var/warm", "/var/cold");
    EXPECT_EQ(mgr.warmPath("my_index"), "/var/warm/my_index");
    EXPECT_EQ(mgr.coldPath("my_index"), "/var/cold/my_index");
}
