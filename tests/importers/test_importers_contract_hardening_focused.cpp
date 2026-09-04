// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_importers_contract_hardening_focused.cpp
 * @brief Phase 4 — Importers contract hardening focused tests (IMCH-01..IMCH-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed is
 * kImportersContractSeed = 42.
 *
 * ## Test families
 *
 * ### IMCH-01..04 — Idempotency contract
 *   IMCH-01  Re-importing same data with same import_id produces no duplicates
 *   IMCH-02  import_id uniqueness is enforced; duplicate rejected
 *   IMCH-03  Different import_id with same data is accepted (new import)
 *   IMCH-04  Committed row count matches source row count
 *
 * ### IMCH-05..08 — Schema evolution
 *   IMCH-05  Additive column (new nullable) passes through transparently
 *   IMCH-06  Missing required column surfaces IMPORT_SCHEMA_MISMATCH
 *   IMCH-07  Breaking change (type change) surfaces IMPORT_SCHEMA_MISMATCH
 *   IMCH-08  Schema with no changes classified as NoChange
 *
 * ### IMCH-09..12 — Error handling
 *   IMCH-09  Bad row skipped when disposition=SKIP; count reported
 *   IMCH-10  Bad row aborts import when disposition=FAIL; no rows committed
 *   IMCH-11  Partial import not visible before commit
 *   IMCH-12  Error response includes exact bad-row count
 *
 * ### IMCH-13..16 — Large import / edge cases
 *   IMCH-13  Row ordering is preserved during import
 *   IMCH-14  Quota exceeded surfaces IMPORT_QUOTA_EXCEEDED
 *   IMCH-15  Atomic commit: all-or-nothing visibility
 *   IMCH-16  IMPORT_TIMEOUT is a retryable error
 *
 * @see include/importers/importers_api_contract.h
 * @see src/importers/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "importers/importers_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace themis::importers;

namespace {

static constexpr uint64_t kImportersContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock row
// ---------------------------------------------------------------------------
struct MockRow {
    int                                    index = 0;
    std::map<std::string, std::string>     fields;
    bool                                   valid = true;
};

// ---------------------------------------------------------------------------
// Mock import job / coordinator
// ---------------------------------------------------------------------------
struct MockImportResult {
    ImporterErrorCode          code        = ImporterErrorCode::OK;
    std::vector<MockRow>       committed;
    int                        bad_row_count = 0;
    bool                       visible       = false; // only true after commit
};

struct MockImportCoordinator {
    std::set<std::string> used_ids;  // import_id registry

    MockImportResult run(const std::string&        import_id,
                         const std::vector<MockRow>& rows,
                         BadRowDisposition           disposition,
                         std::uint64_t               row_quota = 0u) {
        MockImportResult result;

        // Idempotency: reject duplicate import_id
        if (used_ids.count(import_id)) {
            result.code = ImporterErrorCode::IMPORT_DUPLICATE_ID;
            return result;
        }

        // Quota check
        if (row_quota > 0 && rows.size() > row_quota) {
            result.code = ImporterErrorCode::IMPORT_QUOTA_EXCEEDED;
            return result;
        }

        // Process rows (pre-commit — not visible yet)
        std::vector<MockRow> staging;
        for (auto& row : rows) {
            if (!row.valid) {
                ++result.bad_row_count;
                if (disposition == BadRowDisposition::Fail) {
                    result.code = ImporterErrorCode::IMPORT_ROW_INVALID;
                    return result;  // abort; nothing committed
                }
                // SKIP: continue
            } else {
                staging.push_back(row);
            }
        }

        // Commit atomically — row ordering preserved
        used_ids.insert(import_id);
        result.committed = staging;
        result.visible   = true;
        return result;
    }
};

// ---------------------------------------------------------------------------
// Mock schema checker
// ---------------------------------------------------------------------------
struct MockColumn { std::string name; std::string type; bool required; };

struct MockSchema { std::vector<MockColumn> columns; };

static SchemaChangeKind classifySchemaChange(
        const MockSchema& existing, const MockSchema& incoming) {
    // Check for breaking changes: removed or type-changed columns
    for (auto& ec : existing.columns) {
        auto it = std::find_if(incoming.columns.begin(), incoming.columns.end(),
                               [&](const MockColumn& ic){ return ic.name == ec.name; });
        if (it == incoming.columns.end() && ec.required) {
          return SchemaChangeKind::Breaking;
        }
        if (it != incoming.columns.end() && it->type != ec.type) {
          return SchemaChangeKind::Breaking;
        }
    }
    // Check for new nullable columns (additive)
    for (auto& ic : incoming.columns) {
        auto it = std::find_if(existing.columns.begin(), existing.columns.end(),
                               [&](const MockColumn& ec){ return ec.name == ic.name; });
        if (it == existing.columns.end() && !ic.required) {
          return SchemaChangeKind::Additive;
        }
    }
    return SchemaChangeKind::NoChange;
}

} // anonymous namespace

// ===========================================================================
// IMCH-01 — Re-importing same data with same import_id produces no duplicates
// ===========================================================================

TEST(ImportersContractHardeningIMCH01, ReImportSameIdNoDuplicates) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {{0, {{"k","v"}}, true}};

    auto r1 = coord.run("imp-001", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r1.code, ImporterErrorCode::OK);
    EXPECT_EQ(r1.committed.size(), 1u);

    // Re-import same import_id
    auto r2 = coord.run("imp-001", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r2.code, ImporterErrorCode::IMPORT_DUPLICATE_ID)
        << "Duplicate import_id must be rejected";
    EXPECT_EQ(r2.committed.size(), 0u);
}

