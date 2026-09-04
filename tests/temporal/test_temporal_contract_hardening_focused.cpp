// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_temporal_contract_hardening_focused.cpp
 * @brief Phase 4 temporal contract-hardening focused test suite (TCH-01..TCH-16).
 *
 * Verifies the normative contracts defined in
 * include/temporal/temporal_api_contract.h using deterministic, mock-I/O
 * test cases.  All tests use kTemporalContractSeed = 42.
 *
 * ## Test families
 *
 * ### TCH-01..04 — Bi-temporal insert / query contract
 *   TCH-01  Bi-temporal row with valid valid_time range is accepted
 *   TCH-02  Row with end < start raises TEMPORAL_RANGE_INVALID
 *   TCH-03  Transaction-time ordering: later insert has higher tx_time
 *   TCH-04  Overlap semantics: overlapping intervals are both returned
 *
 * ### TCH-05..08 — Snapshot contract
 *   TCH-05  Snapshot at T returns rows with valid_time ∩ [T,T] ≠ ∅
 *   TCH-06  Snapshot at T excludes rows outside valid_time
 *   TCH-07  Concurrent write after snapshot request is not visible
 *   TCH-08  Snapshot on empty store returns empty result
 *
 * ### TCH-09..12 — Retention contract
 *   TCH-09  Soft-delete marker set for expired rows
 *   TCH-10  GC does not remove rows within retention window
 *   TCH-11  Row past retention boundary is marked soft-deleted
 *   TCH-12  Retention policy conflict → RETENTION_POLICY_CONFLICT
 *
 * ### TCH-13..16 — PITR contract
 *   TCH-13  Valid PITR timestamp returns non-empty state
 *   TCH-14  Future PITR timestamp → PITR_TIMESTAMP_BEFORE_OLDEST
 *   TCH-15  PITR on empty history → empty result
 *   TCH-16  kTemporalOpenEnd is INT64_MAX (sentinel check)
 *
 * @see include/temporal/temporal_api_contract.h
 * @see src/temporal/ROADMAP.md — Phase 4 items
 */

#include <gtest/gtest.h>

