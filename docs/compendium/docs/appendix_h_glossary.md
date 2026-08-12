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

**ArgumentStore:** Ethics AI Plugin component that persists `EthicalArgument` objects as ThemisDB `BaseEntity` entries in RocksDB (or an in-memory fallback for tests). See Chapter 24.9.

**EthicalDiscourseEngine:** Ethics AI Plugin orchestrator that coordinates multi-philosophy debates via `initializeDebate()` and synthesises an `EthicalDecision` via `makeDecision()`. See Chapter 24.9.

**EthicsEvaluator:** Ethics AI Plugin component that scores an `EthicalDecision` across five dimensions: Decision Quality, Consistency, Fairness, Alignment, and Transparency. See Chapter 24.9.

**IAudioBackend:** Plugin interface implemented by `WhisperPlugin`. Provides `transcribe()`, `transcribeFile()`, and `detectLanguage()` methods. See Chapter 10.7.x.

**IImageGenerationBackend:** Plugin interface implemented by `SDPlugin`. Provides `generate()`, `generateBatch()`, and `generateImg2Img()` methods. See Chapter 12.10.

**PhilosophyLoader:** Ethics AI Plugin component that loads and caches philosophy school profiles from YAML files. See Chapter 24.9.

**RAGContextEngine:** Ethics AI Plugin component providing 7 optimised AQL query patterns for context retrieval from the ethics knowledge base. See Chapter 24.9.

**SDPlugin:** Stable Diffusion image-generation plugin implementing `IImageGenerationBackend`. Provides text-to-image, batch, and img2img generation with prompt sanitisation and provenance stamps. See Chapter 12.10.

**SDPromptSanitizer:** Stable Diffusion content-policy component that blocks prompts containing forbidden keywords (case-insensitive, file-loadable blocklist). Covers negative prompts (security gap SD-NP-01). See Chapter 12.10.

**WhisperPlugin:** Speech-to-text plugin implementing `IAudioBackend`. Wraps `IWhisperTranscriber` strategy and `IAudioChunkReader` for file I/O. Thread-safe; adds provenance stamps to every result. See Chapter 10.7.x.

**WavAudioChunkReader:** WAV file reader without external library dependency. Supports 16-bit PCM and IEEE float32 RIFF/WAV. Used by `WhisperPlugin` as default audio input. See Chapter 10.7.x.

**LIRS (Low Inter-Reference Recency Set):** Advanced cache eviction algorithm distinguishing LIR (low inter-reference recency, "hot") from HIR (high inter-reference recency, "warm/cold") entries. ThemisDB's LIRS implementation uses `std::shared_mutex` for thread-safe access.

**RCU (Read-Copy-Update):** Lock-free synchronisation technique for shared data: readers proceed without locks while writers create a new version. `g_rcu_reader_count` tracks active readers; writers wait until the count reaches zero.

**UUID v7:** UUID version 7 as defined by RFC 9562, embedding a 48-bit millisecond Unix timestamp for time-sortable identifiers. ThemisDB generates UUID v7 via `generate_uuid_v7()` using a thread-local monotonic sequence counter and MT19937-64 randomness.

**Vector:** Ordered list of numbers

**View:** Virtual table derived from query

**Voice Assistant:** Enterprise feature providing natural language voice interaction using Whisper (STT), Piper (TTS), and llama.cpp (LLM). Enables call center automation, meeting protocol generation, and voice-controlled database queries with DSGVO-compliant storage. See Chapter 10.7.

**WAL (Write-Ahead Log):** Transaction log that records all database changes before they are applied, ensuring durability and enabling replication. In ThemisDB v1.5.0-dev, WAL replication provides zero-data-loss failover with support for synchronous, asynchronous, and hybrid replication modes. See Chapter 16.10.2.

**WAL Replication:** Replication mechanism based on Write-Ahead Log streaming that continuously transfers transaction log entries from primary to replica nodes. Supports sync (zero data loss, higher latency), async (minimal latency, potential data loss), and hybrid modes. See Chapter 16.10.2.

**Warm Data:** Occasionally accessed data

**Whisper:** OpenAI's high-accuracy Speech-to-Text (STT) model integrated into ThemisDB Voice Assistant via whisper.cpp. Supports 100+ languages with auto-detection, speaker diarization, and 5 model sizes (tiny to large) trading accuracy for speed. See Chapter 10.7.

