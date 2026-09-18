/**
 * @file aql_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <set>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

namespace themis {
namespace query {

// Forward declarations
struct ASTNode;
struct Expression;
struct Query; // ensure Query is known before usage in SubqueryExpr

// ============================================================================
// Scope Validation (Phase 2)
// ============================================================================

class ParserScopeContext {
public:
    ParserScopeContext() = default;
    ~ParserScopeContext() = default;

    /**
     * @brief Register Collection.
     * @param[in] collection_name Name of the collection.
     */
    void registerCollection(const std::string& collection_name);

    [[nodiscard]] bool isCollectionInScope(const std::string& collection_name) const;

    [[nodiscard]] Result<bool> validateCollectionAccess(
        const std::string& collection_name,
        const std::string& context_description) const;

    /**
     * @brief Push Scope.
     */
    void pushScope();

    /**
     * @brief Pop Scope.
     */
    void popScope();

    [[nodiscard]] const std::set<std::string>& getRegisteredCollections() const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    std::set<std::string> registered_collections_;
    std::vector<std::set<std::string>> scope_stack_;
    std::string current_scope_prefix_;
    std::vector<std::string> scope_prefix_stack_;
};

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

    // Mutation Nodes (EPIC-004 Phase 1)
    Insert,             // INSERT {doc} INTO collection
    Update,             // UPDATE collection SET / UPDATE {search} WITH {update} IN collection
    Remove,             // REMOVE {doc} IN collection / DELETE FROM collection WHERE ...
    Replace,            // REPLACE {search} WITH {replacement} IN collection
    Upsert,             // UPSERT {search} INSERT {doc} UPDATE {upd} IN collection

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
    AllExpr,            // ALL quantifier for arrays (Phase 3.3)
    SearchClauseNode    // SEARCH ... IN field (Phase 6 FTS)
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

    /**
     * @brief Expression.
     * @return Return value.
     */
    virtual ~Expression() = default;
    /**
     * @brief Get Type.
     * @return Return value.
     */
    virtual ASTNodeType getType() const = 0;
    /**
     * @brief To JSON.
     * @return Return value.
     */
    virtual nlohmann::json toJSON() const = 0;
};

struct LiteralExpr : Expression {
    LiteralValue value;
    
    explicit LiteralExpr(LiteralValue val) : value(std::move(val)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::Literal; }
    nlohmann::json toJSON() const override;
};

struct VariableExpr : Expression {
    std::string name = {};
    
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    
    ASTNodeType getType() const override { return ASTNodeType::Variable; }
    nlohmann::json toJSON() const override {
        return {{"type", "variable"}, {"name", name}};
    }
};

struct FieldAccessExpr : Expression {
    std::shared_ptr<Expression> object;  // Variable or nested FieldAccess
    std::string field = {};
    
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
    std::string name = {};
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
        for (auto &a : arguments) {
          arr.push_back(a->toJSON());
        }
        return {{"type","similarity_call"},{"arguments",arr}};
    }
};

struct ProximityCallExpr : Expression {
    std::vector<std::shared_ptr<Expression>> arguments; // [fieldAccess, pointArray]
    ProximityCallExpr(std::vector<std::shared_ptr<Expression>> args) : arguments(std::move(args)) {}
    ASTNodeType getType() const override { return ASTNodeType::ProximityCall; }
    nlohmann::json toJSON() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (auto &a : arguments) {
          arr.push_back(a->toJSON());
        }
        return {{"type","proximity_call"},{"arguments",arr}};
    }
};

struct ArrayLiteralExpr : Expression {
    std::vector<std::shared_ptr<Expression>> elements;
    
    /**
     * @brief Array Literal Expr.
     * @param[in] elems Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Subquery Expr.
     * @param[in] sq Input parameter.
     * @return Return value.
     */
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
// Mutation AST Nodes (EPIC-004 Phase 1)
// ============================================================================

struct MutationNode {
    /**
     * @brief Mutation Node.
     * @return Return value.
     */
    virtual ~MutationNode() = default;

    /**
     * @brief Get Type.
     * @return Return value.
     */
    virtual ASTNodeType getType() const = 0;

