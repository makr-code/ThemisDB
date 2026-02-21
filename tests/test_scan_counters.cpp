// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for StorageEngine scan performance counters and predicate scan:
//   - scanCounters() initialises to zero
//   - scanRange increments counters
//   - scanPrefix increments counters
//   - early-stop increments sc_early_stops
//   - resetScanCounters() zeroes all counters
//   - scanPredicate – filters entries by predicate, counts correctly
//   - scanPredicate – early-stop via callback
//   - ScanCounters::selectivity() helper

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

class ScanCounterTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_scanctr_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        engine_ = StorageEngine::createDefault();
        ASSERT_TRUE(engine_->open(db_path_).has_value());
    }

    void TearDown() override {
        if (engine_) engine_->close();
        fs::remove_all(db_path_);
    }

    void insertItems(const std::string& prefix, int from, int to) {
        for (int i = from; i < to; ++i) {
            std::string k = prefix + (i < 10 ? "0" : "") + std::to_string(i);
            ASSERT_TRUE(engine_->put(k, "val_" + std::to_string(i)).has_value());
        }
    }

    std::string                    db_path_;
    std::shared_ptr<StorageEngine> engine_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Initial counters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, InitialCountersAreZero) {
    auto c = engine_->scanCounters();
    EXPECT_EQ(c.scan_calls, 0u);
    EXPECT_EQ(c.keys_examined, 0u);
    EXPECT_EQ(c.keys_returned, 0u);
    EXPECT_EQ(c.early_stops, 0u);
    EXPECT_DOUBLE_EQ(c.selectivity(), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// scanRange updates counters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, ScanRange_IncrementsCalls) {
    insertItems("r:", 0, 5);
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });

    EXPECT_EQ(engine_->scanCounters().scan_calls, 1u);
}

TEST_F(ScanCounterTest, ScanRange_CountsExaminedAndReturned) {
    insertItems("x:", 0, 5);
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.keys_examined, 5u);
    EXPECT_EQ(c.keys_returned, 5u);
}

TEST_F(ScanCounterTest, ScanRange_EarlyStop_RecordsEarlyStop) {
    insertItems("e:", 0, 10);
    int visited = 0;
    engine_->scanRange("", "", [&](std::string_view, std::string_view) {
        return ++visited < 3; // stop after 2
    });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.early_stops, 1u);
    EXPECT_EQ(c.keys_returned, 3u);   // 3 returned (callback returns false on 3rd)
    EXPECT_EQ(c.keys_examined, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// scanPrefix updates counters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, ScanPrefix_IncrementsCalls) {
    engine_->put("pfx:a", "1");
    engine_->put("pfx:b", "2");
    engine_->put("other:c", "3");

    engine_->scanPrefix("pfx:", [](std::string_view, std::string_view) { return true; });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.scan_calls, 1u);
    EXPECT_EQ(c.keys_examined, 2u);
    EXPECT_EQ(c.keys_returned, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// resetScanCounters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, Reset_ZeroesAllCounters) {
    insertItems("z:", 0, 5);
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });

    ASSERT_GT(engine_->scanCounters().scan_calls, 0u);

    engine_->resetScanCounters();

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.scan_calls,    0u);
    EXPECT_EQ(c.keys_examined, 0u);
    EXPECT_EQ(c.keys_returned, 0u);
    EXPECT_EQ(c.early_stops,   0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// scanPredicate
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, ScanPredicate_FiltersCorrectly) {
    // Insert 10 items: item:00 – item:09
    insertItems("item:", 0, 10);

    // Only return items where the value ends in "0" or "5"
    std::vector<std::string> collected;
    auto r = engine_->scanPredicate(
        "", "",
        [](std::string_view /*key*/, std::string_view val) {
            // Keep keys ending in digit 0 or 5
            return !val.empty() && (val.back() == '0' || val.back() == '5');
        },
        [&](std::string_view key, std::string_view /*val*/) {
            collected.emplace_back(key);
            return true;
        });
    ASSERT_TRUE(r.has_value());

    // val_0 and val_5 match
    EXPECT_EQ(collected.size(), 2u);
}

TEST_F(ScanCounterTest, ScanPredicate_CountersSeparateExaminedFromReturned) {
    insertItems("p:", 0, 10);

    engine_->scanPredicate(
        "", "",
        [](std::string_view, std::string_view val) {
            return !val.empty() && val.back() == '5'; // only val_5
        },
        [](std::string_view, std::string_view) { return true; });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.scan_calls,    1u);
    EXPECT_EQ(c.keys_examined, 10u);  // all 10 visited by predicate
    EXPECT_EQ(c.keys_returned,  1u);  // only 1 passed predicate
}

TEST_F(ScanCounterTest, ScanPredicate_EarlyStop) {
    insertItems("q:", 0, 10);

    int returned = 0;
    engine_->scanPredicate(
        "", "",
        [](std::string_view, std::string_view) { return true; }, // accept all
        [&](std::string_view, std::string_view) {
            return ++returned < 3; // stop after 2 callbacks (returns false on 3rd)
        });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.early_stops, 1u);
    EXPECT_EQ(c.keys_returned, 3u);
}

TEST_F(ScanCounterTest, ScanPredicate_ClosedEngine_ReturnsError) {
    engine_->close();
    auto r = engine_->scanPredicate(
        "", "",
        [](std::string_view, std::string_view) { return true; },
        [](std::string_view, std::string_view) { return true; });
    EXPECT_FALSE(r.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// ScanCounters::selectivity helper
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, Selectivity_AllPassed) {
    insertItems("s:", 0, 5);
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });
    EXPECT_DOUBLE_EQ(engine_->scanCounters().selectivity(), 1.0);
}

TEST_F(ScanCounterTest, Selectivity_HalfPassed) {
    insertItems("t:", 0, 10);
    engine_->scanPredicate(
        "", "",
        [](std::string_view, std::string_view val) {
            // Keep entries whose numeric digit value is even: val_0, val_2, val_4, val_6, val_8
            return !val.empty() && ((val.back() - '0') % 2 == 0);
        },
        [](std::string_view, std::string_view) { return true; });

    auto s = engine_->scanCounters().selectivity();
    EXPECT_NEAR(s, 0.5, 0.01);
}

TEST_F(ScanCounterTest, Selectivity_NoExamined_ReturnsOne) {
    // No scans done yet, selectivity() should not divide by zero
    EXPECT_DOUBLE_EQ(engine_->scanCounters().selectivity(), 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple scan calls accumulate
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ScanCounterTest, MultipleScanCalls_CountersAccumulate) {
    insertItems("m:", 0, 5);
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });
    engine_->scanRange("", "", [](std::string_view, std::string_view) { return true; });

    auto c = engine_->scanCounters();
    EXPECT_EQ(c.scan_calls, 2u);
    EXPECT_EQ(c.keys_examined, 10u);  // 5 × 2
    EXPECT_EQ(c.keys_returned, 10u);
}
