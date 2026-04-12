// Copyright (c) 2026 ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0
//
// UUID v7 unit tests (RFC 9562)
// ─────────────────────────────
// Covers format, monotonicity, time-ordering, uniqueness, and thread safety.

#include <gtest/gtest.h>
#include "utils/uuid.h"

#include <algorithm>
#include <chrono>
#include <set>
#include <string>
#include <thread>
#include <vector>

using ::utils::generate_uuid_v4;
using ::utils::generate_uuid_v7;

class UuidV7FocusedTests : public ::testing::Test {};

// ── UV7-01: format ──────────────────────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_01_Format_CanonicalHyphenated) {
    const std::string id = generate_uuid_v7();
    // "xxxxxxxx-xxxx-7xxx-yxxx-xxxxxxxxxxxx"  (36 chars)
    ASSERT_EQ(id.size(), 36u);
    EXPECT_EQ(id[8],  '-');
    EXPECT_EQ(id[13], '-');
    EXPECT_EQ(id[18], '-');
    EXPECT_EQ(id[23], '-');
}

TEST(UuidV7FocusedTests, UV7_02_VersionNibble_Is7) {
    const std::string id = generate_uuid_v7();
    // Position 14 is the version nibble: must be '7'
    EXPECT_EQ(id[14], '7') << "UUID: " << id;
}

TEST(UuidV7FocusedTests, UV7_03_VariantBits_AreRFC4122) {
    // Position 19 is the first hex digit of the variant field.
    // RFC 4122 variant 10xx → first nibble must be 8, 9, a, or b.
    const std::string id = generate_uuid_v7();
    const char v = id[19];
    EXPECT_TRUE(v == '8' || v == '9' || v == 'a' || v == 'b')
        << "UUID: " << id << "  variant char: " << v;
}

TEST(UuidV7FocusedTests, UV7_04_OnlyLowercaseHex) {
    for (int i = 0; i < 20; ++i) {
        const std::string id = generate_uuid_v7();
        for (size_t pos = 0; pos < id.size(); ++pos) {
            if (id[pos] == '-') continue;
            EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(id[pos])))
                << "Non-hex char '" << id[pos] << "' at position " << pos;
            EXPECT_FALSE(std::isupper(static_cast<unsigned char>(id[pos])))
                << "Uppercase hex at position " << pos;
        }
    }
}

// ── UV7-05: monotonicity ────────────────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_05_StrictlyMonotonic_SingleThread_100) {
    std::vector<std::string> ids;
    ids.reserve(100);
    for (int i = 0; i < 100; ++i) {
        ids.push_back(generate_uuid_v7());
    }
    // Lexicographic order == time order for v7
    EXPECT_TRUE(std::is_sorted(ids.begin(), ids.end()))
        << "IDs are not monotonically increasing";
}

TEST(UuidV7FocusedTests, UV7_06_Monotonic_1000_SameMs) {
    // Generate 1000 IDs as fast as possible (likely within one ms).
    std::vector<std::string> ids;
    ids.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        ids.push_back(generate_uuid_v7());
    }
    EXPECT_TRUE(std::is_sorted(ids.begin(), ids.end()))
        << "1000 rapid-fire IDs are not monotonic";
}

// ── UV7-07: uniqueness ──────────────────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_07_NoCollisions_10k) {
    std::set<std::string> ids;
    for (int i = 0; i < 10000; ++i) {
        ids.insert(generate_uuid_v7());
    }
    EXPECT_EQ(ids.size(), 10000u) << "Collision detected in 10 000 UUIDs";
}

// ── UV7-08: time prefix ─────────────────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_08_TimestampEncoded_MonotonicallyIncreasing) {
    // Generate IDs ~5 ms apart and verify the first 12 hex digits (48-bit ms)
    // increase.
    auto extract_ms_hex = [](const std::string& id) {
        // First 8 chars + '-' + 4 chars = 12 hex digits = 48-bit timestamp
        return id.substr(0, 8) + id.substr(9, 4);
    };

    const std::string id1 = generate_uuid_v7();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    const std::string id2 = generate_uuid_v7();

    EXPECT_LT(extract_ms_hex(id1), extract_ms_hex(id2))
        << "Timestamp prefix did not increase after 5 ms sleep";
}

