/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_parser.h                                       ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:24:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     778                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • da5848ffb4  2026-03-10  fix: apply all 7 code review recommendations + LSN.toStri... ║
    • a3ec4aa9e9  2026-03-10  refactor: update tenant metrics handling and improve modu... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 190845ecd4  2026-02-23  feat(query): implement multi-statement transaction AQL (B... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

namespace themis {
namespace query {

// Forward declarations
struct ASTNode;
struct Expression;
struct Query; // ensure Query is known before usage in SubqueryExpr

// ============================================================================
// AST Node Types
// ============================================================================

enum class ASTNodeType {
    // Query Nodes
    Query,              // Root node
    ForNode,            // FOR variable IN collection
    FilterNode,         // FILTER condition
    SortNode,           // SORT expr [ASC|DESC]
    LimitNode,          // LIMIT offset, count
    ReturnNode,         // RETURN expression
    LetNode,            // LET variable = expression
    CollectNode,        // COLLECT ... AGGREGATE ... (Phase 2)
    WithNode,           // WITH cteName AS subquery (Phase 3)
    
    // Expressions
    BinaryOp,           // ==, !=, >, <, >=, <=, AND, OR, +, -, *, /
    UnaryOp,            // NOT, -, +
    FunctionCall,       // CONCAT, SUM, LOWER, etc.
    FieldAccess,        // doc.field, doc.nested.field
    Literal,            // "string", 123, true, false, null
    Variable,           // doc, user, etc.
    ArrayLiteral,       // [1, 2, 3] or ["a", "b"]
    ObjectConstruct,    // {name: doc.name, age: doc.age}
    SimilarityCall,     // SIMILARITY(expr, [vector], k?) specialized sugar
    ProximityCall,      // PROXIMITY(expr, [lon,lat]) specialized sugar
    SubqueryExpr,       // Subquery in expression context (Phase 3)
    AnyExpr,            // ANY quantifier for arrays (Phase 3.3)
    AllExpr             // ALL quantifier for arrays (Phase 3.3)
};

// ============================================================================
// Literal Value Types
// ============================================================================

using LiteralValue = std::variant<
    std::nullptr_t,     // null
    bool,               // true/false
    int64_t,            // integers
    double,             // floats
    std::string,        // strings
    nlohmann::json      // complex objects/arrays (for ST_* functions)
>;

// ============================================================================
// Operators
// ============================================================================

enum class BinaryOperator {
    // Comparison
    Eq,                 // ==
    Neq,                // !=
    Lt,                 // <
    Lte,                // <=
    Gt,                 // >
    Gte,                // >=
    
    // Logical
    And,                // AND
    Or,                 // OR
    Xor,                // XOR
    
    // Arithmetic
    Add,                // +
    Sub,                // -
    Mul,                // *
    Div,                // /
    Mod,                // %
    
    // String
    In                  // IN (for membership tests: value IN [array] / value IN variable)
};

enum class UnaryOperator {
    Not,                // NOT
    Minus,              // - (unary minus)
    Plus                // + (unary plus)
};

// ============================================================================
// Expression Nodes
// ============================================================================

struct Expression {
    // Legacy nested type forward declarations for test compatibility
    struct LiteralExpression;
    struct FieldAccessExpression;
    struct BinaryOpExpression;
    struct UnaryOpExpression;
    struct FunctionCallExpression;

    virtual ~Expression() = default;
    virtual ASTNodeType getType() const = 0;
    virtual nlohmann::json toJSON() const = 0;
};

struct LiteralExpr : Expression {
    LiteralValue value;
    
    explicit LiteralExpr(LiteralValue val) : value(std::move(val)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::Literal; }
    nlohmann::json toJSON() const override;
};

struct VariableExpr : Expression {
    std::string name;
    
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::Variable; }
    nlohmann::json toJSON() const override {
        return {{"type", "variable"}, {"name", name}};
    }
};

struct FieldAccessExpr : Expression {
    std::shared_ptr<Expression> object;  // Variable or nested FieldAccess
    std::string field;
    
    FieldAccessExpr(std::shared_ptr<Expression> obj, std::string f)
        : object(std::move(obj)), field(std::move(f)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::FieldAccess; }
    nlohmann::json toJSON() const override;
};

struct BinaryOpExpr : Expression {
    BinaryOperator op;
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    
    BinaryOpExpr(BinaryOperator o, std::shared_ptr<Expression> l, std::shared_ptr<Expression> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::BinaryOp; }
    nlohmann::json toJSON() const override;
};

struct UnaryOpExpr : Expression {
    UnaryOperator op;
    std::shared_ptr<Expression> operand;
    
    UnaryOpExpr(UnaryOperator o, std::shared_ptr<Expression> e)
        : op(o), operand(std::move(e)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::UnaryOp; }
    nlohmann::json toJSON() const override;
};

struct FunctionCallExpr : Expression {
    std::string name;
    std::vector<std::shared_ptr<Expression>> arguments;
    
    FunctionCallExpr(std::string n, std::vector<std::shared_ptr<Expression>> args)
        : name(std::move(n)), arguments(std::move(args)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::FunctionCall; }
    nlohmann::json toJSON() const override;
};

struct SimilarityCallExpr : Expression {
    std::vector<std::shared_ptr<Expression>> arguments; // [fieldAccess, arrayLiteral, optional k]
    SimilarityCallExpr(std::vector<std::shared_ptr<Expression>> args) : arguments(std::move(args)) {}
    ASTNodeType getType() const override { return ASTNodeType::SimilarityCall; }
    nlohmann::json toJSON() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (auto &a : arguments) arr.push_back(a->toJSON());
        return {{"type","similarity_call"},{"arguments",arr}};
    }
};

struct ProximityCallExpr : Expression {
    std::vector<std::shared_ptr<Expression>> arguments; // [fieldAccess, pointArray]
    ProximityCallExpr(std::vector<std::shared_ptr<Expression>> args) : arguments(std::move(args)) {}
    ASTNodeType getType() const override { return ASTNodeType::ProximityCall; }
    nlohmann::json toJSON() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (auto &a : arguments) arr.push_back(a->toJSON());
        return {{"type","proximity_call"},{"arguments",arr}};
    }
};

