# ThemisDB: Die Entwicklung einer Multi-Model-Datenbank

## Technisches Buch - Struktur und Aufbau

**Version:** 1.0.0  
**Stand:** Dezember 2025  
**Autor:** ThemisDB Development Team  
**Zielgruppe:** Datenbankentwickler, Softwarearchitekten, Studierende der Informatik

---

## Über dieses Buch

Dieses Buch dokumentiert die vollständige Entwicklung von ThemisDB - von der initialen Idee bis zur produktionsreifen Multi-Model-Datenbank. Es bietet einen tiefen Einblick in Architekturentscheidungen, Implementierungsdetails und gewonnene Erkenntnisse während der Entwicklung eines modernen Datenbanksystems.

**Wissenschaftlicher Ansatz:**
Das Buch verfolgt einen wissenschaftlich fundierten Ansatz und stellt ThemisDB im Kontext der aktuellen Datenbankforschung und -entwicklung dar:

- **Akademische Fundierung**: Jedes Kapitel referenziert relevante wissenschaftliche Papers und Forschungsarbeiten
- **Vergleichende Analyse**: Systematischer Vergleich mit etablierten Datenbanksystemen (PostgreSQL, MongoDB, Neo4j, ArangoDB, etc.)
- **Begründete Entscheidungen**: Detaillierte Erklärung und Rechtfertigung aller Design-Entscheidungen
- **State-of-the-Art Einordnung**: Positionierung von ThemisDB im Kontext aktueller Entwicklungen und Trends
- **Kritische Reflexion**: Ehrliche Diskussion von Trade-offs, Limitierungen und alternativen Ansätzen

## Zielgruppen

1. **Datenbankentwickler**: Detaillierte technische Implementierung und Code-Beispiele
2. **Softwarearchitekten**: Architekturentscheidungen und Design-Patterns
3. **Studierende**: Praktische Anwendung von Datenbanktheorie
4. **DevOps-Engineers**: Deployment, Skalierung und Operations

---

## Buchstruktur - Logischer Aufbau

Das Buch folgt einer logischen Progression vom Konzept zur Implementierung:

### TEIL I: Grundlagen und Motivation (Kapitel 1-3)
Warum ThemisDB entwickelt wurde und welche Probleme es löst

### TEIL II: Architektur und Design (Kapitel 4-7)
Fundamentale Architekturentscheidungen und Systemdesign

### TEIL III: Kern-Komponenten (Kapitel 8-12)
Implementierung der essentiellen Datenbankkomponenten

### TEIL IV: Multi-Model-Fähigkeiten (Kapitel 13-17)
Spezifische Datenmodelle und deren Integration

### TEIL V: Enterprise-Features (Kapitel 18-22)
Produktionsreife Features für den Unternehmenseinsatz

### TEIL VI: Ecosystem und Zukunft (Kapitel 23-25)
Client-SDKs, Tools und Ausblick

---

## Detailliertes Inhaltsverzeichnis

### **TEIL I: GRUNDLAGEN UND MOTIVATION**

#### **Kapitel 1: Einführung in ThemisDB**

**1.1 Die Evolution der Datenbanksysteme**
- Von hierarchischen DBs zu relationalen Systemen (1970er-1980er)
- NoSQL-Bewegung und die Fragmentierung (2000er)
- Multi-Model als nächste Generation (2010er+)
- Historischer Kontext und technologische Treiber

**1.2 Die Problemstellung: Polyglot Persistence**
- Fragmentierte Datenbanklandschaft in modernen Anwendungen
- Operationelle Komplexität: Multiple Systeme betreiben
- Daten-Silos und Konsistenz-Probleme
- TCO (Total Cost of Ownership) Analyse
- Reale Use Cases und Pain Points

**1.3 Die Vision: Konvergenz statt Fragmentierung**
- Warum Multi-Model? Theoretische Grundlage
- Unified Data Model vs. Native Multi-Model
- ThemisDB Design-Philosophie: "One Storage, Many Views"
- Inspiration und Learnings aus bestehenden Systemen
  - ArangoDB: Multi-Model Pioneer
  - OrientDB: Graph + Document
  - Azure CosmosDB: Global Distribution
  - Was ThemisDB anders macht

**1.4 Kernanforderungen und Design-Ziele**
- Funktionale Anforderungen
  - Multi-Model Support (Document, Graph, Vector, TimeSeries, Geo)
  - ACID-Transaktionen über alle Modelle
  - Flexible Query-Sprache (AQL)
  - Horizontale Skalierbarkeit
- Nicht-funktionale Anforderungen
  - Performance: Sub-ms Latency, >50K ops/sec
  - Skalierbarkeit: Linear bis 1000+ Nodes
  - Verfügbarkeit: 99.99% Uptime
  - Sicherheit: Enterprise-Grade Security
- Trade-offs und Priorisierung
  - CP vs. AP im CAP-Theorem
  - Consistency vs. Performance
  - Simplicity vs. Features

**1.5 Projektumfang und Abgrenzung**
- Was ThemisDB IST
  - Multi-Model Datenbank für moderne Anwendungen
  - Production-ready, Enterprise-fähig
  - Developer-friendly mit SDKs und Tools
- Was ThemisDB NICHT IST
  - Kein Data Warehouse / OLAP System (primär)
  - Kein Stream Processing Engine
  - Kein ML Platform
- Roadmap und zukünftige Erweiterungen
  - Phase 1: Core Features (v1.0) ✅
  - Phase 2: Advanced Analytics (v1.x)
  - Phase 3: AI/ML Integration (v2.0)

**1.6 Vergleichende Positionierung**
- Marktübersicht: Multi-Model Datenbanken
  - ArangoDB: Community vs. Enterprise
  - OrientDB: Graph-fokussiert
  - Azure CosmosDB: Cloud-native
  - FaunaDB: Serverless
  - ThemisDB: Self-hosted, Open-Source-orientiert
- Feature-Matrix und Differenzierung
- Performance-Benchmarks (YCSB, TPC-C)
- Deployment-Modelle und Licensing

**Referenzdokumente:**
- `README.md`
- `docs/architecture/architecture_overview.md`
- `docs/reports/themis_sachstandsbericht_2025.md`
- `docs/reports/competitive_gap_analysis.md`

**Akademische Referenzen:**
- Stonebraker, M., Cetintemel, U. (2005). "One Size Fits All: An Idea Whose Time Has Come and Gone"
- Abadi, D. (2010). "Consistency Tradeoffs in Modern Distributed Database System Design"
- Lu, J., et al. (2019). "Multi-model Databases: A New Journey to Handle the Variety of Data"

---

#### **Kapitel 2: Theoretische Grundlagen**

**2.1 Log-Structured Merge Trees (LSM-Trees)**
- Originalpaper und theoretische Fundierung
  - O'Neil, P., Cheng, E., Gawlick, D., O'Neil, E. (1996). "The Log-Structured Merge-Tree (LSM-Tree)"
  - Komplexitätsanalyse: Write O(1), Read O(log N)
- Moderne Implementierungen im Vergleich
  - LevelDB (Google, 2011): Basis-Implementation
  - RocksDB (Facebook/Meta, 2012): Enterprise-optimiert
  - Cassandra (Apache, 2008): Distributed LSM
  - ScyllaDB: C++ Re-implementation von Cassandra
- B-Tree vs. LSM Trade-offs
  - Seltzer, M., Bostic, K. (1991). "An Implementation of a Log-Structured File System for UNIX"
  - Write Amplification: LSM vs. B-Tree Analyse
  - Read Performance: Bloom Filters und Caching
- Warum LSM für ThemisDB?
  - Write-intensive Workloads (IoT, Logging, TimeSeries)
  - Kompression-Effizienz
  - SSD-Optimierung
  - Benchmark-Daten: ThemisDB vs. PostgreSQL vs. MongoDB

**2.2 MVCC und Transaktionstheorie**
- Theoretische Grundlagen
  - Gray, J., Reuter, A. (1992). "Transaction Processing: Concepts and Techniques"
  - Berenson, H., et al. (1995). "A Critique of ANSI SQL Isolation Levels"
  - Adya, A., et al. (2000). "Generalized Isolation Level Definitions"
- Isolation Levels im Vergleich
  - Read Uncommitted, Read Committed, Repeatable Read, Serializable
  - Snapshot Isolation: Theorie und Praxis
  - Serializable Snapshot Isolation (SSI)
- MVCC-Implementierungen in Produktionssystemen
  - PostgreSQL: Vacuum-basiertes MVCC (Stonebraker, 1986)
  - Oracle: Undo Segments (Oracle Corp., seit 1983)
  - MySQL InnoDB: Rollback Segments (Innobase Oy, 2001)
  - SQL Server: Versioned Rows (Microsoft, 2005)
- ThemisDB MVCC-Design
  - Snapshot Isolation als Standard
  - Version Chain Management
  - Garbage Collection Strategie
  - Performance-Charakteristiken
  - Begründung: Warum SI statt Serializable?

**2.3 Indexierungsstrukturen**

**2.3.1 B-Trees und Varianten**
- Bayer, R., McCreight, E. (1972). "Organization and Maintenance of Large Ordered Indices"
- B+ Trees: Comer, D. (1979). "The Ubiquitous B-Tree"
- Moderne Optimierungen: Cache-Oblivious B-Trees
- Anwendung: Secondary Indexes in ThemisDB

**2.3.2 HNSW für Vector Search**
- Malkov, Y., Yashunin, D. (2018). "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs"
- Vergleich mit Alternativen
  - Annoy (Spotify): Tree-based
  - FAISS (Facebook): Multiple Algorithmen
  - ScaNN (Google): Learned Quantization
- Komplexitätsanalyse: O(log N) construction, O(log N) search
- ThemisDB Integration und Optimierungen

**2.3.3 R-Trees für Spatial Data**
- Guttman, A. (1984). "R-Trees: A Dynamic Index Structure for Spatial Searching"
- R*-Tree: Beckmann, N., et al. (1990). "The R*-tree: An Efficient and Robust Access Method"
- Anwendung: Geospatial Queries in ThemisDB
- Performance: 2D vs. 3D Indexierung

**2.4 CAP-Theorem und Konsistenzmodelle**
- Fundamentale Theoreme
  - Brewer, E. (2000). "Towards Robust Distributed Systems" (PODC Keynote)
  - Gilbert, S., Lynch, N. (2002). "Brewer's Conjecture and the Feasibility of Consistent, Available, Partition-Tolerant Web Services"
  - Abadi, D. (2012). "Consistency Tradeoffs in Modern Distributed Database System Design" (IEEE Computer)
- PACELC-Erweiterung
  - Abadi, D. (2012). "PACELC: An Extension to CAP Theorem"
  - if Partition: Availability vs. Consistency
  - else: Latency vs. Consistency
- Konsistenzmodelle im Detail
  - Strong Consistency (Linearizability)
  - Eventual Consistency
  - Causal Consistency
  - Session Consistency
- ThemisDB Positionierung
  - Default: CP (Consistency + Partition Tolerance)
  - Optional: Tunable Consistency (wie Cassandra)
  - Replication: Strong Consistency für synchronous, Eventual für async
  - Begründung und Trade-offs

**2.5 Kompressionsalgorithmen**

**2.5.1 Allgemeine Kompression**
- LZ4: Collet, Y. (2011). "LZ4: Extremely fast compression"
  - Speed vs. Ratio: 500+ MB/s compression
  - Verwendung: LSM Levels 0-5 in ThemisDB
- ZSTD: Collet, Y., Facebook (2016). "Zstandard: Real-time data compression"
  - Better Ratio: 3x-4x compression
  - Verwendung: LSM Level 6 (bottommost) in ThemisDB
- Snappy: Google (2011): "Snappy: A fast compressor/decompressor"
  - Alternative für LZ4
  - Vergleichs-Benchmarks

**2.5.2 Time Series Spezifische Kompression**
- Gorilla: Pelkonen, T., et al. (2015). "Gorilla: A Fast, Scalable, In-Memory Time Series Database"
  - Delta-of-Delta Encoding für Timestamps
  - XOR-based Compression für Values
  - 90%+ Compression Ratio
- Alternativen
  - Monarch (Google, 2020): Adaptive Compression
  - Prometheus: Custom Encoding
- ThemisDB Implementation
  - Gorilla für Float64 TimeSeries
  - Adaptive für Integer TimeSeries
  - Benchmark-Ergebnisse

**Referenzdokumente:**
- `docs/architecture/architecture_mvcc.md`
- `docs/storage/storage_rocksdb.md`
- `docs/timeseries/timeseries_overview.md`
- `docs/index/index_overview.md`

**Vollständige Bibliographie (Kapitel 2):**

[1] O'Neil, P., Cheng, E., Gawlick, D., O'Neil, E. (1996). "The Log-Structured Merge-Tree (LSM-Tree)". Acta Informatica, 33(4), 351-385.

[2] Bayer, R., McCreight, E. (1972). "Organization and Maintenance of Large Ordered Indices". Acta Informatica, 1(3), 173-189.

[3] Gray, J., Reuter, A. (1992). "Transaction Processing: Concepts and Techniques". Morgan Kaufmann.

[4] Berenson, H., Bernstein, P., Gray, J., et al. (1995). "A Critique of ANSI SQL Isolation Levels". ACM SIGMOD, 24(2), 1-10.

[5] Malkov, Y., Yashunin, D. (2018). "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs". IEEE TPAMI, 42(4), 824-836.

[6] Guttman, A. (1984). "R-Trees: A Dynamic Index Structure for Spatial Searching". ACM SIGMOD, 14(2), 47-57.

[7] Brewer, E. (2000). "Towards Robust Distributed Systems". PODC Keynote.

[8] Gilbert, S., Lynch, N. (2002). "Brewer's Conjecture and the Feasibility of Consistent, Available, Partition-Tolerant Web Services". ACM SIGACT News, 33(2), 51-59.

[9] Pelkonen, T., et al. (2015). "Gorilla: A Fast, Scalable, In-Memory Time Series Database". VLDB Endowment, 8(12), 1816-1827.

[10] Collet, Y. (2016). "Zstandard: Real-time data compression algorithm". RFC 8478.

[11] Abadi, D. (2012). "Consistency Tradeoffs in Modern Distributed Database System Design". IEEE Computer, 45(2), 37-42.

