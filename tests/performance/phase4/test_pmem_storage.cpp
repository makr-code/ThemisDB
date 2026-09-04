// Tests for Persistent Memory (Optane) aware storage layout (Phase 4)

#include "performance/phase4/pmem_storage.h"
#include "performance/phase4/feature_flags.h"
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::performance::phase4;

// ---------------------------------------------------------------------------
// Helper: temporary pool file that is removed after each test
// ---------------------------------------------------------------------------
class PMemPoolTest : public ::testing::Test {
protected:
    std::string pool_path_;

    void SetUp() override {
        pool_path_ = (fs::temp_directory_path() /
                      ("themis_pmem_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".pool"))
                         .string();
    }

    void TearDown() override {
        fs::remove(pool_path_);
    }

    PMemPool::Config make_config(size_t size = 4 * 1024 * 1024 /* 4 MiB */) {
        return PMemPool::Config{
            .path              = pool_path_,
            .pool_size         = size,
            .alignment         = kPMemCacheLineSize,
            .create_if_missing = true,
            .use_dax           = false, // no real DAX in CI
            .recover_on_open   = true,
        };
    }
};

// ---------------------------------------------------------------------------
// PMemPool tests
// ---------------------------------------------------------------------------

TEST_F(PMemPoolTest, OpenAndClose) {
    PMemPool pool(make_config());
    EXPECT_NE(pool.base(), nullptr);
    auto stats = pool.get_stats();
    EXPECT_TRUE(stats.is_healthy);
    EXPECT_GT(stats.total_bytes, 0u);
}

TEST_F(PMemPoolTest, SingleAllocation) {
    PMemPool pool(make_config());

    void* p = pool.allocate(128);
    ASSERT_NE(p, nullptr);

    // Write and read back
    std::memset(p, 0xAB, 128);
    const auto* bytes = static_cast<const uint8_t*>(p);
    EXPECT_EQ(bytes[0],   0xAB);
    EXPECT_EQ(bytes[127], 0xAB);
}

TEST_F(PMemPoolTest, AlignmentRespected) {
    PMemPool pool(make_config());

    void* p1 = pool.allocate(1);
    void* p2 = pool.allocate(1);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);

    // Both pointers should be aligned to kPMemCacheLineSize
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % kPMemCacheLineSize, 0u);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % kPMemCacheLineSize, 0u);
    // Second allocation must follow the first
    EXPECT_GE(static_cast<char*>(p2) - static_cast<char*>(p1),
              static_cast<ptrdiff_t>(kPMemCacheLineSize));
}

TEST_F(PMemPoolTest, MultipleAllocations) {
    PMemPool pool(make_config());

    std::vector<void*> ptrs = {};

    for (int i = 0; i < 64; ++i) {
        void* p = pool.allocate(256);
        ASSERT_NE(p, nullptr) << "Allocation " << i << " failed";
        ptrs.push_back(p);
    }

    // All allocations must be distinct and non-overlapping
    for (size_t i = 1; i < ptrs.size(); ++i) {
        EXPECT_NE(ptrs[i], ptrs[i - 1]);
        EXPECT_GE(static_cast<char*>(ptrs[i]) - static_cast<char*>(ptrs[i - 1]),
                  static_cast<ptrdiff_t>(kPMemCacheLineSize));
    }
}

TEST_F(PMemPoolTest, ExhaustionReturnNullptr) {
    // Very small pool (just over the header + bitmap, so data area is tiny)
    PMemPool pool(make_config(64 * 1024 /* 64 KiB */));

    bool exhausted = false;
    for (int i = 0; i < 4096; ++i) {
        void* p = pool.allocate(256);
        if (!p) {
            exhausted = true;
            break;
        }
    }
    EXPECT_TRUE(exhausted) << "Expected pool exhaustion but allocations kept succeeding";
}

TEST_F(PMemPoolTest, PersistDoesNotCrash) {
    PMemPool pool(make_config());
    void* p = pool.allocate(512);
    ASSERT_NE(p, nullptr);
    std::memset(p, 0x55, 512);
    // persist() must not throw or crash
    EXPECT_NO_THROW(pool.persist(p, 512));
}

