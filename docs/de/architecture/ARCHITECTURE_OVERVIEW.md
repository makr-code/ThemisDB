# ThemisDB Architecture Overview

**Version:** v1.3.0  
**Date:** December 2025  
**Type:** Technical Architecture Documentation  
**Kategorie:** 🧩 Architecture

---

## 📑 Table of Contents

- [📋 Executive Summary](#-executive-summary)
- [🏗️ System Architecture](#️-system-architecture)
- [🔑 Core Components](#-core-components)

---

## 📋 Executive Summary

ThemisDB is a multi-model database system built on an LSM-tree foundation (RocksDB), providing unified access to relational, graph, vector, time-series, and document data models. The architecture emphasizes horizontal scalability, high performance through GPU acceleration, and enterprise-grade security.

### Key Architectural Characteristics

- **Multi-Model Unified Storage:** Single LSM-tree storage layer with specialized indexes
- **Horizontal Scalability:** VCC-URN based sharding with automatic rebalancing
- **High Performance:** GPU acceleration (10 backends), SIMD optimizations, query optimization
- **Enterprise Security:** Field-level encryption, HSM/PKI integration, RBAC, Apache Ranger
- **ACID Transactions:** MVCC with snapshot isolation, distributed SAGA patterns
- **Flexible Replication:** Leader-Follower and Multi-Master (CRDT-based)

---

## 🏗️ System Architecture

### Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Client Layer                            │
│  REST API │ GraphQL │ gRPC │ Wire Protocol │ Native SDKs   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   API & Server Layer                        │
│  HTTP Server │ Authentication │ Rate Limiting │ Load Shed  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Query Layer                              │
│  AQL Parser │ Query Optimizer │ Execution Engine           │
│  Function Libraries │ CTE Cache │ Semantic Cache           │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│              Transaction & Concurrency Layer                │
│  MVCC │ Transaction Manager │ SAGA Coordinator             │
│  Deadlock Detection │ WAL Management                       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Index Layer                               │
│  Vector (HNSW) │ Graph │ Secondary │ Spatial │ Fulltext   │
│  GPU Acceleration │ SIMD Optimization                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Storage Layer                             │
│  RocksDB (LSM-tree) │ Key Schema │ Compression            │
│  WAL │ Snapshot Management │ Compaction                   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│             Cross-Cutting Concerns                          │
│  Security │ Replication │ Sharding │ Monitoring │ CDC      │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔑 Core Components

### 1. Storage Layer

**Primary Technology:** RocksDB (LSM-tree based key-value store)

**Components:**
- **RocksDB Wrapper** (`src/storage/rocksdb_wrapper.cpp`)
  - Configurable LSM-tree parameters
  - Write-ahead logging (WAL)
  - Snapshot isolation support
  - Bloom filters for read optimization
  - Block cache management
  - Compaction strategies (level, universal)

**Key Design Decisions:**
- LSM-tree chosen for write-heavy workloads
- Hierarchical key schema enables multi-model support
- Compression (ZSTD, LZ4) reduces storage footprint
- Memory-mapped files for large datasets

**Performance Characteristics:**
- Write throughput: ~100K ops/sec (single node)
- Read latency: <1ms (cached), <10ms (disk)
- Compression ratio: 3-5x typical

---

### 2. Index Layer

**Purpose:** Specialized indexes for different data models and access patterns

#### Vector Index (HNSW)
**Implementation:** `src/index/vector_index.cpp` (57,750 LOC)

**Architecture:**
```
┌──────────────────────────────────────┐
│         HNSW Graph Structure         │
│                                      │
│  Layer 2: ○────○────○                │
│            │    │    │                │
│  Layer 1: ○────○────○────○           │
│           │ \  │  / │    │           │
│  Layer 0: ○─○─○─○─○─○─○─○─○          │
│         (Full connectivity)          │
└──────────────────────────────────────┘
```

**Features:**
- Multi-layer hierarchical graph
- Efficient k-NN search (O(log n) expected)
- GPU acceleration (10 backends)
- SIMD distance calculations
- Incremental index building
- Persistence to RocksDB

**Distance Metrics:**
- Cosine similarity
- Euclidean (L2)
- Dot product
- Manhattan (L1)
- Hamming

**Performance:**
- Index build: ~10K vectors/sec (CPU), ~100K vectors/sec (GPU)
- Query latency: <1ms for 1M vectors
- Recall@10: >95% at ef_search=100

#### Graph Index
**Implementation:** `src/index/graph_index.cpp` (65,471 LOC)

**Storage Format:**
```
Vertex Store:
  Key: vertex_id
  Value: {properties, edge_list}

Edge Store:
  Key: edge_id
  Value: {source, target, properties}

Index Store (for traversal):
  Key: vertex_id + edge_type
  Value: adjacent_vertices
```

**Algorithms:**
- BFS, DFS for traversal
- Dijkstra, A* for shortest path
- PageRank for importance ranking
- Louvain for community detection
- Betweenness centrality

**Optimizations:**
- Compressed adjacency lists
- Edge property caching
- Parallel graph algorithms
- GPU-accelerated PageRank

#### Secondary Indexes
**Implementation:** `src/index/secondary_index.cpp` (114,074 LOC)

**Index Types:**
- B-tree: Range queries, ordered scans
- Hash: Exact match queries
- Bitmap: Low-cardinality columns
- Full-text: BM25 ranking with stemming

**Features:**
- Composite indexes (multi-column)
- Partial indexes (WHERE clause)
- Expression indexes (computed columns)
- Covering indexes (index-only scans)

---

### 3. Query Layer

**Purpose:** Parse, optimize, and execute queries

#### AQL (Advanced Query Language)

**Query Processing Pipeline:**
```
AQL Query String
      ↓
[Lexer/Parser] → AST
      ↓
[Type Checker] → Typed AST
      ↓
[Optimizer] → Optimized Plan
      ↓
[Code Generator] → Execution Plan
      ↓
[Execution Engine] → Results
```

**Components:**

1. **Parser** (`aql_parser.cpp` - 43,972 LOC)
   - LL(k) grammar
   - Error recovery
   - AST generation

2. **Translator** (`aql_translator.cpp` - 70,388 LOC)
   - AST transformation
   - Type inference
   - Name resolution
   - Semantic validation

3. **Optimizer** (`query_optimizer.cpp` - 7,234 LOC)
   - Cost-based optimization
   - Rule-based transformations
   - Cardinality estimation
   - Join reordering
   - Predicate pushdown
   - Index selection

4. **Execution Engine** (`query_engine.cpp` - 47,658 LOC)
   - Volcano-style iterator model
   - Pipelined execution
   - Parallel execution (thread pool)
   - Memory management

**Optimization Techniques:**
- Predicate pushdown to storage layer
- Join reordering via dynamic programming
- Constant folding
- Common subexpression elimination
- Index-only scans
- Aggregate pushdown

**Performance:**
- Simple queries: <1ms
- Complex joins: <100ms (1M rows)
- Aggregations: 10M rows/sec (single core)

---

### 4. Transaction Layer

**Purpose:** ACID guarantees with MVCC

#### MVCC Implementation

**Version Chain Structure:**
```
Latest Version ← Older ← Oldest
    (v3)         (v2)    (v1)

Each version contains:
- Transaction ID (begin_ts, end_ts)
- Data snapshot
- Pointer to previous version
```

**Isolation Levels:**
- Read Uncommitted
- Read Committed
- Repeatable Read
- Serializable (SSI - Serializable Snapshot Isolation)

**Concurrency Control:**
- Optimistic concurrency control
- Write-write conflict detection
- Deadlock detection (wait-for graph)
- Lock-free reads

**Write-Ahead Logging (WAL):**
- ARIES protocol
- Physiological logging
- Checkpoint management
- Log replay for recovery

**Performance:**
- Transaction throughput: ~50K txn/sec (mixed workload)
- Commit latency: <5ms (sync WAL)
- Recovery time: <1 min for 1GB WAL

---

### 5. Sharding & Distribution

**Purpose:** Horizontal scalability

#### VCC-URN Sharding

**Architecture:**
```
Client Request
      ↓
[Shard Router] → Determines target shard(s)
      ↓
[Shard Manager] → Routes to appropriate shard
      ↓
[Shard Node 1] [Shard Node 2] ... [Shard Node N]
```

**Sharding Strategy:**
- Hash-based sharding (consistent hashing)
- Range-based sharding
- VCC-URN hierarchical partitioning
- Virtual nodes for load distribution

**Components:**

1. **Shard Router** (`shard_router.cpp`)
   - Request routing
   - Multi-shard aggregation
   - Transaction coordination

2. **Shard Manager** (`shard_manager.cpp`)
   - Shard allocation
   - Metadata management
   - Health monitoring

3. **Auto Rebalancer** (`shard_rebalancer.cpp`)
   - Load detection
   - Migration planning
   - Safety mechanisms
   - Zero-downtime migration

**Rebalancing Algorithm:**
```
1. Monitor shard metrics (load, storage, latency)
2. Detect imbalance (weighted scoring)
3. Generate migration plan
4. Execute migration (copy, sync, switch)
5. Verify and cleanup
```

#### Gossip Protocol

**Purpose:** Cluster membership and state sync

**Protocol:**
```
Every node periodically:
1. Select random peer
2. Exchange state information
3. Merge states (CRDTs)
4. Detect failures (timeout)
```

**State Synchronized:**
- Cluster membership
- Shard assignments
- Configuration
- Health status

**Convergence Time:** O(log n) rounds

---

### 6. Replication Layer

**Purpose:** Data redundancy and high availability

#### Replication Modes

**1. Leader-Follower (Async)**
```
Leader ──(WAL Stream)──> Follower 1
       └──(WAL Stream)──> Follower 2
```

**2. Multi-Master (CRDT)**
```
Node 1 ←──(State Sync)──→ Node 2
   ↕                         ↕
Node 3 ←──(State Sync)──→ Node 4
```

**Conflict Resolution:**
- Last-Write-Wins (LWW) with HLC timestamps
- CRDTs for commutative operations
- Application-defined merge functions
- Manual conflict resolution queue

**Consistency Guarantees:**
- Leader-Follower: Eventual consistency, read-your-writes (from leader)
- Multi-Master: Eventual consistency, causal consistency

**Failover:**
- Automatic leader election (Raft-based)
- Promotion of follower to leader
- Split-brain prevention (quorum)

---

### 7. Security Layer

**Purpose:** Data protection and access control

#### Encryption

**Field-Level Encryption:**
```
Plaintext Data
      ↓
[Encryption Key] ← [Key Provider]
      ↓                (Vault/HSM/PKI)
AES-256-GCM
      ↓
Encrypted Data → Storage
```

**Key Providers:**
- HashiCorp Vault (Transit engine)
- HSM (PKCS#11)
- PKI (Certificate-based)
- Local (testing only)

**Features:**
- Transparent encryption/decryption
- Key rotation with lazy re-encryption
- Per-field keys for granularity
- Deterministic encryption for indexing

#### Access Control

**RBAC (Role-Based Access Control):**
```
User ──→ Roles ──→ Permissions ──→ Resources
```

**Apache Ranger Integration:**
- Policy synchronization
- Fine-grained permissions
- Audit logging
- Dynamic policy updates

**Authentication:**
- JWT tokens (RS256, ES256)
- Multi-tenancy via claims
- Token expiry and refresh

---

### 8. Acceleration Layer

**Purpose:** GPU and CPU acceleration for vector operations

#### GPU Backend Architecture

```
┌──────────────────────────────────────┐
│      Backend Registry                │
│  (Selects optimal backend)           │
└──────────────────────────────────────┘
                 ↓
    ┌────────────┴────────────┐
    ↓            ↓            ↓
 [CUDA]      [Vulkan]      [OpenCL]
    ↓            ↓            ↓
 NVIDIA       Any GPU      Any GPU
```

**Backend Selection:**
- Runtime detection of available GPUs
- Capability matching (FP32, FP16, INT8)
- Fallback chain: GPU → SIMD CPU → Scalar CPU

**Operations Accelerated:**
- Vector distance calculations
- Matrix multiplication
- Index building (HNSW)
- Aggregations
- Sorting

**Performance Gains:**
- Distance calculations: 10-50x speedup
- HNSW index build: 10x speedup
- Batch queries: 20-40x speedup

---

### 9. Analytics Layer

**Purpose:** OLAP and complex event processing

#### OLAP Engine

**Columnar Storage:**
```
Row Store:          Columnar Store:
┌──┬──┬──┬──┐      ┌────────┐
│id│a │b │c │      │1│2│3│4 │ (id)
├──┼──┼──┼──┤      ├────────┤
│1 │x │y │z │  →   │x│p│m│t │ (a)
├──┼──┼──┼──┤      ├────────┤
│2 │p │q │r │      │y│q│n│u │ (b)
└──┴──┴──┴──┘      └────────┘
```

**Query Optimization:**
- Column pruning
- Predicate pushdown
- Late materialization
- SIMD vectorized scans

**Window Functions:**
- Partitioning
- Ordering
- Frame specification (ROWS, RANGE)
- Aggregates (SUM, AVG, MIN, MAX)
- Ranking (ROW_NUMBER, RANK, DENSE_RANK)
- Offset (LAG, LEAD)

#### CEP Engine

**Event Pattern Matching:**
```
PATTERN: A → B → C WITHIN 10 seconds
WHERE A.value > 100 AND C.value < 50

Detection:
Events: A₁(t₀) → B₁(t₁) → C₁(t₂)
Match: ✓ (t₂ - t₀ < 10s, conditions met)
```

**Features:**
- Sliding windows
- Tumbling windows
- Session windows
- Pattern matching (regex-like)
- Event correlation

---

### 10. Content Processing

**Purpose:** Extract metadata and content from various file formats

#### Processing Pipeline

```
File Upload
     ↓
[MIME Detection]
     ↓
[Format Router]
     ↓
┌────┴────┬────────┬────────┬────────┐
│  PDF    │ Office │ Image  │ Video  │
└────┬────┴────────┴────────┴────────┘
     ↓
[Content Extraction]
     ↓
[Metadata Extraction]
     ↓
[Indexing]
     ↓
Storage
```

**Processors:**
- PDF: Text, tables, images
- Office: Word, Excel, PowerPoint
- Images: EXIF, OCR, thumbnails
- Video/Audio: Metadata, thumbnails
- CAD: 3D models, metadata
- Geo: Shapefiles, GeoJSON

**Plugin System:**
- Dynamic loading
- Format extension
- Custom processors

---

## 📊 Performance Characteristics

### Throughput

| Operation | Throughput | Notes |
|-----------|------------|-------|
| Writes | 100K ops/sec | Single node, WAL enabled |
| Reads | 500K ops/sec | Cached data |
| Vector Search | 10K qps | 1M vectors, k=10 |
| Graph Traversal | 1M edges/sec | BFS |
| Transactions | 50K txn/sec | Mixed workload |

### Latency

| Operation | Latency (p50) | Latency (p99) |
|-----------|---------------|---------------|
| Point Read | 0.5ms | 2ms |
| Point Write | 1ms | 5ms |
| Vector kNN | 2ms | 10ms |
| Simple Query | 5ms | 20ms |
| Complex Join | 50ms | 200ms |

### Scalability

| Metric | Single Node | 10 Nodes | 100 Nodes |
|--------|-------------|----------|-----------|
| Write Throughput | 100K/s | 900K/s | 8M/s |
| Data Size | 1TB | 10TB | 100TB |
| Query Latency | 10ms | 12ms | 15ms |

---

## 🔒 Security Architecture

### Defense in Depth

```
Layer 1: Network (TLS, mTLS)
Layer 2: Authentication (JWT, PKI)
Layer 3: Authorization (RBAC, Ranger)
Layer 4: Encryption (Field-level AES-256)
Layer 5: Audit (Comprehensive logging)
```

### Compliance

- **GDPR:** PII detection, pseudonymization, right to erasure
- **HIPAA:** Encryption at rest/transit, audit logging
- **SOC 2:** Access controls, monitoring, incident response
- **eIDAS:** Qualified signatures via PKI

---

## 🔍 Monitoring & Observability

### Metrics Collection

**Prometheus Metrics:**
- Request rates, latencies, errors
- Resource utilization (CPU, memory, disk)
- Cache hit rates
- Transaction throughput
- Replication lag

**OpenTelemetry Tracing:**
- Distributed traces across services
- Query execution breakdown
- Index operation timing
- Network latency

**Logging:**
- Structured JSON logging
- Log levels: DEBUG, INFO, WARN, ERROR
- Audit logs for security events
- Query logs for performance analysis

---

## 🚀 Deployment Architecture

### Single Node

```
┌─────────────────────────┐
│    ThemisDB Server      │
│  ┌─────────────────┐    │
│  │ All Components  │    │
│  └─────────────────┘    │
│         Storage         │
└─────────────────────────┘
```

**Use Case:** Development, small workloads

### Clustered

```
┌──────────┐  ┌──────────┐  ┌──────────┐
│ Shard 1  │  │ Shard 2  │  │ Shard 3  │
│ (Leader) │  │ (Leader) │  │ (Leader) │
└────┬─────┘  └────┬─────┘  └────┬─────┘
     │             │             │
┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐
│Follower 1│  │Follower 2│  │Follower 3│
└──────────┘  └──────────┘  └──────────┘
```

**Use Case:** Production, high availability

### Cloud Native

```
┌──────────────────────────────────┐
│      Load Balancer               │
└────────────┬─────────────────────┘
             ↓
┌────────────────────────────────┐
│   Kubernetes Cluster           │
│  ┌──────┐ ┌──────┐ ┌──────┐   │
│  │ Pod1 │ │ Pod2 │ │ Pod3 │   │
│  └──────┘ └──────┘ └──────┘   │
└────────────────────────────────┘
             ↓
┌────────────────────────────────┐
│   Persistent Storage (EBS/PD)  │
└────────────────────────────────┘
```

**Use Case:** Cloud deployments, auto-scaling

---

## 📚 Technology Stack

### Core Dependencies

| Component | Technology | Version |
|-----------|-----------|---------|
| Storage | RocksDB | 8.x |
| GPU Compute | CUDA | 12.x |
| GPU Compute | Vulkan | 1.3 |
| Networking | Boost.Asio | 1.82 |
| JSON | nlohmann/json | 3.11 |
| HTTP | cpp-httplib | 0.14 |
| Compression | ZSTD | 1.5 |
| Logging | spdlog | 1.12 |

### Build System

- **CMake** 3.20+
- **vcpkg** for dependency management
- **C++20** standard
- **Compiler:** GCC 11+, MSVC 2022+, Clang 14+

---

## 🎯 Design Principles

1. **Performance First:** Optimize hot paths, use zero-copy where possible
2. **Horizontal Scalability:** Design for distribution from day one
3. **Operational Simplicity:** Minimize manual intervention, automate operations
4. **Security by Default:** Encryption, authentication, authorization built-in
5. **Extensibility:** Plugin system for custom extensions
6. **Observability:** Comprehensive metrics and tracing
7. **Fault Tolerance:** Graceful degradation, automatic recovery

---

**Document Version:** 1.0.1  
**Last Updated:** April 2026  
**Maintained By:** ThemisDB Architecture Team
