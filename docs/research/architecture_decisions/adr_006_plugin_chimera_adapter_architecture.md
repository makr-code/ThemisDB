# ADR-006: Plugin-Based Adapter Architecture for Multi-Database Benchmarking (Chimera)

**Status:** Accepted  
**Date:** 2023-08-01  
**Deciders:** @themisdb-core-team  
**Modules Affected:** `src/chimera/`  
**Related Research:** [Exponential Backoff and Retry Best Practices](../best_practices/exponential_backoff_retry.md)

---

## Context

ThemisDB includes a vendor-neutral benchmarking and comparison layer called **Chimera**. Chimera must execute identical workloads against 9+ database backends and report normalized performance and correctness metrics. Supported backends include:

- Relational: PostgreSQL, MySQL
- Document: MongoDB, Elasticsearch
- Vector: Pinecone, Qdrant, Weaviate, Milvus
- Graph: Neo4j, Amazon Neptune
- Multi-model: ArangoDB

Requirements at decision time:

- Adding or removing a backend must not require recompiling the Chimera core or any other registered adapter.
- Each adapter must implement a defined interface for its model category (`IRelationalAdapter`, `IDocumentAdapter`, `IVectorAdapter`, `IGraphAdapter`).
- Adapters must support a **simulation/mock mode** so integration tests can run without live database connections in CI.
- Chimera must produce reproducible benchmark results: same workload definition → same sequence of operations on all backends.
- The retry logic for transient failures (network timeouts, throttling) must be centralized — not duplicated per adapter.

## Decision Drivers

- **No recompilation for new adapters:** Adding a new backend must not require changes to `src/chimera/` core or a full rebuild of ThemisDB.
- **Interface segregation:** Each model category has its own interface; a backend may implement one or multiple interfaces (e.g., ArangoDB implements `IRelationalAdapter` + `IDocumentAdapter` + `IGraphAdapter`).
- **Mock/simulation mode:** CI pipelines must be able to run Chimera tests without live database credentials using in-memory mock adapters.
- **Centralized retry logic:** Exponential backoff with jitter (per best practice) is implemented once in `ChimeraClient` base class, not in each adapter.
- **Deterministic workload replay:** Workload definitions are serialized to YAML; Chimera replays them in identical order across all registered adapters.

## Considered Options

| Option | Pros | Cons |
|--------|------|------|
| **Dynamic adapter registry (AdapterFactory singleton)** | Zero recompilation for new adapters (register via static initializer); runtime discovery; interface contracts enforced at compile time; mock injection for CI | Slightly higher initial design complexity; dynamic linking requires careful symbol visibility management |
| **Hard-coded switch/case per backend** | Simple; no dynamic linking | Every new backend requires modifying and recompiling `src/chimera/`; merge conflicts at scale; tightly couples core to vendor SDKs |
| **Microservice per backend** | Independent scaling; language choice per adapter | Network overhead per benchmark operation (~0.5–2 ms per call); IPC serialization obscures true adapter latency; complex deployment for 9+ services |
| **Template meta-programming specializations** | Zero runtime overhead; compile-time checked | All adapters must be known at compile time; no runtime addition; combinatorial template instantiation slows build |

## Decision

**Chosen: Dynamic Adapter Registry via `AdapterFactory` singleton**

Each adapter is implemented as a shared library (`.so`/`.dll`) that registers itself with the `AdapterFactory` singleton via a static initializer:

```cpp
// In each adapter's .cpp file:
static const bool kRegistered = AdapterFactory::instance().registerAdapter(
    "qdrant", []() -> std::unique_ptr<IVectorAdapter> {
        return std::make_unique<QdrantAdapter>();
    }
);
```

The `AdapterFactory` singleton in `src/chimera/adapter_factory.cpp` maintains a `std::unordered_map<std::string, AdapterCreator>` keyed by backend name. At runtime, Chimera loads adapters from the `chimera/adapters/` plugin directory using `dlopen`/`LoadLibrary`; the static initializer fires on load, populating the registry without any call from the core.

**Interface hierarchy:**