**Workload:** Pattern of database usage

---

## Process Module Terms

**BpmnSerializer:** Process module component that imports and exports BPMN 2.0 XML using a state-machine tokenizer (no external XML library). Handles Camunda/Flowable/Signavio/VCC-VPB files. 10 MiB input guard. See Chapter 29.14.

**EPK (Ereignisgesteuerte Prozesskette):** Event-driven Process Chain — a German process notation standard. ThemisDB supports EPK via `EpkSerializer` for both text and JSON formats. See Chapter 29.14.

**EpkSerializer:** Process module component for EPK text/JSON import and export. `importText()` accepts line-based EPK notation; `exportJson()` produces a machine-readable JSON graph. See Chapter 29.14.

**LlmProcessDescriptor:** Process module component that generates structured JSON descriptors and system prompts from process models, optimised for GPT-4, Claude, and local LLMs. See Chapter 29.14.

**ProcessAttachment:** Descriptor of a data object attached to a process instance, stored under `proc:attach:<instance_id>:<object_id>` in RocksDB. See Chapter 29.14.

**ProcessDomain:** Classification for process models: `ADMINISTRATION`, `BUSINESS`, `IT_SERVICE`, `HEALTHCARE`, `FINANCE`, `CUSTOMER_SERVICE`, `CUSTOM`. See Chapter 29.14.

**ProcessGraphRag:** Graph-RAG engine that bridges the process execution graph with LLMs. Produces `ProcessRagContext` with subgraph, attachments, missing documents, similar cases, and a ready-to-send LLM prompt. See Chapter 29.14.

**ProcessLinkType:** Typed relationship between a process instance and a data object or another instance: `HAS_DOCUMENT`, `HAS_METADATA`, `REQUIRES_DOCUMENT`, `IS_INSTANCE_OF`, `SUB_PROCESS`, `CROSS_REFERENCE`, `TRIGGERS`, `EVIDENCE_FOR`. See Chapter 29.14.

**ProcessLinker:** Process module component managing attachments (`proc:attach:`), typed process-to-process links (`proc:link:`), and required-document registrations (`proc:req_doc:`) in RocksDB. See Chapter 29.14.

**ProcessModelManager:** Process module CRUD manager storing versioned process models (`proc:def:<id>`) in RocksDB. Supports BPMN 2.0, EPK, and VCC-VPB import/export as well as deployment to `ProcessGraphManager`. See Chapter 29.14.

**ProcessModelRecord:** Metadata record stored alongside each process model: id, name, notation, domain, state, normalised graph, compliance tags, version, and embedding. See Chapter 29.14.

**ProcessNotation:** Format enum for process models: `BPMN_2_0`, `EPK`, `VCC_VPB`, `CMMN_1_1`, `DMN_1_5`. See Chapter 29.14.

**ProcessRagContext:** Full Graph-RAG result produced by `ProcessGraphRag::retrieve()`. Contains the LLM prompt, subgraph, attachments, similar cases, compliance check, and missing documents list. See Chapter 29.14.

**VccVpbImporter:** Process module component that imports VCC-VPB YAML process definitions into the ThemisDB internal graph format. See Chapter 29.14.

**Verwaltungsvorgang:** German administrative case/procedure. ThemisDB's Process Module and `ProcessGraphRag` are specifically optimised for German Verwaltungsprozesse (e.g., Bauantrag, Führerscheinantrag). See Chapter 29.14.

**LIRS (Low Inter-Reference Recency Set):** Advanced cache eviction algorithm distinguishing LIR (low inter-reference recency, "hot") from HIR (high inter-reference recency, "warm/cold") entries. ThemisDB's LIRS implementation uses `std::shared_mutex` for thread-safe access.

**RCU (Read-Copy-Update):** Lock-free synchronisation technique for shared data: readers proceed without locks while writers create a new version. `g_rcu_reader_count` tracks active readers; writers wait until the count reaches zero.

**UUID v7:** UUID version 7 as defined by RFC 9562, embedding a 48-bit millisecond Unix timestamp for time-sortable identifiers. ThemisDB generates UUID v7 via `generate_uuid_v7()` using a thread-local monotonic sequence counter and MT19937-64 randomness.

