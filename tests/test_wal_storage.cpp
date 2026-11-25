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
#include "observability/metrics_collector.h"
#include "storage/wal_storage.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <thread>
#include <vector>
#if !defined(_WIN32)
#include <cstdlib>
#endif

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string tmpDir(const std::string& suffix) {
    namespace fs = std::filesystem;
    return (fs::temp_directory_path() / ("themis_wal_test_" + suffix)).string();
}

static int64_t metricCounterValue(const std::string& exported,
                                  const std::string& series) {
    const std::string needle = "\n" + series + " ";
    const auto series_pos = exported.find(needle);
    if (series_pos == std::string::npos) {
        return 0;
    }

    const auto value_pos = series_pos + needle.size();
    return std::stoll(exported.substr(value_pos));
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

#if !defined(_WIN32)
TEST_F(WALStorageTest, DirectoryMetadataSyncFailureRejectsOpen) {
    ASSERT_EQ(::setenv("THEMIS_TEST_WAL_DIRECTORY_FSYNC_FAIL_ONCE", "1", 1), 0);

    auto cfg = config();
    cfg.dir = (fs::path(dir_) / "deep" / "nested").string();
    auto wal = WALStorage::open(cfg);
    ::unsetenv("THEMIS_TEST_WAL_DIRECTORY_FSYNC_FAIL_ONCE");

    ASSERT_FALSE(wal);
    EXPECT_TRUE(fs::exists(fs::path(cfg.dir) / WALStorage::segmentName(1)));

    auto retry = WALStorage::open(cfg);
    ASSERT_TRUE(retry.has_value()) << retry.error().message();
}

TEST_F(WALStorageTest, DirectoryMetadataSyncFailureRejectsRotatingAppend) {
    auto cfg = config(1);
    auto wal = WALStorage::open(cfg);
    ASSERT_TRUE(wal.has_value());
    ASSERT_TRUE((*wal)->appendPut("first", "durable").has_value());

    ASSERT_EQ(::setenv("THEMIS_TEST_WAL_DIRECTORY_FSYNC_FAIL_ONCE", "1", 1), 0);
    auto rejected = (*wal)->appendPut("second", "not yet durable");
    ::unsetenv("THEMIS_TEST_WAL_DIRECTORY_FSYNC_FAIL_ONCE");

    ASSERT_FALSE(rejected);
    EXPECT_EQ((*wal)->lastSequence(), 1u);

    auto retried = (*wal)->appendPut("second", "durable after retry");
    ASSERT_TRUE(retried.has_value()) << retried.error().message();
    EXPECT_EQ((*wal)->lastSequence(), 2u);
}
#endif

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
    WALStorage::RecoveryResult recovery_result;
    std::vector<WALStorage::RecoveryResult> progress;
    auto wal2 = WALStorage::open(cfg, [&](const WALStorage::Entry& e) {
        entries.push_back(e);
        return true;
    }, [&](const WALStorage::RecoveryResult& update) {
        progress.push_back(update);
    }, &recovery_result);
    ASSERT_TRUE(wal2.has_value()) << (wal2 ? "" : wal2.error().message());
    EXPECT_EQ(entries.size(), 10u);
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Complete);
    EXPECT_EQ(recovery_result.recovered_entries, 10u);
    EXPECT_EQ(recovery_result.last_valid_lsn, entries.back().sequence);
    EXPECT_GT(recovery_result.total_segments, 1u);
    ASSERT_FALSE(progress.empty());
    EXPECT_EQ(progress.back().status, WALStorage::RecoveryStatus::Complete);
    EXPECT_EQ(progress.back().recovered_entries, 10u);
}

TEST_F(WALStorageTest, RecoveryMetricsIncludeProgressAndTerminalStatus) {
    auto& metrics = observability::MetricsCollector::getInstance();
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("metric-key", "metric-value").has_value());
    }

    const auto before = metrics.getPrometheusMetrics();
    WALStorage::RecoveryResult recovery_result;
    auto wal = WALStorage::open(
        config(), [](const WALStorage::Entry&) { return true; }, nullptr,
        &recovery_result);
    ASSERT_TRUE(wal.has_value());

    const auto exported = metrics.getPrometheusMetrics();
    const std::string complete_status =
        "storage_wal_recovery_attempts_total{status=\"COMPLETE\"}";
    EXPECT_EQ(metricCounterValue(exported, "storage_wal_recovery_entries_total"),
              metricCounterValue(before, "storage_wal_recovery_entries_total") + 1);
    EXPECT_GT(metricCounterValue(exported, "storage_wal_recovery_bytes_total"),
              metricCounterValue(before, "storage_wal_recovery_bytes_total"));
    EXPECT_EQ(metricCounterValue(exported, complete_status),
              metricCounterValue(before, complete_status) + 1);
    EXPECT_NE(exported.find(
                  "storage_wal_recovery_duration_ms{status=\"COMPLETE\",quantile=\"0.5\"}"),
              std::string::npos);
}

