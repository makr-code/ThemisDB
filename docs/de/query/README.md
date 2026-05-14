# Query Module

**Stand:** 6. April 2026
**Version:** 1.9.0
**Kategorie:** Query
**Validated:** 2026-04-06
**Status:** current

---

## Übersicht

Das Query-Modul implementiert den vollständigen AQL-Abfrage-Stack von ThemisDB: Parser,
Optimizer, Execution-Engine und Caching-Infrastruktur. AQL unterstützt relationale,
Dokument-, Graph-, Vektor-, Geo- und Zeitreihen-Modelle sowie SQL- und SPARQL-Kompatibilität.

- **Primäre Dokumentation:** [Query-Modul README](../../../src/query/README.md)
- **Architektur:** [Query-Modul Architektur](../../../src/query/ARCHITECTURE.md)
- **Roadmap:** [Query-Modul Roadmap](../../../src/query/ROADMAP.md)
- **Geplante Erweiterungen:** [Query Future Enhancements](../../../src/query/FUTURE_ENHANCEMENTS.md)

---

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AQLParser | `aql_parser.h` | `aql_parser.cpp` | AQL → AST |
| AQLParser (JSON) | *(aql_parser.h)* | `aql_parser_json.cpp` | JSON-Query → AST |
| AQLTranslator | `aql_translator.h` | `aql_translator.cpp` | AST → interne Query-Repr. |
| SQLParser | `sql_parser.h` | `sql_parser.cpp` | SQL SELECT/INSERT/UPDATE/DELETE → AQL |
| SPARQLParser | `sparql_parser.h` | `sparql_parser.cpp` | SPARQL → AQL (RDF/Knowledge-Graph) |
| QueryOptimizer | `query_optimizer.h` | `query_optimizer.cpp` | Kosten-basierte Planoptimierung |
| AdaptiveOptimizer | `adaptive_optimizer.h` | `adaptive_optimizer.cpp` | Lernende Kostenmodell-Anpassung |
| OptimizerCostModel | `optimizer_cost_model.h` | `optimizer_cost_model.cpp` | Kardinalitäts- und Selektivitätsschätzung |
| RuntimeReoptimizer | `runtime_reoptimizer.h` | `runtime_reoptimizer.cpp` | Laufzeit-Reoptimierung aus Statistiken |
| QueryEngine | `query_engine.h` | `query_engine.cpp` | Physische Ausführung (Scan, Filter, Sort, Join) |
| AQLRunner | `aql_runner.h` | `aql_runner.cpp` | Top-Level-Dispatch (AQL, SQL, RLS, Limits, Abbruch) |
| QueryCanceller | `query_canceller.h` | `query_canceller.cpp` | Kooperativer Query-Abbruch via Request-ID |
| QueryResourceLimits | `query_resource_limits.h` | *(header-only)* | Max-Rows/Memory/Timeout-Guard |
| QueryPlanVisualizer | `query_plan_visualizer.h` | `query_plan_visualizer.cpp` | EXPLAIN / EXPLAIN ANALYZE |
| VectorizedExecution | `vectorized_execution.h` | `vectorized_execution.cpp` | Column-Store-Batch-Verarbeitung (SIMD) |
| ParallelScan | `parallel_scan.h` | *(header-only)* | Paralleler Collection-Scan |
| CrossClusterFederator | `cross_cluster_federation.h` | `cross_cluster_federation.cpp` | Verteilte Query-Federation über Cluster-Endpoints |
| QueryFederation | `query_federation.h` | `query_federation.cpp` | Interne Shard-Level-Federation |
| ShardingManager | `sharding_manager.h` | `sharding_manager_edition.cpp` | Consistent-Hash-Ring für Shard-Key-Routing (GetShardForKey, GetShardsForKeyRange) |
| QueryCache | `query_cache.h` | `query_cache.cpp` | Exakter Query-Result-Cache |
| QueryCacheManager | `query_cache_manager.h` | `query_cache_manager.cpp` | Cache-Verwaltung und -Metriken |
| SemanticCache | `semantic_cache.h` | `semantic_cache.cpp` | Embedding-basierter Ähnlichkeits-Cache |
| WorkloadCacheStrategy | `workload_cache_strategy.h` | `workload_cache_strategy.cpp` | Adaptive Cache-Eviction |
| CTECache | `cte_cache.h` | `cte_cache.cpp` | CTE-Zwischenergebnis-Cache |
| CTESubquery | `cte_subquery.h` | `cte_subquery.cpp` | CTE-Auswertung |
| MaterializedCTE | `materialized_cte.h` | `materialized_cte.cpp` | Inkrementelle CTE-Materialisierung |
| LetEvaluator | `let_evaluator.h` | `let_evaluator.cpp` | LET-Variable-Auswertung |
| WindowEvaluator | `window_evaluator.h` | `window_evaluator.cpp` | Window-Funktionen (RANK, LAG, LEAD, …) |
| StatisticalAggregator | `statistical_aggregator.h` | `statistical_aggregator.cpp` | Statistische Aggregationen |
| ResultStream | `result_stream.h` | `result_stream.cpp` | Ergebnis-Streaming & Pagination |
| ResultTypeAnnotation | `result_type_annotation.h` | `result_type_annotation.cpp` | Typ-Inferenz für SDK-Code-Generierung |
| SubqueryOptimizer | `subquery_optimizer.h` | *(im query_optimizer)* | Subquery-Optimierung (header-only) |
| FunctionRegistry | `functions/function_registry.h` | `functions/function_registry.cpp` | 100+ AQL-Funktionen (25+ Kategorien) |
| UDFRegistry | `functions/udf_registry.h` | `functions/udf_registry.cpp` | User-Defined Functions (C++) |

