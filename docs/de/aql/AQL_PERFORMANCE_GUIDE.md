# AQL Performance Optimization Guide - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Query Optimizer](#query-optimizer)
- [Index-Strategien](#index-strategien)
- [Execution Plans](#execution-plans)
- [Performance-Metriken](#performance-metriken)
- [Optimierungs-Patterns](#optimierungs-patterns)
- [Common Bottlenecks](#common-bottlenecks)
- [Monitoring und Profiling](#monitoring-und-profiling)

---

## Übersicht

ThemisDB verwendet einen Cost-Based Query Optimizer (CBO), der Execution Plans basierend auf Statistiken und geschätzten Kosten erstellt. Dieser Guide zeigt, wie man den Optimizer optimal nutzt.

---

## Query Optimizer

### Optimizer Phases

Der Query Optimizer durchläuft mehrere Phasen:

1. **Parse Phase**: AQL → Abstract Syntax Tree (AST)
2. **Validation Phase**: Syntax und semantische Validierung
3. **Optimization Phase**:
   - Index Selection
   - Join Order Optimization
   - Filter Pushdown
   - Subquery Optimization
   - Constant Folding
4. **Execution Phase**: Plan wird ausgeführt

### Optimizer Rules

```cpp
// Aktivierte Optimizer Rules
enum class OptimizerRule {
    MOVE_FILTERS_UP,              // Filter früh ausführen
    REMOVE_REDUNDANT_FILTERS,     // Redundante Filter entfernen
    REMOVE_UNNECESSARY_CALCULATIONS, // Unnötige Berechnungen entfernen
    USE_INDEXES,                  // Index-Nutzung
    REMOVE_COLLECT_VARIABLES,     // Nicht verwendete COLLECT-Vars
    PROPAGATE_CONSTANT_ATTRIBUTES,// Konstanten-Propagierung
    MOVE_CALCULATIONS_UP,         // Berechnungen früh ausführen
    INLINE_SUBQUERIES,            // Subqueries inlinen
    REMOVE_REDUNDANT_SORTS,       // Redundante SORTs entfernen
    INTERCHANGE_ADJACENT_ENUMERATIONS // FOR-Reihenfolge optimieren
};
```

### Optimizer Statistics

```aql
// Statistics anzeigen
FOR doc IN users
  OPTIONS {
    optimizer: {
      rules: ["all"],
      verbose: true
    }
  }
  FILTER doc.age > 25
  RETURN doc
```

**Output:**
```json
{
  "optimizer": {
    "rules": [
      "move-filters-up",
      "use-indexes"
    ],
    "stats": {
      "rulesExecuted": 2,
      "plansCreated": 3,
      "estimatedCost": 150
    }
  }
}
```

---

## Index-Strategien

### Index Types und Use Cases

| Index Type | Best For | Example |
|------------|----------|---------|
| **Hash** | Equality lookups | `email == "john@example.com"` |
| **BTree** | Range queries | `age > 25 AND age < 50` |
| **Fulltext** | Text search | `SEARCH content IN TOKENS("query")` |
| **Vector** | Similarity search | `VECTOR_DISTANCE(embedding, query)` |
| **Geo** | Geospatial queries | `GEO_DISTANCE(location, point) < 1000` |
| **Composite** | Multiple fields | `status == "active" AND age > 25` |

### Index Selectivity

**Hohe Selectivity (gut für Indexes):**
```aql
// Email (unique): ~1 Dokument pro Wert
CREATE INDEX idx_email ON users(email) HASH UNIQUE

FOR doc IN users
  FILTER doc.email == @email
  RETURN doc
// Estimated cost: 1-10
```

**Niedrige Selectivity (schlecht für Indexes):**
```aql
// Status (wenige Werte): Viele Dokumente pro Wert
CREATE INDEX idx_status ON users(status) HASH

FOR doc IN users
  FILTER doc.status == "active"  // 90% sind "active"
  RETURN doc
// Estimated cost: 900+ (bei 1000 Dokumenten)
```

### Composite Index Order

```aql
// ✅ Optimal: Hohe Selectivity zuerst
CREATE INDEX idx_email_status ON users(email, status)

// ❌ Suboptimal: Niedrige Selectivity zuerst
CREATE INDEX idx_status_email ON users(status, email)
```

**Reason:** Index kann nur von links nach rechts genutzt werden.

### Covering Indexes

```aql
// Covering Index: Alle Query-Felder im Index
CREATE INDEX idx_covering ON users(email, name, age)

// Query liest nur aus Index (kein Document Lookup)
FOR doc IN users
  FILTER doc.email == @email
  RETURN {email: doc.email, name: doc.name, age: doc.age}
```

**Vorteil:** Schneller, da kein Dokument-Zugriff nötig.

---

## Execution Plans

### EXPLAIN verwenden

```aql
EXPLAIN
FOR doc IN users
  FILTER doc.age > 25 AND doc.status == "active"
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
```

**Output:**
```json
{
  "plan": {
    "nodes": [
      {
        "type": "SingletonNode",
        "id": 1,
        "estimatedCost": 1,
        "estimatedNrItems": 1
      },
      {
        "type": "IndexNode",
        "id": 2,
        "indexes": ["idx_age_status"],
        "estimatedCost": 50,
        "estimatedNrItems": 250,
        "condition": "doc.age > 25 AND doc.status == 'active'"
      },
      {
        "type": "CalculationNode",
        "id": 3,
        "estimatedCost": 250,
        "expression": "doc.name"
      },
      {
        "type": "SortNode",
        "id": 4,
        "estimatedCost": 300,
        "estimatedNrItems": 250,
        "elements": [{"inVariable": "doc", "ascending": true}]
      },
      {
        "type": "LimitNode",
        "id": 5,
        "estimatedCost": 310,
        "estimatedNrItems": 10,
        "offset": 0,
        "limit": 10
      },
      {
        "type": "ReturnNode",
        "id": 6,
        "estimatedCost": 320,
        "estimatedNrItems": 10
      }
    ],
    "estimatedCost": 320,
    "estimatedNrItems": 10
  }
}
```

### Execution Node Types

| Node Type | Description | Cost |
|-----------|-------------|------|
| **SingletonNode** | Start node | 1 |
| **IndexNode** | Index scan | Low-Medium (depends on selectivity) |
| **EnumerateCollectionNode** | Full collection scan | High |
| **FilterNode** | Filter operation | Linear with input |
| **CalculationNode** | Compute expression | Low |
| **SortNode** | Sort operation | O(n log n) |
| **LimitNode** | Limit results | Low |
| **CollectNode** | Grouping/Aggregation | Medium-High |
| **SubqueryNode** | Nested query | Varies |

### Comparing Plans

```javascript
// Option 1: Index on age
const plan1 = await db.explain(`
  FOR doc IN users
    FILTER doc.age > 25
    RETURN doc
`);

// Option 2: Index on status + Filter
const plan2 = await db.explain(`
  FOR doc IN users
    FILTER doc.status == "active"
    FILTER doc.age > 25
    RETURN doc
`);

console.log(`Plan 1 cost: ${plan1.plan.estimatedCost}`);
console.log(`Plan 2 cost: ${plan2.plan.estimatedCost}`);
// Choose plan with lower cost
```

---

## Performance-Metriken

### PROFILE für Runtime Stats

```aql
PROFILE
FOR doc IN users
  FILTER doc.age > 25
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
```

**Output:**
```json
{
  "result": [...],
  "profile": {
    "nodes": [
      {
        "id": 2,
        "type": "IndexNode",
        "calls": 1,
        "items": 250,
        "runtime": 12.5
      },
      {
        "id": 4,
        "type": "SortNode",
        "calls": 1,
        "items": 250,
        "runtime": 18.3
      },
      {
        "id": 5,
        "type": "LimitNode",
        "calls": 1,
        "items": 10,
        "runtime": 0.1
      }
    ],
    "stats": {
      "executionTime": 45.2,
      "scanned": 250,
      "filtered": 250,
      "writesExecuted": 0,
      "writesIgnored": 0,
      "cacheHits": 15,
      "cacheMisses": 5
    }
  }
}
```

### Key Metrics

- **executionTime**: Gesamt-Ausführungszeit (ms)
- **scanned**: Anzahl gescannter Dokumente
- **filtered**: Anzahl nach Filter übrig
- **cacheHits/Misses**: Query Cache Performance
- **writesExecuted**: Anzahl Schreiboperationen

### Performance Goals

| Query Type | Target Time | Notes |
|------------|-------------|-------|
| Point Lookup (Index) | < 5ms | Single document by ID/unique key |
| Range Query | < 50ms | Filtered scan with index |
| Aggregation (small) | < 100ms | GROUP BY with < 10K rows |
| Aggregation (large) | < 1s | GROUP BY with > 100K rows |
| Graph Traversal (short) | < 50ms | 1-3 hops |
| Graph Traversal (long) | < 500ms | 4-6 hops |
| Vector Search | < 100ms | K-NN with HNSW index |
| Full-Text Search | < 200ms | Inverted index search |

---

## Optimierungs-Patterns

### Pattern 1: Early Filter Reduction

```aql
// ❌ Schlecht: Spät filtern
FOR doc IN users
  LET projects = (
    FOR p IN projects
      RETURN p
  )
  FILTER LENGTH(projects) > 0
  RETURN {doc, projects}
```

```aql
// ✅ Gut: Früh filtern
FOR doc IN users
  FILTER (
    FOR p IN projects
      FILTER p.owner == doc._id
      LIMIT 1
      RETURN 1
  ) != []
  LET projects = (
    FOR p IN projects
      FILTER p.owner == doc._id
      RETURN p
  )
  RETURN {doc, projects}
```

### Pattern 2: Index-Friendly Queries

```aql
// ❌ Schlecht: Function verhindert Index-Nutzung
FOR doc IN users
  FILTER LOWER(doc.email) == "john@example.com"
  RETURN doc
```

```aql
// ✅ Gut: Index-kompatibel
FOR doc IN users
  FILTER doc.email == "john@example.com"
  RETURN doc
```

**Alternative:** Functional Index erstellen
```aql
CREATE INDEX idx_email_lower ON users(LOWER(email))
```

### Pattern 3: JOIN Order Optimization

```aql
// Optimizer wählt automatisch optimale Reihenfolge
FOR user IN users              // 1M documents
  FOR order IN orders          // 10M documents
    FILTER order.user_id == user._id
    RETURN {user, order}
```

**Optimizer entscheidet:**
- Kleinere Kollektion zuerst (users)
- Index auf foreign key nutzen (orders.user_id)

### Pattern 4: Subquery Elimination

```aql
// ❌ Schlecht: Redundante Subquery
FOR doc IN users
  LET count1 = LENGTH(FOR p IN projects FILTER p.owner == doc._id RETURN 1)
  LET count2 = LENGTH(FOR p IN projects FILTER p.owner == doc._id RETURN 1)
  RETURN {doc, count1, count2}
```

```aql
// ✅ Gut: Einmal berechnen
FOR doc IN users
  LET count = LENGTH(FOR p IN projects FILTER p.owner == doc._id RETURN 1)
  RETURN {doc, count1: count, count2: count}
```

### Pattern 5: Materialized Views (WITH)

```aql
// Wiederverwendbare CTEs für komplexe Queries
WITH
  active_users = (
    FOR user IN users
      FILTER user.status == "active"
      RETURN user
  ),
  recent_orders = (
    FOR order IN orders
      FILTER order.date > DATE_ADD(DATE_NOW(), -30, "days")
      RETURN order
  )

FOR user IN active_users
  FOR order IN recent_orders
    FILTER order.user_id == user._id
    RETURN {user, order}
```

---

## Common Bottlenecks

### 1. Full Collection Scans

**Problem:**
```aql
FOR doc IN users
  FILTER doc.age > 25
  RETURN doc
```

**EXPLAIN zeigt:**
```json
{
  "type": "EnumerateCollectionNode",
  "estimatedCost": 10000  // High cost!
}
```

**Solution:** Index erstellen
```aql
CREATE INDEX idx_age ON users(age) BTREE
```

### 2. Kartesisches Produkt

**Problem:**
```aql
FOR user IN users      // 10K users
  FOR product IN products  // 100K products
    RETURN {user, product}
// Result: 1B rows!
```

**Solution:** JOIN-Condition hinzufügen
```aql
FOR user IN users
  FOR product IN products
    FILTER product.owner == user._id
    RETURN {user, product}
```

### 3. Ineffiziente Sorts

**Problem:**
```aql
FOR doc IN large_collection  // 1M docs
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
// Sortiert 1M Dokumente für 10 Ergebnisse
```

**Solution:** Index-gestützte Sort
```aql
CREATE INDEX idx_name ON large_collection(name) SKIPLIST

// Optimizer nutzt Index automatisch
FOR doc IN large_collection
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
```

### 4. N+1 Subqueries

**Problem:**
```aql
FOR user IN users  // 10K users
  LET orders = (  // 10K subqueries!
    FOR order IN orders
      FILTER order.user_id == user._id
      RETURN order
  )
  RETURN {user, orders}
```

**Solution:** Single Query mit COLLECT
```aql
FOR order IN orders
  FOR user IN users
    FILTER order.user_id == user._id
    COLLECT u = user INTO orderList = order
    RETURN {user: u, orders: orderList}
```

### 5. Unnötige Daten-Transfer

**Problem:**
```aql
FOR doc IN large_docs
  RETURN doc  // Jedes Dokument 1MB
// Transfer: 1GB für 1000 Dokumente
```

**Solution:** Projektion
```aql
FOR doc IN large_docs
  RETURN {
    id: doc._id,
    name: doc.name,
    summary: SUBSTRING(doc.content, 0, 100)
  }
// Transfer: ~10KB pro Dokument
```

---

## Monitoring und Profiling

### Query Monitoring aktivieren

```javascript
// Server config
{
  "query": {
    "tracking": true,
    "trackSlowQueries": true,
    "slowQueryThreshold": 1000  // 1 second
  }
}
```

### Slow Query Log

```javascript
// Slow Queries abfragen
db.query(`
  FOR query IN _queries
    FILTER query.runTime > 1000
    SORT query.runTime DESC
    LIMIT 10
    RETURN {
      query: query.query,
      runTime: query.runTime,
      started: query.started
    }
`);
```

### Query Cache Stats

```javascript
// Cache-Statistiken
db.query(`
  RETURN {
    cacheHits: CACHE_HITS(),
    cacheMisses: CACHE_MISSES(),
    cacheSize: CACHE_SIZE(),
    hitRate: CACHE_HITS() / (CACHE_HITS() + CACHE_MISSES()) * 100
  }
`);
```

### Prometheus Metrics

```
# Query execution time histogram
themis_query_duration_seconds_bucket{le="0.01"} 1250
themis_query_duration_seconds_bucket{le="0.1"} 4500
themis_query_duration_seconds_bucket{le="1.0"} 9800
themis_query_duration_seconds_bucket{le="+Inf"} 10000

# Query cache hit rate
themis_query_cache_hit_rate 0.85

# Slow queries per minute
themis_slow_queries_total 12
```

---

## Performance Checklist

### Query Design
- [ ] Frühe FILTER-Klauseln
- [ ] Index-kompatible Bedingungen
- [ ] Projektion statt komplette Dokumente
- [ ] LIMIT sinnvoll einsetzen
- [ ] Subqueries minimieren

### Index Strategy
- [ ] Indizes für Filter-Felder
- [ ] Composite Indexes für Multi-Field Queries
- [ ] Index Selectivity beachten
- [ ] Covering Indexes nutzen
- [ ] Unnötige Indizes entfernen

### Execution Analysis
- [ ] EXPLAIN für Plan-Analyse
- [ ] PROFILE für Runtime-Stats
- [ ] Geschätzte vs. tatsächliche Kosten vergleichen
- [ ] Bottlenecks identifizieren
- [ ] Alternative Pläne testen

### Monitoring
- [ ] Slow Query Tracking aktiviert
- [ ] Cache Hit Rate überwachen
- [ ] Query-Statistiken regelmäßig prüfen
- [ ] Performance-Metriken in Grafana

---

## Optimizer Hints

### Force Index Usage

```aql
FOR doc IN users
  OPTIONS {indexHint: "idx_email", forceIndexHint: true}
  FILTER doc.email == @email
  RETURN doc
```

### Disable Optimizer Rules

```aql
FOR doc IN users
  OPTIONS {
    optimizer: {
      rules: ["-move-filters-up", "-use-indexes"]
    }
  }
  FILTER doc.age > 25
  RETURN doc
```

### Set Max Plans

```aql
FOR doc IN users
  OPTIONS {
    optimizer: {
      maxPlans: 10
    }
  }
  FILTER doc.age > 25 AND doc.status == "active"
  RETURN doc
```

---

## Siehe auch

- [AQL Syntax Guide](AQL_SYNTAX_GUIDE.md)
- [AQL Best Practices](AQL_BEST_PRACTICES.md)
- [Index Management](../architecture/indexes.md)
- [Performance Monitoring](../observability/monitoring.md)
- [Query Profiling Tools](../tools/profiling.md)