**Vector:** Ordered list of numbers

**View:** Virtual table derived from query

**Voice Assistant:** Enterprise feature providing natural language voice interaction using Whisper (STT), Piper (TTS), and llama.cpp (LLM). Enables call center automation, meeting protocol generation, and voice-controlled database queries with DSGVO-compliant storage. See Chapter 10.7.

**WAL (Write-Ahead Log):** Transaction log that records all database changes before they are applied, ensuring durability and enabling replication. In ThemisDB v1.5.0-dev, WAL replication provides zero-data-loss failover with support for synchronous, asynchronous, and hybrid replication modes. See Chapter 16.10.2.

**WAL Replication:** Replication mechanism based on Write-Ahead Log streaming that continuously transfers transaction log entries from primary to replica nodes. Supports sync (zero data loss, higher latency), async (minimal latency, potential data loss), and hybrid modes. See Chapter 16.10.2.

**Warm Data:** Occasionally accessed data

**Whisper:** OpenAI's high-accuracy Speech-to-Text (STT) model integrated into ThemisDB Voice Assistant via whisper.cpp. Supports 100+ languages with auto-detection, speaker diarization, and 5 model sizes (tiny to large) trading accuracy for speed. See Chapter 10.7.

**Workload:** Pattern of database usage

---

## LLM Module Terms

**IntegrationTestSuite:** LLM module testing class with 14 scenarios covering component integration (LazyLoader + GPU Memory, Scheduler + Paged Attention, Kernel Fusion + Inference, full E2E pipeline), multi-model serving/switching/LoRA management, failure scenarios (OOM, load failure, cancellation, preemption), and performance (high concurrency, burst traffic, long requests). See Chapter 17.24.

**LlamaWrapper:** Central llama.cpp adapter in ThemisDB's LLM module. Implements `ILLMPlugin`, wrapping llama.cpp inference with full production features: Multi-LoRA, KV-Cache / Prefix Cache, RoPE Scaling, grammar-constrained generation, streaming, and multi-modal vision support. See Chapter 17.24.

**MultiLoRAManager:** vLLM-inspired LoRA adapter manager supporting up to N simultaneous adapters, dynamic load/unload without model reload, INT8/INT4 quantization (`quantizeLoRA()`), and multi-GPU placement (ROUND_ROBIN, DATA_PARALLEL, MODEL_PARALLEL). See Chapter 17.24.

**ProductionValidator:** End-to-end validation framework for the LLM module. Covers 72-hour stress tests, load tests (100 concurrent, 50 RPS), quality validation (≥80% pass rate), and performance regression detection (≤1% tolerance). See Chapter 17.24.

**RoPE Scaling:** Rotary Position Embedding scaling for extending the context window beyond a model's training length. ThemisDB supports LINEAR, NTK, YARN, and DYNAMIC methods. YARN provides the best quality for 8×+ extension (4K→32K tokens). See Chapter 17.24.

**VisionEncoder:** CLIP-based image encoder (`include/llm/vision_encoder.h`) used by `LlamaWrapper::generateVision()`. Loads CLIP GGUF models, encodes image files to float embedding vectors, and supports GPU acceleration. Configured via `enable_vision` + `clip_model_path` in `LlamaWrapper::Config`. See Chapter 17.24.

**VisionRequest / VisionResponse:** Structs for multi-modal LLM inference. `VisionRequest` contains `text_prompt`, `image_path`/`image_paths`, and generation parameters. `VisionResponse` contains `text`, `tokens_generated`, `inference_time_ms`, and `image_encoding_time_ms`. See Chapter 17.24.

---

## RAG v2 Module Terms

**BatchEvaluator:** Parallel batch RAG evaluation using configurable worker threads and async futures/promises. Aggregates individual `EvaluationResult`s into statistics (pass_rate, avg_faithfulness, avg_overall). See Chapter 17.3.5.

**CalibrationManager:** Aligns RAGJudge scores with human annotations via temperature scaling, Platt scaling, and isotonic regression. Reports ECE (Expected Calibration Error), Brier score, and Pearson/Spearman correlation. See Chapter 17.3.5.

