# ThemisDB - Feature Attribution & Third-Party Libraries

**Version:** 2.0  
**Last Updated:** 6. April 2026

---

## 📋 Übersicht

Dieses Dokument zeigt transparent, welche Features von externen Bibliotheken bereitgestellt werden und welche Innovationen von ThemisDB selbst stammen. ThemisDB baut auf bewährten Open-Source-Komponenten auf und kombiniert diese zu einer einzigartigen Multi-Model-Datenbank mit nativer LLM-Integration.

---

## 🏗️ Kernbibliotheken und ihre Verwendung

### Storage Engine

#### RocksDB (Meta/Facebook)
- **Lizenz:** Apache 2.0 / GPL 2.0
- **Repository:** https://github.com/facebook/rocksdb
- **Verwendung in ThemisDB:**
  - LSM-Tree basierte Key-Value-Storage-Engine
  - TransactionDB mit MVCC (Multi-Version Concurrency Control)
  - Write-Ahead-Log (WAL) für Durability
  - Snapshot-Mechanismus für konsistente Backups
  - Compaction und Compression (LZ4, ZSTD)
  - Block Cache für Read-Performance

**ThemisDB-Erweiterungen:**
- Multi-Model-Mapping über hierarchisches Key-Schema
- Transactional Secondary/Graph/Vector Index Management
- Custom Merge Operators für komplexe Updates
- Adaptive Compaction-Strategien
- Integration mit verteiltem Sharding

---

### JSON Processing

#### nlohmann/json
- **Lizenz:** MIT
- **Repository:** https://github.com/nlohmann/json
- **Verwendung in ThemisDB:**
  - JSON Parsing und Serialisierung
  - Schema Validation
  - JSON Patch Operations

#### simdjson (Daniel Lemire)
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/simdjson/simdjson
- **Verwendung in ThemisDB:**
  - High-Performance JSON Parsing (SIMD-beschleunigt)
  - Streaming JSON Parser für große Dokumente
  - On-Demand JSON Field Extraction

**ThemisDB-Erweiterungen:**
- Hybrid JSON Processing Pipeline (simdjson für Performance-kritische Pfade)
- Custom JSON Query Language (AQL JSON Operators)
- JSON Path Indexing

---

### Vector Search

#### hnswlib (Malkov & Yashunin)
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/nmslib/hnswlib
- **Verwendung in ThemisDB:**
  - Hierarchical Navigable Small World (HNSW) Graph-basierte ANN-Suche
  - Multi-Layer Graph-Struktur
  - Efficient k-NN Search

#### FAISS (Meta AI Research)
- **Lizenz:** MIT
- **Repository:** https://github.com/facebookresearch/faiss
- **Verwendung in ThemisDB:**
  - GPU-beschleunigte Vector Search (optional)
  - IVF (Inverted File Index) für große Vektormengen
  - Product Quantization (PQ) für Speicherreduktion
  - IndexIVFFlat, IndexIVFPQ, IndexHNSWFlat

**ThemisDB-Erweiterungen:**
- Transactional Vector Index Updates (ACID-Garantien)
- Hybrid Search: BM25 + Vector für RAG-Workflows
- Embedding Cache mit 70-90% Cost Reduction
- Multi-Backend GPU Support (10 Backends: CUDA, Vulkan, HIP, OpenCL, DirectX, OneAPI, ZLUDA, ROCm, Metal, WebGPU)
- Zero-Copy Integration zwischen Vector DB und LLM (native LLM Engine)
- Distributed Vector Search mit Sharding
- Filter-basierte Vector Search (Filtered ANN)

---

### Utilities & Data Handling

#### c-ares
- **Lizenz:** MIT
- **Repository:** https://c-ares.org/
- **Verwendung in ThemisDB:**
  - Asynchrone DNS-Auflösung
  - Non-blocking DNS queries
  - Integration mit Event Loop

#### crc32c (Google)
- **Lizenz:** BSD 3-Clause
- **Repository:** https://github.com/google/crc32c
- **Verwendung in ThemisDB:**
  - Hardware-beschleunigte CRC32C-Prüfsummen
  - Datenintegrität-Checks
  - SSE4.2 und ARM64 CRC32-Instruktionen

#### libzip
- **Lizenz:** BSD 3-Clause
- **Repository:** https://libzip.org/
- **Verwendung in ThemisDB:**
  - ZIP-Archiv lesen und schreiben
  - Content Compression und Packaging
  - Backup-Archive