TEST_F(WALStorageTest, OpenWithProgressCallbackOnly) {
    std::vector<WALStorage::RecoveryResult> progress;
    auto wal = WALStorage::open(
        config(), [](const WALStorage::Entry&) { return true; },
        [&](const WALStorage::RecoveryResult& update) {
            progress.push_back(update);
        });

    ASSERT_TRUE(wal.has_value());
    ASSERT_FALSE(progress.empty());
    EXPECT_EQ(progress.back().status, WALStorage::RecoveryStatus::Complete);
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

TEST_F(WALStorageTest, DeterministicBitFlips_FailClosedAcrossTenThousandCases) {
    const std::string first_key = "bitflip-key";
    const std::string first_value(256, 'x');
    const std::string seg_path = dir_ + "/" + WALStorage::segmentName(1);
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut(first_key, first_value).has_value());
        ASSERT_TRUE((*wal)->appendPut("later-entry", "intact").has_value());
    }

    std::vector<char> original_segment;
    {
        std::ifstream input(seg_path, std::ios::binary);
        ASSERT_TRUE(input.is_open());
        original_segment.assign(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
    }

    constexpr std::size_t RECORD_HEADER_AND_CHECKSUM_BYTES = 25;
    const auto first_record_size =
        RECORD_HEADER_AND_CHECKSUM_BYTES + first_key.size() + first_value.size();
    ASSERT_LT(first_record_size, original_segment.size());

    constexpr std::size_t CASE_COUNT = 10'000;
    std::mt19937_64 random(0x57A6D3B91C204E5FULL);
    std::uniform_int_distribution<std::size_t> byte_distribution(
        0, first_record_size - 1);
    std::uniform_int_distribution<unsigned int> bit_distribution(0, 7);
    const auto previous_log_level = spdlog::get_level();
    spdlog::set_level(spdlog::level::off);

    std::size_t completed_cases = 0;
    for (std::size_t case_index = 0; case_index < CASE_COUNT; ++case_index) {
        auto mutated_segment = original_segment;
        const auto byte_index = byte_distribution(random);
        const auto bit_index = bit_distribution(random);
        mutated_segment[byte_index] = static_cast<char>(
            static_cast<unsigned char>(mutated_segment[byte_index]) ^
            static_cast<unsigned char>(1U << bit_index));

        {
            std::ofstream output(seg_path, std::ios::binary | std::ios::trunc);
            if (!output.is_open()) {
                ADD_FAILURE() << "Could not rewrite WAL for mutation case "
                              << case_index;
                continue;
            }
            output.write(mutated_segment.data(),
                         static_cast<std::streamsize>(mutated_segment.size()));
            output.close();
            if (!output) {
                ADD_FAILURE() << "Could not persist WAL mutation case "
                              << case_index;
                continue;
            }
        }

        WALStorage::RecoveryResult recovery_result;
        auto recovery = WALStorage::open(
            config(), [](const WALStorage::Entry&) { return true; }, nullptr,
            &recovery_result);
        EXPECT_FALSE(recovery.has_value())
            << "Single-bit corruption was accepted in case " << case_index
            << " at byte " << byte_index << ", bit " << bit_index;
        EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted)
            << "Unexpected recovery status in case " << case_index;
        ++completed_cases;
    }

    spdlog::set_level(previous_log_level);
    EXPECT_EQ(completed_cases, CASE_COUNT);
}

