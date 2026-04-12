# Appendix H: Glossary & Terminology Reference

> "Every domain has its jargon. Understand the terminology, understand the system."

---

## H.1 Core Database Concepts

### Base Entity
The fundamental unit of data in ThemisDB. Combines relational, document, graph, vector, and time-series aspects in a single unified structure.

**Example:**
```json
{
  "_id": "users/alice",
  "_key": "alice",
  "_rev": 5,
  "name": "Alice",
  "email": "alice@example.com",
  "created_at": "2025-01-01T10:00:00Z",
  "embedding": [0.1, -0.2, 0.3, ...],
  "graph_edges": ["users/bob", "users/charlie"]
}
```

### Collection
A group of related Base Entities (similar to a SQL table, but more flexible).

**Types:**
- **Relational:** users, orders, products
- **Document:** articles, posts, documents
- **Graph:** relationships, friendships
- **Vector:** embeddings, semantic data

### Transaction
An atomic unit of work. Either all operations succeed (COMMIT) or all rollback. Provides ACID guarantees.

```aql
BEGIN
  INSERT {...} INTO users
  UPDATE {...} IN orders
COMMIT
```

### ACID Guarantees
- **Atomicity:** All or nothing
- **Consistency:** Data integrity maintained
- **Isolation:** No dirty reads/writes
- **Durability:** Persisted to disk

---

## H.2 Query Language (AQL)

### AQL (Adaptive Query Language)
Unified query language for all data models in ThemisDB. Combines SQL (relational), graph traversal, document processing, and vector search.

**Core Constructs:**
- FOR ... IN: Iteration
- FILTER: Conditional selection
- LET: Variable binding
- COLLECT: Grouping/aggregation
- SORT: Ordering
- LIMIT: Result limiting
- RETURN: Output specification

### Bind Variables
Parameterized query parameters (prevent injection attacks).

```aql
FOR user IN users
  FILTER user.email == @email
  RETURN user
```

Run with: `db.query(query, bind_vars={'email': 'alice@example.com'})`

### Query Plan
Execution strategy optimized by the query optimizer.

**Components:**
- **Collection Scan:** Read all documents (slow)
- **Index Seek:** Use index to find matching rows (fast)
- **Filter:** Apply WHERE conditions
- **Sort:** Order results
- **Aggregation:** GROUP BY operations

---

## H.3 Indexing & Performance

### Index
Data structure enabling fast lookups. Trade: space for speed.

**Types:**

| Type | Ordered | Point Lookup | Range Query | Size |
|------|---------|--------------|-------------|------|
| BTree | ✓ | Fast | Fast | Large |
| Hash | ✗ | Very Fast | Slow | Medium |
| Skiplist | ✓ | Fast | Fast | Medium |
| Inverted | ✗ | Fast | N/A | Large |

### Query Cardinality
Estimated number of rows matching query conditions.

```
Low cardinality (< 1% of docs):
  → Use index for fast filtering
  
High cardinality (> 50% of docs):
  → Collection scan might be faster (no index overhead)
```

### Selectivity
How well an index reduces result set.

```
Selectivity = Matching rows / Total rows

High selectivity (< 5%):
  → Use index

Low selectivity (> 50%):
  → Collection scan faster
```

---

## H.4 Replication & Consistency

### Primary (Master)
Single database node accepting writes. Log changes to replicas.

### Follower (Replica)
Read-only copy of primary. Applies changes asynchronously from WAL.

### Replication Lag
Delay between write on primary and replica visibility.

```
Write at Primary: T0
Arrive at Follower: T0 + lag_ms
Visible to read: T0 + lag_ms + apply_time
```

**Acceptable lag:**
- Strongly consistent: 0ms (no lag)
- Eventual consistent: < 5 seconds
- Archive: minutes to hours

### Write-Ahead Log (WAL)
Durability mechanism. All writes logged before applied.

```
Client write:
  1. Write to WAL on disk
  2. Apply to in-memory database
  3. Ack to client
  
On crash:
  1. Replay WAL on startup
  2. Resume from last committed state
```

### Quorum Write
Write acknowledged only after reaching majority of replicas.

```
3-node cluster:
  Quorum = (3 + 1) / 2 = 2 nodes

Write must be on Primary + 1 Follower before ack
```

---

## H.5 Vector Search Concepts

### Embedding
Dense vector representation of unstructured data.

