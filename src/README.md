# ThemisDB Source Code

This directory contains the core implementation of ThemisDB's multi-model database system.

## Directory Structure

### Core Components

- **acceleration/** - GPU and hardware acceleration implementations (CUDA, Vulkan)
- **api/** - HTTP API server implementation
- **auth/** - Authentication and authorization components (JWT, RBAC)
- **cache/** - Semantic caching and query result caching
- **cdc/** - Change Data Capture (CDC) and changefeed implementation
- **content/** - Content management, ingestion, and processing pipelines
- **exporters/** - Data export functionality (JSONL, LLM formats)
- **geo/** - Geospatial query processing and indexing
- **governance/** - Policy engine and compliance governance
- **importers/** - Data import functionality (PostgreSQL, etc.)
- **index/** - Index implementations (vector, graph, adaptive, secondary)
- **llm/** - LLM interaction storage and chain-of-thought features
- **plugins/** - Plugin system infrastructure
- **query/** - AQL query parser, optimizer, and execution engine
- **security/** - Encryption, key management, and PKI integration
- **server/** - Main server components and API handlers
- **sharding/** - Horizontal scaling and sharding implementation
- **storage/** - RocksDB wrapper and storage layer abstractions
- **timeseries/** - Time series data management and compression
- **transaction/** - SAGA pattern and transaction management
- **utils/** - Utility functions and shared components

## Building

See the main [README.md](../README.md) for build instructions.

## Architecture

For detailed architecture documentation, see:
- [Architecture Overview](../docs/architecture.md)
- [Source Code Documentation](../docs/src/README.md)

---

## Overall System Status

ThemisDB has **3 Production-grade modules**, **31 Beta modules**, and **10 Alpha modules** across its source tree. The core data path (`storage` → `index` → `query`/`aql`) is production-ready. Higher-level features (AI/ML, voice, GPU acceleration) are in Beta or Alpha.

| Tier       | Count | Modules                                               |
|------------|-------|-------------------------------------------------------|
| Production | 3     | `aql`, `index`, `storage`                             |
| Beta       | 31    | Most infrastructure and query modules                 |
| Alpha      | 10    | `acceleration`, `base`, `chimera`, `config`, `ingestion`, `network`, `plugins`, `prompt_engineering`, `scheduler`, `training`, `voice` |

---

## Production Readiness Matrix

| Module              | Status     | Description                                          |
|---------------------|------------|------------------------------------------------------|
| acceleration        | 🔴 Alpha   | GPU/hardware acceleration (CUDA, Vulkan)             |
| analytics           | 🟡 Beta    | Analytical query pipeline                            |
| api                 | 🟡 Beta    | HTTP API server implementation                       |
| aql                 | 🟢 Production | AQL language engine, multi-paradigm queries       |
| auth                | 🟡 Beta    | JWT, RBAC, enterprise SSO/MFA                        |
| base                | 🔴 Alpha   | Foundational abstractions                            |
| cache               | 🟡 Beta    | Semantic and query result caching                    |
| cdc                 | 🟡 Beta    | Change Data Capture and changefeeds                  |
| chimera             | 🔴 Alpha   | Hybrid multi-model layer, experimental               |
| config              | 🔴 Alpha   | Configuration management                             |
| content             | 🟡 Beta    | Content ingestion and processing pipelines           |
| core                | 🟡 Beta    | Core database runtime                                |
| exporters           | 🟡 Beta    | Data export (JSONL, LLM formats)                     |
| geo                 | 🟡 Beta    | Geospatial query processing and indexing             |
| governance          | 🟡 Beta    | Policy engine and compliance governance              |
| gpu                 | 🟡 Beta    | GPU compute integration                              |
| graph               | 🟡 Beta    | Property graph queries and traversal                 |
| importers           | 🟡 Beta    | Data import (PostgreSQL, etc.)                       |
| index               | 🟢 Production | HNSW, R-tree, adaptive indexing                  |
| ingestion           | 🔴 Alpha   | Data ingestion pipeline                              |
| llm                 | 🟡 Beta    | LLM interaction storage and chain-of-thought         |
| metadata            | 🟡 Beta    | Metadata management and catalog                      |
| network             | 🔴 Alpha   | Network layer and peer communication                 |
| observability       | 🟡 Beta    | Metrics, tracing, and logging infrastructure         |
| performance         | 🟡 Beta    | Benchmarking and performance optimization            |
| plugins             | 🔴 Alpha   | Plugin system infrastructure                         |
| prompt_engineering  | 🔴 Alpha   | LLM prompt management, experimental                  |
| query               | 🟡 Beta    | AQL optimizer, cost-based planner, execution engine  |
| rag                 | 🟡 Beta    | Retrieval-Augmented Generation pipeline              |
| replication         | 🟡 Beta    | Raft-based replication                               |
| scheduler           | 🔴 Alpha   | Task and job scheduling                              |
| search              | 🟡 Beta    | Full-text and hybrid search                          |
| security            | 🟡 Beta    | Encryption, key management, PKI integration          |
| server              | 🟡 Beta    | Main server components and API handlers              |
| sharding            | 🟡 Beta    | Horizontal scaling and sharding                      |
| storage             | 🟢 Production | RocksDB wrapper, MVCC, backup/recovery           |
| temporal            | 🟡 Beta    | Temporal and bitemporal data support                 |
| themis              | 🟡 Beta    | Core ThemisDB orchestration layer                    |
| timeseries          | 🟡 Beta    | Time series data management and compression          |
| training            | 🔴 Alpha   | ML model training integration, experimental          |
| transaction         | 🟡 Beta    | SAGA pattern and distributed transactions            |
| updates             | 🟡 Beta    | Schema and data update management                    |
| utils               | 🟡 Beta    | Shared utility functions and helpers                 |
| voice               | 🔴 Alpha   | Voice query interface, experimental                  |

---

## Maturity Levels

| Level         | Meaning                                                                                   |
|---------------|-------------------------------------------------------------------------------------------|
| 🟢 Production | Feature-complete, thoroughly tested, used in production deployments. API is stable.       |
| 🟡 Beta       | Core functionality works and is tested. May have rough edges; API may change in minors.   |
| 🔴 Alpha      | Under active development. Functionality may be incomplete. Not recommended for production.|

> See [ROADMAP.md](ROADMAP.md) for the development timeline and graduation milestones for each module.
