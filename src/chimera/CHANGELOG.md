<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Chimera Module

All notable changes to the Chimera multi-database adapter framework are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Production driver integration: `libpqxx` for PostgreSQL (Issue #1632)
- Production driver integration: `libmongocxx` for MongoDB
- Production driver integration: HTTP client (`cpp-httplib`/`cpr`) for Elasticsearch, Pinecone, Qdrant, Weaviate
- Neo4j Bolt/HTTP client integration for production deployments
- Cross-system query federation for hybrid benchmarks (Issue #1642)
- Automated benchmark CI pipeline with regression tracking (Issue #1643)
- Cassandra adapter (wide-column) — Issue #1641
- Adapter-level connection pooling

## [1.7.0] — 2026-03-09
### Added
- Build system: all 9 adapters registered in `cmake/ChimeraAdapters.cmake` (unconditional — no LLM gate)
- Focused standalone test targets for all 10 test files in `tests/CMakeLists.txt`
- Benchmark result normalization and scoring framework (Issue #1985)

## [1.6.0] — 2026-02-10
### Added
- Weaviate adapter for native vector database integration (`chimera/weaviate_adapter.cpp`)
- Qdrant adapter for native vector database integration (`chimera/qdrant_adapter.cpp`)
- Neo4j adapter for native graph database integration (`chimera/neo4j_adapter.cpp`) (Issue #1650)
- Elasticsearch adapter (full-text + vector search) (`chimera/elasticsearch_adapter.cpp`) (Issue #1640)
- Pinecone adapter (managed vector search) (`chimera/pinecone_adapter.cpp`) (Issue #1639)
- Alphabetic vendor-neutral ordering of registered systems in adapter factory

## [1.5.0] — 2026-01-15
### Added
- MongoDB vendor adapter with document + Atlas Vector Search (`chimera/mongodb_adapter.cpp`) (Issue #1630, #1633)
- PostgreSQL vendor adapter (simulation mode) (`chimera/postgresql_adapter.cpp`) (Issue #1629)

## [1.0.0] — 2024-01-01
### Added
- Adapter factory with thread-safe singleton registry (`chimera/adapter_factory.cpp`)
- Dynamic adapter registration without recompilation
- ThemisDB reference adapter implementation (`chimera/themisdb_adapter.cpp`)
- Base adapter infrastructure and connection management
- Multi-model operation wrappers: relational, vector, graph, document
- Transaction coordination interfaces
- System information and metrics collection
- Result type conversions and error handling
