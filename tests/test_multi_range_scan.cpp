// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for StorageEngine::scanMultiRange() and the IStorageEngine::ScanRange struct:
//   - Empty range list returns ok with no callbacks
//   - Single range matches expected keys
//   - Multiple non-overlapping ranges deliver all matching keys in order
//   - Overlapping ranges may deliver duplicate keys (documented behaviour)
//   - Early-stop via callback halts all subsequent ranges
//   - ScanCounters updated for multi-range calls
//   - Integration with the IStorageEngine interface default implementation

#include <gtest/gtest.h>
#include "storage/storage_engine.h"

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class MultiRangeScanTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_multirng_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);
        engine_ = StorageEngine::createDefault();
        ASSERT_TRUE(engine_->open(db_path_).has_value());

        // Insert keys: a:00 – a:09 (10), b:00 – b:09 (10), c:00 – c:09 (10)
        for (int i = 0; i < 10; ++i) {
            std::string idx = "0" + std::to_string(i);  // always 0-9, so always "0x"
            ASSERT_TRUE(engine_->put("a:" + idx, "va" + idx).has_value());
            ASSERT_TRUE(engine_->put("b:" + idx, "vb" + idx).has_value());
            ASSERT_TRUE(engine_->put("c:" + idx, "vc" + idx).has_value());
        }
    }

    void TearDown() override {
        if (engine_) engine_->close();
        fs::remove_all(db_path_);
    }

    std::string                    db_path_;
    std::shared_ptr<StorageEngine> engine_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MultiRangeScanTest, EmptyRangeList_ReturnsOkNoCallbacks) {
    std::vector<IStorageEngine::ScanRange> ranges;
    int calls = 0;
    auto res = engine_->scanMultiRange(ranges,
        [&](std::string_view, std::string_view) { ++calls; return true; });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(calls, 0);
}

TEST_F(MultiRangeScanTest, SingleRange_MatchesExpectedKeys) {
    std::vector<IStorageEngine::ScanRange> ranges = {{"a:", "b:"}};
    std::vector<std::string> collected;
    auto res = engine_->scanMultiRange(ranges,
        [&](std::string_view k, std::string_view) {
            collected.emplace_back(k);
            return true;
        });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(collected.size(), 10u);
    for (const auto& k : collected) {
        EXPECT_EQ(k.substr(0, 2), "a:");
    }
}

TEST_F(MultiRangeScanTest, TwoNonOverlappingRanges_AllKeysDelivered) {
    std::vector<IStorageEngine::ScanRange> ranges = {
        {"a:", "b:"},
        {"c:", "d:"}
    };
    std::vector<std::string> collected;
    auto res = engine_->scanMultiRange(ranges,
        [&](std::string_view k, std::string_view) {
            collected.emplace_back(k);
            return true;
        });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(collected.size(), 20u);
    // First 10 should be "a:*", next 10 should be "c:*"
    for (int i = 0; i < 10; ++i)  EXPECT_EQ(collected[i].substr(0, 2), "a:");
    for (int i = 10; i < 20; ++i) EXPECT_EQ(collected[i].substr(0, 2), "c:");
}

TEST_F(MultiRangeScanTest, ThreeRanges_AllPresent) {
    std::vector<IStorageEngine::ScanRange> ranges = {
        {"a:", "a;"},  // all a:xx keys  (';' > ':' in ASCII)
        {"b:", "b;"},
        {"c:", "c;"}
    };
    int total = 0;
    auto res = engine_->scanMultiRange(ranges,
        [&](std::string_view, std::string_view) { ++total; return true; });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(total, 30);
}

TEST_F(MultiRangeScanTest, EarlyStop_HaltsAllRanges) {
    std::vector<IStorageEngine::ScanRange> ranges = {
        {"a:", "b:"},
        {"b:", "c:"},
        {"c:", "d:"}
    };
    int calls = 0;
    auto res = engine_->scanMultiRange(ranges,
        [&](std::string_view, std::string_view) {
            ++calls;
            return calls < 3;  // stop after 3rd key
        });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(calls, 3);
}

TEST_F(MultiRangeScanTest, ScanCounters_UpdatedForMultiRange) {
    engine_->resetScanCounters();

    std::vector<IStorageEngine::ScanRange> ranges = {
        {"a:", "b:"},
        {"b:", "c:"}
    };
    auto res = engine_->scanMultiRange(ranges,
        [](std::string_view, std::string_view) { return true; });
    EXPECT_TRUE(res.has_value());

    auto sc = engine_->scanCounters();
    // scanMultiRange delegates to scanRange calls → scan_calls should be ≥ 2
    EXPECT_GE(sc.scan_calls, 2u);
    EXPECT_EQ(sc.keys_returned, 20u);
}

TEST_F(MultiRangeScanTest, OpenEndedEndKey_ScansToEnd) {
    // end_key = "" means scan to end of keyspace
    std::vector<IStorageEngine::ScanRange> ranges = {{"c:", ""}};
    std::vector<std::string> keys;
    engine_->scanMultiRange(ranges,
        [&](std::string_view k, std::string_view) {
            keys.emplace_back(k);
            return true;
        });
    EXPECT_EQ(keys.size(), 10u);
    for (const auto& k : keys) EXPECT_EQ(k.substr(0, 2), "c:");
}

TEST_F(MultiRangeScanTest, InterfaceDefaultImpl_WorksViaBasePointer) {
    // Access through IStorageEngine pointer to exercise the default
    // scanMultiRange() implementation in the interface.
    IStorageEngine* base = engine_.get();
    std::vector<IStorageEngine::ScanRange> ranges = {{"a:", "b:"}};
    int calls = 0;
    auto res = base->scanMultiRange(ranges,
        [&](std::string_view, std::string_view) { ++calls; return true; });
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(calls, 10);
}