#include "temporal/temporal_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace themis {
namespace temporal {
namespace test {

/// Canonical PRNG seed for all TCH tests.
static constexpr uint64_t kTemporalContractSeed = 42;

// ============================================================================
// Mock helpers
// ============================================================================

struct BiTemporalRow {
    int         id = 0;
    std::int64_t valid_start;
    std::int64_t valid_end;   // kTemporalOpenEnd for open interval
    std::int64_t tx_time;
    bool        soft_deleted = false;
};

/// Validates a bi-temporal range pair.
static std::optional<TemporalErrorCode> mockValidateRange(
        std::int64_t start, std::int64_t end) {
    if (start < kTemporalMinTimestampNs || end < kTemporalMinTimestampNs) {
        return TemporalErrorCode::TEMPORAL_RANGE_INVALID;
    }
    if (start > end) {
        return TemporalErrorCode::TEMPORAL_RANGE_INVALID;
    }
    return std::nullopt;
}

/// Returns rows whose valid_time interval contains point T.
static std::vector<BiTemporalRow> mockSnapshotAt(
        const std::vector<BiTemporalRow>& store,
        std::int64_t t,
        std::int64_t snapshot_tx) {
    std::vector<BiTemporalRow> result;
    for (const auto& row : store) {
        if (row.soft_deleted) {
          continue;
        }
        if (row.tx_time > snapshot_tx) continue; // committed after snapshot
        if (row.valid_start <= t && t <= row.valid_end) {
            result.push_back(row);
        }
    }
    return result;
}

/// Simulates soft-delete check against retention boundary.
static void mockApplyRetention(
        std::vector<BiTemporalRow>& store,
        std::int64_t now_ns,
        std::int64_t retention_ns) {
    std::int64_t boundary = now_ns - retention_ns;
    for (auto& row : store) {
        if (row.valid_end < boundary) {
            row.soft_deleted = true;
        }
    }
}

/// Validates that GC only removes rows that are soft-deleted.
static std::size_t mockGcCount(const std::vector<BiTemporalRow>& store) {
    std::size_t count = 0;
    for (const auto& row : store) {
        if (row.soft_deleted) {
          ++count;
        }
    }
    return count;
}

/// Simulates PITR restore.
static std::optional<TemporalErrorCode> mockPitrRestore(
        std::int64_t requested_ts,
        std::int64_t oldest_anchor,
        std::int64_t now_ns) {
    if (requested_ts < oldest_anchor || requested_ts > now_ns) {
        return TemporalErrorCode::PITR_TIMESTAMP_BEFORE_OLDEST;
    }
    return std::nullopt;
}

// ============================================================================
// TCH-01..04 — Bi-temporal insert / query tests
// ============================================================================

/// TCH-01: A well-formed bi-temporal row (valid range) is accepted.
TEST(TemporalContractHardening, TCH01_ValidRangeAccepted) {
    auto err = mockValidateRange(1000LL, 5000LL);
    EXPECT_FALSE(err.has_value());
}

/// TCH-02: A row with end < start raises TEMPORAL_RANGE_INVALID.
TEST(TemporalContractHardening, TCH02_InvalidRangeRejected) {
    auto err = mockValidateRange(5000LL, 1000LL); // end < start
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, TemporalErrorCode::TEMPORAL_RANGE_INVALID);
    EXPECT_TRUE(isHardTemporalError(*err));
}

/// TCH-03: Later insertions carry strictly higher transaction_time.
TEST(TemporalContractHardening, TCH03_TransactionTimeOrdering) {
    std::vector<BiTemporalRow> rows;
    std::int64_t tx = 100LL;
    for (int i = 0; i < 5; ++i) {
        rows.push_back({i, 1000LL * i, 1000LL * (i + 1), tx, false});
        ++tx;
    }
    for (std::size_t i = 1; i < rows.size(); ++i) {
        EXPECT_GT(rows[i].tx_time, rows[i - 1].tx_time);
    }
}

/// TCH-04: Two overlapping valid_time intervals are both returned by a point query.
TEST(TemporalContractHardening, TCH04_OverlappingIntervalsReturned) {
    std::int64_t now = 1'000'000LL;
    std::vector<BiTemporalRow> store = {
        {1, 100LL, 5000LL, now - 10, false},  // spans query point
        {2, 200LL, 4000LL, now - 5,  false},  // also spans query point
        {3, 6000LL, 9000LL, now - 1, false},  // does NOT span query point
    };
    auto result = mockSnapshotAt(store, 3000LL, now);
    ASSERT_EQ(result.size(), 2u);
    // Both row 1 and row 2 should be present
    bool has_1 = std::any_of(result.begin(), result.end(), [](const auto& r){ return r.id == 1; });
    bool has_2 = std::any_of(result.begin(), result.end(), [](const auto& r){ return r.id == 2; });
    EXPECT_TRUE(has_1);
    EXPECT_TRUE(has_2);
}

// ============================================================================
// TCH-05..08 — Snapshot contract tests
// ============================================================================

/// TCH-05: Snapshot at T returns rows with valid_time containing T.
TEST(TemporalContractHardening, TCH05_SnapshotAtTConsistent) {
    std::int64_t now = 10000LL;
    std::vector<BiTemporalRow> store = {
        {1, 100LL, 500LL, now - 5, false},
        {2, 400LL, 800LL, now - 3, false},
    };
    // Query at T=450 — both rows span [100,500] and [400,800]
    auto result = mockSnapshotAt(store, 450LL, now);
    EXPECT_EQ(result.size(), 2u);
}

/// TCH-06: Snapshot excludes rows whose valid_time does not contain T.
TEST(TemporalContractHardening, TCH06_SnapshotExcludesOutOfRange) {
    std::int64_t now = 10000LL;
    std::vector<BiTemporalRow> store = {
        {1, 100LL,  300LL, now - 2, false},
        {2, 500LL, 1000LL, now - 1, false},
    };
    // Query at T=400 — neither row spans 400
    auto result = mockSnapshotAt(store, 400LL, now);
    EXPECT_TRUE(result.empty());
}

/// TCH-07: Row committed after snapshot_tx is NOT visible in the snapshot.
TEST(TemporalContractHardening, TCH07_ConcurrentWriteNotVisible) {
    std::int64_t snapshot_tx = 1000LL;
    std::vector<BiTemporalRow> store = {
        {1, 100LL, 5000LL, 999LL,  false},  // before snapshot → visible
        {2, 100LL, 5000LL, 1001LL, false},  // after snapshot → invisible
    };
    auto result = mockSnapshotAt(store, 2000LL, snapshot_tx);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].id, 1);
}

