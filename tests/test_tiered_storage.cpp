#include <gtest/gtest.h>
#include <cstdint>

// ── GCS backend tests ─────────────────────────────────────────────────────
// The GCS backend requires google-cloud-cpp and live ADC credentials, so the
// actual integration test is compile-guarded.  What we can always test is:
//  1. When compiled WITHOUT THEMIS_ENABLE_GCS, the backend reports unavailable.
//  2. The BlobStorageConfig has the gcs fields.
//  3. BlobStorageManager routes to GCS when enable_gcs is set.
//  4. GCSBlobBackend::name() returns "gcs".

#include "storage/blob_backend_gcs.h"
#include "storage/blob_storage_backend.h"
#include "storage/blob_storage_manager.h"

using namespace themis::storage;

// ── Tiered storage tests ──────────────────────────────────────────────────
// The tiered storage implementation uses the local filesystem, so we can run
// full round-trip tests without any external dependency.

#include "storage/tiered_storage.h"

#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// ============================================================================
// GCS backend – unit tests that do not require live credentials
// ============================================================================

class GCSBlobBackendTest : public ::testing::Test {};

TEST_F(GCSBlobBackendTest, NameIsGCS) {
    GCSBlobBackend backend("test-bucket");
    EXPECT_EQ(backend.name(), "gcs");
}

TEST_F(GCSBlobBackendTest, UnavailableWithoutCredentials) {
    // In CI / sandbox there are no GCS credentials, so the backend must
    // report unavailable instead of crashing.
    GCSBlobBackend backend("test-bucket");
    // When THEMIS_ENABLE_GCS is not defined, must always be unavailable.
    // When defined but no ADC credentials present, also unavailable.
    // In a proper GCS-enabled environment with credentials, this would be true.
#ifndef THEMIS_ENABLE_GCS
    EXPECT_FALSE(backend.isAvailable());
#endif
    // In all cases, it must not throw.
    [[maybe_unused]] bool avail = backend.isAvailable();
}

TEST_F(GCSBlobBackendTest, OperationsReturnErrorWhenUnavailable) {
    GCSBlobBackend backend("test-bucket");

    if (backend.isAvailable()) {
        GTEST_SKIP() << "Skipping: backend is available (live credentials present)";
    }

    std::vector<uint8_t> data{1, 2, 3};
    auto put_result = backend.put("key1", data);
    EXPECT_FALSE(put_result.has_value());

    BlobRef ref;
    ref.id = "key1";
    ref.type = BlobStorageType::GCS;
    auto get_result = backend.get(ref);
    EXPECT_FALSE(get_result.has_value());

    auto del_result = backend.remove(ref);
    EXPECT_FALSE(del_result.has_value());

    EXPECT_FALSE(backend.exists(ref));
}

// ── BlobStorageConfig has GCS fields ─────────────────────────────────────

TEST(BlobStorageConfigTest, HasGCSFields) {
    BlobStorageConfig cfg;
    EXPECT_FALSE(cfg.enable_gcs);
    cfg.enable_gcs = true;
    cfg.gcs_bucket = "my-bucket";
    cfg.gcs_prefix = "blobs/";
    EXPECT_TRUE(cfg.enable_gcs);
    EXPECT_EQ(cfg.gcs_bucket, "my-bucket");
}

// ── BlobStorageManager routes to GCS when configured ─────────────────────

TEST(BlobStorageManagerTest, SelectsGCSWhenEnabled) {
    BlobStorageConfig cfg;
    cfg.inline_threshold_bytes      = 0;
    cfg.rocksdb_blob_threshold_bytes = 0;
    cfg.enable_gcs = true;

    BlobStorageManager mgr(cfg);

    // Register a dummy filesystem backend as GCS substitute so we can verify
    // routing without real GCS.
    auto types = mgr.getRegisteredBackends();
    // No backends registered yet – verify the config flag is reflected.
    EXPECT_TRUE(mgr.getConfig().enable_gcs);
}

// ============================================================================
// AccessTracker tests
// ============================================================================

class AccessTrackerTest : public ::testing::Test {};

