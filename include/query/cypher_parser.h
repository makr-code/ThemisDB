/**
 * @file cypher_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Cypher compatibility layer – MATCH/WHERE/RETURN query parsing and AQL
// transpilation.  The generated AQL can be fed directly into executeAql().
//
// Thread-safety note: CypherParser instances are NOT thread-safe.
// Create one instance per thread or protect shared instances with a mutex.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "utils/expected.h"

namespace themis {
namespace query {

// ============================================================================
// Cypher value types
// ============================================================================

using CypherLiteralValue = std::variant<
    std::nullptr_t,  // NULL
    bool,            // true / false
    int64_t,         // integer
    double,          // float
    std::string      // string
>;

// ============================================================================
// Cypher node pattern
// ============================================================================

struct CypherPropertyFilter {
    std::string key = {};
    CypherLiteralValue value;
};

struct CypherNodePattern {
    std::string variable;             ///< empty when anonymous
    std::vector<std::string> labels;  ///< can be multiple: (n:A:B)
    std::vector<CypherPropertyFilter> properties; ///< inline { key: value }
};

// ============================================================================
// Cypher relationship pattern
// ============================================================================

enum class CypherRelDirection {
    Out,   // (a)-[r]->(b)
    In,    // (a)<-[r]-(b)
    Both   // (a)-[r]-(b)
};

struct CypherRelPattern {
    std::string variable;             ///< empty when anonymous
    std::vector<std::string> types;   ///< edge type labels
    CypherRelDirection direction = CypherRelDirection::Out;
    std::optional<int> min_hops;      ///< variable-length: *min..max
    std::optional<int> max_hops;
};

// ============================================================================
// Cypher path pattern: node (-rel-> node)*
// ============================================================================

struct CypherPathSegment {
    CypherRelPattern rel;
    CypherNodePattern node;
};

struct CypherPathPattern {
    CypherNodePattern start;
    std::vector<CypherPathSegment> segments;
};

// ============================================================================
// Cypher WHERE expression
// ============================================================================

enum class CypherExprType {
    Literal,
    Property,   // n.prop
    BinaryOp,
    UnaryOp
};

struct CypherExpr {
    virtual ~CypherExpr() = default;
    virtual CypherExprType exprType() const = 0;
};

struct CypherLiteralExpr : CypherExpr {
    CypherLiteralValue value;
    explicit CypherLiteralExpr(CypherLiteralValue v) : value(std::move(v)) {}
    CypherExprType exprType() const override { return CypherExprType::Literal; }
};

struct CypherPropertyExpr : CypherExpr {
    std::string variable = {};
    std::string property = {};
    CypherPropertyExpr(std::string v, std::string p)
        : variable(std::move(v)), property(std::move(p)) {}
    CypherExprType exprType() const override { return CypherExprType::Property; }
};

struct CypherBinaryOpExpr : CypherExpr {
    std::string op;   ///< "=", "<>", "<", "<=", ">", ">=", "AND", "OR",
                      ///< "IN", "STARTS WITH", "ENDS WITH", "CONTAINS",
                      ///< "NOT IN"
    std::shared_ptr<CypherExpr> left;
    std::shared_ptr<CypherExpr> right;
    CypherBinaryOpExpr(std::string o,
                       std::shared_ptr<CypherExpr> l,
                       std::shared_ptr<CypherExpr> r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    CypherExprType exprType() const override { return CypherExprType::BinaryOp; }
};

struct CypherUnaryOpExpr : CypherExpr {
    std::string op;  ///< "NOT", "IS NULL", "IS NOT NULL"
    std::shared_ptr<CypherExpr> operand;
    CypherUnaryOpExpr(std::string o, std::shared_ptr<CypherExpr> e)
        : op(std::move(o)), operand(std::move(e)) {}
    CypherExprType exprType() const override { return CypherExprType::UnaryOp; }
};

// ============================================================================
// Cypher RETURN item
// ============================================================================

struct CypherReturnItem {
    bool star = false;                 ///< RETURN *
    bool distinct = false;             ///< RETURN DISTINCT
    std::string expression;            ///< raw expression text (variable or prop ref)
    std::string alias;                 ///< AS alias, empty if not present
};

// ============================================================================
// Cypher ORDER BY
// ============================================================================

struct CypherSortSpec {
    std::string expression;
    bool ascending = true;
};

// ============================================================================
// Cypher AST
// ============================================================================

struct CypherASTNode {
    // MATCH clause
    std::vector<CypherPathPattern> match_patterns;

    // WHERE clause (optional)
    std::shared_ptr<CypherExpr> where;

    // RETURN clause
    std::vector<CypherReturnItem> return_items;
    bool return_distinct = false;

    // ORDER BY / SKIP / LIMIT
    std::vector<CypherSortSpec> order_by;
    std::optional<int64_t> skip;
    std::optional<int64_t> limit;
};

// ============================================================================
// Parse error
// ============================================================================

struct CypherParseError {
    std::string message = {};
    size_t position = 0;

    std::string toString() const {
        return "Cypher parse error at position " + std::to_string(position)
               + ": " + message;
    }
};

// ============================================================================
// CypherParser
// Parses a subset of openCypher MATCH/WHERE/RETURN queries into a CypherASTNode.
// Thread-safety: NOT thread-safe; create one instance per thread.
// ============================================================================

/** @brief Thread-safety: NOT thread-safe; create one instance per thread. */
class CypherParser {
public:
    CypherParser() = default;

    /**
     * Parse a Cypher MATCH … [WHERE …] RETURN … query string into an AST.
     *
     * Supported clauses: MATCH, WHERE, RETURN (with DISTINCT, ORDER BY,
     * SKIP, LIMIT).
     *
     * @param cypher_query  The Cypher query to parse.
     * @return              Result<CypherASTNode> – the AST on success, or a
     *                      CypherParseError message on failure.
     *
     * Example:
     *   CypherParser p;
     *   auto ast = p.parse("MATCH (n:User) WHERE n.age > 18 RETURN n.name");
     */
    Result<CypherASTNode> parse(const std::string& cypher_query);

private:
    struct Token;
    struct Lexer;
    struct Parser;
};

// ============================================================================
// CypherToAQLTranspiler
// Translates a CypherASTNode into an AQL query string suitable for execution
// through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator).
// ============================================================================

/** @brief through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator). */
class CypherToAQLTranspiler {
public:
    CypherToAQLTranspiler() = default;

    /**
     * Translate a Cypher AST into an AQL query string.
     *
     * Translation examples:
     *   MATCH (n:User) WHERE n.age > 18 RETURN n.name
     *   → FOR n IN User FILTER n.age > 18 RETURN n.name
     *
     *   MATCH (n:User)-[:FRIEND]->(m:User) RETURN n, m
     *   → FOR n IN User FOR e, m IN 1..1 OUTBOUND n GRAPH "FRIEND" RETURN {n: n, m: m}
     *
     * @param ast  The parsed Cypher AST.
     * @return     Result<std::string> – the AQL string on success.
     */
    Result<std::string> transpile(const CypherASTNode& ast);

private:
    static std::string literalToAQL(const CypherLiteralValue& val);
    static std::string exprToAQL(const CypherExpr& expr, const std::string& default_var);
    static std::string nodePatternToFilter(const CypherNodePattern& node,
                                           const std::string& var);
};

}  // namespace query
}  // namespace themis