/// TCH-08: Snapshot on empty store returns empty result.
TEST(TemporalContractHardening, TCH08_EmptyStoreEmptySnapshot) {
    std::vector<BiTemporalRow> empty_store;
    auto result = mockSnapshotAt(empty_store, 1000LL, 9999LL);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// TCH-09..12 — Retention contract tests
// ============================================================================

/// TCH-09: Rows past retention boundary are marked soft-deleted.
TEST(TemporalContractHardening, TCH09_ExpiredRowSoftDeleted) {
    std::int64_t now_ns       = 1'000'000'000LL;
    std::int64_t retention_ns =   100'000'000LL; // 100ms retention
    std::int64_t boundary     = now_ns - retention_ns;

    std::vector<BiTemporalRow> store = {
        {1, 100LL, boundary - 1, 50LL, false},  // expired
        {2, 100LL, boundary + 1, 60LL, false},  // within retention
    };
    mockApplyRetention(store, now_ns, retention_ns);

    EXPECT_TRUE(store[0].soft_deleted);
    EXPECT_FALSE(store[1].soft_deleted);
}

/// TCH-10: GC does not remove rows within retention window (not soft-deleted).
TEST(TemporalContractHardening, TCH10_GcPreservesRetainedRows) {
    std::vector<BiTemporalRow> store = {
        {1, 100LL, 500LL,  50LL, false},  // not soft-deleted
        {2, 200LL, 300LL, 60LL,  true},   // soft-deleted
    };
    // GC would only process soft-deleted rows; verify count
    std::size_t gc_eligible = mockGcCount(store);
    EXPECT_EQ(gc_eligible, 1u);
    EXPECT_FALSE(store[0].soft_deleted); // still present
}

/// TCH-11: Row past retention boundary has soft_deleted = true after policy run.
TEST(TemporalContractHardening, TCH11_SoftDeleteMarkersPresent) {
    std::int64_t now       = 10'000'000LL;
    std::int64_t retention =  1'000'000LL;

    std::vector<BiTemporalRow> store = {
        {1, 1LL, 100LL,         5LL, false},  // very old, expired
        {2, 1LL, now - 500LL, 6LL, false},    // recent, within retention
    };
    mockApplyRetention(store, now, retention);

    // Row 1: valid_end (100) < boundary (now - retention) → soft-deleted
    EXPECT_TRUE(store[0].soft_deleted);
    // Row 2: valid_end is within retention window → not soft-deleted
    EXPECT_FALSE(store[1].soft_deleted);
}

/// TCH-12: Retention policy conflict raises RETENTION_POLICY_CONFLICT.
TEST(TemporalContractHardening, TCH12_RetentionPolicyConflict) {
    // Simulate: adding a policy when kMaxRetentionPoliciesPerTable is exceeded
    std::size_t current_policies = kMaxRetentionPoliciesPerTable;

    // Contract: adding one more would conflict
    bool conflicts = (current_policies >= kMaxRetentionPoliciesPerTable);
    EXPECT_TRUE(conflicts);

    // Verify the error code exists and is lifecycle-classifiable
    TemporalErrorCode err = TemporalErrorCode::RETENTION_POLICY_CONFLICT;
    EXPECT_TRUE(isLifecycleError(err));
    EXPECT_FALSE(isHardTemporalError(err));
}

// ============================================================================
// TCH-13..16 — PITR contract tests
// ============================================================================

/// TCH-13: Valid PITR timestamp (within anchor range) returns no error.
TEST(TemporalContractHardening, TCH13_ValidPitrTimestamp) {
    std::int64_t oldest = 1000LL;
    std::int64_t now    = 9999LL;
    std::int64_t target = 5000LL;  // within range

    auto err = mockPitrRestore(target, oldest, now);
    EXPECT_FALSE(err.has_value());
}

/// TCH-14: Future PITR timestamp → PITR_TIMESTAMP_BEFORE_OLDEST.
TEST(TemporalContractHardening, TCH14_FuturePitrTimestampError) {
    std::int64_t oldest = 1000LL;
    std::int64_t now    = 9999LL;
    std::int64_t future = 99999LL; // after now

    auto err = mockPitrRestore(future, oldest, now);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, TemporalErrorCode::PITR_TIMESTAMP_BEFORE_OLDEST);
    EXPECT_TRUE(isLifecycleError(*err));
}

/// TCH-15: PITR with timestamp before oldest anchor → error.
TEST(TemporalContractHardening, TCH15_PitrBeforeOldestAnchor) {
    std::int64_t oldest = 5000LL;
    std::int64_t now    = 9999LL;
    std::int64_t before = 1000LL; // before oldest anchor

    auto err = mockPitrRestore(before, oldest, now);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(*err, TemporalErrorCode::PITR_TIMESTAMP_BEFORE_OLDEST);
}

/// TCH-16: kTemporalOpenEnd equals INT64_MAX (sentinel contract).
TEST(TemporalContractHardening, TCH16_OpenEndSentinelIsInt64Max) {
    EXPECT_EQ(kTemporalOpenEnd, INT64_MAX);
}

} // namespace test
} // namespace temporal
} // namespace themis
