/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_temporal_query_engine.cpp                     ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:02:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
namespace tq_detail = themisdb::temporal::detail;

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

// ── joinBiTemporal ────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, JoinBiTemporal_MatchingRows_ReturnsPairs) {
    BiTemporalTable employees{"employees", "n"};
    BiTemporalTable departments{"departments", "n"};

    employees.insertWithValidTime("emp1", {{"name", "Alice"}, {"dept_id", "d1"}}, {1000, 9000});
    employees.insertWithValidTime("emp2", {{"name", "Bob"},   {"dept_id", "d2"}}, {1000, 9000});
    departments.insertWithValidTime("d1", {{"id", "d1"}, {"name", "Engineering"}}, {1000, 9000});
    departments.insertWithValidTime("d2", {{"id", "d2"}, {"name", "Sales"}},       {1000, 9000});

    auto pairs = TemporalQueryEngine::joinBiTemporal(
        employees, departments, now(), 5000,
        [](const VersionedDocument& emp, const VersionedDocument& dept) {
            return emp.data.value("dept_id", "") == dept.data.value("id", "");
        });

    ASSERT_EQ(pairs.size(), 2u);
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

TEST_F(TemporalQueryEngineTest, JoinBiTemporal_ValidTimeOutOfRange_ReturnsEmpty) {
    BiTemporalTable left{"left", "n"};
    BiTemporalTable right{"right", "n"};

    left.insertWithValidTime("k1",  {{"x", 1}}, {1000, 2000});
    right.insertWithValidTime("k2", {{"y", 1}}, {1000, 2000});

    // valid_at=5000 falls outside [1000,2000) for both tables
    auto pairs = TemporalQueryEngine::joinBiTemporal(
        left, right, now(), 5000,
        [](const VersionedDocument&, const VersionedDocument&) { return true; });

    EXPECT_TRUE(pairs.empty());
}

TEST_F(TemporalQueryEngineTest, JoinBiTemporal_DifferentValidTimes_PartialMatch) {
    BiTemporalTable left{"left", "n"};
    BiTemporalTable right{"right", "n"};

    // left row valid in [1000, 3000), right row valid in [2000, 5000)
    // At valid_at=2500 both are valid; at valid_at=1500 only left is valid
    left.insertWithValidTime("k1",  {{"id", "k1"}}, {1000, 3000});
    right.insertWithValidTime("k1", {{"id", "k1"}}, {2000, 5000});

    auto at_2500 = TemporalQueryEngine::joinBiTemporal(
        left, right, now(), 2500,
        [](const VersionedDocument& l, const VersionedDocument& r) {
            return l.data.value("id", "") == r.data.value("id", "");
        });
    EXPECT_EQ(at_2500.size(), 1u);

    auto at_1500 = TemporalQueryEngine::joinBiTemporal(
        left, right, now(), 1500,
        [](const VersionedDocument& l, const VersionedDocument& r) {
            return l.data.value("id", "") == r.data.value("id", "");
        });
    EXPECT_TRUE(at_1500.empty());
}

TEST_F(TemporalQueryEngineTest, JoinBiTemporal_EmptyTables_ReturnsEmpty) {
    BiTemporalTable left{"left", "n"};
    BiTemporalTable right{"right", "n"};

    auto pairs = TemporalQueryEngine::joinBiTemporal(
        left, right, now(), 1000,
        [](const VersionedDocument&, const VersionedDocument&) { return true; });

    EXPECT_TRUE(pairs.empty());
}

// ── queryWithSemantics ────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QuerySequenced_PeriodOverlap_ReturnsVersionsInRange) {
    // A period from t_after_emp1_insert to kMaxTimestamp should include:
    //   emp1 current version (opened after the update, sys_start > t_after_emp1_insert)
    //   emp2 only version (inserted after emp1 insert, so sys_start > t_after_emp1_insert)
    // emp1's original version was closed at t_after_emp1_insert, so its sys_end
    // is approximately t_after_emp1_insert; that closed version may or may not
    // overlap depending on exact timestamps.  We verify at least 2 rows are
    // returned (emp1 current + emp2) and that the result contains emp2.
    auto rows = TemporalQueryEngine::queryWithSemantics(
        table,
        TemporalSemantics::SEQUENCED,
        {t_after_emp1_insert, kMaxTimestamp});
    EXPECT_GE(rows.size(), 2u);
    bool emp2_found = false;
    for (const auto& r : rows) {
        if (r.data.value("name", "") == "Bob") emp2_found = true;
    }
    EXPECT_TRUE(emp2_found);
}

TEST_F(TemporalQueryEngineTest, QueryNonSequenced_ReturnsAllVersions) {
    // NON_SEQUENCED should return every stored version (emp1 has 2, emp2 has 1)
    auto rows = TemporalQueryEngine::queryWithSemantics(
        table,
        TemporalSemantics::NON_SEQUENCED,
        {kMinTimestamp, kMaxTimestamp});
    EXPECT_EQ(rows.size(), 3u); // emp1 × 2 + emp2 × 1
}

