/**
 * @file sql_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.16
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

namespace themis {
namespace query {

// ============================================================================
// SQL Statement Types
// ============================================================================

enum class SQLStatementType {
    Select,
    Insert,
    Update,
    Delete
};

// ============================================================================
// SQL Value – a literal value in a SQL statement
// ============================================================================

using SQLValue = std::variant<
    std::nullptr_t,  // NULL
    bool,            // TRUE / FALSE
    int64_t,         // integer
    double,          // float
    std::string      // string / identifier
>;

// ============================================================================
// SQL Expression nodes (WHERE conditions, SET values)
// ============================================================================

enum class SQLExprType {
    Literal,         // constant value
    Column,          // col or table.col reference
    BinaryOp,        // =, !=, <, <=, >, >=, AND, OR, IN, LIKE
    UnaryOp,         // NOT, IS NULL, IS NOT NULL
    List             // (val1, val2, ...) for IN predicate
};

struct SQLExpr {
    virtual ~SQLExpr() = default;
    [[nodiscard]] virtual SQLExprType type() const = 0;
    [[nodiscard]] virtual std::string toAQL(const std::string& var) const = 0;
};

struct SQLLiteralExpr : SQLExpr {
    SQLValue value;
    explicit SQLLiteralExpr(SQLValue v) : value(std::move(v)) {}
    SQLExprType type() const override { return SQLExprType::Literal; }
    std::string toAQL(const std::string& var) const override;
};

struct SQLColumnExpr : SQLExpr {
    std::string table;   // optional table alias prefix
    std::string column;
    SQLColumnExpr(std::string t, std::string c) : table(std::move(t)), column(std::move(c)) {}
    explicit SQLColumnExpr(std::string c) : column(std::move(c)) {}
    SQLExprType type() const override { return SQLExprType::Column; }
    std::string toAQL(const std::string& var) const override;
};

struct SQLBinaryOpExpr : SQLExpr {
    std::string op;  // "==", "!=", "<", "<=", ">", ">=", "AND", "OR", "IN", "LIKE"
    std::shared_ptr<SQLExpr> left;
    std::shared_ptr<SQLExpr> right;
    SQLBinaryOpExpr(std::string o, std::shared_ptr<SQLExpr> l, std::shared_ptr<SQLExpr> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    SQLExprType type() const override { return SQLExprType::BinaryOp; }
    std::string toAQL(const std::string& var) const override;
};

struct SQLUnaryOpExpr : SQLExpr {
    std::string op;  // "NOT", "IS_NULL", "IS_NOT_NULL"
    std::shared_ptr<SQLExpr> operand;
    SQLUnaryOpExpr(std::string o, std::shared_ptr<SQLExpr> e) : op(std::move(o)), operand(std::move(e)) {}
    SQLExprType type() const override { return SQLExprType::UnaryOp; }
    std::string toAQL(const std::string& var) const override;
};

struct SQLListExpr : SQLExpr {
    std::vector<std::shared_ptr<SQLExpr>> elements;
    explicit SQLListExpr(std::vector<std::shared_ptr<SQLExpr>> elems) : elements(std::move(elems)) {}
    SQLExprType type() const override { return SQLExprType::List; }
    std::string toAQL(const std::string& var) const override;
};

// ============================================================================
// SQL Statement AST nodes
// ============================================================================

struct SQLSortSpec {
    std::string column;
    bool ascending = true;
};

struct SQLSelectStatement {
    bool star = false;  // SELECT *
    std::vector<std::string> columns;
    std::string table;
    std::optional<std::shared_ptr<SQLExpr>> where;
    std::vector<SQLSortSpec> order_by;
    std::optional<int64_t> limit;
    std::optional<int64_t> offset;
};

struct SQLInsertStatement {
    std::string table;
    std::vector<std::string> columns;
    std::vector<SQLValue> values;
};

struct SQLAssignment {
    std::string column;
    SQLValue value;
};

struct SQLUpdateStatement {
    std::string table;
    std::vector<SQLAssignment> assignments;
    std::optional<std::shared_ptr<SQLExpr>> where;
};

struct SQLDeleteStatement {
    std::string table;
    std::optional<std::shared_ptr<SQLExpr>> where;
};

// ============================================================================
// SQL AST – top-level node wrapping the concrete statement
// ============================================================================

struct SQLASTNode {
    SQLStatementType statement_type;

    // Exactly one of the following is set, matching statement_type
    std::optional<SQLSelectStatement> select;
    std::optional<SQLInsertStatement> insert;
    std::optional<SQLUpdateStatement> update;
    std::optional<SQLDeleteStatement> del;
};

// ============================================================================
// SQL Parse Error
// ============================================================================

struct SQLParseError {
    std::string message;
    size_t position = 0;  // character offset into the query string

    std::string toString() const {
        return "SQL parse error at position " + std::to_string(position) + ": " + message;
    }
};

// ============================================================================
// SQLParser
// Parses a SQL query string (SELECT/INSERT/UPDATE/DELETE) into an SQLASTNode.
// Thread-safety: NOT thread-safe; create one instance per thread or protect
// with a mutex (same constraint as AQLParser).
// ============================================================================

/** @brief with a mutex (same constraint as AQLParser). */
class SQLParser {
public:
    SQLParser() = default;

