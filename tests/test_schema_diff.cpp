/**
 * Test: Schema Diff Engine
 *
 * Tests for SchemaDiff / SchemaDiffEngine:
 *
 * Acceptance criteria:
 *   AC-DIFF-1  identical schemas produce empty diff (isEmpty()==true)
 *   AC-DIFF-2  added column is reported as ADDED
 *   AC-DIFF-3  removed column is reported as REMOVED
 *   AC-DIFF-4  type change is reported as TYPE_CHANGED with correct old/new values
 *   AC-DIFF-5  nullability change is reported as NULLABILITY_CHANGED
 *   AC-DIFF-6  index change is reported as INDEX_CHANGED
 *   AC-DIFF-7  added index reported as ADDED
 *   AC-DIFF-8  removed index reported as REMOVED
 *   AC-DIFF-9  changed index (type changed) reported as CHANGED
 *   AC-DIFF-10 addedColumnCount / removedColumnCount / modifiedColumnCount are correct
 *   AC-DIFF-11 column_diffs sorted alphabetically by column_name
 *   AC-DIFF-12 index_diffs sorted alphabetically by index_name
 *   AC-DIFF-13 toJSON contains expected fields (table_name, column_diffs, index_diffs, summary)
 *   AC-DIFF-14 diff with empty 'from' schema gives all columns ADDED
 *   AC-DIFF-15 ColumnDiff::toJSON and IndexDiff::toJSON produce correct fields
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/schema_diff.h"

using namespace themis;           // for SchemaManager
using namespace themis::metadata; // for SchemaDiffEngine, SchemaDiff, etc.

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a TableSchema from a concise column specification
// ─────────────────────────────────────────────────────────────────────────────

struct ColumnSpec {
    std::string name;
    std::string type = {};
    bool        nullable   = true;
    bool        indexed    = false;
    std::string index_type = "";
};

struct IndexSpec {
    std::string              name;
    std::string              type;
    std::vector<std::string> columns;
    bool                     unique = false;
};

static SchemaManager::TableSchema makeSchema(
    const std::string&             table_name,
    const std::vector<ColumnSpec>& cols,
    const std::vector<IndexSpec>&  idxs = {})
{
    SchemaManager::TableSchema s;
    s.name = table_name;
    s.type = "relational";

    for (const auto& c : cols) {
        SchemaManager::PropertyInfo p;
        p.name       = c.name;
        p.type       = c.type;
        p.nullable   = c.nullable;
        p.indexed    = c.indexed;
        p.index_type = c.index_type;
        s.properties.push_back(p);
    }

    for (const auto& i : idxs) {
        SchemaManager::IndexInfo idx;
        idx.name    = i.name;
        idx.type    = i.type;
        idx.columns = i.columns;
        idx.unique  = i.unique;
        s.indexes.push_back(idx);
    }

    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-1 — identical schemas produce empty diff
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, IdenticalSchemasProduceEmptyDiff) {
    auto schema = makeSchema("orders",
        {{"id", "integer", false, true, "regular"},
         {"name", "string", true, false, ""}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(schema, schema);

    EXPECT_TRUE(diff.isEmpty());
    EXPECT_TRUE(diff.column_diffs.empty());
    EXPECT_TRUE(diff.index_diffs.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-2 — added column is reported as ADDED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, AddedColumnReportedAsAdded) {
    auto from = makeSchema("t", {{"id", "integer"}});
    auto to   = makeSchema("t", {{"id", "integer"}, {"email", "string"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_FALSE(diff.isEmpty());
    ASSERT_EQ(diff.column_diffs.size(), 1u);
    EXPECT_EQ(diff.column_diffs[0].diff_type,   ColumnDiffType::ADDED);
    EXPECT_EQ(diff.column_diffs[0].column_name, "email");
    EXPECT_FALSE(diff.column_diffs[0].old_value.has_value());
    EXPECT_TRUE(diff.column_diffs[0].new_value.has_value());
    EXPECT_EQ(diff.column_diffs[0].new_value.value(), "string");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-3 — removed column is reported as REMOVED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, RemovedColumnReportedAsRemoved) {
    auto from = makeSchema("t", {{"id", "integer"}, {"tmp", "string"}});
    auto to   = makeSchema("t", {{"id", "integer"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_EQ(diff.column_diffs.size(), 1u);
    EXPECT_EQ(diff.column_diffs[0].diff_type,   ColumnDiffType::REMOVED);
    EXPECT_EQ(diff.column_diffs[0].column_name, "tmp");
    EXPECT_TRUE(diff.column_diffs[0].old_value.has_value());
    EXPECT_FALSE(diff.column_diffs[0].new_value.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-4 — type change is reported as TYPE_CHANGED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, TypeChangeReportedAsTypeChanged) {
    auto from = makeSchema("t", {{"score", "integer"}});
    auto to   = makeSchema("t", {{"score", "double"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_FALSE(diff.column_diffs.empty());
    auto it = std::find_if(diff.column_diffs.begin(), diff.column_diffs.end(),
        [](const ColumnDiff& d) {
            return d.diff_type == ColumnDiffType::TYPE_CHANGED;
        });
    ASSERT_NE(it, diff.column_diffs.end());
    EXPECT_EQ(it->column_name, "score");
    EXPECT_EQ(it->old_value.value(), "integer");
    EXPECT_EQ(it->new_value.value(), "double");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-5 — nullability change is reported as NULLABILITY_CHANGED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, NullabilityChangeReportedAsNullabilityChanged) {
    auto from = makeSchema("t", {{"email", "string", /*nullable=*/true}});
    auto to   = makeSchema("t", {{"email", "string", /*nullable=*/false}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    auto it = std::find_if(diff.column_diffs.begin(), diff.column_diffs.end(),
        [](const ColumnDiff& d) {
            return d.diff_type == ColumnDiffType::NULLABILITY_CHANGED;
        });
    ASSERT_NE(it, diff.column_diffs.end());
    EXPECT_EQ(it->column_name, "email");
    EXPECT_EQ(it->old_value.value(), "true");
    EXPECT_EQ(it->new_value.value(), "false");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-6 — index change is reported as INDEX_CHANGED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, IndexChangeReportedAsIndexChanged) {
    auto from = makeSchema("t", {{"col", "string", true, /*indexed=*/false, ""}});
    auto to   = makeSchema("t", {{"col", "string", true, /*indexed=*/true, "regular"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    auto it = std::find_if(diff.column_diffs.begin(), diff.column_diffs.end(),
        [](const ColumnDiff& d) {
            return d.diff_type == ColumnDiffType::INDEX_CHANGED;
        });
    ASSERT_NE(it, diff.column_diffs.end());
    EXPECT_EQ(it->column_name, "col");
    EXPECT_EQ(it->old_value.value(), "none");
    EXPECT_EQ(it->new_value.value(), "regular");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-7/8/9 — index-level diffs: ADDED, REMOVED, CHANGED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, AddedIndexReportedAsAdded) {
    auto from = makeSchema("t", {{"id", "integer"}});
    auto to   = makeSchema("t", {{"id", "integer"}},
                           {{"idx_id", "regular", {"id"}, false}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_EQ(diff.index_diffs.size(), 1u);
    EXPECT_EQ(diff.index_diffs[0].diff_type,  IndexDiffType::ADDED);
    EXPECT_EQ(diff.index_diffs[0].index_name, "idx_id");
}

TEST(SchemaDiffFocusedTests, RemovedIndexReportedAsRemoved) {
    auto from = makeSchema("t", {{"id", "integer"}},
                           {{"idx_id", "regular", {"id"}, false}});
    auto to   = makeSchema("t", {{"id", "integer"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_EQ(diff.index_diffs.size(), 1u);
    EXPECT_EQ(diff.index_diffs[0].diff_type,  IndexDiffType::REMOVED);
    EXPECT_EQ(diff.index_diffs[0].index_name, "idx_id");
}

TEST(SchemaDiffFocusedTests, ChangedIndexTypeReportedAsChanged) {
    auto from = makeSchema("t", {{"col", "string"}},
                           {{"idx_col", "regular", {"col"}, false}});
    auto to   = makeSchema("t", {{"col", "string"}},
                           {{"idx_col", "fulltext", {"col"}, false}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_EQ(diff.index_diffs.size(), 1u);
    EXPECT_EQ(diff.index_diffs[0].diff_type,  IndexDiffType::CHANGED);
    EXPECT_EQ(diff.index_diffs[0].index_name, "idx_col");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-10 — addedColumnCount / removedColumnCount / modifiedColumnCount
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, ColumnCountHelpersAreCorrect) {
    // from: id(integer), name(string), old_col(boolean)
    // to:   id(integer), name(double), new_col(string)
    //        → old_col removed, new_col added, name TYPE_CHANGED
    auto from = makeSchema("t",
        {{"id",      "integer"},
         {"name",    "string"},
         {"old_col", "boolean"}});
    auto to   = makeSchema("t",
        {{"id",      "integer"},
         {"name",    "double"},
         {"new_col", "string"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    EXPECT_EQ(diff.addedColumnCount(),    1u);
    EXPECT_EQ(diff.removedColumnCount(),  1u);
    EXPECT_EQ(diff.modifiedColumnCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-11 — column_diffs sorted alphabetically by column_name
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, ColumnDiffsSortedAlphabetically) {
    auto from = makeSchema("t", {{"zebra", "string"}, {"apple", "string"}});
    auto to   = makeSchema("t", {{"mango", "string"}, {"banana", "string"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_GE(diff.column_diffs.size(), 2u);
    for (size_t i = 1; i < diff.column_diffs.size(); ++i) {
        EXPECT_LE(diff.column_diffs[i - 1].column_name,
                  diff.column_diffs[i].column_name);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-12 — index_diffs sorted alphabetically by index_name
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, IndexDiffsSortedAlphabetically) {
    auto from = makeSchema("t", {{"id", "integer"}},
                           {{"z_idx", "regular", {"id"}, false}});
    auto to   = makeSchema("t", {{"id", "integer"}},
                           {{"a_idx", "regular", {"id"}, false},
                            {"m_idx", "fulltext", {"id"}, false}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);

    ASSERT_GE(diff.index_diffs.size(), 2u);
    for (size_t i = 1; i < diff.index_diffs.size(); ++i) {
        EXPECT_LE(diff.index_diffs[i - 1].index_name,
                  diff.index_diffs[i].index_name);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-13 — toJSON contains expected top-level fields and summary object
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, ToJsonContainsExpectedFields) {
    auto from = makeSchema("orders", {{"id", "integer"}, {"old_col", "string"}});
    auto to   = makeSchema("orders", {{"id", "integer"}, {"new_col", "boolean"}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(from, to);
    auto j    = diff.toJSON();

    // Top-level structure
    EXPECT_TRUE(j.contains("table_name"));
    EXPECT_TRUE(j.contains("column_diffs"));
    EXPECT_TRUE(j.contains("index_diffs"));
    EXPECT_TRUE(j.contains("summary"));

    EXPECT_EQ(j["table_name"].get<std::string>(), "orders");

    // Summary sub-object
    ASSERT_TRUE(j["summary"].contains("added_columns"));
    ASSERT_TRUE(j["summary"].contains("removed_columns"));
    ASSERT_TRUE(j["summary"].contains("modified_columns"));
    ASSERT_TRUE(j["summary"].contains("index_changes"));

    EXPECT_EQ(j["summary"]["added_columns"].get<size_t>(),   diff.addedColumnCount());
    EXPECT_EQ(j["summary"]["removed_columns"].get<size_t>(), diff.removedColumnCount());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-14 — diff with empty 'from' schema gives all columns ADDED
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, EmptyFromSchemaGivesAllColumnsAdded) {
    SchemaManager::TableSchema empty_from;
    empty_from.name = "new_table";

    auto to = makeSchema("new_table",
        {{"id",    "integer", false},
         {"email", "string",  true},
         {"score", "double",  true}});

    SchemaDiffEngine engine;
    auto diff = engine.diff(empty_from, to);

    EXPECT_EQ(diff.addedColumnCount(),   3u);
    EXPECT_EQ(diff.removedColumnCount(), 0u);
    for (const auto& cd : diff.column_diffs) {
        EXPECT_EQ(cd.diff_type, ColumnDiffType::ADDED);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-DIFF-15 — ColumnDiff::toJSON and IndexDiff::toJSON produce correct fields
// ─────────────────────────────────────────────────────────────────────────────

TEST(SchemaDiffFocusedTests, ColumnDiffToJsonHasCorrectFields) {
    ColumnDiff cd;
    cd.diff_type   = ColumnDiffType::TYPE_CHANGED;
    cd.column_name = "score";
    cd.old_value   = "integer";
    cd.new_value   = "double";

    auto j = cd.toJSON();

    EXPECT_EQ(j["diff_type"].get<std::string>(),   "TYPE_CHANGED");
    EXPECT_EQ(j["column_name"].get<std::string>(), "score");
    EXPECT_EQ(j["old_value"].get<std::string>(),   "integer");
    EXPECT_EQ(j["new_value"].get<std::string>(),   "double");
}

TEST(SchemaDiffFocusedTests, ColumnDiffToJsonNullValuesForAdded) {
    ColumnDiff cd;
    cd.diff_type   = ColumnDiffType::ADDED;
    cd.column_name = "new_col";
    cd.old_value   = std::nullopt;
    cd.new_value   = "string";

    auto j = cd.toJSON();

    EXPECT_EQ(j["diff_type"].get<std::string>(), "ADDED");
    EXPECT_TRUE(j["old_value"].is_null());
    EXPECT_EQ(j["new_value"].get<std::string>(), "string");
}

TEST(SchemaDiffFocusedTests, IndexDiffToJsonHasCorrectFields) {
    IndexDiff id;
    id.diff_type  = IndexDiffType::CHANGED;
    id.index_name = "idx_email";

    auto j = id.toJSON();

    EXPECT_EQ(j["diff_type"].get<std::string>(),  "CHANGED");
    EXPECT_EQ(j["index_name"].get<std::string>(), "idx_email");
}
