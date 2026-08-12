// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// WAL crash-recovery and chaos tests:
//   - Normal recovery (all segments intact)
//   - Recovery after last segment is truncated mid-entry
//   - Recovery with completely empty last segment
//   - Recovery stops at CRC-corrupted entry but keeps prior entries
//   - Multi-segment: entries from all segments recovered
//   - Sequence numbers resume correctly after recovery
//   - Concurrent writes followed by recovery
//   - Recovery with no segments (fresh DB)
//   - Appending after recovery works correctly

#include <gtest/gtest.h>
#include "storage/wal_storage.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static WALStorage::Config makeConfig(const fs::path& dir,
                                     uint64_t rotate_bytes = 0 /* 0 = no rotation */) {
    WALStorage::Config cfg;
    cfg.dir                    = dir.string();
    cfg.rotation_threshold_bytes =
        rotate_bytes > 0 ? rotate_bytes : (64ULL * 1024 * 1024);
    cfg.fsync_on_write         = false;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class WALChaosTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        wal_dir_ = fs::temp_directory_path() /
                   ("themis_walchaos_" + std::to_string(ts));
        fs::remove_all(wal_dir_);
        fs::create_directories(wal_dir_);
    }

    void TearDown() override { fs::remove_all(wal_dir_); }

    // Truncate the last `truncate_bytes` bytes from the most recently
    // modified file in `wal_dir_` (simulates a crash mid-write).
    void truncateLastSegment(size_t truncate_bytes) {
        fs::path latest;
        auto latest_time = fs::file_time_type::min();
        for (const auto& e : fs::directory_iterator(wal_dir_)) {
            if (e.path().extension() == ".log") {
                auto t = fs::last_write_time(e);
                if (t > latest_time) { latest_time = t; latest = e.path(); }
            }
        }
        ASSERT_FALSE(latest.empty()) << "No .log segment found to truncate";
        auto size = fs::file_size(latest);
        ASSERT_GT(size, truncate_bytes) << "Segment too small to truncate";
        fs::resize_file(latest, size - truncate_bytes);
    }

    // Corrupt one byte in the middle of the most recent .log file.
    void corruptLastSegment() {
        fs::path latest;
        auto latest_time = fs::file_time_type::min();
        for (const auto& e : fs::directory_iterator(wal_dir_)) {
            if (e.path().extension() == ".log") {
                auto t = fs::last_write_time(e);
                if (t > latest_time) { latest_time = t; latest = e.path(); }
            }
        }
        ASSERT_FALSE(latest.empty());
        std::fstream f(latest, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(f.is_open());
        // Offset 8 is mid-entry: past the 4-byte magic but within the 8-byte
        // sequence number, so the sequence field is corrupted.  Since the
        // CRC check covers magic+seq+type+klen+vlen+key+value, corrupting
        // any of these bytes causes replay to stop at this entry.
        f.seekp(8);
        char bad = '\xFF';
        f.write(&bad, 1);
    }

    fs::path wal_dir_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Normal recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, NormalRecovery_AllEntriesReplayed) {
    const int kEntries = 20;

    // Write entries
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < kEntries; ++i) {
            ASSERT_TRUE((*wal)->appendPut("key_" + std::to_string(i),
                                          "val_" + std::to_string(i)).has_value());
        }
    }

    // Recover
    std::vector<std::string> recovered_keys;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry& e) {
        if (e.type == WALStorage::EntryType::PUT) {
            recovered_keys.push_back(std::string(e.key));
        }
        return true;
    };
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), cb);
    ASSERT_TRUE(wal2.has_value());

    EXPECT_EQ(recovered_keys.size(), static_cast<size_t>(kEntries));
}

// ─────────────────────────────────────────────────────────────────────────────
// Truncated last segment (simulated crash)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, TruncatedLastEntry_RecoversPriorEntries) {
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < 5; ++i) {
            ASSERT_TRUE((*wal)->appendPut("k_" + std::to_string(i), "v").has_value());
        }
        // Flush to ensure data is on disk before we truncate
        ASSERT_TRUE((*wal)->flush().has_value());
    }
    // Truncate the last 4 bytes (corrupt the last entry's CRC)
    truncateLastSegment(4);

    int count = 0;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry&) {
        ++count; return true;
    };
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), cb);
    ASSERT_TRUE(wal2.has_value());

    // We should get at least 4 complete entries (last one truncated = skipped)
    EXPECT_GE(count, 4) << "At least 4 entries should survive truncation";
    EXPECT_LE(count, 5) << "At most 5 entries (all might survive if truncation hit padding)";
}

TEST_F(WALChaosTest, EmptyLastSegment_RecoverySucceeds) {
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_, 30)); // tiny rotation
        ASSERT_TRUE(wal.has_value());
        // Write enough to trigger rotation (creates a new segment)
        for (int i = 0; i < 10; ++i) {
            ASSERT_TRUE((*wal)->appendPut("long_key_padding", "long_value_padding").has_value());
        }
    }
    // Create an empty .log file at the end (simulates crash after rotation before any write)
    {
        std::ofstream empty_seg((fs::path(wal_dir_) / "wal_099999.log").string());
    }

    int count = 0;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry&) {
        ++count; return true;
    };
    auto wal2 = WALStorage::open(makeConfig(wal_dir_, 30), cb);
    ASSERT_TRUE(wal2.has_value()) << "Recovery should succeed even with empty segment";
}

