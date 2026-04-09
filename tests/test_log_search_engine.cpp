/**
 * @file test_log_search_engine.cpp
 * @brief Focused unit tests for LogSearchEngine (observability module).
 *
 * Test suite: LogSearchEngineFocusedTests
 */

#include <gtest/gtest.h>
#include "observability/log_search_engine.h"
#include "observability/log_aggregator.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::observability;
using Level = themis::core::concerns::ILogger::Level;
using namespace std::chrono_literals;

// ================================================================
// Helpers
// ================================================================

namespace {

LogEntry makeEntry(Level level,
                   const std::string& message,
                   const std::map<std::string, std::string>& fields = {},
                   std::chrono::system_clock::time_point tp =
                       std::chrono::system_clock::now())
{
    LogEntry e;
    e.level     = level;
    e.message   = message;
    e.fields    = fields;
    e.timestamp = tp;
    return e;
}

} // namespace

// ================================================================
// Fixture
// ================================================================

class LogSearchEngineTest : public ::testing::Test {
protected:
    LogSearchEngine engine_;
};

// ================================================================
// Empty / trivial
// ================================================================

TEST_F(LogSearchEngineTest, Search_EmptyEntries_ReturnsEmpty) {
    LogSearchQuery q;
    auto result = engine_.search({}, q);
    EXPECT_EQ(0u, result.total_matched);
    EXPECT_TRUE(result.entries.empty());
}

TEST_F(LogSearchEngineTest, Count_EmptyEntries_ReturnsZero) {
    LogSearchQuery q;
    EXPECT_EQ(0u, engine_.count({}, q));
}

TEST_F(LogSearchEngineTest, Search_NoFilters_ReturnsAll) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO,  "msg1"),
        makeEntry(Level::WARN,  "msg2"),
        makeEntry(Level::ERROR, "msg3"),
    };
    LogSearchQuery q;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(3u, result.total_matched);
    EXPECT_EQ(3u, result.entries.size());
}

// ================================================================
// Level filter
// ================================================================

TEST_F(LogSearchEngineTest, LevelFilter_ExcludesBelowMinLevel) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::DEBUG, "debug msg"),
        makeEntry(Level::INFO,  "info msg"),
        makeEntry(Level::WARN,  "warn msg"),
        makeEntry(Level::ERROR, "error msg"),
    };
    LogSearchQuery q;
    q.min_level = Level::WARN;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(2u, result.total_matched);
    for (const auto& e : result.entries) {
        EXPECT_GE(static_cast<int>(e.level), static_cast<int>(Level::WARN));
    }
}

TEST_F(LogSearchEngineTest, LevelFilter_TraceIncludesAll) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::TRACE, "t"),
        makeEntry(Level::DEBUG, "d"),
        makeEntry(Level::INFO,  "i"),
    };
    LogSearchQuery q;
    q.min_level = Level::TRACE;
    EXPECT_EQ(3u, engine_.search(entries, q).total_matched);
}

// ================================================================
// Time range filter
// ================================================================

TEST_F(LogSearchEngineTest, TimeRange_FromTime_FiltersOlder) {
    auto now    = std::chrono::system_clock::now();
    auto old    = now - std::chrono::hours(2);
    auto recent = now - std::chrono::minutes(5);
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "old",    {}, old),
        makeEntry(Level::INFO, "recent", {}, recent),
        makeEntry(Level::INFO, "now",    {}, now),
    };
    LogSearchQuery q;
    q.has_from_time = true;
    q.from_time = now - std::chrono::hours(1);
    auto result = engine_.search(entries, q);
    EXPECT_EQ(2u, result.total_matched);
}

TEST_F(LogSearchEngineTest, TimeRange_ToTime_FiltersNewer) {
    auto now = std::chrono::system_clock::now();
    auto tp1 = now - std::chrono::hours(3);
    auto tp2 = now - std::chrono::hours(1);
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "a", {}, tp1),
        makeEntry(Level::INFO, "b", {}, tp2),
        makeEntry(Level::INFO, "c", {}, now),
    };
    LogSearchQuery q;
    q.has_to_time = true;
    q.to_time = now - std::chrono::minutes(30);
    auto result = engine_.search(entries, q);
    EXPECT_EQ(2u, result.total_matched);
}

// ================================================================
// Message contains
// ================================================================