```python
"Alice is an engineer" 
  → [0.1, -0.2, 0.3, 0.15, -0.08, ...]  # 384 dimensions
```

### Similarity
Measure of how close two vectors are.

| Metric | Range | Best For |
|--------|-------|----------|
| Cosine | [-1, 1] | Normalized embeddings (angles) |
| Euclidean | [0, ∞) | Raw embeddings (distances) |
| Dot Product | (-∞, ∞) | Unnormalized embeddings |

### Nearest Neighbor Search
Find K closest vectors to a query vector.

```aql
FOR v IN vectors
  FILTER DISTANCE(v.embedding, @query) < @threshold
  SORT DISTANCE(v.embedding, @query)
  LIMIT @top_k
  RETURN v
```

### HNSW (Hierarchical Navigable Small Worlds)
Approximate nearest neighbor index algorithm.

**Parameters:**
- **M:** Connections per node (16-64, default 16)
- **ef_construct:** Quality during index building
- **ef_search:** Quality during searches

---

## H.6 Graph Concepts

### Node (Vertex)
Entity in a graph. Represents an object or concept.

### Edge (Relationship)
Connection between two nodes. Directed or undirected.

### Graph Traversal
Following edges to discover related nodes.

```
Alice ← (follows) ← Bob ← (follows) ← Charlie
            |                           |
            └────────────────┬──────────┘
                             ▼
                     (mutual connections)
```

### Path
Sequence of nodes and edges.

```
Shortest path from Alice to Charlie:
  Alice --(follows)-- Bob --(follows)-- Charlie
  Length: 2 hops
```

### Centrality
Measure of node importance.

**Types:**
- **Degree:** Number of connections
- **Betweenness:** How often on shortest paths
- **Closeness:** Average distance to others
- **PageRank:** Voting-based importance

---

## H.7 Time-Series Concepts

### Time-Series Data
Ordered sequence of measurements over time.

```
Metric: CPU Usage
Timestamp 1: 45%
Timestamp 2: 52%
Timestamp 3: 48%
...
```

### Bucket / Time Window
Grouping data into time periods.

```aql
FOR metric IN metrics
  COLLECT hour = DATE_FORMAT(metric.timestamp, '%yyyy-%mm-%dd %hh:00')
  AGGREGATE avg = AVG(metric.value)
  RETURN { hour, avg }
```

### Downsampling
Reducing time-series resolution (e.g., minute → hour).

```
High resolution: Every second (86,400 points/day)
Low resolution: Every hour (24 points/day)
Compression: 99.97% reduction
```

---

## H.8 Operational Concepts

### SLA (Service Level Agreement)
Contractual commitment on availability and performance.

```
SLA: "99.9% uptime with p99 latency < 200ms"
  → Downtime allowed: 43.2 minutes/month
  → 1 in 1000 queries can exceed 200ms
```

### SLI (Service Level Indicator)
Measurable metric of actual performance.

```
SLI: Actual uptime = 99.92%
SLI: Actual p99 latency = 187ms
```

### Error Budget
Acceptable downtime/errors for SLA.

```
SLA: 99.95% uptime
Error budget = 0.05% = 21.6 minutes/month

Used: 15 minutes
Remaining: 6.6 minutes (use for deployments/maintenance)
```

### MTTR (Mean Time To Recovery)
Average time to fix a problem.

```
Incident starts: 10:00
Recovery complete: 10:15
MTTR = 15 minutes
```

### MTTF (Mean Time To Failure)
Average time before next failure.

```
Downtime: 15 minutes
Recovery: 30 minutes (MTTR)
Uptime: 30 days until next failure
MTTF = 30 days
```

---

## H.9 Security Concepts

### RBAC (Role-Based Access Control)
Access control based on user roles.

```
Role: 'data_analyst'
Permissions:
  - READ from analytics_*
  - EXECUTE report queries
  - NO WRITE/DELETE
```

### JWT (JSON Web Token)
Stateless authentication token.

```
Header: { alg: "HS256", typ: "JWT" }
Payload: { sub: "alice", exp: 1726000000 }
Signature: HMAC-SHA256(header + payload, secret)
```

### Encryption at Rest
Data encrypted on disk.

```
File on disk: [encrypted bytes]
In memory: Clear text (needed for processing)
Key location: HSM (Hardware Security Module)
```