**Gesamt:** 30 Header, 26 Source-Dateien in `src/query/` + Header-only-Komponenten

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

## Query-Typen

### AQL-Abfragen
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

### Graph-Traversal
```sql
FOR v, e, p IN 1..3 OUTBOUND @start GRAPH 'social'
  FILTER v.active == true
  RETURN {vertex: v, path: p}
```

### SQL-Kompatibilität (Passthrough)
```sql
SELECT name, age FROM users WHERE age > 18 ORDER BY name LIMIT 10
```
Wird intern zu AQL transpiliert via `sql_parser.cpp`.

### Query-Abbruch via Request-ID
```cpp
// In einem HTTP-Worker-Thread ausführen:
auto result = executeAqlCancellable(aql, engine, "request-42");

// Aus einem anderen Thread abbrechen (z.B. HTTP-Cancel-Handler):
QueryCanceller::instance().cancel("request-42");
```

## Wichtige Schnittstellen

```cpp
// Einfache AQL-Ausführung
Result<nlohmann::json> executeAql(const std::string& aql, QueryEngine& engine);

// Mit Ressourcen-Limits (max_rows, max_memory_bytes, timeout_ms)
Result<nlohmann::json> executeAqlWithLimits(
    const std::string& aql, QueryEngine& engine,
    const QueryResourceLimits& limits);

// Mit kooperativem Abbruch
Result<nlohmann::json> executeAqlCancellable(
    const std::string& aql, QueryEngine& engine,
    const std::string& request_id,
    QueryCanceller& canceller = QueryCanceller::instance());

// SQL-Passthrough
Result<nlohmann::json> executeSQL(const std::string& sql, QueryEngine& engine);

// EXPLAIN / EXPLAIN ANALYZE
Result<nlohmann::json> explainAql(const std::string& aql, QueryEngine& engine,
                                   bool analyze = false);
Result<std::string>    explainAqlText(const std::string& aql, QueryEngine& engine,
                                       bool analyze = false);
Result<std::string>    explainAqlDot(const std::string& aql, QueryEngine& engine);
```

## Usage

- **AQL ausführen:** `executeAql(...)` als Standard-Entry-Point für Query-Pipeline (Parse → Optimize → Execute)
- **Ressourcen begrenzen:** `executeAqlWithLimits(...)` für `max_rows`, `max_memory_bytes`, `timeout_ms`
- **Kooperativer Abbruch:** `executeAqlCancellable(...)` + `QueryCanceller::cancel(request_id)`
- **Fehleranalyse:** `explainAql(...)`, `explainAqlText(...)`, `explainAqlDot(...)` für Plan-Diagnostik

## Installation

Das Query-Modul wird als Teil von ThemisDB gebaut:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

Siehe [`SETUP.md`](../../../SETUP.md) für vollständige Abhängigkeiten und Build-Umgebung.

## Troubleshooting

- Parser-/Syntaxprobleme: [`../../troubleshooting/aql_troubleshooting.md`](../../troubleshooting/aql_troubleshooting.md)
- Laufzeit-, Timeout- und Federation-Probleme: [`../../troubleshooting/query_troubleshooting.md`](../../troubleshooting/query_troubleshooting.md)
- Hybrid-Query-spezifische Performance: [`query_hybrid_benchmarks.md`](query_hybrid_benchmarks.md)

## Verwandte Dokumentation

- [Primäre Quelldokumentation](../../../src/query/README.md) — vollständige API-Referenz
- [Architektur](../../../src/query/ARCHITECTURE.md) — Komponenten-Diagramm, Datenfluss
- [Roadmap](../../../src/query/ROADMAP.md) — Implementierungsstand und geplante Features
- [Geplante Erweiterungen](../../../src/query/FUTURE_ENHANCEMENTS.md) — geplante APIs, Grenzen, Backlog
- [AQL Syntax](../aql/aql_syntax.md) — AQL-Sprachreferenz
- [Hybrid Search](query_vector_hybrid.md) — Vector + Filter Hybridabfragen
- [Filtered Vector Queries](query_filtered_vector.md) — Gefilterte Vektorsuche
- [Hybrid Overview](query_hybrid_overview.md) — Übersicht Hybrid-Queries
- [Query Benchmarks](query_hybrid_benchmarks.md) — Performance-Messungen
- [Missing Implementations](MISSING_IMPLEMENTATIONS.md) — Bekannte Lücken und geplante Arbeiten

---

## Shard-Key-Routing (v1.9.0)

`QueryFederation` nutzt jetzt den `ShardingManager` für präzises Shard-Key-Routing:

- **Prädikat-Extraktion**: `._key==` / `._key>=` / `._key<=` Prädikate werden aus der AQL-Where-Klausel extrahiert
- **Routing-Strategien**: `PARTITION_PRUNING` (direktes Shard-Key-Routing) vs. `SCATTER_GATHER` (Broadcast bei fehlendem Schlüssel)
- **Warnung bei >10 Shards**: `SCATTER_GATHER`-Abfragen über mehr als 10 Shards erzeugen eine Warnung

**Konstruktoren:**
```cpp
QueryFederation(ShardRouter, ShardingManager&, Config)  // Shard-Key-Routing
QueryFederation(ShardRouter, Config)                     // Einfaches Routing
```