struct ArrayLiteralExpr : Expression {
    std::vector<std::shared_ptr<Expression>> elements;
    
    explicit ArrayLiteralExpr(std::vector<std::shared_ptr<Expression>> elems)
        : elements(std::move(elems)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::ArrayLiteral; }
    nlohmann::json toJSON() const override;
};

struct ObjectConstructExpr : Expression {
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> fields;
    
    explicit ObjectConstructExpr(std::vector<std::pair<std::string, std::shared_ptr<Expression>>> f)
        : fields(std::move(f)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::ObjectConstruct; }
    nlohmann::json toJSON() const override;
};

// Subquery Expression (Phase 3)
struct SubqueryExpr : Expression {
    std::shared_ptr<Query> subquery;
    
    explicit SubqueryExpr(std::shared_ptr<Query> sq)
        : subquery(std::move(sq)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::SubqueryExpr; }
    nlohmann::json toJSON() const override;
};

// ANY quantifier: ANY var IN array SATISFIES condition
struct AnyExpr : Expression {
    std::string variable;                          // Loop variable
    std::shared_ptr<Expression> arrayExpr;         // Array to iterate
    std::shared_ptr<Expression> condition;         // Condition to test
    
    AnyExpr(std::string var, std::shared_ptr<Expression> arr, std::shared_ptr<Expression> cond)
        : variable(std::move(var)), arrayExpr(std::move(arr)), condition(std::move(cond)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::AnyExpr; }
    nlohmann::json toJSON() const override {
        return {
            {"type", "any"},
            {"variable", variable},
            {"array", arrayExpr ? arrayExpr->toJSON() : nlohmann::json()},
            {"condition", condition ? condition->toJSON() : nlohmann::json()}
        };
    }
};

// ALL quantifier: ALL var IN array SATISFIES condition
struct AllExpr : Expression {
    std::string variable;                          // Loop variable
    std::shared_ptr<Expression> arrayExpr;         // Array to iterate
    std::shared_ptr<Expression> condition;         // Condition to test
    
