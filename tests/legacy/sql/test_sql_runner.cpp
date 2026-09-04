// Integration tests for the SQL dialect compatibility layer (executeSQL).
// Verifies that executeSQL() correctly parses SQL, transpiles it to AQL, and
// executes it through the QueryEngine.

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>

#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "utils/error_registry.h"

using namespace themis;
using namespace themis::query;

// ─── helpers ────────────────────────────────────────────────────────────────

static std::string tmpSQLRunnerPath(const std::string& suffix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_sqlr_" + suffix + std::to_string(now))).string();
}

// ════════════════════════════════════════════════════════════════════════════
// executeSQL – parse-error tests (use the fixture so cleanup is handled)
// ════════════════════════════════════════════════════════════════════════════

class ExecuteSQLTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = tmpSQLRunnerPath("int_");
        RocksDBWrapper::Config cfg;
        cfg.db_path = dbPath_;
        cfg.enable_blobdb = false;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        ASSERT_TRUE(idx_->createIndex("users", "city").ok);

        // Pre-populate: two users in Berlin, one in Paris
        BaseEntity::FieldMap f1{{"name", std::string("Alice")}, {"age", std::string("30")}, {"city", std::string("Berlin")}};
        BaseEntity::FieldMap f2{{"name", std::string("Bob")},   {"age", std::string("25")}, {"city", std::string("Berlin")}};
        BaseEntity::FieldMap f3{{"name", std::string("Carol")}, {"age", std::string("40")}, {"city", std::string("Paris")}};
        ASSERT_TRUE(idx_->put("users", BaseEntity::fromFields("alice", f1)).ok);
        ASSERT_TRUE(idx_->put("users", BaseEntity::fromFields("bob",   f2)).ok);
        ASSERT_TRUE(idx_->put("users", BaseEntity::fromFields("carol", f3)).ok);

        engine_ = std::make_unique<QueryEngine>(*db_, *idx_);
    }

    void TearDown() override {
        engine_.reset();
        idx_.reset();
        if (db_) {
          db_->close();
        }
        db_.reset();
        std::filesystem::remove_all(dbPath_);
    }

    std::string dbPath_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
};

// ════════════════════════════════════════════════════════════════════════════
// executeSQL – parse-error edge cases (no DB access; parse fails first)
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ExecuteSQLTest, InvalidSQL_ReturnsParseError) {
    auto result = executeSQL("UPSERT INTO users VALUES (1)", *engine_);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}

TEST_F(ExecuteSQLTest, EmptyString_ReturnsParseError) {
    auto result = executeSQL("", *engine_);
    EXPECT_FALSE(result.has_value());
}

// SELECT * with a WHERE clause
TEST_F(ExecuteSQLTest, SelectWithWhere_Succeeds) {
    auto result = executeSQL(
        "SELECT * FROM users WHERE city = 'Berlin'", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_TRUE(result->is_object() || result->is_array());
}

// SELECT * without a WHERE clause
TEST_F(ExecuteSQLTest, SelectStar_Succeeds) {
    auto result = executeSQL("SELECT * FROM users", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// SELECT specific columns
TEST_F(ExecuteSQLTest, SelectColumns_Succeeds) {
    auto result = executeSQL("SELECT name, age FROM users", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// INSERT returns a well-formed response (INSERT generates AQL INSERT)
TEST_F(ExecuteSQLTest, Insert_Succeeds) {
    auto result = executeSQL(
        "INSERT INTO users (name, age, city) VALUES ('Dave', 35, 'London')",
        *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// UPDATE returns a well-formed response
TEST_F(ExecuteSQLTest, Update_Succeeds) {
    auto result = executeSQL(
        "UPDATE users SET city = 'Munich' WHERE name = 'Alice'", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// DELETE returns a well-formed response
TEST_F(ExecuteSQLTest, Delete_Succeeds) {
    auto result = executeSQL(
        "DELETE FROM users WHERE city = 'Paris'", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// Malformed SQL (missing FROM) returns parse error
TEST_F(ExecuteSQLTest, MalformedSQL_ReturnsError) {
    auto result = executeSQL("SELECT * users", *engine_);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}

// Case-insensitive keywords are accepted
TEST_F(ExecuteSQLTest, CaseInsensitiveKeywords_Succeeds) {
    auto result = executeSQL(
        "select * from users where city = 'Berlin'", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// Trailing semicolon is accepted
TEST_F(ExecuteSQLTest, TrailingSemicolon_Succeeds) {
    auto result = executeSQL("SELECT * FROM users;", *engine_);
    ASSERT_TRUE(result.has_value()) << result.error().message();
}

// ════════════════════════════════════════════════════════════════════════════
// executeSQL – unsupported SQL statement types return parse errors
// ════════════════════════════════════════════════════════════════════════════

TEST_F(ExecuteSQLTest, UnsupportedMerge_ReturnsParseError) {
    auto result = executeSQL("MERGE INTO users USING src ON users.id = src.id", *engine_);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}

TEST_F(ExecuteSQLTest, UnsupportedCreateTable_ReturnsParseError) {
    auto result = executeSQL("CREATE TABLE orders (id INT, total FLOAT)", *engine_);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}

TEST_F(ExecuteSQLTest, UnsupportedTruncate_ReturnsParseError) {
    auto result = executeSQL("TRUNCATE TABLE users", *engine_);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_PARSE_FAILED);
}
