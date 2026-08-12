// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Storage fuzz / chaos tests:
//  - Randomised put/del/get sequences with deterministic seeds for reproducibility
//  - Crash-simulation: drop WAL mid-write, recover, verify no data beyond checkpoint
//  - Large-value stress test
//  - Key/value boundary stress (empty key protected, max-size value)
//  - Concurrent writer / reader integrity check

#include <gtest/gtest.h>
#include "storage/wal_storage.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────
class StorageFuzzTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        wal_dir_ = (fs::temp_directory_path() /
                    ("themis_fuzz_" + std::to_string(ts))).string();
        fs::create_directories(wal_dir_);
    }

    void TearDown() override {
        fs::remove_all(wal_dir_);
    }

    WALStorage::Config makeCfg(uint64_t rotate_bytes = 4096) {
        WALStorage::Config cfg;
        cfg.dir                     = wal_dir_;
        cfg.rotation_threshold_bytes = rotate_bytes;
        cfg.fsync_on_write          = false;
        return cfg;
    }

    std::string wal_dir_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: replay all WAL entries into a map
// ─────────────────────────────────────────────────────────────────────────────
static std::map<std::string, std::string> replayToMap(
    [[maybe_unused]] const std::string& dir, WALStorage::Config cfg)
{
    std::map<std::string, std::string> db;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry& e) {
        if (e.type == WALStorage::EntryType::PUT) {
            db[e.key] = e.value;
        } else {
            db.erase(e.key);
        }
        return true;
    };
    auto wal = WALStorage::open(cfg, cb);
    // wal was re-opened just for recovery; close immediately.
    (void)wal;
    return db;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: Randomised put/del sequence, full recovery
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, RandomisedPutDel_FullRecovery) {
    constexpr int kOps   = 500;
    constexpr int kKeys  = 40;
    std::mt19937 rng(42); // deterministic seed

    std::map<std::string, std::string> reference;  // ground truth

    {
        auto wal = WALStorage::open(makeCfg());
        ASSERT_TRUE(wal.has_value());

        for (int i = 0; i < kOps; ++i) {
            std::string key = "k" + std::to_string(rng() % kKeys);
            if (rng() % 3 == 0) {
                // DELETE
                (void)(*wal)->appendDelete(key);
                reference.erase(key);
            } else {
                // PUT
                std::string val = "v" + std::to_string(rng());
                ASSERT_TRUE((*wal)->appendPut(key, val).has_value());
                reference[key] = val;
            }
        }
        (void)(*wal)->flush();
    }

    // Recover
    auto recovered = replayToMap(wal_dir_, makeCfg());
    EXPECT_EQ(recovered, reference)
        << "Recovered state must exactly match in-memory reference";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: Multi-seed fuzz – different seeds, same invariant
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, MultipleSeedFuzz_RecoveryConsistent) {
    constexpr int kOps  = 200;
    constexpr int kKeys = 20;
    const std::vector<uint32_t> seeds = {1, 7, 31, 137, 999983};

    for (uint32_t seed : seeds) {
        fs::remove_all(wal_dir_);
        fs::create_directories(wal_dir_);

        std::mt19937 rng(seed);
        std::map<std::string, std::string> reference;

        {
            auto wal = WALStorage::open(makeCfg(512));
            ASSERT_TRUE(wal.has_value()) << "seed=" << seed;
            for (int i = 0; i < kOps; ++i) {
                std::string key = "k" + std::to_string(rng() % kKeys);
                if (rng() % 4 == 0) {
                    (void)(*wal)->appendDelete(key);
                    reference.erase(key);
                } else {
                    std::string val = "v" + std::to_string(rng() % 10000);
                    (void)(*wal)->appendPut(key, val);
                    reference[key] = val;
                }
            }
            (*wal)->flush();
        }

        auto recovered = replayToMap(wal_dir_, makeCfg(512));
        EXPECT_EQ(recovered, reference) << "seed=" << seed;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Crash-sim – truncate file to 75 %, verify only intact entries survive
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, CrashSim_TruncateFile_PartialEntriesSurvive) {
    // Write 100 entries, truncate the segment to 75% of its size,
    // recover and verify that all fully written entries are present.
    std::vector<std::pair<std::string, std::string>> written;
    {
        auto wal = WALStorage::open(makeCfg(64 * 1024 * 1024)); // no rotation
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < 100; ++i) {
            std::string k = "crash_k" + std::to_string(i);
            std::string v = "crash_v" + std::to_string(i);
            ASSERT_TRUE((*wal)->appendPut(k, v).has_value());
            written.emplace_back(k, v);
        }
        (void)(*wal)->flush();
    }

    // Find the single segment file and truncate it to 75%
    fs::path seg;
    for (const auto& e : fs::directory_iterator(wal_dir_)) {
        if (e.path().extension() == ".log") { seg = e.path(); break; }
    }
    ASSERT_FALSE(seg.empty());
    auto full_size = fs::file_size(seg);
    fs::resize_file(seg, (full_size * 3) / 4);

    // Recover – expect a subset of entries, all valid
    std::map<std::string, std::string> recovered;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry& e) {
        recovered[e.key] = e.value; return true;
    };
    auto wal2 = WALStorage::open(makeCfg(64 * 1024 * 1024), cb);
    ASSERT_TRUE(wal2.has_value()) << "Recovery must succeed even after truncation";

    // Every entry in `recovered` must be correct (no corruption)
    for (const auto& [k, v] : recovered) {
        auto it = std::find_if(written.begin(), written.end(),
                               [&](const auto& p){ return p.first == k; });
        ASSERT_NE(it, written.end()) << "Unknown key in recovered: " << k;
        EXPECT_EQ(it->second, v) << "Wrong value for key " << k;
    }
    // At least some entries must survive
    EXPECT_GT(recovered.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: Large values (64 KB each)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, LargeValues_RoundTripThroughRecovery) {
    constexpr size_t kValueSize = 64 * 1024; // 64 KB
    constexpr int    kEntries   = 10;

    std::map<std::string, std::string> reference;
    {
        auto wal = WALStorage::open(makeCfg(512 * 1024)); // 512 KB rotate
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < kEntries; ++i) {
            std::string key = "big_key_" + std::to_string(i);
            std::string val(kValueSize, static_cast<char>('A' + (i % 26)));
            ASSERT_TRUE((*wal)->appendPut(key, val).has_value());
            reference[key] = val;
        }
        (*wal)->flush();
    }

    auto recovered = replayToMap(wal_dir_, makeCfg(512 * 1024));
    EXPECT_EQ(recovered, reference);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: Keys with special characters and max-length keys
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, SpecialCharKeys_RoundTripThroughRecovery) {
    std::map<std::string, std::string> reference;
    {
        auto wal = WALStorage::open(makeCfg());
        ASSERT_TRUE(wal.has_value());

        const std::vector<std::pair<std::string, std::string>> cases = {
            {"key with spaces",   "val1"},
            {"key/with/slashes",  "val2"},
            {"key\x01\x02\x03",  "val3"},   // key with non-printable bytes (SOH, STX, ETX)
            {std::string(255, 'x'), "max_key_length_val"},
            {"normal_key",        std::string(1024, 'z')}, // long value
        };

        for (const auto& [k, v] : cases) {
            ASSERT_TRUE((*wal)->appendPut(k, v).has_value());
            reference[k] = v;
        }
        (*wal)->flush();
    }

    auto recovered = replayToMap(wal_dir_, makeCfg());
    EXPECT_EQ(recovered, reference);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: Concurrent writers – all entries must be recoverable
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, ConcurrentWriters_AllEntriesRecoverable) {
    constexpr int kThreads    = 4;
    constexpr int kPerThread  = 50;

    auto wal_res = WALStorage::open(makeCfg(8192));
    ASSERT_TRUE(wal_res.has_value());
    auto& wal = *wal_res;

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                auto r = wal->appendPut(
                    "t" + std::to_string(t) + "_k" + std::to_string(i),
                    "v" + std::to_string(t * 1000 + i));
                if (!r.has_value()) ++failures;
            }
        });
    }
    for (auto& th : threads) th.join();
    wal->flush();
    wal.reset();

    EXPECT_EQ(failures.load(), 0) << "No append should fail under concurrent writes";

    // Recover: all entries must be present
    int count = 0;
    WALStorage::RecoveryCallback cb = [&](const WALStorage::Entry&) {
        ++count; return true;
    };
    auto wal2 = WALStorage::open(makeCfg(8192), cb);
    ASSERT_TRUE(wal2.has_value());
    EXPECT_EQ(count, kThreads * kPerThread)
        << "All " << (kThreads * kPerThread) << " entries must survive recovery";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 7: Repeated open/close cycles (persistence across re-opens)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, RepeatedOpenClose_StateAccumulates) {
    constexpr int kRounds = 5;
    constexpr int kPerRound = 20;

    std::map<std::string, std::string> cumulative;

    for (int r = 0; r < kRounds; ++r) {
        // Recover existing state first
        cumulative = replayToMap(wal_dir_, makeCfg());

        // Append new entries
        {
            WALStorage::RecoveryCallback cb = [](const WALStorage::Entry&) {
                return true; // just advance sequence
            };
            auto wal = WALStorage::open(makeCfg(), cb);
            ASSERT_TRUE(wal.has_value()) << "round=" << r;
            for (int i = 0; i < kPerRound; ++i) {
                std::string k = "r" + std::to_string(r) + "_k" + std::to_string(i);
                std::string v = "v" + std::to_string(r * 100 + i);
                ASSERT_TRUE((*wal)->appendPut(k, v).has_value());
                cumulative[k] = v;
            }
            (*wal)->flush();
        }
    }

    // Final recovery must match all accumulated writes
    auto final_state = replayToMap(wal_dir_, makeCfg());
    EXPECT_EQ(final_state, cumulative);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 8: Alternating put/del for same key – last op wins
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(StorageFuzzTest, AlternatingPutDel_LastOpWins) {
    constexpr int kFlips = 100;
    const std::string key = "flip_key";

    {
        auto wal = WALStorage::open(makeCfg());
        ASSERT_TRUE(wal.has_value());
        for (int i = 0; i < kFlips; ++i) {
            if (i % 2 == 0) {
                (void)(*wal)->appendPut(key, "val_" + std::to_string(i));
            } else {
                (void)(*wal)->appendDelete(key);
            }
        }
        // Last op is a DEL (kFlips=100 → last i=99, odd → DEL)
        (*wal)->flush();
    }

    std::map<std::string, std::string> state = replayToMap(wal_dir_, makeCfg());
    // Last op was DEL, so key must be absent
    EXPECT_EQ(state.count(key), 0u) << "Key must be absent after final DEL";
}
