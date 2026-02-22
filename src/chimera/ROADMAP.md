# Chimera Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Alpha** — ThemisDB reference adapter and adapter factory infrastructure are functional. Vendor-neutral benchmarking architecture supports relational, document, vector, and graph operations. PostgreSQL, MongoDB, and Weaviate adapters are planned.

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

## In Progress 🚧
- [I] PostgreSQL vendor adapter implementation (Target: Q2 2026) (Issue: #1629)
- [P] MongoDB vendor adapter implementation (Target: Q2 2026) (Issue: #1630)
- [I] Benchmark result normalization and scoring framework (Target: Q3 2026) (Issue: #1985)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] PostgreSQL adapter (relational + pgvector) (Issue: #1632)
- [I] MongoDB adapter (document + Atlas Vector Search) (Issue: #1633)
- [!] Weaviate adapter (native vector database) (Issue: #2374)
- [I] Qdrant adapter (native vector database) (Issue: #1646)
- [I] Unified benchmark harness (workload definitions, warm-up, run, report) (Issue: #2375)
- [I] Adapter capability matrix (which operations each system supports) (Issue: #2376)

### Long-term (6-12 months)
- [I] Neo4j adapter (native graph database) (Issue: #1650)
- [I] Pinecone adapter (managed vector search) (Issue: #1639)
- [I] Elasticsearch adapter (full-text + vector) (Issue: #1640)
- [I] Cassandra adapter (wide-column) (Issue: #1641)
- [I] Cross-system query federation for hybrid benchmarks (Issue: #1642)
- [I] Automated benchmark CI pipeline with regression tracking (Issue: #1643)

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
- [I] PostgreSQL vendor adapter (`chimera/adapters/postgres_adapter.cpp`, Target: Q2 2026) (Issue: #1656)
- [P] MongoDB vendor adapter (`chimera/adapters/mongodb_adapter.cpp`, Target: Q2 2026) (Issue: #1657)
- [ ] Benchmark result normalization and scoring framework (Target: Q3 2026)

### Phase 3: Ecosystem Expansion & Reporting (Status: Planned 📋)
- [ ] Weaviate adapter (native vector database)
- [ ] Qdrant adapter (native vector database)
- [ ] Unified benchmark harness (workload definitions, warm-up, run, report)
- [ ] Adapter capability matrix (which operations each system supports)
- [I] Benchmark result aggregation and reporting dashboard (Issue: #1649)
- [ ] Neo4j adapter (native graph database)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1651)
- [x] Integration tests (adapter factory, ThemisDB adapter)
- [I] Performance benchmarks (adapter overhead measurement) (Issue: #1652)
- [I] Security audit (connection credential handling) (Issue: #1653)
- [I] Documentation complete (Issue: #1654)
- [I] API stability guaranteed (Issue: #1655)

## Known Issues & Limitations
- Only the ThemisDB reference adapter is currently implemented
- Benchmark harness test suites are in a separate module and not yet integrated
- No vendor adapters for external databases yet

## Breaking Changes
- Adapter interface is stable; new capability methods will be added with default no-op implementations (backward-compatible)
