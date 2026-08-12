// Unit tests for SQLParser and SQLToAQLTranspiler
// (SQL dialect compatibility layer – SELECT/INSERT/UPDATE/DELETE passthrough)

#include <gtest/gtest.h>
#include "query/sql_parser.h"

using namespace themis::query;

// ============================================================================
// Helper
// ============================================================================

static SQLASTNode mustParse(const std::string& sql) {
    SQLParser parser;
    auto result = parser.parse(sql);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string mustTranspile(const SQLASTNode& ast) {
    SQLToAQLTranspiler t;
    auto result = t.transpile(ast);
    EXPECT_TRUE(result.has_value()) << (!result ? result.error().message() : "");
    return result.value();
}

static std::string sqlToAQL(const std::string& sql) {
    return mustTranspile(mustParse(sql));
}

// ============================================================================
// SQLParser – SELECT
// ============================================================================

TEST(SQLParserTest, SelectStar) {
    auto ast = mustParse("SELECT * FROM users");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Select);
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->star);
    EXPECT_EQ(ast.select->table, "users");
    EXPECT_FALSE(ast.select->where.has_value());
}

TEST(SQLParserTest, SelectColumns) {
    auto ast = mustParse("SELECT name, age FROM users");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Select);
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_FALSE(ast.select->star);
    ASSERT_EQ(ast.select->columns.size(), 2u);
    EXPECT_EQ(ast.select->columns[0], "name");
    EXPECT_EQ(ast.select->columns[1], "age");
    EXPECT_EQ(ast.select->table, "users");
}

TEST(SQLParserTest, SelectWithWhere) {
    auto ast = mustParse("SELECT * FROM users WHERE age > 30");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, SelectWithOrderBy) {
    auto ast = mustParse("SELECT * FROM users ORDER BY name ASC");
    ASSERT_TRUE(ast.select.has_value());
    ASSERT_EQ(ast.select->order_by.size(), 1u);
    EXPECT_EQ(ast.select->order_by[0].column, "name");
    EXPECT_TRUE(ast.select->order_by[0].ascending);
}

TEST(SQLParserTest, SelectWithOrderByDesc) {
    auto ast = mustParse("SELECT * FROM products ORDER BY price DESC");
    ASSERT_TRUE(ast.select.has_value());
    ASSERT_EQ(ast.select->order_by.size(), 1u);
    EXPECT_EQ(ast.select->order_by[0].column, "price");
    EXPECT_FALSE(ast.select->order_by[0].ascending);
}

TEST(SQLParserTest, SelectWithLimit) {
    auto ast = mustParse("SELECT * FROM users LIMIT 10");
    ASSERT_TRUE(ast.select.has_value());
    ASSERT_TRUE(ast.select->limit.has_value());
    EXPECT_EQ(*ast.select->limit, 10);
    EXPECT_FALSE(ast.select->offset.has_value());
}

TEST(SQLParserTest, SelectWithLimitOffset) {
    auto ast = mustParse("SELECT * FROM users LIMIT 20 OFFSET 40");
    ASSERT_TRUE(ast.select.has_value());
    ASSERT_TRUE(ast.select->limit.has_value());
    ASSERT_TRUE(ast.select->offset.has_value());
    EXPECT_EQ(*ast.select->limit, 20);
    EXPECT_EQ(*ast.select->offset, 40);
}

TEST(SQLParserTest, SelectWithSemicolon) {
    auto ast = mustParse("SELECT * FROM users;");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_EQ(ast.select->table, "users");
}

TEST(SQLParserTest, SelectWithDistinct) {
    // DISTINCT is accepted but treated like plain SELECT
    auto ast = mustParse("SELECT DISTINCT name FROM users");
    ASSERT_TRUE(ast.select.has_value());
    ASSERT_EQ(ast.select->columns.size(), 1u);
    EXPECT_EQ(ast.select->columns[0], "name");
}

// ============================================================================
// SQLParser – INSERT
// ============================================================================

TEST(SQLParserTest, InsertBasic) {
    auto ast = mustParse("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Insert);
    ASSERT_TRUE(ast.insert.has_value());
    EXPECT_EQ(ast.insert->table, "users");
    ASSERT_EQ(ast.insert->columns.size(), 2u);
    EXPECT_EQ(ast.insert->columns[0], "name");
    EXPECT_EQ(ast.insert->columns[1], "age");
    ASSERT_EQ(ast.insert->values.size(), 2u);
    EXPECT_EQ(std::get<std::string>(ast.insert->values[0]), "Alice");
    EXPECT_EQ(std::get<int64_t>(ast.insert->values[1]), 30);
}

TEST(SQLParserTest, InsertWithNullValue) {
    auto ast = mustParse("INSERT INTO users (name, age) VALUES ('Bob', NULL)");
    ASSERT_TRUE(ast.insert.has_value());
    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(ast.insert->values[1]));
}

