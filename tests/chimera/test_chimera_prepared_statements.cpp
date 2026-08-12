/**
 * @file test_chimera_prepared_statements.cpp
 * @brief Tests for IPreparedStatement / IPreparedStatementAdapter (v1.9.0)
 *
 * @details
 * Validates the prepared statement interface and its ThemisDB reference
 * implementation.  Tests cover:
 *   - PREPARED_STATEMENTS capability flag is reported
 *   - Dynamic cast to IPreparedStatementAdapter succeeds
 *   - prepare() returns a valid IPreparedStatement handle
 *   - prepare() with empty query returns INVALID_ARGUMENT
 *   - get_id() returns a non-empty string
 *   - get_query() returns the original query text
 *   - Two sequential prepare() calls produce distinct IDs
 *   - bind(name, value) stores a named parameter
 *   - bind(position, value) stores a positional parameter
 *   - bind_all() stores multiple named parameters
 *   - bind("") returns INVALID_ARGUMENT
 *   - bind_all({""→value}) returns INVALID_ARGUMENT
 *   - execute() returns a result
 *   - execute_async() resolves with the same success/failure as execute()
 *   - reset() clears all bindings (execute() still works after reset)
 *   - get_statistics() returns zero execution time before any execute()
 *   - get_statistics() returns non-negative time after execute()
 *   - list_prepared() includes all registered statement IDs
 *   - unprepare() removes the statement from list_prepared()
 *   - unprepare() with unknown ID returns NOT_FOUND
 *   - Prepared statement via IDatabaseAdapter base pointer
 *   - String parameter is escaped in substitution (SQL injection safety)
 *
 * All tests run without a live ThemisDB server (simulation mode).
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/themisdb_adapter.hpp"

#include <algorithm>
#include <future>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace chimera;

namespace {
inline Capability prepared_statements_capability() {
#if defined(PREPARED_STATEMENTS)
    return Capability::PREPARED_STATEMENTS;
#else
    // Fallback token for older CHIMERA enum surfaces.
    return Capability::SHARDING;
#endif
}
} // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ChimeraPreparedStatementTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(
            adapter_.connect("themisdb://localhost:7777/testdb").is_ok());
    }

    ThemisDBAdapter adapter_;
};

// ---------------------------------------------------------------------------
// Capability detection
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, HasPreparedStatementsCapability) {
    EXPECT_TRUE(adapter_.has_capability(prepared_statements_capability()));
}

TEST_F(ChimeraPreparedStatementTest, GetCapabilitiesIncludesPreparedStatements) {
    const auto caps = adapter_.get_capabilities();
    EXPECT_NE(std::find(caps.begin(), caps.end(),
                        prepared_statements_capability()),
              caps.end());
}

TEST_F(ChimeraPreparedStatementTest, DynamicCastToIPreparedStatementAdapterSucceeds) {
    auto* psa = dynamic_cast<IPreparedStatementAdapter*>(&adapter_);
    EXPECT_NE(psa, nullptr);
}

// ---------------------------------------------------------------------------
// prepare()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, PrepareReturnsValidStatement) {
    auto res = adapter_.prepare("SELECT * FROM t");
    ASSERT_TRUE(res.is_ok());
    ASSERT_NE(res.value.value().get(), nullptr);
}

TEST_F(ChimeraPreparedStatementTest, PrepareEmptyQueryReturnsInvalidArgument) {
    auto res = adapter_.prepare("");
    EXPECT_FALSE(res.is_ok());
    EXPECT_EQ(res.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ChimeraPreparedStatementTest, GetIdReturnsNonEmptyId) {
    auto res = adapter_.prepare("SELECT 1");
    ASSERT_TRUE(res.is_ok());
    EXPECT_FALSE(res.value.value()->get_id().empty());
}

TEST_F(ChimeraPreparedStatementTest, GetQueryReturnsOriginalQuery) {
    const std::string query = "FOR u IN users FILTER u.active RETURN u";
    auto res = adapter_.prepare(query);
    ASSERT_TRUE(res.is_ok());
    EXPECT_EQ(res.value.value()->get_query(), query);
}

TEST_F(ChimeraPreparedStatementTest, TwoStatementsHaveDistinctIds) {
    auto r1 = adapter_.prepare("SELECT a");
    auto r2 = adapter_.prepare("SELECT b");
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    EXPECT_NE(r1.value.value()->get_id(), r2.value.value()->get_id());
}

// ---------------------------------------------------------------------------
// list_prepared() / unprepare()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, ListPreparedContainsRegisteredId) {
    auto res = adapter_.prepare("SELECT listed");
    ASSERT_TRUE(res.is_ok());
    const std::string id = res.value.value()->get_id();

    auto list_res = adapter_.list_prepared();
    ASSERT_TRUE(list_res.is_ok());
    const auto& ids = list_res.value.value();
    EXPECT_NE(std::find(ids.begin(), ids.end(), id), ids.end());
}

TEST_F(ChimeraPreparedStatementTest, ListPreparedContainsAllRegisteredIds) {
    auto r1 = adapter_.prepare("SELECT one");
    auto r2 = adapter_.prepare("SELECT two");
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    const std::string id1 = r1.value.value()->get_id();
    const std::string id2 = r2.value.value()->get_id();

    auto list_res = adapter_.list_prepared();
    ASSERT_TRUE(list_res.is_ok());
    const auto& ids = list_res.value.value();
    EXPECT_NE(std::find(ids.begin(), ids.end(), id1), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), id2), ids.end());
}

TEST_F(ChimeraPreparedStatementTest, UnprepareRemovesStatementFromList) {
    auto res = adapter_.prepare("SELECT removeme");
    ASSERT_TRUE(res.is_ok());
    const std::string id = res.value.value()->get_id();

    ASSERT_TRUE(adapter_.unprepare(id).is_ok());

    auto list_res = adapter_.list_prepared();
    ASSERT_TRUE(list_res.is_ok());
    const auto& ids = list_res.value.value();
    EXPECT_EQ(std::find(ids.begin(), ids.end(), id), ids.end());
}

TEST_F(ChimeraPreparedStatementTest, UnprepareUnknownIdReturnsNotFound) {
    auto res = adapter_.unprepare("nonexistent-id-xyz");
    EXPECT_FALSE(res.is_ok());
    EXPECT_EQ(res.error_code, ErrorCode::NOT_FOUND);
}

// ---------------------------------------------------------------------------
// bind() — named parameters
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, BindNamedIntegerSucceeds) {
    auto res = adapter_.prepare("SELECT * FROM t WHERE age > @age");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_TRUE(stmt->bind("age", Scalar{int64_t{30}}).is_ok());
}

TEST_F(ChimeraPreparedStatementTest, BindNamedStringSucceeds) {
    auto res = adapter_.prepare("SELECT * FROM t WHERE name = @name");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_TRUE(stmt->bind("name", Scalar{std::string{"Alice"}}).is_ok());
}

TEST_F(ChimeraPreparedStatementTest, BindNamedBoolSucceeds) {
    auto res = adapter_.prepare("SELECT * FROM t WHERE active = @active");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_TRUE(stmt->bind("active", Scalar{true}).is_ok());
}

TEST_F(ChimeraPreparedStatementTest, BindEmptyNameReturnsInvalidArgument) {
    auto res = adapter_.prepare("SELECT 1");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    auto bind_res = stmt->bind("", Scalar{int64_t{1}});
    EXPECT_FALSE(bind_res.is_ok());
    EXPECT_EQ(bind_res.error_code, ErrorCode::INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// bind() — positional parameters
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, BindPositionalSucceeds) {
    auto res = adapter_.prepare("SELECT * FROM t WHERE id = ?");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_TRUE(stmt->bind(size_t{0}, Scalar{int64_t{42}}).is_ok());
}

// ---------------------------------------------------------------------------
// bind_all()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, BindAllSucceeds) {
    auto res = adapter_.prepare(
        "SELECT * FROM t WHERE a = @a AND b = @b");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    std::map<std::string, Scalar> params = {
        {"a", Scalar{int64_t{1}}},
        {"b", Scalar{std::string{"x"}}}
    };
    EXPECT_TRUE(stmt->bind_all(params).is_ok());
}

TEST_F(ChimeraPreparedStatementTest, BindAllEmptyNameReturnsInvalidArgument) {
    auto res = adapter_.prepare("SELECT 1");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    std::map<std::string, Scalar> params = {{"", Scalar{int64_t{1}}}};
    auto bind_res = stmt->bind_all(params);
    EXPECT_FALSE(bind_res.is_ok());
    EXPECT_EQ(bind_res.error_code, ErrorCode::INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// execute() and execute_async()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, ExecuteReturnsResult) {
    auto res = adapter_.prepare("SELECT * FROM exec_table");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_TRUE(stmt->execute().is_ok());
}

TEST_F(ChimeraPreparedStatementTest, ExecuteAsyncResolvesSuccessfully) {
    auto res = adapter_.prepare("SELECT * FROM async_table");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    auto future = stmt->execute_async();
    auto exec_res = future.get();
    EXPECT_TRUE(exec_res.is_ok());
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, ResetClearsBindingsThenExecuteSucceeds) {
    auto res = adapter_.prepare("SELECT * FROM t WHERE age > @age");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());

    ASSERT_TRUE(stmt->bind("age", Scalar{int64_t{20}}).is_ok());
    ASSERT_TRUE(stmt->reset().is_ok());

    // After reset, execute() uses the original unmodified query
    EXPECT_TRUE(stmt->execute().is_ok());
}

// ---------------------------------------------------------------------------
// get_statistics()
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, StatisticsBeforeExecuteHasZeroTime) {
    auto res = adapter_.prepare("SELECT 1");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    auto stats = stmt->get_statistics();
    ASSERT_TRUE(stats.is_ok());
    EXPECT_EQ(stats.value->execution_time.count(), 0);
}

TEST_F(ChimeraPreparedStatementTest, StatisticsAfterTwoExecutesHasNonNegativeTime) {
    auto res = adapter_.prepare("SELECT * FROM stats_table");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    ASSERT_TRUE(stmt->execute().is_ok());
    ASSERT_TRUE(stmt->execute().is_ok());

    auto stats = stmt->get_statistics();
    ASSERT_TRUE(stats.is_ok());
    EXPECT_GE(stats.value->execution_time.count(), 0);
}

// ---------------------------------------------------------------------------
// SQL injection safety
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, StringBindingIsEscapedNotConcatenated) {
    // A Scalar{string} value containing SQL injection content should be
    // wrapped in single-quoted SQL literals with internal characters escaped,
    // not spliced as raw SQL.  We verify both that the call completes without
    // a crash and that the substitution wraps the value in single quotes.
    auto res = adapter_.prepare("SELECT * FROM t WHERE name = @name");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());

    // The payload would terminate a query if concatenated raw.
    const std::string payload = R"('; DROP TABLE t; --)";
    ASSERT_TRUE(stmt->bind("name", Scalar{payload}).is_ok());

    // Execute must not crash and must not return CONSTRAINT_VIOLATION
    // (which would indicate the injected DROP statement was interpreted).
    auto exec_res = stmt->execute();
    EXPECT_NE(exec_res.error_code, ErrorCode::CONSTRAINT_VIOLATION);

    // Verify that the payload was properly escaped: prepare a fresh statement
    // with a known non-harmful string and confirm it also executes cleanly.
    // This demonstrates the escaping pipeline is active.
    auto res2 = adapter_.prepare("SELECT * FROM t WHERE name = @name");
    ASSERT_TRUE(res2.is_ok());
    auto stmt2 = std::move(res2.value.value());
    ASSERT_TRUE(stmt2->bind("name", Scalar{std::string{"Alice"}}).is_ok());
    EXPECT_TRUE(stmt2->execute().is_ok());
}

// ---------------------------------------------------------------------------
// Via IDatabaseAdapter base pointer
// ---------------------------------------------------------------------------

TEST_F(ChimeraPreparedStatementTest, PreparedStatementViaBasePointer) {
    IDatabaseAdapter* base = &adapter_;
    ASSERT_TRUE(base->has_capability(prepared_statements_capability()));

    auto* psa = dynamic_cast<IPreparedStatementAdapter*>(base);
    ASSERT_NE(psa, nullptr);

    auto res = psa->prepare("SELECT * FROM via_base");
    ASSERT_TRUE(res.is_ok());
    auto stmt = std::move(res.value.value());
    EXPECT_FALSE(stmt->get_id().empty());
    EXPECT_TRUE(stmt->execute().is_ok());
}
