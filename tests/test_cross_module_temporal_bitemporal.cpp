/**
 * @file test_cross_module_temporal_bitemporal.cpp
 * @brief Cross-module integration tests: TemporalQueryEngine interacting with
 *        both SystemVersionedTable and BiTemporalTable.
 *
 * These tests exercise the production boundary between three temporal-module
 * components that each have their own unit-test suites but whose interactions
 * are not tested in isolation:
 *
 *   TemporalQueryEngine  ←→  SystemVersionedTable
 *   TemporalQueryEngine  ←→  BiTemporalTable
 *   SystemVersionedTable ←→  SystemVersionedTable  (joinAsOf)
 *   BiTemporalTable      ←→  BiTemporalTable        (joinBiTemporal)
 *
 * Group A – executeTemporalQuery dispatcher (SystemVersionedTable)
 * ----------------------------------------------------------------
 * Validates that each TemporalClause (AS_OF, FROM_TO, BETWEEN_AND,
 * CONTAINED_IN, ALL) is correctly dispatched through
 * executeTemporalQuery(SystemVersionedTable&, TemporalQuerySpec) and that the
 * results are consistent with the direct low-level API calls.
 *
 * Group B – executeTemporalQuery dispatcher (BiTemporalTable)
 * -----------------------------------------------------------
 * Validates executeTemporalQuery(BiTemporalTable&, TemporalQuerySpec) for the
 * clauses supported by application-time queries.
 *
 * Group C – Cross-table join consistency
 * ----------------------------------------
 * Validates:
 *   • joinAsOf returns pairs that are both current at the specified instant.
 *   • joinBiTemporal respects both sys_as_of and valid_at simultaneously.
 *   • A join predicate that matches no rows produces an empty result.
 *
 * Group D – Temporal invariants under mutation
 * ----------------------------------------------
 * Validates:
 *   • Deleted rows appear only when include_deleted == true (ALL clause).
 *   • AS_OF before any insertion returns an empty result set.
 *   • FROM_TO captures all intermediate versions of a repeatedly updated row.
 *   • BiTemporalTable uniqueness constraint is enforced and detectable.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "temporal/temporal_query_engine.h"
#include "temporal/system_versioned_table.h"
#include "temporal/bi_temporal.h"
#include "temporal/temporal_types.h"

#include <functional>
#include <string>
#include <thread>
#include <chrono>

#include "utils/test_stability.h"

using namespace themisdb::temporal;

// ============================================================================
// Shared helpers
// ============================================================================

/// Return a timestamp guaranteed to be strictly before t.
static Timestamp before(Timestamp t) {
    return t - 1;
}

/// Spin until the wall-clock millisecond counter advances by at least 1 unit,
/// guaranteeing that the next now() call returns a strictly greater Timestamp.
/// Uses yield() to avoid a blind fixed-duration sleep (flaky on loaded CI).
static void advanceClock() {
    themis::test::wait_for_clock_advance_ms();
}

// ============================================================================
// Shared fixture: two SystemVersionedTables pre-populated for joins
// ============================================================================

struct TemporalCrossModuleFixture : ::testing::Test {
protected:
    SystemVersionedTable emp{"employees", "node_a"};
    SystemVersionedTable dept{"departments", "node_a"};

    Timestamp t_before_any{0};
    Timestamp t_after_dept_insert{0};
    Timestamp t_after_emp_insert{0};
    Timestamp t_after_emp_update{0};

    void SetUp() override {
        t_before_any = now();
        advanceClock();

        dept.insert("d1", {{"name", "Engineering"}, {"budget", 1000000}});
        dept.insert("d2", {{"name", "Sales"},       {"budget", 500000}});
        t_after_dept_insert = now();
        advanceClock();

        emp.insert("e1", {{"name", "Alice"}, {"dept_id", "d1"}, {"salary", 70000}});
        emp.insert("e2", {{"name", "Bob"},   {"dept_id", "d2"}, {"salary", 55000}});
        t_after_emp_insert = now();
        advanceClock();

        // Update e1: salary raise
        emp.update("e1", {{"salary", 80000}});
        t_after_emp_update = now();
    }
};

// ============================================================================
// Group A – executeTemporalQuery over SystemVersionedTable
// ============================================================================

struct SystemVersionedDispatchTest : TemporalCrossModuleFixture {};

// ---------------------------------------------------------------------------
// A-1: AS_OF clause returns only rows current at the specified instant
// ---------------------------------------------------------------------------
TEST_F(SystemVersionedDispatchTest, ExecuteTemporalQuery_AsOf_MatchesDirectQueryAsOf) {
    auto spec    = TemporalQuerySpec::asOf(t_after_emp_insert);
    auto via_spec = TemporalQueryEngine::executeTemporalQuery(emp, spec);
    auto direct   = TemporalQueryEngine::queryAsOf(emp, t_after_emp_insert);

    ASSERT_EQ(via_spec.size(), direct.size())
        << "executeTemporalQuery(AS_OF) must match queryAsOf()";
    EXPECT_EQ(via_spec.size(), 2u) << "Two employees inserted; both must be current";
}

// ---------------------------------------------------------------------------
// A-2: FROM_TO clause matches queryFromTo() result
// ---------------------------------------------------------------------------
TEST_F(SystemVersionedDispatchTest, ExecuteTemporalQuery_FromTo_MatchesDirectQueryFromTo) {
    auto spec    = TemporalQuerySpec::fromTo(kMinTimestamp, kMaxTimestamp);
    auto via_spec = TemporalQueryEngine::executeTemporalQuery(emp, spec);
    auto direct   = TemporalQueryEngine::queryFromTo(emp, kMinTimestamp, kMaxTimestamp);

    ASSERT_EQ(via_spec.size(), direct.size())
        << "executeTemporalQuery(FROM_TO) must match queryFromTo()";
    // e1 has 2 versions (insert + update), e2 has 1 → total 3
    EXPECT_EQ(via_spec.size(), 3u);
}

// ---------------------------------------------------------------------------
// A-3: BETWEEN_AND clause returns all versions in the closed range [s, e]
// ---------------------------------------------------------------------------
TEST_F(SystemVersionedDispatchTest, ExecuteTemporalQuery_BetweenAnd_ReturnsVersionsInClosedRange) {
    // Range covers the update period: both the pre- and post-update version of e1
    // should be present when the range spans the update.
    auto spec = TemporalQuerySpec::betweenAnd(t_after_emp_insert, t_after_emp_update);
    auto rows = TemporalQueryEngine::executeTemporalQuery(emp, spec);

    // At least the post-insert version of e1 and e2 must be present.
    EXPECT_GE(rows.size(), 2u)
        << "BETWEEN_AND over the insert/update range must return at least 2 rows";
}

// ---------------------------------------------------------------------------
// A-4: CONTAINED_IN clause excludes rows whose period extends outside [s, e)
// ---------------------------------------------------------------------------
TEST_F(SystemVersionedDispatchTest, ExecuteTemporalQuery_ContainedIn_ExcludesOpenVersions) {
    // Open-ended (current) rows have sys_time.end == kMaxTimestamp, which is
    // NOT contained within any finite range.
    auto spec = TemporalQuerySpec::containedIn(kMinTimestamp, t_after_emp_update);
    auto rows = TemporalQueryEngine::executeTemporalQuery(emp, spec);

    // The current (open-ended) row for e1 must NOT appear because its
    // sys_time.end == kMaxTimestamp is outside the query range.
    // Only closed historical versions (e.g. the first version of e1) qualify.
    for (const auto& row : rows) {
        EXPECT_FALSE(row.isCurrent())
            << "CONTAINED_IN must not return open-ended (current) rows";
    }
}

// ---------------------------------------------------------------------------
// A-5: ALL clause returns all stored versions (open and closed)
// ---------------------------------------------------------------------------
TEST_F(SystemVersionedDispatchTest, ExecuteTemporalQuery_All_IncludesAllVersions) {
    // Close e2's current version so it has no open row.
    emp.deleteRow("e2");
    advanceClock();

    // ALL with include_deleted=true must return all stored versions:
    //   e1 → 2 versions (insert + update), e2 → 1 closed version = 3 total.
    TemporalQuerySpec spec = TemporalQuerySpec::all();
    spec.include_deleted   = true;
    auto all_rows = TemporalQueryEngine::executeTemporalQuery(emp, spec);
    EXPECT_GE(all_rows.size(), 3u)
        << "ALL must return all versions including the closed e2 version";

    // AS_OF at now() must NOT return e2 (closed row: sys_time.end < now()).
    auto current = TemporalQueryEngine::executeTemporalQuery(
        emp, TemporalQuerySpec::asOf(now()));
    // Only e1 is still current; e2's sys_time was closed by deleteRow().
    EXPECT_EQ(current.size(), 1u)
        << "After deleteRow(e2), only e1 must be current at now()";
}

// ============================================================================
// Group B – executeTemporalQuery over BiTemporalTable
// ============================================================================

struct BiTemporalDispatchTest : ::testing::Test {
protected:
    BiTemporalTable contracts{"contracts", "node_a"};

    // Valid-time periods (application time)
    static constexpr Timestamp kVT_Start  = 1000;
    static constexpr Timestamp kVT_End    = 5000;
    static constexpr Timestamp kVT_Start2 = 6000;
    static constexpr Timestamp kVT_End2   = 9000;

    void SetUp() override {
        contracts.insertWithValidTime("c1", {{"value", 100}, {"status", "active"}},
                                      {kVT_Start, kVT_End});
        contracts.insertWithValidTime("c2", {{"value", 200}, {"status", "pending"}},
                                      {kVT_Start2, kVT_End2});
    }
};

// ---------------------------------------------------------------------------
// B-1: AS_OF valid-time clause returns rows whose valid-time contains valid_at
// ---------------------------------------------------------------------------
TEST_F(BiTemporalDispatchTest, ExecuteTemporalQuery_BiTemporal_AsOf_MatchesQueryApplicationTime) {
    constexpr Timestamp query_vt = kVT_Start + 500; // inside c1's valid-time

    auto spec    = TemporalQuerySpec::asOf(query_vt);
    auto via_spec = TemporalQueryEngine::executeTemporalQuery(contracts, spec);
    auto direct   = TemporalQueryEngine::queryApplicationTime(contracts, query_vt);

    ASSERT_EQ(via_spec.size(), direct.size())
        << "executeTemporalQuery(BiTemporal, AS_OF) must match queryApplicationTime()";
    EXPECT_EQ(via_spec.size(), 1u) << "Only c1 is valid at this point";
}

// ---------------------------------------------------------------------------
// B-2: FROM_TO valid-time clause matches queryApplicationTimeRange()
// ---------------------------------------------------------------------------
TEST_F(BiTemporalDispatchTest, ExecuteTemporalQuery_BiTemporal_FromTo_MatchesQueryApplicationTimeRange) {
    // Range that overlaps both contracts
    auto spec    = TemporalQuerySpec::fromTo(kVT_Start, kVT_End2);
    auto via_spec = TemporalQueryEngine::executeTemporalQuery(contracts, spec);
    auto direct   = TemporalQueryEngine::queryApplicationTimeRange(
                        contracts, kVT_Start, kVT_End2);

    ASSERT_EQ(via_spec.size(), direct.size())
        << "executeTemporalQuery(BiTemporal, FROM_TO) must match queryApplicationTimeRange()";
    EXPECT_GE(via_spec.size(), 1u);
}

// ---------------------------------------------------------------------------
// B-3: AS_OF between two non-overlapping valid-time periods returns empty
// ---------------------------------------------------------------------------
TEST_F(BiTemporalDispatchTest, ExecuteTemporalQuery_BiTemporal_AsOf_BetweenPeriods_ReturnsEmpty) {
    // Gap between c1 and c2: e.g. kVT_End + 1  (> c1.end, < c2.start)
    constexpr Timestamp gap_ts = kVT_End + 100;

    auto spec = TemporalQuerySpec::asOf(gap_ts);
    auto rows = TemporalQueryEngine::executeTemporalQuery(contracts, spec);

    EXPECT_TRUE(rows.empty())
        << "No contract is valid during the gap between kVT_End and kVT_Start2";
}

// ============================================================================
// Group C – Cross-table join consistency
// ============================================================================

struct JoinConsistencyTest : TemporalCrossModuleFixture {};

// ---------------------------------------------------------------------------
// C-1: joinAsOf returns only pairs current at the specified instant
// ---------------------------------------------------------------------------
TEST_F(JoinConsistencyTest, JoinAsOf_MatchingPredicate_ReturnsPairsCurrentAtInstant) {
    // Join predicate: employee's dept_id equals the department's key.
    auto predicate = [](const VersionedDocument& e, const VersionedDocument& d) -> bool {
        if (!e.data.contains("dept_id")) {
          return false;
        }
        return e.data["dept_id"].get<std::string>() == d.key;
    };

    auto pairs = TemporalQueryEngine::joinAsOf(emp, dept, t_after_emp_insert, predicate);

    // e1 → d1 and e2 → d2 should match; result must be non-empty.
    EXPECT_GE(pairs.size(), 1u)
        << "joinAsOf must return at least one matching (employee, department) pair";

    // Every row in each pair must have been current at t_after_emp_insert.
    for (const auto& [left, right] : pairs) {
        EXPECT_TRUE(left.sys_time.start  <= t_after_emp_insert)
            << "Left row must have started at or before the join instant";
        EXPECT_TRUE(right.sys_time.start <= t_after_emp_insert)
            << "Right row must have started at or before the join instant";
    }
}

// ---------------------------------------------------------------------------
// C-2: joinAsOf with never-matching predicate returns empty
// ---------------------------------------------------------------------------
TEST_F(JoinConsistencyTest, JoinAsOf_NeverMatchingPredicate_ReturnsEmpty) {
    auto always_false = [](const VersionedDocument&, const VersionedDocument&) {
        return false;
    };

    auto pairs = TemporalQueryEngine::joinAsOf(emp, dept, t_after_emp_insert, always_false);
    EXPECT_TRUE(pairs.empty())
        << "joinAsOf with a never-matching predicate must return an empty vector";
}

// ---------------------------------------------------------------------------
// C-3: joinAsOf on empty right table returns empty
// ---------------------------------------------------------------------------
TEST_F(JoinConsistencyTest, JoinAsOf_EmptyRightTable_ReturnsEmpty) {
    SystemVersionedTable empty_table{"empty", "node_a"};

    auto always_true = [](const VersionedDocument&, const VersionedDocument&) {
        return true;
    };

    auto pairs = TemporalQueryEngine::joinAsOf(emp, empty_table, t_after_emp_insert, always_true);
    EXPECT_TRUE(pairs.empty())
        << "joinAsOf against an empty table must return an empty vector";
}

// ---------------------------------------------------------------------------
// C-4: joinBiTemporal requires both sys_as_of and valid_at to match
// ---------------------------------------------------------------------------
TEST_F(JoinConsistencyTest, JoinBiTemporal_BothConditionsMustHold) {
    BiTemporalTable bt_left{"bt_orders", "node_a"};
    BiTemporalTable bt_right{"bt_items",  "node_a"};

    constexpr Timestamp kVT1_Start = 1000;
    constexpr Timestamp kVT1_End   = 5000;
    constexpr Timestamp kVT2_Start = 2000;
    constexpr Timestamp kVT2_End   = 6000;

    bt_left.insertWithValidTime("o1",  {{"item", "A"}}, {kVT1_Start, kVT1_End});
    bt_right.insertWithValidTime("i1", {{"name", "A"}}, {kVT2_Start, kVT2_End});

    auto match_all = [](const VersionedDocument&, const VersionedDocument&) {
        return true;
    };

    Timestamp sys_now = now();
    // valid_at = 3000 is inside BOTH valid-time periods → should match
    auto pairs_match = TemporalQueryEngine::joinBiTemporal(
        bt_left, bt_right, sys_now, 3000, match_all);
    EXPECT_GE(pairs_match.size(), 1u)
        << "Both rows are valid at vt=3000; join must produce a result";

    // valid_at = 100 is before BOTH valid-time periods → should not match
    auto pairs_miss = TemporalQueryEngine::joinBiTemporal(
        bt_left, bt_right, sys_now, 100, match_all);
    EXPECT_TRUE(pairs_miss.empty())
        << "No rows are valid at vt=100; join must return empty";
}

// ============================================================================
// Group D – Temporal invariants under mutation
// ============================================================================

struct TemporalMutationInvariantsTest : ::testing::Test {};

// ---------------------------------------------------------------------------
// D-1: AS_OF strictly before any insertion returns empty
// ---------------------------------------------------------------------------
TEST_F(TemporalMutationInvariantsTest, AsOf_BeforeAnyInsert_ReturnsEmpty) {
    SystemVersionedTable t{"t_d1", "n"};

    Timestamp before_insert = now();
    advanceClock();
    t.insert("k1", {{"v", 1}});

    auto rows = TemporalQueryEngine::queryAsOf(t, before_insert);
    EXPECT_TRUE(rows.empty())
        << "AS_OF before the first insertion must return an empty result";
}

// ---------------------------------------------------------------------------
// D-2: FROM_TO over full history captures all intermediate versions
// ---------------------------------------------------------------------------
TEST_F(TemporalMutationInvariantsTest, FromTo_FullRange_CapturesAllVersions) {
    SystemVersionedTable t{"t_d2", "n"};

    t.insert("k1", {{"v", 1}});
    advanceClock();
    t.update("k1", {{"v", 2}});
    advanceClock();
    t.update("k1", {{"v", 3}});

    auto rows = TemporalQueryEngine::queryFromTo(t, kMinTimestamp, kMaxTimestamp);
    // k1 now has 3 versions (insert + 2 updates)
    EXPECT_EQ(rows.size(), 3u)
        << "FROM_TO over full range must return all 3 versions of k1";
}

// ---------------------------------------------------------------------------
// D-3: Deleted rows excluded by default; included when include_deleted is set
// ---------------------------------------------------------------------------
TEST_F(TemporalMutationInvariantsTest, DeletedRows_ExcludedByDefaultIncludedWithFlag) {
    SystemVersionedTable t{"t_d3", "n"};

    // k1 is a logically-deleted row: it carries {"deleted": true} in its data.
    // This is the convention used by executeTemporalQuery's include_deleted filter
    // (see applyDeletedFilter in temporal_query_engine.cpp).
    t.insert("k1", {{"v", 1}, {"deleted", true}});
    t.insert("k2", {{"v", 2}});
    advanceClock();

    Timestamp after_insert = now();

    // Without include_deleted (default false): only k2 must be visible.
    auto rows_default = TemporalQueryEngine::executeTemporalQuery(
        t, TemporalQuerySpec::asOf(after_insert));
    EXPECT_EQ(rows_default.size(), 1u)
        << "AS_OF must exclude the logically-deleted row by default";

    // With ALL + include_deleted=true: both rows must appear.
    TemporalQuerySpec all_spec = TemporalQuerySpec::all();
    all_spec.include_deleted   = true;
    auto rows_all = TemporalQueryEngine::executeTemporalQuery(t, all_spec);
    EXPECT_GE(rows_all.size(), 2u)
        << "ALL with include_deleted=true must include the deleted row";
}

// ---------------------------------------------------------------------------
// D-4: BiTemporalTable uniqueness conflict prevents double-insertion
// ---------------------------------------------------------------------------
TEST_F(TemporalMutationInvariantsTest, BiTemporal_UniqueConstraint_PreventsDuplicateInsert) {
    BiTemporalTable bt{"bt_d4", "n"};

    TimeRange vt1{1000, 4000};
    EXPECT_TRUE(bt.insertWithValidTime("c1", {{"v", 1}}, vt1))
        << "First insertion must succeed";

    // Overlapping valid-time period → must fail
    TimeRange vt_overlap{2000, 5000};
    EXPECT_FALSE(bt.insertWithValidTime("c1", {{"v", 2}}, vt_overlap))
        << "Overlapping valid-time insert must be rejected";

    // hasUniquenessConflict must report the conflict before attempting insertion
    EXPECT_TRUE(bt.hasUniquenessConflict("c1", vt_overlap))
        << "hasUniquenessConflict must return true for the overlapping period";

    // Non-overlapping period → must succeed
    TimeRange vt_gap{5000, 8000};
    EXPECT_FALSE(bt.hasUniquenessConflict("c1", vt_gap))
        << "Non-overlapping period must not trigger a uniqueness conflict";
    EXPECT_TRUE(bt.insertWithValidTime("c1", {{"v", 3}}, vt_gap))
        << "Non-overlapping valid-time insert must succeed";
}

// ---------------------------------------------------------------------------
// D-5: BiTemporalTable::findGaps reports gaps consistent with queryApplicationTime
// ---------------------------------------------------------------------------
TEST_F(TemporalMutationInvariantsTest, BiTemporal_FindGaps_ConsistentWithQueryResults) {
    BiTemporalTable bt{"bt_d5", "n"};

    // Insert two non-adjacent valid-time periods leaving a gap in [4000, 6000)
    bt.insertWithValidTime("contract",
                           {{"status", "active"}}, {1000, 4000});
    bt.insertWithValidTime("contract",
                           {{"status", "renewed"}}, {6000, 9000});

    // findGaps over [1000, 9000) must identify [4000, 6000)
    auto gaps = bt.findGaps("contract", 1000, 9000);
    ASSERT_EQ(gaps.size(), 1u) << "Exactly one gap expected between periods";
    EXPECT_EQ(gaps[0].start, 4000) << "Gap must start at 4000";
    EXPECT_EQ(gaps[0].end,   6000) << "Gap must end at 6000";

    // queryApplicationTime at a gap timestamp must return empty
    auto rows_in_gap = TemporalQueryEngine::queryApplicationTime(bt, 5000);
    EXPECT_TRUE(rows_in_gap.empty())
        << "No row is valid at a gap timestamp; queryApplicationTime must return empty";

    // queryApplicationTime inside each covered period must return a row
    auto rows_p1 = TemporalQueryEngine::queryApplicationTime(bt, 2000);
    EXPECT_EQ(rows_p1.size(), 1u)
        << "queryApplicationTime at vt=2000 must return the first contract row";

    auto rows_p2 = TemporalQueryEngine::queryApplicationTime(bt, 7000);
    EXPECT_EQ(rows_p2.size(), 1u)
        << "queryApplicationTime at vt=7000 must return the second contract row";
}