    AllExpr(std::string var, std::shared_ptr<Expression> arr, std::shared_ptr<Expression> cond)
        : variable(std::move(var)), arrayExpr(std::move(arr)), condition(std::move(cond)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::AllExpr; }
    nlohmann::json toJSON() const override {
        return {
            {"type", "all"},
            {"variable", variable},
            {"array", arrayExpr ? arrayExpr->toJSON() : nlohmann::json()},
            {"condition", condition ? condition->toJSON() : nlohmann::json()}
        };
    }
};

// ============================================================================
// Query Nodes
// ============================================================================

struct SortSpec {
    std::shared_ptr<Expression> expression;
    bool ascending = true;  // true = ASC, false = DESC
    
    nlohmann::json toJSON() const {
        return {
            {"expression", expression->toJSON()},
            {"ascending", ascending}
        };
    }
};

struct ForNode {
    std::string variable;           // Loop variable (e.g., "doc", "user")
    std::string collection;         // Collection name (e.g., "users")
    
    nlohmann::json toJSON() const {
        return {
            {"type", "for"},
            {"variable", variable},
            {"collection", collection}
        };
    }
};

struct FilterNode {
    std::shared_ptr<Expression> condition;
    
    explicit FilterNode(std::shared_ptr<Expression> cond)
        : condition(std::move(cond)) {}
    
    nlohmann::json toJSON() const {
        return {
            {"type", "filter"},
            {"condition", condition->toJSON()}
        };
    }
};

struct SortNode {
    std::vector<SortSpec> specifications;
    
    explicit SortNode(std::vector<SortSpec> specs)
        : specifications(std::move(specs)) {}
    
    nlohmann::json toJSON() const {
        nlohmann::json specs_json = nlohmann::json::array();
        for (const auto& spec : specifications) {
            specs_json.push_back(spec.toJSON());
        }
        return {
            {"type", "sort"},
            {"specifications", specs_json}
        };
    }
};

struct LimitNode {
    int64_t offset = 0;
    int64_t count = 0;
    
    LimitNode(int64_t off, int64_t cnt) : offset(off), count(cnt) {}
    
    nlohmann::json toJSON() const {
        return {
            {"type", "limit"},
            {"offset", offset},
            {"count", count}
        };
    }
};

struct ReturnNode {
    std::shared_ptr<Expression> expression;
    
    explicit ReturnNode(std::shared_ptr<Expression> expr)
        : expression(std::move(expr)) {}
    
    nlohmann::json toJSON() const {
        return {
            {"type", "return"},
            {"expression", expression->toJSON()}
        };
    }
};

// ============================================================================
// LET Node
// ============================================================================

struct LetNode {
    std::string variable;                              // variable name defined by LET
    std::shared_ptr<Expression> expression;            // expression to bind to variable

    nlohmann::json toJSON() const {
        return {
            {"type", "let"},
            {"variable", variable},
            {"expression", expression ? expression->toJSON() : nlohmann::json()}
        };
    }
};

// ============================================================================
// Collect/GROUP BY Nodes (MVP)
// ============================================================================

struct CollectNode {
    // Group-by variables: varName = expression (MVP: typically a single field access like doc.city)
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> groups;

    struct Aggregation {
        std::string varName;                               // output variable name, e.g., "cnt"
        std::string funcName;                              // COUNT, SUM, AVG, MIN, MAX (case-insensitive)
        std::shared_ptr<Expression> argument;              // may be null (COUNT())
    };
    std::vector<Aggregation> aggregations;                 // optional

    nlohmann::json toJSON() const {
        nlohmann::json j;
        j["type"] = "collect";
        nlohmann::json g = nlohmann::json::array();
        for (const auto& [v, e] : groups) {
            g.push_back({{"var", v}, {"expr", e ? e->toJSON() : nlohmann::json()}});
        }
        j["groups"] = g;
        nlohmann::json a = nlohmann::json::array();
        for (const auto& ag : aggregations) {
            a.push_back({{"var", ag.varName}, {"func", ag.funcName}, {"arg", ag.argument ? ag.argument->toJSON() : nlohmann::json()}});
        }
        j["aggregations"] = a;
        return j;
    }
};

