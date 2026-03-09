# Chimera Module — Architecture Guide

**Version:** 1.1  
**Last Updated:** 2026-03-09  
**Module Path:** `src/chimera/`

---

## 1. Overview

The **Chimera** module (Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource
Assessment) provides a vendor-neutral database adapter architecture. Its primary purpose is
to enable fair benchmarking of diverse database systems (relational, document, graph, vector,
hybrid) through a single unified interface.

Named after the mythological creature with multiple forms, the adapter layer abstracts each
database system's native API while exposing consistent query, transaction, and metrics
interfaces to benchmark and integration consumers.

---

## 2. Design Principles

- **Interface Segregation** – `IDatabaseAdapter` is split into functional sub-interfaces
  (IRelationalOps, IVectorOps, IGraphOps, IDocumentOps) so adapters implement only what
  they support.
- **Factory Pattern** – adapters are registered by name at startup; consumers create them
  by string name without knowing the concrete class.
- **Thread-Safe Registry** – the singleton factory registry is protected by a mutex so
  adapters can be registered concurrently during static initialization.
- **Vendor Neutrality** – no ThemisDB-internal types leak into the adapter interface; all
  data passes through neutral `nlohmann::json` or `AdapterResult<T>` types.
- **Result Types** – all operations return `AdapterResult<T>` (a `std::expected`-like type)
  to avoid exception-based control flow in benchmark hot paths.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `adapter_factory.cpp` | Thread-safe singleton registry: register/create/list adapters |
| `themisdb_adapter.cpp` | ThemisDB reference implementation of `IDatabaseAdapter` |
| `mongodb_adapter.cpp` | MongoDB adapter (document + Atlas Vector Search) |
| `postgresql_adapter.cpp` | PostgreSQL adapter (relational + pgvector) |
| `elasticsearch_adapter.cpp` | Elasticsearch adapter (full-text + vector search) |
| `pinecone_adapter.cpp` | Pinecone adapter (managed vector search) |
| `qdrant_adapter.cpp` | Qdrant adapter (native vector database) |
| `weaviate_adapter.cpp` | Weaviate adapter (native vector database) |
| `neo4j_adapter.cpp` | Neo4j adapter (native graph database) |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Benchmark Harness / Integration Consumer           │
│   AdapterFactory::create("ThemisDB") → IDatabaseAdapter        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     AdapterFactory (singleton)                  │
│  register_adapter(name, creator_fn) → O(log n) registry         │
│  create(name) → std::unique_ptr<IDatabaseAdapter>               │
│  get_supported_systems() → sorted vector<string>                │
└──────┬───────────┬──────────┬──────────┬────────┬──────┬───────┘
       │           │          │          │        │      │
┌──────▼──────┐ ┌─▼──────┐ ┌─▼──────┐ ┌─▼────┐ ┌▼───┐ ┌▼──────┐ ┌───────────┐
│ThemisDB     │ │MongoDB │ │Postgre-│ │Elas- │ │Pine│ │Qdrant│ │Weaviate│ │Neo4j     │
│Adapter      │ │Adapter │ │SQL     │ │search│ │cone│ │Adapter│ │Adapter │ │Adapter   │
│(reference)  │ │doc+vec │ │rel+vec │ │text+ │ │vec │ │vec    │ │vec     │ │graph     │
│All 5 ops    │ │IDocOps │ │IRelOps │ │vec   │ │only│ │only   │ │only    │ │IGraphOps │
└─────────────┘ └────────┘ │IVecOps │ │IVec  │ └────┘ └───────┘ └────────┘ └──────────┘
                            └────────┘ └──────┘