TEST(SQLParserTest, InsertColumnValueMismatch) {
    SQLParser parser;
    auto result = parser.parse("INSERT INTO users (name, age) VALUES ('Alice')");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SQLParser – UPDATE
// ============================================================================

TEST(SQLParserTest, UpdateBasic) {
    auto ast = mustParse("UPDATE users SET age = 31 WHERE name = 'Alice'");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Update);
    ASSERT_TRUE(ast.update.has_value());
    EXPECT_EQ(ast.update->table, "users");
    ASSERT_EQ(ast.update->assignments.size(), 1u);
    EXPECT_EQ(ast.update->assignments[0].column, "age");
    EXPECT_EQ(std::get<int64_t>(ast.update->assignments[0].value), 31);
    EXPECT_TRUE(ast.update->where.has_value());
}

TEST(SQLParserTest, UpdateMultipleColumns) {
    auto ast = mustParse("UPDATE products SET price = 9.99, stock = 100");
    ASSERT_TRUE(ast.update.has_value());
    ASSERT_EQ(ast.update->assignments.size(), 2u);
    EXPECT_EQ(ast.update->assignments[0].column, "price");
    EXPECT_EQ(ast.update->assignments[1].column, "stock");
    EXPECT_FALSE(ast.update->where.has_value());
}

// ============================================================================
// SQLParser – DELETE
// ============================================================================

TEST(SQLParserTest, DeleteBasic) {
    auto ast = mustParse("DELETE FROM users WHERE age < 18");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Delete);
    ASSERT_TRUE(ast.del.has_value());
    EXPECT_EQ(ast.del->table, "users");
    EXPECT_TRUE(ast.del->where.has_value());
}

TEST(SQLParserTest, DeleteWithoutWhere) {
    auto ast = mustParse("DELETE FROM temp_table");
    ASSERT_TRUE(ast.del.has_value());
    EXPECT_EQ(ast.del->table, "temp_table");
    EXPECT_FALSE(ast.del->where.has_value());
}

// ============================================================================
// SQLParser – WHERE expression types
// ============================================================================

