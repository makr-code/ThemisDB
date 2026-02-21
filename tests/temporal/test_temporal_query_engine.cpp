/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_temporal_query_engine.cpp                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Tests for TemporalQueryEngine
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_query_engine.h"

using namespace themisdb::temporal;

class TemporalQueryEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // emp1: Insert → Update
        table.insert("emp1", {{"name", "Alice"}, {"dept", "Engineering"}});
        t_after_emp1_insert = now();
        table.update("emp1", {{"salary", 55000}});

        // emp2: Insert only
        table.insert("emp2", {{"name", "Bob"}, {"dept", "Sales"}});
    }

    SystemVersionedTable table{"test_table", "node_a"};
    Timestamp t_after_emp1_insert{0};
};

// ── queryAsOf ────────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryAsOf_Now_ReturnsBothCurrentRows) {
    auto rows = TemporalQueryEngine::queryAsOf(table, now());
    EXPECT_EQ(rows.size(), 2u);
}

TEST_F(TemporalQueryEngineTest, QueryAsOf_WithFilter_ReturnsMatchingRows) {
    auto rows = TemporalQueryEngine::queryAsOf(
        table, now(),
        {RowFilter{"dept", "Engineering"}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].data["name"], "Alice");
}

TEST_F(TemporalQueryEngineTest, QueryAsOf_FilterNoMatch_ReturnsEmpty) {
    auto rows = TemporalQueryEngine::queryAsOf(
        table, now(),
        {RowFilter{"dept", "Marketing"}});
    EXPECT_TRUE(rows.empty());
}

// ── queryFromTo ──────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryFromTo_FullRange_ReturnsAllVersions) {
    auto rows = TemporalQueryEngine::queryFromTo(table, kMinTimestamp, kMaxTimestamp);
    // emp1 has 2 versions, emp2 has 1 → total 3
    EXPECT_EQ(rows.size(), 3u);
}

TEST_F(TemporalQueryEngineTest, QueryKeyFromTo_ReturnsVersionsInRange) {
    // Range from t_after_emp1_insert onwards should include the update version
    auto rows = TemporalQueryEngine::queryKeyFromTo(
        table, "emp1", t_after_emp1_insert, kMaxTimestamp);
    // At least the current version (after update)
    EXPECT_GE(rows.size(), 1u);
}

// ── evaluatePredicate ────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, Predicate_Overlaps_True) {
    TimeRange a{100, 300};
    TimeRange b{200, 400};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::OVERLAPS, a, b));
}

TEST_F(TemporalQueryEngineTest, Predicate_Overlaps_False) {
    TimeRange a{100, 200};
    TimeRange b{200, 300};
    // [100,200) and [200,300) do not overlap (half-open interval)
    EXPECT_FALSE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::OVERLAPS, a, b));
}

TEST_F(TemporalQueryEngineTest, Predicate_Contains_Period) {
    TimeRange outer{100, 500};
    TimeRange inner{200, 400};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::CONTAINS, outer, inner));
    EXPECT_FALSE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::CONTAINS, inner, outer));
}

TEST_F(TemporalQueryEngineTest, Predicate_Precedes) {
    TimeRange a{100, 200};
    TimeRange b{200, 300};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::PRECEDES, a, b));
    EXPECT_FALSE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::PRECEDES, b, a));
}

TEST_F(TemporalQueryEngineTest, Predicate_Meets) {
    TimeRange a{100, 200};
    TimeRange b{200, 300};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::MEETS, a, b));
}

TEST_F(TemporalQueryEngineTest, Predicate_Equals) {
    TimeRange a{100, 200};
    TimeRange b{100, 200};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::EQUALS, a, b));
    TimeRange c{100, 201};
    EXPECT_FALSE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::EQUALS, a, c));
}

TEST_F(TemporalQueryEngineTest, Predicate_Succeeds) {
    TimeRange a{300, 400};
    TimeRange b{100, 300};
    EXPECT_TRUE(TemporalQueryEngine::evaluatePredicate(
        TemporalOperator::SUCCEEDS, a, b));
}

// ── intersect ────────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, Intersect_Overlap_ReturnsOverlapRange) {
    TimeRange a{100, 300};
    TimeRange b{200, 400};
    auto result = TemporalQueryEngine::intersect(a, b);
    EXPECT_EQ(result.start, 200);
    EXPECT_EQ(result.end, 300);
}

TEST_F(TemporalQueryEngineTest, Intersect_NoOverlap_ReturnsEmptyRange) {
    TimeRange a{100, 200};
    TimeRange b{300, 400};
    auto result = TemporalQueryEngine::intersect(a, b);
    EXPECT_EQ(result.start, result.end); // empty
}


// ── joinAsOf ──────────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, JoinAsOf_MatchingRows_ReturnsJoinedPairs) {
    SystemVersionedTable employees{"employees", "n"};
    SystemVersionedTable departments{"departments", "n"};

    employees.insert("emp1", {{"name", "Alice"}, {"dept_id", "d1"}});
    employees.insert("emp2", {{"name", "Bob"},   {"dept_id", "d2"}});
    departments.insert("d1", {{"id", "d1"}, {"name", "Engineering"}});
    departments.insert("d2", {{"id", "d2"}, {"name", "Sales"}});

    auto pairs = TemporalQueryEngine::joinAsOf(
        employees, departments, kMaxTimestamp,
        [](const VersionedDocument& emp, const VersionedDocument& dept) {
            return emp.data.value("dept_id", "") == dept.data.value("id", "");
        });

    ASSERT_EQ(pairs.size(), 2u);
    // Verify both employees got matched to a department
    bool alice_matched = false, bob_matched = false;
    for (const auto& [emp, dept] : pairs) {
        if (emp.data["name"] == "Alice" && dept.data["name"] == "Engineering")
            alice_matched = true;
        if (emp.data["name"] == "Bob" && dept.data["name"] == "Sales")
            bob_matched = true;
    }
    EXPECT_TRUE(alice_matched);
    EXPECT_TRUE(bob_matched);
}

TEST_F(TemporalQueryEngineTest, JoinAsOf_NoMatch_ReturnsEmpty) {
    SystemVersionedTable t1{"t1", "n"};
    SystemVersionedTable t2{"t2", "n"};
    t1.insert("k1", {{"x", 1}});
    t2.insert("k2", {{"y", 2}});

    auto pairs = TemporalQueryEngine::joinAsOf(
        t1, t2, kMaxTimestamp,
        [](const VersionedDocument& l, const VersionedDocument& r) {
            return l.data.value("x", 0) == r.data.value("y", 0); // no match
        });

    EXPECT_TRUE(pairs.empty());
}

TEST_F(TemporalQueryEngineTest, JoinAsOf_EmptyTable_ReturnsEmpty) {
    SystemVersionedTable t1{"t1", "n"};
    SystemVersionedTable t2{"t2", "n"};
    t1.insert("k1", {{"x", 1}});
    // t2 is empty

    auto pairs = TemporalQueryEngine::joinAsOf(
        t1, t2, kMaxTimestamp,
        [](const VersionedDocument&, const VersionedDocument&) { return true; });

    EXPECT_TRUE(pairs.empty());
}