    /**
     * Parse a SQL query string into an AST.
     *
     * Supported dialects: PostgreSQL / MySQL compatible subset covering
     * SELECT, INSERT INTO, UPDATE … SET, DELETE FROM.
     *
     * @param sql_query  The SQL statement to parse.
     * @return           Result<SQLASTNode> – the AST on success, or a
     *                   SQLParseError converted to themis::Error on failure.
     *
     * Example:
     *   SQLParser parser;
     *   auto result = parser.parse("SELECT name, age FROM users WHERE age > 30");
     *   if (result) { ... use result.value() ... }
     */
    Result<SQLASTNode> parse(const std::string& sql_query);

private:
    // Internal tokenizer / parser state
    struct Token;
    struct Lexer;
    struct Parser;
};

// ============================================================================
// SQLToAQLTranspiler
// Translates an SQLASTNode into an AQL query string suitable for execution
// through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator).
// ============================================================================

/** @brief through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator). */
class SQLToAQLTranspiler {
public:
    SQLToAQLTranspiler() = default;

    /**
     * Translate a SQL AST into an AQL query string.
     *
     * @param ast  The parsed SQL AST.
     * @return     Result<std::string> – the AQL string on success.
     *
     * Translation examples:
     *   SELECT name, age FROM users WHERE age > 30 ORDER BY name
     *   →  FOR _doc IN users FILTER _doc.age > 30 SORT _doc.name ASC
     *      RETURN {name: _doc.name, age: _doc.age}
     *
     *   INSERT INTO users (name, age) VALUES ("Alice", 30)
     *   →  INSERT {name: "Alice", age: 30} INTO users
     *
     *   UPDATE users SET age = 31 WHERE name == "Alice"
     *   →  FOR _doc IN users FILTER _doc.name == "Alice"
     *      UPDATE _doc WITH {age: 31} IN users
     *
     *   DELETE FROM users WHERE age < 18
     *   →  FOR _doc IN users FILTER _doc.age < 18 REMOVE _doc IN users
     */
    Result<std::string> transpile(const SQLASTNode& ast);

private:
    static std::string transpileSelect(const SQLSelectStatement& stmt);
    static std::string transpileInsert(const SQLInsertStatement& stmt);
    static std::string transpileUpdate(const SQLUpdateStatement& stmt);
    static std::string transpileDelete(const SQLDeleteStatement& stmt);
    static std::string valueToAQL(const SQLValue& val);
};

}  // namespace query
}  // namespace themis