### Encryption in Transit
Data encrypted while traveling network.

```
TLS 1.3 + Perfect Forward Secrecy
  → Even if long-term key compromised, old traffic safe
```

### MFA (Multi-Factor Authentication)
Multiple authentication mechanisms.

```
1. Password (something you know)
2. TOTP (something you have) - app code
3. WebAuthn (something you are) - fingerprint
```

---

## H.10 Architecture Concepts

### Monolithic Architecture
All components in single process. Easy to develop, hard to scale.

### Microservices
Each service independent. Scales well, operational complexity.

### ThemisDB: Hybrid Approach
- **Monolithic Core:** Single database engine
- **Distributed:** Horizontal sharding for scale-out
- **Multi-Model:** All data models in one engine (no microservices)

### Horizontal Scaling
Add more nodes (scale out). Divide data via sharding.

```
1 node (10TB limit)
  ↓
2 nodes (5TB each)
  ↓
4 nodes (2.5TB each)
```

### Vertical Scaling
Upgrade hardware (scale up). More RAM/CPU per node.

```
32GB RAM
  ↓
64GB RAM
```

---

## H.11 Monitoring & Observability

### Metrics
Quantitative measurements (numbers).

```
CPU usage: 45%
Memory: 8.2 GB
Queries per second: 1,200
Latency p99: 185ms
```

### Logs
Textual records of events.

```
2025-01-01T10:00:00Z [INFO] Query executed: SELECT * FROM users
2025-01-01T10:00:01Z [WARN] Query latency: 245ms > threshold 200ms
2025-01-01T10:00:02Z [ERROR] Connection timeout from client 1.2.3.4
```

### Traces
Request flow through system (distributed tracing).

```
User Request
  ├─ API Gateway (5ms)
  ├─ Authentication (12ms)
  ├─ Database Query (185ms)
  │   ├─ Query Plan (2ms)
  │   ├─ Index Seek (80ms)
  │   ├─ Filter (50ms)
  │   └─ Return (53ms)
  └─ Response (3ms)
Total: 205ms
```

### Cardinality
Number of unique values.

```
High cardinality: User IDs (millions)
Low cardinality: Status (10 values)
```

---

## H.12 Alphabetical Glossary

**API:** Application Programming Interface - interface for programmatic access

**ACID:** Atomicity, Consistency, Isolation, Durability guarantees

**AQL:** Adaptive Query Language - ThemisDB's query language

**Backup:** Copy of data for recovery

**Batch:** Group of operations processed together

**Cardinality:** Number of distinct values

**Cache:** Fast memory storage layer

**Changefeed:** Stream of database changes

**Collection:** Group of related entities

**Consistency:** Data integrity maintained across system

**Continuous Batching:** Dynamic request batching technique for LLM inference that allows new requests to join active batches, dramatically improving throughput (176%) and reducing latency (57%) compared to static batching. See Chapter 20.9A.3.

**Cursor:** Pointer to result set for iteration

**Denormalization:** Storing redundant data for performance

**Document:** Semi-structured data (JSON-like)

**DSGVO:** German data protection regulation (GDPR equivalent)

**Edge:** Connection in graph

**Embedding:** Vector representation of data

**Failover:** Automatic switch to backup system

**Flash Attention:** IO-aware attention mechanism for LLMs that uses SRAM tiling instead of HBM storage, reducing GPU memory usage by 37% and increasing throughput by 69%. Requires NVIDIA Ampere+ GPUs. See Chapter 20.9A.1.

**Flush:** Write data from memory to disk

**Garbage Collection:** Freeing unused memory

**GBNF (GGML BNF):** Grammar notation used by llama.cpp for constrained generation. Extends EBNF with specific syntax for controlling LLM output format. See Grammar-Constrained Generation.

**Grammar-Constrained Generation:** LLM technique that uses EBNF/GBNF grammar rules to guarantee syntactically valid outputs (JSON, XML, CSV). Achieves 95-99% success rate vs 60-70% without constraints, eliminating need for output validation and retries. See Chapter 17.12.6.

**Graph:** Network of connected nodes

**Heap:** Memory area for dynamic allocation

**HNSW:** Hierarchical Navigable Small Worlds - vector index

**Hot Data:** Frequently accessed data

**Hot Spare:** Fully configured standby node in a database cluster that can automatically take over (failover) when an active shard fails, typically within <5 seconds. Provides high availability with zero data loss when combined with WAL replication. See Chapter 16.10.1.