**DocumentSplitter:** Configurable text chunking for RAG ingestion pipelines. Strategies: FIXED (token count), SENTENCE (boundary-aware), SEMANTIC (embedding-similarity), RECURSIVE (hierarchical). Configurable `chunk_size` and `chunk_overlap`. See Chapter 17.3.5.

**EvaluationCache:** Thread-safe LRU cache for `EvaluationResult` objects with TTL expiry and invalidation triggers. Tracks hit/miss/eviction statistics. Prevents redundant LLM judge calls for identical inputs. See Chapter 17.3.5.

**EvaluationMode (RAG):** Evaluation speed/depth trade-off for `RAGJudge`. `FAST` (~100 ms, single-dimension), `BALANCED` (~500 ms, multi-dimension, default), `THOROUGH` (~2 s, CoT + NLI verification). See Chapter 17.3.5.

**HallucinationDashboard:** Rolling-window hallucination rate tracker for `RAGJudge` evaluations. Reports current rate (0.0–1.0) and trend (IMPROVING/STABLE/DEGRADING) over a configurable window. See Chapter 17.3.5.

**HybridRetriever:** Fuses BM25 (sparse/keyword) and vector (dense/semantic) candidate lists using Reciprocal Rank Fusion (RRF, k=60) or linear combination. Configurable per-source weights (default 0.5/0.5). See Chapter 17.3.5.

**RAGJudge:** Central RAG evaluation orchestrator in `themis::rag::judge`. Evaluates generated answers across 5 dimensions: Faithfulness, Relevance, Completeness, Coherence, Ethical Compliance. Supports pairwise comparison, batch evaluation, and pluggable NLI/G-Eval scorers. See Chapter 17.3.5.

**RRF (Reciprocal Rank Fusion):** Rank-based fusion formula combining multiple ranked lists: `score(d) = Σ 1/(k + rank(d))`. The constant k=60 (default) controls rank-sensitivity. Used by `HybridRetriever` to combine BM25 and vector results. See Chapter 17.3.5.

Understanding these terms is essential for:
- **Development:** Writing efficient queries
- **Operations:** Configuring and monitoring
- **Architecture:** Designing systems
- **Communication:** Discussing with team

Keep this glossary handy when learning or explaining ThemisDB concepts.

---

## v1.5.0 Update: Neue Begriffe (Phase 3 / Q3-Q4 2026)

### Verschlüsselung & Security

**AES-256-GCM:** Advanced Encryption Standard im Galois/Counter Mode mit 256-bit-Schlüssel. Bietet Authenticated Encryption with Associated Data (AEAD) — kombiniert Vertraulichkeit und Integrität in einem Pass. In ThemisDB verwendet für Column-Level Encryption, Vektor-Verschlüsselung und verschlüsselte Backups. Empfohlen durch BSI TR-02102-1.

**Column-Level Encryption (CLE):** Feingranulare Verschlüsselung einzelner Datenbankfelder at-rest. ThemisDB implementiert CLE über das `EncryptedField<T>` Template; die Verschlüsselung ist transparent für Anwendungscode. Algorithmus: AES-256-GCM. Implementierung: `include/security/encryption.h`. Siehe Kapitel 22b.

**DEK (Data Encryption Key):** Datenverschlüsselungsschlüssel in der KEK/DEK-Hierarchie. Einer pro Feld-Kategorie (z.B. `user_emails`, `vector_embeddings`). Versioniert für Lazy Re-Encryption bei Key Rotation. Wird durch KEK verschlüsselt gespeichert.

**Dual-Write Pattern:** Key-Rotation-Strategie, bei der neue Schreibvorgänge sofort mit dem neuen Schlüssel (v_new) verschlüsselt werden, während Lesevorgänge den alten Schlüssel (v_old) noch akzeptieren. Ermöglicht unterbrechungsfreie Schlüsselrotation.

**EncryptedField\<T\>:** C++ Template-Klasse (`include/security/encryption.h`), die ein Feld transparenter Verschlüsselung unterwirft. Ruft `FieldEncryption::encrypt()` on set und `decrypt()` on get auf. Unterstützt alle serialisierbaren Typen inkl. `std::vector<float>` für Embeddings. Siehe Kapitel 22b.

