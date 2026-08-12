/**
 * @file test_ingestion_reconfig.cpp
 * @brief Unit tests for dynamic source reconfiguration without restart
 *        (IngestionManager::reconfigureSource and
 *         IngestionAdminApi::reconfigureSource)
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::ingestion;

// ============================================================================
// IngestionManager::reconfigureSource – core behaviour
// ============================================================================

TEST(ReconfigureSourceTest, ReturnsFalseForUnknownSource) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "ghost";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nowhere";
    cfg.enabled   = true;

    EXPECT_FALSE(mgr.reconfigureSource("ghost", cfg));
}

TEST(ReconfigureSourceTest, UpdatesExistingSource) {
    IngestionManager mgr("test_db");

    SourceConfig original;
    original.source_id = "src1";
    original.type      = SourceType::FILESYSTEM;
    original.location  = "/tmp/original";
    original.priority  = 3;
    original.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(original));

    SourceConfig updated = original;
    updated.location  = "/tmp/updated";
    updated.priority  = 7;
    updated.options["key"] = "value";

    EXPECT_TRUE(mgr.reconfigureSource("src1", updated));

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "src1");
    EXPECT_EQ(sources[0].location,  "/tmp/updated");
    EXPECT_EQ(sources[0].priority,  7);
    EXPECT_EQ(sources[0].options.at("key"), "value");
}

TEST(ReconfigureSourceTest, SourceIdPreservedRegardlessOfNewConfigId) {
    IngestionManager mgr("test_db");

    SourceConfig original;
    original.source_id = "canonical_id";
    original.type      = SourceType::FILESYSTEM;
    original.location  = "/tmp/a";
    original.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(original));

    // Supply a new_config whose source_id field differs from the lookup key
    SourceConfig cfg_with_wrong_id = original;
    cfg_with_wrong_id.source_id = "wrong_id";
    cfg_with_wrong_id.location  = "/tmp/b";

    EXPECT_TRUE(mgr.reconfigureSource("canonical_id", cfg_with_wrong_id));

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    // source_id must stay "canonical_id", not "wrong_id"
    EXPECT_EQ(sources[0].source_id, "canonical_id");
    EXPECT_EQ(sources[0].location,  "/tmp/b");
}

TEST(ReconfigureSourceTest, CanToggleEnabledFlag) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "toggle_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/toggle";
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    cfg.enabled = false;
    EXPECT_TRUE(mgr.reconfigureSource("toggle_src", cfg));

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_FALSE(sources[0].enabled);

    cfg.enabled = true;
    EXPECT_TRUE(mgr.reconfigureSource("toggle_src", cfg));
    sources = mgr.getRegisteredSources();
    EXPECT_TRUE(sources[0].enabled);
}

TEST(ReconfigureSourceTest, DoesNotAffectOtherSources) {
    IngestionManager mgr("test_db");

    SourceConfig a;
    a.source_id = "src_a";
    a.type      = SourceType::FILESYSTEM;
    a.location  = "/tmp/a";
    a.priority  = 1;
    a.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(a));

    SourceConfig b;
    b.source_id = "src_b";
    b.type      = SourceType::FILESYSTEM;
    b.location  = "/tmp/b";
    b.priority  = 2;
    b.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(b));

    SourceConfig new_a = a;
    new_a.location = "/tmp/a_new";
    EXPECT_TRUE(mgr.reconfigureSource("src_a", new_a));

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 2u);

    bool found_a = false, found_b = false;
    for (const auto& s : sources) {
        if (s.source_id == "src_a") {
            EXPECT_EQ(s.location, "/tmp/a_new");
            found_a = true;
        }
        if (s.source_id == "src_b") {
            EXPECT_EQ(s.location, "/tmp/b");
            found_b = true;
        }
    }
    EXPECT_TRUE(found_a);
    EXPECT_TRUE(found_b);
}

TEST(ReconfigureSourceTest, ThreadSafeConcurrentReconfigure) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "concurrent_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/v0";
    cfg.priority  = 1;
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&mgr, &success_count, i]() {
            SourceConfig updated;
            updated.source_id = "concurrent_src";
            updated.type      = SourceType::FILESYSTEM;
            updated.location  = "/tmp/v" + std::to_string(i + 1);
            updated.priority  = i + 1;
            updated.enabled   = true;
            if (mgr.reconfigureSource("concurrent_src", updated)) {
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(success_count.load(), kThreads);

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "concurrent_src");
}

TEST(ReconfigureSourceTest, ConcurrentReconfigureAndGetRegisteredSources) {
    IngestionManager mgr("test_db");

    SourceConfig cfg;
    cfg.source_id = "shared_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/initial";
    cfg.priority  = 1;
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    constexpr int kWriters = 4;
    constexpr int kReaders = 4;
    constexpr int kIter    = 50;

    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};

    // Reader threads: continuously call getRegisteredSources()
    for (int i = 0; i < kReaders; ++i) {
        threads.emplace_back([&mgr, &stop]() {
            while (!stop.load(std::memory_order_relaxed)) {
                auto sources = mgr.getRegisteredSources();
                // source_id must always remain "shared_src"
                for (const auto& s : sources) {
                    (void)s.location; // just ensure no crash / data race
                }
            }
        });
    }

    // Writer threads: reconfigure the source repeatedly
    for (int i = 0; i < kWriters; ++i) {
        threads.emplace_back([&mgr, i]() {
            for (int j = 0; j < kIter; ++j) {
                SourceConfig updated;
                updated.source_id = "shared_src";
                updated.type      = SourceType::FILESYSTEM;
                updated.location  = "/tmp/w" + std::to_string(i) + "_" + std::to_string(j);
                updated.priority  = (i * kIter + j) % 10 + 1;
                updated.enabled   = (j % 2 == 0);
                mgr.reconfigureSource("shared_src", updated);
            }
        });
    }

    // Wait for writers to finish, then stop readers
    for (int i = kReaders; i < kReaders + kWriters; ++i) {
        threads[i].join();
    }
    stop.store(true, std::memory_order_relaxed);
    for (int i = 0; i < kReaders; ++i) {
        threads[i].join();
    }

    // After all writes, source must still be present and have a valid state
    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].source_id, "shared_src");
}

// ============================================================================
// IngestionAdminApi::reconfigureSource – delegation
// ============================================================================

TEST(AdminApiReconfigureSourceTest, DelegatesToManager) {
    IngestionManager mgr("test_db");
    IngestionAdminApi admin(mgr);

    SourceConfig cfg;
    cfg.source_id = "admin_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/admin_original";
    cfg.priority  = 2;
    cfg.enabled   = true;
    ASSERT_TRUE(mgr.registerSource(cfg));

    SourceConfig new_cfg = cfg;
    new_cfg.location = "/tmp/admin_updated";
    new_cfg.priority = 9;

    EXPECT_TRUE(admin.reconfigureSource("admin_src", new_cfg));

    auto sources = mgr.getRegisteredSources();
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0].location, "/tmp/admin_updated");
    EXPECT_EQ(sources[0].priority, 9);
}

TEST(AdminApiReconfigureSourceTest, ReturnsFalseForUnknownSource) {
    IngestionManager mgr("test_db");
    IngestionAdminApi admin(mgr);

    SourceConfig cfg;
    cfg.source_id = "nobody";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/x";
    cfg.enabled   = true;

    EXPECT_FALSE(admin.reconfigureSource("nobody", cfg));
}
