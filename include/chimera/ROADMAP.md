<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Chimera Module Public Headers

**Module Path:** `include/chimera/`  
**Implementation Roadmap:** `../../src/chimera/ROADMAP.md`

---

## Current Status

Public headers at v1.8.0. Eight database adapters (MongoDB, PostgreSQL, Elasticsearch,
Neo4j, Pinecone, Qdrant, Weaviate, ThemisDB) plus generic connection pool and retry
policy headers are all stable.

---

## Completed Features

- [x] `IDatabaseAdapter` with batch operations and execution metadata
- [x] `IConnectionPool<C>` with health check and idle timeout
- [x] `RetryPolicy` with jitter modes (full, equal, decorrelated)
- [x] MongoDB, PostgreSQL, Elasticsearch adapters
- [x] Neo4j graph adapter
- [x] Pinecone, Qdrant, Weaviate vector store adapters
- [x] ThemisDB self-adapter for federated patterns

---

## Planned Features

- [ ] `CassandraAdapter` for Apache Cassandra (Target: Q3 2026)
- [ ] `RedisAdapter` for Redis as a Chimera-managed data source (Target: Q3 2026)
- [ ] `ICrossAdapterJoin` for federated join across heterogeneous adapters (Target: Q4 2026)
- [ ] `IAdapterHealthProbe` for liveness/readiness across all adapters (Target: Q4 2026)

---

## Implementation Phases

### Phase 1: Core Adapter Interface
- [x] `IDatabaseAdapter`, `IConnectionPool`, `RetryPolicy`

### Phase 2: Relational & Document Adapters
- [x] MongoDB, PostgreSQL adapters

### Phase 3: Search & Graph Adapters
- [x] Elasticsearch, Neo4j adapters

### Phase 4: Vector Store Adapters
- [x] Pinecone, Qdrant, Weaviate adapters

### Phase 5: Self and Federated
- [x] `ThemisDBAdapter`
- [ ] `ICrossAdapterJoin` (Q4 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs present
- [ ] Doxygen fully annotated on all 11 headers

---

## Production Readiness Checklist

- [x] All 8 database adapters have stable headers
- [x] Connection pool and retry policy headers shared by all adapters
- [x] Async-first API contract
- [ ] `CassandraAdapter` and `RedisAdapter` headers published
- [ ] `ICrossAdapterJoin` header published