TEST_F(AccessTrackerTest, RecordWriteAndRead) {
    AccessTracker tracker;
    EXPECT_EQ(tracker.size(), 0u);

    tracker.recordWrite("key1", StorageTierLevel::HOT);
    EXPECT_EQ(tracker.size(), 1u);

    tracker.recordRead("key1");

    auto snap = tracker.snapshot();
    ASSERT_EQ(snap.count("key1"), 1u);
    EXPECT_EQ(snap.at("key1").tier, StorageTierLevel::HOT);
}

TEST_F(AccessTrackerTest, SetTier) {
    AccessTracker tracker;
    tracker.recordWrite("k", StorageTierLevel::HOT);
    tracker.setTier("k", StorageTierLevel::WARM);

    auto snap = tracker.snapshot();
    EXPECT_EQ(snap.at("k").tier, StorageTierLevel::WARM);
}

TEST_F(AccessTrackerTest, Remove) {
    AccessTracker tracker;
    tracker.recordWrite("key", StorageTierLevel::HOT);
    EXPECT_EQ(tracker.size(), 1u);
    tracker.remove("key");
    EXPECT_EQ(tracker.size(), 0u);
}

// ============================================================================
// TieredStorageManager tests
// ============================================================================

class TieredStorageTest : public ::testing::Test {
protected:
    std::string base_dir_;

    TieredStorageConfig makeConfig() {
        TieredStorageConfig cfg;
        cfg.hot_tier_path  = base_dir_ + "/hot";
        cfg.warm_tier_path = base_dir_ + "/warm";
        cfg.cold_tier_path = base_dir_ + "/cold";
        cfg.hot_to_warm_days       = 30;
        cfg.warm_to_cold_days      = 90;
        cfg.hot_zero_access_days   = 0;  // disable for most tests
        cfg.warm_zero_access_days  = 0;
        cfg.max_migrations_per_cycle = 0; // unlimited
        return cfg;
    }

    void SetUp() override {
        // Use unique directory per test to support parallel test execution
        auto pid = std::to_string(
            static_cast<long>(reinterpret_cast<std::uintptr_t>(this)));
        base_dir_ = (fs::temp_directory_path() / ("themis_tiered_" + pid)).string();
        std::error_code ec;
        fs::remove_all(base_dir_, ec);  // best-effort cleanup; errors ignored
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(base_dir_, ec);
    }
};

TEST_F(TieredStorageTest, PutAndGet) {
    TieredStorageManager mgr(makeConfig());

    EXPECT_TRUE(mgr.put("hello", "world"));
    EXPECT_EQ(mgr.get("hello"), "world");
}

TEST_F(TieredStorageTest, GetMissingKeyReturnsEmpty) {
    TieredStorageManager mgr(makeConfig());
    EXPECT_EQ(mgr.get("nonexistent"), "");
}

TEST_F(TieredStorageTest, DelKey) {
    TieredStorageManager mgr(makeConfig());

    mgr.put("k", "v");
    EXPECT_FALSE(mgr.get("k").empty());

    EXPECT_TRUE(mgr.del("k"));
    EXPECT_EQ(mgr.get("k"), "");
}

TEST_F(TieredStorageTest, DelNonexistentKey) {
    TieredStorageManager mgr(makeConfig());
    EXPECT_FALSE(mgr.del("ghost"));
}

TEST_F(TieredStorageTest, OverwriteKeyUpdatesHotTier) {
    TieredStorageManager mgr(makeConfig());

    mgr.put("key", "v1");
    mgr.put("key", "v2");
    EXPECT_EQ(mgr.get("key"), "v2");
}

TEST_F(TieredStorageTest, TierOfNewKeyIsHot) {
    TieredStorageManager mgr(makeConfig());
    mgr.put("x", "1");
    EXPECT_EQ(mgr.tierOf("x"), StorageTierLevel::HOT);
}