// ============================================================================
// WITH Clause / CTEs (Phase 3)
// ============================================================================

// Forward declaration for Query (already declared above)

// Single CTE definition
struct CTEDefinition {
    std::string name;                                  // CTE name (e.g., "expensiveHotels")
    std::shared_ptr<Query> subquery;                   // The subquery AST
    nlohmann::json toJSON() const; // out-of-line defined in aql_parser.cpp
};

// WITH clause node
struct WithNode {
    std::vector<CTEDefinition> ctes;                   // One or more CTEs
    
    nlohmann::json toJSON() const {
        nlohmann::json j;
        j["type"] = "with";
        nlohmann::json ctesJson = nlohmann::json::array();
        for (const auto& cte : ctes) {
            ctesJson.push_back(cte.toJSON());
        }
        j["ctes"] = ctesJson;
        return j;
    }
};

// ============================================================================
// Query AST (Root)
// ============================================================================

struct Query {
    std::shared_ptr<WithNode> with_clause;  // Phase 3: WITH clause for CTEs (optional)
    ForNode for_node; // first FOR (backwards compatibility)
    std::vector<ForNode> for_nodes; // support multiple FOR clauses for joins
    std::vector<std::shared_ptr<FilterNode>> filters;
    std::shared_ptr<SortNode> sort;
    std::shared_ptr<LimitNode> limit;
    std::shared_ptr<ReturnNode> return_node;
    std::vector<LetNode> let_nodes; // LET bindings in order of appearance
    std::shared_ptr<CollectNode> collect; // optional GROUP BY/AGGREGATE
    // Optional: Graph Traversal-Klausel (FOR v[,e[,p]] IN min..max OUTBOUND|INBOUND|ANY start GRAPH name)
    // Wenn gesetzt, beschreibt sie eine Traversal-Query statt einer Collection-Iteration.
    struct TraversalNode {
        enum class Direction { Outbound, Inbound, Any };
        std::string varVertex; // v
        std::string varEdge;   // optional, leer wenn nicht gesetzt (Phase 2)
        std::string varPath;   // optional, leer wenn nicht gesetzt (Phase 2)
        int minDepth = 1;
        int maxDepth = 1;
        Direction direction = Direction::Outbound;
        std::string startVertex; // Primary Key des Startknotens
        std::string graphName;   // Graph-Name (aktuell informativ)
        std::string edgeType;    // optional: Kanten-Typ-Filter (wenn gesetzt)
            bool shortestPath = false; // SHORTEST_PATH Syntax aktiviert
            std::string shortestPathTarget; // Zielknoten für Pfad

        nlohmann::json toJSON() const {
            const char* dir = direction == Direction::Outbound ? "OUTBOUND" : (direction == Direction::Inbound ? "INBOUND" : "ANY");
            nlohmann::json j = {
                {"type", "traversal"},
                {"varVertex", varVertex},
                {"varEdge", varEdge},
                {"varPath", varPath},
                {"minDepth", minDepth},
                {"maxDepth", maxDepth},
                {"direction", dir},
                {"startVertex", startVertex},
                {"graphName", graphName}
            };
            if (!edgeType.empty()) {
                j["edgeType"] = edgeType;
            }
                if (shortestPath) {
                    j["shortestPath"] = true;
                    j["shortestPathTarget"] = shortestPathTarget;
                }
            return j;
        }
    };
    std::shared_ptr<TraversalNode> traversal;
    