[12] Seltzer, M., Bostic, K. (1991). "An Implementation of a Log-Structured File System for UNIX". USENIX Winter Conference.

---

#### **Kapitel 3: Technologie-Entscheidungen und vergleichende Analyse**

**3.1 Programmiersprache: Warum C++20?**

**3.1.1 Vergleichende Sprachanalyse**
- C++ vs. Rust
  - Rust: Memory Safety ohne GC (Klabnik, S., Nichols, C. (2018). "The Rust Programming Language")
  - C++: Mature Ecosystem, Backwards Compatibility
  - Performance: Beide comparable, LLVM-basiert
  - Developer Productivity: Rust steiler Lernkurve
  - Entscheidung: C++20 wegen Ecosystem und Team-Expertise
  
- C++ vs. Go
  - Go: Simplicity, Built-in Concurrency (Donovan, A., Kernighan, B. (2015). "The Go Programming Language")
  - Performance Gap: 2-3x langsamer für numerische Operationen
  - GC Pauses: Problematisch für Low-Latency DBs
  - Benchmark-Daten: TechEmpower Benchmarks
  
- C++ vs. Java
  - JVM: Mature, GC Optimizations (Goetz, B., et al. (2006). "Java Concurrency in Practice")
  - JIT Warmup: Cold Start Probleme
  - Memory Overhead: 2-4x höher
  - HBase, Cassandra Beispiele: JVM Tuning Complexity

**3.1.2 C++20 Features und Moderne Praktiken**
- Zero-Cost Abstractions
  - Stroustrup, B. (2013). "The C++ Programming Language, 4th Edition"
  - Meyers, S. (2014). "Effective Modern C++"
- Memory Management ohne GC
  - RAII (Resource Acquisition Is Initialization)
  - Smart Pointers: unique_ptr, shared_ptr
  - Move Semantics und Perfect Forwarding
- Concepts für Type Safety (C++20)
  - Compile-time Constraints
  - Better Error Messages
  - Example: `template<StorageBackend T>`
- Coroutines für Async I/O (C++20)
  - Stackless Coroutines
  - Zero-Overhead Abstraction
  - Alternative zu Callbacks

**3.2 Storage Engine: RocksDB Evaluierung**

**3.2.1 Systematischer Vergleich**
- RocksDB (Facebook/Meta)
  - Dong, S., et al. (2021). "RocksDB: Evolution of Development Priorities in a Key-Value Store"
  - Production: Instagram, WhatsApp, Uber, LinkedIn
  - Active Development: 500+ Contributors
  - Customizable: Compaction, Compression, Bloom Filters
  
- WiredTiger (MongoDB Inc.)
  - Ursprünglich BerkeleyDB Team
  - MongoDB Default Storage Engine seit 3.2
  - B-Tree statt LSM: Read-optimiert
  - Benchmark: RocksDB 2x schneller für Writes
  
- LMDB (Symas Corp.)
  - Chu, H. (2011). "Lightning Memory-Mapped Database"
  - Memory-Mapped, Copy-on-Write
  - Sehr einfache API
  - Limitation: Fixed DB Size
  
- BerkeleyDB (Oracle)
  - Olson, M., et al. (1999). "Berkeley DB"
  - Legacy, Oracle-owned seit 2006
  - Licensing Issues
  - Performance: Outdated

**3.2.2 Entscheidungsmatrix**
| Kriterium | RocksDB | WiredTiger | LMDB | BerkeleyDB |
|-----------|---------|------------|------|------------|
| Write Perf | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| Read Perf | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| Flexibility | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| Community | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| License | Apache 2.0 | Dual | OpenLDAP | AGPL/Comm |

**3.2.3 ThemisDB Begründung**
- Write-Heavy Workload Optimierung
- Column Families für Multi-Model
- Production-Proven (Meta Scale)
- Active Community und Support
- Customization Options

**3.3 HTTP Server: Boost.Beast Evaluation**

**3.3.1 Anforderungen**
- HTTP/1.1 und HTTP/2 Support
- WebSocket für Real-time Updates
- Low-Latency: <1ms overhead
- Thread-Safety und Async I/O
- Header-Only bevorzugt

**3.3.2 Alternativen-Vergleich**
- Boost.Beast
  - Vinnie Falco (Ripple Labs)
  - Header-Only, Boost Ecosystem
  - Async, Built on ASIO
  - Production: Ripple, XRPL
  
- cpp-httplib
  - Einfachste API
  - Synchronous Primary
  - Weniger Features
  
- Pistache
  - Modern C++, REST-fokussiert
  - Good Performance
  - Smaller Community
  
- Crow (Jinja2-inspired)
  - Flask-ähnliche API
  - Header-Only
  - Weniger mature

**3.3.3 Benchmark-Ergebnisse**
- TechEmpower Web Framework Benchmarks
- Beast: 500K+ req/sec (plaintext)
- Latency p99: <1ms
- Entscheidung: Beast wegen Boost Integration

**3.4 Parallelisierung: Intel TBB**

**3.4.1 Concurrency-Modelle**
- Threading Building Blocks (TBB)
  - Reinders, J. (2007). "Intel Threading Building Blocks"
  - Task-based Parallelism
  - Work-Stealing Scheduler
  - NUMA-aware
  
- OpenMP
  - Standard für Shared-Memory Parallel
  - #pragma basiert
  - Weniger flexibel
  
- std::thread (C++11)
  - Low-level, explizite Threads
  - Kein Work Stealing
  - Manual Load Balancing

**3.4.2 ThemisDB Use Cases**
- Parallel Index Updates
- Batch Operations
- Query Parallelization
- Compaction Tasks

**3.4.3 Performance-Charakteristiken**
- Scalability: Linear bis 64+ Cores
- Overhead: <5% vs. Manual Threading
- Benchmark: PARSEC Suite

**3.5 Dependency Management**

**3.5.1 vcpkg vs. Conan vs. CMake FetchContent**
- vcpkg (Microsoft)
  - Microsoft (2016). "vcpkg: C++ Library Manager"
  - CMake Integration
  - Binary Caching
  - 2000+ Libraries
  
- Conan
  - JFrog, Python-based
  - Artifactory Integration
  - More Complex
  
- CMake FetchContent
  - Built-in, No Extra Tool
  - Source-only
  - Slower Builds

**3.5.2 Build vs. Buy Entscheidungen**
- Wann eigene Implementation?
  - Core Differentiator (z.B. MVCC)
  - Performance-Critical Path
  - Spezifische Requirements
  
- Wann Third-Party?
  - Commodity Functionality (JSON Parsing)
  - Mature, Well-Tested (Compression)
  - Active Maintenance

**3.5.3 Lizenz-Analyse**
| Library | License | Commercial OK | Copyleft |
|---------|---------|---------------|----------|
| RocksDB | Apache 2.0 | ✅ | ❌ |
| Boost | BSL 1.0 | ✅ | ❌ |
| Intel TBB | Apache 2.0 | ✅ | ❌ |
| simdjson | Apache 2.0 | ✅ | ❌ |

**Referenzdokumente:**
- `docs/guides/guides_build_strategy.md`
- `CMakeLists.txt`
- `vcpkg.json`

**Vollständige Bibliographie (Kapitel 3):**

[1] Stroustrup, B. (2013). "The C++ Programming Language, 4th Edition". Addison-Wesley.

[2] Klabnik, S., Nichols, C. (2018). "The Rust Programming Language". No Starch Press.

[3] Donovan, A., Kernighan, B. (2015). "The Go Programming Language". Addison-Wesley.

[4] Meyers, S. (2014). "Effective Modern C++". O'Reilly Media.

[5] Dong, S., Callaghan, M., Galanis, L., et al. (2021). "RocksDB: Evolution of Development Priorities in a Key-Value Store Serving Large-scale Applications". ACM TOCS, 39(4).

[6] Chu, H. (2011). "Lightning Memory-Mapped Database". OpenLDAP Project.

[7] Olson, M., Bostic, K., Seltzer, M. (1999). "Berkeley DB". USENIX Annual Technical Conference.

[8] Reinders, J. (2007). "Intel Threading Building Blocks: Outfitting C++ for Multi-core Processor Parallelism". O'Reilly Media.

[9] Goetz, B., Peierls, T., Bloch, J., et al. (2006). "Java Concurrency in Practice". Addison-Wesley.

[10] TechEmpower (2023). "Web Framework Benchmarks - Round 22". https://www.techempower.com/benchmarks/
  - Lizenz-Kompatibilität und Vendor Lock-in

**Referenzdokumente:**
- `docs/guides/guides_build_strategy.md`
- `CMakeLists.txt`
- `vcpkg.json`

**Vergleichende Analysen:**
- RocksDB vs. WiredTiger Performance Studies
- C++ vs. Rust Database Implementations
- Modern C++ Best Practices (Stroustrup et al.)

---

### **TEIL II: ARCHITEKTUR UND DESIGN**

#### **Kapitel 4: Systemarchitektur**

**4.1 Schichtenarchitektur: Prinzipien und Patterns**
- Layered Architecture Pattern
  - Buschmann, F., et al. (1996). "Pattern-Oriented Software Architecture, Volume 1"
  - Separation of Concerns
  - Dependency Inversion Principle (Martin, R. C. (2000). "Design Principles and Design Patterns")
- Vergleich mit anderen Datenbank-Architekturen
  - PostgreSQL: Process-per-Connection vs. Thread-per-Connection
  - MongoDB: Pluggable Storage Engine Architecture
  - MySQL: Handler Interface Design
  - ThemisDB: Hybrid Approach
- Clean Architecture Prinzipien
  - Martin, R. C. (2017). "Clean Architecture: A Craftsman's Guide to Software Structure"
  - Core Business Logic unabhängig von Frameworks
  - Testability und Maintainability

**4.2 HTTP/REST API Layer**
- RESTful Design Principles
  - Fielding, R. T. (2000). "Architectural Styles and the Design of Network-based Software Architectures" (Dissertation)
  - Richardson Maturity Model (Fowler, M. (2010))
  - HATEOAS: Hypermedia as the Engine of Application State
- HTTP/2 und Performance
  - RFC 7540: HTTP/2 Specification
  - Multiplexing und Stream Prioritization
  - Server Push Überlegungen
- API Versioning Strategies
  - URI Versioning vs. Header Versioning
  - Semantic Versioning (SemVer 2.0.0)
  - Backward Compatibility Guarantees
- Vergleich mit anderen DB APIs
  - MongoDB: Custom Wire Protocol vs. HTTP
  - CouchDB: HTTP-Native Design
  - ArangoDB: REST API + Custom Protocol
  - ThemisDB Entscheidung: HTTP/REST First, Wire Protocol Optional
- Error Handling Standards
  - RFC 7807: Problem Details for HTTP APIs
  - Structured Error Responses
  - Client-Friendly Error Messages

**4.3 Query Engine Layer**
- Query Processing Pipeline
  - Graefe, G. (1993). "Query Evaluation Techniques for Large Databases". ACM Computing Surveys
  - Parsing → Optimization → Execution Stages
  - Cost-Based vs. Rule-Based Optimization
- Volcano/Iterator Model
  - Graefe, G. (1994). "Volcano - An Extensible and Parallel Query Evaluation System". IEEE TKDE
  - Push-based vs. Pull-based Execution
  - Materialization vs. Pipelining
- Vergleich mit anderen Query Engines
  - PostgreSQL: Planner und Executor
  - MongoDB: Aggregation Pipeline
  - ArangoDB: AQL Execution Engine
  - ClickHouse: Vectorized Execution
  - ThemisDB: Hybrid Iterator + Vectorization
- Query Optimization Techniques
  - Predicate Push-Down
  - Join Reordering (Selinger, P. G., et al. (1979). "Access Path Selection in a Relational Database"))
  - Index Selection
  - Parallel Execution Planning

**4.4 Index Layer: Multi-Model-Projektionen**
- Index Abstraction Design
  - Pluggable Index Architecture
  - Common Index Interface
  - Specialized Implementations
- Index Selection Strategy
  - Cost Model für verschiedene Index-Typen
  - Automatic Index Recommendations
  - Query Workload Analysis
- Vergleich Index-Architekturen
  - PostgreSQL: GiST, GIN, SP-GiST, BRIN
  - MongoDB: Index Plugins
  - Elasticsearch: Inverted Index + Doc Values
  - ThemisDB: Unified Index Manager
- Index Synchronization
  - Transactional Index Updates
  - Eventual Consistency für Secondary Indexes
  - Index Rebuild Strategies

**4.5 Base Entity Layer: Canonical Storage**
- Document-Oriented Storage Model
  - DeCandia, G., et al. (2007). "Dynamo: Amazon's Highly Available Key-Value Store". SOSP
  - Schema-on-Read vs. Schema-on-Write
  - Flexible vs. Rigid Schemas
- JSON als Storage Format
  - BSON (MongoDB) vs. JSON (CouchDB) vs. Protocol Buffers
  - simdjson: Performance-Optimierung (Langdale, G., Lemire, D. (2019). "Parsing Gigabytes of JSON per Second")
  - Memory Overhead Analysis
- Metadata Management
  - Version Tracking (MVCC Integration)
  - Timestamps (Lamport, L. (1978). "Time, Clocks, and the Ordering of Events")
  - Blob Size Optimization
- Schema Evolution
  - Backward Compatibility Patterns
  - Forward Compatibility Considerations
  - Migration Strategies
- Vergleich mit Alternativen
  - Protobuf: Typed, Compact, aber weniger flexibel
  - Avro: Schema Evolution Support
  - MessagePack: Kompakter als JSON
  - ThemisDB: JSON für Flexibilität, simdjson für Performance

**4.6 Storage Layer (RocksDB) Integration**
- RocksDB API Abstraktion
  - Storage Engine Interface Design
  - Pluggable Storage Backends (Future: andere Engines)
  - Transaction Coordination
- Column Families Strategy
  - Separate CFs für Entities, Indexes, Metadata
  - Independent Compaction per CF
  - Resource Isolation
- Write Path Optimierung
  - Batch Writes
  - Write-Ahead-Log (WAL) Tuning
  - Memtable Sizing
- Read Path Optimierung
  - Block Cache Tuning
  - Bloom Filters
  - Prefix Iterators