TEST_F(LogSearchEngineTest, MessageContains_MatchesSubstring) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "Slow query detected"),
        makeEntry(Level::INFO, "Query executed normally"),
        makeEntry(Level::INFO, "Connection established"),
    };
    LogSearchQuery q;
    q.message_contains = "query";
    auto result = engine_.search(entries, q);
    EXPECT_EQ(2u, result.total_matched);
}

TEST_F(LogSearchEngineTest, MessageContains_NoMatch_ReturnsEmpty) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "nothing interesting here"),
    };
    LogSearchQuery q;
    q.message_contains = "xyz_not_present";
    EXPECT_EQ(0u, engine_.search(entries, q).total_matched);
}

// ================================================================
// Field filters
// ================================================================

TEST_F(LogSearchEngineTest, FieldFilter_Equals_Match) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"query_id", "q-42"}}),
        makeEntry(Level::INFO, "m", {{"query_id", "q-99"}}),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"query_id", "q-42", FieldMatchOp::EQUALS});
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
    EXPECT_EQ("q-42", result.entries[0].fields.at("query_id"));
}

TEST_F(LogSearchEngineTest, FieldFilter_NotEquals_Match) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"status", "ok"}}),
        makeEntry(Level::INFO, "m", {{"status", "error"}}),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"status", "ok", FieldMatchOp::NOT_EQUALS});
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
    EXPECT_EQ("error", result.entries[0].fields.at("status"));
}

TEST_F(LogSearchEngineTest, FieldFilter_Contains_Match) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"component", "storage_engine"}}),
        makeEntry(Level::INFO, "m", {{"component", "query_planner"}}),
        makeEntry(Level::INFO, "m", {{"component", "cache"}}),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"component", "storage", FieldMatchOp::CONTAINS});
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
}

TEST_F(LogSearchEngineTest, FieldFilter_StartsWith_Match) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"query_id", "q-001"}}),
        makeEntry(Level::INFO, "m", {{"query_id", "q-002"}}),
        makeEntry(Level::INFO, "m", {{"query_id", "r-001"}}),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"query_id", "q-", FieldMatchOp::STARTS_WITH});
    auto result = engine_.search(entries, q);
    EXPECT_EQ(2u, result.total_matched);
}

TEST_F(LogSearchEngineTest, FieldFilter_MissingKey_DoesNotMatch) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "no fields"),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"nonexistent", "val", FieldMatchOp::EQUALS});
    EXPECT_EQ(0u, engine_.search(entries, q).total_matched);
}

TEST_F(LogSearchEngineTest, FieldFilter_MissingKey_NotEquals_Matches) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "no fields"),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"nonexistent", "val", FieldMatchOp::NOT_EQUALS});
    EXPECT_EQ(1u, engine_.search(entries, q).total_matched);
}

TEST_F(LogSearchEngineTest, MultipleFieldFilters_ANDSemantics) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"a", "1"}, {"b", "x"}}),
        makeEntry(Level::INFO, "m", {{"a", "1"}, {"b", "y"}}),
        makeEntry(Level::INFO, "m", {{"a", "2"}, {"b", "x"}}),
    };
    LogSearchQuery q;
    q.field_filters.push_back({"a", "1", FieldMatchOp::EQUALS});
    q.field_filters.push_back({"b", "x", FieldMatchOp::EQUALS});
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
}

// ================================================================
// Pagination
// ================================================================

TEST_F(LogSearchEngineTest, Limit_RestrictsResults) {
    std::vector<LogEntry> entries;
    for (int i = 0; i < 10; ++i)
        entries.push_back(makeEntry(Level::INFO, "m" + std::to_string(i)));
    LogSearchQuery q;
    q.limit = 3;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(10u, result.total_matched);
    EXPECT_EQ(3u, result.entries.size());
}

TEST_F(LogSearchEngineTest, Offset_SkipsEntries) {
    std::vector<LogEntry> entries;
    for (int i = 0; i < 10; ++i)
        entries.push_back(makeEntry(Level::INFO, "m" + std::to_string(i)));
    LogSearchQuery q;
    q.offset = 7;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(10u, result.total_matched);
    EXPECT_EQ(3u, result.entries.size());
}

TEST_F(LogSearchEngineTest, OffsetBeyondTotal_ReturnsEmpty) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "only one"),
    };
    LogSearchQuery q;
    q.offset = 5;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
    EXPECT_TRUE(result.entries.empty());
}

