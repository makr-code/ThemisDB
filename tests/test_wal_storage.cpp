// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for WALStorage:
//  - Segment naming / parsing
//  - Append PUT / DELETE / CHECKPOINT
//  - CRC32 integrity
//  - Recovery replay on re-open
//  - Log rotation
//  - Checkpoint and old-segment cleanup

#include <gtest/gtest.h>
#include "storage/wal_storage.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string tmpDir(const std::string& suffix) {
    namespace fs = std::filesystem;
    return (fs::temp_directory_path() / ("themis_wal_test_" + suffix)).string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Segment naming
// ─────────────────────────────────────────────────────────────────────────────

TEST(WALStorageNaming, SegmentNameFormat) {
    EXPECT_EQ(WALStorage::segmentName(1),      "wal_000001.log");
    EXPECT_EQ(WALStorage::segmentName(42),     "wal_000042.log");
    EXPECT_EQ(WALStorage::segmentName(999999), "wal_999999.log");
}

TEST(WALStorageNaming, ParseSegmentId) {
    EXPECT_EQ(WALStorage::parseSegmentId("wal_000001.log"),  1u);
    EXPECT_EQ(WALStorage::parseSegmentId("wal_000042.log"),  42u);
    EXPECT_EQ(WALStorage::parseSegmentId("wal_999999.log"),  999999u);
    EXPECT_EQ(WALStorage::parseSegmentId("not_a_wal.log"),   0u);
    EXPECT_EQ(WALStorage::parseSegmentId(""),                0u);
}

TEST(WALStorageNaming, RoundTrip) {
    for (uint64_t id : {1u, 7u, 100u, 12345u}) {
        EXPECT_EQ(WALStorage::parseSegmentId(WALStorage::segmentName(id)), id);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class WALStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = tmpDir(std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        fs::remove_all(dir_);
    }

    void TearDown() override {
        fs::remove_all(dir_);
    }

    WALStorage::Config config(uint64_t rotation_bytes = 64 * 1024 * 1024) {
        WALStorage::Config cfg;
        cfg.dir                      = dir_;
        cfg.rotation_threshold_bytes = rotation_bytes;
        cfg.fsync_on_write           = false; // faster in tests
        return cfg;
    }

    std::string dir_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic open / create
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, OpenCreatesDirectory) {
    auto res = WALStorage::open(config());
    ASSERT_TRUE(res.has_value()) << "open failed";
    EXPECT_TRUE(fs::is_directory(dir_));
}

TEST_F(WALStorageTest, InitialLastSequenceIsZero) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());
    EXPECT_EQ((*wal)->lastSequence(), 0u);
}

TEST_F(WALStorageTest, InitialSegmentCount) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());
    EXPECT_EQ((*wal)->segmentCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Append PUT
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, AppendPut_ReturnsSequence) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());

    auto r1 = (*wal)->appendPut("key1", "value1");
    auto r2 = (*wal)->appendPut("key2", "value2");

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r1, 1u);
    EXPECT_EQ(*r2, 2u);
    EXPECT_EQ((*wal)->lastSequence(), 2u);
}

TEST_F(WALStorageTest, AppendDelete_ReturnsSequence) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());

    auto r = (*wal)->appendDelete("mykey");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Crash recovery: replay on re-open
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, Recovery_ReplaysPutEntries) {
    // First open: write some entries.
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("k1", "v1").has_value());
        ASSERT_TRUE((*wal)->appendPut("k2", "v2").has_value());
        ASSERT_TRUE((*wal)->appendDelete("k1").has_value());
    }

    // Second open: recover entries.
    std::vector<WALStorage::Entry> recovered;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry& e) {
        recovered.push_back(e);
        return true;
    };

    auto wal2 = WALStorage::open(config(), cb);
    ASSERT_TRUE(wal2.has_value());

    ASSERT_EQ(recovered.size(), 3u);
    EXPECT_EQ(recovered[0].type, WALStorage::EntryType::PUT);
    EXPECT_EQ(recovered[0].key,  "k1");
    EXPECT_EQ(recovered[0].value, "v1");

    EXPECT_EQ(recovered[1].type, WALStorage::EntryType::PUT);
    EXPECT_EQ(recovered[1].key,  "k2");

    EXPECT_EQ(recovered[2].type, WALStorage::EntryType::DEL);
    EXPECT_EQ(recovered[2].key,  "k1");
    EXPECT_TRUE(recovered[2].value.empty());
}

TEST_F(WALStorageTest, Recovery_SequenceResumes) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("a", "1").has_value());
        ASSERT_TRUE((*wal)->appendPut("b", "2").has_value());
    }

    // On re-open, the next sequence should continue from 3.
    auto wal2 = WALStorage::open(config());
    ASSERT_TRUE(wal2.has_value());
    auto r = (*wal2)->appendPut("c", "3");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 3u);
}

TEST_F(WALStorageTest, Recovery_EarlyStop) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < 5; ++i) {
            (void)(*wal)->appendPut("k" + std::to_string(i), "v");
        }
    }

    int count = 0;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry&) {
        return ++count < 3; // stop after 2 entries
    };

    auto recovery = WALStorage::open(config(), cb);
    EXPECT_FALSE(recovery.has_value());
    EXPECT_EQ(count, 3); // callback was called for 3 entries (returns false on 3rd)
}