**Index:** Data structure for fast lookups

**Ingestion:** Loading data into database

**Inverted Index:** Index mapping values to documents

**Isolation:** Transactions don't interfere

**JIT:** Just-In-Time compilation

**JSON:** JavaScript Object Notation

**JWT:** JSON Web Token - stateless auth

**Keyspace:** Logical division of data

**Latency:** Time delay for operation

**LoRA (Low-Rank Adaptation):** Efficient fine-tuning technique for large language models that adds trainable low-rank matrices to pretrained models, reducing memory requirements by 99% and training time by 3-10x compared to full fine-tuning. Multiple LoRA adapters can run on a single base model. See Chapter 17.12.5.

**LSM:** Log-Structured Merge tree

**MVCC:** Multi-Version Concurrency Control

**Normalization:** Organizing data to reduce redundancy

**Node:** Entity in graph or cluster

**OLAP:** Online Analytical Processing

**OLTP:** Online Transaction Processing

**Optimization:** Making something run faster

**Paged Attention:** Memory management technique for LLM attention mechanisms that organizes KV-cache into fixed-size pages instead of continuous memory allocation, reducing GPU memory waste by 80% and increasing concurrent request capacity by 5x. See Chapter 17.12.4.

**Pagination:** Dividing results into pages

**Partition:** Division of data across nodes

**Percentile:** Value below which percentage falls

**Persistence:** Data survives shutdown

**Piper:** Fast, local neural Text-to-Speech (TTS) engine used in ThemisDB Voice Assistant. Provides natural-sounding voice synthesis in multiple languages with <50ms latency and minimal CPU usage. See Chapter 10.7.

**Prefix Caching:** LLM optimization that caches the attention states of frequently used prompt prefixes (such as system prompts), enabling 75% cost savings and 95% latency reduction for repeated queries with common prompt beginnings. See Chapter 17.12.1.

**Projection:** Selecting subset of columns

**Query Plan:** Execution strategy for query

**Quorum:** Majority consensus

**Range:** Ordered selection of values

**Replica:** Copy of data

**Replication:** Process of copying data to replicas

**Response Caching:** Intelligent caching system for LLM responses that uses embedding-based semantic similarity to identify and reuse answers to similar questions, providing 60-80% cost savings for repetitive queries. Supports configurable TTL and similarity thresholds. See Chapter 17.12.2.

**RocksDB:** Embedded key-value store (ThemisDB storage engine)

**Rollback:** Undo transaction

**RoPE (Rotary Position Embedding):** Position encoding technique for transformer models that enables context window extension beyond training length through scaling. ThemisDB supports Linear, NTK-aware, and YaRN scaling methods to extend context from 4K to 32K+ tokens. See Chapter 17.12.7.

**RoPE Scaling:** Technique to extend LLM context windows beyond their original training length by adjusting the rotary position embedding frequency. YaRN method achieves 8x context extension (4K→32K tokens) with <10% quality loss. See Chapter 17.12.7.

**RPO:** Recovery Point Objective (max data loss)

**RTO:** Recovery Time Objective (max downtime)

**Sampling:** Statistical subset of data

**Schema:** Data structure definition

**Selectivity:** Fraction of rows matching filter

**Serialization:** Converting to byte format

**Sharding:** Horizontal data partitioning

**Snapshot:** Point-in-time data view

**Speculative Decoding:** LLM acceleration technique that uses a small, fast "draft model" to speculatively generate multiple tokens in parallel, which are then validated by the larger "target model". Achieves 2-3x speedup with 82-88% token acceptance rates. See Chapter 20.9A.2.

**SQL:** Structured Query Language

**SSTable:** Sorted String Table

**StreamingIngestManager:** High-throughput key-value ingest component using an in-memory ring buffer drained by a background flush thread into a single RocksDB WriteBatch. Supports ≥ 1 M events/s at ≤ 50 ms end-to-end latency. OverflowPolicy BLOCK or DROP. See Chapter 11.8.

**TsStreamCursor:** Lazy, paginated streaming cursor over TSStore query results. Fetches results in configurable pages (default 4 096 DataPoints) to avoid materialising large result sets in memory. See Chapter 9.11.2.