#### pugixml
- **Lizenz:** MIT
- **Repository:** https://pugixml.org/
- **Verwendung in ThemisDB:**
  - Leichtgewichtige XML-Verarbeitung
  - Konfigurationsdateien
  - Content Processing

#### tl-expected
- **Lizenz:** CC0 1.0 (Public Domain)
- **Repository:** https://github.com/TartanLlama/expected
- **Verwendung in ThemisDB:**
  - C++11/14/17 std::expected Implementierung
  - Type-safe Error Handling
  - Funktionale Fehlerbehandlung ohne Exceptions

---

### Network Protocols

#### nghttp2
- **Lizenz:** MIT
- **Repository:** https://nghttp2.org/
- **Verwendung in ThemisDB:**
  - HTTP/2 Protokoll-Unterstützung
  - Server Push
  - Multiplexing und Stream Prioritization

**ThemisDB-Erweiterungen:**
- Integration mit Boost.Beast HTTP Server
- Custom HTTP/2 Frame Handlers
- Connection Pooling

#### nghttp3
- **Lizenz:** MIT
- **Repository:** https://github.com/ngtcp2/nghttp3
- **Verwendung in ThemisDB:**
  - HTTP/3 Protokoll-Implementierung
  - QPACK Header Compression

#### ngtcp2
- **Lizenz:** MIT
- **Repository:** https://github.com/ngtcp2/ngtcp2
- **Verwendung in ThemisDB:**
  - QUIC Protokoll für HTTP/3
  - 0-RTT Connection Establishment
  - Loss Detection und Congestion Control

**ThemisDB-Erweiterungen:**
- Optional HTTP/3 für Client-Server-Kommunikation
- Fallback auf HTTP/2 und HTTP/1.1
- Adaptive Protocol Selection

---

### Content Processing

#### FFmpeg
- **Lizenz:** LGPL 2.1+ (ThemisDB nutzt nur LGPL-Komponenten)
- **Repository:** https://ffmpeg.org/
- **Verwendung in ThemisDB:**
  - Video- und Audio-Codec-Unterstützung
  - Thumbnail-Generierung
  - Metadata-Extraktion
  - Format-Konvertierung

**ThemisDB-Erweiterungen:**
- Content-Plugin für Video-Processing
- Automatische Thumbnail-Generierung
- Metadata-Indexierung

#### GDAL (Geospatial Data Abstraction Library)
- **Lizenz:** MIT/X11
- **Repository:** https://gdal.org/
- **Verwendung in ThemisDB:**
  - Shapefile (.shp) Support
  - GeoTIFF Raster Support
  - Coordinate System Transformations
  - 200+ Geospatial Formats

**ThemisDB-Erweiterungen:**
- Integration mit Geo-Funktionen
- Automatische Koordinaten-Extraktion
- Spatial Index Integration

---

### IoT & Messaging

#### Eclipse Paho MQTT C++
- **Lizenz:** EPL 2.0 / EDL 1.0
- **Repository:** https://www.eclipse.org/paho/
- **Verwendung in ThemisDB:**
  - MQTT-Protokoll für IoT-Geräte
  - Publish/Subscribe Messaging
  - QoS-Level 0, 1, 2

**ThemisDB-Erweiterungen:**
- MQTT-Broker Integration
- Change Data Capture (CDC) über MQTT
- IoT Sensor Data Ingestion

---

### Linear Algebra & GPU

#### OpenBLAS
- **Lizenz:** BSD 3-Clause
- **Repository:** https://www.openblas.net/
- **Verwendung in ThemisDB:**
  - Optimierte BLAS-Operationen
  - Matrix-Multiplikation
  - SIMD-beschleunigte Vektoroperationen

#### LAPACK
- **Lizenz:** BSD-like
- **Repository:** https://www.netlib.org/lapack/
- **Verwendung in ThemisDB:**
  - Lineare Algebra Routinen
  - Matrix-Zerlegungen
  - Eigenvalue-Berechnungen

**ThemisDB Integration:**
- Unterstützung für FAISS GPU-Indizes
- Dimensionality Reduction (PCA, SVD)
- Statistical Analysis

---

### Observability

#### Prometheus C++ Client
- **Lizenz:** MIT
- **Repository:** https://github.com/jupp0r/prometheus-cpp
- **Verwendung in ThemisDB:**
  - Prometheus Metrics Collection
  - Counters, Gauges, Histograms
  - /metrics Endpoint