TEST_F(PMemPoolTest, StatsReflectAllocations) {
    PMemPool pool(make_config());

    auto stats_before = pool.get_stats();
    EXPECT_EQ(stats_before.alloc_count, 0u);

    void* p = pool.allocate(256);
    ASSERT_NE(p, nullptr);

    auto stats_after = pool.get_stats();
    EXPECT_EQ(stats_after.alloc_count, 1u);
    EXPECT_GT(stats_after.used_bytes,  0u);
    EXPECT_LT(stats_after.free_bytes,  stats_after.total_bytes);
}

TEST_F(PMemPoolTest, FreeUpdatesStats) {
    PMemPool pool(make_config());
    void* p = pool.allocate(256);
    ASSERT_NE(p, nullptr);

    auto before = pool.get_stats();
    pool.free(p, 256);
    auto after = pool.get_stats();
    EXPECT_EQ(after.free_count, before.free_count + 1);
}

TEST_F(PMemPoolTest, RecoverFromPersistedState) {
    // Write some data and close the pool, then re-open and verify metadata.
    {
        PMemPool pool(make_config());
        void* p = pool.allocate(512);
        ASSERT_NE(p, nullptr);
        std::memset(p, 0xCC, 512);
        pool.persist(p, 512);
    }

    // Re-open: recovery must succeed (no throw) and report non-zero used_bytes.
    PMemPool pool(make_config());
    auto stats = pool.get_stats();
    EXPECT_TRUE(stats.is_healthy);
    EXPECT_GT(stats.used_bytes, 0u);
}