    nlohmann::json toJSON() const {
        nlohmann::json j = {
            {"type", "query"},
            {"for", for_node.toJSON()}
        };
        
        if (with_clause) {
            j["with"] = with_clause->toJSON();
        }
        
        if (!for_nodes.empty()) {
            nlohmann::json fors = nlohmann::json::array();
            for (const auto& f : for_nodes) fors.push_back(f.toJSON());
            j["fors"] = std::move(fors);
        }
        
        if (!filters.empty()) {
            nlohmann::json filters_json = nlohmann::json::array();
            for (const auto& filter : filters) {
                filters_json.push_back(filter->toJSON());
            }
            j["filters"] = filters_json;
        }
        
        if (sort) {
            j["sort"] = sort->toJSON();
        }
        
        if (limit) {
            j["limit"] = limit->toJSON();
        }
        
        if (return_node) {
            j["return"] = return_node->toJSON();
        }
        if (!let_nodes.empty()) {
            nlohmann::json lets = nlohmann::json::array();
            for (const auto& l : let_nodes) lets.push_back(l.toJSON());
            j["lets"] = std::move(lets);
        }
        if (collect) {
            j["collect"] = collect->toJSON();
        }
        if (traversal) {
            j["traversal"] = traversal->toJSON();
        }
        
        return j;
    }
};

// ============================================================================
// Parser Error
// ============================================================================

struct ParseError {
    std::string message;
    size_t line = 0;
    size_t column = 0;
    std::string context;  // Snippet of the query around the error
    
    std::string toString() const {
        std::string result = "Parse error at line " + std::to_string(line) 
                           + ", column " + std::to_string(column) + ": " + message;
        if (!context.empty()) {
            result += "\n  " + context;
        }
        return result;
    }
};

// Note: ParseResult struct removed - now using Result<std::shared_ptr<Query>> directly

// ============================================================================
// Multi-Statement Transaction AQL
// ============================================================================

/// Terminal action for a multi-statement transaction block.
enum class AqlTransactionAction {
    Commit,   ///< COMMIT – execute all statements atomically
    Rollback  ///< ROLLBACK – discard all statements
};

/// A parsed multi-statement AQL transaction block.
/// Syntax:
///   BEGIN
///     <AQL statement 1>
///     <AQL statement 2> ...
///   COMMIT | ROLLBACK
struct AqlTransactionBlock {
    std::vector<std::shared_ptr<Query>> statements; ///< Individual AQL queries in order
    AqlTransactionAction action = AqlTransactionAction::Commit;

    nlohmann::json toJSON() const {
        nlohmann::json j;
        j["type"] = "transaction_block";
        j["action"] = (action == AqlTransactionAction::Commit) ? "COMMIT" : "ROLLBACK";
        nlohmann::json stmts = nlohmann::json::array();
        for (const auto& stmt : statements) {
            stmts.push_back(stmt ? stmt->toJSON() : nlohmann::json());
        }
        j["statements"] = stmts;
        return j;
    }
};

// ============================================================================
// AQL Parser
// ============================================================================

class AQLParser {
public:
    AQLParser() = default;
    
    /**
     * Parse an AQL query string into an AST.
     * 
     * @param query_string The AQL query to parse
     * @return Result<std::shared_ptr<Query>> containing either the AST or an error
     * 
     * Example:
     *   auto result = parser.parse("FOR doc IN users FILTER doc.age > 18 RETURN doc");
     *   if (result) {
     *       // Use *result
     *   } else {
     *       // Handle result.error()
     *   }
     */
    Result<std::shared_ptr<Query>> parse(const std::string& query_string);

    /**
     * Parse a multi-statement transaction block.
     *
     * Expects input of the form:
     *   BEGIN
     *     <AQL statement 1>
     *     <AQL statement 2>
     *     ...
     *   COMMIT | ROLLBACK
     *
     * Each statement must be a valid AQL query (starting with FOR or WITH).
     *
     * @param input  The full multi-statement AQL transaction string.
     * @return       Result<AqlTransactionBlock> or an error.
     */
    Result<AqlTransactionBlock> parseTransactionBlock(const std::string& input);

    /**
     * @brief Parse a standalone AQL expression string into an Expression tree.
     *
     * Used by QueryEngine evaluators to evaluate filter conditions independently
     * of a full FOR…RETURN query.  Kept public because external callers such as
     * QueryEngine::evalAqlExpression() and QueryExpressionEvaluator::canEvaluate()
     * construct a local AQLParser and call this method directly.
     */
    std::shared_ptr<Expression> parseExpression(const std::string& expr_str);

private:
    // Helper methods (implemented in aql_parser.cpp)
    std::shared_ptr<Expression> parsePrimaryExpression(const std::string& expr_str);
    BinaryOperator stringToOperator(const std::string& op_str);
    // New: parse membership expression left IN right
    std::shared_ptr<Expression> parseMembership(std::shared_ptr<Expression> left);
};

}  // namespace query
}  // namespace themis

