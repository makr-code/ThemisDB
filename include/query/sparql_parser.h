/**
 * @file sparql_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
// SPARQL Term
// ============================================================================

enum class SPARQLTermType {
    Variable,      // ?var or $var
    URIRef,        // <uri>
    PrefixedName,  // prefix:local or bare name
    Literal        // "string", 123, 3.14, true, false
};

/// A literal value carried by a Literal term
using SPARQLLiteralValue = std::variant<
    std::nullptr_t,  // NULL
    bool,            // true / false
    int64_t,         // integer
    double,          // float
    std::string      // string / URI content
>;

/// One component of a triple pattern (subject, predicate, or object)
struct SPARQLTerm {
    SPARQLTermType   type;
    std::string      value;           ///< variable name (without ?/$), URI, prefix:local, or raw literal text
    SPARQLLiteralValue literal_value; ///< populated when type == Literal
    bool             is_literal_value = false;
};

// ============================================================================
// SPARQL Filter Expression AST
// ============================================================================

enum class SPARQLExprType {
    Literal,
    Variable,
    BinaryOp,
    UnaryOp
};

struct SPARQLExpr {
    virtual ~SPARQLExpr() = default;
    virtual SPARQLExprType type() const = 0;
};

struct SPARQLLiteralExpr : SPARQLExpr {
    SPARQLLiteralValue value;
    SPARQLExprType type() const override { return SPARQLExprType::Literal; }
};

struct SPARQLVariableExpr : SPARQLExpr {
    std::string name;  ///< variable name without ?/$
    SPARQLExprType type() const override { return SPARQLExprType::Variable; }
};

struct SPARQLBinaryOpExpr : SPARQLExpr {
    std::string                  op;     ///< "==", "!=", "<", "<=", ">", ">=", "&&", "||"
    std::shared_ptr<SPARQLExpr>  left;
    std::shared_ptr<SPARQLExpr>  right;
    SPARQLExprType type() const override { return SPARQLExprType::BinaryOp; }
};

struct SPARQLUnaryOpExpr : SPARQLExpr {
    std::string                  op;     ///< "!"
    std::shared_ptr<SPARQLExpr>  operand;
    SPARQLExprType type() const override { return SPARQLExprType::UnaryOp; }
};

// ============================================================================
// SPARQL Where-Clause items
// ============================================================================

/// A triple pattern: subject predicate object
struct SPARQLTriplePattern {
    SPARQLTerm subject;
    SPARQLTerm predicate;
    SPARQLTerm object;
};

enum class SPARQLClauseKind {
    TriplePattern,
    Filter
};

struct SPARQLWhereClause {
    SPARQLClauseKind                  kind;
    std::optional<SPARQLTriplePattern> triple;
    std::shared_ptr<SPARQLExpr>        filter_expr;  ///< set when kind == Filter
};

// ============================================================================
// SPARQL SELECT statement AST
// ============================================================================

struct SPARQLOrderSpec {
    std::string variable;   ///< variable name without ?/$
    bool        ascending = true;
};

struct SPARQLSelectStatement {
    bool                          star = false;    ///< SELECT *
    std::vector<std::string>      variables;       ///< projected variable names (without ?/$)
    std::optional<std::string>    from_graph;      ///< FROM URI (optional)
    std::vector<SPARQLWhereClause> where_clauses;
    std::vector<SPARQLOrderSpec>  order_by;
    std::optional<int64_t>        limit;
    std::optional<int64_t>        offset;
};

/// Top-level SPARQL AST node (SELECT only in this release)
struct SPARQLASTNode {
    SPARQLSelectStatement select;
};

// ============================================================================
// SPARQLParser
//
// Parses a SPARQL 1.1 SELECT query subset into a SPARQLASTNode.
//
// Supported features:
//   SELECT ?var1 ?var2 | SELECT *
//   FROM <graph_uri>                    (optional)
//   WHERE { triple-pattern* FILTER()* }
//   ORDER BY [ASC|DESC](?var) | ?var
//   LIMIT n [OFFSET m]
//
// Triple-pattern terms: ?var, <URI>, prefix:local, string/integer/float/bool literals
// FILTER: comparison (==, !=, <, <=, >, >=), logical (&&, ||, !), parentheses
//
// Thread-safety: NOT thread-safe; create one instance per thread or protect
// with a mutex (same constraint as AQLParser and SQLParser).
// ============================================================================

/** @brief with a mutex (same constraint as AQLParser and SQLParser). */
class SPARQLParser {
public:
    SPARQLParser() = default;

    /**
     * Parse a SPARQL SELECT query into an AST.
     *
     * @param sparql_query  The SPARQL query string.
     * @return              Result<SPARQLASTNode> – AST on success, or an error.
     *
     * Example:
     *   SPARQLParser parser;
     *   auto result = parser.parse(
     *     "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10");
     *   if (result) { ... use result.value() ... }
     */
    Result<SPARQLASTNode> parse(const std::string& sparql_query);

private:
    struct Token;
    struct Lexer;
    struct ParserImpl;
};

// ============================================================================
// SPARQLToAQLTranspiler
//
// Translates a SPARQLASTNode into an AQL query string suitable for execution
// through the existing AQL pipeline.
//
// RDF triples are expected to be stored as documents in a collection with the
// fields "subject", "predicate", and "object".  The default collection name is
// "rdf_triples"; pass a custom name to the constructor to override.
//
// Translation examples:
//
//   SELECT ?s ?p ?o WHERE { ?s ?p ?o }
//   → FOR _t0 IN rdf_triples
//     RETURN {s: _t0.subject, p: _t0.predicate, o: _t0.object}
//
//   SELECT ?person ?name WHERE {
//     ?person rdf:type foaf:Person .
//     ?person foaf:name ?name .
//   }
//   → FOR _t0 IN rdf_triples
//       FILTER _t0.predicate == "rdf:type" AND _t0.object == "foaf:Person"
//     FOR _t1 IN rdf_triples
//       FILTER _t1.subject == _t0.subject AND _t1.predicate == "foaf:name"
//     RETURN {person: _t0.subject, name: _t1.object}
//
//   SELECT ?s WHERE { ?s ex:value ?v . FILTER(?v > 100) } LIMIT 5
//   → FOR _t0 IN rdf_triples
//       FILTER _t0.predicate == "ex:value"
//     FILTER (_t0.object > 100)
//     LIMIT 5
//     RETURN _t0.subject
// ============================================================================

/** @brief RETURN _t0.subject. */
class SPARQLToAQLTranspiler {
public:
    explicit SPARQLToAQLTranspiler(std::string collection = "rdf_triples")
        : collection_(std::move(collection)) {}

    /**
     * Translate a SPARQL AST into an AQL query string.
     *
     * @param ast  The parsed SPARQL AST.
     * @return     Result<std::string> – AQL string on success.
     */
    Result<std::string> transpile(const SPARQLASTNode& ast);

private:
    std::string collection_;

    std::string transpileSelect(const SPARQLSelectStatement& stmt);
};

}  // namespace query
}  // namespace themis