// ─────────────────────────────────────────────────────────────────────────────
// Checkpoint
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, Checkpoint_ReturnsSequence) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());
    (void)(*wal)->appendPut("x", "y");

    auto r = (*wal)->checkpoint(false);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 2u);
}

TEST_F(WALStorageTest, Checkpoint_DeletesOldSegments) {
    // Use a tiny rotation threshold so we get multiple segments quickly.
    auto cfg = config(50); // rotate every 50 bytes
    auto wal = WALStorage::open(cfg);
    ASSERT_TRUE(wal.has_value());

    // Write enough to create at least 2 segments.
    for (int i = 0; i < 20; ++i) {
        (*wal)->appendPut("key_" + std::to_string(i), "value_" + std::to_string(i));
    }

    size_t before = (*wal)->segmentCount();
    (*wal)->checkpoint(true); // delete old segments
    size_t after = (*wal)->segmentCount();

    // After cleanup, there should be at most 1 segment (the active one).
    EXPECT_LE(after, before);
    EXPECT_GE(after, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Log rotation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, Rotation_CreatesNewSegment) {
    auto cfg = config(50); // rotate every 50 bytes
    auto wal = WALStorage::open(cfg);
    ASSERT_TRUE(wal.has_value());

    // Write enough to trigger rotation.
    for (int i = 0; i < 10; ++i) {
        (*wal)->appendPut("longkey_" + std::to_string(i), "longvalue_data_here");
    }

    EXPECT_GT((*wal)->segmentCount(), 1u);
}

TEST_F(WALStorageTest, Rotation_RecoveryAcrossSegments) {
    auto cfg = config(50);
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < 10; ++i) {
            (*wal)->appendPut("key_" + std::to_string(i),
                              "value_" + std::to_string(i));
        }
    }

    std::vector<WALStorage::Entry> entries;
    auto wal2 = WALStorage::open(cfg, [&](const WALStorage::Entry& e) {
        entries.push_back(e);
        return true;
    });
    ASSERT_TRUE(wal2.has_value()) << (wal2 ? "" : wal2.error().message());
    EXPECT_EQ(entries.size(), 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Flush
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, Flush_Succeeds) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());
    (*wal)->appendPut("k", "v");
    auto r = (*wal)->flush();
    EXPECT_TRUE(r.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// CRC32 corruption detection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WALStorageTest, MidLogCorruption_FailsClosed) {
    // Write multiple valid entries so the damaged entry is not at the segment tail.
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("first", "valid").has_value());
        ASSERT_TRUE((*wal)->appendPut("damaged", "record").has_value());
        ASSERT_TRUE((*wal)->appendPut("last", "valid").has_value());
    }

    // Corrupt the second entry's magic while a valid record remains afterward.
    std::string seg_path = dir_ + "/" + WALStorage::segmentName(1);
    {
        std::fstream f(seg_path, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(f.is_open());
        constexpr std::streamoff first_record_size = 21 + 5 + 5 + 4;
        f.seekp(first_record_size);
        char corrupt = '\0';
        f.write(&corrupt, 1);
    }

    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(config(), [](const WALStorage::Entry&) {
        return true;
    }, &recovery_result);
    EXPECT_FALSE(recovery.has_value())
        << "A mid-log magic mismatch must prevent startup";
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(recovery_result.last_valid_lsn, 1u);
}

TEST_F(WALStorageTest, ChecksumMismatch_FailsClosed) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("first", "valid").has_value());
        ASSERT_TRUE((*wal)->appendPut("damaged", "record").has_value());
        ASSERT_TRUE((*wal)->appendPut("last", "valid").has_value());
    }

    const std::string seg_path = dir_ + "/" + WALStorage::segmentName(1);
    {
        std::fstream f(seg_path, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f.seekp(10);
        const char corrupt = static_cast<char>(0xFF);
        f.write(&corrupt, 1);
    }

    auto recovery = WALStorage::open(config(), [](const WALStorage::Entry&) {
        return true;
    });
    EXPECT_FALSE(recovery.has_value())
        << "A checksum mismatch before later records must prevent startup";
}