// ---------------------------------------------------------------------------
// Backward-compatibility shim for older tests
// Provides Expression::... types used by legacy tests while mapping to
// the current Expression evaluation in LetEvaluator.
// ---------------------------------------------------------------------------
namespace themis {
namespace query {

// Generic JSON literal expression (legacy tests pass arbitrary JSON)
struct JsonLiteralExpr : Expression {
    nlohmann::json value;
    JsonLiteralExpr() = default;
    explicit JsonLiteralExpr(const nlohmann::json& v) : value(v) {}
    ASTNodeType getType() const override { return ASTNodeType::Literal; }
    nlohmann::json toJSON() const override {
        return {{"type","literal"},{"value", value}};
    }
};

// Legacy field-access with path vector, e.g., {"doc","address","city"}
struct PathFieldAccessExpr : Expression {
    std::vector<std::string> path; // may include numeric indices as strings
    PathFieldAccessExpr() = default;
    explicit PathFieldAccessExpr(std::vector<std::string> p) : path(std::move(p)) {}
    ASTNodeType getType() const override { return ASTNodeType::FieldAccess; }
    nlohmann::json toJSON() const override {
        return {{"type","field_access"},{"path", path}};
    }
};

// Legacy binary op with string operator
struct StringBinaryOpExpr : Expression {
    std::string op; // "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "AND", "OR"
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    StringBinaryOpExpr() = default;
    ASTNodeType getType() const override { return ASTNodeType::BinaryOp; }
    nlohmann::json toJSON() const override {
        return {{"type","binary_op"},{"op",op},{"left", left? left->toJSON(): nlohmann::json()},{"right", right? right->toJSON(): nlohmann::json()}};
    }
};

// Legacy unary op with string operator ("NOT", "-")
struct StringUnaryOpExpr : Expression {
    std::string op;
    std::shared_ptr<Expression> operand;
    StringUnaryOpExpr() = default;
    ASTNodeType getType() const override { return ASTNodeType::UnaryOp; }
    nlohmann::json toJSON() const override {
        return {{"type","unary_op"},{"op",op},{"operand", operand? operand->toJSON(): nlohmann::json()}};
    }
};

// Legacy function-call with explicit functionName and arguments
struct CompatFunctionCallExpr : Expression {
    std::string functionName;
    std::vector<std::shared_ptr<Expression>> arguments;
    CompatFunctionCallExpr() = default;
    ASTNodeType getType() const override { return ASTNodeType::FunctionCall; }
    nlohmann::json toJSON() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (auto &a : arguments) arr.push_back(a? a->toJSON(): nlohmann::json());
        return {{"type","function_call"},{"name", functionName},{"arguments", arr}};
    }
};

// Define nested legacy types inside base `Expression` for compatibility
struct Expression::LiteralExpression : public JsonLiteralExpr {
    using JsonLiteralExpr::JsonLiteralExpr;
    LiteralExpression() = default;
};
struct Expression::FieldAccessExpression : public PathFieldAccessExpr {
    using PathFieldAccessExpr::PathFieldAccessExpr;
    FieldAccessExpression() = default;
};
struct Expression::BinaryOpExpression : public StringBinaryOpExpr {
    using StringBinaryOpExpr::StringBinaryOpExpr;
    BinaryOpExpression() = default;
};
struct Expression::UnaryOpExpression : public StringUnaryOpExpr {
    using StringUnaryOpExpr::StringUnaryOpExpr;
    UnaryOpExpression() = default;
};
struct Expression::FunctionCallExpression : public CompatFunctionCallExpr {
    using CompatFunctionCallExpr::CompatFunctionCallExpr;
    FunctionCallExpression() = default;
};

} // namespace query
} // namespace themis