**TSStore::putBatch:** Zero-copy batch write API for TSStore. Accepts `std::span<const TSRow>` and commits all rows in a single RocksDB WriteBatch for maximum write throughput. See Chapter 9.11.1.

**TemporalCompressor:** Component responsible for compressing and decompressing temporal (time-versioned) JSON data in ThemisDB. Supports ZSTD and LZ4 algorithms. LZ4 is optimised for high-throughput, low-latency hot paths. See Chapter 9.11.3.

**TTL:** Time-To-Live (auto-delete after interval)

**Transaction:** Atomic unit of work

**Throughput:** Operations per unit time

**LockFreeHistogram:** Header-only, lock-free latency histogram using per-bucket atomic counters. `record()` costs ≤ 20 ns. Supports Exponential and Linear bucket modes. See Chapter 21.1.1.

**RequestCoalescer:** Cache singleflight implementation that coalesces concurrent requests with the same key so that the backend function `fn()` is executed exactly once per in-flight key, eliminating thundering-herd cache-miss storms. See Chapter 21.1.2.

**IoUringBatchedSender:** Linux io_uring-backed batched network sender that submits multiple WireProtocolBatcher flush operations as a single `io_uring_enter()` syscall, reducing syscall count from O(N) to O(1) per round. Falls back to `writev(2)` when io_uring is unavailable. See Chapter 21.1.3.

**ColumnarCache:** LRU in-memory cache for columnar `ColumnSegment` objects. Cached data is in the same layout as `ColumnBatch`, enabling zero-copy analytics access. Supports PinGuard RAII for eviction protection. See Chapter 15.13.1.

**IStreamingJoin:** Interface for streaming join operators over `ColumnBatch` streams. Concrete implementations: `HashJoin` (equi-join, Inner/LeftOuter) and `IntervalJoin` (time-based event correlation). See Chapter 15.13.2.

**AiHardwareDispatcher:** Universal AI-hardware dispatch layer that selects the best available backend at runtime following the priority chain: NPU → ONNX Runtime → GPU → CPU. Supports INT4/W4A8/W8A8 precision modes. See Chapter 16.11.

**LIRS (Low Inter-Reference Recency Set):** Advanced cache eviction algorithm distinguishing LIR (low inter-reference recency, "hot") from HIR (high inter-reference recency, "warm/cold") entries. ThemisDB's LIRS implementation uses `std::shared_mutex` for thread-safe access.

**RCU (Read-Copy-Update):** Lock-free synchronisation technique for shared data: readers proceed without locks while writers create a new version. `g_rcu_reader_count` tracks active readers; writers wait until the count reaches zero.

**UUID v7:** UUID version defined by RFC 9562 embedding a 48-bit millisecond Unix timestamp for time-sortable identifiers. ThemisDB generates UUID v7 via `generate_uuid_v7()` using a thread-local monotonic sequence counter and MT19937-64 randomness.

**Vector:** Ordered list of numbers

**View:** Virtual table derived from query

**Voice Assistant:** Enterprise feature providing natural language voice interaction using Whisper (STT), Piper (TTS), and llama.cpp (LLM). Enables call center automation, meeting protocol generation, and voice-controlled database queries with DSGVO-compliant storage. See Chapter 10.7.

**WAL (Write-Ahead Log):** Transaction log that records all database changes before they are applied, ensuring durability and enabling replication. In ThemisDB v1.5.0-dev, WAL replication provides zero-data-loss failover with support for synchronous, asynchronous, and hybrid replication modes. See Chapter 16.10.2.

**WAL Replication:** Replication mechanism based on Write-Ahead Log streaming that continuously transfers transaction log entries from primary to replica nodes. Supports sync (zero data loss, higher latency), async (minimal latency, potential data loss), and hybrid modes. See Chapter 16.10.2.

**Warm Data:** Occasionally accessed data

**Whisper:** OpenAI's high-accuracy Speech-to-Text (STT) model integrated into ThemisDB Voice Assistant via whisper.cpp. Supports 100+ languages with auto-detection, speaker diarization, and 5 model sizes (tiny to large) trading accuracy for speed. See Chapter 10.7.

**Workload:** Pattern of database usage

---

## Summary

Understanding these terms is essential for:
- **Development:** Writing efficient queries
- **Operations:** Configuring and monitoring
- **Architecture:** Designing systems
- **Communication:** Discussing with team

Keep this glossary handy when learning or explaining ThemisDB concepts.
