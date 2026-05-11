# 🔍 AQL (Advanced Query Language) Module

**Category:** 🔍 Core AQL  
**Version:** v1.5.0  
**Status:** ✅ Production Ready  
**Datum:** 9. März 2026

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🤖 LLM-AQL Komponenten](#-llm-aql-komponenten)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB implementiert eine erweiterte Version von **AQL (Advanced Query Language)** – eine deklarative, SQL-ähnliche Abfragesprache mit zusätzlichen Features für Multi-Model-Queries über relationale Daten, Graphen, Vektoren und Dokumente.

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

## ✨ Features & Highlights

### 🎯 Kern-Features

- **Multi-Model-Support:** Relationale, Graph-, Vektor- und Dokumentenabfragen in einer Sprache
- **SQL-ähnliche Syntax:** Schnelle Adoption durch bekannte FOR-FILTER-SORT-Struktur
- **Automatische Optimierung:** Intelligente Index-Auswahl via Query Optimizer
- **Graph-Traversierung:** BFS mit OUTBOUND/INBOUND/ANY und SHORTEST_PATH
- **Vektor-Suche:** Native SIMILARITY() mit HNSW-Integration
- **Subqueries & CTEs:** WITH-Klauseln und verschachtelte Abfragen
- **Window Functions:** ROW_NUMBER, RANK, LAG, LEAD, etc.
- **Volltext-Suche:** BM25-basiertes FULLTEXT() mit Stemming

---

## 🤖 LLM-AQL Komponenten

Neben der Kernsprache enthält das AQL-Modul eine KI-gestützte Hilfsschicht für
LLM-unterstützte Abfrageautoren und Entwickler-Tooling. Diese Komponenten leben
in `src/aql/` (Implementierung) und `include/aql/` (Schnittstellen).

> 🔗 **Primäre Dokumentation:** [src/aql/README.md](../../../src/aql/README.md) ·
> [include/aql/README.md](../../../include/aql/README.md)

| Komponente | Datei | Beschreibung |
|---|---|---|
| `LlmAqlHandler` | `llm_aql_handler.cpp` | LLM-Befehle: INFER, RAG, EMBED, MODEL, LORA |
| `AQLQueryBuilder` | `aql_query_builder.cpp` | Schema-bewusste, programmatische AQL-Konstruktion |
| `AQLQueryValidator` | `aql_query_validator.cpp` | Strukturelle Validierung & Linting generierter Queries |
| `AQLSyntaxHighlighter` | `aql_syntax_highlighter.cpp` | ANSI-Farb-Highlighting und Fehlermarkierung |
| `AQLConfidenceScorer` | `aql_confidence_scorer.cpp` | Konfidenz-Score für LLM-generierte Queries (0.0–1.0) |
| `AQLAutocomplete` | `aql_autocomplete.cpp` | Token-Level-Autocompletion (LSP-kompatibel) |
| `AQLFewShotExampleLibrary` | `aql_fewshot_example_library.cpp` | Beispiel-Bibliothek für verbesserte NL→AQL-Genauigkeit |
| `AQLConversationContext` | `aql_conversation_context.cpp` | Multi-Turn-Gesprächshistorie mit Context-Window-Budget |
| `AQLOptimizerAdvisor` | `aql_optimizer_advisor.cpp` | Query-Plan-Erklärung & Optimierungshinweise |
| `AQLQueryTemplateLibrary` | `aql_query_template_library.cpp` | Vorvalidierte Abfrage-Templates |
| `AQLLoraFinetuner` | `aql_lora_finetuner.cpp` | LoRA-Adapter-Fine-Tuning auf AQL-Korpora |
| `AQLMigrationAssistant` | `aql_migration_assistant.cpp` | Migration Legacy-AQL (ArangoDB → ThemisDB) |
| `LLMMetricsCollector` | `llm_metrics_collector.cpp` | Latenz, Token-Zähler, Cache-Hit-Metriken |
| `ReActAgent` | `aql_agent.cpp` | Mehrstufiger Reasoning-Agent mit Tool-Calling (Phase 4) |
| `AQLTokenStream` | *(header-only)* | Thread-sicherer Streaming-Token-Iterator (Phase 4) |
| `IAsyncLLMBackend` | *(header-only)* | Nicht-blockierendes Async-Backend-Interface (Phase 4) |
| `MultiModalInferRequest` | *(header-only)* | Bild/Audio/Video-Inferenz-Request (Phase 4) |
| `AQLQueryDiffExplainer` | `aql_query_diff_explainer.cpp` | Klausel-Level-Diff zwischen zwei AQL-Queries (regelbasiert) |
| `AQLRollbackSuggester` | `aql_rollback_suggester.cpp` | Kompensierender Rollback-Query für Mutationsstatements |
| `AQLIngestionBridge` | `aql_ingestion_bridge.cpp` | INSERT/UPSERT-Payload-Anreicherung via Ingestion-Pipeline |
| `AQLModelRouter` | `aql_model_router.cpp` | Query-Typ-basiertes LLM-Backend-Routing |
| `IClassifyFn` / `ClassifyBridge` | `classify_bridge.cpp` | Zero-Shot-Textklassifikations-Interface |
| `LLMAQLEmbeddingBridge` | `llm_aql_embedding_bridge.cpp` | Embedding-Bridge für semantisches Few-Shot-Ranking |

### Schnelles Beispiel

```aql
-- Natürlichsprachliche Abfrage → AQL (via LLM)
LLM INFER 'Alle Nutzer, die sich letzten Monat angemeldet haben'
  MODEL 'llama-3-8b'
-- Ergebnis wird intern durch translateNLToAQL() in gültiges AQL übersetzt

-- RAG-Abfrage
LLM RAG 'Was sind die Vorteile von Vektordatenbanken?'
  SEARCH IN documentation
  TOP 5
  MODEL 'llama-3-8b'

-- Streaming-Erklärung (SSE)
-- POST /api/v1/llm/aql/explain/stream
-- Body: {"aql": "FOR u IN users FILTER u.age > 30 RETURN u"}
```

### Status (v1.5.0)

✅ **Stabil:** NL→AQL-Übersetzung, Konfidenz-Scoring, SSE-Streaming, Autocompletion,
LoRA-Adapter, Konversationskontext, Query-Validierung, Few-Shot-Bibliothek,
ReActAgent, AQLTokenStream, IAsyncLLMBackend, AQLQueryDiffExplainer,
AQLRollbackSuggester, AQLIngestionBridge, AQLModelRouter  
🔬 **Experimentell:** Multi-Modal (Bilder, Audio), Fine-Tuning-Pipeline, verteilte LLM-Inferenz

---

## 🚀 Schnellstart

### Basis-Query

```aql
FOR doc IN users
  FILTER doc.age > 18
  SORT doc.name ASC
  LIMIT 0, 10
  RETURN doc
```

### Mit LET-Bindings

```aql
FOR doc IN products
  LET discount = doc.price * 0.1
  LET finalPrice = doc.price - discount
  RETURN {name: doc.name, price: finalPrice}
```

### Graph-Traversierung

```aql
FOR v, e, p IN 1..3 OUTBOUND @start GRAPH 'social'
  FILTER v.active == true
  RETURN {vertex: v, path: p}
```

### Vektor-Suche mit Filtern

```aql
FOR doc IN products
  FILTER doc.category == "electronics"
  LET score = SIMILARITY(doc.embedding, @queryVector, 10)
  SORT score DESC
  RETURN {doc, score}
```

---

## 📖 Detaillierte Dokumentation

### 📚 Source-Code Komponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AQLParser | `aql_parser.h` | `aql_parser.cpp` | Parser & AST |
| AQLTranslator | `aql_translator.h` | `aql_translator.cpp` | AST → Execution Plan |
| AQLRunner | `aql_runner.h` | `aql_runner.cpp` | Query Execution |
| QueryOptimizer | `query_optimizer.h` | `query_optimizer.cpp` | Plan Optimization |
| LetEvaluator | `let_evaluator.h` | `let_evaluator.cpp` | LET Bindings |
| CTECache | `cte_cache.h` | `cte_cache.cpp` | WITH Clauses |
| WindowEvaluator | `window_evaluator.h` | `window_evaluator.cpp` | Window Functions |

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

### ✅ DO: Index-Nutzung optimieren

```aql
-- ✅ GUT: Nutzt Index auf 'city'
FOR u IN users
  FILTER u.city == "Berlin"
  RETURN u

-- ❌ SCHLECHT: Keine Index-Nutzung möglich
FOR u IN users
  FILTER LOWER(u.city) == "berlin"
  RETURN u
```

### ✅ DO: Früh filtern

```aql
-- ✅ GUT: Filter früh anwenden
FOR u IN users
  FILTER u.active == true
  FOR o IN orders
    FILTER o.user_id == u._key
    RETURN o

-- ❌ SCHLECHT: Kartesisches Produkt dann Filter
FOR u IN users
  FOR o IN orders
    FILTER u.active == true AND o.user_id == u._key
    RETURN o
```

### ✅ DO: CTEs für wiederverwendete Subqueries

```aql
-- ✅ GUT: CTE materialisiert einmal
WITH activeUsers AS (
  FOR u IN users FILTER u.active == true RETURN u
)
FOR u IN activeUsers
  RETURN u.name
```

### ⚠️ VORSICHT: Nested-Loop-Performance

```aql
-- Kann teuer sein bei großen Collections
FOR u IN users
  FOR o IN orders
    FILTER o.user_id == u._key  -- O(n*m) ohne Hash-Join
    RETURN {user: u, order: o}
```

---

## 🔧 Troubleshooting

### Query läuft zu langsam

**Problem:** Query dauert mehrere Sekunden

**Lösung:**
1. Nutze `explain: true` zur Plananalyse
2. Prüfe Index-Nutzung mit `plan.mode`
3. Erstelle fehlende Indizes auf Filter-Spalten
4. Vermeide Funktionsaufrufe in FILTER (wenn möglich)

### Out of Memory bei großen CTEs

**Problem:** `QueryEngine::executeCTEs failed: out of memory`

**Lösung:**
- CTECache hat automatisches Spill-to-Disk (100MB Default)
- Reduziere CTE-Größe durch frühes Filtern
- Prüfe ob CTE mehrfach verwendet wird (Materialisierung lohnt sich)

### Unerwartete Ergebnisse bei Graph-Traversierung

**Problem:** Zu viele/wenige Vertices zurückgegeben

**Lösung:**
1. Prüfe Depth-Range: `1..3` vs `1..6`
2. Nutze FILTER auf Vertex/Edge-Properties
3. Teste mit `SHORTEST_PATH` für exakte Pfade
4. Aktiviere Tracing für `edges_expanded` Metrik

### LLM-Übersetzung liefert ungültiges AQL

**Problem:** `translateNLToAQL()` wirft `LLMException(INVALID_RESPONSE)` oder die Query schlägt bei der Ausführung fehl.

**Lösung:**
1. `schema_context` mit Collection-/Feldnamen angeben für bessere Genauigkeit.
2. Code-Generierungsmodell verwenden (z.B. DeepSeek-Coder, Codestral).
3. Validierungsmodus `RETRY_ON_ERROR` aktivieren: Handler sendet Fehlerannotation als Feedback an LLM.
4. Bei `PROMPT_INJECTION`-Fehler: NL-Eingabe vor Übergabe bereinigen.

### LLM-Inferenz-Timeout

**Problem:** `LLMException(TIMEOUT)` bei `executeInfer()` oder `executeRAG()`.

**Lösung:**
- Timeout erhöhen: `handler.setTimeoutConfig({.infer_timeout = 600})`.
- `max_tokens` reduzieren.
- Kleineres/quantisiertes Modell (4-bit GGUF) laden.
- GPU-Offload aktivieren: Modell mit `GPU_LAYERS <n>` neuladen.

### Circuit Breaker offen

**Problem:** `LLMException(CIRCUIT_OPEN)` für einen Befehlstyp, andere funktionieren.

**Lösung:** Pro-Befehl-Circuit-Breaker (INFER, RAG, EMBED, FINETUNE) sind isoliert.
Status prüfen via `handler.getCircuitBreakerStates()` oder `LLM STATS`-Befehl.
Nach 60 Sekunden öffnet sich das Fenster automatisch wieder.

---

## 📚 Siehe auch

### 🔗 Primäre Dokumentation (Quelle der Details)

- [src/aql/README.md](../../../src/aql/README.md) — Implementierungs-Übersicht: LLM-Befehle, NL→AQL, Komponenten
- [src/aql/ARCHITECTURE.md](../../../src/aql/ARCHITECTURE.md) — Architektur: Komponenten-Diagramm, Datenfluss, Threading
- [src/aql/ROADMAP.md](../../../src/aql/ROADMAP.md) — Roadmap: abgeschlossene Features, geplante Phasen
- [src/aql/FUTURE_ENHANCEMENTS.md](../../../src/aql/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen (Multi-Modal, Agenten, Fine-Tuning)
- [include/aql/README.md](../../../include/aql/README.md) — Interface-Spezifikation: Header, API-Verträge

### 📘 Kern-Dokumentation

- [AQL Syntax Referenz](aql_syntax.md) - Vollständige Sprachdefinition
- [Query Engine Architektur](aql_query_engine.md) - Parser, Translator, Optimizer
- [Functions Reference](aql_functions_reference.md) - Alle 355+ Built-in Functions

### 🔎 Erweiterte Features

- **[🎯 AQL Phases 1-3 Consolidated Guide](AQL_PHASES_1_2_3_CONSOLIDATED.md)** - Komplette Dokumentation aller Hybrid Query Features, Syntax Sugar und Subqueries
- [Hybrid Queries Guide](aql_hybrid_queries.md) - Vector+Geo, Content+Geo (Phase 2)
- [Hybrid Queries Phase 1.5](aql_hybrid_queries_phase15.md) - Performance Optimizations
- [Subquery & CTE Reference](aql_subquery_reference.md) - WITH, Scalar Subqueries (Phase 3)
- [Subquery Implementation](aql_subquery_implementation.md) - Implementation Details
- [Pattern Matching](aql_pattern_matching.md) - Graph-Pattern ohne neue Syntax
- [EXPLAIN & PROFILE](aql_explain_profile.md) - Performance-Analyse

### ⚙️ Operationen & Tooling

- [Prompt Engineering Guide](aql_prompt_engineering.md) - LLM-Integration
- [Language Scope Analysis](aql_language_scope.md) - Feature-Vergleich mit anderen DBs
- [Implementation Status](IMPLEMENTATION_STATUS_ANALYSIS.md) - Was ist implementiert?

### 🌐 Externe Referenzen

- [ArangoDB AQL Documentation](https://www.arangodb.com/docs/stable/aql/) - Original-Inspiration
- [Neo4j Cypher](https://neo4j.com/docs/cypher-manual/current/) - Graph Query Language
- [PostgreSQL](https://www.postgresql.org/docs/current/queries.html) - SQL Reference

---

## 📝 Changelog

### v1.5.0 - 9. März 2026
- ✅ **LLM-AQL-Abschnitt:** Phase 4/5 Komponenten ergänzt (ReActAgent, AQLTokenStream, IAsyncLLMBackend, AQLQueryDiffExplainer, AQLRollbackSuggester, AQLIngestionBridge, AQLModelRouter, LLMAQLEmbeddingBridge, ClassifyBridge)
- ✅ **Status-Update:** Stable-Liste aktualisiert; Agenten-Framework aus Experimentell nach Stabil verschoben
- ✅ **Troubleshooting:** LLM-spezifische Problemlösungen ergänzt (Timeout, Circuit Breaker, ungültiges AQL)
- ✅ **Primär-Links:** Verlinkung auf `src/aql/README.md` und `include/aql/README.md`

### v1.3.0 - 22. Dezember 2025
- ✅ **Template-Update:** Standardisierung auf v1.3.0 Dokumentationsformat
- ✅ **Struktur:** 8-Abschnitte-Format mit Emojis und TOC
- ✅ **Navigation:** Verbesserte interne Verlinkungen zu allen AQL-Dokumenten
- ✅ **Kategorisierung:** Klare Trennung Core/Advanced/Reference/Operations

### v1.0.0 - 5. Dezember 2025
- Initial Release mit vollständiger Multi-Model-Unterstützung
- Parser, Translator, Query Engine komplett
- 355+ Built-in Functions implementiert
- Subqueries, CTEs, Window Functions verfügbar
