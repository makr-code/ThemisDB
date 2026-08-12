// Tests for multi-statement transaction AQL (BEGIN/COMMIT/ROLLBACK)

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>

#include "query/aql_parser.h"
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;
using namespace themis::query;

// ============================================================================
// Parser tests (no DB required)
// ============================================================================

TEST(AqlMultiStatementTransactionParser, ParseBeginCommit) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN users FILTER doc.active == true RETURN doc
        COMMIT
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Commit);
    ASSERT_EQ(result->statements.size(), 1u);
    EXPECT_NE(result->statements[0], nullptr);
}

TEST(AqlMultiStatementTransactionParser, ParseBeginRollback) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN orders RETURN doc
        ROLLBACK
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Rollback);
    ASSERT_EQ(result->statements.size(), 1u);
}

TEST(AqlMultiStatementTransactionParser, ParseMultipleStatements) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN users FILTER doc.active == true RETURN doc
          FOR order IN orders RETURN order
        COMMIT
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Commit);
    ASSERT_EQ(result->statements.size(), 2u);
    EXPECT_NE(result->statements[0], nullptr);
    EXPECT_NE(result->statements[1], nullptr);
}

TEST(AqlMultiStatementTransactionParser, ParsePostgresStyleSemicolons) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN;
          FOR doc IN users FILTER doc.active == true RETURN doc;
          FOR order IN orders RETURN order;
        COMMIT;
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Commit);
    ASSERT_EQ(result->statements.size(), 2u);
}

TEST(AqlMultiStatementTransactionParser, ErrorMissingBegin) {
    AQLParser parser;
    std::string aql = "FOR doc IN users RETURN doc COMMIT";
    auto result = parser.parseTransactionBlock(aql);
    EXPECT_FALSE(result);
}

TEST(AqlMultiStatementTransactionParser, ErrorMissingTerminator) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN users RETURN doc
    )";
    auto result = parser.parseTransactionBlock(aql);
    EXPECT_FALSE(result);
}

TEST(AqlMultiStatementTransactionParser, ErrorTrailingTokensAfterTerminator) {
    AQLParser parser;
    std::string aql = "BEGIN COMMIT FOR doc IN users RETURN doc";
    auto result = parser.parseTransactionBlock(aql);
    EXPECT_FALSE(result);
}

TEST(AqlMultiStatementTransactionParser, EmptyBlockCommit) {
    // An empty transaction block (no statements) with COMMIT is valid
    AQLParser parser;
    std::string aql = "BEGIN COMMIT";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Commit);
    EXPECT_EQ(result->statements.size(), 0u);
}

TEST(AqlMultiStatementTransactionParser, ToJsonCommit) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN users RETURN doc
        COMMIT
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result);
    auto j = result->toJSON();
    EXPECT_EQ(j["type"], "transaction_block");
    EXPECT_EQ(j["action"], "COMMIT");
    EXPECT_EQ(j["statements"].size(), 1u);
}

TEST(AqlMultiStatementTransactionParser, ToJsonRollback) {
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          FOR doc IN users RETURN doc
        ROLLBACK
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result);
    auto j = result->toJSON();
    EXPECT_EQ(j["action"], "ROLLBACK");
}

TEST(AqlMultiStatementTransactionParser, ParseWithCteSingleStatement) {
    // Verifies that a FOR inside a CTE subquery is not mistaken for a
    // new top-level statement (parenthesis-depth tracking fix).
    AQLParser parser;
    std::string aql = R"(
        BEGIN
          WITH expensive AS (FOR x IN products FILTER x.price > 100 RETURN x)
          FOR doc IN expensive RETURN doc
        COMMIT
    )";
    auto result = parser.parseTransactionBlock(aql);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ(result->action, AqlTransactionAction::Commit);
    // The WITH clause + its FOR loop must be parsed as ONE statement
    ASSERT_EQ(result->statements.size(), 1u);
    EXPECT_NE(result->statements[0], nullptr);
    EXPECT_NE(result->statements[0]->with_clause, nullptr);
}

// ============================================================================
// Runner tests (require DB)
// ============================================================================

static std::string tmpPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (name + std::to_string(now))).string();
}

class MultiStatementAqlRunnerTest : public ::testing::Test {
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;

    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path = tmpPath("themis_mstx_");
        cfg.enable_blobdb = false;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        ASSERT_TRUE(idx_->createIndex("users", "active").ok);
        ASSERT_TRUE(idx_->createIndex("orders", "status").ok);

        auto put = [&](const std::string& tbl, const std::string& pk,
                       BaseEntity::FieldMap fields) {
            auto e = BaseEntity::fromFields(pk, std::move(fields));
            ASSERT_TRUE(idx_->put(tbl, e).ok);
        };
        // Note: field values stored in secondary index predicates are strings
        put("users", "u1", {{"active", std::string("true")}, {"name", std::string("Alice")}});
        put("users", "u2", {{"active", std::string("false")}, {"name", std::string("Bob")}});
        put("orders", "o1", {{"status", std::string("open")}, {"user_id", std::string("u1")}});

        engine_ = std::make_unique<QueryEngine>(*db_, *idx_);
    }

    void TearDown() override {
        engine_.reset();
        idx_.reset();
        db_->close();
        db_.reset();
    }
};

TEST_F(MultiStatementAqlRunnerTest, CommitReturnsCombinedResults) {
    std::string aql = R"(
        BEGIN
          FOR doc IN users FILTER doc.active == "true" RETURN doc
          FOR order IN orders FILTER order.status == "open" RETURN order
        COMMIT
    )";
    auto result = executeMultiStatementAql(aql, *engine_);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    auto& j = *result;
    EXPECT_EQ(j["type"], "commit");
    EXPECT_EQ(j["results"].size(), 2u);
}

TEST_F(MultiStatementAqlRunnerTest, CommitWithSemicolonsReturnsCombinedResults) {
    std::string aql = R"(
        BEGIN;
          FOR doc IN users FILTER doc.active == "true" RETURN doc;
          FOR order IN orders FILTER order.status == "open" RETURN order;
        COMMIT;
    )";
    auto result = executeMultiStatementAql(aql, *engine_);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    auto& j = *result;
    EXPECT_EQ(j["type"], "commit");
    EXPECT_EQ(j["results"].size(), 2u);
}

TEST_F(MultiStatementAqlRunnerTest, RollbackReturnsMetadataOnly) {
    std::string aql = R"(
        BEGIN
          FOR doc IN users RETURN doc
        ROLLBACK
    )";
    auto result = executeMultiStatementAql(aql, *engine_);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    auto& j = *result;
    EXPECT_EQ(j["type"], "rollback");
    EXPECT_EQ(j["statements"], 1u);
}

TEST_F(MultiStatementAqlRunnerTest, EmptyCommitSucceeds) {
    std::string aql = "BEGIN COMMIT";
    auto result = executeMultiStatementAql(aql, *engine_);
    ASSERT_TRUE(result) << (result ? "" : result.error().message());
    EXPECT_EQ((*result)["type"], "commit");
    EXPECT_EQ((*result)["results"].size(), 0u);
}

TEST_F(MultiStatementAqlRunnerTest, ParseErrorReturnsErr) {
    std::string aql = "FOR doc IN users RETURN doc"; // missing BEGIN
    auto result = executeMultiStatementAql(aql, *engine_);
    EXPECT_FALSE(result);
}
