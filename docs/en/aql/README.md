# 🔍 AQL (Advanced Query Language) Module

**Category:** 🔍 Core AQL  
**Version:** v1.4.0  
**Status:** ✅ Production Ready  
**Date:** January 2026

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB implementiert eine erweiterte Version von **AQL (Advanced Query Language)** – eine deklarative, SQL-ähnliche Abfragesprache mit zusätzlichen Features für Multi-Model-Queries über relationale Daten, Graphen, Vektoren und Dokumente.

### Vollständige AQL-Dokumentation (v1.4)

ThemisDB v1.4 bietet umfassende AQL-Dokumentation mit Performance-Optimierung und Best Practices:

- **[AQL Syntax Guide](AQL_SYNTAX_GUIDE.md)** - Komplette Sprachreferenz mit 50+ Beispielen, Datentypen, Operatoren, Funktionen, Graph/Vector-Operationen
- **[AQL Best Practices](AQL_BEST_PRACTICES.md)** - Query-Struktur, Index-Nutzung, Performance-Optimierung, Sicherheit, Edge Cases, Testing
- **[AQL Performance Guide](AQL_PERFORMANCE_GUIDE.md)** - Query Optimizer Internals, Index-Strategien, Execution Plans, Profiling, Monitoring
- **[AQL Query Templates](aql_query_templates.md)** - Fluent builder and pre-built templates for common AQL patterns (Go client)

## Source Code Reference

| Component | Header | Source | Description |
|-----------|--------|--------|-------------|
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

## ✨ Features & Highlights

### 🎯 Core Features

- **Multi-Model Support:** Relational, graph, vector, and document queries in one language
- **SQL-like Syntax:** Fast adoption through familiar FOR-FILTER-SORT structure
- **Automatic Optimization:** Intelligent index selection via Query Optimizer
- **Graph Traversal:** BFS with OUTBOUND/INBOUND/ANY and SHORTEST_PATH
- **Vector Search:** Native SIMILARITY() with HNSW integration
- **Subqueries & CTEs:** WITH clauses and nested queries
- **Window Functions:** ROW_NUMBER, RANK, LAG, LEAD, etc.
- **Full-Text Search:** BM25-based FULLTEXT() with stemming

---

## 🚀 Quick Start

### Basic Query

```aql
FOR doc IN users
  FILTER doc.age > 18
  SORT doc.name ASC
  LIMIT 0, 10
  RETURN doc
```

### With LET Bindings

```aql
FOR doc IN products
  LET discount = doc.price * 0.1
  LET finalPrice = doc.price - discount
  RETURN {name: doc.name, price: finalPrice}
```

### Graph Traversal

```aql
FOR v, e, p IN 1..3 OUTBOUND @start GRAPH 'social'
  FILTER v.active == true
  RETURN {vertex: v, path: p}
```

### Vector Search with Filters

```aql
FOR doc IN products
  FILTER doc.category == "electronics"
  LET score = SIMILARITY(doc.embedding, @queryVector, 10)
  SORT score DESC
  RETURN {doc, score}
```

---

## 📖 Detailed Documentation

### 🔢 AST Node Types

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

### ⚙️ Operators

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

### 🔧 Built-in Functions

| Category | Functions |
|----------|-----------|
| **String** | `CONCAT`, `SUBSTRING`, `UPPER`, `LOWER`, `LENGTH`, `TRIM` |
| **Math** | `ABS`, `CEIL`, `FLOOR`, `ROUND`, `SUM`, `AVG`, `MIN`, `MAX` |
| **Array** | `LENGTH`, `FIRST`, `LAST`, `NTH`, `FLATTEN`, `UNIQUE` |
| **Geo/Spatial** | `ST_Point`, `ST_Distance`, `ST_Within`, `ST_Contains`, `ST_Intersects` |
| **Vector** | `SIMILARITY`, `PROXIMITY`, `COSINE_SIMILARITY` |
| **Graph** | `SHORTEST_PATH`, traversal operators |
| **Date/Time** | `DATE_NOW`, `DATE_ISO8601`, `DATE_ADD`, `DATE_DIFF` |
| **Window** | `ROW_NUMBER`, `RANK`, `DENSE_RANK`, `LAG`, `LEAD` |

---

## 💡 Best Practices

### ✅ DO: Optimize Index Usage

```aql
-- ✅ GOOD: Uses index on 'city'
FOR u IN users
  FILTER u.city == "Berlin"
  RETURN u

-- ❌ BAD: Cannot use index
FOR u IN users
  FILTER LOWER(u.city) == "berlin"
  RETURN u
```

