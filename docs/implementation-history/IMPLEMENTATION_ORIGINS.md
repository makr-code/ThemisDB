# ThemisDB - Implementation Origins & Code Attribution

**Version:** 1.4.0  
**Last Updated:** April 2026  
**Purpose:** Comprehensive documentation of code origins, inspirations, and attributions

---

## 📋 Table of Contents

1. [Project Origins](#project-origins)
2. [Core Concepts & Innovations](#core-concepts--innovations)
3. [External Algorithm Implementations](#external-algorithm-implementations)
4. [Library Integrations](#library-integrations)
5. [Design Inspirations](#design-inspirations)
6. [Academic Paper References](#academic-paper-references)

---

## Project Origins

### ThemisDB Overview

**ThemisDB** is an open-source multi-model database system combining relational, graph, vector, and document models with native LLM integration.

- **License:** MIT License with Government Clause
- **Copyright:** © 2025 The ThemisDB Authors
- **Repository:** https://github.com/makr-code/ThemisDB
- **First Release:** Version 1.0.0 (2025)

### Development Philosophy

ThemisDB follows the principle of **"standing on the shoulders of giants"** - building upon excellent open-source projects while creating unique innovations:

1. **Use proven algorithms** from academic papers
2. **Integrate best-in-class libraries** (RocksDB, FAISS, llama.cpp)
3. **Create unique innovations** where existing solutions don't meet requirements
4. **Attribute all sources transparently** to honor original creators

---

## Core Concepts & Innovations

### 1. VCC-URN (Virtual Content Container - Uniform Resource Name)

**Origin:** ThemisDB Original Design  
**Files:** `include/sharding/urn.h`, `include/sharding/urn_resolver.h`

**What is VCC-URN?**

A unified addressing scheme for multi-model databases with sharding support.

```
Format: urn:themis:{model}:{namespace}:{collection}:{uuid}
Example: urn:themis:vector:embeddings:documents:f47ac10b-58cc-4372-a567-0e02b2c3d479
```

**Inspiration:**
- RFC 8141: Uniform Resource Names (URN) standard
- Azure Cosmos DB: Hierarchical partition keys concept
- Apache Cassandra: Partition key + clustering key pattern

**ThemisDB Innovation:**
- Combines URN standard with multi-model awareness
- Content-based routing for efficient sharding
- Cross-model query support via unified addressing
- Integration with VCC-PKI for cryptographic verification

**Status:** ✅ Original ThemisDB implementation (v1.0.0+)

---

### 2. Unified Multi-Model Storage Architecture

**Origin:** ThemisDB Original Design  
**Files:** `include/storage/base_entity.h`, `src/storage/base_entity.cpp`

**What is the Base Entity Model?**

A canonical storage pattern where all data types (rows, documents, nodes, edges, vectors) are stored as flexible binary blobs with multiple projection layers.

**Design Philosophy:**
> "One canonical storage, multiple projection layers"

**Inspiration:**
- **ArangoDB**: Multi-model architecture concept
- **CozoDB**: Hybrid relational-graph-vector design
- **Azure Cosmos DB**: Multi-model APIs over single storage

**ThemisDB Innovations (Beyond Inspirations):**

| Feature | ThemisDB | ArangoDB | CozoDB | Cosmos DB |
|---------|----------|----------|--------|-----------|
| Unified Storage | ✅ True single storage | ⚠️ Separate engines | ✅ Yes | ⚠️ Multiple engines |
| ACID Across Models | ✅ All models | ⚠️ Limited | ✅ Yes | ⚠️ Configurable |
| Transactional Vectors | ✅ Yes | ❌ No | ❌ No | ❌ No |
| Integrated LLM | ✅ Native llama.cpp | ❌ No | ❌ No | ❌ No |
| Field Encryption | ✅ In-entity AES-256 | ⚠️ External | ❌ No | ⚠️ External |
| Zero-Copy Vector→LLM | ✅ Yes | ❌ No | ❌ No | ❌ No |

**Status:** ✅ Original ThemisDB implementation (v1.0.0+)

---

### 3. Native LLM Integration

**Origin:** ThemisDB Original Feature  
**Files:** `include/llm/llama_wrapper.h`, `src/llm/llama_wrapper.cpp`

**What is it?**

ThemisDB is the **world's first database** with an integrated LLM inference engine (llama.cpp), enabling:
- Zero-copy memory sharing between Vector DB and LLM
- 100-1000x cost reduction vs cloud APIs
- Full data sovereignty (no external API calls)

**External Component:**
- **llama.cpp**: https://github.com/ggerganov/llama.cpp (MIT License)
- Creator: Georgi Gerganov

**ThemisDB Integration:**
- Custom wrapper for database integration
- Multi-GPU backend support (10 backends)
- Continuous batching (vLLM-inspired)
- Zero-copy RAG pipeline

**Status:** ✅ v1.3.0+ (Optional feature)

---

## External Algorithm Implementations

### Vector Search & Indexing

#### 1. HNSW (Hierarchical Navigable Small World)

**Files:** `include/index/vector_index.h`

**Source:**
- Algorithm: HNSW (Hierarchical Navigable Small World Graphs)
- Paper: Malkov, Y. A., & Yashunin, D. A. (2018)  
  "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs"  
  IEEE Transactions on Pattern Analysis and Machine Intelligence
- Library: hnswlib - https://github.com/nmslib/hnswlib
- License: Apache 2.0

**ThemisDB Integration:**
- Transactional updates with MVCC
- RocksDB persistence layer
- Audit logging support

---

#### 2. FAISS (Facebook AI Similarity Search)

**Files:** `include/index/advanced_vector_index.h`, `include/acceleration/faiss_gpu_backend.h`

**Source:**
- Library: FAISS (Facebook AI Similarity Search)
- Repository: https://github.com/facebookresearch/faiss
- Paper: Johnson, J., Douze, M., & Jégou, H. (2019)  
  "Billion-scale similarity search with GPUs"  
  IEEE Transactions on Big Data, 7(3), 535-547
- arXiv: https://arxiv.org/abs/1702.08734
- License: MIT

**Index Types Used:**
- `IndexFlatL2`: Exact L2 distance search
- `IndexFlatIP`: Exact inner product search
- `IndexIVFFlat`: Inverted file with flat quantizer
- `IndexIVFPQ`: Inverted file with product quantization

**ThemisDB Integration:**
- Multi-backend GPU support wrapper
- RocksDB persistence
- ACID transaction integration

---

#### 3. Product Quantization (PQ)

**Files:** `include/index/product_quantizer.h`

**Source:**
- Algorithm: Product Quantization
- Paper: Jégou, H., Douze, M., & Schmid, C. (2011)  
  "Product Quantization for Nearest Neighbor Search"  
  IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI)
- DOI: 10.1109/TPAMI.2010.57
- URL: https://hal.inria.fr/inria-00514462

**Implementation:**
- Custom implementation based on algorithm description
- Optimized for ThemisDB's RocksDB storage
- Integrated with ACID transactions

---

### Time-Series Storage

#### 4. Gorilla Compression

**Files:** `include/timeseries/gorilla.h`, `src/timeseries/gorilla.cpp`

**Source:**
- Algorithm: Gorilla Time Series Compression
- Paper: Pelkonen, T., Franklin, S., et al. (2015)  
  "Gorilla: A Fast, Scalable, In-Memory Time Series Database"  
  Proceedings of the VLDB Endowment, Vol. 8, No. 12
- Company: Facebook (Meta)
- URL: http://www.vldb.org/pvldb/vol8/p1816-teller.pdf

**Features:**
- Delta-of-delta timestamp encoding with ZigZag + varint
- XOR-based value compression with leading/trailing zero optimization
- Custom implementation for ThemisDB

---

#### 5. Hypertables (TimescaleDB-inspired)

**Files:** `include/timeseries/hypertable.h`

**Source:**
- Concept: Hypertables for time-series partitioning
- Inspired by: TimescaleDB
- Project: https://github.com/timescale/timescaledb
- License: Timescale License (Apache 2.0 compatible for Community Edition)
- Paper: Freedman, A., et al. (2017)  
  "TimescaleDB: An Open-Source Time-Series SQL Database"

**ThemisDB Implementation:**
- Uses RocksDB Column Families instead of PostgreSQL partitions
- Custom chunk management
- Integration with ThemisDB's MVCC transaction system
- TTL-based automatic cleanup

---

### Distributed Systems

#### 6. Raft Consensus

**Files:** `include/sharding/raft_state.h`, `src/sharding/raft_state.cpp`

**Source:**
- Algorithm: Raft Consensus Protocol
- Paper: Ongaro, D., & Ousterhout, J. (2014)  
  "In Search of an Understandable Consensus Algorithm"  
  USENIX Annual Technical Conference (ATC '14)
- URL: https://raft.github.io/
- Extended Paper: https://raft.github.io/raft.pdf
- License: Algorithm is freely implementable

**ThemisDB Implementation:**
- Custom implementation with RocksDB persistent log
- Integration with VCC-URN sharding
- mTLS support for secure cluster communication
- Optimized for database replication workloads

---

#### 7. Gossip Protocol

**Files:** `include/sharding/gossip_protocol.h`, `src/sharding/gossip_protocol.cpp`

**Source:**
- Algorithm: Gossip Protocol (Epidemic/Anti-Entropy)
- Inspired by: Apache Cassandra's Gossip Implementation
- Paper: van Renesse, R., Birman, K. P., & Vogels, W. (2003)  
  "Astrolabe: A robust and scalable technology for distributed system monitoring"  
  ACM Transactions on Computer Systems, 21(2), 164-206
- Cassandra Docs: https://cassandra.apache.org/doc/latest/architecture/gossip.html
- License: Apache 2.0 (Cassandra)

**ThemisDB Implementation:**
- Integration with VCC-URN sharding
- mTLS certificate-based peer validation
- Datacenter/region-aware topology
- Optimized for database cluster state synchronization

---

### LLM Optimization Techniques

#### 8. FlashAttention-inspired LoRA

**Files:** `include/llm/lora_framework/flash_lora.h`

**Source:**
- Algorithm: FlashAttention
- Papers:
  - Dao, T., et al. (2022): "FlashAttention: Fast and Memory-Efficient Exact Attention with IO-Awareness"
  - Dao, T. (2023): "FlashAttention-2: Faster Attention with Better Parallelism and Work Partitioning"
- URL: https://github.com/Dao-AILab/flash-attention
- License: BSD-3-Clause

**LoRA Paper:**
- Hu, E. J., et al. (2021): "LoRA: Low-Rank Adaptation of Large Language Models"
- Microsoft Research

**ThemisDB Implementation:**
- Fused LoRA kernels inspired by FlashAttention memory optimization
- Integration with llama.cpp
- Multi-GPU support (10 backends)

---

#### 9. vLLM-inspired Multi-LoRA Management

**Files:** `include/llm/multi_lora_manager.h`

**Source:**
- Inspiration: vLLM (UC Berkeley)
- Repository: https://github.com/vllm-project/vllm
- License: Apache 2.0
- Features: Continuous batching, efficient adapter switching

**ThemisDB Implementation:**
- Multiple LoRA adapters loaded simultaneously
- Efficient adapter switching during inference
- GPU memory management
- Integration with ThemisDB's transaction system

---

#### 10. Ollama-inspired Model Loading

**Files:** `include/llm/model_loader.h`

**Source:**
- Inspiration: Ollama
- Repository: https://github.com/ollama/ollama
- License: MIT
- Feature: Lazy model loading

**ThemisDB Implementation:**
- Models loaded on-demand when first requested
- Automatic model unloading when not used
- Memory-efficient model management

---

### Storage & Performance

#### 11. Dostoevsky LSM Merge Strategy

**Files:** `include/performance/dostoevsky.h`

**Source:**
- Paper: Dayan, N., & Idreos, S. (2018)  
  "Dostoevsky: Better Space-Time Trade-Offs for LSM-Trees via Adaptive Removal of Superfluous Merging"  
  SIGMOD '18
- Institution: Harvard University
- URL: https://dl.acm.org/doi/10.1145/3183713.3196927

**ThemisDB Implementation:**
- Adaptive LSM compaction strategy
- Integration with RocksDB
- Optimized for time-series workloads

---

#### 12. DiskANN Vector Search

**Files:** `include/performance/phase3/diskann.h`

**Source:**
- Paper: Subramanya, S. J., et al. (2019)  
  "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node"  
  NeurIPS '19
- Company: Microsoft Research
- URL: https://papers.nips.cc/paper/2019/hash/09853c7fb1d3f8ee67a61b6bf4a7f8e6-Abstract.html

**ThemisDB Implementation:**
- Disk-based vector index for billion-scale datasets
- SSD-optimized data layout
- Integration with RocksDB

---

### Serialization & Data Formats

#### 13. Binary Serialization (VelocyPack/MessagePack-inspired)

**Files:** `include/utils/serialization.h`

**Source:**
- Inspired by:
  - **VelocyPack**: https://github.com/arangodb/velocypack (Apache 2.0)
  - **MessagePack**: https://msgpack.org/ (Apache 2.0)

**ThemisDB Implementation:**
- Custom binary format optimized for:
  - Compact representation
  - Fast encoding/decoding
  - Native float vector support for embeddings
  - Zero-copy operations where possible
- Not a direct copy - custom implementation with ThemisDB-specific optimizations

---

## Library Integrations

### Core Dependencies

| Library | Purpose | License | Files |
|---------|---------|---------|-------|
| **RocksDB** | Storage engine | Apache 2.0 / GPL 2.0 | `storage/rocksdb_wrapper.*` |
| **FAISS** | Vector search | MIT | `acceleration/faiss_gpu_backend.*` |
| **hnswlib** | HNSW index | Apache 2.0 | `index/vector_index.*` |
| **llama.cpp** | LLM inference | MIT | `llm/llama_wrapper.*` |
| **nlohmann/json** | JSON parsing | MIT | Throughout |
| **simdjson** | Fast JSON | Apache 2.0 | `utils/json_parser.*` |
| **Apache Arrow** | Columnar format | Apache 2.0 | `analytics/*` |
| **OpenSSL** | TLS/Crypto | Apache 2.0 | `security/*` |
| **Boost.Asio** | Networking | Boost License | `network/*` |
| **spdlog** | Logging | MIT | Throughout |
| **Google Test** | Testing | BSD-3-Clause | `tests/*` |
| **Google Benchmark** | Benchmarks | Apache 2.0 | `benchmarks/*` |

See [docs/de/legal/ATTRIBUTIONS.md](docs/de/legal/ATTRIBUTIONS.md) for complete list.

---

## Design Inspirations

### Multi-Model Databases

1. **ArangoDB** - Multi-model architecture concept
   - URL: https://www.arangodb.com/
   - License: Apache 2.0
   - Inspiration: Unified query language (AQL), multi-model storage
   - ThemisDB Difference: True unified storage vs separate engines

2. **CozoDB** - Hybrid relational-graph-vector design
   - URL: https://github.com/cozodb/cozo
   - License: MPL-2.0
   - Inspiration: Datalog-based queries, hybrid model support
   - ThemisDB Difference: ACID across all models, integrated LLM

3. **Azure Cosmos DB** - Multi-model with unified API
   - Company: Microsoft
   - Inspiration: Multi-API support, global distribution
   - ThemisDB Difference: Open source, native LLM, lower cost

---

### Time-Series Databases

4. **TimescaleDB** - Hypertable concept
   - URL: https://www.timescale.com/
   - License: Timescale License (Apache 2.0 compatible)
   - Inspiration: Chunk-based partitioning, time-series optimization
   - ThemisDB Difference: RocksDB-based vs PostgreSQL-based

5. **InfluxDB** - Time-series optimization
   - URL: https://www.influxdata.com/
   - License: MIT (core)
   - Inspiration: Tag-based indexing, retention policies

---

### Distributed Systems

6. **Apache Cassandra** - Gossip protocol, distributed architecture
   - URL: https://cassandra.apache.org/
   - License: Apache 2.0
   - Inspiration: Gossip-based membership, consistent hashing
   - ThemisDB Difference: VCC-URN vs token-based partitioning

7. **Google Spanner** - TrueTime concept
   - Company: Google
   - Paper: https://research.google/pubs/pub39966/
   - Inspiration: Distributed transactions, external consistency
   - ThemisDB Status: TrueTime-inspired clock (experimental)

---

### Analytics Databases

8. **DuckDB** - Embedded analytics engine
   - URL: https://duckdb.org/
   - License: MIT
   - Inspiration: Embedded analytics, columnar processing

9. **Apache Druid** - Real-time analytics
   - URL: https://druid.apache.org/
   - License: Apache 2.0
   - Inspiration: Time-series aggregates, roll-ups

---

## Academic Paper References

### Complete Citation List

1. **HNSW**  
   Malkov & Yashunin (2018), IEEE TPAMI

2. **FAISS**  
   Johnson, Douze & Jégou (2019), IEEE Trans. Big Data

3. **Product Quantization**  
   Jégou, Douze & Schmid (2011), IEEE TPAMI

4. **Gorilla Compression**  
   Pelkonen et al. (2015), VLDB

5. **Raft Consensus**  
   Ongaro & Ousterhout (2014), USENIX ATC

6. **Gossip Protocol**  
   van Renesse, Birman & Vogels (2003), ACM TOCS

7. **FlashAttention**  
   Dao et al. (2022, 2023), arXiv

8. **LoRA**  
   Hu et al. (2021), arXiv

9. **Dostoevsky LSM**  
   Dayan & Idreos (2018), SIGMOD

10. **DiskANN**  
    Subramanya et al. (2019), NeurIPS

11. **TimescaleDB**  
    Freedman et al. (2017), Technical Report

12. **Google Spanner**  
    Corbett et al. (2012), OSDI

---

## Attribution Guidelines

### For Contributors

When adding new code inspired by external sources:

1. **Add inline comments** with source attribution
2. **Update this document** with the new source
3. **Include paper DOI/URL** if applicable
4. **Verify license compatibility** with MIT
5. **Document adaptations** made for ThemisDB

### Citation Format

```cpp
/**
 * @brief Feature description
 * 
 * @sources
 * - Algorithm: [Name]
 * - Paper: [Authors] ([Year]) "[Title]" [Conference/Journal]
 * - URL: [Link]
 * - License: [License]
 * - ThemisDB Adaptation: [What we changed]
 */
```

---

## License Compatibility

All external code and algorithms used in ThemisDB are compatible with the MIT License:

- ✅ MIT License
- ✅ Apache 2.0
- ✅ BSD Licenses
- ✅ Boost Software License
- ✅ Freely implementable algorithms (Raft, HNSW, etc.)
- ⚠️ GPL 2.0 (RocksDB) - Dual-licensed with Apache 2.0

See [LICENSE](LICENSE) for ThemisDB's license terms.

---

## Conclusion

**ThemisDB honors its sources** while creating unique value:

- **We build on proven algorithms** from academic research
- **We integrate best-in-class libraries** from the open-source ecosystem
- **We create innovations** where existing solutions don't meet our needs
- **We attribute transparently** to honor the work of others

> *"We don't take credit for others' work – we build on it and create something new."*

---

**ThemisDB – Built with ❤️ for the database community**

*Standing on the shoulders of giants, reaching for new heights.*