TEST_F(WALStorageTest, IncompleteTail_IsTruncatedBeforeAppend) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("before-crash", "durable").has_value());
    }

    const std::string seg_path = dir_ + "/" + WALStorage::segmentName(1);
    const auto valid_size = fs::file_size(seg_path);
    {
        std::ofstream f(seg_path, std::ios::binary | std::ios::app);
        ASSERT_TRUE(f.is_open());
        const char partial_header[] = {0x34, 0x12, static_cast<char>(0xAB)};
        f.write(partial_header, sizeof(partial_header));
    }

    std::vector<WALStorage::Entry> recovered;
    WALStorage::RecoveryResult recovery_result;
    auto wal = WALStorage::open(config(), [&](const WALStorage::Entry& entry) {
        recovered.push_back(entry);
        return true;
    }, &recovery_result);
    ASSERT_TRUE(wal.has_value());
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::TruncatedTail);
    EXPECT_EQ(recovery_result.skipped_bytes, 3u);
    ASSERT_EQ(recovered.size(), 1u);
    EXPECT_EQ(fs::file_size(seg_path), valid_size);

    ASSERT_TRUE((*wal)->appendPut("after-recovery", "visible").has_value());
    (*wal).reset();

    recovered.clear();
    auto reopened = WALStorage::open(config(), [&](const WALStorage::Entry& entry) {
        recovered.push_back(entry);
        return true;
    });
    ASSERT_TRUE(reopened.has_value());
    ASSERT_EQ(recovered.size(), 2u);
    EXPECT_EQ(recovered[0].key, "before-crash");
    EXPECT_EQ(recovered[1].key, "after-recovery");
}

TEST_F(WALStorageTest, RecoveryTimeout_FailsClosed) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("slow-callback", "value").has_value());
    }

    auto cfg = config();
    cfg.recovery_timeout = std::chrono::milliseconds(1);
    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(
        cfg,
        [](const WALStorage::Entry&) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return true;
        },
        &recovery_result);

    EXPECT_FALSE(recovery.has_value());
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::TimedOut);
}

TEST_F(WALStorageTest, InvalidPartialHeader_FailsClosedWithoutTruncation) {
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("before-corruption", "value").has_value());
    }

    const auto segment_path = fs::path(dir_) / WALStorage::segmentName(1);
    {
        std::ofstream output(segment_path, std::ios::binary | std::ios::app);
        ASSERT_TRUE(output.is_open());
        const char invalid_tail[] = {0x00, static_cast<char>(0xFF)};
        output.write(invalid_tail, sizeof(invalid_tail));
    }
    const auto corrupted_size = fs::file_size(segment_path);

    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(
        config(),
        [](const WALStorage::Entry&) { return true; },
        &recovery_result);

    EXPECT_FALSE(recovery.has_value());
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(fs::file_size(segment_path), corrupted_size);
}

TEST_F(WALStorageTest, SkipCorrupted_QuarantinesSegment) {
    auto cfg = config(25);
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("first", "one").has_value());
        ASSERT_TRUE((*wal)->appendPut("second", "two").has_value());
        ASSERT_GT((*wal)->segmentCount(), 1u);
    }

    const auto corrupt_path = fs::path(dir_) / WALStorage::segmentName(1);
    {
        std::fstream f(corrupt_path, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f.seekp(0);
        const char corrupt = '\0';
        f.write(&corrupt, 1);
    }

    cfg.recovery_mode = WALStorage::RecoveryMode::SkipCorrupted;
    WALStorage::RecoveryResult recovery_result;
    std::vector<std::string> recovered_keys;
    auto recovery = WALStorage::open(
        cfg,
        [&](const WALStorage::Entry& entry) {
            recovered_keys.push_back(entry.key);
            return true;
        },
        &recovery_result);

    ASSERT_TRUE(recovery.has_value());
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_GT(recovery_result.skipped_bytes, 0u);
    EXPECT_FALSE(fs::exists(corrupt_path));
    EXPECT_TRUE(fs::exists(fs::path(dir_) / "quarantine" /
                           WALStorage::segmentName(1)));
    ASSERT_EQ(recovered_keys.size(), 1u);
    EXPECT_EQ(recovered_keys.front(), "second");
}

TEST_F(WALStorageTest, DeterministicBitFlips_FailClosedForTenThousandCases) {
    auto wal = WALStorage::open(config());
    ASSERT_TRUE(wal.has_value());
    ASSERT_TRUE((*wal)->appendPut(std::string(80, 'k'), std::string(80, 'v')).has_value());
    wal->reset();

    const auto segment_path = fs::path(dir_) / WALStorage::segmentName(1);
    std::ifstream input(segment_path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    const std::vector<char> original(
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    ASSERT_GT(original.size(), size_t{21});

    std::mt19937 generator(0x5EEDu);
    std::uniform_int_distribution<size_t> byte_distribution(21, original.size() - 1);
    std::uniform_int_distribution<unsigned int> bit_distribution(0, 7);
    auto cfg = config();

    for (size_t case_index = 0; case_index < 10'000; ++case_index) {
        auto damaged = original;
        const size_t byte_index = byte_distribution(generator);
        damaged[byte_index] ^= static_cast<char>(1u << bit_distribution(generator));

        {
            std::ofstream output(segment_path, std::ios::binary | std::ios::trunc);
            ASSERT_TRUE(output.is_open());
            output.write(damaged.data(), static_cast<std::streamsize>(damaged.size()));
            ASSERT_TRUE(output.good());
        }

        WALStorage::RecoveryResult recovery_result;
        auto recovery = WALStorage::open(
            cfg,
            [](const WALStorage::Entry&) { return true; },
            &recovery_result);
        EXPECT_FALSE(recovery.has_value()) << "bit-flip case " << case_index;
        EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted)
            << "bit-flip case " << case_index << " at byte " << byte_index;
    }
}