TEST(SQLParserTest, WhereAndCondition) {
    auto ast = mustParse("SELECT * FROM users WHERE age > 18 AND city = 'Berlin'");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereOrCondition) {
    auto ast = mustParse("SELECT * FROM users WHERE age < 18 OR age > 65");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereNotCondition) {
    auto ast = mustParse("SELECT * FROM users WHERE NOT active = 1");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereIsNull) {
    auto ast = mustParse("SELECT * FROM users WHERE email IS NULL");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereIsNotNull) {
    auto ast = mustParse("SELECT * FROM users WHERE email IS NOT NULL");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereInList) {
    auto ast = mustParse("SELECT * FROM users WHERE id IN (1, 2, 3)");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereLike) {
    auto ast = mustParse("SELECT * FROM users WHERE name LIKE '%Alice%'");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereNotEqual) {
    auto ast = mustParse("SELECT * FROM users WHERE status != 'inactive'");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

TEST(SQLParserTest, WhereNotEqualAngleBracket) {
    auto ast = mustParse("SELECT * FROM users WHERE status <> 'inactive'");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
}

// ============================================================================
// SQLParser – error cases
// ============================================================================

TEST(SQLParserTest, InvalidKeyword) {
    SQLParser parser;
    auto result = parser.parse("UPSERT INTO users VALUES (1)");
    EXPECT_FALSE(result.has_value());
}

TEST(SQLParserTest, MissingFromInSelect) {
    SQLParser parser;
    auto result = parser.parse("SELECT * users");
    EXPECT_FALSE(result.has_value());
}

TEST(SQLParserTest, MissingTableName) {
    SQLParser parser;
    auto result = parser.parse("SELECT * FROM");
    EXPECT_FALSE(result.has_value());
}

TEST(SQLParserTest, InsertMissingValues) {
    SQLParser parser;
    auto result = parser.parse("INSERT INTO users (name, age)");
    EXPECT_FALSE(result.has_value());
}

TEST(SQLParserTest, UpdateMissingSet) {
    SQLParser parser;
    auto result = parser.parse("UPDATE users WHERE id = 1");
    EXPECT_FALSE(result.has_value());
}

TEST(SQLParserTest, DeleteMissingFrom) {
    SQLParser parser;
    auto result = parser.parse("DELETE users WHERE id = 1");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// SQLToAQLTranspiler – SELECT transpilation
// ============================================================================

TEST(SQLTranspilerTest, SelectStarToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users");
    EXPECT_NE(aql.find("FOR _doc IN users"), std::string::npos);
    EXPECT_NE(aql.find("RETURN _doc"), std::string::npos);
    EXPECT_EQ(aql.find("FILTER"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectColumnsToAQL) {
    std::string aql = sqlToAQL("SELECT name, age FROM users");
    EXPECT_NE(aql.find("FOR _doc IN users"), std::string::npos);
    EXPECT_NE(aql.find("RETURN"), std::string::npos);
    EXPECT_NE(aql.find("name: _doc.name"), std::string::npos);
    EXPECT_NE(aql.find("age: _doc.age"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithWhereToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE age > 30");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("_doc.age > 30"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithEqualityToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE city = 'Berlin'");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("_doc.city == \"Berlin\""), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithOrderByAscToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users ORDER BY name ASC");
    EXPECT_NE(aql.find("SORT _doc.name ASC"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithOrderByDescToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users ORDER BY price DESC");
    EXPECT_NE(aql.find("SORT _doc.price DESC"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithLimitToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users LIMIT 10");
    EXPECT_NE(aql.find("LIMIT 10"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithLimitOffsetToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users LIMIT 10 OFFSET 20");
    EXPECT_NE(aql.find("LIMIT 20, 10"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithAndConditionToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE age > 18 AND city = 'Berlin'");
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("AND"), std::string::npos);
    EXPECT_NE(aql.find("_doc.age > 18"), std::string::npos);
    EXPECT_NE(aql.find("_doc.city == \"Berlin\""), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithIsNullToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE email IS NULL");
    EXPECT_NE(aql.find("_doc.email == null"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithIsNotNullToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE email IS NOT NULL");
    EXPECT_NE(aql.find("_doc.email != null"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithInListToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE id IN (1, 2, 3)");
    EXPECT_NE(aql.find("_doc.id IN ["), std::string::npos);
}

TEST(SQLTranspilerTest, SelectWithLikeToAQL) {
    std::string aql = sqlToAQL("SELECT * FROM users WHERE name LIKE '%Alice%'");
    EXPECT_NE(aql.find("LIKE("), std::string::npos);
    EXPECT_NE(aql.find("_doc.name"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectSingleColumnReturn) {
    std::string aql = sqlToAQL("SELECT name FROM users");
    EXPECT_NE(aql.find("RETURN _doc.name"), std::string::npos);
}

TEST(SQLTranspilerTest, SelectFullPipeline) {
    std::string aql = sqlToAQL(
        "SELECT name, age FROM users WHERE age > 30 ORDER BY name ASC LIMIT 5");
    EXPECT_NE(aql.find("FOR _doc IN users"), std::string::npos);
    EXPECT_NE(aql.find("FILTER _doc.age > 30"), std::string::npos);
    EXPECT_NE(aql.find("SORT _doc.name ASC"), std::string::npos);
    EXPECT_NE(aql.find("LIMIT 5"), std::string::npos);
    EXPECT_NE(aql.find("RETURN"), std::string::npos);
}

// ============================================================================
// SQLToAQLTranspiler – INSERT transpilation
// ============================================================================

TEST(SQLTranspilerTest, InsertToAQL) {
    std::string aql = sqlToAQL("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    EXPECT_NE(aql.find("INSERT {"), std::string::npos);
    EXPECT_NE(aql.find("name: \"Alice\""), std::string::npos);
    EXPECT_NE(aql.find("age: 30"), std::string::npos);
    EXPECT_NE(aql.find("INTO users"), std::string::npos);
}

TEST(SQLTranspilerTest, InsertNullValueToAQL) {
    std::string aql = sqlToAQL("INSERT INTO users (name, email) VALUES ('Bob', NULL)");
    EXPECT_NE(aql.find("email: null"), std::string::npos);
}

// ============================================================================
// SQLToAQLTranspiler – UPDATE transpilation
// ============================================================================

TEST(SQLTranspilerTest, UpdateToAQL) {
    std::string aql = sqlToAQL("UPDATE users SET age = 31 WHERE name = 'Alice'");
    EXPECT_NE(aql.find("FOR _doc IN users"), std::string::npos);
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("UPDATE _doc WITH {"), std::string::npos);
    EXPECT_NE(aql.find("age: 31"), std::string::npos);
    EXPECT_NE(aql.find("IN users"), std::string::npos);
}

TEST(SQLTranspilerTest, UpdateWithoutWhereToAQL) {
    std::string aql = sqlToAQL("UPDATE products SET active = 0");
    EXPECT_NE(aql.find("FOR _doc IN products"), std::string::npos);
    EXPECT_EQ(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("UPDATE _doc WITH {"), std::string::npos);
}

// ============================================================================
// SQLToAQLTranspiler – DELETE transpilation
// ============================================================================

TEST(SQLTranspilerTest, DeleteToAQL) {
    std::string aql = sqlToAQL("DELETE FROM users WHERE age < 18");
    EXPECT_NE(aql.find("FOR _doc IN users"), std::string::npos);
    EXPECT_NE(aql.find("FILTER _doc.age < 18"), std::string::npos);
    EXPECT_NE(aql.find("REMOVE _doc IN users"), std::string::npos);
}

TEST(SQLTranspilerTest, DeleteWithoutWhereToAQL) {
    std::string aql = sqlToAQL("DELETE FROM temp_table");
    EXPECT_NE(aql.find("FOR _doc IN temp_table"), std::string::npos);
    EXPECT_EQ(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("REMOVE _doc IN temp_table"), std::string::npos);
}

// ============================================================================
// SQLParser – case-insensitive keyword matching
// ============================================================================

TEST(SQLParserTest, CaseInsensitiveKeywords) {
    auto ast = mustParse("select * from users where age > 18 order by name limit 5");
    ASSERT_EQ(ast.statement_type, SQLStatementType::Select);
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->star);
    EXPECT_EQ(ast.select->table, "users");
    ASSERT_TRUE(ast.select->limit.has_value());
    EXPECT_EQ(*ast.select->limit, 5);
    ASSERT_EQ(ast.select->order_by.size(), 1u);
}

// ============================================================================
// SQLParser – string escaping in transpilation
// ============================================================================

TEST(SQLTranspilerTest, StringEscaping) {
    // SQL doubled-single-quote escape: 'O''Brien' should parse as O'Brien
    auto ast = mustParse("SELECT * FROM users WHERE name = 'O''Brien'");
    ASSERT_TRUE(ast.select.has_value());
    EXPECT_TRUE(ast.select->where.has_value());
    std::string aql = mustTranspile(ast);
    EXPECT_NE(aql.find("FILTER"), std::string::npos);
    EXPECT_NE(aql.find("O'Brien"), std::string::npos);
}

// ============================================================================
// SQLParser – Numeric overflow guards (REL-16..17, issue #5177)
// ============================================================================

static bool sqlParseError(const std::string& sql) {
    SQLParser parser;
    auto result = parser.parse(sql);
    return !result.has_value();
}

// LIMIT with out-of-range integer is rejected
TEST(SQLParserTest, LimitOverflowIsError) {
    EXPECT_TRUE(sqlParseError("SELECT * FROM users LIMIT 99999999999999999999"));
}

// OFFSET with out-of-range integer is rejected
TEST(SQLParserTest, OffsetOverflowIsError) {
    EXPECT_TRUE(sqlParseError("SELECT * FROM users LIMIT 10 OFFSET 99999999999999999999"));
}

// Integer literal overflow in a WHERE expression is rejected
TEST(SQLParserTest, IntLiteralOverflowInWhereIsError) {
    EXPECT_TRUE(sqlParseError(
        "SELECT * FROM users WHERE id = 99999999999999999999"));
}

// Float literal overflow in a WHERE expression is rejected
TEST(SQLParserTest, FloatLiteralOverflowInWhereIsError) {
    EXPECT_TRUE(sqlParseError(
        "SELECT * FROM users WHERE score = 1e99999"));
}

// Valid LIMIT / OFFSET are still accepted after adding the guard
TEST(SQLParserTest, ValidLimitOffsetStillAccepted) {
    EXPECT_FALSE(sqlParseError("SELECT * FROM users LIMIT 100 OFFSET 50"));
}