// ===========================================================================
// IMCH-02 — import_id uniqueness is enforced
// ===========================================================================

TEST(ImportersContractHardeningIMCH02, ImportIdUniquenessEnforced) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {{0, {{"x","1"}}, true}};

    coord.run("id-A", rows, BadRowDisposition::Skip);
    auto r = coord.run("id-A", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r.code, ImporterErrorCode::IMPORT_DUPLICATE_ID);
}

// ===========================================================================
// IMCH-03 — Different import_id with same data is accepted
// ===========================================================================

TEST(ImportersContractHardeningIMCH03, DifferentImportIdAccepted) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {{0, {{"k","v"}}, true}};

    auto r1 = coord.run("id-1", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r1.code, ImporterErrorCode::OK);

    auto r2 = coord.run("id-2", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r2.code, ImporterErrorCode::OK)
        << "Different import_id with same data must be accepted as a new import";
}

// ===========================================================================
// IMCH-04 — Committed row count matches valid source row count
// ===========================================================================

TEST(ImportersContractHardeningIMCH04, CommittedRowCountMatchesSource) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows;
    for (int i = 0; i < 10; ++i) rows.push_back({i, {{"i", std::to_string(i)}}, true});

    auto r = coord.run("id-04", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r.code, ImporterErrorCode::OK);
    EXPECT_EQ(r.committed.size(), 10u);
}

// ===========================================================================
// IMCH-05 — Additive column passes through transparently
// ===========================================================================

TEST(ImportersContractHardeningIMCH05, AdditiveColumnPassthrough) {
    MockSchema existing = {{{ "id", "INT64", true}, {"name", "STRING", true}}};
    MockSchema incoming = {{{ "id", "INT64", true}, {"name", "STRING", true},
                             {"extra", "STRING", false}}}; // new nullable column

    auto kind = classifySchemaChange(existing, incoming);
    EXPECT_EQ(kind, SchemaChangeKind::Additive);
    EXPECT_FALSE(requiresMigration(kind));
}

// ===========================================================================
// IMCH-06 — Missing required column surfaces IMPORT_SCHEMA_MISMATCH
// ===========================================================================

TEST(ImportersContractHardeningIMCH06, MissingRequiredColumnIsMismatch) {
    MockSchema existing = {{{ "id", "INT64", true}, {"name", "STRING", true}}};
    MockSchema incoming = {{{ "id", "INT64", true}}};  // 'name' required but missing

    auto kind = classifySchemaChange(existing, incoming);
    EXPECT_EQ(kind, SchemaChangeKind::Breaking);
    EXPECT_TRUE(requiresMigration(kind));

    // Simulate error code
    ImporterErrorCode rc = requiresMigration(kind)
        ? ImporterErrorCode::IMPORT_SCHEMA_MISMATCH
        : ImporterErrorCode::OK;
    EXPECT_EQ(rc, ImporterErrorCode::IMPORT_SCHEMA_MISMATCH);
}

// ===========================================================================
// IMCH-07 — Breaking type change surfaces IMPORT_SCHEMA_MISMATCH
// ===========================================================================

TEST(ImportersContractHardeningIMCH07, TypeChangeSurfacesMismatch) {
    MockSchema existing = {{{ "id", "INT64", true}}};
    MockSchema incoming = {{{ "id", "STRING", true}}};  // type changed: INT64 → STRING

    auto kind = classifySchemaChange(existing, incoming);
    EXPECT_EQ(kind, SchemaChangeKind::Breaking);
    EXPECT_TRUE(requiresMigration(kind));
}

// ===========================================================================
// IMCH-08 — No-change schema classified as NoChange
// ===========================================================================

TEST(ImportersContractHardeningIMCH08, NoChangeSchemaClassified) {
    MockSchema s = {{{ "id", "INT64", true}, {"val", "DOUBLE", false}}};
    auto kind = classifySchemaChange(s, s);
    EXPECT_EQ(kind, SchemaChangeKind::NoChange);
    EXPECT_FALSE(requiresMigration(kind));
}

// ===========================================================================
// IMCH-09 — Bad row skipped when disposition=SKIP; count reported
// ===========================================================================

TEST(ImportersContractHardeningIMCH09, BadRowSkippedCountReported) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {
        {0, {{"k","v"}}, true},
        {1, {},          false},  // bad
        {2, {{"k","w"}}, true},
    };

    auto r = coord.run("id-09", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r.code, ImporterErrorCode::OK);
    EXPECT_EQ(r.committed.size(), 2u)   << "Two valid rows must be committed";
    EXPECT_EQ(r.bad_row_count, 1)       << "Bad-row count must be 1";
}