- Vergleich Storage Engines
  - B-Tree Engines (WiredTiger, InnoDB): Read-optimiert
  - LSM Engines (RocksDB, LevelDB): Write-optimiert
  - Hybrid (MyRocks): LSM mit B-Tree Features
  - ThemisDB: Pure LSM für Write-Heavy Workloads

**Referenzdokumente:**
- `docs/architecture/architecture_overview.md`
- `docs/architecture/architecture_strategic.md`
- `docs/architecture/architecture_multi_model.md`
- `docs/server/server_overview.md`

**Vollständige Bibliographie (Kapitel 4):**

[1] Buschmann, F., Meunier, R., Rohnert, H., et al. (1996). "Pattern-Oriented Software Architecture, Volume 1: A System of Patterns". Wiley.

[2] Martin, R. C. (2000). "Design Principles and Design Patterns". Object Mentor.

[3] Martin, R. C. (2017). "Clean Architecture: A Craftsman's Guide to Software Structure and Design". Prentice Hall.

[4] Fielding, R. T. (2000). "Architectural Styles and the Design of Network-based Software Architectures". UC Irvine Dissertation.

[5] Graefe, G. (1993). "Query Evaluation Techniques for Large Databases". ACM Computing Surveys, 25(2), 73-169.

[6] Graefe, G. (1994). "Volcano - An Extensible and Parallel Query Evaluation System". IEEE TKDE, 6(1), 120-135.

[7] Selinger, P. G., Astrahan, M. M., Chamberlin, D. D., et al. (1979). "Access Path Selection in a Relational Database Management System". ACM SIGMOD, 23-34.

[8] DeCandia, G., Hastorun, D., Jampani, M., et al. (2007). "Dynamo: Amazon's Highly Available Key-Value Store". SOSP, 205-220.

[9] Langdale, G., Lemire, D. (2019). "Parsing Gigabytes of JSON per Second". VLDB Journal, 28(6), 941-960.

[10] Lamport, L. (1978). "Time, Clocks, and the Ordering of Events in a Distributed System". CACM, 21(7), 558-565.

[11] Fowler, M. (2010). "Richardson Maturity Model". https://martinfowler.com/articles/richardsonMaturityModel.html

[12] IETF (2015). "RFC 7540: Hypertext Transfer Protocol Version 2 (HTTP/2)".

---

#### **Kapitel 5: Base Entity Design**

**5.1 Canonical Storage Format: Philosophie und Design**
- One Storage Format to Rule Them All
  - Multi-Model durch Index-Projektionen
  - Single Source of Truth
  - Consistency durch gemeinsamen Storage
- JSON als Basis-Format
  - Schema Flexibility vs. Type Safety Trade-off
  - Self-Describing Data
  - Human-Readable für Debugging
- Vergleich Document Formats
  - BSON (MongoDB): Binary JSON mit Typen
  - JSONB (PostgreSQL): Binary mit Indexierung
  - Plain JSON (CouchDB): Text-basiert
  - ThemisDB: JSON mit simdjson Parsing
- Document-Oriented vs. Relational
  - Codd, E. F. (1970). "A Relational Model of Data for Large Shared Data Banks"
  - Cattell, R. (2011). "Scalable SQL and NoSQL Data Stores"
  - Impedance Mismatch Problem
  - ThemisDB Hybrid: Relational Queries auf Document Storage

**5.2 Key-Schema und Namensräume**
- Key Design: `table:primary_key`
  - Namespace Isolation
  - Lexicographic Ordering Benefits
  - Range Scan Efficiency
- Alternative Key Schemas
  - UUID-basiert (Cassandra-Style)
  - Hash-basiert (Consistent Hashing)
  - Hierarchisch (HBase Row Key Design)
  - ThemisDB: Simple, Readable, Scannable
- Compound Keys und Clustering
  - CQL (Cassandra): Partition Key + Clustering Columns
  - DynamoDB: Hash Key + Sort Key
  - ThemisDB: Application-Level Compound Keys
- Key Size Optimierung
  - Short Keys vs. Descriptive Keys
  - Prefix Compression
  - Storage Overhead Analysis

**5.3 JSON-Serialisierung mit simdjson**
- JSON Parsing Performance
  - Langdale, G., Lemire, D. (2019). "Parsing Gigabytes of JSON per Second"
  - SIMD-basierte Parallelisierung
  - Zero-Copy Deserialization
- Vergleich JSON Libraries
  - RapidJSON: Populär, C++
  - nlohmann/json: Modern C++, einfache API
  - simdjson: 2-5x schneller
  - sajson: Single-Header
- Benchmark-Ergebnisse
  - Parsing Speed: GB/sec
  - Memory Overhead
  - API Ergonomics vs. Performance
- Schema Validation
  - JSON Schema Draft-07
  - Runtime Validation vs. Static Typing
  - Performance Impact

**5.4 Metadaten: Version, Timestamp, Blob Size**
- MVCC Version Tracking
  - Version ID als monotonic counter
  - Transaction ID Integration
  - Garbage Collection Hints
- Timestamp Design
  - Lamport Timestamps vs. Physical Time
  - Hybrid Logical Clocks (Kulkarni, S., et al. (2014))
  - Clock Skew Handling
- Blob Size Management
  - Inline vs. External Storage
  - Large Object Handling (> 1MB)
  - Compression Metadata
- Metadata Overhead
  - Fixed-Size vs. Variable-Size Metadata
  - Space Efficiency Analysis
  - Read/Write Performance Impact

**5.5 Schema Evolution und Compatibility**
- Schema Evolution Patterns
  - Kleppmann, M. (2017). "Designing Data-Intensive Applications"
  - Forward Compatibility: Old Reader, New Writer
  - Backward Compatibility: New Reader, Old Writer
  - Full Compatibility: Both Directions
- Versioning Strategies
  - Schema Version in Document
  - Global Schema Registry (Avro-Style)
  - Code Handles Multiple Versions
- Migration Strategies
  - Lazy Migration (On-Read)
  - Eager Migration (Background Job)
  - Dual-Write Pattern
- Breaking Changes Handling
  - Deprecation Period
  - Graceful Degradation
  - Error Handling

**5.6 Vergleichende Analyse: Storage Formate**

| Format | Pro | Con | Verwendet von |
|--------|-----|-----|---------------|
| JSON | Human-readable, Flexible | Verbos, Parsing-Overhead | CouchDB, ThemisDB |
| BSON | Typed, Traversable | Binary, Größer als JSON | MongoDB |
| JSONB | Indexed, Compressed | PostgreSQL-spezifisch | PostgreSQL |
| Protobuf | Compact, Typed, Fast | Schema erforderlich | gRPC, Vitess |
| Avro | Schema Evolution, Compact | Komplexer | Kafka, Hadoop |

**ThemisDB Entscheidung:**
- JSON für Developer Experience
- simdjson für Performance-Parität mit Binary Formats
- Schema-on-Read für Flexibilität
- Optional: Schema Validation für Production

**Referenzdokumente:**
- `docs/architecture/architecture_base_entity.md`
- `include/storage/base_entity.hpp`
- `src/storage/base_entity.cpp`
- `tests/test_base_entity.cpp`

**Vollständige Bibliographie (Kapitel 5):**

[1] Codd, E. F. (1970). "A Relational Model of Data for Large Shared Data Banks". CACM, 13(6), 377-387.

[2] Cattell, R. (2011). "Scalable SQL and NoSQL Data Stores". ACM SIGMOD Record, 39(4), 12-27.

[3] Langdale, G., Lemire, D. (2019). "Parsing Gigabytes of JSON per Second". VLDB Journal, 28(6), 941-960.

[4] Kulkarni, S., Demirbas, M., Madappa, D., et al. (2014). "Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases". OPODIS.

[5] Kleppmann, M. (2017). "Designing Data-Intensive Applications". O'Reilly Media.

[6] IETF (2017). "JSON Schema: A Media Type for Describing JSON Documents". Draft-07.

---

#### **Kapitel 6: MVCC Transaction Design**

**6.1 Snapshot Isolation: Theorie und Praxis**
- Snapshot Isolation Definition
  - Berenson, H., et al. (1995). "A Critique of ANSI SQL Isolation Levels"
  - Consistent Read View
  - Write-Write Conflict Detection
  - Keine Read Locks
- SI vs. Serializable
  - Write Skew Anomaly
  - Fekete, A., et al. (2005). "Making Snapshot Isolation Serializable"
  - Serializable Snapshot Isolation (SSI)
  - PostgreSQL SSI Implementation
- Warum SI für ThemisDB?
  - Performance: Keine Read Locks
  - Scalability: Reader-Writer Separation
  - Predictability: Feste Snapshot-Zeit
  - Acceptable Anomalies für Use Cases

**6.2 Version Chain Management**
- Version Storage Strategies
  - Append-Only (PostgreSQL): Neue Version = Neuer Tuple
  - Time-Travel (Oracle): Undo Tablespace
  - Delta Storage (MySQL InnoDB): Rollback Segments
  - ThemisDB: RocksDB Versioning via Key Encoding
- Version Chain Organization
  - Backward Chaining: Newest → Oldest
  - Forward Chaining: Oldest → Newest
  - ThemisDB: Implicit via LSM Tree Levels
- Version Visibility Rules
  - Transaction Start Timestamp
  - Version Create/Delete Timestamps
  - Snapshot Visibility Algorithm
- Read Performance Optimierung
  - Version Pruning
  - Visibility Cache
  - Fast Path für Newest Version

**6.3 Garbage Collection**
- When to GC?
  - No Active Transactions need old versions
  - Retention Policies (Point-in-Time Recovery)
  - Storage Pressure Triggers
- GC Strategies Vergleich
  - PostgreSQL: Vacuum (Full vs. Lazy)
  - MySQL: Purge Thread
  - Oracle: Automatic Undo Retention
  - CockroachDB: MVCC GC with TTL
- ThemisDB GC Design
  - Background Compaction-based GC
  - RocksDB Compaction Filter Integration
  - Configurable Retention Period
  - Manual vs. Automatic Trigger
- GC Performance Impact
  - I/O Overhead
  - CPU Usage
  - Write Amplification
  - User-Facing Latency

**6.4 Conflict Detection und Resolution**
- Write-Write Conflicts
  - First-Committer-Wins Rule
  - Last-Writer-Wins (Cassandra)
  - Application-Level Resolution
- Conflict Detection Timing
  - Optimistic: Bei Commit (ThemisDB)
  - Pessimistic: Bei Write (2PL)
  - Hybrid: Predicate Locks
- Retry Logic
  - Exponential Backoff
  - Transaction Priority
  - Deadlock Avoidance
- Distributed Transactions
  - 2PC (Two-Phase Commit)
  - 3PC (Three-Phase Commit)
  - Paxos/Raft Consensus
  - ThemisDB: Local MVCC + Distributed Coordination (Future)

**6.5 ACID-Garantien**
- Atomicity Implementation
  - WAL (Write-Ahead Logging)
  - RocksDB WriteBatch
  - All-or-Nothing Semantics
- Consistency Enforcement
  - Application-Level Constraints
  - Schema Validation
  - Referential Integrity (Optional)
- Isolation Levels Support
  - Read Uncommitted: Nicht unterstützt
  - Read Committed: Nicht unterstützt
  - Repeatable Read: Nicht unterstützt
  - Snapshot Isolation: Standard
  - Serializable: Future mit SSI
- Durability Guarantees
  - fsync() bei Commit
  - WAL Flush Policies
  - Replication für Disaster Recovery
  - Recovery Point Objective (RPO)

**6.6 Vergleichende MVCC-Analyse**

| System | Storage | Visibility | GC | Isolation |
|--------|---------|------------|----|-----------| 
| PostgreSQL | Append-Only | Tuple Headers | Vacuum | SI + SSI |
| MySQL InnoDB | Rollback Segments | Undo Log | Purge Thread | RC, RR |
| Oracle | Undo Tablespace | SCN | Auto Undo Mgmt | RC, SI |
| CockroachDB | Versioned KV | MVCC Timestamps | GC Policy | SI + SSI |
| **ThemisDB** | LSM Versions | Snapshot TS | Compaction Filter | SI |

**Design Trade-offs:**
- ✅ Pro LSM-based MVCC: Write Performance, Compression
- ✅ Pro SI: Read Performance, No Locks
- ❌ Con: Write Skew möglich
- ❌ Con: Storage Overhead bis GC

**Referenzdokumente:**
- `docs/architecture/architecture_mvcc.md`
- `docs/transaction/transaction_overview.md`
- `include/transaction/mvcc_transaction.hpp`
- `src/transaction/mvcc_transaction.cpp`

**Vollständige Bibliographie (Kapitel 6):**

[1] Berenson, H., Bernstein, P., Gray, J., et al. (1995). "A Critique of ANSI SQL Isolation Levels". ACM SIGMOD, 24(2), 1-10.

[2] Fekete, A., Liarokapis, D., O'Neil, E., et al. (2005). "Making Snapshot Isolation Serializable". ACM TODS, 30(2), 492-528.

[3] Gray, J., Reuter, A. (1992). "Transaction Processing: Concepts and Techniques". Morgan Kaufmann.

[4] Bernstein, P. A., Newcomer, E. (2009). "Principles of Transaction Processing, 2nd Edition". Morgan Kaufmann.

[5] Ports, D. R., Grittner, K. (2012). "Serializable Snapshot Isolation in PostgreSQL". VLDB Endowment, 5(12), 1850-1861.

[6] Wu, Y., Arulraj, J., Lin, J., et al. (2017). "An Empirical Evaluation of In-Memory Multi-Version Concurrency Control". VLDB Endowment, 10(7), 781-792.

---

#### **Kapitel 7: Query Engine und AQL**

**7.1 AQL-Syntax und Semantik**
- Query Language Design Philosophie
  - Declarative vs. Imperative
  - SQL-ähnlich vs. Eigenständig
  - ArangoDB AQL als Inspiration
- AQL Syntax Overview
  - FOR-IN Iteration
  - FILTER Predicates
  - LET Variable Binding
  - COLLECT Aggregation
  - SORT Ordering
  - LIMIT/SKIP Pagination
  - RETURN Projection
- Multi-Model Query Support
  - Document Queries
  - Graph Traversal (FOR ... IN OUTBOUND)
  - Vector Search (VECTOR_SIMILARITY)
  - Time Series (Temporal Predicates)
