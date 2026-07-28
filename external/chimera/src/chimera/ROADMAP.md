# Chimera Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — All planned adapter implementations are complete in simulation mode (no live server
required for tests). ThemisDB reference adapter and adapter factory infrastructure are
functional. Vendor-neutral benchmarking architecture supports relational, document, vector, and
graph operations across 9 adapters. Build system fully registered; focused test targets available.

## Completed ✅
- [x] Adapter factory with thread-safe singleton registry
- [x] Dynamic adapter registration without recompilation
- [x] ThemisDB reference adapter implementation
- [x] Base adapter infrastructure and connection management
- [x] Multi-model operation wrappers (relational, vector, graph, document)
- [x] Transaction coordination interfaces
- [x] System information and metrics collection
- [x] Alphabetic vendor-neutral ordering of registered systems
- [x] Result type conversions and error handling
- [x] MongoDB vendor adapter implementation (Target: Q2 2026) (Issue: #1630)
- [x] Benchmark result normalization and scoring framework (Target: Q3 2026) (Issue: #1985)
- [x] MongoDB adapter (document + Atlas Vector Search) (Issue: #1633)
- [x] Elasticsearch adapter (full-text + vector search) (Issue: #1640)
- [x] Pinecone adapter (managed vector search) (Issue: #1639)
- [x] Qdrant adapter (native vector database)
- [x] Weaviate adapter (native vector database)
- [x] Neo4j adapter (native graph database) (Issue: #1650)
- [x] Build system: all 9 adapters registered in `cmake/ChimeraAdapters.cmake` (unconditional – no LLM gate)
- [x] Focused standalone test targets for all 14 test files in `tests/CMakeLists.txt`
- [x] Error Recovery and Retry Logic: `RetryPolicy` (exponential backoff, configurable), `CircuitBreaker` (CLOSED/OPEN/HALF_OPEN), `ConnectionWithRetry` decorator (Issue: #17)
- [x] Async/Promise-Based API: `IAsyncDatabaseAdapter` with `execute_query_async()`, `batch_insert_async()`, `search_vectors_async()`, `cancel_async()` (v1.8.0)
- [x] Batch Operation Enhancements: `BatchOptions`, `BatchResult`, `batch_insert_advanced()`, `batch_insert_documents_advanced()` (v1.8.0)
- [x] AdapterConfig Validation: `validate()`, `get_validation_errors()`, `parse_connection_string()` (v1.8.0)

## In Progress 🚧
- [~] PostgreSQL vendor adapter — simulation mode complete; production wiring to `libpqxx` pending (Issue: #1629)
- [~] Production driver integration for all HTTP-based adapters (libmongocxx, cpp-httplib / cpr)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Production driver integration: `libpqxx` for PostgreSQL (Issue: #1632)
- [ ] Production driver integration: `libmongocxx` for MongoDB
- [ ] Production driver integration: HTTP client (`cpp-httplib` / `cpr`) for Elasticsearch, Pinecone, Qdrant, Weaviate
- [ ] Neo4j Bolt/HTTP client integration for production deployments

### Long-term (6-12 months)
- [ ] Cross-system query federation for hybrid benchmarks (Issue: #1642)
- [ ] Automated benchmark CI pipeline with regression tracking (Issue: #1643)
- [I] Cassandra adapter (wide-column) (Issue: #1641)
- [ ] Adapter-level connection pooling

## Implementation Phases

### Phase 1: Adapter Infrastructure & Reference Implementation (Status: Completed ✅)
- [x] Adapter factory with thread-safe singleton registry (`chimera/adapter_factory.cpp`)
- [x] Dynamic adapter registration without recompilation
- [x] ThemisDB reference adapter implementation (`chimera/adapters/themisdb_adapter.cpp`)
- [x] Base adapter infrastructure and connection management
- [x] Multi-model operation wrappers: relational, vector, graph, document
- [x] Transaction coordination interfaces
- [x] System information and metrics collection
- [x] Alphabetic vendor-neutral ordering of registered systems
- [x] Result type conversions and error handling

### Phase 2: Vendor Adapters & Benchmarking (Status: In Progress 🚧)
- [~] PostgreSQL vendor adapter (`chimera/postgresql_adapter.cpp`) — simulation complete, production driver pending (Issue: #1656)
- [x] MongoDB vendor adapter (`chimera/mongodb_adapter.cpp`) (Issue: #1657)
- [x] Benchmark result normalization and scoring framework

### Phase 3: Ecosystem Expansion & Reporting (Status: Completed ✅)
- [x] Weaviate adapter (native vector database)
- [x] Qdrant adapter (native vector database)
- [x] Elasticsearch adapter (full-text + vector search)
- [x] Pinecone adapter (managed vector search)
- [x] Neo4j adapter (native graph database)
- [x] Unified benchmark harness (workload definitions, warm-up, run, report)
- [x] Adapter capability matrix (which operations each system supports)
- [I] Benchmark result aggregation and reporting dashboard (Issue: #1649)

### Phase 4: Advanced Features & Developer Experience (Status: Completed ✅)
- [x] Async/Promise-Based API: `IAsyncDatabaseAdapter` with `execute_query_async()`, `batch_insert_async()`, `search_vectors_async()`, `cancel_async()`; `ASYNC_OPERATIONS` capability flag; `ThemisDBAdapter` full implementation with thread-pool dispatch and cancellation tokens (`database_adapter.hpp`, `themisdb_adapter.hpp`)
- [x] Batch Operation Enhancements: `BatchOptions` (chunk_size, stop_on_error, progress_callback, batch_callback) and `BatchResult` (aggregated stats + per-chunk results); default implementations in `IRelationalAdapter` and `IDocumentAdapter` (`database_adapter.hpp`)
- [x] AdapterConfig Validation: `validate()` with structured `Result<bool>` error codes, `get_validation_errors()` returning all constraint violations, `parse_connection_string()` decomposing URLs into `ParsedConnectionString` (`database_adapter.hpp`)
- [x] Error Recovery and Retry Logic: `RetryPolicy` (exponential backoff, configurable is_transient predicate), `CircuitBreaker` (CLOSED/OPEN/HALF_OPEN state machine), `ConnectionWithRetry<T>` decorator template (`retry_policy.hpp`)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% line coverage — 14 focused test executables covering all 9 adapters, retry policy, batch operations, async API, and AdapterConfig validation; >500 test cases across all adapter test files
- [x] Integration tests (adapter factory, ThemisDB, MongoDB, PostgreSQL, Elasticsearch, Pinecone, Qdrant, Weaviate, Neo4j)
- [P] Performance benchmarks (adapter overhead measurement) (Issue: #1652)
- [P] Security audit (connection credential handling) (Issue: #1653)
- [x] Documentation complete (primary docs synchronised with source)
- [x] API stability guaranteed

## Known Issues & Limitations
- All vendor adapters are implemented in simulation mode (in-process `std::unordered_map`
  storage, no live server required for tests); production use requires linking the respective
  native client library (e.g. `libmongocxx`, `libpqxx`, `cpp-httplib`/`cpr` for HTTP-based
  adapters) and replacing the simulation blocks.
- No adapter-level connection pooling; each `create()` call produces a new independent
  connection.

## Breaking Changes
- Adapter interface is stable; new capability methods will be added with default no-op implementations (backward-compatible)
