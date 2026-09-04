// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Additional tests for the StorageEngine after production-readiness improvements:
//  - open/close lifecycle backed by real RocksDB
//  - put / get / del round-trip
//  - Key-not-found error
//  - scanRange – ordered iteration, open-ended bounds, predicate early-stop
//  - scanPrefix – prefix filtering

#include <gtest/gtest.h>
#include "storage/storage_engine.h"

#include <filesystem>
#include <string>
#include <vector>
#include <utility>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class StorageEngineProdTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_storage_prod_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        engine_ = StorageEngine::createDefault();
        ASSERT_TRUE(engine_->open(db_path_).has_value())
            << "Failed to open StorageEngine at: " << db_path_;
    }

    void TearDown() override {
        if (engine_) {
          engine_->close();
        }
        fs::remove_all(db_path_);
    }

    // Insert a sorted set of keys: prefix + 2-digit suffix.
    void insertRange(const std::string& prefix, int from, int to) {
        for (int i = from; i < to; ++i) {
            std::string k = prefix + (i < 10 ? "0" : "") + std::to_string(i);
            ASSERT_TRUE(engine_->put(k, "val_" + std::to_string(i)).has_value());
        }
    }

    std::string                    db_path_ = {};
    std::shared_ptr<StorageEngine> engine_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineProdTest, OpenAlreadyOpen_ReturnsError) {
    auto r = engine_->open(db_path_);
    EXPECT_FALSE(r.has_value()) << "Expected error on double-open";
}

TEST_F(StorageEngineProdTest, CloseAndReopenSucceeds) {
    engine_->close();
    auto r = engine_->open(db_path_);
    EXPECT_TRUE(r.has_value()) << "Reopen after close should succeed";
}

// ─────────────────────────────────────────────────────────────────────────────
// put / get / del
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineProdTest, PutGetRoundTrip) {
    ASSERT_TRUE(engine_->put("hello", "world").has_value());

    auto r = engine_->get("hello");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "world");
}

TEST_F(StorageEngineProdTest, Get_MissingKey_ReturnsError) {
    auto r = engine_->get("no_such_key_xyz");
    EXPECT_FALSE(r.has_value());
}

TEST_F(StorageEngineProdTest, Del_RemovesKey) {
    ASSERT_TRUE(engine_->put("to_delete", "v").has_value());
    ASSERT_TRUE(engine_->del("to_delete").has_value());

    auto r = engine_->get("to_delete");
    EXPECT_FALSE(r.has_value());
}

TEST_F(StorageEngineProdTest, Put_Overwrite) {
    engine_->put("k", "v1");
    engine_->put("k", "v2");

    auto r = engine_->get("k");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// scanRange
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineProdTest, ScanRange_InOrder) {
    insertRange("item:", 0, 5); // item:00 .. item:04

    std::vector<std::string> keys;
    auto r = engine_->scanRange(
        "item:00", "item:05",
        [&](std::string_view k, std::string_view) {
            keys.emplace_back(k);
            return true;
        });
    ASSERT_TRUE(r.has_value());

    ASSERT_EQ(keys.size(), 5u);
    for (size_t i = 0; i < keys.size(); ++i) {
        // Keys must arrive in sorted (ascending) order.
        if (i > 0) {
          EXPECT_GT(keys[i], keys[i - 1]);
        }
    }
}

TEST_F(StorageEngineProdTest, ScanRange_HalfOpen_EndExclusive) {
    insertRange("x:", 0, 3); // x:00, x:01, x:02

    std::vector<std::string> keys;
    engine_->scanRange("x:00", "x:02", [&](std::string_view k, std::string_view) {
        keys.emplace_back(k);
        return true;
    });

    // x:02 must NOT be in results (exclusive upper bound).
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_EQ(keys.back(), "x:01");
}

TEST_F(StorageEngineProdTest, ScanRange_EarlyStop) {
    insertRange("e:", 0, 10);

    int count = 0;
    engine_->scanRange("e:00", "e:99", [&](std::string_view, std::string_view) {
        return ++count < 3; // stop after 2
    });
    EXPECT_EQ(count, 3); // callback returns false on 3rd call
}

TEST_F(StorageEngineProdTest, ScanRange_EmptyResult) {
    bool called = false;
    engine_->scanRange("z:zz", "z:zzz", [&](std::string_view, std::string_view) {
        called = true;
        return true;
    });
    EXPECT_FALSE(called);
}

TEST_F(StorageEngineProdTest, ScanRange_FullKeyspace) {
    insertRange("p:", 0, 5);

    std::vector<std::string> keys;
    engine_->scanRange("p:00", "p:99", [&](std::string_view k, std::string_view) {
        keys.emplace_back(k);
        return true;
    });
    // All 5 keys in the p:-namespace must appear.
    EXPECT_EQ(keys.size(), 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// scanPrefix
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineProdTest, ScanPrefix_FindsMatchingKeys) {
    engine_->put("doc:1", "a");
    engine_->put("doc:2", "b");
    engine_->put("doc:3", "c");
    engine_->put("other:1", "d");

    std::vector<std::string> keys;
    auto r = engine_->scanPrefix(
        "doc:",
        [&](std::string_view k, std::string_view) {
            keys.emplace_back(k);
            return true;
        });
    ASSERT_TRUE(r.has_value());

    ASSERT_EQ(keys.size(), 3u);
    for (auto& k : keys) {
        EXPECT_TRUE(k.rfind("doc:", 0) == 0) << "Key does not have prefix: " << k;
    }
}

TEST_F(StorageEngineProdTest, ScanPrefix_NoMatch) {
    engine_->put("abc:1", "x");

    bool called = false;
    engine_->scanPrefix("xyz:", [&](std::string_view, std::string_view) {
        called = true;
        return true;
    });
    EXPECT_FALSE(called);
}

TEST_F(StorageEngineProdTest, ScanPrefix_EarlyStop) {
    for (int i = 0; i < 10; ++i) {
        engine_->put("pfx:" + std::to_string(i), "v");
    }

    int count = 0;
    engine_->scanPrefix("pfx:", [&](std::string_view, std::string_view) {
        return ++count < 3;
    });
    EXPECT_EQ(count, 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Closed engine rejects operations
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineProdTest, ClosedEngine_PutFails) {
    engine_->close();
    EXPECT_FALSE(engine_->put("k", "v").has_value());
}

TEST_F(StorageEngineProdTest, ClosedEngine_GetFails) {
    engine_->close();
    EXPECT_FALSE(engine_->get("k").has_value());
}

TEST_F(StorageEngineProdTest, ClosedEngine_DelFails) {
    engine_->close();
    EXPECT_FALSE(engine_->del("k").has_value());
}

TEST_F(StorageEngineProdTest, ClosedEngine_ScanRangeFails) {
    engine_->close();
    auto r = engine_->scanRange("", "", [](std::string_view, std::string_view) {
        return true;
    });
    EXPECT_FALSE(r.has_value());
}