- Vergleich Query Languages
  - SQL: Standard, aber nicht Multi-Model
  - MQL (MongoDB): JSON-basiert, weniger lesbar
  - Cypher (Neo4j): Graph-fokussiert
  - AQL (ArangoDB): Multi-Model, aber proprietär
  - GraphQL: API-Layer, nicht DB Query Language
  - ThemisDB AQL: ArangoDB-inspired mit Extensions

**7.2 Parser-Implementierung**
- Parser Architecture
  - Lexical Analysis (Tokenization)
  - Syntax Analysis (AST Construction)
  - Semantic Analysis (Type Checking)
- Parsing Techniques
  - Recursive Descent
  - LL(k) Parsing
  - LALR Parsing (yacc/bison)
  - ThemisDB: Hand-Written Recursive Descent
- Abstract Syntax Tree (AST)
  - Node Types: Expression, Statement, Query
  - Visitor Pattern für AST Traversal
  - Gamma, E., et al. (1994). "Design Patterns: Elements of Reusable Object-Oriented Software"
- Error Handling
  - Syntax Error Recovery
  - Semantic Error Reporting
  - User-Friendly Error Messages
- Parser Performance
  - Parse Time Overhead
  - AST Memory Footprint
  - Caching Prepared Statements

**7.3 Query Optimizer**
- Optimization Goals
  - Minimize Execution Time
  - Minimize Memory Usage
  - Minimize I/O Operations
- Cost-Based Optimization
  - Selinger, P. G., et al. (1979). "Access Path Selection in a Relational Database"
  - Cardinality Estimation
  - Selectivity Estimation
  - Cost Models für Operatoren
- Rule-Based Optimization
  - Predicate Push-Down
  - Projection Push-Down
  - Constant Folding
  - Dead Code Elimination
- Join Optimization
  - Join Order Selection
  - Left-Deep vs. Bushy Trees
  - Dynamic Programming (Selinger)
  - Greedy Heuristics
- Index Selection
  - Index-Only Scans
  - Index Intersection
  - Covering Indexes
- Query Rewriting
  - Subquery Unnesting
  - View Expansion
  - Common Subexpression Elimination

**7.4 Execution Engine**
- Execution Models
  - Iterator Model (Volcano): Graefe, G. (1994)
  - Materialization Model
  - Vectorized Model: Boncz, P., et al. (2005). "MonetDB/X100"
  - Compilation (LLVM JIT): Neumann, T. (2011). "HyPer"
  - ThemisDB: Iterator + Vectorization Hybrid
- Operator Implementations
  - Scan: Sequential, Index, Range
  - Filter: Predicate Evaluation
  - Project: Column Extraction
  - Join: Hash Join, Nested-Loop Join, Sort-Merge Join
  - Aggregate: Hash-based Aggregation
  - Sort: External Sort, Top-K
- Parallelization
  - Intra-Query Parallelism
  - Inter-Query Parallelism
  - Partition-based Parallelism
  - TBB Task-based Execution
- Memory Management
  - Memory Budgets per Query
  - Spill-to-Disk für Large Joins
  - Buffer Pool Integration

**7.5 JOIN-Implementierung**
- Hash Join
  - Build Phase: Hash Table Creation
  - Probe Phase: Matching
  - Grace Hash Join für Large Tables
  - Hybrid Hash Join (Shapiro, L. D. (1986))
- Nested-Loop Join
  - Simple Nested-Loop
  - Index Nested-Loop
  - Block Nested-Loop
  - When to Use: Small Inner Table
- Sort-Merge Join
  - External Sort
  - Merge Phase
  - Best for Pre-Sorted Input
- Join Type Support
  - Inner Join
  - Left/Right/Full Outer Join
  - Semi Join / Anti Join
  - Cross Join
- Multi-Way Joins
  - Binary Join Tree
  - Star Join Optimization
  - Broadcast Join (Spark-Style)

**7.6 Predicate Push-Down**
- Push-Down Optimization
  - Filter Early, Filter Often
  - Reduce Data Movement
  - Index Utilization
- Storage-Level Push-Down
  - RocksDB Prefix Filtering
  - Bloom Filter Usage
  - Skip Irrelevant SST Files
- Index Push-Down
  - Secondary Index Filtering
  - Vector Index Filtering
  - Graph Traversal Pruning
- Limitations
  - Non-Deterministic Functions
  - Correlated Subqueries
  - Aggregate Functions

**7.7 Vergleichende Query Engine Analyse**

| System | Execution Model | Optimization | Parallelism |
|--------|----------------|--------------|-------------|
| PostgreSQL | Iterator | Cost-Based | Process-based |
| MySQL | Iterator | Cost-Based + Hints | Limited |
| MongoDB | Aggregation Pipeline | Rule-Based | Sharded |
| ClickHouse | Vectorized | Cost-Based | Thread-based |
| DuckDB | Vectorized + JIT | Cost-Based | Thread-based |
| **ThemisDB** | Iterator + Vectorized | Cost + Rule-Based | TBB Tasks |

**ThemisDB Design Rationale:**
- Iterator Model: Simplicity, Memory Efficiency
- Vectorization: SIMD for Numeric Operations
- TBB: Easy Parallelization
- Extensible: Plugin für Custom Operators

**Referenzdokumente:**
- `docs/aql/aql_syntax.md`
- `docs/aql/aql_query_engine.md`
- `docs/query/query_optimizer.md`
- `include/query/aql_parser.hpp`
- `src/query/query_executor.cpp`

**Vollständige Bibliographie (Kapitel 7):**

[1] Selinger, P. G., Astrahan, M. M., Chamberlin, D. D., et al. (1979). "Access Path Selection in a Relational Database Management System". ACM SIGMOD, 23-34.

[2] Graefe, G. (1994). "Volcano - An Extensible and Parallel Query Evaluation System". IEEE TKDE, 6(1), 120-135.

[3] Graefe, G. (1993). "Query Evaluation Techniques for Large Databases". ACM Computing Surveys, 25(2), 73-169.

[4] Boncz, P., Zukowski, M., Nes, N. (2005). "MonetDB/X100: Hyper-Pipelining Query Execution". CIDR.

[5] Neumann, T. (2011). "Efficiently Compiling Efficient Query Plans for Modern Hardware". VLDB Endowment, 4(9), 539-550.

[6] Shapiro, L. D. (1986). "Join Processing in Database Systems with Large Main Memories". ACM TODS, 11(3), 239-264.

[7] Gamma, E., Helm, R., Johnson, R., Vlissides, J. (1994). "Design Patterns: Elements of Reusable Object-Oriented Software". Addison-Wesley.

[8] Ibaraki, T., Kameda, T. (1984). "On the Optimal Nesting Order for Computing N-Relational Joins". ACM TODS, 9(3), 482-502.

[9] Chaudhuri, S. (1998). "An Overview of Query Optimization in Relational Systems". PODS, 34-43.

---

### **TEIL III: KERN-KOMPONENTEN**

#### **Kapitel 8: Storage Layer**

**8.1 RocksDB-Integration: Architecture und Design**
- RocksDB als Embedded Database
  - Dong, S., et al. (2021). "RocksDB: Evolution of Development Priorities"
  - Embedded vs. Client-Server Architecture
  - Thread-Safety und Concurrent Access
  - Memory-Mapped Files vs. Direct I/O
- LSM-Tree Implementation Details
  - Memtable (SkipList vs. HashLinkedList)
  - Immutable Memtable Queue
  - SSTable (Sorted String Table) Format
  - Level Structure (L0-L6)
- RocksDB API Design
  - Put/Get/Delete Operations
  - Batch Writes für Atomicity
  - Iterators für Range Scans
  - Snapshots für Consistent Reads
- Performance Tuning Knobs
  - Write Buffer Size
  - Max Write Buffer Number
  - Level0 File Num Compaction Trigger
  - Target File Size Base

**8.2 Column Families: Multi-Tenant Storage**
- Column Family Concept
  - Independent LSM Trees
  - Separate Memtables und SSTables
  - Isolated Compaction
- ThemisDB CF Strategy
  - CF für Entities (Default)
  - CF für Secondary Indexes
  - CF für Vector Indexes
  - CF für Metadata
- Vergleich mit Alternativen
  - HBase: Column Families für Schema
  - Cassandra: Column Families = Tables
  - ScyllaDB: Per-Table Configuration
  - ThemisDB: Logical Separation
- CF Performance Implications
  - Independent Write Buffers
  - Compaction Isolation
  - Read Amplification

**8.3 Compaction-Strategie**
- Compaction Goals
  - Reduce Space Amplification
  - Reduce Read Amplification
  - Manage Write Amplification
- Compaction Algorithms
  - Leveled Compaction (Default)
  - Universal Compaction
  - FIFO Compaction
  - Vergleich: Seltzer, M., et al. (1996). "File System Logging"
- Leveled Compaction Deep Dive
  - L0: Overlapping Files
  - L1-L6: Non-Overlapping Files
  - Size Ratio: 10x per Level
  - Compaction Triggers
- Write Amplification Analysis
  - Formula: WA = (Data Written to Disk) / (Data Written by User)
  - Typical WA: 10-30x für Leveled
  - Optimization Techniques
- Compaction Scheduling
  - Background Threads
  - Rate Limiting
  - Priority-based Scheduling

**8.4 Write-Ahead-Log (WAL)**
- WAL Purpose
  - Durability Guarantee
  - Crash Recovery
  - Point-in-Time Recovery
- WAL Implementation
  - Sequential Writes
  - fsync() Policies
  - WAL Recycling
- WAL vs. Memtable
  - WAL: Sequential Write
  - Memtable: Random Access Structure
  - Recovery: Replay WAL
- Vergleich WAL Strategien
  - PostgreSQL: WAL Archiving
  - MySQL: Redo Log + Undo Log
  - Oracle: Redo Logs
  - ThemisDB/RocksDB: Simple WAL

**8.5 Block Cache und Memory Management**
- Block Cache Design
  - LRU Cache Implementation
  - Shard-based Locking
  - Cache Key: File + Offset
- Cache Policies
  - LRU (Least Recently Used)
  - LRU-K: O'Neil, E., et al. (1993). "The LRU-K Page Replacement Algorithm"
  - Clock Algorithm
  - LIRS: Jiang, S., Zhang, X. (2002). "LIRS: Low Inter-reference Recency Set"
- Memory Budget
  - Block Cache: 1GB Default
  - Write Buffers: 256MB per CF
  - Index/Filter Blocks
  - Bloom Filters
- Cache Hit Ratio
  - Monitoring und Tuning
  - Working Set Size
  - Prefetching Strategies

**8.6 Compression Strategy**
- Compression Algorithms Comparison
  - LZ4: Fast, Moderate Ratio
  - Snappy: Google, Fast Decompression
  - ZSTD: Facebook, Best Ratio
  - Zlib: Standard, Slower
- Per-Level Compression
  - L0-L5: LZ4 (Speed Priority)
  - L6 (Bottommost): ZSTD (Ratio Priority)
  - Rationale: Frequent Access vs. Cold Data
- Compression Effectiveness
  - Compression Ratio Measurement
  - Decompression Speed
  - CPU vs. I/O Trade-off
- Block-Level Compression
  - 4KB-64KB Blocks
  - Compression in SSTables
  - Aligned Reads

**8.7 Vergleichende Storage Engine Analyse**

| Engine | Structure | Compaction | Read | Write |
|--------|-----------|------------|------|-------|
| RocksDB | LSM | Leveled | Good | Excellent |
| LevelDB | LSM | Leveled | Good | Excellent |
| WiredTiger | B-Tree | Reconciliation | Excellent | Good |
| InnoDB | B-Tree | None | Excellent | Good |
| LMDB | B-Tree | Copy-on-Write | Excellent | Good |

**ThemisDB Rationale:**
- LSM für Write-Heavy Workloads
- Leveled Compaction für Read Performance
- ZSTD Bottommost für Storage Efficiency

**Referenzdokumente:**
- `docs/storage/storage_rocksdb.md`
- `docs/storage/storage_tuning.md`
- `include/storage/storage_engine.hpp`
- `src/storage/rocksdb_wrapper.cpp`

**Vollständige Bibliographie (Kapitel 8):**

[1] Dong, S., Callaghan, M., Galanis, L., et al. (2021). "RocksDB: Evolution of Development Priorities in a Key-Value Store Serving Large-scale Applications". ACM TOCS, 39(4).

[2] O'Neil, P., Cheng, E., Gawlick, D., O'Neil, E. (1996). "The Log-Structured Merge-Tree (LSM-Tree)". Acta Informatica, 33(4), 351-385.

[3] Seltzer, M., Bostic, K., McKusick, M., et al. (1996). "File System Logging versus Clustering: A Performance Comparison". USENIX Winter, 249-264.

[4] O'Neil, E., O'Neil, P., Weikum, G. (1993). "The LRU-K Page Replacement Algorithm for Database Disk Buffering". ACM SIGMOD, 297-306.

[5] Jiang, S., Zhang, X. (2002). "LIRS: An Efficient Low Inter-reference Recency Set Replacement Policy". ACM SIGMETRICS, 31-42.

[6] Collet, Y. (2016). "Zstandard: Real-time data compression algorithm". RFC 8478.

---

#### **Kapitel 9: Indexierung**

**9.1 Index-Architektur: Multi-Model Index Design**
- Index als Materialized View
  - Query Performance Optimization
  - Space vs. Speed Trade-off
  - Staleness vs. Consistency
- Index Types in ThemisDB
  - Secondary Indexes (B-Tree-like)
  - Full-Text Indexes (Inverted Index)
  - Vector Indexes (HNSW)
  - Graph Indexes (Adjacency Lists)
  - Spatial Indexes (R-Tree)
- Index Manager Design
  - Pluggable Index Architecture
  - Common Index Interface
  - Index Registration
  - Query Planner Integration
- Vergleich Index Architekturen
  - PostgreSQL: Extensible Index Framework (GiST, GIN, SP-GiST, BRIN)
  - Hellerstein, J., et al. (1995). "Generalized Search Trees for Database Systems"
  - MongoDB: Index Plugins
  - Elasticsearch: Lucene-basiert
  - ThemisDB: Multi-Model Unified

**9.2 Secondary Indexes: Equality, Range, Composite**
- Secondary Index Fundamentals
  - Index Key → Primary Key Mapping
  - Non-Unique Indexes
  - Covering Indexes