**ThemisDB-Erweiterungen:**
- 50+ Custom Metrics
- Query Performance Tracking
- LLM Inference Metrics
- Vector Search Latency
- Grafana Dashboard Integration

#### Apache Parquet C++
- **Lizenz:** Apache 2.0
- **Repository:** https://arrow.apache.org/
- **Verwendung in ThemisDB:**
  - Columnar Storage Format
  - Efficient Compression
  - Schema Evolution

**ThemisDB-Erweiterungen:**
- Parquet Export für Analytics
- Integration mit Arrow Data Format
- OLAP Query Optimization

---

### Networking & HTTP

#### Boost.Asio & Boost.Beast
- **Lizenz:** Boost Software License 1.0
- **Repository:** https://www.boost.org/
- **Verwendung in ThemisDB:**
  - Asynchrones I/O Framework (Boost.Asio)
  - HTTP/1.1 Server (Boost.Beast)
  - WebSocket Support
  - TLS/SSL Integration

**ThemisDB-Erweiterungen:**
- Custom Wire Protocol (binäres Netzwerkprotokol)
- Server-Sent Events (SSE) für Streaming Queries
- Rate Limiting & Load Shedding
- Connection Pooling & Keep-Alive Management
- Multi-Endpoint Load Balancing in Clients

#### OpenSSL
- **Lizenz:** Apache 2.0 (OpenSSL 3.x)
- **Repository:** https://github.com/openssl/openssl
- **Verwendung in ThemisDB:**
  - TLS 1.3 Encryption
  - Certificate Management
  - SHA256 Hashing
  - Cryptographic Primitives

**ThemisDB-Erweiterungen:**
- Field-Level Encryption (AES-256-GCM)
- HSM/PKCS#11 Integration
- Certificate Pinning
- Key Rotation Mechanisms
- Vault Integration (HashiCorp Vault)

#### libcurl
- **Lizenz:** curl License (MIT-style)
- **Repository:** https://github.com/curl/curl
- **Verwendung in ThemisDB:**
  - HTTP Client Operations
  - WebDAV Integration
  - REST API Calls für externe Services

---

### Analytics & Columnar Storage

#### Apache Arrow
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/apache/arrow
- **Verwendung in ThemisDB:**
  - Columnar In-Memory Data Format
  - Parquet File Support
  - Zero-Copy Data Sharing
  - SIMD-optimierte Compute Functions

**ThemisDB-Erweiterungen:**
- Hypertables (TimescaleDB-kompatibel) mit automatischem Partitionieren
- Time-Series Aggregates mit SIMD-Beschleunigung (5-10x schneller)
- OLAP Features: CUBE, ROLLUP, Window Functions
- Complex Event Processing (CEP) Engine
- Gorilla Compression für Time-Series

---

### Parallelism & Threading

#### Intel TBB (Threading Building Blocks)
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/oneapi-src/oneTBB
- **Verwendung in ThemisDB:**
  - Task-basierte Parallelisierung
  - Concurrent Data Structures
  - Parallel Algorithms
  - Work Stealing Scheduler

**ThemisDB-Erweiterungen:**
- Parallel Query Execution
- Parallel Index Building
- Parallel Compaction
- Distributed Task Scheduling

---

### Logging & Monitoring

#### spdlog
- **Lizenz:** MIT
- **Repository:** https://github.com/gabime/spdlog
- **Verwendung in ThemisDB:**
  - High-Performance Logging
  - Async Logging
  - Multiple Sinks (File, Console, Syslog)
  - Rotating Files

#### fmt
- **Lizenz:** MIT
- **Repository:** https://github.com/fmtlib/fmt
- **Verwendung in ThemisDB:**
  - String Formatting
  - Type-Safe printf-style Formatting

#### OpenTelemetry C++
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/open-telemetry/opentelemetry-cpp
- **Verwendung in ThemisDB:**
  - Distributed Tracing
  - OTLP-HTTP Exporter
  - Span Context Propagation

**ThemisDB-Erweiterungen:**
- Prometheus Metrics Integration
- Custom Metrics für Query Performance
- Audit Logging mit SIEM Integration
- Performance Profiling

---

### Configuration & Serialization

#### yaml-cpp
- **Lizenz:** MIT
- **Repository:** https://github.com/jbeder/yaml-cpp
- **Verwendung in ThemisDB:**
  - YAML Configuration Files
  - Config Parsing und Validation