```
IBaseAdapter
├── IRelationalAdapter   (execute_sql, begin_txn, commit_txn)
├── IDocumentAdapter     (insert_doc, find_docs, update_doc, delete_doc)
├── IVectorAdapter       (upsert_vector, ann_search, delete_vector)
└── IGraphAdapter        (add_vertex, add_edge, traverse, shortest_path)
```

A backend may inherit from multiple interfaces. `ArangoDBAdapter` inherits `IRelationalAdapter + IDocumentAdapter + IGraphAdapter`.

**Mock adapters** (`src/chimera/mocks/`) implement all interfaces with in-memory `std::unordered_map` storage and configurable latency injection. They are registered under names `mock_relational`, `mock_vector`, etc. and used by default in CI when the `CHIMERA_MOCK_MODE=1` environment variable is set.

**Centralized retry:** `IBaseAdapter::executeWithRetry()` implements exponential backoff with full jitter (base 100 ms, max 30 s, jitter factor 0.5) per the Exponential Backoff best practice. All adapter operation methods call this helper; adapter authors do not implement retry themselves.

## Consequences

### Positive
- New adapters can be added as standalone shared libraries without touching `src/chimera/` — important for community-contributed adapters.
- Interface segregation allows the benchmark harness to query only the interfaces a backend implements, gracefully skipping unsupported operation types.
- Mock adapters enable full Chimera benchmark suite to run in CI without any external database dependencies (< 5 s for the full suite in mock mode).
- Centralized retry logic ensures consistent behavior across all adapters for transient failure scenarios.

### Negative / Trade-offs
- **Dynamic linking complexity:** `dlopen` on Linux and `LoadLibrary` on Windows require careful symbol visibility (`-fvisibility=hidden` + explicit export macros). *Mitigation: `src/chimera/adapter_export.hpp` provides `CHIMERA_ADAPTER_EXPORT` macro; adapter template in `src/chimera/adapter_template/` pre-configures CMake targets correctly.*
- **ABI stability:** Adapter shared libraries compiled against one version of `IBaseAdapter` may be binary-incompatible with a newer version. *Mitigation: the adapter interface version is encoded in the library name (`libqdrant_adapter_v2.so`); `AdapterFactory` rejects mismatched versions with a clear error.*
- **Mock fidelity:** In-memory mocks do not reproduce real database latency distributions. *Accepted because: mock mode is for correctness testing only; performance benchmarks always run against live backends.*

### Neutral
- Adapters that target cloud APIs (Pinecone, Weaviate) use the centralized HTTP client (`src/chimera/chimera_http_client.cpp`) which wraps Boost.Asio (ADR-003) for consistent connection pooling and TLS configuration.
- The `CHIMERA_ADAPTER_PATH` environment variable overrides the default plugin directory, supporting custom adapter directories in enterprise deployments.

## Validation

- [x] AdapterFactory singleton registers and discovers 3 mock adapters in unit test
- [x] Static initializer registration verified for PostgreSQL, Qdrant, and MongoDB adapters
- [x] Mock mode CI run: full Chimera benchmark suite in < 5 s without live databases
- [x] ABI version mismatch correctly rejected with error message
- [x] Retry logic: exponential backoff with jitter tested with simulated network failures
- [ ] Community adapter contribution guide published to `src/chimera/adapter_template/README.md`
- [ ] Benchmark result normalization and comparison report format finalized

## Follow-up Actions

- [ ] Publish adapter contribution guide with CMake template to `src/chimera/adapter_template/`.
- [ ] Implement `ChimeraResultReporter` for normalized cross-backend comparison output (JSON + Markdown table).
- [ ] Add adapter health-check endpoint called before benchmark run to detect misconfigured connections early.
- [ ] Version `IBaseAdapter` interfaces with semantic versioning and document upgrade path.

## Related Decisions

- [ADR-003: Boost.Beast + Asio for HTTP/WebSocket/MQTT Server](adr_003_boost_beast_asio_http_server.md)
- [ADR-004: Native Multi-Model Data Model](adr_004_multi_model_data_model.md)

---
**Last Updated:** 2026-04-06