- B-Tree for Secondary Indexes
  - Ordered Keys
  - Range Scan Support
  - Prefix Compression
- Index Types
  - **Equality Index**: Hash-based or B-Tree
  - **Range Index**: B-Tree required
  - **Composite Index**: Multiple Columns
    - Column Order Matters
    - Prefix Matching
- Index Maintenance
  - Insert: Update Index + Data
  - Delete: Mark as Deleted
  - Update: Delete + Insert
- Index Selection Algorithm
  - Cardinality Estimation
  - Selectivity Calculation
  - Cost Estimation

**9.3 Fulltext-Indexierung**
- Inverted Index Design
  - Zobel, J., Moffat, A. (2006). "Inverted Files for Text Search Engines"
  - Term → Document List Mapping
  - Position Information (für Phrase Queries)
  - Term Frequency (TF)
- Text Processing Pipeline
  - Tokenization (Whitespace, Unicode)
  - Normalization (Lowercasing)
  - Stemming (Porter, M. F. (1980). "An Algorithm for Suffix Stripping")
  - Stop Words Removal
- Ranking Algorithms
  - TF-IDF: Salton, G., McGill, M. (1983). "Introduction to Modern Information Retrieval"
  - BM25: Robertson, S., Zaragoza, H. (2009). "The Probabilistic Relevance Framework: BM25 and Beyond"
  - Cosine Similarity
- Vergleich Full-Text Engines
  - Lucene/Elasticsearch: Industry Standard
  - PostgreSQL Full-Text: Built-in
  - Sphinx: Dedicated Search Engine
  - ThemisDB: Lightweight, Embedded

**9.4 Index-Persistierung**
- Index Storage Strategies
  - Separate Index Files
  - Embedded in Data Files
  - RocksDB Column Families für Indexes
- Index Serialization
  - Binary Format
  - Compression
  - Versioning
- Index Loading
  - Lazy Loading (On-Demand)
  - Eager Loading (Startup)
  - Memory-Mapped Indexes
- Index Checkpointing
  - Periodic Snapshots
  - Incremental Updates
  - Recovery from Crash

**9.5 Index Maintenance und Rebuilding**
- Online Index Building
  - No Downtime
  - Background Process
  - Progress Tracking
- Index Rebuild Triggers
  - Schema Changes
  - Corruption Detection
  - Performance Degradation
- Concurrent Index Creation
  - Snapshot-based Building
  - Merge with New Writes
  - Atomic Swap
- Index Statistics
  - Cardinality
  - Null Count
  - Histogram (Distribution)
  - Query Optimizer Usage

**9.6 Vergleichende Index-Analyse**

| System | Secondary | Full-Text | Vector | Spatial | Graph |
|--------|-----------|-----------|--------|---------|-------|
| PostgreSQL | B-Tree, Hash | GIN | pgvector | PostGIS | - |
| MongoDB | B-Tree | Text Index | Atlas Vector | 2dsphere | - |
| Elasticsearch | - | Inverted | Dense Vector | Geo | - |
| Neo4j | - | Lucene | - | - | Native |
| **ThemisDB** | B-Tree | Inverted | HNSW | R-Tree | Adjacency |

**ThemisDB Unified Approach:**
- All Index Types in One System
- Consistent Interface
- Cross-Model Queries

**Referenzdokumente:**
- `docs/index/index_overview.md`
- `docs/index/index_secondary.md`
- `include/index/index_manager.hpp`
- `src/index/secondary_index.cpp`

**Vollständige Bibliographie (Kapitel 9):**

[1] Bayer, R., McCreight, E. (1972). "Organization and Maintenance of Large Ordered Indices". Acta Informatica, 1(3), 173-189.

[2] Hellerstein, J., Naughton, J., Pfeffer, A. (1995). "Generalized Search Trees for Database Systems". VLDB, 562-573.

[3] Zobel, J., Moffat, A. (2006). "Inverted Files for Text Search Engines". ACM Computing Surveys, 38(2), Article 6.

[4] Porter, M. F. (1980). "An Algorithm for Suffix Stripping". Program, 14(3), 130-137.

[5] Salton, G., McGill, M. J. (1983). "Introduction to Modern Information Retrieval". McGraw-Hill.

[6] Robertson, S., Zaragoza, H. (2009). "The Probabilistic Relevance Framework: BM25 and Beyond". Foundations and Trends in Information Retrieval, 3(4), 333-389.

---

#### **Kapitel 10: HTTP Server**

**10.1 Boost.Beast-Architektur**
- Beast Design Philosophy
  - Header-Only Library
  - Based on Boost.Asio
  - Zero-Copy I/O
  - Extensible Parser
- HTTP/1.1 Support
  - RFC 7230-7235: HTTP/1.1 Specification
  - Keep-Alive Connections
  - Chunked Transfer Encoding
  - Pipeline Support
- Async I/O Model
  - Event Loop (Reactor Pattern)
  - Schmidt, D., et al. (2000). "Pattern-Oriented Software Architecture, Volume 2"
  - Completion Handlers
  - Strand for Thread Safety
- Vergleich HTTP Libraries
  - cpp-httplib: Simplicity
  - Pistache: Modern C++
  - Crow: Flask-like API
  - Beast: Performance + Flexibility
  - ThemisDB: Beast für Production-Grade

**10.2 REST API Design**
- RESTful Principles
  - Fielding, R. T. (2000). "Architectural Styles and the Design of Network-based Software Architectures"
  - Resource-Oriented
  - Stateless Communication
  - HTTP Verbs (GET, POST, PUT, DELETE, PATCH)
- Resource Hierarchy
  - `/entities/{table}:{key}` - Single Entity
  - `/entities/{table}` - Collection
  - `/query` - AQL Endpoint
  - `/admin` - Administration
- HTTP Method Semantics
  - GET: Idempotent, Cacheable
  - POST: Create, Non-Idempotent
  - PUT: Update/Replace, Idempotent
  - DELETE: Remove, Idempotent
  - PATCH: Partial Update
- Versioning Strategy
  - URI Versioning: `/v1/entities`
  - Header Versioning: `Accept: application/vnd.themisdb.v1+json`
  - ThemisDB: URI Versioning für Simplicity

**10.3 Request-Routing**
- Routing Algorithms
  - Trie-based Router
  - Regex-based Router
  - Path Template Matching
- Route Registration
  - Static Routes: `/health`, `/metrics`
  - Parameterized Routes: `/entities/{table}:{key}`
  - Wildcard Routes: `/files/*`
- Handler Chain
  - Middleware Pattern
  - Pre-Processing (Auth, Logging)
  - Main Handler
  - Post-Processing (Metrics, Cleanup)
- Performance Considerations
  - Route Lookup: O(log N) or O(1)
  - Handler Caching
  - Connection Pooling

**10.4 Error Handling**
- HTTP Status Codes
  - 2xx: Success
  - 4xx: Client Error
  - 5xx: Server Error
  - Proper Status Code Selection
- RFC 7807: Problem Details
  - Structured Error Responses
  - Machine-Readable Format
  - Human-Readable Title/Detail
- Error Response Format
  ```json
  {
    "type": "https://docs.themisdb.com/errors#entity-not-found",
    "title": "Entity Not Found",
    "status": 404,
    "detail": "Entity users:123 does not exist",
    "instance": "/entities/users:123"
  }
  ```
- Exception Handling Strategy
  - Catch at Handler Level
  - Log with Context
  - Return Appropriate Status
  - Never Leak Internal Details

**10.5 Middleware-Pattern**
- Middleware Chain
  - Request Interception
  - Response Modification
  - Cross-Cutting Concerns
- Common Middleware
  - **Authentication**: JWT, API Keys
  - **Authorization**: RBAC Checks
  - **Logging**: Request/Response Logging
  - **Metrics**: Prometheus Metrics
  - **CORS**: Cross-Origin Resource Sharing
  - **Rate Limiting**: Token Bucket
- Middleware Ordering
  - Logging (First)
  - CORS
  - Authentication
  - Authorization
  - Rate Limiting
  - Handler
  - Error Handling (Last)
- Middleware Implementation
  - Handler Wrapper Pattern
  - Decorator Pattern (GoF)
  - Functional Composition

**10.6 Vergleichende Server-Architektur**

| Server | Model | Concurrency | Performance | Language |
|--------|-------|-------------|-------------|----------|
| nginx | Event-Driven | epoll/kqueue | Excellent | C |
| Node.js | Event-Driven | libuv | Good | JavaScript |
| Go net/http | Goroutines | Go Scheduler | Good | Go |
| Boost.Beast | Async I/O | Asio | Excellent | C++ |
| **ThemisDB** | Async + TBB | Asio + TBB | Excellent | C++ |

**ThemisDB Hybrid Approach:**
- Async I/O für Network Efficiency
- TBB für CPU-bound Tasks
- Best of Both Worlds

**Referenzdokumente:**
- `docs/server/server_overview.md`
- `docs/api/api_reference.md`
- `include/server/http_server.hpp`
- `src/server/request_handler.cpp`

**Vollständige Bibliographie (Kapitel 10):**

[1] Fielding, R. T. (2000). "Architectural Styles and the Design of Network-based Software Architectures". UC Irvine Dissertation.

[2] Schmidt, D., Stal, M., Rohnert, H., Buschmann, F. (2000). "Pattern-Oriented Software Architecture, Volume 2: Patterns for Concurrent and Networked Objects". Wiley.

[3] IETF (2014). "RFC 7230-7235: Hypertext Transfer Protocol (HTTP/1.1)".

[4] IETF (2016). "RFC 7807: Problem Details for HTTP APIs".

[5] Gamma, E., Helm, R., Johnson, R., Vlissides, J. (1994). "Design Patterns: Elements of Reusable Object-Oriented Software". Addison-Wesley.

---

#### **Kapitel 11: Security**

**11.1 Authentication & Authorization (RBAC)**
- Authentication Mechanisms
  - JWT (JSON Web Tokens): RFC 7519
  - API Keys
  - OAuth 2.0: RFC 6749
  - mTLS (Mutual TLS)
- JWT Implementation
  - Header: Algorithm (HS256, RS256)
  - Payload: Claims (user_id, roles, exp)
  - Signature: HMAC or RSA
  - Stateless Authentication
- Role-Based Access Control (RBAC)
  - Sandhu, R., et al. (1996). "Role-Based Access Control Models"
  - Users → Roles → Permissions
  - Principle of Least Privilege
  - Role Hierarchies
- Permission Model
  - Resource: Table, Collection
  - Action: READ, WRITE, DELETE, ADMIN
  - Scope: Global, Database, Table, Document
- Vergleich Authorization Models
  - ACL (Access Control Lists): Fine-grained
  - RBAC: Scalable, Manageable
  - ABAC (Attribute-Based): Most Flexible
  - ThemisDB: RBAC für Balance

**11.2 Field-Level Encryption**
- Encryption-at-Rest
  - AES-256-GCM
  - NIST SP 800-38D: Galois/Counter Mode
  - Authenticated Encryption
- Field-Level vs. Full-DB Encryption
  - Field-Level: Selective, Query-Aware
  - Full-DB: Transparent, All-or-Nothing
  - ThemisDB: Field-Level für Flexibility
- Encryption Implementation
  - Encrypt on Write
  - Decrypt on Read
  - Encrypted Index Support (Limited)
- Key Rotation
  - Lazy Re-Encryption
  - Background Re-Encryption
  - Zero-Downtime Rotation
- Performance Impact
  - Encryption Overhead: ~10-20%
  - AES-NI Hardware Acceleration
  - Batch Operations Optimization

**11.3 Key Management (VCC-PKI)**
- Key Management Challenges
  - Boneh, D., Shoup, V. (2020). "A Graduate Course in Applied Cryptography"
  - Key Generation
  - Key Distribution
  - Key Rotation
  - Key Revocation
- VCC-PKI Design
  - Virtual Cluster Coordinator PKI
  - Hierarchical Key Structure
  - Master Key → Data Encryption Keys (DEK)
  - Key Derivation Function (KDF)
- Key Storage
  - Hardware Security Module (HSM)
  - Cloud KMS (AWS KMS, GCP KMS)
  - Vault (HashiCorp)
  - Environment Variables (Development)
- Key Rotation Strategy
  - Automated Rotation Schedule
  - Manual Emergency Rotation
  - Gradual Re-Encryption
  - Audit Trail

**11.4 TLS/SSL**
- Transport Layer Security
  - TLS 1.3: RFC 8446
  - Perfect Forward Secrecy (PFS)
  - Certificate Validation
- Certificate Management
  - X.509 Certificates
  - Let's Encrypt Integration
  - Certificate Rotation
  - CRL (Certificate Revocation List)
- Cipher Suite Selection
  - Recommended: TLS_AES_256_GCM_SHA384
  - Avoid: Deprecated Ciphers (RC4, DES)
  - OWASP Guidelines
- mTLS (Mutual TLS)
  - Client Certificate Authentication
  - Strong Authentication
  - Zero Trust Architecture

**11.5 Audit Logging**
- Audit Requirements
  - Who (User/Role)
  - What (Action)
  - When (Timestamp)
  - Where (Resource)
  - How (Success/Failure)
- Audit Log Format
  - Structured Logging (JSON)
  - Tamper-Evident (Hashing/Signing)
  - Searchable
  - Archivable
- Compliance Requirements
  - GDPR: Art. 30 (Records of Processing Activities)
  - SOC 2: Logging and Monitoring
  - ISO 27001: A.12.4 (Log Management)
  - HIPAA: Audit Controls
- Log Storage
  - Separate Audit Database
  - Write-Once Storage
  - Long-Term Retention (7 years+)
  - SIEM Integration (Splunk, ELK)

**11.6 Vergleichende Security-Analyse**

| System | Auth | Encryption | TLS | Audit |
|--------|------|------------|-----|-------|
| PostgreSQL | SCRAM-SHA-256 | pgcrypto | TLS 1.3 | pg_audit |
| MongoDB | SCRAM-SHA-256 | Field-Level | TLS 1.3 | Audit Log |
| CouchDB | Basic/Cookie | - | TLS | Log Files |
| ArangoDB | JWT | Field-Level | TLS | Enterprise |
| **ThemisDB** | JWT/mTLS | Field-Level (VCC-PKI) | TLS 1.3 | Structured |