### ✅ DO: Filter Early

```aql
-- ✅ GOOD: Apply filter early
FOR u IN users
  FILTER u.active == true
  FOR o IN orders
    FILTER o.user_id == u._key
    RETURN o

-- ❌ BAD: Cartesian product then filter
FOR u IN users
  FOR o IN orders
    FILTER u.active == true AND o.user_id == u._key
    RETURN o
```

### ✅ DO: Use CTEs for Reused Subqueries

```aql
-- ✅ GOOD: CTE materialized once
WITH activeUsers AS (
  FOR u IN users FILTER u.active == true RETURN u
)
FOR u IN activeUsers
  RETURN u.name
```

### ⚠️ CAUTION: Nested Loop Performance

```aql
-- Can be expensive with large collections
FOR u IN users
  FOR o IN orders
    FILTER o.user_id == u._key  -- O(n*m) without hash join
    RETURN {user: u, order: o}
```

---

## 🔧 Troubleshooting

### Query Runs Too Slow

**Problem:** Query takes several seconds

**Solution:**
1. Use `explain: true` for plan analysis
2. Check index usage with `plan.mode`
3. Create missing indexes on filter columns
4. Avoid function calls in FILTER (when possible)

### Out of Memory with Large CTEs

**Problem:** `QueryEngine::executeCTEs failed: out of memory`

**Solution:**
- CTECache has automatic spill-to-disk (100MB default)
- Reduce CTE size by early filtering
- Check if CTE is used multiple times (materialization pays off)

### Unexpected Results in Graph Traversal

**Problem:** Too many/few vertices returned

**Solution:**
1. Check depth range: `1..3` vs `1..6`
2. Use FILTER on vertex/edge properties
3. Test with `SHORTEST_PATH` for exact paths
4. Enable tracing for `edges_expanded` metric

---

## 📚 See Also

### 📘 Core Documentation

- [AQL Syntax Reference](../../de/aql/aql_syntax.md) - Complete language definition
- [Query Engine Architecture](../../de/aql/aql_query_engine.md) - Parser, Translator, Optimizer
- [Functions Reference](../../de/aql/aql_functions_reference.md) - All 355+ built-in functions

### 🔎 Advanced Features

- [Hybrid Queries Guide](../../de/aql/aql_hybrid_queries.md) - Vector+Geo, Content+Geo
- [Subquery & CTE Reference](../../de/aql/aql_subquery_reference.md) - WITH, scalar subqueries
- [Pattern Matching](../../de/aql/aql_pattern_matching.md) - Graph patterns without new syntax
- [EXPLAIN & PROFILE](../../de/aql/aql_explain_profile.md) - Performance analysis
- [AQL Query Templates](aql_query_templates.md) - Fluent builder and pre-built templates (Go client)

### ⚙️ Operations & Tooling

- [Prompt Engineering Guide](../../de/aql/aql_prompt_engineering.md) - LLM integration
- [Language Scope Analysis](../../de/aql/aql_language_scope.md) - Feature comparison with other DBs
- [Implementation Status](../../de/aql/IMPLEMENTATION_STATUS_ANALYSIS.md) - What is implemented?

### 🌐 External References

- [ArangoDB AQL Documentation](https://www.arangodb.com/docs/stable/aql/) - Original inspiration
- [Neo4j Cypher](https://neo4j.com/docs/cypher-manual/current/) - Graph query language
- [PostgreSQL](https://www.postgresql.org/docs/current/queries.html) - SQL reference

---

## 📝 Changelog

### v1.4.0 - February 2026
- ✅ **AQL Query Templates:** Added fluent `AQLQueryBuilder` and 14 pre-built template functions in the Go client
- ✅ **Documentation:** Added [AQL Query Templates Guide](aql_query_templates.md)

### v1.3.0 - December 22, 2025
- ✅ **Template Update:** Standardization to v1.3.0 documentation format
- ✅ **Structure:** 8-section format with emojis and TOC
- ✅ **Navigation:** Improved internal links to all AQL documents
- ✅ **Categorization:** Clear separation Core/Advanced/Reference/Operations

### v1.0.0 - December 5, 2025
- Initial release with complete multi-model support
- Parser, Translator, Query Engine complete
- 355+ built-in functions implemented
- Subqueries, CTEs, window functions available

---

> **Note:** Most detailed AQL documentation is currently available in German. English translations are in progress.  
> For the most up-to-date information, please refer to the [German AQL documentation](../../de/aql/).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
