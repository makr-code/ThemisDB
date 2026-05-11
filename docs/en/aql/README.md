# 🔍 AQL (Advanced Query Language) Module

**Category:** 🔍 Core AQL  
**Version:** v1.5.0  
**Status:** ✅ Production Ready  
**Date:** May 2026

---

## 📑 Table of Contents

- [📋 Overview](#-overview)
- [✨ Features & Highlights](#-features--highlights)
- [🤖 LLM-AQL Components](#-llm-aql-components)
- [🚀 Quick Start](#-quick-start)
- [📖 Detailed Documentation](#-detailed-documentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 See Also](#-see-also)
- [📝 Changelog](#-changelog)

---

## 📋 Overview

ThemisDB implements an extended version of **AQL (Advanced Query Language)** — a declarative,
SQL-like query language with additional features for multi-model queries over relational data,
graphs, vectors, and documents.

### Full AQL Documentation (v1.5)

ThemisDB v1.5 provides comprehensive AQL documentation with performance optimisation and best
practices:

- **[AQL Syntax Guide](../../de/aql/AQL_SYNTAX_GUIDE.md)** - Complete language reference
- **[AQL Best Practices](../../de/aql/AQL_BEST_PRACTICES.md)** - Query structure, index usage, performance, security
- **[AQL Performance Guide](../../de/aql/AQL_PERFORMANCE_GUIDE.md)** - Query Optimizer internals, index strategies, profiling
- **[AQL Query Templates](aql_query_templates.md)** - Fluent builder and pre-built templates (Go client)
- **[AQL Functions Reference](aql_functions_reference.md)** - Auto-generated reference for all 346+ built-in functions

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

---

## 🤖 LLM-AQL Components

In addition to the core language, the AQL module includes an AI-assisted layer for LLM-supported
query authoring and developer tooling. These components live in `src/aql/` (implementation)
and `include/aql/` (interfaces).

> 🔗 **Primary documentation:** [src/aql/README.md](../../../src/aql/README.md) ·
> [include/aql/README.md](../../../include/aql/README.md) ·
> [src/aql/ROADMAP.md](../../../src/aql/ROADMAP.md)

| Component | File | Description |
|---|---|---|
| `LlmAqlHandler` | `llm_aql_handler.cpp` | LLM commands: INFER, RAG, EMBED, MODEL, LORA |
| `AQLQueryBuilder` | `aql_query_builder.cpp` | Schema-aware, programmatic AQL construction |
| `AQLQueryValidator` | `aql_query_validator.cpp` | Structural validation and linting of generated queries |
| `AQLSyntaxHighlighter` | `aql_syntax_highlighter.cpp` | ANSI colour highlighting and error annotation |
| `AQLConfidenceScorer` | `aql_confidence_scorer.cpp` | Confidence score for LLM-generated queries (0.0–1.0) |
| `AQLAutocomplete` | `aql_autocomplete.cpp` | Token-level autocompletion (LSP-compatible) |
| `AQLFewShotExampleLibrary` | `aql_fewshot_example_library.cpp` | Example library for improved NL→AQL accuracy |
| `AQLConversationContext` | `aql_conversation_context.cpp` | Multi-turn conversation history with bounded context window |
| `AQLOptimizerAdvisor` | `aql_optimizer_advisor.cpp` | Query-plan explanation and optimisation hints |
| `AQLQueryTemplateLibrary` | `aql_query_template_library.cpp` | Pre-validated query templates |
| `AQLLoRAFinetuner` | `aql_lora_finetuner.cpp` | LoRA adapter fine-tuning on AQL corpora |
| `AQLMigrationAssistant` | `aql_migration_assistant.cpp` | Legacy AQL migration (ArangoDB → ThemisDB) |
| `LLMMetricsCollector` | `llm_metrics_collector.cpp` | Latency, token counts, cache-hit metrics |
| `ReActAgent` | `aql_agent.cpp` | Multi-step reasoning agent with tool calling (Phase 4) |
| `AQLTokenStream` | *(header-only)* | Thread-safe streaming token iterator (Phase 4) |
| `IAsyncLLMBackend` | *(header-only)* | Non-blocking async backend interface (Phase 4) |
| `MultiModalInferRequest` | *(header-only)* | Image/audio/video inference request (Phase 4) |
| `AQLQueryDiffExplainer` | `aql_query_diff_explainer.cpp` | Clause-level structural diff (rule-based) |
| `AQLRollbackSuggester` | `aql_rollback_suggester.cpp` | Compensating rollback query generation (rule-based) |
| `AQLIngestionBridge` | `aql_ingestion_bridge.cpp` | INSERT/UPSERT payload enrichment via ingestion pipeline |
| `AQLModelRouter` | `aql_model_router.cpp` | Query-type based LLM backend routing |
| `IClassifyFn` / `ClassifyBridge` | `classify_bridge.cpp` | Zero-shot text classification interface |
| `LLMAQLEmbeddingBridge` | `llm_aql_embedding_bridge.cpp` | Embedding bridge for semantic few-shot ranking |

### Quick Example

```aql
-- Natural-language query → AQL (via LLM)
LLM INFER 'All users who signed up last month'
  MODEL 'llama-3-8b'
-- The result is internally translated to valid AQL via translateNLToAQL()

-- RAG query
LLM RAG 'What are the advantages of vector databases?'
  SEARCH IN documentation
  TOP 5
  MODEL 'llama-3-8b'

-- Streaming explanation (SSE)
-- POST /api/v1/llm/aql/explain/stream
-- Body: {"aql": "FOR u IN users FILTER u.age > 30 RETURN u"}
```

### Status (v1.5.0)

✅ **Stable:** NL→AQL translation, confidence scoring, SSE streaming, autocompletion,
LoRA adapters, conversation context, query validation, few-shot library, ReActAgent,
AQLTokenStream, IAsyncLLMBackend, AQLQueryDiffExplainer, AQLRollbackSuggester
🔬 **Experimental:** Multi-modal pipeline (images, audio), fine-tuning integration,
distributed LLM inference

---

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

### LLM Translation Returns Invalid AQL

**Problem:** `translateNLToAQL()` throws `LLMException(INVALID_RESPONSE)` or the returned
query fails on execution.

**Solution:**
1. Provide a `schema_context` string with collection/field names for more accurate translation.
2. Switch to a code-generation model (e.g. DeepSeek-Coder, Codestral) for better AQL output.
3. Use `RETRY_ON_ERROR` validation mode so the handler re-submits with error feedback.
4. If `PROMPT_INJECTION` is thrown, sanitise the NL input before passing it.

### LLM Inference Times Out

**Problem:** `LLMException(TIMEOUT)` from `executeInfer()` or `executeRAG()`.

**Solution:**
- Increase timeouts: `handler.setTimeoutConfig({.infer_timeout = 600})`.
- Reduce `max_tokens` in inference options.
- Load a smaller or quantised (4-bit GGUF) model.
- Enable GPU offload by reloading the model with `GPU_LAYERS <n>`.

### Circuit Breaker Open

**Problem:** `LLMException(CIRCUIT_OPEN)` for one command type while others work.

**Solution:** Per-command circuit breakers (INFER, RAG, EMBED, FINETUNE) are isolated.
Inspect state via `handler.getCircuitBreakerStates()` or the `LLM STATS` command.
Wait for the 60-second window to expire or reset programmatically.

---

## 📚 See Also

### 🔗 Primary Source Documentation

- [src/aql/README.md](../../../src/aql/README.md) — implementation overview: LLM commands, NL→AQL, components, troubleshooting
- [include/aql/README.md](../../../include/aql/README.md) — interface specification: headers, API contracts
- [src/aql/ROADMAP.md](../../../src/aql/ROADMAP.md) — roadmap: completed features, planned phases
- [src/aql/FUTURE_ENHANCEMENTS.md](../../../src/aql/FUTURE_ENHANCEMENTS.md) — planned improvements with interface specs
- [src/aql/ARCHITECTURE.md](../../../src/aql/ARCHITECTURE.md) — architecture: component diagram, data-flow, threading

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

### v1.5.0 - May 2026
- ✅ **LLM-AQL Components Section:** Added overview of all AI-assisted components
  (NL→AQL, SSE streaming, LoRA, ReActAgent, AQLTokenStream, IAsyncLLMBackend,
  AQLQueryDiffExplainer, AQLRollbackSuggester, AQLIngestionBridge, AQLModelRouter)
- ✅ **LLM Troubleshooting:** Added LLM-specific problem/solution entries
- ✅ **Cross-references:** Added links to `src/aql/README.md`, `include/aql/README.md`,
  `src/aql/ROADMAP.md`, `src/aql/FUTURE_ENHANCEMENTS.md`, `src/aql/ARCHITECTURE.md`
- ✅ **Version bump:** Updated to v1.5.0 Production Ready

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

**Version:** 1.5.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