**ThemisDB Security Posture:**
- Defense in Depth
- Encryption Everywhere
- Least Privilege
- Audit Everything

**Referenzdokumente:**
- `docs/security/security_overview.md`
- `docs/security/security_encryption_strategy.md`
- `docs/security/security_key_management.md`
- `include/security/encryption_manager.hpp`

**Vollständige Bibliographie (Kapitel 11):**

[1] Sandhu, R., Coyne, E., Feinstein, H., Youman, C. (1996). "Role-Based Access Control Models". IEEE Computer, 29(2), 38-47.

[2] NIST (2007). "SP 800-38D: Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM) and GMAC".

[3] Boneh, D., Shoup, V. (2020). "A Graduate Course in Applied Cryptography". Cambridge University Press.

[4] IETF (2018). "RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3".

[5] IETF (2015). "RFC 7519: JSON Web Token (JWT)".

[6] IETF (2012). "RFC 6749: The OAuth 2.0 Authorization Framework".

---

#### **Kapitel 12: Content Pipeline**

**12.1 Pipeline-Architektur**
- Pipeline Pattern
  - Gamma, E., et al. (1994). "Design Patterns" (Pipes and Filters)
  - Data Flow Architecture
  - Stage-based Processing
  - Composable Components
- Content Processing Workflow
  - Input: Raw Documents (PDF, HTML, Text)
  - Extraction: Text, Metadata, Entities
  - Transformation: Normalization, Cleaning
  - Enrichment: Embeddings, Tags
  - Output: Structured Data + Indexes
- Pipeline Stages
  - **Ingestion**: File Upload, Stream Processing
  - **Extraction**: Content Parsing
  - **Analysis**: NLP, Entity Recognition
  - **Indexing**: Full-Text, Vector
  - **Storage**: Persist to Database
- Parallelization Strategy
  - Parallel Stages (TBB Task Groups)
  - Batch Processing
  - Stream Processing
  - Backpressure Handling

**12.2 Content Processors**
- Processor Interface
  - `process(Document) → Document`
  - Chainable
  - Stateless
  - Error Handling
- Built-in Processors
  - **TextExtractor**: PDF, DOCX, HTML
  - **TokenExtractor**: Tokenization
  - **EntityExtractor**: NER (Named Entity Recognition)
  - **EmbeddingGenerator**: Vector Embeddings
  - **MetadataExtractor**: Author, Date, Language
- Custom Processors
  - Plugin Architecture
  - Dynamic Loading
  - Configuration via JSON/YAML
- Processor Composition
  - Sequential: P1 → P2 → P3
  - Parallel: [P1, P2, P3] → Merge
  - Conditional: if condition then P1 else P2

**12.3 Text Extraction**
- PDF Extraction
  - PDFBox (Java), poppler (C++)
  - Text Layer vs. OCR
  - Layout Preservation
- HTML Extraction
  - Beautiful Soup Concept
  - DOM Traversal
  - Script/Style Removal
  - Boilerplate Detection
- Office Documents
  - DOCX: XML-based
  - XLSX: Spreadsheet Parsing
  - PPTX: Slide Extraction
- OCR (Optical Character Recognition)
  - Tesseract Integration
  - Smith, R. (2007). "An Overview of the Tesseract OCR Engine"
  - Image Preprocessing
  - Language Detection

**12.4 Entity Extraction**
- Named Entity Recognition (NER)
  - Nadeau, D., Sekine, S. (2007). "A Survey of Named Entity Recognition and Classification"
  - Person, Organization, Location
  - Rule-Based vs. ML-Based
- NER Algorithms
  - CRF (Conditional Random Fields)
  - BiLSTM-CRF
  - Transformer-based (BERT)
- Entity Linking
  - Disambiguation
  - Knowledge Base Linking
  - Entity Resolution
- Custom Entity Types
  - Domain-Specific Entities
  - Regex Patterns
  - Dictionary-Based

**12.5 Embedding Generation**
- Word Embeddings
  - Word2Vec: Mikolov, T., et al. (2013). "Efficient Estimation of Word Representations"
  - GloVe: Pennington, J., et al. (2014). "GloVe: Global Vectors for Word Representation"
  - FastText: Bojanowski, P., et al. (2017). "Enriching Word Vectors with Subword Information"
- Sentence/Document Embeddings
  - BERT: Devlin, J., et al. (2019). "BERT: Pre-training of Deep Bidirectional Transformers"
  - Sentence-BERT: Reimers, N., Gurevych, I. (2019). "Sentence-BERT"
  - Universal Sentence Encoder
- Embedding Models Integration
  - ONNX Runtime
  - TensorFlow Lite
  - PyTorch C++ Frontend
  - Model Serving
- Dimensionality
  - 128, 256, 512, 768 dimensions
  - Trade-off: Accuracy vs. Storage
  - PCA/t-SNE for Reduction

**12.6 Vergleichende Content Pipeline Analyse**

| System | Text Extract | NER | Embeddings | Search |
|--------|--------------|-----|------------|--------|
| Elasticsearch | Tika | - | Dense Vector | Full-Text |
| Vespa | Custom | - | Transformers | Hybrid |
| Weaviate | - | - | Transformers | Vector-First |
| Milvus | - | - | External | Vector-Only |
| **ThemisDB** | Built-in | Optional | Pluggable | Multi-Model |

**ThemisDB Flexibility:**
- Built-in Basic Processors
- Plugin für Advanced NLP
- Bring-Your-Own Embeddings
- Unified Storage

**Referenzdokumente:**
- `docs/architecture/architecture_content_pipeline.md`
- `docs/content/content_overview.md`
- `include/content/content_processor.hpp`
- `src/content/text_extractor.cpp`

**Vollständige Bibliographie (Kapitel 12):**

[1] Gamma, E., Helm, R., Johnson, R., Vlissides, J. (1994). "Design Patterns: Elements of Reusable Object-Oriented Software". Addison-Wesley.

[2] Smith, R. (2007). "An Overview of the Tesseract OCR Engine". Proceedings of the Ninth International Conference on Document Analysis and Recognition, 629-633.

[3] Nadeau, D., Sekine, S. (2007). "A Survey of Named Entity Recognition and Classification". Lingvisticae Investigationes, 30(1), 3-26.

[4] Mikolov, T., Chen, K., Corrado, G., Dean, J. (2013). "Efficient Estimation of Word Representations in Vector Space". ICLR.

[5] Pennington, J., Socher, R., Manning, C. (2014). "GloVe: Global Vectors for Word Representation". EMNLP, 1532-1543.

[6] Bojanowski, P., Grave, E., Joulin, A., Mikolov, T. (2017). "Enriching Word Vectors with Subword Information". TACL, 5, 135-146.

[7] Devlin, J., Chang, M., Lee, K., Toutanova, K. (2019). "BERT: Pre-training of Deep Bidirectional Transformers for Language Understanding". NAACL, 4171-4186.

[8] Reimers, N., Gurevych, I. (2019). "Sentence-BERT: Sentence Embeddings using Siamese BERT-Networks". EMNLP-IJCNLP, 3982-3992.

---

### **TEIL IV: MULTI-MODEL-FÄHIGKEITEN**

#### **Kapitel 13: Graph Database**

**13.1 Graph-Modell in ThemisDB**
- Property Graph Model
  - Rodriguez, M. A., Neubauer, P. (2010). "Constructions from Dots and Lines"
  - Vertices (Nodes) + Edges (Relationships)
  - Properties on both Vertices and Edges
  - Directed vs. Undirected Graphs
- Graph Storage Strategies
  - Native Graph Storage (Neo4j): Pointer-based
  - Document-based Graph Storage (ArangoDB, ThemisDB)
  - Relational Graph Storage (PostgreSQL + Extensions)
- ThemisDB Graph Design
  - Vertices as Documents
  - Edges with `_from` and `_to` fields
  - Edge Collections
  - Bidirectional Indexing
- Vergleich Graph Models
  - RDF (Resource Description Framework): Triples
  - Labeled Property Graph: Properties + Labels
  - Hypergraphs: N-ary relationships
  - ThemisDB: Labeled Property Graph

**13.2 Edge Storage und Indexierung**
- Edge Representation
  - `_from`: Source Vertex ID
  - `_to`: Target Vertex ID
  - Properties: Weight, Type, Metadata
- Edge Collections
  - Separate Collection für Edges
  - Schema: Flexible Properties
  - Versioning mit MVCC
- Graph Indexierung
  - **Outgoing Edge Index (Outdex)**: `_from` → Edges
  - **Incoming Edge Index (Indeg)**: `_to` → Edges
  - Composite Indexes: `(_from, edge_type)`
  - Performance: O(1) Edge Lookup
- Storage Overhead
  - Edge Storage: ~100 bytes per edge
  - Index Overhead: 2x (Outdex + Indeg)
  - Comparison: Native vs. Document-based

**13.3 Traversal-Algorithmen**
- Breadth-First Search (BFS)
  - Cormen, T. H., et al. (2009). "Introduction to Algorithms, 3rd Edition"
  - Level-by-Level Exploration
  - Queue-based Implementation
  - Shortest Path (Unweighted Graphs)
- Depth-First Search (DFS)
  - Stack-based (or Recursive)
  - Path Finding
  - Cycle Detection
- Bidirectional Search
  - Search from both Source and Target
  - Meet-in-the-Middle
  - 2x Speedup
- Traversal Optimization
  - Early Termination
  - Visited Set (Hash Set)
  - Edge Direction Filtering

**13.4 Shortest-Path Algorithmen**
- Dijkstra's Algorithm
  - Dijkstra, E. W. (1959). "A Note on Two Problems in Connexion with Graphs"
  - Single-Source Shortest Path
  - Priority Queue (Min-Heap)
  - Complexity: O((V + E) log V)
- A* (A-Star) Algorithm
  - Hart, P., et al. (1968). "A Formal Basis for the Heuristic Determination of Minimum Cost Paths"
  - Heuristic-guided Search
  - Faster than Dijkstra with good heuristics
  - Used in Navigation Systems
- Bellman-Ford Algorithm
  - Negative Weights Support
  - Slower: O(VE)
- All-Pairs Shortest Path
  - Floyd-Warshall: O(V³)
  - Johnson's Algorithm: O(V² log V + VE)

**13.5 Advanced Graph Operations**
- Pattern Matching
  - Cypher-style MATCH queries
  - Subgraph Isomorphism
  - Regular Path Queries
- Community Detection
  - Louvain Method: Blondel, V. D., et al. (2008)
  - Label Propagation
  - Modularity Optimization
- Centrality Measures
  - PageRank: Brin, S., Page, L. (1998)
  - Betweenness Centrality
  - Closeness Centrality
