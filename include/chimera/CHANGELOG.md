<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Chimera Module Public Headers

All notable changes to public headers in `include/chimera/`.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `pinecone_adapter.hpp`: `PineconeAdapter` for Pinecone vector store
- `qdrant_adapter.hpp`: `QdrantAdapter` for Qdrant vector database
- `weaviate_adapter.hpp`: `WeaviateAdapter` for Weaviate vector search

### Changed
- `database_adapter.hpp`: `IDatabaseAdapter` extended with `batchInsert()` and `batchUpdate()` async methods
- `connection_pool.hpp`: `PoolConfig` extended with `health_check_interval_ms` and `max_idle_time_ms`
- `retry_policy.hpp`: Added `jitter_mode` enum (full, equal, decorrelated)

## [1.7.0] — 2026-03-09
### Added
- `neo4j_adapter.hpp`: `Neo4jAdapter` for Neo4j Cypher query execution
- `themisdb_adapter.hpp`: `ThemisDBAdapter` for self-referential federated queries

### Changed
- `database_adapter.hpp`: `QueryResult` extended with `execution_time_ms` and `rows_affected`
- `elasticsearch_adapter.hpp`: `ElasticsearchConfig` extended with `index_prefix` and `refresh_policy`

## [1.6.0] — 2026-02-01
### Added
- Initial public header set: `database_adapter.hpp`, `connection_pool.hpp`, `retry_policy.hpp`
- `mongodb_adapter.hpp`: MongoDB CRUD and aggregation adapter
- `postgresql_adapter.hpp`: PostgreSQL query adapter
- `elasticsearch_adapter.hpp`: Elasticsearch query adapter
