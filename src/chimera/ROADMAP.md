# Chimera Module Roadmap

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
- [ ] PostgreSQL vendor adapter implementation (Target: Q2 2026)
- [ ] MongoDB vendor adapter implementation (Target: Q2 2026)
- [ ] Benchmark result normalization and scoring framework (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] PostgreSQL adapter (relational + pgvector)
- [ ] MongoDB adapter (document + Atlas Vector Search)
- [ ] Weaviate adapter (native vector database)
- [ ] Qdrant adapter (native vector database)
- [ ] Unified benchmark harness (workload definitions, warm-up, run, report)
- [ ] Adapter capability matrix (which operations each system supports)

### Long-term (6-12 months)
- [ ] Neo4j adapter (native graph database)
- [ ] Pinecone adapter (managed vector search)
- [ ] Elasticsearch adapter (full-text + vector)
- [ ] Cassandra adapter (wide-column)
- [ ] Cross-system query federation for hybrid benchmarks
- [ ] Automated benchmark CI pipeline with regression tracking

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (adapter factory, ThemisDB adapter)
- [ ] Performance benchmarks (adapter overhead measurement)
- [ ] Security audit (connection credential handling)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Only the ThemisDB reference adapter is currently implemented
- Benchmark harness test suites are in a separate module and not yet integrated
- No vendor adapters for external databases yet

## Breaking Changes
- Adapter interface is stable; new capability methods will be added with default no-op implementations (backward-compatible)