- Graph Analytics
  - Connected Components
  - Strongly Connected Components (Tarjan's Algorithm)
  - Triangle Counting

**13.6 Vergleichende Graph DB Analyse**

| System | Storage | Traversal | Query Language | ACID |
|--------|---------|-----------|----------------|------|
| Neo4j | Native Pointers | Fast | Cypher | Full |
| ArangoDB | Document-based | Good | AQL | Full |
| JanusGraph | Pluggable | Moderate | Gremlin | Tunable |
| Amazon Neptune | Dual (TinkerPop/RDF) | Good | Gremlin/SPARQL | Full |
| **ThemisDB** | Document-based | Good | AQL | Full |

**ThemisDB Graph Approach:**
- Unified Storage with Documents
- Efficient Bidirectional Indexes
- ACID Transactions
- Multi-Model Queries

**Referenzdokumente:**
- `docs/features/features_graph.md`
- `docs/index/index_graph.md`
- `include/graph/graph_traversal.hpp`
- `src/graph/shortest_path.cpp`

**Vollständige Bibliographie (Kapitel 13):**

[1] Rodriguez, M. A., Neubauer, P. (2010). "Constructions from Dots and Lines". Bulletin of the American Society for Information Science and Technology, 36(6), 35-41.

[2] Cormen, T. H., Leiserson, C. E., Rivest, R. L., Stein, C. (2009). "Introduction to Algorithms, 3rd Edition". MIT Press.

[3] Dijkstra, E. W. (1959). "A Note on Two Problems in Connexion with Graphs". Numerische Mathematik, 1(1), 269-271.

[4] Hart, P. E., Nilsson, N. J., Raphael, B. (1968). "A Formal Basis for the Heuristic Determination of Minimum Cost Paths". IEEE Transactions on Systems Science and Cybernetics, 4(2), 100-107.

[5] Blondel, V. D., Guillaume, J.-L., Lambiotte, R., Lefebvre, E. (2008). "Fast Unfolding of Communities in Large Networks". Journal of Statistical Mechanics: Theory and Experiment, P10008.

[6] Brin, S., Page, L. (1998). "The Anatomy of a Large-Scale Hypertextual Web Search Engine". Computer Networks and ISDN Systems, 30(1-7), 107-117.

---

#### **Kapitel 14: Vector Database**

**14.1 Vector Embeddings: Konzepte und Anwendungen**
- Embedding Spaces
  - High-Dimensional Vectors (128-1536 dimensions)
  - Semantic Similarity
  - Dense vs. Sparse Vectors
- Embedding Use Cases
  - Text Similarity (Sentence Embeddings)
  - Image Search (CNN Features)
  - Recommendation Systems
  - Anomaly Detection
- Distance Metrics
  - **Euclidean Distance**: L2 Norm
  - **Cosine Similarity**: Angle-based
  - **Dot Product**: Inner Product
  - **Manhattan Distance**: L1 Norm
- Normalization
  - L2 Normalization
  - MinMax Scaling
  - StandardScaler

**14.2 HNSW-Algorithmus**
- Hierarchical Navigable Small World
  - Malkov, Y., Yashunin, D. (2018). "Efficient and robust approximate nearest neighbor search using HNSW"
  - Multi-Layer Graph Structure
  - Greedy Search with Backtracking
  - Probabilistic Layer Assignment
- HNSW Construction
  - Layer Selection: exp(-ln(uniform(0,1)) * ml)
  - Insert Algorithm: Top-down
  - Connection Creation: M neighbors per layer
  - Complexity: O(log N) search, O(log N) insertion
- HNSW Parameters
  - **M**: Max connections per element (16-64)
  - **ef_construction**: Search scope during build (100-400)
  - **ef_search**: Search scope during query (100-500)
- Vergleich ANN Algorithmen
  - **LSH (Locality-Sensitive Hashing)**: Indyk, P., Motwani, R. (1998)
  - **Annoy (Spotify)**: Tree-based
  - **FAISS (Facebook)**: Inverted File + Product Quantization
  - **ScaNN (Google)**: Learned Quantization
  - **HNSW**: Best Recall/Speed Trade-off

**14.3 Similarity Search Operations**
- k-Nearest Neighbors (k-NN)
  - Find k most similar vectors
  - Applications: Recommendation, Clustering
  - Exact vs. Approximate
- Range Search
  - Find all vectors within distance r
  - Radius-based Queries
  - Less common than k-NN
- Batch Operations
  - Multiple Query Vectors
  - Parallelization with TBB
  - Batch Efficiency
- Filtering
  - Pre-Filtering: Filter before search
  - Post-Filtering: Filter after search
  - Hybrid: Best Recall

**14.4 Vector Index Optimierung**
- Index Building Strategy
  - Incremental vs. Batch Building
  - Memory Budget
  - Build Time vs. Query Time Trade-off
- Quantization
  - Product Quantization: Jégou, H., et al. (2011)
  - Scalar Quantization
  - Space Reduction: 8x-32x
  - Accuracy Loss: 1-5%
- Memory Management
  - On-Disk vs. In-Memory
  - Memory-Mapped Index
  - Cache Strategies
- Multi-Vector Queries
  - Query Fusion
  - Reciprocal Rank Fusion
  - Weighted Combination

**14.5 Index Persistence und Recovery**
- Index Serialization
  - Binary Format
  - Version Tagging
  - Checksum Validation
- Incremental Updates
  - Add Vectors: O(log N)
  - Delete Vectors: Mark as Deleted
  - Rebuild Threshold
- Recovery Mechanisms
  - Index Checkpoint
  - WAL for Vector Ops
  - Crash Recovery
- Index Statistics
  - Vector Count
  - Dimensionality
  - Average Degree
  - Memory Usage

**14.6 Vergleichende Vector DB Analyse**

| System | Algorithm | Language | Max Dimensions | Filters |
|--------|-----------|----------|----------------|---------|
| Milvus | IVF, HNSW | Go/C++ | 32768 | ✅ Pre/Post |
| Weaviate | HNSW | Go | 65536 | ✅ Pre |
| Qdrant | HNSW | Rust | 65536 | ✅ Pre/Post |
| Pinecone | Proprietary | Cloud | 20000 | ✅ Metadata |
| **ThemisDB** | HNSW | C++ | 4096 | ✅ AQL Filters |

**ThemisDB Vector Strengths:**
- Unified Multi-Model Queries
- ACID Transactions on Vectors
- Flexible AQL Filtering
- Embedded, No Separate Service

**Referenzdokumente:**
- `docs/features/features_vector_ops.md`
- `docs/index/index_vector.md`
- `include/vector/hnsw_index.hpp`
- `src/vector/similarity_search.cpp`

**Vollständige Bibliographie (Kapitel 14):**

[1] Malkov, Y. A., Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs". IEEE Transactions on Pattern Analysis and Machine Intelligence, 42(4), 824-836.

[2] Indyk, P., Motwani, R. (1998). "Approximate Nearest Neighbors: Towards Removing the Curse of Dimensionality". Proceedings of STOC, 604-613.

[3] Jégou, H., Douze, M., Schmid, C. (2011). "Product Quantization for Nearest Neighbor Search". IEEE Transactions on Pattern Analysis and Machine Intelligence, 33(1), 117-128.

[4] Johnson, J., Douze, M., Jégou, H. (2019). "Billion-scale similarity search with GPUs". IEEE Transactions on Big Data.

---

#### **Kapitel 15: Time Series**

**15.1 Time Series-Modell**
- Time Series Fundamentals
  - Timestamped Data Points
  - Regular vs. Irregular Intervals
  - Univariate vs. Multivariate
- Time Series Storage Patterns
  - Row-Based: One row per measurement
  - Columnar: Column per metric
  - Hybrid: ThemisDB approach
- Data Model Design
  - Timestamp (64-bit Unix time)
  - Metric Name/ID
  - Value (Float64, Int64)
  - Tags/Labels (Metadata)
- Vergleich TS Databases
  - InfluxDB: Line Protocol, TSM Engine
  - TimescaleDB: PostgreSQL Extension, Hypertables
  - Prometheus: Pull-based, Local Storage
  - OpenTSDB: HBase-based
  - ThemisDB: Multi-Model Integration

**15.2 Gorilla Compression**
- Facebook's Gorilla Algorithm
  - Pelkonen, T., et al. (2015). "Gorilla: A Fast, Scalable, In-Memory Time Series Database"
  - Delta-of-Delta Timestamp Encoding
  - XOR-based Value Compression
  - 90%+ Compression Ratio
- Timestamp Compression
  - Delta Encoding: Δt = t[i] - t[i-1]
  - Delta-of-Delta: ΔΔt = Δt[i] - Δt[i-1]
  - Variable-Length Encoding
  - Typical Compression: 12 bytes → 1.37 bytes
- Value Compression
  - XOR with Previous Value
  - Leading/Trailing Zero Suppression
  - Control Bits: 1 bit for "same", 2 bits for "changed"
  - Float64 Compression: 8 bytes → 1-2 bytes average
- Decompression Performance
  - Sequential Read: Necessary
  - Random Access: Limited
  - Trade-off: Compression vs. Query Speed

**15.3 Continuous Aggregates**
- Materialized Views for Time Series
  - Pre-computed Aggregations
  - Rollup Windows: 1min, 5min, 1hour, 1day
  - Automatic Refresh
- Aggregation Functions
  - COUNT, SUM, AVG
  - MIN, MAX
  - STDDEV, VARIANCE
  - FIRST, LAST
  - PERCENTILE (P50, P95, P99)
- Incremental Maintenance
  - Background Worker
  - Watermark Tracking
  - Partial Aggregates
- Use Cases
  - Dashboards: Real-time Metrics
  - Alerting: Threshold Monitoring
  - Analytics: Historical Trends

**15.4 Retention Policies**
- Time-to-Live (TTL)
  - Automatic Data Deletion
  - Per-Metric Policies
  - Space Management
- Retention Strategies
  - Drop Old Data: Simple Deletion
  - Downsampling: Reduce Resolution
  - Archiving: Move to Cold Storage
- Implementation
  - Background Compaction Filter
  - Timestamp-based Deletion
  - Lazy Deletion during Compaction
- Compliance
  - GDPR: Right to be Forgotten
  - Data Minimization
  - Audit Trail for Deletions

**15.5 Downsampling**
- Downsampling Concept
  - Reduce Data Resolution
  - Aggregate High-Frequency → Low-Frequency
  - Example: 1sec → 1min → 1hour
- Downsampling Methods
  - **Averaging**: Mean of window
  - **Sampling**: First/Last/Random point
  - **Min/Max**: Range preservation
  - **Sum**: Total aggregation
- Multi-Level Downsampling
  - Raw Data: 1 second (7 days retention)
  - Level 1: 1 minute (90 days retention)
  - Level 2: 1 hour (1 year retention)
  - Level 3: 1 day (∞ retention)
- Implementation
  - Continuous Aggregates + TTL
  - Background Jobs
  - Query Routing to appropriate level

**15.6 Vergleichende Time Series DB Analyse**

| System | Compression | Aggregates | Retention | Query Language |
|--------|-------------|------------|-----------|----------------|
| InfluxDB | TSM | Continuous Queries | TTL | InfluxQL, Flux |
| TimescaleDB | PostgreSQL | Continuous Aggregates | Retention Policies | SQL |
| Prometheus | Gorilla-like | Recording Rules | Retention | PromQL |
| OpenTSDB | HBase | Pre-Aggregation | TTL | Custom |
| **ThemisDB** | Gorilla | Continuous Aggregates | TTL + Downsample | AQL |

**ThemisDB TS Advantages:**
- Multi-Model: Join TS with Documents/Graph
- ACID Transactions
- Flexible Retention Policies
- Standard AQL Queries

**Referenzdokumente:**
- `docs/features/features_time_series.md`
- `docs/timeseries/timeseries_overview.md`
- `include/timeseries/gorilla_compression.hpp`
- `src/timeseries/continuous_aggregates.cpp`

**Vollständige Bibliographie (Kapitel 15):**

[1] Pelkonen, T., Franklin, S., Teller, J., et al. (2015). "Gorilla: A Fast, Scalable, In-Memory Time Series Database". Proceedings of the VLDB Endowment, 8(12), 1816-1827.

[2] Dunning, T., Friedman, E. (2016). "Streaming Architecture: New Designs Using Apache Kafka and MapR Streams". O'Reilly Media.

[3] Shvachko, K., Kuang, H., Radia, S., Chansler, R. (2010). "The Hadoop Distributed File System". IEEE MSST, 1-10.

---

#### **Kapitel 16: Geospatial**

**16.1 Spatial Data Types**
- Geometric Primitives
  - Point: (longitude, latitude)
  - LineString: Sequence of Points
  - Polygon: Closed LineString
  - MultiPoint, MultiLineString, MultiPolygon
- Coordinate Systems
  - WGS84 (GPS): lat/lon, EPSG:4326
  - Web Mercator: EPSG:3857
  - Projected Coordinates
  - Snyder, J. P. (1987). "Map Projections - A Working Manual"
- GeoJSON Standard
  - RFC 7946: GeoJSON Format
  - Feature Collections
  - Geometry Objects
  - Properties
- Well-Known Text (WKT)
  - OGC Simple Features Specification
  - Text Representation
  - POINT(lon lat), LINESTRING, POLYGON

**16.2 R-Tree und R*-Tree Indexierung**
- R-Tree Fundamentals
  - Guttman, A. (1984). "R-Trees: A Dynamic Index Structure for Spatial Searching"
  - Bounding Boxes (MBRs)
  - Hierarchical Structure
  - Overlap Minimization
- R*-Tree Improvements
  - Beckmann, N., et al. (1990). "The R*-tree: An Efficient and Robust Access Method"
  - Better Split Algorithm
  - Forced Reinsert
  - Improved Query Performance
- Insertion Algorithm
  - ChooseLeaf: Minimize area enlargement
  - Split Node: Quadratic/Linear split
  - Adjust Tree: Propagate changes upward
- Query Types
  - Point Query: Exact match
  - Range Query: Within bounding box
  - Nearest Neighbor: k-NN spatial
  - Intersection Query

**16.3 Geospatial Queries**
- Spatial Predicates
  - **ST_Within**: Point/Polygon containment
  - **ST_Intersects**: Overlap check
  - **ST_Distance**: Distance calculation
  - **ST_DWithin**: Within distance radius
  - **ST_Contains**: Reverse containment
  - **ST_Crosses**: LineString crossing
- Distance Calculations
  - **Haversine Formula**: Great circle distance
    - Sinnott, R. W. (1984). "Virtues of the Haversine"
  - **Vincenty Formula**: More accurate, ellipsoidal
  - **Euclidean Distance**: Planar approximation
- Spatial Joins
  - Brinkhoff, T., et al. (1993). "Efficient Processing of Spatial Joins"
  - R-Tree Index Scans
  - Filter-Refine Strategy
  - Plane-Sweep Algorithm
- Performance Optimization
  - Bounding Box Filter (Fast)
  - Exact Geometry Test (Slow)
  - Spatial Index Required

**16.4 GeoJSON Support**
- GeoJSON Format
  - RFC 7946 (2016)
  - JSON-based Geometry Encoding
  - FeatureCollection, Feature, Geometry
- Parsing and Validation
  - Schema Validation
  - Coordinate Validation
  - Topology Checks
- Storage Strategy
  - Native GeoJSON in Documents
  - Extracted Geometry in R*-Tree Index
  - Dual Representation
- Integration with Web Maps
  - Leaflet.js
  - Mapbox GL JS
  - Google Maps
  - OpenStreetMap

**16.5 Vergleichende Geospatial DB Analyse**

| System | Index | Formats | Functions | Standard |
|--------|-------|---------|-----------|----------|
| PostGIS | GiST, SP-GiST | WKT, WKB, GeoJSON | 400+ | OGC SFS |
| MongoDB | 2dsphere | GeoJSON | 20+ | GeoJSON |
| Elasticsearch | BKD-Tree | GeoJSON, WKT | 30+ | GeoJSON |
| ArangoDB | S2 Geometry | GeoJSON | 20+ | GeoJSON |
| **ThemisDB** | R*-Tree | GeoJSON, WKT | 15+ | OGC SFS |

**ThemisDB Geo Positioning:**
- Multi-Model: Geo + Graph + Vector
- R*-Tree for Performance
  - Standard GeoJSON Support
- Essential Functions (vs. Exhaustive)

**Referenzdokumente:**
- `docs/geo/geo_architecture.md`
- `docs/features/features_geo.md`
- `include/geo/rtree_index.hpp`
- `src/geo/spatial_queries.cpp`

**Vollständige Bibliographie (Kapitel 16):**

[1] Guttman, A. (1984). "R-Trees: A Dynamic Index Structure for Spatial Searching". ACM SIGMOD Record, 14(2), 47-57.

[2] Beckmann, N., Kriegel, H.-P., Schneider, R., Seeger, B. (1990). "The R*-tree: An Efficient and Robust Access Method for Points and Rectangles". ACM SIGMOD Record, 19(2), 322-331.

[3] Snyder, J. P. (1987). "Map Projections - A Working Manual". U.S. Geological Survey Professional Paper 1395.

[4] Sinnott, R. W. (1984). "Virtues of the Haversine". Sky and Telescope, 68(2), 159.

[5] Brinkhoff, T., Kriegel, H.-P., Seeger, B. (1993). "Efficient Processing of Spatial Joins Using R-Trees". ACM SIGMOD Record, 22(2), 237-246.

[6] IETF (2016). "RFC 7946: The GeoJSON Format".

---

#### **Kapitel 17: Hybrid Search**

**17.1 Multi-Modal Search Integration**
- Hybrid Search Concept
  - Combining Multiple Search Paradigms
  - Full-Text + Vector + Graph
  - Complementary Strengths
- Search Modalities
  - **Keyword Search**: Traditional full-text
  - **Semantic Search**: Vector similarity
  - **Structural Search**: Graph traversal
  - **Spatial Search**: Geospatial queries
- Use Cases
  - E-Commerce: Product search with recommendations
  - Knowledge Graphs: Entity + Semantic search
  - Document Search: Content + Metadata + Embeddings
- Challenges
  - Score Normalization
  - Result Fusion
  - Performance Optimization

**17.2 Ranking Strategies**
- Score Normalization
  - Min-Max Normalization
  - Z-Score Normalization
  - Sigmoid Function
  - Issue: Different Score Ranges
- BM25 for Full-Text
  - Robertson, S., Zaragoza, H. (2009). "The Probabilistic Relevance Framework: BM25 and Beyond"
  - Term Frequency Saturation
  - Document Length Normalization
  - Typical Scores: 0-20
- Cosine Similarity for Vectors
  - Range: [-1, 1] or [0, 1] (normalized)
  - Semantic Similarity
  - Typical Scores: 0.7-0.99 for relevant
- Graph Relevance
  - PageRank-based Scoring
  - Path Length Penalty
  - Edge Weight Integration

**17.3 Query Fusion Techniques**
- Reciprocal Rank Fusion (RRF)
  - Cormack, G. V., et al. (2009). "Reciprocal Rank Fusion outperforms Condorcet and Individual Rank Learning"
  - Formula: RRF(d) = Σ 1/(k + rank(d))
  - k = 60 (typical)
  - No Score Calibration Needed
- Linear Combination
  - Weighted Sum: α·BM25 + β·Cosine + γ·Graph
  - Weight Tuning: Grid Search, Bayesian Optimization
  - Requires Score Normalization
- Cascade Ranking
  - Stage 1: Fast pre-filtering (BM25)
  - Stage 2: Re-ranking (Vector similarity)
  - Stage 3: Fine-tuning (Graph context)
- Learning to Rank (L2R)
  - Liu, T.-Y. (2009). "Learning to Rank for Information Retrieval"
  - LambdaMART, RankNet
  - Feature Engineering
  - ML Model Training

**17.4 Performance-Optimierung**
- Query Planning
  - Push-Down Filters Early
  - Index Selection
  - Parallel Execution
- Early Termination
  - Top-k Pruning
  - Score Threshold
  - Time Budgets
- Caching Strategies
  - Query Result Cache
  - Embedding Cache
  - Graph Path Cache
- Approximate Methods
  - ANN instead of Exact k-NN
  - Sampling for Large Result Sets
  - Probabilistic Data Structures (Bloom Filters)

**17.5 Vergleichende Hybrid Search Analyse**

| System | Full-Text | Vector | Graph | Fusion |
|--------|-----------|--------|-------|--------|
| Elasticsearch | BM25 | Dense Vector | - | Script-based |
| Weaviate | BM25 | HNSW | - | Alpha (Weighted) |
| Vespa | BM25 | ANN | - | Ranking Expressions |
| ArangoDB | - | - | Native | AQL |
| **ThemisDB** | BM25 | HNSW | Native | AQL + RRF |

**ThemisDB Hybrid Search Strengths:**
- True Multi-Model in Single Query
- AQL for Complex Combinations
- ACID Transactions
- Flexible Ranking Strategies

**Referenzdokumente:**
- `docs/search/hybrid_search_design.md`
- `docs/query/query_fusion.md`
- `include/search/hybrid_ranker.hpp`
- `src/search/reciprocal_rank_fusion.cpp`

**Vollständige Bibliographie (Kapitel 17):**

[1] Robertson, S., Zaragoza, H. (2009). "The Probabilistic Relevance Framework: BM25 and Beyond". Foundations and Trends in Information Retrieval, 3(4), 333-389.

[2] Cormack, G. V., Clarke, C. L. A., Buettcher, S. (2009). "Reciprocal Rank Fusion outperforms Condorcet and Individual Rank Learning Methods". Proceedings of SIGIR, 758-759.

[3] Liu, T.-Y. (2009). "Learning to Rank for Information Retrieval". Foundations and Trends in Information Retrieval, 3(3), 225-331.

[4] Burges, C. J. C. (2010). "From RankNet to LambdaRank to LambdaMART: An Overview". Microsoft Research Technical Report.

---

### **TEIL V: ENTERPRISE-FEATURES**

#### **Kapitel 18: Sharding und Horizontale Skalierung**
- 18.1 Sharding-Strategie
- 18.2 VCC-URN Consistent Hashing
- 18.3 Shard-Koordination
- 18.4 P2P Gossip Protocol
- 18.5 Auto-Rebalancing

**Referenzdokumente:**
- `docs/sharding/sharding_overview.md`
- `docs/sharding/sharding_vcc_urn.md`
- `docs/reports/SHARDING_AUTO_REBALANCING.md`

---

#### **Kapitel 19: Replication**
- 19.1 Leader-Follower Replication
- 19.2 Multi-Master Replication
- 19.3 CRDT-basierte Konfliktauflösung
- 19.4 Vector Clocks und Hybrid Logical Clocks
- 19.5 RAID-like Redundancy (MIRROR, STRIPE, PARITY)

**Referenzdokumente:**
- `docs/replication/README.md`
- `docs/replication/replication_crdt.md`

---

#### **Kapitel 20: GPU Acceleration**
- 20.1 GPU-Computing-Architektur
- 20.2 CUDA-Backend
- 20.3 Vulkan-Backend
- 20.4 Weitere Backends (HIP, DirectX, OpenCL, OneAPI)
- 20.5 Performance-Benchmarks

**Referenzdokumente:**
- `docs/performance/performance_gpu.md`
- `docs/features/features_gpu.md`

---

#### **Kapitel 21: Analytics (CEP und OLAP)**
- 21.1 Complex Event Processing (CEP)
- 21.2 Event Pattern Language (EPL)
- 21.3 OLAP Operations (CUBE, ROLLUP)
- 21.4 Window Functions
- 21.5 Columnar Store

**Referenzdokumente:**
- `docs/analytics/analytics_cep.md`
- `docs/analytics/analytics_olap.md`

---

#### **Kapitel 22: Multi-Tenancy und Rate Limiting**
- 22.1 Tenant Isolation
- 22.2 Resource Quotas
- 22.3 Token Bucket Rate Limiter
- 22.4 Per-Client Rate Limiting
- 22.5 Load Shedding

**Referenzdokumente:**
- `docs/enterprise/README.md`
- `docs/enterprise/enterprise_scalability.md`

---

### **TEIL VI: ECOSYSTEM UND ZUKUNFT**

#### **Kapitel 23: Client SDKs**
- 23.1 SDK-Architektur
- 23.2 Python SDK
- 23.3 JavaScript/TypeScript SDK
- 23.4 Rust SDK
- 23.5 Go SDK
- 23.6 Java SDK
- 23.7 C# SDK
- 23.8 Swift SDK

**Referenzdokumente:**
- `docs/clients/README.md`
- `clients/*/README.md` (für jedes SDK)

---

#### **Kapitel 24: Admin Tools und Operations**
- 24.1 WPF Admin Tools (7 Tools)
- 24.2 Monitoring und Observability
- 24.3 Backup und Recovery
- 24.4 Disaster Recovery
- 24.5 Operations Runbook

**Referenzdokumente:**
- `docs/admin_tools/README.md`
- `docs/observability/README.md`
- `docs/guides/guides_operations_runbook.md`

---

#### **Kapitel 25: Zukunft und Roadmap**
- 25.1 Lessons Learned
- 25.2 Performance-Optimierungen
- 25.3 Geplante Features
- 25.4 Community und Open Source
- 25.5 Ausblick

**Referenzdokumente:**
- `docs/roadmap/roadmap_overview.md`
- `docs/development/DEVELOPMENT_SUMMARY.md`

---

## Anhänge

### **Anhang A: API-Referenz**
- REST API vollständige Referenz
- GraphQL Schema
- WebSocket Protocol

**Referenzdokumente:**
- `docs/api/api_reference.md`
- `openapi/openapi.yaml`

---

### **Anhang B: Code-Metriken**
- Lines of Code pro Modul
- Test Coverage
- Cyclomatic Complexity
- Performance Benchmarks

**Referenzdokumente:**
- `docs/development/SOURCE_CODE_AUDIT.md`
- `docs/reports/BENCHMARK_AND_TEST_AUDIT.md`

---

### **Anhang C: Compliance und Security**
- BSI C5
- ISO 27001
- DSGVO
- SOC 2
- Security Audit Results

**Referenzdokumente:**
- `docs/compliance/compliance_full_checklist.md`
- `docs/security/SECURITY_AUDIT_REPORT.md`

---

### **Anhang D: Glossar**
- Begriffsdefinitionen
- Akronyme
- Technische Terminologie

**Referenzdokumente:**
- `docs/glossar.md`

---

### **Anhang E: Vergleichende Systemanalyse**
- ThemisDB vs. PostgreSQL (Relational + Extensions)
- ThemisDB vs. MongoDB (Document Store)
- ThemisDB vs. Neo4j (Graph Database)
- ThemisDB vs. ArangoDB (Multi-Model)
- ThemisDB vs. InfluxDB (Time Series)
- ThemisDB vs. Milvus/Weaviate (Vector Search)
- Feature-Matrix und Performance-Vergleiche
- Architektur-Unterschiede und Design-Philosophien

**Referenzdokumente:**
- `docs/reports/competitive_gap_analysis.md`

---

### **Anhang F: Akademische Referenzen**
- Vollständige Bibliographie aller zitierten Papers
- Chronologische Entwicklung der Datenbankforschung
- Einflussreiche Arbeiten und deren Relevanz für ThemisDB
- Aktuelle Forschungsthemen und offene Probleme

**Kategorien:**
- Storage Engines (LSM, B-Tree, etc.)
- Concurrency Control (MVCC, 2PL, OCC)
- Query Processing & Optimization
- Distributed Systems & Consensus
- Index Structures (HNSW, R-Tree, etc.)
- Compression Algorithms

---

## Lesepfade für unterschiedliche Zielgruppen

### Für Einsteiger (Quick Start)
1. Kapitel 1: Einführung
2. Kapitel 4: Systemarchitektur
3. Kapitel 7: Query Engine und AQL
4. Kapitel 23: Client SDKs
5. Kapitel 24: Admin Tools

### Für Datenbank-Entwickler (Vertieft)
1. Teil I: Grundlagen (Kapitel 1-3)
2. Teil II: Architektur (Kapitel 4-7)
3. Teil III: Kern-Komponenten (Kapitel 8-12)
4. Teil IV: Multi-Model (Kapitel 13-17)
5. Anhang B: Code-Metriken

### Für Architekten (Strategisch)
1. Kapitel 1: Einführung
2. Kapitel 3: Technologie-Entscheidungen
3. Kapitel 4: Systemarchitektur
4. Teil V: Enterprise-Features (Kapitel 18-22)
5. Kapitel 25: Zukunft und Roadmap

### Für Operations/DevOps
1. Kapitel 10: HTTP Server
2. Kapitel 18: Sharding
3. Kapitel 19: Replication
4. Kapitel 22: Multi-Tenancy und Rate Limiting
5. Kapitel 24: Admin Tools und Operations
6. Anhang C: Compliance

---

## Schreibrichtlinien

### Stil
- **Technisch präzise**: Exakte Beschreibungen ohne Vereinfachungen
- **Wissenschaftlich fundiert**: Referenzen zu akademischen Papers und Forschung
- **Vollständige Quellenangaben**: Alle Behauptungen, Benchmarks und Konzepte müssen zitiert werden
- **Vergleichend**: Systematischer Vergleich mit anderen Datenbanksystemen
- **Begründet**: Jede Design-Entscheidung wird erklärt und gerechtfertigt
- **Code-Beispiele**: Reale Code-Snippets aus dem Projekt
- **Diagramme**: UML, Sequenzdiagramme, Architekturdiagramme
- **Benchmarks**: Messbare Performance-Daten mit Vergleichswerten und Quellenangaben

### Format
- **Markdown**: Alle Kapitel in Markdown
- **Code-Highlighting**: Syntax-Highlighting für C++, SQL, JSON
- **Cross-References**: Verweise zwischen Kapiteln
- **Footnotes**: Für zusätzliche Details

### Qualitätssicherung
- **Technical Review**: Peer Review durch Core-Team
- **Code Validation**: Alle Code-Beispiele müssen kompilieren
- **Link Validation**: Alle Referenzen müssen gültig sein
- **Citation Validation**: Alle Quellen müssen vollständig und korrekt zitiert sein
- **Consistency Check**: Terminologie-Konsistenz
- **Academic Rigor**: Wissenschaftliche Standards einhalten (IEEE/ACM Citation Style)

---

## Dokumentations-Mapping

Jedes Kapitel verweist auf die entsprechenden Dokumente im `docs/` Verzeichnis:

| Kapitel | Primäre Dokumente | Sekundäre Dokumente |
|---------|-------------------|---------------------|
| 1 | `README.md`, `architecture_overview.md` | `themis_sachstandsbericht_2025.md` |
| 2 | `architecture_mvcc.md`, `storage_rocksdb.md` | - |
| 3 | `guides_build_strategy.md` | `CMakeLists.txt`, `vcpkg.json` |
| 4 | `architecture_overview.md` | `architecture_strategic.md` |
| 5 | `architecture_base_entity.md` | `base_entity.hpp`, `base_entity.cpp` |
| ... | ... | ... |

(Vollständige Tabelle siehe separate Datei `book/chapter_mapping.md`)

---

## Nächste Schritte

1. **Kapitel-Templates erstellen**: Detaillierte Templates für jedes Kapitel
2. **Diagramme erstellen**: Architekturdiagramme, Sequenzdiagramme, etc.
3. **Code-Beispiele sammeln**: Repräsentative Code-Snippets aus dem Projekt
4. **Review-Prozess definieren**: Technical Review, Code Review
5. **Publishing-Strategie**: Format (PDF, HTML, E-Book), Versionierung

---

## Lizenz und Copyright

**Copyright**: © 2025 ThemisDB Development Team  
**Lizenz**: TBD (abhängig von Veröffentlichungsstrategie)

---

**Version History:**
- 1.0.0 (Dezember 2025): Initiale Struktur
