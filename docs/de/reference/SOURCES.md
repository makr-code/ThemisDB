# ThemisDB - Source Code References

**Version:** 1.4.0  
**Last Updated:** April 2026  
**Purpose:** Detailed mapping of code files to their origins and inspirations

---

## Quick Reference Table

| ThemisDB File | Origin/Inspiration | Type | Status |
|---------------|-------------------|------|--------|
| `include/index/vector_index.h` | hnswlib | Library Integration | ✅ Attributed |
| `include/index/advanced_vector_index.h` | FAISS | Library Integration | ✅ Attributed |
| `include/index/product_quantizer.h` | PQ Paper (Jégou et al.) | Algorithm | ✅ Attributed |
| `include/acceleration/faiss_gpu_backend.h` | FAISS | Library Integration | ✅ Attributed |
| `include/timeseries/gorilla.h` | Gorilla Paper (Facebook) | Algorithm | ✅ Attributed |
| `include/timeseries/hypertable.h` | TimescaleDB | Concept | ✅ Attributed |
| `include/sharding/raft_state.h` | Raft Paper (Ongaro) | Algorithm | ✅ Attributed |
| `include/sharding/gossip_protocol.h` | Cassandra | Algorithm | ✅ Attributed |
| `include/sharding/urn.h` | ThemisDB Original | Original | ✅ Documented |
| `include/storage/base_entity.h` | ThemisDB Original | Original | ✅ Documented |
| `include/utils/serialization.h` | VelocyPack/MessagePack | Inspiration | ✅ Attributed |
| `include/query/functions/graph_functions.h` | ArangoDB AQL | Syntax Compatibility | ✅ Attributed |
| `include/query/functions/geo_functions.h` | OGC/PostGIS | Standards/Inspiration | ✅ Attributed |
| `include/llm/llama_wrapper.h` | llama.cpp | Library Integration | ✅ Documented |
| `include/llm/lora_framework/flash_lora.h` | FlashAttention | Algorithm | ✅ Documented |
| `include/llm/multi_lora_manager.h` | vLLM | Concept | ✅ Documented |
| `include/llm/model_loader.h` | Ollama | Concept | ✅ Documented |
| `include/performance/dostoevsky.h` | Dostoevsky Paper | Algorithm | ✅ Documented |
| `include/performance/phase3/diskann.h` | DiskANN Paper | Algorithm | ✅ Documented |

---

## Vector Search Implementation

### HNSW Index (`include/index/vector_index.h`)

**Attribution Block:**
```cpp
/// @sources
/// - HNSW Algorithm: Malkov, Y. A., & Yashunin, D. A. (2018).
///   "Efficient and robust approximate nearest neighbor search using 
///    Hierarchical Navigable Small World graphs"
///   IEEE Transactions on Pattern Analysis and Machine Intelligence
/// - Library: hnswlib - https://github.com/nmslib/hnswlib
/// - License: Apache 2.0
/// - ThemisDB Integration: Transactional updates, RocksDB persistence, 
///   audit logging
```

**Code Sections:**
- Lines 30-36: Attribution comment
- Lines 45-55: VectorIndexManager class declaration
- Lines 75-98: CRUD operations (ThemisDB additions for transactions)

**What's From hnswlib:**
- Core HNSW algorithm
- Multi-layer graph structure
- Greedy search algorithm

**What's ThemisDB Original:**
- Transactional wrapper with MVCC
- RocksDB persistence integration
- Audit logging for vector operations
- WriteBatch and TransactionWrapper support

---

### FAISS Integration (`include/index/advanced_vector_index.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Based on: FAISS (Facebook AI Similarity Search)
 * - Library: https://github.com/facebookresearch/faiss
 * - Paper: Johnson, J., Douze, M., & Jégou, H. (2019). 
 *          "Billion-scale similarity search with GPUs." 
 *          IEEE Transactions on Big Data.
 * - License: MIT
 * - ThemisDB Integration: Transactional wrapper with ACID guarantees,
 *   multi-backend GPU support, and RocksDB persistence layer
 */
```

**Code Sections:**
- Lines 11-23: Attribution and feature description
- Lines 42-49: Index type enum (from FAISS)
- Lines 57-94: ThemisDB wrapper API

**What's From FAISS:**
- IVF (Inverted File) index structure
- Product Quantization (PQ) algorithm
- GPU acceleration kernels
- Index types: IndexFlatL2, IndexFlatIP, IndexIVFFlat, IndexIVFPQ

**What's ThemisDB Original:**
- ACID transaction support
- Multi-backend GPU abstraction (10 backends)
- RocksDB persistence layer
- Training workflow integration

---

### Product Quantization (`include/index/product_quantizer.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Algorithm: Product Quantization
 * - Paper: Jégou, H., Douze, M., & Schmid, C. (2011). 
 *          "Product Quantization for Nearest Neighbor Search"
 *          IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI)
 * - DOI: 10.1109/TPAMI.2010.57
 * - URL: https://hal.inria.fr/inria-00514462
 * - Implementation Inspiration: FAISS library (Meta AI Research)
 * - ThemisDB Extension: Integrated with RocksDB storage and ACID transactions
 */