TEST_F(WALStorageTest, DeterministicMultiBitMutations_FailClosedAcross1024Cases) {
    const std::string first_key = "multibit-key";
    const std::string first_value(256, 'm');
    const std::string seg_path = dir_ + "/" + WALStorage::segmentName(1);
    {
        auto wal = WALStorage::open(config());
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut(first_key, first_value).has_value());
        ASSERT_TRUE((*wal)->appendPut("later-entry", "intact").has_value());
    }

    std::vector<char> original_segment;
    {
        std::ifstream input(seg_path, std::ios::binary);
        ASSERT_TRUE(input.is_open());
        original_segment.assign(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
    }

    constexpr std::size_t RECORD_HEADER_AND_CHECKSUM_BYTES = 25;
    const auto first_record_size =
        RECORD_HEADER_AND_CHECKSUM_BYTES + first_key.size() + first_value.size();
    ASSERT_LT(first_record_size, original_segment.size());

    constexpr std::size_t CASE_COUNT = 1'024;
    const auto record_bit_count = first_record_size * 8;
    std::mt19937_64 random(0xB17F11D3A5C09E27ULL);
    std::uniform_int_distribution<std::size_t> bit_distribution(
        0, record_bit_count - 1);
    const auto previous_log_level = spdlog::get_level();
    spdlog::set_level(spdlog::level::off);

    std::size_t completed_cases = 0;
    for (std::size_t case_index = 0; case_index < CASE_COUNT; ++case_index) {
        auto mutated_segment = original_segment;
        const auto first_bit = bit_distribution(random);
        auto second_bit = bit_distribution(random);
        while (second_bit == first_bit) {
            second_bit = bit_distribution(random);
        }
        for (const auto bit_index : {first_bit, second_bit}) {
            const auto byte_index = bit_index / 8;
            const auto bit_offset = bit_index % 8;
            mutated_segment[byte_index] = static_cast<char>(
                static_cast<unsigned char>(mutated_segment[byte_index]) ^
                static_cast<unsigned char>(1U << bit_offset));
        }

        {
            std::ofstream output(seg_path, std::ios::binary | std::ios::trunc);
            if (!output.is_open()) {
                ADD_FAILURE() << "Could not rewrite WAL for mutation case "
                              << case_index;
                continue;
            }
            output.write(mutated_segment.data(),
                         static_cast<std::streamsize>(mutated_segment.size()));
            output.close();
            if (!output) {
                ADD_FAILURE() << "Could not persist WAL mutation case "
                              << case_index;
                continue;
            }
        }

        WALStorage::RecoveryResult recovery_result;
        auto recovery = WALStorage::open(
            config(), [](const WALStorage::Entry&) { return true; }, nullptr,
            &recovery_result);
        EXPECT_FALSE(recovery.has_value())
            << "Two-bit corruption was accepted in case " << case_index
            << " at bits " << first_bit << " and " << second_bit;
        EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted)
            << "Unexpected recovery status in case " << case_index;
        ++completed_cases;
    }

    spdlog::set_level(previous_log_level);
    EXPECT_EQ(completed_cases, CASE_COUNT);
}

TEST_F(WALStorageTest, CorruptionInLaterSegmentFailsClosedAfterValidPrefix) {
    auto cfg = config(1);
    const std::vector<std::string> keys = {"segment-one", "segment-two",
                                           "segment-three"};
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        for (const auto& key : keys) {
            ASSERT_TRUE((*wal)->appendPut(key, "valid-value").has_value());
        }
        ASSERT_EQ((*wal)->segmentCount(), 3u);
    }

    const auto third_segment =
        dir_ + "/" + WALStorage::segmentName(3);
    {
        std::fstream segment(third_segment,
                             std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(segment.is_open());
        segment.seekp(0);
        const char corrupt_magic = '\0';
        segment.write(&corrupt_magic, 1);
        ASSERT_TRUE(segment.good());
    }

    std::vector<WALStorage::Entry> recovered;
    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(
        cfg, [&](const WALStorage::Entry& entry) {
            recovered.push_back(entry);
            return true;
        }, nullptr, &recovery_result);

    EXPECT_FALSE(recovery.has_value())
        << "Corruption in a later segment must prevent successful startup";
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(recovery_result.segment_id, 3u);
    EXPECT_EQ(recovery_result.segment, third_segment);
    EXPECT_EQ(recovery_result.last_valid_lsn, 2u);
    EXPECT_EQ(recovery_result.recovered_entries, 2u);
    ASSERT_EQ(recovered.size(), 2u);
    EXPECT_EQ(recovered[0].key, keys[0]);
    EXPECT_EQ(recovered[1].key, keys[1]);
}

