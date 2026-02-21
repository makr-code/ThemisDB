# AQL Query Template Library

**Category:** 🔍 Core AQL  
**Module:** Go Client (`clients/go`)  
**Version:** v1.4.0+  
**Status:** ✅ Production Ready  
**Date:** February 2026

---

## 📑 Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Fluent Builder Reference](#fluent-builder-reference)
  - [For / ForRange / ForTraversal](#for--forrange--fortraversal)
  - [Let](#let)
  - [Filter](#filter)
  - [Collect](#collect)
  - [Sort](#sort)
  - [Limit](#limit)
  - [Return / ReturnDistinct](#return--returndistinct)
  - [Build](#build)
- [Template Functions Reference](#template-functions-reference)
  - [SimpleQueryTemplate](#simplequerytemplate)
  - [JoinQueryTemplate](#joinquerytemplate)
  - [GraphTraversalTemplate](#graphtraversaltemplate)
  - [AggregationTemplate](#aggregationtemplate)
  - [VectorSearchTemplate](#vectorsearchtemplate)
  - [DML Templates](#dml-templates)
  - [PaginatedQueryTemplate](#paginatedquerytemplate)
  - [LLM Templates](#llm-templates)
- [Constants](#constants)
- [Best Practices](#best-practices)
- [See Also](#see-also)

---

## Overview

The AQL Query Template Library provides two complementary ways to build AQL queries
without writing raw query strings:

1. **`AQLQueryBuilder`** – a fluent, method-chaining builder for constructing any AQL
   query clause by clause.
2. **Template functions** – one-liner helpers that cover the most common query patterns
   (simple select, join, graph traversal, aggregation, vector search, DML, LLM, pagination).

Both are located in `clients/go/aql_templates.go` and require no additional dependencies.

---

## Quick Start

```go
import themisdb "github.com/makr-code/ThemisDB/clients/go"

ctx := context.Background()
client := themisdb.NewClient(themisdb.Config{
    Endpoints: []string{"http://localhost:8080"},
})

// --- Fluent builder ---
query := themisdb.NewAQLQuery().
    For("u", "users").
    Filter("u.age > 18").
    Sort("u.name", themisdb.SortAsc).
    Limit(0, 10).
    Return("u").
    Build()

var users []map[string]interface{}
_ = client.Query(ctx, query, &users)

// --- One-liner template ---
q := themisdb.SimpleQueryTemplate("orders", "o", "o.status == 'open'", "o.created_at", themisdb.SortDesc, 50)
_ = client.Query(ctx, q, &orders)
```

---

## Fluent Builder Reference

### For / ForRange / ForTraversal

```go
// Iterate a collection
NewAQLQuery().For("u", "users")
// => FOR u IN users

// Iterate a numeric range
NewAQLQuery().ForRange("i", 1, 5)
// => FOR i IN 1..5

// Graph traversal – vertex only
NewAQLQuery().ForTraversal("v", "", "", 1, 3, TraversalOutbound, "'users/alice'", "friends")
// => FOR v IN 1..3 OUTBOUND 'users/alice' friends

// Graph traversal – vertex + edge + path
NewAQLQuery().ForTraversal("v", "e", "p", 1, 3, TraversalOutbound, "'users/alice'", "friends")
// => FOR v, e, p IN 1..3 OUTBOUND 'users/alice' friends

// Multiple edge collections
NewAQLQuery().ForTraversal("v", "", "", 1, 3, TraversalAny, "'users/alice'", "friends", "colleagues")
// => FOR v IN 1..3 ANY 'users/alice' friends, colleagues
```

**Parameters:**

| Parameter | Description |
|-----------|-------------|
| `vertexVar` | Vertex variable name (required) |
| `edgeVar` | Edge variable name (empty to omit) |
| `pathVar` | Path variable name (empty to omit) |
| `minDepth` | Minimum traversal depth |
| `maxDepth` | Maximum traversal depth |
| `direction` | `TraversalOutbound`, `TraversalInbound`, or `TraversalAny` |
| `startVertex` | Start vertex expression, e.g. `"'users/alice'"` |
| `edgeCollections` | One or more edge collection names |

---

### Let

```go
NewAQLQuery().Let("fullName", "CONCAT(u.first, ' ', u.last)")
// => LET fullName = CONCAT(u.first, ' ', u.last)
```

Multiple `Let` calls add multiple LET clauses.

---

### Filter

```go
NewAQLQuery().
    For("u", "users").
    Filter("u.age > 18").
    Filter("u.active == true")
// =>
// FOR u IN users
// FILTER u.age > 18
// FILTER u.active == true
```

Multiple `Filter` calls add separate FILTER clauses (AND semantics at the engine level).

---

### Collect

```go
// With AGGREGATE
NewAQLQuery().Collect("city = o.city", "total = SUM(o.amount)")
// => COLLECT city = o.city AGGREGATE total = SUM(o.amount)

// Group only (no aggregation)
NewAQLQuery().Collect("city = u.city", "")
// => COLLECT city = u.city
```

---

### Sort

```go
// Single sort key
NewAQLQuery().Sort("u.name", SortAsc)
// => SORT u.name ASC

// Multiple sort keys (call Sort multiple times)
NewAQLQuery().Sort("u.city", SortAsc).Sort("u.name", SortAsc)
// => SORT u.city ASC, u.name ASC
```

---

### Limit

```go
// No offset
NewAQLQuery().Limit(0, 10)
// => LIMIT 10

// With offset
NewAQLQuery().Limit(20, 10)
// => LIMIT 20, 10
```

---

### Return / ReturnDistinct

```go
NewAQLQuery().Return("u")
// => RETURN u

NewAQLQuery().Return("{ name: u.name, age: u.age }")
// => RETURN { name: u.name, age: u.age }

NewAQLQuery().ReturnDistinct("u.city")
// => RETURN DISTINCT u.city
```

---

### Build

`Build()` assembles all clauses in AQL order and returns the final query string:

```
FOR ...
FOR ...      (nested loops)
LET ...
FILTER ...
COLLECT ...
SORT ...
LIMIT ...
RETURN ...
```

---

## Template Functions Reference

### SimpleQueryTemplate

```go
func SimpleQueryTemplate(
    collection, variable string,
    filterCondition string,     // empty → no FILTER
    sortField string,           // empty → no SORT
    sortDir SortDirection,
    limit int,                  // 0 → no LIMIT
) string
```

**Example:**

```go
q := SimpleQueryTemplate("users", "u", "u.age > 18", "u.name", SortAsc, 10)
// FOR u IN users
// FILTER u.age > 18
// SORT u.name ASC
// LIMIT 10
// RETURN u
```

---

### JoinQueryTemplate

```go
func JoinQueryTemplate(
    collA, varA string,
    collB, varB string,
    joinCondition string,
    projection string,
) string
```

**Example:**

```go
q := JoinQueryTemplate(
    "users", "u",
    "orders", "o",
    "o.user_id == u._key",
    "{ user_name: u.name, order_total: o.total }",
)
// FOR u IN users
// FOR o IN orders
// FILTER o.user_id == u._key
// RETURN { user_name: u.name, order_total: o.total }
```

---

### GraphTraversalTemplate

```go
func GraphTraversalTemplate(
    vertexVar string,
    minDepth, maxDepth int,
    direction TraversalDirection,
    startVertex string,
    edgeCollection string,
    returnExpr string,
) string
```

**Example:**

```go
q := GraphTraversalTemplate("v", 1, 3, TraversalOutbound, "'users/john'", "friends", "v.name")
// FOR v IN 1..3 OUTBOUND 'users/john' friends
// RETURN v.name
```

---

### AggregationTemplate

```go
func AggregationTemplate(
    collection, variable string,
    filterCondition string,     // empty → no FILTER
    groupExpr string,           // e.g. "city = o.city"
    aggregation string,         // e.g. "total = SUM(o.amount)"
    sortField string,           // empty → no SORT
    sortDir SortDirection,
    returnExpr string,
) string
```

**Example:**

```go
q := AggregationTemplate(
    "orders", "o",
    "o.status == 'completed'",
    "city = o.city",
    "total_sales = SUM(o.amount)",
    "total_sales", SortDesc,
    "{ city, total_sales }",
)
// FOR o IN orders
// FILTER o.status == 'completed'
// COLLECT city = o.city AGGREGATE total_sales = SUM(o.amount)
// SORT total_sales DESC
// RETURN { city, total_sales }
```

---

### VectorSearchTemplate

```go
func VectorSearchTemplate(
    collection, variable string,
    similarityExpr string,   // e.g. "SIMILARITY(doc.embedding, @qv, 10)"
    returnExpr string,
) string
```

**Example:**

```go
q := VectorSearchTemplate("documents", "doc", "SIMILARITY(doc.embedding, @queryVector, 10)", "doc")
// FOR doc IN documents
// FILTER SIMILARITY(doc.embedding, @queryVector, 10)
// RETURN doc
```

---

### DML Templates

#### InsertTemplate

```go
func InsertTemplate(docExpr, collection string) string
```

```go
InsertTemplate("{ name: @name, age: @age }", "users")
// INSERT { name: @name, age: @age } INTO users
```

#### UpdateTemplate

```go
func UpdateTemplate(keyExpr, collection, updateExpr string) string
```

```go
UpdateTemplate("@key", "users", "{ status: @status }")
// UPDATE @key WITH { status: @status } IN users
```

#### ReplaceTemplate

```go
func ReplaceTemplate(keyExpr, collection, docExpr string) string
```

```go
ReplaceTemplate("@key", "users", "{ name: @name }")
// REPLACE @key WITH { name: @name } IN users
```

#### DeleteTemplate

```go
func DeleteTemplate(keyExpr, collection string) string
```

```go
DeleteTemplate("{ _key: @key }", "users")
// REMOVE { _key: @key } IN users
```

#### UpsertTemplate

```go
func UpsertTemplate(searchExpr, insertExpr, updateExpr, collection string) string
```

```go
UpsertTemplate(
    "{ email: @email }",
    "{ email: @email, name: @name, created: DATE_NOW() }",
    "{ name: @name }",
    "users",
)
// UPSERT { email: @email }
// INSERT { email: @email, name: @name, created: DATE_NOW() }
// UPDATE { name: @name }
// IN users
```

---

### PaginatedQueryTemplate

```go
func PaginatedQueryTemplate(
    collection, variable string,
    filterCondition string,
    sortField string,
    sortDir SortDirection,
    offset, pageSize int,
) string
```

**Example:**

```go
// Page 3 with 10 items per page
q := PaginatedQueryTemplate("products", "p", "p.active == true", "p.name", SortAsc, 20, 10)
// FOR p IN products
// FILTER p.active == true
// SORT p.name ASC
// LIMIT 20, 10
// RETURN p
```

---

### LLM Templates

#### LLMInferTemplate

```go
func LLMInferTemplate(prompt, model, options string) string
```

```go
LLMInferTemplate("Explain databases", "llama-2-7b", "{ max_tokens: 200, temperature: 0.7 }")
// LLM INFER "Explain databases"
//   USING MODEL "llama-2-7b"
//   OPTIONS { max_tokens: 200, temperature: 0.7 }
```

Pass empty strings for `model` or `options` to omit those clauses.

#### LLMRagTemplate

```go
func LLMRagTemplate(prompt, collection string, topN int, loraAdapter string) string
```

```go
LLMRagTemplate("What are key features?", "documents", 5, "technical-docs")
// LLM RAG "What are key features?"
//   FROM COLLECTION documents
//   TOP 5
//   USING LORA "technical-docs"
```

#### LLMEmbedTemplate

```go
func LLMEmbedTemplate(text, model string) string
```

```go
LLMEmbedTemplate("ThemisDB is a multi-model database", "all-minilm")
// LLM EMBED "ThemisDB is a multi-model database"
//   USING MODEL "all-minilm"
```

---

## Constants

### SortDirection

| Constant | Value | Description |
|----------|-------|-------------|
| `SortAsc` | `"ASC"` | Ascending sort order |
| `SortDesc` | `"DESC"` | Descending sort order |

### TraversalDirection

| Constant | Value | Description |
|----------|-------|-------------|
| `TraversalOutbound` | `"OUTBOUND"` | Follow outgoing edges |
| `TraversalInbound` | `"INBOUND"` | Follow incoming edges |
| `TraversalAny` | `"ANY"` | Follow edges in either direction |

---

## Best Practices

### Use bind parameters for user-supplied values

Template functions generate the structural AQL; always pass user data as bind parameters
(`@paramName`) rather than embedding them directly in the expression strings:

```go
// ✅ GOOD – value comes from a bind parameter
q := SimpleQueryTemplate("users", "u", "u.email == @email", "u.name", SortAsc, 1)
client.Query(ctx, q, &result) // pass { "@email": userInput } as bind vars

// ❌ BAD – injection risk
email := getUserInput()
q := SimpleQueryTemplate("users", "u", "u.email == '"+email+"'", "u.name", SortAsc, 1)
```

### Prefer the builder for complex, dynamic queries

When the set of filters is determined at runtime, use the fluent builder to avoid
fragile string concatenation:

```go
b := themisdb.NewAQLQuery().For("p", "products")
if req.Category != "" {
    b.Filter("p.category == @category")
}
if req.MinPrice > 0 {
    b.Filter("p.price >= @minPrice")
}
b.Sort("p.name", themisdb.SortAsc).Limit(0, req.PageSize).Return("p")
query := b.Build()
```

### Re-use templates, not built strings

Template functions are cheap to call. Build fresh strings for each request rather than
caching the query string with embedded values.

---

## See Also

- [AQL Syntax Reference](../../de/aql/aql_syntax.md)
- [AQL Functions Reference](../../de/aql/aql_functions_reference.md)
- [Hybrid Queries Guide](../../de/aql/aql_hybrid_queries.md)
- [Go Client README](../../../clients/go/README.md)
- [Source: `aql_templates.go`](../../../clients/go/aql_templates.go)
- [Tests: `aql_templates_test.go`](../../../clients/go/aql_templates_test.go)

---

**Version:** 1.4.0 | **License:** Apache 2.0 | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
