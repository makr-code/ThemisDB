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
- [x] Focused standalone test targets for all 10 test files in `tests/CMakeLists.txt`

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

## Production Readiness Checklist
- [x] Unit tests coverage > 80% line coverage — 10 focused test executables covering all 9 adapters, >500 test cases across all adapter test files
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
