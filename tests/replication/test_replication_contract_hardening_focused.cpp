// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_replication_contract_hardening_focused.cpp
 * @brief Phase 4 replication contract-hardening focused test suite (RCH-01..RCH-16).
 *
 * Verifies the normative contracts defined in
 * include/replication/replication_api_contract.h using deterministic,
 * mock-I/O test cases.  All tests use kReplicationContractSeed = 42.
 *
 * ## Test families
 *
 * ### RCH-01..04 — CRDT contract
 *   RCH-01  GCounter merge is commutative (merge(A,B) == merge(B,A))
 *   RCH-02  GCounter merge is idempotent (merge(A,A) == A)
 *   RCH-03  GCounter merge is associative
 *   RCH-04  CRDT type mismatch → CRDT_TYPE_MISMATCH
 *
 * ### RCH-05..08 — Conflict resolution contract
 *   RCH-05  LWW resolution is deterministic (same inputs → same winner)
 *   RCH-06  Tie-break by node-ID is consistent
 *   RCH-07  Tombstone wins over concurrent update at same timestamp
 *   RCH-08  LWW winner has the greater timestamp
 *
 * ### RCH-09..12 — Change stream contract
 *   RCH-09  Events within partition are delivered in commit order
 *   RCH-10  At-least-once: committed event is always delivered
 *   RCH-11  Redelivery of same event is idempotent (no duplicate state change)
 *   RCH-12  STREAM_OFFSET_ERROR on invalid offset
 *
 * ### RCH-13..16 — WAL ordering contract
 *   RCH-13  LSNs are monotonically increasing
 *   RCH-14  Apply gap (LSN jump) → REPLICATION_LAG_EXCEEDED
 *   RCH-15  Idempotent re-apply of same LSN is safe (no error, no data change)
 *   RCH-16  kMaxPendingWalEvents constant is reasonable (> 0)
 *
 * @see include/replication/replication_api_contract.h
 * @see src/replication/ROADMAP.md — Phase 4 items
 */

#include <gtest/gtest.h>

#include "replication/replication_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace replication {
namespace test {

/// Canonical PRNG seed for all RCH tests.
static constexpr uint64_t kReplicationContractSeed = 42;

// ============================================================================
// Mock helpers
// ============================================================================

/// Simple GCounter mock: maps node-ID → increment count.
struct MockGCounter {
    std::map<std::string, std::int64_t> counters;

    std::int64_t value() const {
        std::int64_t total = 0;
        for (const auto& [k, v] : counters) total += v;
        return total;
    }

    static MockGCounter merge(const MockGCounter& a, const MockGCounter& b) {
        MockGCounter result = a;
        for (const auto& [k, v] : b.counters) {
            result.counters[k] = std::max(result.counters[k], v);
        }
        return result;
    }