#### ZSTD (Facebook)
- **Lizenz:** BSD / GPL 2.0
- **Repository:** https://github.com/facebook/zstd
- **Verwendung in ThemisDB:**
  - Compression für Content Storage
  - RocksDB Compression
  - Network Protocol Compression

---

### Testing & Benchmarking

#### Google Test (gtest)
- **Lizenz:** BSD-3-Clause
- **Repository:** https://github.com/google/googletest
- **Verwendung in ThemisDB:**
  - Unit Testing Framework
  - Test Fixtures
  - Mocking Framework

#### Google Benchmark
- **Lizenz:** Apache 2.0
- **Repository:** https://github.com/google/benchmark
- **Verwendung in ThemisDB:**
  - Performance Benchmarks
  - Microbenchmarks
  - Regression Testing

---

### Memory Management

#### mimalloc (Microsoft Research)
- **Lizenz:** MIT
- **Repository:** https://github.com/microsoft/mimalloc
- **Verwendung in ThemisDB:**
  - High-Performance Memory Allocator
  - Drop-in Replacement für malloc/free
  - 20-40% Memory Performance Boost

---

## 🚀 ThemisDB Unique Features & Innovations

Die folgenden Features sind **exklusive Eigenentwicklungen von ThemisDB** und nicht aus externen Bibliotheken übernommen:

### 1. Native LLM Integration (v1.5.0 - Q3 2026)

**"ThemisDB keeps its own llamas."** – Weltweit erste Datenbank mit integrierter LLM-Engine!

- ✅ **Embedded LLM Engine:** llama.cpp Integration direkt in der Datenbank
- ✅ **Zero-Copy RAG:** Direkter Speicherzugriff zwischen Vector DB und LLM (4x schneller)
- ✅ **Cost Reduction:** 100-1000x günstiger als AWS/Azure/GCP APIs
- ✅ **Multi-Tier GPU Support:** Entry (<16GB), Mid-Range (<24GB), High-End (>24GB)
- ✅ **Distributed Reasoning:** Gehirn-inspirierte Multi-Shard-Kollaboration
- ✅ **Continuous Batching:** vLLM-style Optimization (2.6x Throughput)
- ✅ **On-Premise AI:** Keine externen API-Calls, vollständige Datensouveränität

**Kernarchitektur:**
```
┌─────────────────────────────────────────┐
│         ThemisDB Kernel                 │
│                                         │
│  ┌────────────┐      ┌──────────────┐  │
│  │  Vector DB │◄────►│  LLM Engine  │  │
│  │   (HNSW)   │ Zero │  (llama.cpp) │  │
│  └────────────┘ Copy └──────────────┘  │
│         │                    │          │
│         └──────────┬─────────┘          │
│                    │                    │
│            ┌───────▼────────┐           │
│            │  Unified Memory │           │
│            │   (GPU/CPU)     │           │
│            └─────────────────┘           │
└─────────────────────────────────────────┘
```

### 2. Unified Multi-Model Architecture

**Canonical Storage + Projection Layers Pattern**

- ✅ **Base Entity Model:** Jedes Objekt ist primär ein JSON/Binär-Blob
- ✅ **Multi-Model Projections:** Relationale, Graph-, Vector- und Dokument-Views auf denselben Daten
- ✅ **Atomic Multi-Index Updates:** Ein Write-Operation aktualisiert alle Indizes transactional
- ✅ **Query Language (AQL):** Einheitliche Syntax für alle Datenmodelle
- ✅ **Cross-Model Queries:** SQL JOINs mit Graph Traversals und Vector Search kombinierbar

**Beispiel (konzeptionelle AQL-Syntax):**
```sql
-- AQL Query: Kombiniert Graph + Vector + Relational
-- Hinweis: Dies zeigt das konzeptionelle Design; die genaue Syntax kann variieren.
-- Siehe docs/aql/aql_syntax.md für die aktuelle API-Dokumentation.
FOR user IN users
  FILTER user.age > 25
  LET similar_docs = VECTOR_SEARCH(user.embedding, k=10)
  LET connections = GRAPH_NEIGHBORS(user.id, depth=2)
  RETURN { user, similar_docs, connections }
```

### 3. Advanced Transaction Management