**HSMProvider:** PKCS#11-basierter Key Provider für Hardware Security Module. Vollständig implementiert in `src/security/hsm_provider_pkcs11.cpp` (1056 Zeilen). Verwendet `dlopen/dlsym` für dynamisches Laden der PKCS#11-Bibliothek. Schützt Master Keys und KEKs in Hardware. Siehe Kapitel 22b.6.3.

**IV (Initialization Vector):** Zufälliger 96-bit-Wert, der pro Verschlüsselungsoperation in AES-256-GCM neu generiert wird. Verhindert deterministische Ciphertexte bei identischem Plaintext. IV-Wiederverwendung ist fatal für GCM-Sicherheit.

**KEK (Key Encryption Key):** Schlüsselverschlüsselungsschlüssel in der KEK/DEK-Hierarchie. Verschlüsselt die DEKs. Jährliche Rotation empfohlen (BSI C5 CRY-02). Geschützt durch Master Key im HSM oder Cloud-KMS.

**Kyber-768:** Post-Quantum-Schlüsselaustausch-Algorithmus (CRYSTALS-Kyber) aus dem NIST PQC Round-3-Finalisierungsverfahren. In ThemisDB als Hybrid-Option zusammen mit X25519 verfügbar (`security.post_quantum.enabled: true`). Schützt gegen Quantum-Adversarien bei der Key-Exchange-Phase.

**Lazy Re-Encryption:** Strategie, bei der Daten erst beim nächsten Schreibzugriff mit dem neuen Schlüssel verschlüsselt werden. Vermeidet einen einmaligen, blockierenden Bulk-Re-Encryption-Pass bei Key Rotation.

**mTLS (Mutual TLS):** TLS-Variante, bei der sowohl Client als auch Server ihr Zertifikat vorlegen und gegenseitig verifizieren. In ThemisDB verpflichtend für Shard-to-Shard-Kommunikation. Implementierung: `src/security/` (OpenSSL). Konfiguration: `cluster.shard_communication.mtls`. Siehe Kapitel 22b.2.2.

**VaultKeyProvider:** HashiCorp-Vault-Integration für Enterprise Key Management. Vollständig implementiert in `src/security/vault_key_provider.cpp` (739 Zeilen). Unterstützt Vault KV v1/v2 und Transit Engine. Thread-sicher mit Retry-Logic und Key-Cache (1h TTL). Siehe Kapitel 22b.6.3.

### Deployment & Operations

**Edition Control Mechanism:** Laufzeit-Durchsetzung von Editions-Grenzen (Minimal/Community/Enterprise/Hyperscaler). Liest Edition-Flags aus der Lizenzkonfiguration und blockiert Features, die für die aktive Edition nicht lizenziert sind. Implementierung: `src/edition/`. Dokumentation: `docs/de/deployment/EDITION_CONTROL_MECHANISMS.md`.

**Edition Limits Matrix:** Tabelle der Feature-Grenzen pro Edition (max. Cluster-Nodes, Shards, gleichzeitige Verbindungen, Enterprise-Features). Verbindlich für Deployment-Planung. Referenz: `docs/de/deployment/EDITION_LIMITS_MATRIX.md`.

### Observability & SRE

**SLO (Service Level Objective):** Messbare Zielvorgabe für Service-Qualität (z.B. „99.9 % Requests unter 50 ms"). ThemisDB Observability-Stack exportiert SLO-Burn-Rate-Metriken via OpenTelemetry. Konfiguration: `docs/de/observability/observability_alerting.md`.

**SLI (Service Level Indicator):** Messgröße für SLO-Bewertung (z.B. Error Rate, P99-Latenz, Throughput). Wird von ThemisDB-Metriken direkt in den OpenTelemetry-Exporter eingespeist. Referenz: `docs/de/observability/observability_metrics.md`.

**OpenTelemetry (OTel):** Vendor-neutrales Observability-Framework für Traces, Metriken und Logs. ThemisDB exportiert alle Observability-Daten über OTLP (OpenTelemetry Protocol) zu konfigurierbaren Backends (Jaeger, Prometheus, OTEL Collector). Siehe Kapitel 38.

---

**Appendix H — Stand:** v1.5.0-dev (Q3/Q4 2026 Update)