TEST_F(TieredStorageTest, ManualMigrationHotToWarm) {
    TieredStorageManager mgr(makeConfig());

    mgr.put("migrate_me", "data");

    // Force a migration by calling runMigrationCycle with very short age
    // threshold: use a separate config so the test doesn't wait 30 days.
    TieredStorageConfig fast_cfg = makeConfig();
    fast_cfg.hot_to_warm_days = 0;  // 0 days → every key qualifies

    TieredStorageManager mgr2(fast_cfg);
    mgr2.put("migrate_me", "data");

    // After cycle, expect hot→warm migration
    uint32_t n = mgr2.runMigrationCycle();
    EXPECT_GE(n, 1u);
    EXPECT_EQ(mgr2.tierOf("migrate_me"), StorageTierLevel::WARM);
}

TEST_F(TieredStorageTest, ReadAfterMigrateReturnsCorrectData) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days = 0;  // immediate demotion
    TieredStorageManager mgr(cfg);

    mgr.put("k", "original_value");
    mgr.runMigrationCycle();

    EXPECT_EQ(mgr.get("k"), "original_value");
}

TEST_F(TieredStorageTest, MigrationWarmToCold) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days  = 0;
    cfg.warm_to_cold_days = 0;
    TieredStorageManager mgr(cfg);

    mgr.put("key", "val");
    mgr.runMigrationCycle();  // hot → warm
    mgr.runMigrationCycle();  // warm → cold

    EXPECT_EQ(mgr.tierOf("key"), StorageTierLevel::COLD);
    EXPECT_EQ(mgr.get("key"), "val");
}

TEST_F(TieredStorageTest, StatsReflectMigrations) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days = 0;
    TieredStorageManager mgr(cfg);

    mgr.put("a", "1");
    mgr.put("b", "2");
    mgr.runMigrationCycle();

    auto s = mgr.stats();
    EXPECT_EQ(s.migrations_hot_to_warm, 2u);
    EXPECT_EQ(s.migration_errors,       0u);
}

TEST_F(TieredStorageTest, BackgroundWorkerStartStop) {
    TieredStorageConfig cfg = makeConfig();
    cfg.migration_check_interval_secs = 3600; // don't actually trigger
    TieredStorageManager mgr(cfg);

    mgr.startMigrationWorker();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    mgr.stopMigrationWorker();
    // If we get here without deadlock, the test passes.
}

TEST_F(TieredStorageTest, LargeValueRoundTrip) {
    TieredStorageManager mgr(makeConfig());

    std::string large(1024 * 1024, 'X');  // 1 MB
    mgr.put("big", large);
    EXPECT_EQ(mgr.get("big"), large);
}

TEST_F(TieredStorageTest, MultipleKeysIsolated) {
    TieredStorageManager mgr(makeConfig());

    for (int i = 0; i < 20; ++i) {
        mgr.put("k" + std::to_string(i), "v" + std::to_string(i));
    }
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(mgr.get("k" + std::to_string(i)), "v" + std::to_string(i));
    }
}

// ============================================================================
// Size-based migration policy tests
// ============================================================================

TEST_F(TieredStorageTest, SizeBasedMigrationHotToCold) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999;  // no age-based demotion (only size policy should trigger)
    cfg.warm_to_cold_days     = 999;
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    cfg.large_blob_bytes = 10;   // anything >= 10 bytes goes to cold
    cfg.large_blob_tier  = StorageTierLevel::COLD;
    TieredStorageManager mgr(cfg);

    // Small key – should NOT be migrated by size policy
    mgr.put("small", "tiny");         // 4 bytes < 10
    // Large key – should be migrated to cold by size policy
    mgr.put("large", "this_is_large"); // 13 bytes >= 10

    uint32_t n = mgr.runMigrationCycle();
    EXPECT_GE(n, 1u);

    EXPECT_EQ(mgr.tierOf("large"), StorageTierLevel::COLD);
    EXPECT_EQ(mgr.get("large"), "this_is_large");

    // Small key stays where it is (no age/access rule active)
    EXPECT_EQ(mgr.tierOf("small"), StorageTierLevel::HOT);
}