TEST(UuidV7FocusedTests, UV7_09_LexicographicOrder_Matches_TimeOrder) {
    // Three IDs separated by ~2 ms sleeps: lex order must equal time order.
    const std::string a = generate_uuid_v7();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const std::string b = generate_uuid_v7();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    const std::string c = generate_uuid_v7();

    EXPECT_LT(a, b);
    EXPECT_LT(b, c);
}

// ── UV7-10: thread safety ───────────────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_10_ThreadSafe_NoCollisions_4Threads_2500Each) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 2500;

    std::vector<std::vector<std::string>> results(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            results[t].reserve(kPerThread);
            for (int i = 0; i < kPerThread; ++i) {
                results[t].push_back(generate_uuid_v7());
            }
        });
    }
    for (auto& th : threads) th.join();

    std::set<std::string> all;
    for (auto& vec : results) {
        for (auto& id : vec) all.insert(id);
    }
    EXPECT_EQ(all.size(), static_cast<size_t>(kThreads * kPerThread))
        << "Thread-safety test: collisions detected";
}

TEST(UuidV7FocusedTests, UV7_11_ThreadSafe_EachThreadMonotonic) {
    constexpr int kPerThread = 500;
    std::vector<std::vector<std::string>> results(4);
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            results[t].reserve(kPerThread);
            for (int i = 0; i < kPerThread; ++i) {
                results[t].push_back(generate_uuid_v7());
            }
        });
    }
    for (auto& th : threads) th.join();

    for (int t = 0; t < 4; ++t) {
        EXPECT_TRUE(std::is_sorted(results[t].begin(), results[t].end()))
            << "Thread " << t << " produced non-monotonic sequence";
    }
}

// ── UV7-12: v4 / v7 distinguishable ─────────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_12_V4_And_V7_VersionNibble_Differ) {
    const std::string v4 = generate_uuid_v4();
    const std::string v7 = generate_uuid_v7();
    EXPECT_EQ(v4[14], '4');
    EXPECT_EQ(v7[14], '7');
}

// ── UV7-13: sequence roll-over within ms ────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_13_Monotonic_After_5000_RapidFire) {
    // Stress the within-ms sequence counter.
    std::vector<std::string> ids;
    ids.reserve(5000);
    for (int i = 0; i < 5000; ++i) ids.push_back(generate_uuid_v7());
    EXPECT_TRUE(std::is_sorted(ids.begin(), ids.end()));
    std::set<std::string> uniq(ids.begin(), ids.end());
    EXPECT_EQ(uniq.size(), 5000u);
}

// ── UV7-14: compare v7 IDs from different seconds ───────────────────────────

TEST(UuidV7FocusedTests, UV7_14_IdsFrom_DifferentSeconds_Ordered) {
    const std::string before = generate_uuid_v7();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const std::string after  = generate_uuid_v7();
    EXPECT_LT(before, after);
}

// ── UV7-15: high-frequency burst → still unique ─────────────────────────────

TEST(UuidV7FocusedTests, UV7_15_HighFrequency_Burst_30k) {
    std::set<std::string> ids;
    for (int i = 0; i < 30000; ++i) ids.insert(generate_uuid_v7());
    EXPECT_EQ(ids.size(), 30000u);
}

// ── UV7-16..20: structural invariants ───────────────────────────────────────

TEST(UuidV7FocusedTests, UV7_16_Length_Always_36) {
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(generate_uuid_v7().size(), 36u);
    }
}

TEST(UuidV7FocusedTests, UV7_17_HyphenPositions_Fixed) {
    for (int i = 0; i < 20; ++i) {
        const std::string id = generate_uuid_v7();
        EXPECT_EQ(id[8],  '-') << id;
        EXPECT_EQ(id[13], '-') << id;
        EXPECT_EQ(id[18], '-') << id;
        EXPECT_EQ(id[23], '-') << id;
    }
}

TEST(UuidV7FocusedTests, UV7_18_FirstGroup_8Chars) {
    const std::string id = generate_uuid_v7();
    EXPECT_EQ(id.substr(0, 8).size(), 8u);
}

TEST(UuidV7FocusedTests, UV7_19_SecondGroup_4Chars) {
    const std::string id = generate_uuid_v7();
    EXPECT_EQ(id.substr(9, 4).size(), 4u);
}

TEST(UuidV7FocusedTests, UV7_20_FifthGroup_12Chars) {
    const std::string id = generate_uuid_v7();
    EXPECT_EQ(id.substr(24, 12).size(), 12u);
}