TEST_F(LogSearchEngineTest, LimitZero_MeansNoLimit) {
    std::vector<LogEntry> entries;
    for (int i = 0; i < 20; ++i)
        entries.push_back(makeEntry(Level::INFO, "m"));
    LogSearchQuery q;
    q.limit = 0;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(20u, result.entries.size());
}

// ================================================================
// Sort order
// ================================================================

TEST_F(LogSearchEngineTest, SortAscending_OldestFirst) {
    auto now = std::chrono::system_clock::now();
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "newest", {}, now),
        makeEntry(Level::INFO, "oldest", {}, now - std::chrono::hours(2)),
        makeEntry(Level::INFO, "middle", {}, now - std::chrono::hours(1)),
    };
    LogSearchQuery q;
    q.ascending = true;
    auto result = engine_.search(entries, q);
    ASSERT_EQ(3u, result.entries.size());
    EXPECT_EQ("oldest", result.entries[0].message);
    EXPECT_EQ("newest", result.entries[2].message);
}

TEST_F(LogSearchEngineTest, SortDescending_NewestFirst) {
    auto now = std::chrono::system_clock::now();
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "newest", {}, now),
        makeEntry(Level::INFO, "oldest", {}, now - std::chrono::hours(2)),
        makeEntry(Level::INFO, "middle", {}, now - std::chrono::hours(1)),
    };
    LogSearchQuery q;
    q.ascending = false;
    auto result = engine_.search(entries, q);
    ASSERT_EQ(3u, result.entries.size());
    EXPECT_EQ("newest", result.entries[0].message);
    EXPECT_EQ("oldest", result.entries[2].message);
}

// ================================================================
// Count
// ================================================================

TEST_F(LogSearchEngineTest, Count_MatchesSearchTotalMatched) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO,  "i"),
        makeEntry(Level::WARN,  "w"),
        makeEntry(Level::ERROR, "e"),
    };
    LogSearchQuery q;
    q.min_level = Level::WARN;
    EXPECT_EQ(engine_.search(entries, q).total_matched,
              engine_.count(entries, q));
}

// ================================================================
// distinctFieldValues
// ================================================================

TEST_F(LogSearchEngineTest, DistinctFieldValues_ReturnsUniqueValues) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "m", {{"component", "storage"}}),
        makeEntry(Level::INFO, "m", {{"component", "cache"}}),
        makeEntry(Level::INFO, "m", {{"component", "storage"}}),
        makeEntry(Level::INFO, "m", {}),
    };
    auto vals = engine_.distinctFieldValues(entries, "component");
    EXPECT_EQ(2u, vals.size());
}

TEST_F(LogSearchEngineTest, DistinctFieldValues_AbsentKey_ReturnsEmpty) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::INFO, "no fields"),
    };
    auto vals = engine_.distinctFieldValues(entries, "nonexistent");
    EXPECT_TRUE(vals.empty());
}

// ================================================================
// Combined filters (AND semantics across types)
// ================================================================

TEST_F(LogSearchEngineTest, CombinedFilters_AllMustMatch) {
    std::vector<LogEntry> entries = {
        makeEntry(Level::WARN, "Slow query", {{"query_id", "q-1"}}),
        makeEntry(Level::INFO, "Slow query", {{"query_id", "q-2"}}),
        makeEntry(Level::WARN, "Fast query", {{"query_id", "q-3"}}),
    };
    LogSearchQuery q;
    q.min_level = Level::WARN;
    q.message_contains = "Slow";
    auto result = engine_.search(entries, q);
    EXPECT_EQ(1u, result.total_matched);
    EXPECT_EQ("q-1", result.entries[0].fields.at("query_id"));
}

// ================================================================
// Result metadata
// ================================================================

TEST_F(LogSearchEngineTest, ResultMetadata_OffsetAndLimitPreserved) {
    std::vector<LogEntry> entries;
    for (int i = 0; i < 20; ++i)
        entries.push_back(makeEntry(Level::INFO, "m"));
    LogSearchQuery q;
    q.offset = 5;
    q.limit  = 7;
    auto result = engine_.search(entries, q);
    EXPECT_EQ(5u,  result.offset);
    EXPECT_EQ(7u,  result.limit);
    EXPECT_EQ(20u, result.total_matched);
    EXPECT_EQ(7u,  result.entries.size());
}
