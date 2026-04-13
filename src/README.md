# ThemisDB Source Code

This directory contains the core implementation of ThemisDB's multi-model database system.

## Directory Structure

### Core Infrastructure

- **base/** - Foundational abstractions, module system, and WASM sandbox
- **config/** - Configuration management, hot-reload, and schema validation
- **core/** - Core database runtime and adapter orchestration
- **themis/** - Core ThemisDB orchestration layer and wire protocol
- **utils/** - Shared utility functions and helpers (UUID, codecs, PKI)

### Storage Layer

- **storage/** - RocksDB wrapper, MVCC, WAL, backup/recovery, columnar format
- **timeseries/** - Time series data management and Gorilla compression
- **temporal/** - Temporal and bitemporal data support
- **cache/** - Semantic caching, query result caching, LRU/LIRS eviction
- **metadata/** - Schema introspection, catalog, and statistics

### Query Engine

- **query/** - AQL optimizer, cost-based planner, and execution engine
- **aql/** - AQL language engine and multi-paradigm query translation
- **index/** - HNSW vector, R-tree, adaptive, and secondary indexes
- **search/** - Full-text and hybrid search

### API & Server

- **api/** - HTTP/GraphQL API server implementation
- **server/** - Main server components and HTTP handlers
- **network/** - Network layer, wire protocol, and peer communication

### Security & Auth

- **auth/** - Authentication and authorization (JWT, RBAC, LDAP, SSO/MFA)
- **security/** - Encryption, key management, and PKI integration

### Analytics & Performance

- **analytics/** - OLAP pipeline, CEP, ML serving, and streaming analytics
- **performance/** - Benchmarking, lock-free data structures, and profiling
- **observability/** - Metrics, distributed tracing, and logging infrastructure
- **gpu/** - GPU compute integration and kernel management
- **acceleration/** - Hardware acceleration (CUDA, Vulkan, NPU dispatch)

### Data Pipeline

- **content/** - Content ingestion, processing, and embedding pipelines
- **ingestion/** - Data ingestion adapters and batch loaders
- **importers/** - Data import connectors (PostgreSQL, MySQL, MongoDB, Kafka)
- **exporters/** - Data export functionality (JSONL, LLM formats, Parquet)
- **cdc/** - Change Data Capture (CDC) and changefeed implementation
- **updates/** - Schema and binary update management
- **maintenance/** - Centralized database maintenance orchestration (cron scheduling, windows, health reporting)
- **scheduler/** - Task and job scheduling (cron-like automation)

### Distribution & Resilience

- **sharding/** - Horizontal scaling and sharding implementation
- **replication/** - Raft-based replication and WAL archival
- **transaction/** - SAGA pattern and distributed transaction management
- **failover/** - Automatic failover orchestration and disaster recovery
- **chaos/** - Fault injection and deterministic chaos scheduling

### AI & LLM

- **llm/** - LLM interaction storage, chain-of-thought, and LoRA management
- **rag/** - Retrieval-Augmented Generation pipeline
- **prompt_engineering/** - LLM prompt management and optimization
- **training/** - ML model training integration and provenance tracking

### Specialized

- **geo/** - Geospatial query processing and indexing
- **graph/** - Property graph queries and traversal
- **chimera/** - Hybrid multi-model layer (ThemisDB/MongoDB/Qdrant/Neo4j adapter)
- **governance/** - Policy engine and compliance governance
- **process/** - BPMN 2.0/EPK/VCC-VPB process model management, LLM descriptors, Graph-RAG
- **voice/** - Voice query interface for natural language interaction

### Plugins

- **plugins/** - Plugin system infrastructure and registry
- **llama_cpp/** - llama.cpp LLM backend plugin
- **whisper/** - Whisper audio transcription plugin
- **stable_diffusion/** - Stable Diffusion image generation plugin
- **user_storage_encrypted/** - Per-user encrypted blob storage plugin
- **onnx_clip/** - ONNX CLIP plugin for image/text embeddings
- **rpc_grpc/** - gRPC RPC plugin
- **ethics_ai/** - AI ethics evaluation and bias detection

## Building

See the main [README.md](../README.md) for build instructions.

## Architecture

For detailed architecture documentation, see:
- [Architecture Overview](../docs/architecture.md)
- [Source Code Documentation](../docs/src/README.md)

---

## Overall System Status

ThemisDB has **42 Production-grade modules** and **13 Beta modules** across its source tree. The entire core data path and all AI/LLM layers are production-ready.

| Tier       | Count | Modules                                                                                                                                                                               |
|------------|-------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Production | 42    | 42 of 55 modules — all except the 13 Beta modules listed below                                                                                                                       |
| Beta       | 13    | `ethics_ai`, `geo`, `gpu`, `llama_cpp`, `plugins`, `process`, `rag`, `security`, `sharding`, `stable_diffusion`, `timeseries`, `transaction`, `whisper`                             |

---

## Production Readiness Matrix

| Module              | Status               | Description                                          |
|---------------------|----------------------|------------------------------------------------------|
| acceleration        | 🟢 Production        | GPU/hardware acceleration (CUDA, Vulkan)             |
| analytics           | 🟢 Production        | Analytical query pipeline                            |
| api                 | 🟢 Production        | HTTP API server implementation                       |
| aql                 | 🟢 Production        | AQL language engine, multi-paradigm queries          |
| auth                | 🟢 Production        | JWT, RBAC, enterprise SSO/MFA                        |
| base                | 🟢 Production        | Foundational abstractions                            |
| cache               | 🟢 Production        | Semantic and query result caching                    |
| cdc                 | 🟢 Production        | Change Data Capture and changefeeds                  |
| chaos               | 🟢 Production        | Fault injection and deterministic chaos scheduling   |
| chimera             | 🟢 Production        | Hybrid multi-model layer                             |
| config              | 🟢 Production        | Configuration management                             |
| content             | 🟢 Production        | Content ingestion and processing pipelines           |
| core                | 🟢 Production        | Core database runtime                                |
| ethics_ai           | 🟡 Beta              | AI ethics evaluation and bias detection              |
| exporters           | 🟢 Production        | Data export (JSONL, LLM formats)                     |
| failover            | 🟢 Production        | Automatic failover orchestration and disaster recovery |
| geo                 | 🟡 Beta              | Geospatial query processing and indexing             |
| governance          | 🟢 Production        | Policy engine and compliance governance              |
| gpu                 | 🟡 Beta              | GPU compute integration                              |
| graph               | 🟢 Production        | Property graph queries and traversal                 |
| importers           | 🟢 Production        | Data import (PostgreSQL, etc.)                       |
| index               | 🟢 Production        | HNSW, R-tree, adaptive indexing                      |
| ingestion           | 🟢 Production        | Data ingestion pipeline                              |
| llm                 | 🟢 Production        | LLM interaction storage and chain-of-thought         |
| llama_cpp           | 🟡 Beta              | llama.cpp LLM backend plugin                         |
| maintenance         | 🟢 Production        | Centralized DB maintenance orchestration             |
| metadata            | 🟢 Production        | Metadata management and catalog                      |
| network             | 🟢 Production        | Network layer and peer communication                 |
| observability       | 🟢 Production        | Metrics, tracing, and logging infrastructure         |
| onnx_clip           | 🟢 Production        | ONNX CLIP plugin for image/text embeddings           |
| performance         | 🟢 Production        | Benchmarking and performance optimization            |
| plugins             | 🟡 Beta              | Plugin system infrastructure                         |
| process             | 🟡 Beta              | BPMN/EPK/VCC-VPB process modeling, Graph-RAG         |
| prompt_engineering  | 🟢 Production        | LLM prompt management                                |
| query               | 🟢 Production        | AQL optimizer, cost-based planner, execution engine  |
| rag                 | 🟡 Beta              | Retrieval-Augmented Generation pipeline              |
| replication         | 🟢 Production        | Raft-based replication                               |
| rpc_grpc            | 🟢 Production        | gRPC RPC plugin                                      |
| scheduler           | 🟢 Production        | Task and job scheduling                              |
| search              | 🟢 Production        | Full-text and hybrid search                          |
| security            | 🟡 Beta              | Encryption, key management, PKI integration          |
| server              | 🟢 Production        | Main server components and API handlers              |
| sharding            | 🟡 Beta              | Horizontal scaling and sharding                      |
| stable_diffusion    | 🟡 Beta              | Stable Diffusion image generation plugin             |
| storage             | 🟢 Production        | RocksDB wrapper, MVCC, backup/recovery               |
| temporal            | 🟢 Production        | Temporal and bitemporal data support                 |
| themis              | 🟢 Production        | Core ThemisDB orchestration layer                    |
| timeseries          | 🟡 Beta              | Time series data management and compression          |
| training            | 🟢 Production        | ML model training integration                        |
| transaction         | 🟡 Beta              | SAGA pattern and distributed transactions            |
| updates             | 🟢 Production        | Schema and data update management                    |
| user_storage_encrypted | 🟢 Production     | Per-user encrypted blob storage plugin               |
| utils               | 🟢 Production        | Shared utility functions and helpers                 |
| voice               | 🟢 Production        | Voice query interface                                |
| whisper             | 🟡 Beta              | Whisper audio transcription plugin                   |

---

## Maturity Levels

> **Source of truth:** The `Maturity Level` field in each source file header is the authoritative status for that file. The table above reflects the overall module status derived from those headers.

| Level             | Meaning                                                                                   |
|-------------------|-------------------------------------------------------------------------------------------|
| 🟢 Production     | Feature-complete, thoroughly tested, used in production deployments. API is stable.       |
| 🟡 Beta           | Core functionality works and is tested. May have rough edges; API may change in minors.   |
| 🔴 Alpha          | Under active development. Functionality may be incomplete. Not recommended for production.|

> See [ROADMAP.md](ROADMAP.md) for the development timeline and graduation milestones for each module.
