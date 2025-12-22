# AQL (ArangoDB Query Language) Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** AQL

---

## Übersicht

ThemisDB implementiert eine erweiterte Version von AQL (ArangoDB Query Language) mit zusätzlichen Features für Multi-Model-Queries.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AQLParser | `aql_parser.h` | `aql_parser.cpp` | Parser & AST |
| AQLTranslator | `aql_translator.h` | `aql_translator.cpp` | AST → Execution Plan |
| AQLRunner | `aql_runner.h` | `aql_runner.cpp` | Query Execution |
| QueryOptimizer | `query_optimizer.h` | `query_optimizer.cpp` | Plan Optimization |
| LetEvaluator | `let_evaluator.h` | `let_evaluator.cpp` | LET Bindings |
| CTECache | `cte_cache.h` | `cte_cache.cpp` | WITH Clauses |
| WindowEvaluator | `window_evaluator.h` | `window_evaluator.cpp` | Window Functions |

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
    // Membership
    In
};

enum class UnaryOperator {
    Not, Minus, Plus
};
```

## Query Syntax

### Basic Query
```aql
FOR doc IN users
  FILTER doc.age > 18
  SORT doc.name ASC
  LIMIT 0, 10
  RETURN doc
```

### LET Bindings
```aql
FOR doc IN products
  LET discount = doc.price * 0.1
  LET finalPrice = doc.price - discount
  RETURN {name: doc.name, price: finalPrice}
```

### COLLECT / Aggregation
```aql
FOR doc IN orders
  COLLECT category = doc.category
  AGGREGATE total = SUM(doc.amount), count = LENGTH(1)
  RETURN {category, total, count}
```

### Subqueries
```aql
FOR user IN users
  LET orders = (
    FOR order IN orders
      FILTER order.user_id == user.id
      RETURN order
  )
  RETURN {user, orderCount: LENGTH(orders)}
```

### Graph Traversal
```aql
FOR v, e, p IN 1..3 OUTBOUND @start GRAPH 'social'
  FILTER v.active == true
  RETURN {vertex: v, path: p}
```

### Hybrid Queries (Vector + Filter)
```aql
FOR doc IN products
  FILTER doc.category == "electronics"
  LET score = SIMILARITY(doc.embedding, @queryVector, 10)
  SORT score DESC
  RETURN {doc, score}
```

### Window Functions
```aql
FOR doc IN sales
  COLLECT year = DATE_YEAR(doc.date)
  AGGREGATE total = SUM(doc.amount)
  WINDOW running = SUM(total) OVER (ORDER BY year)
  RETURN {year, total, running}
```

## Built-in Functions

| Category | Functions |
|----------|-----------|
| **String** | CONCAT, LOWER, UPPER, TRIM, SUBSTRING, LENGTH |
| **Math** | SUM, AVG, MIN, MAX, COUNT, FLOOR, CEIL, ROUND |
| **Date** | DATE_NOW, DATE_YEAR, DATE_MONTH, DATE_DAY |
| **Array** | LENGTH, FIRST, LAST, PUSH, POP, FLATTEN |
| **Geo** | ST_DISTANCE, ST_CONTAINS, ST_INTERSECTS |
| **Vector** | SIMILARITY, PROXIMITY |

## Verwandte Dokumentation

- [aql_syntax.md](aql_syntax.md) - Vollständige Syntax-Referenz
- [aql_hybrid_queries.md](aql_hybrid_queries.md) - Hybrid Query Details
- [aql_pattern_matching.md](aql_pattern_matching.md) - Pattern Matching
- [aql_subquery_reference.md](aql_subquery_reference.md) - Subquery-Referenz