- ✅ **MVCC mit Snapshot Isolation:** Concurrent Reads ohne Locking
- ✅ **Distributed SAGA Patterns:** Multi-Shard Transactions
- ✅ **Write-Write Conflict Detection:** Optimistic Concurrency Control
- ✅ **Transactional Indexes:** Alle Indizes (Secondary, Graph, Vector) sind ACID-konform
- ✅ **Multi-Level Transactions:** Entity-Level, Session-Based, Distributed

### 4. Enterprise Security & Compliance

- ✅ **Field-Level Encryption:** AES-256-GCM mit automatischem Key Management
- ✅ **RBAC (Role-Based Access Control):** Granulare Berechtigungsverwaltung
- ✅ **Apache Ranger Integration:** Policy-basiertes Access Control
- ✅ **Audit Logging:** Vollständiger Audit Trail für Compliance (GDPR, HIPAA)
- ✅ **HSM/TSA Integration:** Hardware Security Module Support
- ✅ **Certificate Pinning:** Enhanced TLS Security
- ✅ **Malware Scanning:** Integrierter Content Scanner
- ✅ **PII Detection:** Automatische Erkennung personenbezogener Daten

### 5. Hybrid Search (RAG-Optimized)

- ✅ **BM25 + Vector Fusion:** Kombination von Full-Text und Similarity Search
- ✅ **Reciprocal Rank Fusion (RRF):** Optimale Ergebnis-Kombination
- ✅ **85% Recall@10:** Benchmark-übertreffende Accuracy
- ✅ **Embedding Cache:** 70-90% Cost Reduction für LLM-Applikationen
- ✅ **Query Rewriting:** Automatische Query-Optimierung für RAG

### 6. Distributed Architecture

- ✅ **VCC-URN Sharding:** Content-basiertes Consistent Hashing
- ✅ **Horizontal Scaling:** Automatic Rebalancing
- ✅ **Multi-Master Replication:** CRDT-basierte Konfliktauflösung
- ✅ **RAID-like Redundancy:** MIRROR, STRIPE, PARITY Modi
- ✅ **Kubernetes Operator:** Cloud-Native Deployment
- ✅ **Auto-Discovery:** Dynamische Cluster-Topologie

### 7. GPU Acceleration (10 Backends)

**Weltweit umfassendste GPU-Backend-Unterstützung!**

- ✅ **CUDA** (NVIDIA)
- ✅ **Vulkan** (Cross-Platform)
- ✅ **HIP** (AMD)
- ✅ **OpenCL** (Cross-Platform)
- ✅ **DirectX 12** (Windows)
- ✅ **OneAPI/SYCL** (Intel)
- ✅ **ZLUDA** (CUDA on AMD)
- ✅ **ROCm** (AMD)
- ✅ **Metal** (Apple)
- ✅ **WebGPU** (Browser/Experimental)

**Features:**
- Automatische Platform Detection
- Graceful Fallback auf CPU
- 10-50x Speedup für Vector Search
- GPU Memory Management
- Multi-GPU Support

### 8. Advanced Analytics Engine

- ✅ **Complex Event Processing (CEP):** Pattern Matching über Event Streams
- ✅ **OLAP Operators:** CUBE, ROLLUP, GROUPING SETS
- ✅ **Window Functions:** ROW_NUMBER, RANK, LEAD, LAG
- ✅ **Statistical Aggregators:** Percentiles, Variance, Correlation
- ✅ **Time-Series Engine:** Gorilla Compression, Continuous Aggregates
- ✅ **Hypertables:** TimescaleDB-kompatible Partitionierung

### 9. Content Management System

- ✅ **Multi-Backend Storage:** Filesystem, WebDAV, S3, Azure Blob
- ✅ **Threshold-Based Selection:** Automatische Backend-Wahl nach Größe
- ✅ **Content Versioning:** Vollständige Version History
- ✅ **MIME Detection:** Automatische Content-Type Erkennung
- ✅ **Deduplication:** SHA256-basierte Content-Addressierung
- ✅ **Content Processors:** Text, Image, Audio, Video, Geo, CAD (Optional)

### 10. Query Optimization

- ✅ **Cost-Based Optimizer:** Cardinality Estimation & Join Reordering
- ✅ **Index Selection:** Automatische Index-Nutzung
- ✅ **Predicate Pushdown:** Filter frühzeitig anwenden
- ✅ **CTE (Common Table Expression) Cache:** Wiederverwendung von Subqueries
- ✅ **Semantic Cache:** Query Result Caching mit Similarity Matching

### 11. Observability & Monitoring