```

---

## 4. Data Flow

### 4.1 Adapter Registration (startup)

```
Static initializer:
    AdapterFactory::register_adapter("ThemisDB",
        []() { return std::make_unique<ThemisDBAdapter>(); });
    AdapterFactory::register_adapter("MongoDB",
        []() { return std::make_unique<MongoDBAdapter>(); });
    AdapterFactory::register_adapter("PostgreSQL",
        []() { return std::make_unique<PostgreSQLAdapter>(); });
    AdapterFactory::register_adapter("Elasticsearch",
        []() { return std::make_unique<ElasticsearchAdapter>(); });
    AdapterFactory::register_adapter("Pinecone",
        []() { return std::make_unique<PineconeAdapter>(); });
    AdapterFactory::register_adapter("Qdrant",
        []() { return std::make_unique<QdrantAdapter>(); });
    AdapterFactory::register_adapter("Weaviate",
        []() { return std::make_unique<WeaviateAdapter>(); });
    AdapterFactory::register_adapter("Neo4j",
        []() { return std::make_unique<Neo4jAdapter>(); });
```

### 4.2 Benchmark Execution

```
benchmark_harness: AdapterFactory::create("ThemisDB")
    │
    ▼
ThemisDBAdapter::connect(config)
    │
    ▼
ThemisDBAdapter::insertDocument(collection, doc)  → AdapterResult<DocId>
ThemisDBAdapter::vectorSearch(collection, vec, k) → AdapterResult<Results>
ThemisDBAdapter::graphTraversal(start, depth)     → AdapterResult<Paths>
    │
    ▼
benchmark_harness: record latency / throughput / accuracy
    │
    ▼
ThemisDBAdapter::disconnect()
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Wraps** | `src/storage/` | Storage operations via ThemisDB internal APIs |
| **Wraps** | `src/index/` | Vector search and graph traversal |
| **Wraps** | `src/transaction/` | Transaction coordination |
| **Used by** | `benchmarks/` | Benchmark harness |
| **Used by** | `clients/` | External client integrations |
| **Used by** | `adapters/` | Additional vendor adapters |

---

## 6. Threading & Concurrency Model

- `AdapterFactory` registry read-path is lock-free (map is read-only after registration).
- Registration uses `std::mutex` to prevent concurrent map modification.
- Each `IDatabaseAdapter` instance is **not** thread-safe; callers must create one adapter
  per thread or serialize access externally.
- `ThemisDBAdapter` internally delegates to thread-safe ThemisDB engine APIs.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Registry lookup | `std::map<string, creator>` O(log n); n ≤ ~20 adapters |
| Adapter pooling | Benchmark harness may pool adapter instances per thread |
| Neutral data types | `nlohmann::json` for cross-adapter data exchange (no serialization overhead in hot path) |

---

## 8. Security Considerations

- Adapter connections use credentials passed through the `AdapterConfig` struct; credentials
  are not logged.
- The factory registry does not execute user-supplied code; adapter registrations are
  compile-time static initializers.
- External adapters (MongoDB, PostgreSQL) should validate connection strings to prevent
  SSRF or credential injection.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `chimera.default_adapter` | "ThemisDB" | Default adapter when none specified |
| `chimera.connection_timeout_ms` | 5000 | Adapter connection timeout |
| `chimera.query_timeout_ms` | 30000 | Per-query timeout |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Unknown adapter name | `create()` returns `nullptr` |
| Duplicate registration | `register_adapter()` returns `false`; existing entry preserved |
| Connection failure | `AdapterResult` with structured error; no exception thrown |
| Query failure | `AdapterResult` error variant with error code and message |

---

## 11. Known Limitations & Future Work

- All vendor adapters operate in simulation mode (in-process `std::unordered_map` storage);
  production deployments require linking the respective native client library and replacing
  simulation blocks with real API calls (e.g. `libmongocxx`, `libpqxx`, `cpp-httplib`/`cpr`
  for HTTP-based adapters).
- No adapter-level connection pooling; each `create()` call creates a new connection.
- Operations not applicable to a system's data model (e.g. relational ops on Qdrant) return
  `AdapterResult` with `ErrorCode::NOT_IMPLEMENTED`.

---

## 12. References

- `src/chimera/README.md` — module overview
- `src/chimera/FUTURE_ENHANCEMENTS.md` — roadmap
- `adapters/` — external adapter implementations
- `benchmarks/` — benchmark harness
- `ARCHITECTURE.md` (root) — full system architecture
