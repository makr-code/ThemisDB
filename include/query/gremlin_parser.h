/**
 * @file gremlin_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Gremlin compatibility layer – graph traversal query parsing and AQL
// transpilation.  The generated AQL can be fed directly into executeAql().
//
// Thread-safety note: GremlinParser instances are NOT thread-safe.
// Create one instance per thread or protect shared instances with a mutex.

#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "utils/expected.h"

namespace themis {
namespace query {

// ============================================================================
// Gremlin value type
// ============================================================================

using GremlinValue = std::variant<
    std::nullptr_t,  // null
    bool,
    int64_t,
    double,
    std::string
>;

// ============================================================================
// Gremlin predicate (P.eq, P.gt, P.within, …)
// ============================================================================

enum class GremlinPredOp {
    Eq, Neq, Lt, Lte, Gt, Gte,
    Within, Without
};

struct GremlinPredicate {
    GremlinPredOp op;
    std::vector<GremlinValue> values;
};

// ============================================================================
// Gremlin step kinds
// ============================================================================

enum class GremlinStepKind {
    // Start steps
    V, E,
    // Filter steps
    Has, HasLabel, HasNot, HasId,
    // Traversal steps
    Out, In, Both, OutE, InE, BothE,
    OutV, InV, BothV,
    // Terminal / aggregation steps
    Values, ValueMap, Id, Label,
    Count, Dedup, Limit, Range,
    As, Select,
    Order, By
};

// ============================================================================
// A single Gremlin traversal step
// ============================================================================

struct GremlinStep {
    GremlinStepKind kind;

    // General-purpose string arguments (labels, property keys, aliases)
    std::vector<std::string> strings;

    // General-purpose value arguments (IDs, literal values)
    std::vector<GremlinValue> values;

    // Predicate (for has() with P.xxx)
    std::optional<GremlinPredicate> predicate;

    // Count arguments for limit() / range() / repeat()
    std::optional<int64_t> count;
    std::optional<int64_t> count2;  // upper bound for range()

    // Sort direction for by()
    bool ascending = true;
};

// ============================================================================
// Gremlin traversal AST
// ============================================================================

struct GremlinASTNode {
    // Ordered list of traversal steps (first step should be V or E)
    std::vector<GremlinStep> steps;
};

// ============================================================================
// Parse error
// ============================================================================

struct GremlinParseError {
    std::string message = {};
    size_t position = 0;

    std::string toString() const {
        return "Gremlin parse error at position " + std::to_string(position)
               + ": " + message;
    }
};

// ============================================================================
// GremlinParser
// Parses a subset of the Apache TinkerPop Gremlin traversal language into a
// GremlinASTNode.  Supports: g.V(), g.E(), hasLabel(), has(), hasNot(),
// out()/in()/both(), values(), valueMap(), id(), label(), count(), dedup(),
// limit(), range(), as(), select(), order().by().
//
// Thread-safety: NOT thread-safe; create one instance per thread or protect
// with a mutex.
// ============================================================================

/** @brief with a mutex. */
class GremlinParser {
public:
    GremlinParser() = default;

    /**
     * Parse a Gremlin traversal string into an AST.
     *
     * @param gremlin  The Gremlin traversal string (must start with "g.V()"
     *                 or "g.E()").
     * @return         Result<GremlinASTNode> on success, or a parse error.
     *
     * Example:
     *   GremlinParser p;
     *   auto ast = p.parse("g.V().hasLabel('User').has('age', P.gt(18))");
     */
    Result<GremlinASTNode> parse(const std::string& gremlin);

private:
    struct Token;
    struct Lexer;
    struct Parser;
};

// ============================================================================
// GremlinToAQLTranspiler
// Translates a GremlinASTNode into an AQL query string suitable for execution
// through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator).
// ============================================================================

/** @brief through the existing AQL pipeline (executeAql / AQLParser / AQLTranslator). */
class GremlinToAQLTranspiler {
public:
    GremlinToAQLTranspiler() = default;

    /**
     * Translate a Gremlin traversal AST into an AQL query string.
     *
     * Translation examples:
     *   g.V().hasLabel('User').has('name','Alice').out('FRIEND').values('name')
     *   → FOR _v IN User FILTER _v.name == "Alice"
     *       FOR _e, _n IN 1..1 OUTBOUND _v GRAPH "FRIEND"
     *       RETURN _n.name
     *
     *   g.V().hasLabel('User').count()
     *   → RETURN LENGTH(FOR _v IN User RETURN _v)
     *
     * @param ast  The parsed Gremlin AST.
     * @return     Result<std::string> – the AQL string on success.
     */
    Result<std::string> transpile(const GremlinASTNode& ast);

private:
    static std::string valueToAQL(const GremlinValue& val);
    static std::string predicateToAQL(const GremlinPredicate& pred,
                                      const std::string& lhs);
};

}  // namespace query
}  // namespace themis
