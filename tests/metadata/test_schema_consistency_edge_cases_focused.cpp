// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_schema_consistency_edge_cases_focused.cpp
 * @brief Phase B1: Edge-case stress tests for SchemaConsistencyChecker.
 * @note Test IDs: MCH-C01..MCH-C08
 *
 *   MCH-C01  ConsistencyIssue default-constructed has empty fields
 *   MCH-C02  ConsistencyIssue toJSON() with empty table_name serializes cleanly
 *   MCH-C03  Multiple ConsistencyIssues can be collected into a vector
 *   MCH-C04  ConsistencyIssue detail field survives round-trip through toJSON()
 *   MCH-C05  issue_type "orphan_key" serializes to correct JSON string
 *   MCH-C06  issue_type "stale_stats" serializes to correct JSON string
 *   MCH-C07  issue_type "missing_constraint" serializes to correct JSON string
 *   MCH-C08  Vector of issues can be serialized to a JSON array
 *
 * Self-contained: no RocksDB, no background threads.
 * Canonical PRNG seed: kConsistencySeed = 42.
 *
 * @see include/metadata/schema_consistency_checker.h
 * @see src/metadata/ROADMAP.md — Phase B stress-coverage items
 */

#include <gtest/gtest.h>

#include "metadata/schema_consistency_checker.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis;

namespace {

[[maybe_unused]] static constexpr uint64_t kConsistencySeed = 42;

ConsistencyIssue makeIssue(const std::string& type,
                            const std::string& table,
                            const std::string& column,
                            const std::string& detail) {
    ConsistencyIssue ci;
    ci.issue_type  = type;
    ci.table_name  = table;
    ci.column_name = column;
    ci.detail      = detail;
    return ci;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-C01: ConsistencyIssue default-constructed has empty fields
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC01_DefaultConstructedIsEmpty) {
    ConsistencyIssue ci;
    EXPECT_TRUE(ci.issue_type.empty());
    EXPECT_TRUE(ci.table_name.empty());
    EXPECT_TRUE(ci.column_name.empty());
    EXPECT_TRUE(ci.detail.empty());
}

// ---------------------------------------------------------------------------
// MCH-C02: ConsistencyIssue toJSON() with empty table_name serializes cleanly
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC02_EmptyTableNameSerializes) {
    const auto ci = makeIssue("stale_stats", /*table=*/"", /*column=*/"", "Global issue");
    ASSERT_NO_THROW({
        const auto j = ci.toJSON();
        EXPECT_EQ(j["table_name"].get<std::string>(), "");
        EXPECT_EQ(j["detail"].get<std::string>(), "Global issue");
    });
}

// ---------------------------------------------------------------------------
// MCH-C03: Multiple ConsistencyIssues can be collected into a vector
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC03_VectorOfIssues) {
    std::vector<ConsistencyIssue> issues;
    issues.push_back(makeIssue("orphan_key",         "orders",   "id",      "Orphaned key"));
    issues.push_back(makeIssue("stale_stats",        "users",    "",        "Stats too old"));
    issues.push_back(makeIssue("missing_constraint", "sessions", "",        "No constraints"));

    ASSERT_EQ(issues.size(), 3u);
    EXPECT_EQ(issues[0].issue_type, "orphan_key");
    EXPECT_EQ(issues[1].issue_type, "stale_stats");
    EXPECT_EQ(issues[2].issue_type, "missing_constraint");
}

// ---------------------------------------------------------------------------
// MCH-C04: ConsistencyIssue detail field survives round-trip through toJSON()
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC04_DetailFieldRoundTrip) {
    const std::string detail_text = "Stats older than 86400 seconds on table 'users'";
    const auto ci = makeIssue("stale_stats", "users", "", detail_text);
    const auto j  = ci.toJSON();

    EXPECT_EQ(j["detail"].get<std::string>(), detail_text);
}

// ---------------------------------------------------------------------------
// MCH-C05: issue_type "orphan_key" serializes to correct JSON string
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC05_OrphanKeyIssueTypeSerializes) {
    const auto ci = makeIssue("orphan_key", "catalog", "version", "Unknown prefix");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "orphan_key");
}

// ---------------------------------------------------------------------------
// MCH-C06: issue_type "stale_stats" serializes to correct JSON string
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC06_StaleStatsIssueTypeSerializes) {
    const auto ci = makeIssue("stale_stats", "metrics", "value", "Stats expired");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "stale_stats");
}

// ---------------------------------------------------------------------------
// MCH-C07: issue_type "missing_constraint" serializes to correct JSON string
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC07_MissingConstraintIssueTypeSerializes) {
    const auto ci = makeIssue("missing_constraint", "accounts", "", "No constraints defined");
    EXPECT_EQ(ci.toJSON()["issue_type"].get<std::string>(), "missing_constraint");
}

// ---------------------------------------------------------------------------
// MCH-C08: Vector of issues can be serialized to a JSON array
// ---------------------------------------------------------------------------
TEST(SchemaConsistencyEdgeCasesTest, MCHC08_VectorSerializesToJsonArray) {
    std::vector<ConsistencyIssue> issues;
    issues.push_back(makeIssue("orphan_key",         "t1", "", "d1"));
    issues.push_back(makeIssue("stale_stats",        "t2", "", "d2"));
    issues.push_back(makeIssue("missing_constraint", "t3", "", "d3"));

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& ci : issues) {
        arr.push_back(ci.toJSON());
    }

    ASSERT_EQ(arr.size(), 3u);
    EXPECT_EQ(arr[0]["issue_type"].get<std::string>(), "orphan_key");
    EXPECT_EQ(arr[1]["issue_type"].get<std::string>(), "stale_stats");
    EXPECT_EQ(arr[2]["issue_type"].get<std::string>(), "missing_constraint");
}