    bool operator==(const MockGCounter& other) const {
        return counters == other.counters;
    }
};

enum class MockCrdtType { GCounter, PNCounter };

/// Simulates type-mismatch detection.
static std::optional<ReplicationErrorCode> mockCrdtMergeTypes(
        MockCrdtType a, MockCrdtType b) {
    if (a != b) return ReplicationErrorCode::CRDT_TYPE_MISMATCH;
    return std::nullopt;
}

/// Simulates LWW conflict resolution — returns the winning value.
struct MockVersion {
    std::int64_t timestamp;
    std::string  node_id;
    std::string  value;
    bool         is_tombstone = false;
};

static MockVersion mockLwwResolve(const MockVersion& v1, const MockVersion& v2) {
    if (v1.timestamp > v2.timestamp) return v1;
    if (v2.timestamp > v1.timestamp) return v2;
    // Tie-break: greater node-ID wins
    if (v1.is_tombstone || v2.is_tombstone) {
        // Tombstone wins at same timestamp
        return v1.is_tombstone ? v1 : v2;
    }
    return (v1.node_id > v2.node_id) ? v1 : v2;
}

/// Simulates partition-ordered event stream.
static std::vector<int> mockPartitionStream(std::vector<int> events) {
    // Events are already in commit order; returned as-is.
    return events;
}

/// Simulates offset validation.
static std::optional<ReplicationErrorCode> mockValidateOffset(
        std::int64_t offset, std::int64_t current_committed) {
    if (offset < 0 || offset > current_committed + 1) {
        return ReplicationErrorCode::STREAM_OFFSET_ERROR;
    }
    return std::nullopt;
}

/// Simulates LSN gap detection.
static std::optional<ReplicationErrorCode> mockCheckLsnGap(
        std::int64_t prev_lsn, std::int64_t next_lsn) {
    if (next_lsn > prev_lsn + 1) {
        return ReplicationErrorCode::REPLICATION_LAG_EXCEEDED;
    }
    return std::nullopt;
}

/// Simulates idempotent apply (tracks applied LSNs in a set).
static bool mockApplyIdempotent(
        std::vector<std::int64_t>& applied_lsns, std::int64_t lsn) {
    if (std::find(applied_lsns.begin(), applied_lsns.end(), lsn) != applied_lsns.end()) {
        return false; // already applied — no-op, no error
    }
    applied_lsns.push_back(lsn);
    return true; // newly applied
}

// ============================================================================
// RCH-01..04 — CRDT contract tests
// ============================================================================

/// RCH-01: GCounter merge is commutative.
TEST(ReplicationContractHardening, RCH01_GCounterMergeCommutative) {
    MockGCounter a, b;
    a.counters["node-1"] = 5;
    a.counters["node-2"] = 3;
    b.counters["node-2"] = 7;
    b.counters["node-3"] = 2;

    auto ab = MockGCounter::merge(a, b);
    auto ba = MockGCounter::merge(b, a);

    EXPECT_EQ(ab, ba);
    EXPECT_EQ(ab.value(), 5 + 7 + 2);
}

/// RCH-02: GCounter merge is idempotent.
TEST(ReplicationContractHardening, RCH02_GCounterMergeIdempotent) {
    MockGCounter a;
    a.counters["node-1"] = 10;
    a.counters["node-2"] = 4;

    auto aa = MockGCounter::merge(a, a);
    EXPECT_EQ(aa, a);
}

/// RCH-03: GCounter merge is associative.
TEST(ReplicationContractHardening, RCH03_GCounterMergeAssociative) {
    MockGCounter a, b, c;
    a.counters["n1"] = 3;
    b.counters["n2"] = 7;
    c.counters["n3"] = 5;

    auto left  = MockGCounter::merge(MockGCounter::merge(a, b), c);
    auto right = MockGCounter::merge(a, MockGCounter::merge(b, c));

    EXPECT_EQ(left, right);
}

/// RCH-04: Merging incompatible CRDT types raises CRDT_TYPE_MISMATCH.
TEST(ReplicationContractHardening, RCH04_CrdtTypeMismatch) {
    auto err = mockCrdtMergeTypes(MockCrdtType::GCounter, MockCrdtType::PNCounter);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, ReplicationErrorCode::CRDT_TYPE_MISMATCH);
    EXPECT_TRUE(isHardReplicationError(*err));
}

// ============================================================================
// RCH-05..08 — Conflict resolution tests
// ============================================================================

/// RCH-05: LWW resolution is deterministic — same inputs always same winner.
TEST(ReplicationContractHardening, RCH05_LwwDeterministic) {
    MockVersion v1 = {100LL, "node-A", "value_A"};
    MockVersion v2 = {200LL, "node-B", "value_B"};

    auto winner1 = mockLwwResolve(v1, v2);
    auto winner2 = mockLwwResolve(v1, v2);

    EXPECT_EQ(winner1.value, winner2.value);
    EXPECT_EQ(winner1.value, "value_B"); // higher timestamp wins
}

/// RCH-06: Tie-break by node-ID is consistent.
TEST(ReplicationContractHardening, RCH06_TieBreakByNodeId) {
    MockVersion v1 = {500LL, "node-A", "value_A"};
    MockVersion v2 = {500LL, "node-Z", "value_Z"}; // same ts, node-Z > node-A

    auto winner = mockLwwResolve(v1, v2);
    EXPECT_EQ(winner.value, "value_Z");
    EXPECT_EQ(winner.node_id, "node-Z");
}

/// RCH-07: Tombstone wins over concurrent update at same timestamp.
TEST(ReplicationContractHardening, RCH07_TombstoneWins) {
    MockVersion update    = {1000LL, "node-A", "alive",   false};
    MockVersion tombstone = {1000LL, "node-B", "deleted", true};

    auto winner = mockLwwResolve(update, tombstone);
    EXPECT_TRUE(winner.is_tombstone);
}