```

**Code Sections:**
- Lines 10-20: Attribution
- Lines 56-71: Core PQ operations (based on paper)
- Lines 84-98: ThemisDB-specific additions

**What's From Paper:**
- K-means clustering for codebook training
- Subspace decomposition
- Asymmetric distance computation
- Encoding/decoding algorithms

**What's ThemisDB Original:**
- RocksDB integration for persistent codebooks
- Transaction-safe training and encoding
- Memory usage tracking
- Integration with ThemisDB index manager

---

## Time-Series Implementation

### Gorilla Compression (`include/timeseries/gorilla.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Algorithm: Gorilla Time Series Compression
 * - Paper: Pelkonen, T., Franklin, S., et al. (2015)
 *          "Gorilla: A Fast, Scalable, In-Memory Time Series Database"
 *          Proceedings of the VLDB Endowment, Vol. 8, No. 12
 * - Company: Facebook (Meta)
 * - URL: http://www.vldb.org/pvldb/vol8/p1816-teller.pdf
 * - Implementation: Custom implementation for ThemisDB based on algorithm
 */
```

**Code Sections:**
- Lines 17-31: BitWriter/BitReader (custom implementation)
- Lines 49-62: GorillaEncoder (based on paper algorithm)
- Lines 64-77: GorillaDecoder (based on paper algorithm)

**What's From Paper:**
- Delta-of-delta timestamp encoding
- XOR-based value compression
- Leading/trailing zero optimization
- ZigZag encoding for signed integers

**What's ThemisDB Original:**
- Complete C++ implementation (no library dependency)
- Integration with ThemisDB storage format
- Custom bit-level I/O operations
- RocksDB persistence

**Implementation Notes:**
- Algorithm faithfully implements paper description
- No direct code copied - implemented from algorithm specification
- Optimized for ThemisDB's time-series use cases

---

### Hypertables (`include/timeseries/hypertable.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Concept inspired by: TimescaleDB Hypertables
 * - Project: TimescaleDB - https://github.com/timescale/timescaledb
 * - License: Timescale License (Apache 2.0 compatible for Community Edition)
 * - Paper: Freedman, A., et al. (2017) "TimescaleDB: An Open-Source 
 *          Time-Series SQL Database"
 * - ThemisDB Implementation: Uses RocksDB Column Families instead of 
 *   PostgreSQL partitions, custom chunk management, and integration with 
 *   ThemisDB's MVCC transaction system
 */