    /**
     * @brief To JSON.
     * @return Return value.
     */
    virtual nlohmann::json toJSON() const = 0;
};

struct SetClause {
    std::string field;                         ///< Target field path (may contain dots)
    std::shared_ptr<Expression> value;         ///< Right-hand side expression

    nlohmann::json toJSON() const {
        return {{"field", field},
                {"value", value ? value->toJSON() : nlohmann::json()}};
    }
};

struct InsertNode : MutationNode {
    std::string collection;                                ///< Target collection
    std::vector<std::shared_ptr<Expression>> documents;   ///< One or more document expressions
    bool return_new = false;                               ///< RETURN NEW requested

    ASTNodeType getType() const override { return ASTNodeType::Insert; }
    nlohmann::json toJSON() const override {
        nlohmann::json docs = nlohmann::json::array();
        for (const auto& d : documents) {
          docs.push_back(d ? d->toJSON() : nlohmann::json());
        }
        return {{"type", "INSERT"},
                {"collection", collection},
                {"documents", docs},
                {"return_new", return_new}};
    }
};

struct UpdateNode : MutationNode {
    std::string collection;                    ///< Target collection
    std::shared_ptr<Expression> filter;        ///< WHERE / FILTER condition (may be nullptr)
    std::vector<SetClause> set_clauses;        ///< SET k=v pairs (SQL-style)
    std::shared_ptr<Expression> search_expr;  ///< AQL-native search expression
    std::shared_ptr<Expression> update_expr;  ///< AQL-native WITH expression
    bool return_new = false;                   ///< RETURN NEW
    bool return_old = false;                   ///< RETURN OLD
    std::optional<int64_t> limit;              ///< Optional LIMIT count

    ASTNodeType getType() const override { return ASTNodeType::Update; }
    nlohmann::json toJSON() const override {
        nlohmann::json j{{"type", "UPDATE"}, {"collection", collection}};
        if (filter) {
          j["filter"] = filter->toJSON();
        }
        if (!set_clauses.empty()) {
            nlohmann::json sc = nlohmann::json::array();
            for (const auto& c : set_clauses) {
              sc.push_back(c.toJSON());
            }
            j["set_clauses"] = sc;
        }
        if (search_expr) {
          j["search_expr"] = search_expr->toJSON();
        }
        if (update_expr) {
          j["update_expr"]  = update_expr->toJSON();
        }
        j["return_new"] = return_new;
        j["return_old"] = return_old;
        if (limit.has_value()) {
          j["limit"] = *limit;
        }
        return j;
    }
};

struct RemoveNode : MutationNode {
    std::string collection;                    ///< Target collection
    std::shared_ptr<Expression> filter;        ///< WHERE / FILTER condition (may be nullptr)
    std::shared_ptr<Expression> doc_expr;      ///< AQL-native document expression
    bool return_removed = false;               ///< RETURN OLD (removed document)
    std::optional<int64_t> limit;              ///< Optional LIMIT count

    ASTNodeType getType() const override { return ASTNodeType::Remove; }
    nlohmann::json toJSON() const override {
        nlohmann::json j{{"type", "REMOVE"}, {"collection", collection}};
        if (filter) {
          j["filter"]   = filter->toJSON();
        }
        if (doc_expr) {
          j["doc_expr"] = doc_expr->toJSON();
        }
        j["return_removed"] = return_removed;
        if (limit.has_value()) {
          j["limit"] = *limit;
        }
        return j;
    }
};

struct ReplaceNode : MutationNode {
    std::string collection;                    ///< Target collection
    std::shared_ptr<Expression> search_expr;  ///< Search expression
    std::shared_ptr<Expression> replacement;   ///< Replacement document expression
    bool return_new = false;                   ///< RETURN NEW
    bool return_old = false;                   ///< RETURN OLD

