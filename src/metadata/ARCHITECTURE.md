> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Metadata Module — Architecture Guide
<!-- status: current | validated: 2026-04-06 | commit: 4c1a2dfc1 -->

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/metadata/`

---

## 1. Overview

The Metadata module manages ThemisDB's schema catalog — the authoritative source of
information about collections, fields, types, indexes, constraints, and statistics. It
provides INFORMATION_SCHEMA-compatible views, automatic schema discovery by scanning
RocksDB keys, index recommendations based on query patterns, schema versioning, and
audit logging for schema changes.

---

## 2. Design Principles

- **Schema Discovery** – collections and their field types can be discovered automatically
  by scanning stored entities; no explicit DDL is required.
- **Cached Reads** – metadata is read far more often than written; a TTL-based cache with
  a read-write lock provides fast, thread-safe access.
- **SQL-Standard INFORMATION_SCHEMA** – metadata is exposed through standard
  `INFORMATION_SCHEMA.TABLES`, `COLUMNS`, and `INDEXES` views, enabling familiar
  introspection queries.
- **Index Advisor** – `index_recommender.cpp` analyzes slow query patterns and suggests
  new indexes; auto-creation is configurable.
- **Schema Versioning** – all schema changes are version-stamped and audited, enabling
  rollback and compliance audit.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `schema_manager.cpp` | Central metadata catalog: collection/field/type discovery and registry |
| `information_schema.cpp` | SQL INFORMATION_SCHEMA views (TABLES, COLUMNS, INDEXES) |
| `statistics_collector.cpp` | Table/index statistics for query optimizer |
| `index_recommender.cpp` | Recommends new indexes based on query patterns |
| `schema_version_manager.cpp` | Schema version tracking and history |
| `schema_audit_log.cpp` | Audit log for schema changes (DDL events) |
| `schema_constraints.cpp` | NOT NULL, UNIQUE, CHECK constraint management |
| `schema_consistency_checker.cpp` | Validates schema consistency vs. stored data |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Query Engine / Optimizer (src/query/)              │
│   schema.getCollection("users") → CollectionMetadata           │
│   statistics.getCardinality("users.age") → 250                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    SchemaManager                                 │
│                                                                  │
│  discover() → scan RocksDB keys → infer field types             │
│  register(collection, schema) → persist to catalog              │
│  getMetadata(collection) → from cache (TTL 60s)                 │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Metadata Cache (read-write lock, configurable TTL)      │   │
│  └─────────────────────────────────────────────────────────┘   │
└───┬──────────────────┬─────────────────────────────────────────┘
    │                  │
┌───▼──────┐  ┌────────▼─────────────────────────────────────────┐
│ Stats    │  │  SchemaVersionManager + SchemaAuditLog            │
│Collector │  │  version: 42 | DDL events logged                  │
└──────────┘  └──────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Schema Discovery (First Use)

```
schema_manager.discoverSchema("users_collection")
    │
    ├─ scan RocksDB key prefix for "users"
    ├─ sample N entities → inspect field names and JSON types
    ├─ infer schema: {name: string, age: int, email: string, ...}
    ├─ register schema in catalog (RocksDB persistence)
    ├─ cache result (TTL 60s)
    └─ return CollectionMetadata
```

### 4.2 Query Optimizer Statistics

```
statistics_collector.getCardinality("users", "age")
    │
    ├─ check stats cache (TTL 5 min)
    ├─ cache miss → scan index statistics or sample collection
    └─ return {cardinality: 250, null_fraction: 0.02, avg_width: 4}
```

### 4.3 Index Recommendation

```
index_recommender.analyze(slow_query_log)
    │
    ├─ parse query predicates → identify frequently filtered fields
    ├─ check existing indexes → no index on "users.email"?
    ├─ estimate benefit: high selectivity → recommend B-tree index
    └─ emit recommendation or auto-create (if enabled)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/query/` | Schema and statistics for query planning |
| **Called by** | `src/aql/` | Schema context for NL-to-AQL translation |
| **Called by** | `src/index/` | Index metadata registration |
| **Uses** | `src/storage/` | Schema persistence via RocksDB |
| **Provides to** | `src/server/` | INFORMATION_SCHEMA query endpoints |

---

## 6. Threading & Concurrency Model

- `SchemaManager` uses a `shared_mutex`: many readers hold shared lock, schema mutations
  hold exclusive lock.
- `statistics_collector` cache has its own per-table read-write lock.
- `SchemaAuditLog` uses an append-only lock-free structure.
- Cache TTL expiry uses a background refresh thread.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| TTL cache | Metadata reads avoid RocksDB lookups (hot path) |
| Lazy discovery | Schema discovered on first access, not at startup |
| Statistics sampling | Cardinality estimated via random sampling, not full scan |
| Incremental stats | Only updated tables trigger statistics refresh |

---

## 8. Security Considerations

- Schema changes are audited with principal, timestamp, and change delta.
- Schema access is scoped per tenant; cross-tenant schema leakage is prevented.
- `INFORMATION_SCHEMA` views filter to the authenticated user's visible collections.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `metadata.cache.ttl_s` | 60 | Metadata cache TTL |
| `metadata.statistics.ttl_s` | 300 | Statistics cache TTL |
| `metadata.discovery.sample_size` | 100 | Entities sampled during schema discovery |
| `metadata.index_recommender.enabled` | true | Enable index recommendations |
| `metadata.index_recommender.auto_create` | false | Auto-create recommended indexes |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Schema not found | Return empty metadata; trigger auto-discovery |
| Statistics unavailable | Return default estimates; log warning |
| Schema consistency error | Log error; do not auto-repair; alert operator |
| Cache invalidation failure | Re-fetch from storage on next access |

---

## 11. Known Limitations & Future Work

- Schema version rollback is implemented but not exposed via API.
- Schema constraints enforcement (NOT NULL, UNIQUE) at insert time is partial.
- Cross-collection foreign key discovery is experimental.
- Statistics histogram generation (for selectivity estimation) is planned.

---

## 12. References

- `src/metadata/README.md` — module overview
- `docs/schema_management.md` — schema management guide
- `ARCHITECTURE.md` (root) — full system architecture