TEST_F(TemporalQueryEngineTest, QueryNonSequenced_WithFilter_FiltersAcrossAllVersions) {
    auto rows = TemporalQueryEngine::queryWithSemantics(
        table,
        TemporalSemantics::NON_SEQUENCED,
        {kMinTimestamp, kMaxTimestamp},
        {RowFilter{"dept", "Engineering"}});
    // Both emp1 versions have dept=Engineering; emp2 has dept=Sales
    EXPECT_EQ(rows.size(), 2u);
}

TEST_F(TemporalQueryEngineTest, QuerySequenced_EmptyPeriod_ReturnsEmpty) {
    // A period wholly before any data was inserted returns nothing
    auto rows = TemporalQueryEngine::queryWithSemantics(
        table,
        TemporalSemantics::SEQUENCED,
        {kMinTimestamp, 0}); // time 0 is before any realistic wall-clock insert
    EXPECT_TRUE(rows.empty());
}

// ── queryBetween ──────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryBetween_FullRange_ReturnsAllVersions) {
    // BETWEEN kMinTimestamp AND kMaxTimestamp must return every version
    auto rows = TemporalQueryEngine::queryBetween(table, kMinTimestamp, kMaxTimestamp);
    // emp1 has 2 versions, emp2 has 1 → total 3
    EXPECT_EQ(rows.size(), 3u);
}

TEST_F(TemporalQueryEngineTest, QueryBetween_NarrowRange_ReturnsVersionsInRange) {
    // A range centred on now() should include at least the two current rows
    Timestamp t_now = now();
    auto rows = TemporalQueryEngine::queryBetween(table, t_now - 60000, t_now);
    EXPECT_GE(rows.size(), 2u); // at least both current rows
}

TEST_F(TemporalQueryEngineTest, QueryBetween_BeforeAnyInsert_ReturnsEmpty) {
    // A range wholly before any realistic wall-clock data is empty
    auto rows = TemporalQueryEngine::queryBetween(table, kMinTimestamp, 0);
    EXPECT_TRUE(rows.empty());
}

TEST_F(TemporalQueryEngineTest, QueryBetween_WithFilter_ReturnsFilteredVersions) {
    auto rows = TemporalQueryEngine::queryBetween(
        table, kMinTimestamp, kMaxTimestamp,
        {RowFilter{"dept", "Sales"}});
    // Only emp2 has dept=Sales
    ASSERT_GE(rows.size(), 1u);
    for (const auto& r : rows) {
        EXPECT_EQ(r.data.value("dept", ""), "Sales");
    }
}

// ── queryApplicationTime ──────────────────────────────────────────────────────

class ApplicationTimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Contract c1: valid [1000, 5000)
        table.insertWithValidTime("c1", {{"type", "NDA"},  {"client", "Acme"}},  {1000, 5000});
        // Contract c2: valid [3000, 9000)
        table.insertWithValidTime("c2", {{"type", "SLA"},  {"client", "Beta"}},  {3000, 9000});
        // Contract c3: valid [6000, 8000)
        table.insertWithValidTime("c3", {{"type", "MSA"},  {"client", "Gamma"}}, {6000, 8000});
    }

    BiTemporalTable table{"contracts", "node_a"};
};

TEST_F(ApplicationTimeTest, QueryApplicationTime_PointInRange_ReturnsActiveRows) {
    // At valid_at=4000: c1 [1000,5000) and c2 [3000,9000) are active
    auto rows = TemporalQueryEngine::queryApplicationTime(table, 4000);
    EXPECT_EQ(rows.size(), 2u);
    bool c1_found = false, c2_found = false;
    for (const auto& r : rows) {
        if (r.key == "c1") c1_found = true;
        if (r.key == "c2") c2_found = true;
    }
    EXPECT_TRUE(c1_found);
    EXPECT_TRUE(c2_found);
}

TEST_F(ApplicationTimeTest, QueryApplicationTime_PointAfterAllExpiry_ReturnsEmpty) {
    // At valid_at=10000: no contracts are active
    auto rows = TemporalQueryEngine::queryApplicationTime(table, 10000);
    EXPECT_TRUE(rows.empty());
}

TEST_F(ApplicationTimeTest, QueryApplicationTime_WithFilter_ReturnsFilteredRows) {
    // At valid_at=7000: c2 [3000,9000) and c3 [6000,8000) are active
    auto rows = TemporalQueryEngine::queryApplicationTime(
        table, 7000, {RowFilter{"client", "Gamma"}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].key, "c3");
}

TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_OverlappingRange_ReturnsMatchingRows) {
    // Range [4500, 7500): overlaps c2 [3000,9000) and c3 [6000,8000)
    // c1 ends at 5000, [4500,7500) overlaps [1000,5000) → c1 also included
    auto rows = TemporalQueryEngine::queryApplicationTimeRange(table, 4500, 7500);
    EXPECT_GE(rows.size(), 2u); // at least c2 and c3
}

TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_NoOverlap_ReturnsEmpty) {
    // Range [9000, 10000) does not overlap any contract
    auto rows = TemporalQueryEngine::queryApplicationTimeRange(table, 9000, 10000);
    EXPECT_TRUE(rows.empty());
}

TEST_F(ApplicationTimeTest, QueryApplicationTimeRange_FullRange_ReturnsAllRows) {
    auto rows = TemporalQueryEngine::queryApplicationTimeRange(
        table, kMinTimestamp, kMaxTimestamp);
    EXPECT_EQ(rows.size(), 3u);
}

// ── queryAsOfWithIndex ────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_PopulatedIndex_ReturnsCorrectRows) {
    // Build a TemporalIndex from the table's current snapshot
    TemporalIndex index{"test_index"};
    Timestamp t_now = now();
    auto all = TemporalQueryEngine::queryAsOf(table, t_now);
    for (const auto& row : all) {
        index.insert({row.key, row.sys_time, row.data});
    }

    auto rows = TemporalQueryEngine::queryAsOfWithIndex(table, index, t_now);
    EXPECT_EQ(rows.size(), 2u); // emp1 and emp2 current rows
}

TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_EmptyIndex_FallsBackToFullScan) {
    TemporalIndex empty_index{"empty"};
    // An empty (uninitialized) index falls back to queryAsOf → both current rows.
    auto rows = TemporalQueryEngine::queryAsOfWithIndex(table, empty_index, now());
    EXPECT_EQ(rows.size(), 2u);
}

TEST_F(TemporalQueryEngineTest, QueryAsOfWithIndex_PopulatedIndexNoMatch_ReturnsEmpty) {
    // Build an index that covers a time range wholly in the past (e.g. t=1..2).
    TemporalIndex populated_index{"old_index"};
    populated_index.insert({"emp1", {1, 2}, {}});

    // Querying with a current timestamp finds no candidates, but since the
    // index is populated we should get an empty result (not a fallback scan).
    auto rows = TemporalQueryEngine::queryAsOfWithIndex(
        table, populated_index, now());
    EXPECT_TRUE(rows.empty());
}

// ── QueryCache ────────────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryCache_MissAndHit) {
    QueryCache cache(32);
    Timestamp t_now = now();

    // First access: cache miss
    EXPECT_FALSE(cache.get("t", t_now).has_value());

    // Populate cache
    auto rows = TemporalQueryEngine::queryAsOf(table, t_now);
    cache.put("t", t_now, rows);

    // Second access: cache hit
    auto hit = cache.get("t", t_now);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->size(), rows.size());
}

TEST_F(TemporalQueryEngineTest, QueryCache_InvalidateByTable) {
    QueryCache cache(32);
    Timestamp t = 12345;
    cache.put("tbl", t, {});
    EXPECT_TRUE(cache.get("tbl", t).has_value());

    cache.invalidate("tbl");
    EXPECT_FALSE(cache.get("tbl", t).has_value());
}

TEST_F(TemporalQueryEngineTest, QueryCache_Clear_RemovesAllEntries) {
    QueryCache cache(32);
    cache.put("a", 1, {});
    cache.put("b", 2, {});
    EXPECT_EQ(cache.size(), 2u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
}

TEST_F(TemporalQueryEngineTest, QueryCache_EvictsLRUWhenFull) {
    QueryCache cache(2); // max 2 entries
    cache.put("tbl", 1, {});
    cache.put("tbl", 2, {});
    // Evict entry 1 (LRU) by inserting a third
    cache.put("tbl", 3, {});
    EXPECT_EQ(cache.size(), 2u);
    // Entry 3 must be present; entry 1 must have been evicted
    EXPECT_TRUE(cache.get("tbl", 3).has_value());
    EXPECT_FALSE(cache.get("tbl", 1).has_value());
}

// ── queryAsOfCached ───────────────────────────────────────────────────────────

TEST_F(TemporalQueryEngineTest, QueryAsOfCached_SecondCallHitsCache) {
    QueryCache cache(32);
    Timestamp t_now = now();

    auto r1 = tq_detail::queryAsOfCached(table, t_now, cache);
    EXPECT_EQ(r1.size(), 2u);

    // Cache must now contain the entry
    EXPECT_TRUE(cache.get(table.tableName(), t_now).has_value());

    auto r2 = tq_detail::queryAsOfCached(table, t_now, cache);
    EXPECT_EQ(r1.size(), r2.size());
}

TEST_F(TemporalQueryEngineTest, QueryAsOfCached_WithFilter_FiltersPostCache) {
    QueryCache cache(32);
    Timestamp t_now = now();

    auto rows = tq_detail::queryAsOfCached(
        table, t_now, cache, {RowFilter{"dept", "Engineering"}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].data["name"], "Alice");

    // The cache should store the unfiltered result (size 2)
    auto cached = cache.get(table.tableName(), t_now);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->size(), 2u);
}