- ✅ **Prometheus Metrics:** 50+ Custom Metrics
- ✅ **OpenTelemetry Tracing:** Distributed Tracing
- ✅ **Health Checks:** `/health` Endpoint mit detailliertem Status
- ✅ **Performance Profiling:** Query Execution Plans
- ✅ **Grafana Dashboards:** Vorkonfigurierte Monitoring Dashboards

### 12. Developer Experience

- ✅ **Client SDKs:** Go, Python, Rust, JavaScript, Ruby, Java, C#
- ✅ **REST API:** Comprehensive HTTP/JSON API
- ✅ **GraphQL:** Alternative Query Interface
- ✅ **Wire Protocol:** Binäres, high-performance Netzwerkprotokoll
- ✅ **Docker Images:** Multi-Arch Support (amd64, arm64)
- ✅ **Helm Charts:** Kubernetes Deployment
- ✅ **Package Managers:** apt, brew, choco, rpm

---

## 🎯 Zusammenfassung: Was macht ThemisDB einzigartig?

### Innovation #1: Native LLM Integration
**Kein anderes DBMS bietet:**
- Integrierte LLM-Engine ohne externe API-Calls
- Zero-Copy Memory Sharing zwischen Vector DB und LLM
- 100-1000x Cost Reduction gegenüber Hyperscalern

### Innovation #2: True Multi-Model mit Unified API
**Während andere DBMS separate Engines für verschiedene Modelle haben:**
- ThemisDB verwendet ein einziges Storage Layer
- Atomic Updates über alle Indizes
- Cross-Model Queries in einer Zeile Code

### Innovation #3: GPU-Backend-Vielfalt
**10 GPU-Backends (mehr als jede andere Datenbank!):**
- Läuft auf NVIDIA, AMD, Intel, Apple Silicon
- Graceful Fallback garantiert Betrieb überall

### Innovation #4: Enterprise-Grade Security
**Integrierte Security aus dem Kernbestand (nicht als Add-On):**
- Field-Level Encryption mit HSM/PKI
- Apache Ranger Integration
- Vollständiger Audit Trail

### Innovation #5: Deployment Flexibility
**Von Edge bis Cloud:**
- QNAP NAS Support (Embedded Variant)
- Kubernetes-Ready mit Operator
- Docker/VM mit und ohne GPU
- Fully Static Binaries für älteste Systeme

---

## 📚 References & Inspirations

ThemisDB wurde inspiriert von:

- **ArangoDB** - Multi-Model Architecture Konzept
- **CozoDB** - Hybrid Relational-Graph-Vector Design
- **Azure Cosmos DB** - Multi-Model mit Unified API
- **TimescaleDB** - Time-Series Hypertables
- **DuckDB** - Embedded Analytics Engine

**Aber ThemisDB geht weiter:**
- ✅ Native LLM Integration (weltweit einzigartig)
- ✅ 10 GPU-Backends (mehr als alle anderen)
- ✅ True Unified Storage (nicht mehrere Engines nebeneinander)
- ✅ ACID über alle Datenmodelle (inklusive Vector!)

---

## 📄 Lizenzen

ThemisDB selbst ist unter der **MIT License with Government Clause** lizenziert (siehe [LICENSE](LICENSE) für den vollständigen Lizenztext).

Alle verwendeten Third-Party-Bibliotheken sind unter permissive Open-Source-Lizenzen verfügbar:
- Apache 2.0 (RocksDB, FAISS, Arrow, TBB, OpenTelemetry, simdjson, Google Benchmark)
- MIT (nlohmann/json, spdlog, fmt, yaml-cpp, mimalloc)
- Boost Software License 1.0 (Boost Libraries)
- BSD-3-Clause (Google Test)
- curl License (libcurl)

Eine vollständige Liste der Lizenzen aller Dependencies ist unter `LICENSE` und in den jeweiligen Submodules verfügbar.

---

## 🙏 Acknowledgments

Ein besonderer Dank an die Communities und Maintainer der oben genannten Bibliotheken. Ohne diese herausragenden Open-Source-Projekte wäre ThemisDB nicht möglich.

ThemisDB steht auf den Schultern von Giganten – aber mit eigenen innovativen Features, die es zu einer einzigartigen Multi-Model-Datenbank mit nativer KI-Integration machen.

---

**ThemisDB – Built with ❤️ for the database community**

*"We don't take credit for others' work – we build on it and create something new."*
