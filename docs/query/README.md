# Query Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Query

---

## Übersicht

Das Query-Modul implementiert den AQL-Parser, Query-Optimizer und Query-Engine für ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AQLParser | `aql_parser.h` | `aql_parser.cpp` | AQL Parser & AST |
| AQLTranslator | `aql_translator.h` | `aql_translator.cpp` | AST → Execution Plan |
| QueryEngine | `query_engine.h` | `query_engine.cpp` | Query Execution |
| QueryOptimizer | `query_optimizer.h` | `query_optimizer.cpp` | Plan Optimization |
| SemanticCache | `semantic_cache.h` | `semantic_cache.cpp` | Query Result Cache |
| WindowEvaluator | `window_evaluator.h` | `window_evaluator.cpp` | Window Functions |
| LetEvaluator | `let_evaluator.h` | `let_evaluator.cpp` | LET Bindings |
| CTECache | `cte_cache.h` | `cte_cache.cpp` | Common Table Expressions |
| SubqueryOptimizer | `subquery_optimizer.h` | `subquery_optimizer.cpp` | Subquery Optimization |

**Gesamt:** 12 Header, 12 Source-Dateien, ~12,500 LOC

## AST Node Types

```cpp
enum class ASTNodeType {
    // Query Nodes
    Query,              // Root node
    ForNode,            // FOR variable IN collection
    FilterNode,         // FILTER condition
    SortNode,           // SORT expr [ASC|DESC]
    LimitNode,          // LIMIT offset, count
    ReturnNode,         // RETURN expression
    LetNode,            // LET variable = expression
    CollectNode,        // COLLECT ... AGGREGATE ...
    WithNode,           // WITH cteName AS subquery
    
    // Expressions
    BinaryOp,           // ==, !=, >, <, AND, OR, +, -, *, /
    UnaryOp,            // NOT, -, +
    FunctionCall,       // CONCAT, SUM, LOWER, etc.
    FieldAccess,        // doc.field, doc.nested.field
    Literal,            // "string", 123, true, false, null
    Variable,           // doc, user
    ArrayLiteral,       // [1, 2, 3]
    ObjectConstruct,    // {name: doc.name}
    SimilarityCall,     // SIMILARITY(expr, [vector], k?)
    ProximityCall,      // PROXIMITY(expr, [lon,lat])
    SubqueryExpr,       // Subquery in expression
    AnyExpr,            // ANY quantifier
    AllExpr             // ALL quantifier
};
```

## Operators

```cpp
enum class BinaryOperator {
    // Comparison
    Eq, Neq, Lt, Lte, Gt, Gte,
    // Logical
    And, Or, Xor,
    // Arithmetic
    Add, Sub, Mul, Div, Mod,
    // Special
    In, Like, Regex
};
```

## Query Types

### AQL Queries
```sql
FOR doc IN users
  FILTER doc.age > 18
  SORT doc.name ASC
  LIMIT 0, 10
  RETURN doc
```

### Hybrid Queries (Vector + Filter)
```sql
FOR doc IN products
  FILTER doc.category == "electronics"
  LET score = SIMILARITY(doc.embedding, @queryVector, 10)
  SORT score DESC
  RETURN {doc, score}
```

### Graph Traversal
```sql
FOR v, e, p IN 1..3 OUTBOUND @start GRAPH 'social'
  FILTER v.active == true
  RETURN {vertex: v, path: p}
```

### Window Functions
```sql
FOR doc IN sales
  COLLECT year = DATE_YEAR(doc.date)
  AGGREGATE total = SUM(doc.amount)
  WINDOW running = SUM(total) OVER (ORDER BY year)
  RETURN {year, total, running}
```

## QueryOptimizer

```cpp
class QueryOptimizer {
    // Optimization Rules
    enum class Rule {
        PushDownFilter,      // Move FILTERs closer to scan
        IndexSelection,      // Select best index
        JoinReordering,      // Optimize join order
        SubqueryFlattening,  // Flatten correlated subqueries
        ConstantFolding,     // Evaluate constants at compile time
        DeadCodeElimination  // Remove unused expressions
    };
    
    ExecutionPlan optimize(const Query& ast);
    void applyRule(Rule rule, ExecutionPlan& plan);
};
```

## Query Engine

```cpp
class QueryEngine {
    // Execute AQL query
    Result execute(const std::string& aql, const Bindings& params);
    
    // Prepare for repeated execution
    PreparedQuery prepare(const std::string& aql);
    Result execute(const PreparedQuery& query, const Bindings& params);
    
    // Explain query plan
    ExplainResult explain(const std::string& aql);
};
```

## Verwandte Dokumentation

- [AQL Syntax](../aql/aql_syntax.md) - AQL-Referenz
- [Hybrid Search](query_vector_hybrid.md) - Vector + Filter
- [Query Benchmarks](query_hybrid_benchmarks.md) - Performance