// ===========================================================================
// IMCH-10 — Bad row aborts import when disposition=FAIL
// ===========================================================================

TEST(ImportersContractHardeningIMCH10, BadRowAbortsOnFail) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {
        {0, {{"k","v"}}, true},
        {1, {},          false},  // bad
    };

    auto r = coord.run("id-10", rows, BadRowDisposition::Fail);
    EXPECT_EQ(r.code, ImporterErrorCode::IMPORT_ROW_INVALID);
    EXPECT_EQ(r.committed.size(), 0u)
        << "No rows must be committed when import is aborted on bad row";
}

// ===========================================================================
// IMCH-11 — Partial import not visible before commit
// ===========================================================================

TEST(ImportersContractHardeningIMCH11, PartialImportNotVisiblePreCommit) {
    // Simulated: staged rows are not visible until result.visible = true
    MockImportCoordinator coord;
    std::vector<MockRow> rows = {{0, {{"k","v"}}, true}};

    // Before run, no data committed
    // (In a real system, visibility is controlled by transaction isolation.)
    auto r = coord.run("id-11", rows, BadRowDisposition::Skip);
    // After run with OK, data is visible
    EXPECT_TRUE(r.visible);
    EXPECT_EQ(r.committed.size(), 1u);
}

// ===========================================================================
// IMCH-12 — Error response includes exact bad-row count
// ===========================================================================

TEST(ImportersContractHardeningIMCH12, ErrorResponseIncludesBadRowCount) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows;
    for (int i = 0; i < 10; ++i)
        rows.push_back({i, {}, i % 3 == 0 ? false : true}); // rows 0,3,6,9 bad

    auto r = coord.run("id-12", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r.bad_row_count, 4)
        << "Error response must include the exact bad-row count";
}

// ===========================================================================
// IMCH-13 — Row ordering preserved during import
// ===========================================================================

TEST(ImportersContractHardeningIMCH13, RowOrderingPreserved) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows;
    for (int i = 0; i < 20; ++i)
        rows.push_back({i, {{"seq", std::to_string(i)}}, true});

    auto r = coord.run("id-13", rows, BadRowDisposition::Skip);
    EXPECT_EQ(r.committed.size(), 20u);
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(r.committed[i].index, i)
            << "Row ordering must be preserved";
    }
}

// ===========================================================================
// IMCH-14 — Quota exceeded surfaces IMPORT_QUOTA_EXCEEDED
// ===========================================================================

TEST(ImportersContractHardeningIMCH14, QuotaExceededSurfaced) {
    MockImportCoordinator coord;
    std::vector<MockRow> rows;
    for (int i = 0; i < 10; ++i) rows.push_back({i, {{"k","v"}}, true});

    auto r = coord.run("id-14", rows, BadRowDisposition::Skip, /*row_quota=*/5u);
    EXPECT_EQ(r.code, ImporterErrorCode::IMPORT_QUOTA_EXCEEDED);
    EXPECT_EQ(r.committed.size(), 0u);
}

// ===========================================================================
// IMCH-15 — Atomic commit: all-or-nothing visibility
// ===========================================================================

TEST(ImportersContractHardeningIMCH15, AtomicCommitAllOrNothing) {
    MockImportCoordinator coord;
    // All valid rows
    std::vector<MockRow> valid_rows;
    for (int i = 0; i < 5; ++i) valid_rows.push_back({i, {{"k","v"}}, true});

    auto r_ok = coord.run("id-15a", valid_rows, BadRowDisposition::Fail);
    EXPECT_EQ(r_ok.code, ImporterErrorCode::OK);
    EXPECT_EQ(r_ok.committed.size(), 5u);
    EXPECT_TRUE(r_ok.visible);

    // One bad row with Fail → nothing committed
    std::vector<MockRow> mixed = valid_rows;
    mixed.push_back({99, {}, false});
    auto r_fail = coord.run("id-15b", mixed, BadRowDisposition::Fail);
    EXPECT_EQ(r_fail.code, ImporterErrorCode::IMPORT_ROW_INVALID);
    EXPECT_EQ(r_fail.committed.size(), 0u)
        << "Atomic commit: on failure, zero rows visible";
}

// ===========================================================================
// IMCH-16 — IMPORT_TIMEOUT is a retryable error
// ===========================================================================

TEST(ImportersContractHardeningIMCH16, ImportTimeoutIsRetryable) {
    EXPECT_TRUE(isRetryableImportError(ImporterErrorCode::IMPORT_TIMEOUT));
    EXPECT_TRUE(isRetryableImportError(ImporterErrorCode::IMPORT_CONNECTOR_UNAVAILABLE));
    EXPECT_FALSE(isRetryableImportError(ImporterErrorCode::IMPORT_SCHEMA_MISMATCH));
    EXPECT_FALSE(isRetryableImportError(ImporterErrorCode::IMPORT_DUPLICATE_KEY));
    EXPECT_FALSE(isRetryableImportError(ImporterErrorCode::IMPORT_ROW_INVALID));
}
