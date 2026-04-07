<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# Chimera Module — Public Header Architecture

**Version:** 1.8.0  
**Last Updated:** 2026-04-06  
**Module Path:** `include/chimera/`  
**Implementation:** `../../src/chimera/`

---

## 1. Overview

The `include/chimera/` directory exposes public C++ headers for ThemisDB's Chimera
multi-database adapter layer. Chimera provides a unified async interface to heterogeneous
databases — MongoDB, PostgreSQL, Elasticsearch, Neo4j, Pinecone, Qdrant, Weaviate, and
ThemisDB itself — with connection pooling, retry policies, and a common adapter contract.

---

## 2. Design Principles

- **Unified Adapter Contract** – `database_adapter.hpp` defines the single `IDatabaseAdapter`
  interface; all database-specific adapters implement this contract.
- **Async First** – All adapter operations return `std::future` or coroutine handles; no
  synchronous blocking database calls in the public API.
- **Connection Pooling** – `connection_pool.hpp` provides a backend-agnostic connection pool
  used by all adapters; pool lifecycle is managed independently of adapter instances.
- **Retry Resilience** – `retry_policy.hpp` defines configurable exponential-backoff retry
  policies; all adapters accept a `RetryPolicy` at construction.
- **ThemisDB Self-Adapter** – `themisdb_adapter.hpp` allows Chimera to route queries to
  ThemisDB itself, enabling federated query patterns.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `database_adapter.hpp` | `IDatabaseAdapter`, `AdapterConfig`, `QueryResult` | Primary adapter interface |
| `connection_pool.hpp` | `IConnectionPool<C>`, `PoolConfig` | Generic connection pool |
| `retry_policy.hpp` | `RetryPolicy`, `RetryConfig` | Exponential-backoff retry configuration |
| `mongodb_adapter.hpp` | `MongoDBAdapter`, `MongoDBConfig` | MongoDB CRUD and aggregation |
| `postgresql_adapter.hpp` | `PostgreSQLAdapter`, `PostgreSQLConfig` | PostgreSQL query adapter |
| `elasticsearch_adapter.hpp` | `ElasticsearchAdapter`, `ElasticsearchConfig` | Elasticsearch query adapter |
| `neo4j_adapter.hpp` | `Neo4jAdapter`, `Neo4jConfig` | Neo4j graph query adapter |
| `pinecone_adapter.hpp` | `PineconeAdapter`, `PineconeConfig` | Pinecone vector store adapter |
| `qdrant_adapter.hpp` | `QdrantAdapter`, `QdrantConfig` | Qdrant vector database adapter |
| `weaviate_adapter.hpp` | `WeaviateAdapter`, `WeaviateConfig` | Weaviate vector search adapter |
| `themisdb_adapter.hpp` | `ThemisDBAdapter`, `ThemisDBAdapterConfig` | ThemisDB self-adapter |

> **Implementation details:** `../../src/chimera/`