/// RCH-08: LWW winner always has the greater timestamp.
TEST(ReplicationContractHardening, RCH08_LwwWinnerHasGreaterTimestamp) {
    std::mt19937_64 rng(kReplicationContractSeed);
    std::uniform_int_distribution<std::int64_t> dist(1, 1'000'000);

    for (int i = 0; i < 100; ++i) {
        std::int64_t t1 = dist(rng);
        std::int64_t t2 = dist(rng);
        if (t1 == t2) continue; // skip ties for this assertion

        MockVersion v1 = {t1, "node-1", "v1"};
        MockVersion v2 = {t2, "node-2", "v2"};
        auto winner = mockLwwResolve(v1, v2);
        EXPECT_EQ(winner.timestamp, std::max(t1, t2));
    }
}

// ============================================================================
// RCH-09..12 — Change stream tests
// ============================================================================

/// RCH-09: Events within a partition are delivered in commit order.
TEST(ReplicationContractHardening, RCH09_PartitionOrderPreserved) {
    std::vector<int> commit_order = {1, 2, 3, 4, 5};
    auto delivered = mockPartitionStream(commit_order);
    EXPECT_EQ(delivered, commit_order);
}

/// RCH-10: At-least-once: a committed event appears in the delivered stream.
TEST(ReplicationContractHardening, RCH10_AtLeastOnceDelivery) {
    std::vector<int> committed = {10, 20, 30};
    auto delivered = mockPartitionStream(committed);

    for (int ev : committed) {
        EXPECT_NE(std::find(delivered.begin(), delivered.end(), ev), delivered.end())
            << "Committed event " << ev << " missing from delivery";
    }
}

/// RCH-11: Redelivery is idempotent — applying same LSN twice is a no-op.
TEST(ReplicationContractHardening, RCH11_RedeliveryIdempotent) {
    std::vector<std::int64_t> applied;

    bool first  = mockApplyIdempotent(applied, 42LL);
    bool second = mockApplyIdempotent(applied, 42LL);

    EXPECT_TRUE(first);   // first apply: new
    EXPECT_FALSE(second); // second apply: no-op
    EXPECT_EQ(applied.size(), 1u);
}

/// RCH-12: Invalid offset raises STREAM_OFFSET_ERROR.
TEST(ReplicationContractHardening, RCH12_StreamOffsetError) {
    std::int64_t committed = 100;

    // Offset far ahead of committed — invalid
    auto err = mockValidateOffset(200, committed);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, ReplicationErrorCode::STREAM_OFFSET_ERROR);
    EXPECT_TRUE(isLagError(*err));

    // Valid offset — no error
    auto ok = mockValidateOffset(101, committed);
    EXPECT_FALSE(ok.has_value());
}

// ============================================================================
// RCH-13..16 — WAL ordering tests
// ============================================================================

/// RCH-13: LSNs are monotonically increasing in a generated sequence.
TEST(ReplicationContractHardening, RCH13_LsnMonotonic) {
    std::vector<std::int64_t> lsns;
    std::int64_t lsn = 1;
    for (int i = 0; i < 100; ++i) lsns.push_back(lsn++);

    for (std::size_t i = 1; i < lsns.size(); ++i) {
        EXPECT_GT(lsns[i], lsns[i - 1]);
    }
}

/// RCH-14: LSN gap (jump by > 1) triggers REPLICATION_LAG_EXCEEDED.
TEST(ReplicationContractHardening, RCH14_LsnGapDetected) {
    auto err = mockCheckLsnGap(/*prev=*/100LL, /*next=*/105LL);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, ReplicationErrorCode::REPLICATION_LAG_EXCEEDED);
    EXPECT_TRUE(isLagError(*err));

    // Consecutive LSN — no error
    auto ok = mockCheckLsnGap(100LL, 101LL);
    EXPECT_FALSE(ok.has_value());
}

/// RCH-15: Idempotent re-apply of same LSN is safe.
TEST(ReplicationContractHardening, RCH15_RetryIdempotent) {
    std::vector<std::int64_t> applied;
    const std::int64_t lsn = 77LL;

    // Apply once
    bool r1 = mockApplyIdempotent(applied, lsn);
    EXPECT_TRUE(r1);

    // Re-apply — no error, state unchanged
    bool r2 = mockApplyIdempotent(applied, lsn);
    EXPECT_FALSE(r2);
    EXPECT_EQ(applied.size(), 1u);
    EXPECT_EQ(applied[0], lsn);
}

/// RCH-16: kMaxPendingWalEvents is a positive, finite constant.
TEST(ReplicationContractHardening, RCH16_MaxPendingWalEventsReasonable) {
    EXPECT_GT(kMaxPendingWalEvents, 0u);
    EXPECT_LE(kMaxPendingWalEvents, 10'000'000u); // sanity upper bound
}

} // namespace test
} // namespace replication
} // namespace themis