```

**Code Sections:**
- Lines 18-29: Concept description
- Lines 31-62: Hypertable class (ThemisDB implementation)
- Lines 76-98: Chunk management (ThemisDB original)

**What's From TimescaleDB:**
- Concept of time-based partitioning ("chunks")
- Automatic chunk creation on insert
- TTL-based data retention
- Chunk interval configuration

**What's ThemisDB Different:**
- RocksDB Column Families vs PostgreSQL table partitions
- No SQL engine dependency
- Integration with ThemisDB's MVCC
- Different compression strategy (ZSTD vs PostgreSQL TOAST)

**No Direct Code Used:**
- Concept only - completely independent implementation
- Different storage backend (RocksDB vs PostgreSQL)
- Different API (C++ vs SQL)

---

## Distributed Systems

### Raft Consensus (`include/sharding/raft_state.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Algorithm: Raft Consensus Protocol
 * - Paper: Ongaro, D., & Ousterhout, J. (2014)
 *          "In Search of an Understandable Consensus Algorithm"
 *          USENIX Annual Technical Conference (ATC '14)
 * - URL: https://raft.github.io/
 * - Extended Paper: https://raft.github.io/raft.pdf
 * - License: Algorithm is freely implementable (no license restrictions)
 * - ThemisDB Implementation: Custom implementation with:
 *   - Integration with RocksDB for persistent log storage
 *   - Support for ThemisDB's VCC-URN sharding scheme
 *   - mTLS support for secure cluster communication
 *   - Optimized for database replication workloads
 */
```

**Code Sections:**
- Lines 19-42: Raft structs (VoteRequest, VoteResponse, etc.) - from paper
- Lines 47-65: RaftConfig - ThemisDB additions
- Lines 95-185: RaftState class - custom implementation

**What's From Paper:**
- Leader/Follower/Candidate states
- Election timeout mechanism
- Log replication algorithm
- Vote request/response protocol
- AppendEntries RPC structure

**What's ThemisDB Original:**
- Complete C++ implementation (no library used)
- RocksDB-based persistent log
- VCC-URN integration
- mTLS certificate validation
- Custom health monitoring

**Implementation Notes:**
- Algorithm implemented from paper specification
- No code copied from other Raft implementations
- Tailored for database replication (not general consensus)

---

### Gossip Protocol (`include/sharding/gossip_protocol.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Algorithm: Gossip Protocol (Epidemic/Anti-Entropy)
 * - Inspired by: Apache Cassandra's Gossip Implementation
 * - Paper: van Renesse, R., Birman, K. P., & Vogels, W. (2003)
 *          "Astrolabe: A robust and scalable technology for distributed 
 *           system monitoring"
 *          ACM Transactions on Computer Systems, 21(2), 164-206
 * - Cassandra: https://cassandra.apache.org/doc/latest/architecture/gossip.html
 * - License: Apache 2.0 (Cassandra)
 * - ThemisDB Implementation: Custom gossip protocol with:
 *   - Integration with VCC-URN sharding
 *   - mTLS certificate-based peer validation
 *   - Datacenter/region-aware topology
 *   - Optimized for database cluster state synchronization
 */
```

**Code Sections:**
- Lines 26-50: PeerInfo struct - similar to Cassandra's endpoint state
- Lines 65-95: GossipMessage - based on Cassandra message format
- Lines 110-180: GossipProtocol class - ThemisDB implementation

**What's From Cassandra Concept:**
- Periodic peer selection algorithm
- Version vector for anti-entropy
- Heartbeat mechanism
- Suspicion/alive state transitions

**What's ThemisDB Different:**
- mTLS certificate validation (not in Cassandra)
- VCC-URN aware routing
- JSON-based message format (Cassandra uses custom binary)
- Different failure detector tuning

**No Direct Code Used:**
- Concept and algorithm pattern inspired by Cassandra
- Completely independent C++ implementation
- Different protocol format and wire encoding

---

## ThemisDB Original Concepts

### VCC-URN System (`include/sharding/urn.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Concept: VCC-URN (Virtual Content Container - Uniform Resource Name)
 * - Origin: ThemisDB Original Design
 * - Purpose: Unified addressing scheme for multi-model database with sharding
 * - Inspiration: 
 *   - URN Standard: RFC 8141 (Uniform Resource Names)
 *   - Azure Cosmos DB: Hierarchical partition keys
 *   - Cassandra: Partition key + clustering key
 * - Innovation: Combines URN standard with multi-model awareness and 
 *               content-based routing
 * - Implementation: ThemisDB Core Team
 * - First Introduced: ThemisDB v1.0.0
 */
```

**100% ThemisDB Original:**
- URN format design: `urn:themis:{model}:{namespace}:{collection}:{uuid}`
- Content-based routing algorithm
- Multi-model awareness in addressing
- Integration with PKI for cryptographic verification

**Inspired By (Concept Only):**
- RFC 8141: URN syntax rules
- Cosmos DB: Hierarchical partition concept
- Cassandra: Partition+clustering pattern

**No Code From External Sources**

---

### Base Entity Model (`include/storage/base_entity.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Concept: Unified Multi-Model Storage with Canonical Entity Pattern
 * - Origin: ThemisDB Original Design
 * - Design Philosophy: "One canonical storage, multiple projection layers"
 * - Inspiration:
 *   - ArangoDB: Multi-model architecture with unified storage
 *   - CozoDB: Hybrid relational-graph-vector design
 *   - Azure Cosmos DB: Multi-model APIs over single storage engine
 * - Innovation: ThemisDB extends the multi-model concept with:
 *   - True unified storage (not multiple engines)
 *   - ACID transactions across all models simultaneously
 *   - Atomic multi-index updates (secondary, graph, vector, fulltext)
 *   - Zero-overhead model projection (no data duplication)
 * - Implementation: ThemisDB Core Team
 * - First Introduced: ThemisDB v1.0.0
 */
```

**100% ThemisDB Original:**
- BaseEntity class design
- Multi-index atomic update mechanism
- Lazy parsing with field extraction
- MVCC snapshot integration
- Field-level encryption within entity

**Inspired By (Architecture Pattern):**
- ArangoDB: Multi-model concept
- CozoDB: Hybrid model design
- Cosmos DB: Unified API concept

**Key Differentiators:**
- Transactional vector indexes (unique to ThemisDB)
- Integrated LLM with zero-copy access (unique)
- True single storage layer (ArangoDB uses multiple engines)

**No Code From External Sources**

---

## Query Language

### Graph Functions (`include/query/functions/graph_functions.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Query Language Inspiration: ArangoDB AQL (Arango Query Language)
 * - Repository: https://github.com/arangodb/arangodb
 * - License: Apache 2.0
 * - Documentation: https://www.arangodb.com/docs/stable/aql/graphs.html
 * - ThemisDB Implementation: Custom graph functions with AQL-compatible syntax
 *   - Integrated with ThemisDB's graph index
 *   - ACID transaction support
 *   - Optimized for RocksDB storage backend
 */
```

**What's AQL-Compatible (Syntax):**
- Function names: SHORTEST_PATH, GRAPH_NEIGHBORS, etc.
- Edge format: _from, _to, _type fields
- Direction specifiers: OUTBOUND, INBOUND, ANY

**What's ThemisDB Implementation:**
- Complete C++ implementation (no AQL code used)
- Dijkstra's algorithm for shortest path
- BFS/DFS traversal implementations
- PageRank algorithm
- Centrality calculations

**Relationship:**
- Syntax compatible for migration ease
- Completely independent implementation
- Different execution engine (ThemisDB vs ArangoDB)

---

### Geo Functions (`include/query/functions/geo_functions.h`)

**Attribution Block:**
```cpp
/**
 * @sources
 * - Standards: OGC Simple Features Specification
 * - GeoJSON: RFC 7946
 * - Inspiration: ArangoDB Geo Functions
 * - Inspiration: PostGIS
 * - ThemisDB Implementation: Custom spatial functions with AQL-compatible syntax
 */
```

**What's From Standards:**
- OGC function names: ST_DISTANCE, ST_CONTAINS, etc.
- GeoJSON format
- WGS84 coordinate system (EPSG:4326)

**What's Inspired By:**
- ArangoDB: Function naming for AQL compatibility
- PostGIS: Spatial predicate behavior

**What's ThemisDB Implementation:**
- Haversine formula for great-circle distance
- Point-in-polygon algorithms
- MBR (Minimum Bounding Rectangle) calculations
- Custom GeoJSON parser

---

## LLM Integration

### llama.cpp Wrapper (`include/llm/llama_wrapper.h`)

**External Component:**
- Library: llama.cpp
- Repository: https://github.com/ggerganov/llama.cpp
- License: MIT
- Creator: Georgi Gerganov

**ThemisDB Wrapper:**
- Lifecycle management (load/unload)
- Multi-backend GPU support (10 backends)
- Memory pooling
- Inference request queuing
- Integration with ThemisDB vector search (zero-copy)

**No llama.cpp Code Modified:**
- Uses llama.cpp as external library
- Wrapper provides database-specific features

---

## Summary

### Attribution Coverage

- ✅ **13 external algorithms** properly attributed with papers
- ✅ **8 library integrations** documented with licenses
- ✅ **5 database inspirations** noted (ArangoDB, CozoDB, Cosmos DB, TimescaleDB, Cassandra)
- ✅ **2 ThemisDB original concepts** documented (VCC-URN, Base Entity)

### Implementation Types

1. **Library Integration** (5 files)
   - Direct use of external library with ThemisDB wrapper
   - Examples: FAISS, hnswlib, llama.cpp

2. **Algorithm Implementation** (6 files)
   - Custom implementation based on paper specification
   - Examples: Raft, Gorilla, Product Quantization

3. **Concept Inspiration** (4 files)
   - Design pattern inspired by other systems
   - Complete independent implementation
   - Examples: Hypertables, Gossip Protocol

4. **Syntax Compatibility** (2 files)
   - Function names compatible for migration
   - Independent implementation
   - Examples: Graph Functions, Geo Functions

5. **ThemisDB Original** (2 files)
   - Unique ThemisDB innovations
   - Examples: VCC-URN, Base Entity Model

---

## License Compliance

All external sources are MIT/Apache 2.0 compatible:

- ✅ MIT: FAISS, llama.cpp, nlohmann/json, spdlog, mimalloc
- ✅ Apache 2.0: hnswlib, RocksDB, Arrow, TBB, simdjson, ArangoDB (Community), Cassandra
- ✅ BSD-3: Google Test, FlashAttention
- ✅ Boost License: Boost.Asio
- ✅ Freely Implementable: Raft, Gorilla (from papers)

No GPL code directly incorporated (RocksDB dual-licensed as Apache 2.0).

---

**For detailed attributions, see:**
- Main document: [IMPLEMENTATION_ORIGINS.md](IMPLEMENTATION_ORIGINS.md)
- Legal details: [docs/de/legal/ATTRIBUTIONS.md](docs/de/legal/ATTRIBUTIONS.md)
- Third-party licenses: [docs/de/legal/THIRD_PARTY_LICENSES.md](docs/de/legal/THIRD_PARTY_LICENSES.md)