TEST_F(WALStorageTest,
       IncompleteRecordInEarlierSegment_FailsClosedWithoutTruncation) {
    auto cfg = config(1);
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        ASSERT_TRUE((*wal)->appendPut("first-segment", "valid").has_value());
        ASSERT_TRUE((*wal)->appendPut("second-segment", "valid").has_value());
        ASSERT_EQ((*wal)->segmentCount(), 2u);
    }

    const auto first_segment_path =
        dir_ + "/" + WALStorage::segmentName(1);
    const auto original_size = fs::file_size(first_segment_path);
    std::vector<char> original_segment;
    {
        std::ifstream input(first_segment_path, std::ios::binary);
        ASSERT_TRUE(input.is_open());
        original_segment.assign(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
        ASSERT_EQ(original_segment.size(), original_size);
    }
    {
        std::fstream segment(first_segment_path,
                             std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(segment.is_open());
        segment.seekp(17);
        const char oversized_value_length[] = {
            static_cast<char>(0xFF), static_cast<char>(0xFF),
            static_cast<char>(0xFF), static_cast<char>(0x7F)};
        segment.write(oversized_value_length, sizeof(oversized_value_length));
        ASSERT_TRUE(segment.good());
    }

    const auto corrupted_size = fs::file_size(first_segment_path);
    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(
        cfg, [](const WALStorage::Entry&) { return true; }, nullptr,
        &recovery_result);

    EXPECT_FALSE(recovery.has_value())
        << "An incomplete record in a non-final segment cannot be treated "
           "as a crash-truncated tail";
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(recovery_result.segment_id, 1u);
    EXPECT_EQ(recovery_result.recovered_entries, 0u);
    EXPECT_EQ(fs::file_size(first_segment_path), corrupted_size);
    EXPECT_EQ(corrupted_size, original_size);

    {
        std::ofstream segment(first_segment_path,
                             std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(segment.is_open());
        segment.write(original_segment.data(),
                      static_cast<std::streamsize>(original_segment.size()));
        ASSERT_TRUE(segment.good());
    }
    {
        std::ofstream segment(first_segment_path,
                             std::ios::binary | std::ios::app);
        ASSERT_TRUE(segment.is_open());
        const char partial_header[] = {0x34, 0x12, static_cast<char>(0xAB)};
        segment.write(partial_header, sizeof(partial_header));
        ASSERT_TRUE(segment.good());
    }

    const auto partial_size = fs::file_size(first_segment_path);
    WALStorage::RecoveryResult partial_recovery_result;
    auto partial_recovery = WALStorage::open(
        cfg, [](const WALStorage::Entry&) { return true; }, nullptr,
        &partial_recovery_result);

    EXPECT_FALSE(partial_recovery.has_value())
        << "A partial header in a non-final segment cannot be treated as a "
           "crash-truncated tail";
    EXPECT_EQ(partial_recovery_result.status,
              WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(partial_recovery_result.segment_id, 1u);
    EXPECT_EQ(partial_recovery_result.recovered_entries, 1u);
    EXPECT_EQ(fs::file_size(first_segment_path), partial_size);
}

TEST_F(WALStorageTest,
       DeterministicCorruptionAcrossPayloadLengthsAndSegments_FailsClosed) {
    auto cfg = config(1);
    const std::vector<std::size_t> payload_lengths = {
        0, 1, 7, 31, 255, 1024, 3, 512, 64, 2048, 17, 127};
    std::vector<std::string> keys;
    keys.reserve(payload_lengths.size());
    uint64_t segment_count = 0;
    {
        auto wal = WALStorage::open(cfg);
        ASSERT_TRUE(wal.has_value());
        for (std::size_t index = 0; index < payload_lengths.size(); ++index) {
            keys.push_back("payload-" + std::to_string(index));
            const std::string value(
                payload_lengths[index],
                static_cast<char>('a' + index % 26));
            ASSERT_TRUE((*wal)->appendPut(keys.back(), value).has_value());
        }
        segment_count = (*wal)->segmentCount();
        ASSERT_EQ(segment_count, payload_lengths.size());
    }

    std::vector<std::vector<char>> original_segments;
    original_segments.reserve(static_cast<std::size_t>(segment_count));
    for (uint64_t segment_id = 1; segment_id <= segment_count; ++segment_id) {
        const auto segment_path =
            dir_ + "/" + WALStorage::segmentName(segment_id);
        ASSERT_TRUE(fs::exists(segment_path)) << "Missing segment " << segment_id;
        std::ifstream input(segment_path, std::ios::binary);
        ASSERT_TRUE(input.is_open()) << "Could not read " << segment_path;
        original_segments.emplace_back(std::istreambuf_iterator<char>(input),
                                       std::istreambuf_iterator<char>());
        ASSERT_FALSE(original_segments.back().empty());
    }
    ASSERT_EQ(original_segments.size(), payload_lengths.size());

    std::vector<std::vector<std::size_t>> corruption_offsets;
    std::vector<std::vector<std::size_t>> corruption_bits;
    corruption_offsets.reserve(original_segments.size());
    corruption_bits.reserve(original_segments.size());
    for (const auto& segment : original_segments) {
        auto& offsets = corruption_offsets.emplace_back();
        auto& bits = corruption_bits.emplace_back();
        for (std::size_t offset = 0; offset < segment.size(); ++offset) {
            if (offset < 13 || offset > 20) {
                offsets.push_back(offset);
                for (std::size_t bit = 0; bit < 8; ++bit) {
                    bits.push_back(offset * 8 + bit);
                }
            }
        }
        ASSERT_FALSE(offsets.empty());
        ASSERT_GT(bits.size(), 1u);
    }

    constexpr std::size_t CASE_COUNT = 256;
    std::mt19937_64 random(0x5E6D3A9017C4B28FULL);
    std::uniform_int_distribution<std::size_t> segment_distribution(
        0, original_segments.size() - 1);
    std::vector<std::size_t> cases_per_segment(original_segments.size(), 0);
    const auto previous_log_level = spdlog::get_level();
    spdlog::set_level(spdlog::level::off);

    const auto rewrite_segment = [](const std::string& path,
                                    const std::vector<char>& contents) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return false;
        }
        output.write(contents.data(),
                     static_cast<std::streamsize>(contents.size()));
        output.close();
        return static_cast<bool>(output);
    };

    std::size_t completed_cases = 0;
    for (std::size_t case_index = 0; case_index < CASE_COUNT; ++case_index) {
        const auto segment_index = segment_distribution(random);
        const auto segment_id = static_cast<uint64_t>(segment_index + 1);
        const auto segment_path =
            dir_ + "/" + WALStorage::segmentName(segment_id);
        auto mutated_segment = original_segments[segment_index];
        const auto& valid_offsets = corruption_offsets[segment_index];
        std::uniform_int_distribution<std::size_t> byte_distribution(
            0, valid_offsets.size() - 1);
        const auto byte_index = valid_offsets[byte_distribution(random)];
        std::uniform_int_distribution<unsigned int> bit_distribution(0, 7);
        const auto bit_index = bit_distribution(random);
        mutated_segment[byte_index] = static_cast<char>(
            static_cast<unsigned char>(mutated_segment[byte_index]) ^
            static_cast<unsigned char>(1U << bit_index));

        if (!rewrite_segment(segment_path, mutated_segment)) {
            ADD_FAILURE() << "Could not persist mutation case " << case_index
                          << " in segment " << segment_id;
            break;
        }

        WALStorage::RecoveryResult recovery_result;
        auto recovery = WALStorage::open(
            cfg, [](const WALStorage::Entry&) { return true; }, nullptr,
            &recovery_result);
        EXPECT_FALSE(recovery.has_value())
            << "Corruption was accepted in case " << case_index
            << " in segment " << segment_id << " at byte " << byte_index
            << ", bit " << bit_index;
        EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted)
            << "Unexpected recovery status in case " << case_index;
        EXPECT_EQ(recovery_result.segment_id, segment_id)
            << "Recovery reported the wrong corrupted segment in case "
            << case_index;
        EXPECT_EQ(recovery_result.recovered_entries, segment_index)
            << "Recovery reported the wrong valid prefix in case " << case_index;
        if (recovery.has_value()) {
            (*recovery).reset();
        }

        if (!rewrite_segment(segment_path, original_segments[segment_index])) {
            ADD_FAILURE() << "Could not restore segment " << segment_id
                          << " after mutation case " << case_index;
            break;
        }
        ++cases_per_segment[segment_index];
        ++completed_cases;
    }

    constexpr std::size_t MULTI_BIT_CASES_PER_SEGMENT = 32;
    const auto multi_bit_case_count =
        original_segments.size() * MULTI_BIT_CASES_PER_SEGMENT;
    std::vector<std::size_t> multi_bit_cases_per_segment(
        original_segments.size(), 0);
    std::size_t completed_multi_bit_cases = 0;
    for (std::size_t case_index = 0; case_index < multi_bit_case_count;
         ++case_index) {
        const auto segment_index =
            case_index % original_segments.size();
        const auto segment_id = static_cast<uint64_t>(segment_index + 1);
        const auto segment_path =
            dir_ + "/" + WALStorage::segmentName(segment_id);
        auto mutated_segment = original_segments[segment_index];
        const auto& valid_bits = corruption_bits[segment_index];
        std::uniform_int_distribution<std::size_t> bit_distribution(
            0, valid_bits.size() - 1);
        const auto first_bit = valid_bits[bit_distribution(random)];
        auto second_bit = valid_bits[bit_distribution(random)];
        while (second_bit == first_bit) {
            second_bit = valid_bits[bit_distribution(random)];
        }
        for (const auto bit_index : {first_bit, second_bit}) {
            const auto byte_index = bit_index / 8;
            const auto bit_offset = bit_index % 8;
            mutated_segment[byte_index] = static_cast<char>(
                static_cast<unsigned char>(mutated_segment[byte_index]) ^
                static_cast<unsigned char>(1U << bit_offset));
        }

        if (!rewrite_segment(segment_path, mutated_segment)) {
            ADD_FAILURE() << "Could not persist two-bit mutation case "
                          << case_index << " in segment " << segment_id;
            break;
        }

        WALStorage::RecoveryResult recovery_result;
        auto recovery = WALStorage::open(
            cfg, [](const WALStorage::Entry&) { return true; }, nullptr,
            &recovery_result);
        EXPECT_FALSE(recovery.has_value())
            << "Two-bit corruption was accepted in case " << case_index
            << " in segment " << segment_id << " at bits " << first_bit
            << " and " << second_bit;
        EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted)
            << "Unexpected recovery status in two-bit case " << case_index;
        EXPECT_EQ(recovery_result.segment_id, segment_id)
            << "Recovery reported the wrong corrupted segment in two-bit case "
            << case_index;
        EXPECT_EQ(recovery_result.recovered_entries, segment_index)
            << "Recovery reported the wrong valid prefix in two-bit case "
            << case_index;
        if (recovery.has_value()) {
            (*recovery).reset();
        }

        if (!rewrite_segment(segment_path, original_segments[segment_index])) {
            ADD_FAILURE() << "Could not restore segment " << segment_id
                          << " after two-bit mutation case " << case_index;
            break;
        }
        ++multi_bit_cases_per_segment[segment_index];
        ++completed_multi_bit_cases;
    }

    spdlog::set_level(previous_log_level);
    EXPECT_EQ(completed_cases, CASE_COUNT);
    for (std::size_t segment_index = 0;
         segment_index < cases_per_segment.size(); ++segment_index) {
        EXPECT_GT(cases_per_segment[segment_index], 0u)
            << "No mutation case exercised segment " << segment_index + 1;
        EXPECT_EQ(multi_bit_cases_per_segment[segment_index],
                  MULTI_BIT_CASES_PER_SEGMENT)
            << "Unexpected two-bit case count for segment " << segment_index + 1;
    }
    EXPECT_EQ(completed_multi_bit_cases, multi_bit_case_count);
}