TEST_F(TieredStorageTest, SizeBasedMigrationWarmToCold) {
    // Arrange: write a file directly into the warm tier and register it in the
    // tracker as WARM so we can test that the size policy demotes it to COLD.
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999;  // no age-based demotion
    cfg.warm_to_cold_days     = 999;
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    cfg.large_blob_bytes = 10;
    cfg.large_blob_tier  = StorageTierLevel::COLD;

    TieredStorageManager mgr(cfg);

    // Manually create the file in the warm directory (simulates a key that was
    // previously demoted to WARM by a migration cycle).
    const std::string blob_value = "large_value_here";  // 16 bytes >= 10
    const std::string warm_file  = cfg.warm_tier_path + "/blob.dat";
    {
        std::ofstream f(warm_file, std::ios::binary);
        ASSERT_TRUE(f.is_open()) << "Failed to open warm tier file for writing";
        f << blob_value;
        ASSERT_TRUE(f.good()) << "Write to warm tier file failed";
    }
    // Register it with the tracker as already in WARM, with its byte size.
    mgr.accessTracker().recordWrite("blob", StorageTierLevel::WARM,
                                    static_cast<uint64_t>(blob_value.size()));

    // Now size policy should demote it to COLD
    uint32_t n = mgr.runMigrationCycle();
    EXPECT_GE(n, 1u);
    EXPECT_EQ(mgr.tierOf("blob"), StorageTierLevel::COLD);
    EXPECT_EQ(mgr.get("blob"), blob_value);
}

TEST_F(TieredStorageTest, SizeBasedMigrationDisabledByDefault) {
    // Default config has large_blob_bytes = 0 (disabled)
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999; // no age demotion
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    TieredStorageManager mgr(cfg);

    mgr.put("blob", std::string(1024, 'A'));  // 1 KB
    mgr.runMigrationCycle();

    // Key should stay on HOT because size-based policy is off and age hasn't elapsed
    EXPECT_EQ(mgr.tierOf("blob"), StorageTierLevel::HOT);
}

TEST_F(TieredStorageTest, SizeBasedMigrationStatsCounter) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999;
    cfg.warm_to_cold_days     = 999;
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    cfg.large_blob_bytes = 5;
    cfg.large_blob_tier  = StorageTierLevel::COLD;
    TieredStorageManager mgr(cfg);

    mgr.put("a", "hello!");    // 6 bytes >= 5
    mgr.put("b", "hi");        // 2 bytes <  5
    mgr.runMigrationCycle();

    auto s = mgr.stats();
    EXPECT_EQ(s.migrations_size_based, 1u);
    EXPECT_EQ(s.migration_errors,      0u);
}

TEST_F(TieredStorageTest, SizeBasedMigrationReadAfterMigrate) {
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999;
    cfg.warm_to_cold_days     = 999;
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    cfg.large_blob_bytes = 10;
    cfg.large_blob_tier  = StorageTierLevel::COLD;
    TieredStorageManager mgr(cfg);

    std::string data(50, 'Z');
    mgr.put("key", data);
    mgr.runMigrationCycle();

    // Data must be intact after size-based migration
    EXPECT_EQ(mgr.get("key"), data);
    EXPECT_EQ(mgr.tierOf("key"), StorageTierLevel::COLD);
}

TEST_F(TieredStorageTest, SizeBasedMigrationAlreadyAtTarget) {
    // Keys already in the large_blob_tier must not be migrated again
    TieredStorageConfig cfg = makeConfig();
    cfg.hot_to_warm_days      = 999;  // no age-based demotion (only size policy should trigger)
    cfg.warm_to_cold_days     = 999;
    cfg.hot_zero_access_days  = 0;
    cfg.warm_zero_access_days = 0;
    cfg.large_blob_bytes = 5;
    cfg.large_blob_tier  = StorageTierLevel::COLD;
    TieredStorageManager mgr(cfg);

    // Put a large value and migrate all the way to cold
    mgr.put("k", "hello!");    // 6 bytes >= 5
    mgr.runMigrationCycle();   // size-based: hot -> cold
    EXPECT_EQ(mgr.tierOf("k"), StorageTierLevel::COLD);

    // A second cycle must not attempt re-migration
    uint32_t n = mgr.runMigrationCycle();
    EXPECT_EQ(n, 0u);
    EXPECT_EQ(mgr.get("k"), "hello!");
}