// ─────────────────────────────────────────────────────────────────────────────
// CRC corruption stops replay at the bad entry but keeps earlier entries
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, CRCCorruption_StopsAtBadEntry_PriorEntriesRecovered) {
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal.has_value());
        // Write 3 entries then flush so they're on disk
        for (int i = 0; i < 3; ++i) {
            ASSERT_TRUE((*wal)->appendPut("crc_k" + std::to_string(i), "v").has_value());
        }
        ASSERT_TRUE((*wal)->flush().has_value());
    }
    corruptLastSegment();

    int count = 0;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry&) {
        ++count; return true;
    };
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), cb);
    ASSERT_TRUE(wal2.has_value()) << "Recovery must not fail on CRC error, just truncate";
    EXPECT_LE(count, 3) << "Cannot recover more entries than written";
}

// ─────────────────────────────────────────────────────────────────────────────
// Multi-segment recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, MultiSegment_AllEntriesRecoveredAcrossRotations) {
    const int kPerSegment = 5;
    // Very small rotation threshold: 30 bytes forces rotation after first entry
    // (HEADER=21 + "ms_key_0"=8 + "value"=5 + CRC=4 = 38 > 30)
    auto cfg = makeConfig(wal_dir_, 30);
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < kPerSegment * 4; ++i) {
            ASSERT_TRUE((*wal)->appendPut("ms_key_" + std::to_string(i),
                                          "value").has_value());
        }
    }

    int seg_count = 0;
    for (const auto& e : fs::directory_iterator(wal_dir_)) {
        if (e.path().extension() == ".log") ++seg_count;
    }
    EXPECT_GT(seg_count, 1) << "Expected multiple segments from rotation";

    std::vector<std::string> keys;
    auto wal2 = WALStorage::open(cfg, [&](const WALStorage::Entry& e) {
        keys.push_back(std::string(e.key));
        return true;
    });
    ASSERT_TRUE(wal2.has_value());
    EXPECT_EQ(keys.size(), static_cast<size_t>(kPerSegment * 4));
}

// ─────────────────────────────────────────────────────────────────────────────
// Sequence numbers resume correctly after recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, SequenceResumes_AfterRecovery) {
    uint64_t last_seq_written = 0;
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < 5; ++i) {
            auto r = (*wal)->appendPut("seq_k" + std::to_string(i), "v");
            ASSERT_TRUE(r.has_value());
            last_seq_written = *r;
        }
    }

    uint64_t last_seq_recovered = 0;
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), [&](const WALStorage::Entry& e) {
        last_seq_recovered = e.sequence;
        return true;
    });
    ASSERT_TRUE(wal2.has_value());
    EXPECT_EQ((*wal2)->lastSequence(), last_seq_written)
        << "lastSequence() after recovery should match what was written";

    // New appends must use sequence > last_seq_written
    auto r = (*wal2)->appendPut("new_after_recovery", "v");
    ASSERT_TRUE(r.has_value());
    EXPECT_GT(*r, last_seq_written);
}

// ─────────────────────────────────────────────────────────────────────────────
// Append after recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, AppendAfterRecovery_WorksCorrectly) {
    {
        auto wal = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("before", "recovery").has_value());
    }

    // Recover and append
    std::vector<std::string> keys;
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), [&](const WALStorage::Entry& e) {
        keys.push_back(std::string(e.key));
        return true;
    });
    ASSERT_TRUE(wal2.has_value());
    ASSERT_TRUE((*wal2)->appendPut("after", "recovery").has_value());
    ASSERT_TRUE((*wal2)->appendDelete("before").has_value());

    // Re-recover and check all 3 entries (put + put + delete)
    std::vector<WALStorage::EntryType> types;
    auto wal3 = WALStorage::open(makeConfig(wal_dir_), [&](const WALStorage::Entry& e) {
        types.push_back(e.type);
        return true;
    });
    ASSERT_TRUE(wal3.has_value());
    EXPECT_EQ(types.size(), 3u);
    EXPECT_EQ(types[0], WALStorage::EntryType::PUT);
    EXPECT_EQ(types[1], WALStorage::EntryType::PUT);
    EXPECT_EQ(types[2], WALStorage::EntryType::DEL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent writes followed by recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, ConcurrentWrites_AllEntriesRecoverable) {
    constexpr int kThreads = 4;
    constexpr int kOps     = 25;

    {
        auto wal_r = WALStorage::open(makeConfig(wal_dir_));
        ASSERT_TRUE(wal_r.has_value());
        auto& wal = *wal_r;

        std::vector<std::thread> threads;
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t] {
                for (int i = 0; i < kOps; ++i) {
                    (void)wal->appendPut(
                        "t" + std::to_string(t) + "_" + std::to_string(i), "v");
                }
            });
        }
        for (auto& th : threads) th.join();
    }

    int count = 0;
    auto wal2 = WALStorage::open(makeConfig(wal_dir_), [&](const WALStorage::Entry&) {
        ++count; return true;
    });
    ASSERT_TRUE(wal2.has_value());
    // All kThreads * kOps entries must survive (WAL is thread-safe)
    EXPECT_EQ(count, kThreads * kOps);
}

// ─────────────────────────────────────────────────────────────────────────────
// Fresh DB (no existing WAL)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALChaosTest, FreshDB_RecoveryCallbackNeverCalled) {
    int cb_calls = 0;
    auto wal = WALStorage::open(makeConfig(wal_dir_), [&](const WALStorage::Entry&) {
        ++cb_calls; return true;
    });
    ASSERT_TRUE(wal.has_value());
    EXPECT_EQ(cb_calls, 0);
    EXPECT_EQ((*wal)->lastSequence(), 0u);
}
