// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_ingestion_contract_hardening_focused.cpp
 * @brief Phase 4 — Ingestion contract hardening focused tests (INCH-01..INCH-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed is
 * kIngestionContractSeed = 42.
 *
 * ## Test families
 *
 * ### INCH-01..04 — Write-path durability contract
 *   INCH-01  Ack implies durability: no silent drop after ack
 *   INCH-02  Buffer full → INGESTION_BUFFER_FULL signal (not silent block)
 *   INCH-03  INGESTION_BUFFER_FULL is a back-off error (isBackOffCode = true)
 *   INCH-04  Back-pressure signal delivered within kBackPressureSignalDeadline
 *
 * ### INCH-05..08 — Batch atomicity contract
 *   INCH-05  Partial batch not visible before commit
 *   INCH-06  Batch commit preserves row ordering
 *   INCH-07  Rollback on partial failure — no rows visible
 *   INCH-08  Batch size ≤ kMaxBatchSize enforced
 *
 * ### INCH-09..12 — Schema validation contract
 *   INCH-09  Valid schema → write accepted and acked
 *   INCH-10  Invalid schema → INGESTION_SCHEMA_INVALID before ack
 *   INCH-11  Schema validation happens before any data is written
 *   INCH-12  INGESTION_SCHEMA_INVALID is a permanent rejection (not retryable)
 *
 * ### INCH-13..16 — Back-pressure and quota
 *   INCH-13  Producer back-pressure signal is explicit (not silent)
 *   INCH-14  Quota exceeded → INGESTION_QUOTA_EXCEEDED
 *   INCH-15  INGESTION_QUOTA_EXCEEDED is a back-off error
 *   INCH-16  Strong-ack durability levels identified correctly
 *
 * @see include/ingestion/ingestion_api_contract.h
 * @see src/ingestion/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "ingestion/ingestion_api_contract.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace themis::ingestion;
using namespace std::chrono_literals;

namespace {

static constexpr uint64_t kIngestionContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock row
// ---------------------------------------------------------------------------
struct MockRow {
    int         index;
    std::string value;
    bool        schema_valid = true;
};

// ---------------------------------------------------------------------------
// Mock ingestion buffer
// ---------------------------------------------------------------------------
struct MockBuffer {
    std::size_t capacity;
    std::size_t used = 0;

    bool isFull() const { return used >= capacity; }

    IngestionErrorCode push(std::size_t count = 1) {
        if (used + count > capacity)
            return IngestionErrorCode::INGESTION_BUFFER_FULL;
        used += count;
        return IngestionErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Mock batch writer
// ---------------------------------------------------------------------------
struct MockBatchResult {
    IngestionErrorCode   code      = IngestionErrorCode::OK;
    std::vector<MockRow> committed;
    bool                 visible   = false; // only true after commit
    bool                 acked     = false;
};

struct MockBatchWriter {
    std::size_t   max_batch_size = kDefaultBatchSize;
    DurabilityLevel durability   = DurabilityLevel::LocalWal;

    MockBatchResult write(const std::vector<MockRow>& rows, bool simulate_partial_fail = false) {
        MockBatchResult result;

        // Batch size check
        if (rows.size() > max_batch_size) {
            result.code = IngestionErrorCode::INGESTION_QUOTA_EXCEEDED;
            return result;
        }

        // Schema validation before any write
        for (auto& r : rows) {
            if (!r.schema_valid) {
                result.code = IngestionErrorCode::INGESTION_SCHEMA_INVALID;
                return result;  // no data written
            }
        }

        // Simulate partial failure mid-commit
        if (simulate_partial_fail) {
            result.code = IngestionErrorCode::INGESTION_BATCH_ROLLBACK;
            return result;  // nothing committed
        }

        // Commit atomically (row ordering preserved)
        result.committed = rows;
        result.visible   = true;
        result.acked     = true;
        return result;
    }
};

// ---------------------------------------------------------------------------
// Mock quota checker
// ---------------------------------------------------------------------------
struct MockQuotaChecker {
    std::uint64_t limit;   ///< rows/s (0 = unlimited)
    std::uint64_t used = 0;

    IngestionErrorCode checkAndConsume(std::uint64_t rows) {
        if (limit == 0u) return IngestionErrorCode::OK;
        if (used + rows > limit) return IngestionErrorCode::INGESTION_QUOTA_EXCEEDED;
        used += rows;
        return IngestionErrorCode::OK;
    }
};

} // anonymous namespace

// ===========================================================================
// INCH-01 — Ack implies durability: no silent drop after ack
// ===========================================================================

TEST(IngestionContractHardeningINCH01, AckImpliesDurability) {
    MockBatchWriter writer;
    std::vector<MockRow> rows = {{0, "v0"}, {1, "v1"}};

    auto r = writer.write(rows);
    EXPECT_EQ(r.code, IngestionErrorCode::OK);
    EXPECT_TRUE(r.acked)  << "Ack must be sent after successful commit";
    EXPECT_TRUE(r.visible)<< "Data must be committed (durable) before ack";
    EXPECT_EQ(r.committed.size(), 2u) << "No silent drop: all rows must be committed";
}

// ===========================================================================
// INCH-02 — Buffer full → INGESTION_BUFFER_FULL (not silent block)
// ===========================================================================

TEST(IngestionContractHardeningINCH02, BufferFullSignalNotSilentBlock) {
    MockBuffer buf{/*capacity=*/2};
    EXPECT_EQ(buf.push(2), IngestionErrorCode::OK);
    EXPECT_TRUE(buf.isFull());

    auto rc = buf.push(1);
    EXPECT_EQ(rc, IngestionErrorCode::INGESTION_BUFFER_FULL)
        << "Buffer-full must surface INGESTION_BUFFER_FULL signal";
}

// ===========================================================================
// INCH-03 — INGESTION_BUFFER_FULL is a back-off error
// ===========================================================================

TEST(IngestionContractHardeningINCH03, BufferFullIsBackOff) {
    EXPECT_TRUE(isBackOffCode(IngestionErrorCode::INGESTION_BUFFER_FULL));
    EXPECT_FALSE(isPermanentRejection(IngestionErrorCode::INGESTION_BUFFER_FULL));
}

// ===========================================================================
// INCH-04 — Back-pressure signal delivered within kBackPressureSignalDeadline
// ===========================================================================

TEST(IngestionContractHardeningINCH04, BackPressureSignalDeadlineContract) {
    EXPECT_LE(kBackPressureSignalDeadline.count(), 50)
        << "Back-pressure signal must be delivered within 50 ms per contract";
    EXPECT_GE(kBackPressureSignalDeadline.count(), 1);

    // Simulate: signal generated immediately on buffer-full detection
    auto t0 = std::chrono::steady_clock::now();
    MockBuffer buf{1};
    buf.push(1);  // full
    auto signal_code = buf.push(1);  // triggers signal
    auto t1 = std::chrono::steady_clock::now();

    EXPECT_EQ(signal_code, IngestionErrorCode::INGESTION_BUFFER_FULL);
    EXPECT_LT(t1 - t0, kBackPressureSignalDeadline)
        << "In-memory signal must be delivered well within the deadline";
}

// ===========================================================================
// INCH-05 — Partial batch not visible before commit
// ===========================================================================

TEST(IngestionContractHardeningINCH05, PartialBatchNotVisiblePreCommit) {
    // Simulate: simulate_partial_fail = true → nothing committed
    MockBatchWriter writer;
    std::vector<MockRow> rows = {{0, "v0"}, {1, "v1"}};

    auto r = writer.write(rows, /*simulate_partial_fail=*/true);
    EXPECT_EQ(r.code, IngestionErrorCode::INGESTION_BATCH_ROLLBACK);
    EXPECT_FALSE(r.visible) << "Partial batch must not be visible after rollback";
    EXPECT_EQ(r.committed.size(), 0u);
}

// ===========================================================================
// INCH-06 — Batch commit preserves row ordering
// ===========================================================================

TEST(IngestionContractHardeningINCH06, BatchCommitPreservesOrdering) {
    MockBatchWriter writer;
    std::vector<MockRow> rows;
    for (int i = 0; i < 10; ++i) rows.push_back({i, "v" + std::to_string(i)});

    auto r = writer.write(rows);
    EXPECT_EQ(r.code, IngestionErrorCode::OK);
    ASSERT_EQ(r.committed.size(), 10u);
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(r.committed[i].index, i) << "Row ordering must be preserved";
}

// ===========================================================================
// INCH-07 — Rollback on partial failure — no rows visible
// ===========================================================================

TEST(IngestionContractHardeningINCH07, RollbackNoRowsVisible) {
    MockBatchWriter writer;
    std::vector<MockRow> rows = {{0, "a"}, {1, "b"}, {2, "c"}};

    auto r = writer.write(rows, /*simulate_partial_fail=*/true);
    EXPECT_NE(r.code, IngestionErrorCode::OK);
    EXPECT_EQ(r.committed.size(), 0u)
        << "Rollback: zero rows must be committed on partial failure";
}

// ===========================================================================
// INCH-08 — Batch size ≤ kMaxBatchSize enforced
// ===========================================================================

TEST(IngestionContractHardeningINCH08, BatchSizeLimitEnforced) {
    // kMaxBatchSize is the contract constant
    EXPECT_GT(kMaxBatchSize, 0u);
    EXPECT_GE(kMaxBatchSize, kDefaultBatchSize);

    MockBatchWriter writer;
    writer.max_batch_size = 5;

    std::vector<MockRow> big_batch;
    for (int i = 0; i < 6; ++i) big_batch.push_back({i, "v"});

    auto r = writer.write(big_batch);
    EXPECT_EQ(r.code, IngestionErrorCode::INGESTION_QUOTA_EXCEEDED)
        << "Batch exceeding max size must be rejected";
}

// ===========================================================================
// INCH-09 — Valid schema → write accepted and acked
// ===========================================================================

TEST(IngestionContractHardeningINCH09, ValidSchemaAccepted) {
    MockBatchWriter writer;
    std::vector<MockRow> rows = {{0, "ok", /*schema_valid=*/true}};

    auto r = writer.write(rows);
    EXPECT_EQ(r.code, IngestionErrorCode::OK);
    EXPECT_TRUE(r.acked);
}

// ===========================================================================
// INCH-10 — Invalid schema → INGESTION_SCHEMA_INVALID before ack
// ===========================================================================

TEST(IngestionContractHardeningINCH10, InvalidSchemaRejectedBeforeAck) {
    MockBatchWriter writer;
    std::vector<MockRow> rows = {{0, "bad", /*schema_valid=*/false}};

    auto r = writer.write(rows);
    EXPECT_EQ(r.code, IngestionErrorCode::INGESTION_SCHEMA_INVALID);
    EXPECT_FALSE(r.acked) << "No ack must be sent when schema is invalid";
    EXPECT_EQ(r.committed.size(), 0u) << "No data must be written on schema error";
}

// ===========================================================================
// INCH-11 — Schema validation happens before any data is written
// ===========================================================================

TEST(IngestionContractHardeningINCH11, SchemaValidationBeforeDataWrite) {
    MockBatchWriter writer;
    // Mix: valid rows, then one invalid
    std::vector<MockRow> rows = {{0, "ok"}, {1, "ok"}, {2, "bad", false}};

    auto r = writer.write(rows);
    EXPECT_EQ(r.code, IngestionErrorCode::INGESTION_SCHEMA_INVALID);
    EXPECT_EQ(r.committed.size(), 0u)
        << "Schema validation must abort before any row is written";
}

// ===========================================================================
// INCH-12 — INGESTION_SCHEMA_INVALID is a permanent rejection
// ===========================================================================

TEST(IngestionContractHardeningINCH12, SchemaInvalidIsPermanentRejection) {
    EXPECT_TRUE(isPermanentRejection(IngestionErrorCode::INGESTION_SCHEMA_INVALID));
    EXPECT_FALSE(isBackOffCode(IngestionErrorCode::INGESTION_SCHEMA_INVALID));
}

// ===========================================================================
// INCH-13 — Producer back-pressure signal is explicit (not silent)
// ===========================================================================

TEST(IngestionContractHardeningINCH13, BackPressureExplicitSignal) {
    MockBuffer buf{3};
    buf.push(3);  // fill to capacity

    // Next push must return explicit error code (not hang/block silently)
    auto rc = buf.push(1);
    EXPECT_EQ(rc, IngestionErrorCode::INGESTION_BUFFER_FULL);
    EXPECT_NE(rc, IngestionErrorCode::OK)
        << "Back-pressure must produce an explicit error, not silent blocking";
}

// ===========================================================================
// INCH-14 — Quota exceeded → INGESTION_QUOTA_EXCEEDED
// ===========================================================================

TEST(IngestionContractHardeningINCH14, QuotaExceededSurfaced) {
    MockQuotaChecker qc{/*limit=*/100u};
    EXPECT_EQ(qc.checkAndConsume(100u), IngestionErrorCode::OK);
    auto rc = qc.checkAndConsume(1u);
    EXPECT_EQ(rc, IngestionErrorCode::INGESTION_QUOTA_EXCEEDED);
}

// ===========================================================================
// INCH-15 — INGESTION_QUOTA_EXCEEDED is a back-off error
// ===========================================================================

TEST(IngestionContractHardeningINCH15, QuotaExceededIsBackOff) {
    EXPECT_TRUE(isBackOffCode(IngestionErrorCode::INGESTION_QUOTA_EXCEEDED));
    EXPECT_FALSE(isPermanentRejection(IngestionErrorCode::INGESTION_QUOTA_EXCEEDED));
}

// ===========================================================================
// INCH-16 — Strong-ack durability levels identified correctly
// ===========================================================================

TEST(IngestionContractHardeningINCH16, StrongAckDurabilityLevels) {
    EXPECT_FALSE(isStrongAck(DurabilityLevel::LocalWal))
        << "LocalWal is not a strong-ack level";
    EXPECT_TRUE(isStrongAck(DurabilityLevel::QuorumSync))
        << "QuorumSync is a strong-ack level";
    EXPECT_TRUE(isStrongAck(DurabilityLevel::FsyncLocal))
        << "FsyncLocal is a strong-ack level";
}