TEST_F(WALStorageTest, IncompleteRecordBeforeCompleteRecord_FailsClosed) {
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
        constexpr std::streamoff FIRST_FRAME_SIZE = 25 + 5 + 5;
        f.seekp(FIRST_FRAME_SIZE + 17);
        const char oversized_value_length[] = {
            static_cast<char>(0xFF), static_cast<char>(0xFF),
            static_cast<char>(0xFF), static_cast<char>(0x7F)};
        f.write(oversized_value_length, sizeof(oversized_value_length));
        ASSERT_TRUE(f.good());
    }

    WALStorage::RecoveryResult recovery_result;
    auto recovery = WALStorage::open(
        config(), [](const WALStorage::Entry&) { return true; }, nullptr,
        &recovery_result);
    ASSERT_FALSE(recovery.has_value());
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::Corrupted);
    EXPECT_EQ(recovery_result.recovered_entries, 1u);
    EXPECT_NE(recovery_result.error_message.find(
                  "incomplete WAL record precedes another complete record"),
              std::string::npos);
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
    EXPECT_EQ(recovery_result.status, WALStorage::RecoveryStatus::TruncatedTail);
    EXPECT_EQ(recovery_result.skipped_bytes, 3u);
    EXPECT_EQ(recovery_result.bytes_processed, valid_size);
    EXPECT_EQ(recovery_result.recovered_entries, 1u);

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
    EXPECT_EQ(fs::path(recovery_result.segment).lexically_normal(),
              corrupt_path.lexically_normal());
    EXPECT_EQ(recovery_result.segment_id, 1u);
    EXPECT_FALSE(recovery_result.error_message.empty());
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
    const auto original = std::vector<char>(
        std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
    ASSERT_GT(original.size(), static_cast<size_t>(21));

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