    ASTNodeType getType() const override { return ASTNodeType::Replace; }
    nlohmann::json toJSON() const override {
        nlohmann::json j{{"type", "REPLACE"}, {"collection", collection}};
        if (search_expr) {
          j["search_expr"]  = search_expr->toJSON();
        }
        if (replacement) {
          j["replacement"]   = replacement->toJSON();
        }
        j["return_new"] = return_new;
        j["return_old"] = return_old;
        return j;
    }
};

struct UpsertNode : MutationNode {
    std::string collection;                    ///< Target collection
    std::shared_ptr<Expression> search_expr;  ///< Search / match expression
    std::shared_ptr<Expression> insert_doc;   ///< Document to insert when no match
    std::shared_ptr<Expression> update_doc;   ///< Update expression when match found
    bool return_new = false;                   ///< RETURN NEW
    bool return_old = false;                   ///< RETURN OLD

    ASTNodeType getType() const override { return ASTNodeType::Upsert; }
    nlohmann::json toJSON() const override {
        nlohmann::json j{{"type", "UPSERT"}, {"collection", collection}};
        if (search_expr) {
          j["search_expr"] = search_expr->toJSON();
        }
        if (insert_doc) {
          j["insert_doc"]  = insert_doc->toJSON();
        }
        if (update_doc) {
          j["update_doc"]  = update_doc->toJSON();
        }
        j["return_new"] = return_new;
        j["return_old"] = return_old;
        return j;
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
    
    /**
     * @brief Filter Node.
     * @param[in] cond Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Sort Node.
     * @param[in] specs Input parameter.
     * @return Return value.
     */
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
    
    /**
     * @brief Return Node.
     * @param[in] expr Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief To JSON.
     * @return Return value.
     */
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
// FTS (Full-Text Search) types — Phase 6, SEARCH clause (Target: Q3–Q4 2026)
// ============================================================================

enum class FtsPredType : uint8_t {
    TERM,       ///< Simple term match (default).
    PHRASE,     ///< Exact phrase match ("hello world").
    PROXIMITY,  ///< Proximity / NEAR match (term1 NEAR[n] term2).
    PREFIX,     ///< Prefix / STARTS_WITH match.
};

struct FtsPredicateNode {
    std::string field;
    std::string term;
    FtsPredType pred_type{FtsPredType::TERM};
    double boost{1.0};
    std::string analyzer;
    uint32_t proximity_distance{0};
};

struct SearchClauseNode {
    std::vector<FtsPredicateNode> predicates;
    std::string in_field;
    std::string default_analyzer;
    double top_boost{1.0};

    ASTNodeType getType() const noexcept { return ASTNodeType::SearchClauseNode; }
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
    std::shared_ptr<SearchClauseNode> search_clause; // optional SEARCH clause (Phase 6 FTS)
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
            for (const auto& f : for_nodes) {
              fors.push_back(f.toJSON());
            }
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
            for (const auto& l : let_nodes) {
              lets.push_back(l.toJSON());
            }
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
    std::string message = {};
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
// Continuous Query DDL AST (Phase 8.1)
// ============================================================================

enum class ContinuousQueryDDLType {
    CREATE,    ///< CREATE CONTINUOUS QUERY …
    DROP,      ///< DROP   CONTINUOUS QUERY NAME
    SHOW,      ///< SHOW   CONTINUOUS QUERIES
    DESCRIBE   ///< DESCRIBE CONTINUOUS QUERY NAME
};

struct ContinuousQueryDDL {
    ContinuousQueryDDLType ddl_type{ContinuousQueryDDLType::SHOW};

    std::string query_name;

    struct CreateSpec {
        std::string source_collection;  ///< ON COLLECTION
        std::string window_type;        ///< "TIME" | "COUNT" | "TUMBLING"
        int64_t     range_ms{0};        ///< TIME/TUMBLING: window width ms
        int64_t     slide_ms{0};        ///< TIME: slide interval ms
        int64_t     rows{0};            ///< COUNT: window width in tuples
        int64_t     slide_rows{0};      ///< COUNT: slide step in tuples
        std::string aql_body;           ///< AQL expression after RETURN
    };
    CreateSpec spec;

    nlohmann::json toJSON() const {
        auto type_str = [&]() -> std::string {
            switch (ddl_type) {
                case ContinuousQueryDDLType::CREATE:   return "CREATE";
                case ContinuousQueryDDLType::DROP:     return "DROP";
                case ContinuousQueryDDLType::SHOW:     return "SHOW";
                case ContinuousQueryDDLType::DESCRIBE: return "DESCRIBE";
            }
            return "UNKNOWN";
        };
        nlohmann::json j{{"ddl_type", type_str()}, {"query_name", query_name}};
        if (ddl_type == ContinuousQueryDDLType::CREATE) {
            j["spec"] = {
                {"source_collection", spec.source_collection},
                {"window_type",       spec.window_type},
                {"range_ms",          spec.range_ms},
                {"slide_ms",          spec.slide_ms},
                {"rows",              spec.rows},
                {"slide_rows",        spec.slide_rows},
                {"aql_body",          spec.aql_body}
            };
        }
        return j;
    }
};

// ============================================================================
// Schema DDL (AQL Phase 2 — CREATE/DROP/ALTER COLLECTION/INDEX/VIEW)
// ============================================================================

enum class SchemaDDLType {
    CREATE_COLLECTION, ///< CREATE COLLECTION name [OPTIONS {...}]
    DROP_COLLECTION,   ///< DROP COLLECTION name [IF EXISTS]
    CREATE_INDEX,      ///< CREATE [UNIQUE] INDEX name ON collection (fields…)
    DROP_INDEX,        ///< DROP INDEX name ON collection [IF EXISTS]
    CREATE_VIEW,       ///< CREATE VIEW name AS FOR … RETURN …
    DROP_VIEW,         ///< DROP VIEW name [IF EXISTS]
    ALTER_COLLECTION   ///< ALTER COLLECTION name SET OPTIONS {...}
};

struct FieldDef {
    std::string name;           ///< Field name (e.g. "email").
    std::string type_hint;      ///< Optional type hint (e.g. "string", "number", "geo").
    bool        nullable{true}; ///< Whether the field may be null/absent.
};

struct IndexDef {
    std::string            name;            ///< Index name.
    std::string            collection;      ///< Target collection.
    std::vector<FieldDef>  fields;          ///< Indexed fields.
    bool                   unique{false};   ///< Unique constraint.
    bool                   sparse{false};   ///< Sparse (skip null values).
    std::string            index_type;      ///< "hash", "skiplist", "geo", "fulltext", "vector".
};

struct SchemaDDL {
    SchemaDDLType          ddl_type{SchemaDDLType::CREATE_COLLECTION};
    std::string            name;            ///< Object name (collection/index/view).
    std::string            collection;      ///< Relevant collection (for index ops).
    bool                   if_exists{false};///< IF EXISTS/IF NOT EXISTS modifier.
    std::vector<FieldDef>  fields;          ///< Field list (for CREATE COLLECTION).
    IndexDef               index_def;       ///< Index descriptor (for CREATE INDEX).
    std::string            view_body;       ///< AQL body (for CREATE VIEW … AS …).
    nlohmann::json         options;         ///< Engine-specific options (optional).

    [[nodiscard]] std::string typeString() const {
        switch (ddl_type) {
            case SchemaDDLType::CREATE_COLLECTION: return "CREATE COLLECTION";
            case SchemaDDLType::DROP_COLLECTION:   return "DROP COLLECTION";
            case SchemaDDLType::CREATE_INDEX:      return "CREATE INDEX";
            case SchemaDDLType::DROP_INDEX:        return "DROP INDEX";
            case SchemaDDLType::CREATE_VIEW:       return "CREATE VIEW";
            case SchemaDDLType::DROP_VIEW:         return "DROP VIEW";
            case SchemaDDLType::ALTER_COLLECTION:  return "ALTER COLLECTION";
        }
        return "UNKNOWN";
    }
};

// ============================================================================
// Multi-Statement Transaction AQL
// ============================================================================

enum class AqlTransactionAction {
    Commit,   ///< COMMIT – execute all statements atomically
    Rollback  ///< ROLLBACK – discard all statements
};

// ============================================================================
// AqlStatement — Phase 4: mixed query/mutation transaction entry
// ============================================================================

struct AqlStatement {
    enum class Kind {
        Query,    ///< A read query (FOR / WITH), held in @c query.
        Mutation, ///< A DML mutation (INSERT / UPDATE / …), held in @c mutation.
    };

    Kind kind = Kind::Query;
    std::shared_ptr<Query>        query;    ///< Set when kind == Kind::Query.
    std::shared_ptr<MutationNode> mutation; ///< Set when kind == Kind::Mutation.
};

// ============================================================================
// AqlTransactionBlock
// ============================================================================

struct AqlTransactionBlock {
    std::vector<std::shared_ptr<Query>> statements;

    std::vector<AqlStatement> ordered_statements;

    AqlTransactionAction action = AqlTransactionAction::Commit;

    nlohmann::json toJSON() const {
        nlohmann::json j;
        j["type"] = "transaction_block";
        j["action"] = (action == AqlTransactionAction::Commit) ? "COMMIT" : "ROLLBACK";

        if (!ordered_statements.empty()) {
            nlohmann::json stmts = nlohmann::json::array();
            for (const auto& s : ordered_statements) {
                if (s.kind == AqlStatement::Kind::Mutation && s.mutation) {
                    stmts.push_back(s.mutation->toJSON());
                } else if (s.query) {
                    stmts.push_back(s.query->toJSON());
                } else {
                    stmts.push_back(nlohmann::json());
                }
            }
            j["statements"] = stmts;
        } else {
            nlohmann::json stmts = nlohmann::json::array();
            for (const auto& stmt : statements) {
                stmts.push_back(stmt ? stmt->toJSON() : nlohmann::json());
            }
            j["statements"] = stmts;
        }
        return j;
    }
};

// ============================================================================
// AQL Parser
// ============================================================================

class AQLParser {
public:
    AQLParser() = default;
    ~AQLParser() = default;
    
    AQLParser(AQLParser&&) noexcept = default;
    
    AQLParser& operator=(AQLParser&&) noexcept = default;
    
    // Delete copy operations (stateless but still follows best practices)
    AQLParser(const AQLParser&) = delete;
    AQLParser& operator=(const AQLParser&) = delete;
    
    /**
     * @brief Parse.
     * @param[in] query_string Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<Query>> parse(const std::string& query_string);

    /**
     * @brief Parse Transaction Block.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    Result<AqlTransactionBlock> parseTransactionBlock(const std::string& input);

    /**
     * @brief Parse Expression.
     * @param[in] expr_str Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Expression> parseExpression(const std::string& expr_str);

    [[nodiscard]] Result<ContinuousQueryDDL> parseDDL(const std::string& input);

    [[nodiscard]] Result<std::shared_ptr<MutationNode>> parseMutation(const std::string& input);

    [[nodiscard]] Result<SchemaDDL> parseSchemaDDL(const std::string& input);

private:
    /**
     * @brief Helper methods (implemented in aql_parser.
     * @param[in] expr_str Input parameter.
     * @return Return value.
     * @details cpp)
     */
    std::shared_ptr<Expression> parsePrimaryExpression(const std::string& expr_str);
    /**
     * @brief String To Operator.
     * @param[in] op_str Input parameter.
     * @return Return value.
     */
    BinaryOperator stringToOperator(const std::string& op_str);
    /**
     * @brief New: parse membership expression left IN right
     * @param[in] left Input parameter.
     * @return Return value.
     */
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
    std::string op = {};
    std::shared_ptr<Expression> operand;
    StringUnaryOpExpr() = default;
    ASTNodeType getType() const override { return ASTNodeType::UnaryOp; }
    nlohmann::json toJSON() const override {
        return {{"type","unary_op"},{"op",op},{"operand", operand? operand->toJSON(): nlohmann::json()}};
    }
};

// Legacy function-call with explicit functionName and arguments
struct CompatFunctionCallExpr : Expression {
    std::string functionName = {};
    std::vector<std::shared_ptr<Expression>> arguments;
    CompatFunctionCallExpr() = default;
    ASTNodeType getType() const override { return ASTNodeType::FunctionCall; }
    nlohmann::json toJSON() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (auto &a : arguments) {
          arr.push_back(a? a->toJSON(): nlohmann::json());
        }
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