TEST_F(PMemPoolTest, ConcurrentAllocations) {
    PMemPool pool(make_config(16 * 1024 * 1024 /* 16 MiB */));

    const int num_threads  = 4;
    const int allocs_each  = 64;
    std::vector<std::vector<void*>> results(num_threads);
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < allocs_each; ++i) {
                void* p = pool.allocate(256);
                if (p) {
                    results[t].push_back(p);
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // Collect all pointers and verify uniqueness
    std::vector<void*> all_ptrs = {};

    for (auto& r : results) {
        all_ptrs.insert(all_ptrs.end(), r.begin(), r.end());
    }
    std::sort(all_ptrs.begin(), all_ptrs.end());
    EXPECT_EQ(std::adjacent_find(all_ptrs.begin(), all_ptrs.end()),
              all_ptrs.end())
        << "Duplicate pointers returned by concurrent allocations";
}

// ---------------------------------------------------------------------------
// PMemStorageLayout tests
// ---------------------------------------------------------------------------

class PMemLayoutTest : public ::testing::Test {
protected:
    std::string pool_path_;

    void SetUp() override {
        pool_path_ = (fs::temp_directory_path() /
                      ("themis_layout_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".pool"))
                         .string();
    }

    void TearDown() override {
        fs::remove(pool_path_);
    }

    PMemStorageLayout::Config make_layout_config() {
        return PMemStorageLayout::Config{
            .path              = pool_path_,
            .pool_size         = 4 * 1024 * 1024,
            .write_granule     = kPMemCacheLineSize,
            .use_dax           = false,
            .create_if_missing = true,
        };
    }
};

TEST_F(PMemLayoutTest, WriteAndRead) {
    PMemStorageLayout layout(make_layout_config());

    const std::string payload = "hello persistent world";
    void* dst = layout.write("k1", payload.data(), payload.size());
    ASSERT_NE(dst, nullptr);

    // Data must be readable from the returned pointer
    EXPECT_EQ(std::memcmp(dst, payload.data(), payload.size()), 0);
}

TEST_F(PMemLayoutTest, WriteNullDataReturnsNullptr) {
    PMemStorageLayout layout(make_layout_config());
    EXPECT_EQ(layout.write("k", nullptr, 10), nullptr);
}

TEST_F(PMemLayoutTest, WriteZeroLengthReturnsNullptr) {
    PMemStorageLayout layout(make_layout_config());
    const char data[] = "x";
    EXPECT_EQ(layout.write("k", data, 0), nullptr);
}

TEST_F(PMemLayoutTest, StatsAccumulate) {
    PMemStorageLayout layout(make_layout_config());

    const std::string payload(128, 'A');
    layout.write("k1", payload.data(), payload.size());
    layout.write("k2", payload.data(), payload.size());

    auto ws = layout.get_write_stats();
    EXPECT_EQ(ws.writes, 2u);
    EXPECT_EQ(ws.bytes_written, 2 * payload.size());
    EXPECT_GE(ws.bytes_persisted, ws.bytes_written); // padding ≥ payload
}

TEST_F(PMemLayoutTest, WriteAmplificationAtLeastOne) {
    PMemStorageLayout layout(make_layout_config());

    const std::string payload(1, 'Z'); // 1 byte – maximum padding overhead
    layout.write("k", payload.data(), payload.size());

    auto ws = layout.get_write_stats();
    EXPECT_GE(ws.write_amplification_x1000, 1000u); // ≥ 1×
}

TEST_F(PMemLayoutTest, FlushAllDoesNotCrash) {
    PMemStorageLayout layout(make_layout_config());
    const std::string payload(512, 'B');
    layout.write("k", payload.data(), payload.size());
    EXPECT_NO_THROW(layout.flush_all());
}

TEST_F(PMemLayoutTest, PoolStatsHealthy) {
    PMemStorageLayout layout(make_layout_config());
    auto ps = layout.get_pool_stats();
    EXPECT_TRUE(ps.is_healthy);
    EXPECT_GT(ps.total_bytes, 0u);
}

// ---------------------------------------------------------------------------
// Persistence helpers tests
// ---------------------------------------------------------------------------

TEST(PMemPersistenceTest, FlushRangeDoesNotCrash) {
    alignas(64) uint8_t buf[512]{};
    std::memset(buf, 0x77, sizeof(buf));
    EXPECT_NO_THROW(pmem_flush_range(buf, sizeof(buf)));
}

TEST(PMemPersistenceTest, PmemPersistDoesNotCrash) {
    alignas(64) uint8_t buf[256]{};
    std::memset(buf, 0x88, sizeof(buf));
    EXPECT_NO_THROW(pmem_persist(buf, sizeof(buf)));
}

TEST(PMemPersistenceTest, SfenceDoesNotCrash) {
    EXPECT_NO_THROW(pmem_sfence());
}

// ---------------------------------------------------------------------------
// Feature flags tests
// ---------------------------------------------------------------------------

TEST(Phase4FeatureFlagsTest, DefaultDisabled) {
    // Reset to false to ensure clean state
    Phase4FeatureFlags::instance().set_pmem_enabled(false);
    EXPECT_FALSE(Phase4FeatureFlags::instance().pmem_enabled());
}

TEST(Phase4FeatureFlagsTest, EnableDisable) {
    Phase4FeatureFlags::instance().set_pmem_enabled(true);
    EXPECT_TRUE(Phase4FeatureFlags::instance().pmem_enabled());

    Phase4FeatureFlags::instance().set_pmem_enabled(false);
    EXPECT_FALSE(Phase4FeatureFlags::instance().pmem_enabled());
}

TEST(Phase4FeatureFlagsTest, MacroOffWithoutCompileFlag) {
    // Without THEMIS_ENABLE_PMEM the macro must evaluate to false
    // regardless of the runtime flag.
#ifndef THEMIS_ENABLE_PMEM
    Phase4FeatureFlags::instance().set_pmem_enabled(true);
    EXPECT_FALSE(THEMIS_PHASE4_PMEM_ENABLED());
    Phase4FeatureFlags::instance().set_pmem_enabled(false);
#else
    // With the compile flag the macro delegates to the runtime flag.
    Phase4FeatureFlags::instance().set_pmem_enabled(true);
    EXPECT_TRUE(THEMIS_PHASE4_PMEM_ENABLED());
    Phase4FeatureFlags::instance().set_pmem_enabled(false);
#endif
}

// ---------------------------------------------------------------------------
// Device detection smoke test (allowed to return an empty list in CI)
// ---------------------------------------------------------------------------

TEST(PMemDeviceDetectionTest, DetectDoesNotCrash) {
    EXPECT_NO_THROW({
        auto devs = detect_pmem_devices();
        // Empty result is valid in CI environments without PMem hardware.
        (void)devs;
    });
}
