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
- **maintenance/** - Centralized database maintenance orchestration (cron scheduling, windows, health reporting)
- **plugins/** - Plugin system infrastructure
- **process/** - BPMN 2.0/EPK/VCC-VPB process model management, LLM descriptors, Graph-RAG
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

ThemisDB has **43 Production-grade modules**, **4 Beta modules**, and **3 Alpha modules** across its source tree. Core data paths are production-ready while selected emerging modules are still hardening.

| Tier              | Count | Modules                                                                      |
|-------------------|-------|------------------------------------------------------------------------------|
| Production        | 43    | 43 of 50 modules                                                             |
| Beta              | 4     | `chimera`, `gpu`, `process`, `sharding`                                      |
| Alpha             | 3     | `ethics_ai`, `onnx_clip`, `rpc_grpc`                                         |

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
| chimera             | 🟡 Beta              | Hybrid multi-model benchmark adapters (simulation complete, production driver wiring pending) |
| config              | 🟢 Production        | Configuration management                             |
| content             | 🟢 Production        | Content ingestion and processing pipelines           |
| core                | 🟢 Production        | Core database runtime                                |
| exporters           | 🟢 Production        | Data export (JSONL, LLM formats)                     |
| ethics_ai           | 🔴 Alpha             | Ethical reasoning plugin skeleton and discourse engine |
| geo                 | 🟢 Production        | Geospatial query processing and indexing             |
| governance          | 🟢 Production        | Policy engine and compliance governance              |
| gpu                 | 🟡 Beta              | GPU compute integration and backend hardening        |
| graph               | 🟢 Production        | Property graph queries and traversal                 |
| importers           | 🟢 Production        | Data import (PostgreSQL, etc.)                       |
| index               | 🟢 Production        | HNSW, R-tree, adaptive indexing                      |
| ingestion           | 🟢 Production        | Data ingestion pipeline                              |
| llm                 | 🟢 Production        | LLM interaction storage and chain-of-thought         |
| maintenance         | 🟢 Production        | Centralized DB maintenance orchestration             |
| metadata            | 🟢 Production        | Metadata management and catalog                      |
| network             | 🟢 Production        | Network layer and peer communication                 |
| observability       | 🟢 Production        | Metrics, tracing, and logging infrastructure         |
| onnx_clip           | 🔴 Alpha             | ONNX CLIP image-analysis backend plugin              |
| performance         | 🟢 Production        | Benchmarking and performance optimization            |
| plugins             | 🟢 Production        | Plugin system infrastructure                         |
| process             | 🟡 Beta              | BPMN/EPK/VCC-VPB process modeling, Graph-RAG         |
| prompt_engineering  | 🟢 Production        | LLM prompt management                                |
| query               | 🟢 Production        | AQL optimizer, cost-based planner, execution engine  |
| rag                 | 🟢 Production        | Retrieval-Augmented Generation pipeline              |
| replication         | 🟢 Production        | Raft-based replication                               |
| rpc_grpc            | 🔴 Alpha             | Pluggable gRPC RPC server module (mTLS-capable)      |
| scheduler           | 🟢 Production        | Task and job scheduling                              |
| search              | 🟢 Production        | Full-text and hybrid search                          |
| security            | 🟢 Production        | Encryption, key management, PKI integration          |
| server              | 🟢 Production        | Main server components and API handlers              |
| sharding            | 🟡 Beta              | Horizontal scaling and sharding                      |
| storage             | 🟢 Production        | RocksDB wrapper, MVCC, backup/recovery               |
| temporal            | 🟢 Production        | Temporal and bitemporal data support                 |
| themis              | 🟢 Production        | Core ThemisDB orchestration layer                    |
| timeseries          | 🟢 Production        | Time series data management and compression          |
| training            | 🟢 Production        | ML model training integration                        |
| transaction         | 🟢 Production        | SAGA pattern and distributed transactions            |
| updates             | 🟢 Production        | Schema and data update management                    |
| user_storage_encrypted | 🟢 Production     | Encrypted user storage backend (gocryptfs + key rotation) |
| utils               | 🟢 Production        | Shared utility functions and helpers                 |
| voice               | 🟢 Production        | Voice query interface                                |

---

## Maturity Levels

> **Source of truth:** The `Maturity Level` field in each source file header is the authoritative status for that file. The table above reflects the overall module status derived from those headers.

| Level                  | Meaning                                                                                   |
|------------------------|-------------------------------------------------------------------------------------------|
| 🟢 Production          | Feature-complete, thoroughly tested, used in production deployments. API is stable.       |
| 🟡 Beta                | Core functionality works and is tested. May have rough edges; API may change in minors.   |
| 🔴 Alpha               | Under active development. Functionality may be incomplete. Not recommended for production.|

> See [ROADMAP.md](ROADMAP.md) for the development timeline and graduation milestones for each module.

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/README.md/README.md`](../../include/README.md/README.md) for the public API.
