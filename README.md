<div align="center">
  <h1>🗄️ ThemisDB</h1>
  <p><strong>High-Performance Multi-Model Database with Native AI/LLM Integration</strong></p>
  
  [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
  [![Security Scanning](https://github.com/makr-code/ThemisDB/actions/workflows/security-scan.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/security-scan.yml)
  [![Documentation](https://github.com/makr-code/ThemisDB/actions/workflows/docs.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/docs.yml)
  [![Coverage](https://img.shields.io/badge/coverage-view%20report-brightgreen)](https://makr-code.github.io/ThemisDB/coverage/)
  [![Docker](https://img.shields.io/badge/docker-themisdb%2Fthemisdb-blue?logo=docker)](https://hub.docker.com/r/themisdb/themisdb)
  [![Version](https://img.shields.io/badge/version-1.4.1--dev-blue)](https://github.com/makr-code/ThemisDB/releases)
  [![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
</div>

## What is ThemisDB?

ThemisDB is a **production-ready multi-model database** that combines relational, graph, vector, and document models in a single system with full ACID transaction support. Built on RocksDB for high performance and reliability.

> *"ThemisDB keeps its own llamas."* – Optional native LLM integration with llama.cpp for AI workloads directly in your database.

### Key Features

- 🔒 **ACID Transactions** - Full snapshot isolation with MVCC
- 🔍 **Multi-Model** - Relational, Graph, Vector, Document in one database
- 🚀 **High Performance** - 45K writes/s, 120K reads/s, GPU-accelerated vector search
- 🛡️ **Enterprise Security** - TLS 1.3, RBAC, field-level encryption, audit logging
- 🧠 **AI-Ready** - Optional LLM engine, vector search, image analysis, voice assistant
- 🌐 **Modern Protocols** - HTTP/2, WebSocket, gRPC, MQTT, PostgreSQL Wire, GraphQL

**📚 [Full Documentation](https://makr-code.github.io/ThemisDB/)** · **[Release Notes](CHANGELOG.md)**

---

## Quick Start

### Request Flow Overview

```mermaid
flowchart LR
    A[Client Request] --> B{Protocol}
    B -->|REST/HTTP| C[HTTP Server]
    B -->|gRPC| D[gRPC Server]
    B -->|WebSocket| E[WebSocket Server]
    
    C & D & E --> F[Authentication]
    F --> G[Rate Limiting]
    G --> H[Query Parser]
    H --> I[Query Optimizer]
    I --> J[Execution Engine]
    
    J --> K{Operation Type}
    K -->|Read| L[MVCC Read]
    K -->|Write| M[Transaction]
    K -->|Query| N[Index Lookup]
    
    L & M & N --> O[Storage Layer]
    O --> P[Response]
    P --> Q[Client]
    
    style A fill:#e1f5ff
    style O fill:#ffe1e1
    style Q fill:#e1ffe1
```

### 🐳 Docker (Recommended)

```bash
# Pull and run the latest version
docker pull themisdb/themisdb:latest

# Run with Docker
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Verify installation
curl http://localhost:8080/health
```

**Default Ports:**
- `8080` - HTTP/REST API, GraphQL
- `18765` - Binary Wire Protocol, gRPC
- `4318` - OpenTelemetry/Prometheus metrics

> **📖 Complete Port Reference:** See [docs/de/deployment/PORT_REFERENCE.md](docs/de/deployment/PORT_REFERENCE.md)

### 💻 From Source

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Setup and build (Linux/macOS)
./scripts/setup.sh
./scripts/build.sh

# Setup and build (Windows)
.\scripts\setup.ps1
.\scripts\build.ps1

# Start server
./build/themis_server --config config.yaml
```

> **📖 Build Guide:** See [docs/de/guides/guides_build_strategy.md](docs/de/guides/guides_build_strategy.md) for detailed build instructions.

### Deployment Architecture

```mermaid
graph TB
    subgraph "Production Deployment"
        subgraph "Edge Layer"
            CDN[CDN/Edge Cache]
            WAF[Web Application Firewall]
        end
        
        subgraph "Application Layer"
            APP1[Client Application 1]
            APP2[Client Application 2]
            APP3[Client Application 3]
        end
        
        subgraph "Database Layer"
            subgraph "ThemisDB Cluster"
                DB1[ThemisDB Node 1<br/>Leader]
                DB2[ThemisDB Node 2<br/>Follower]
                DB3[ThemisDB Node 3<br/>Follower]
            end
        end
        
        subgraph "Monitoring & Observability"
            PROM[Prometheus]
            GRAF[Grafana]
            JAEGER[Jaeger Tracing]
        end
        
        subgraph "Backup & Recovery"
            BACKUP[Backup Storage<br/>S3/Object Store]
        end
    end
    
    CDN --> WAF
    WAF --> APP1 & APP2 & APP3
    APP1 & APP2 & APP3 --> DB1
    DB1 -.Replication.-> DB2 & DB3
    
    DB1 --> PROM
    PROM --> GRAF
    DB1 --> JAEGER
    DB1 -.Backup.-> BACKUP
    
    style DB1 fill:#e1ffe1
    style DB2 fill:#e1ffe1
    style DB3 fill:#e1ffe1
    style PROM fill:#e1f5ff
    style GRAF fill:#e1f5ff
```

### 📦 Package Managers

**Linux (Debian/Ubuntu):**
```bash
# Download the latest release from GitHub
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb_amd64.deb
sudo apt install ./themisdb_amd64.deb
sudo systemctl start themisdb
```

**macOS (Homebrew):**
```bash
brew install themisdb
brew services start themisdb
```

**Windows (Chocolatey):**
```powershell
choco install themisdb
```

---

## 5-Minute Tutorial

### Data Models Integration

```mermaid
graph TB
    subgraph "Application Use Cases"
        UC1[User Profiles<br/>Document Model]
        UC2[Social Graph<br/>Graph Model]
        UC3[Recommendations<br/>Vector Search]
        UC4[Metrics<br/>Time-Series]
    end
    
    subgraph "ThemisDB Unified API"
        API[Single API Endpoint]
    end
    
    subgraph "Query Processing"
        PARSER[AQL Parser]
        OPT[Query Optimizer]
    end
    
    subgraph "Execution Layer"
        DOC[Document Engine]
        GRAPH[Graph Engine]
        VECTOR[Vector Engine]
        TS[Time-Series Engine]
    end
    
    subgraph "Storage"
        STORAGE[RocksDB<br/>Unified Key-Value Store]
    end
    
    UC1 --> API
    UC2 --> API
    UC3 --> API
    UC4 --> API
    
    API --> PARSER
    PARSER --> OPT
    
    OPT --> DOC
    OPT --> GRAPH
    OPT --> VECTOR
    OPT --> TS
    
    DOC --> STORAGE
    GRAPH --> STORAGE
    VECTOR --> STORAGE
    TS --> STORAGE
    
    style API fill:#e1f5ff
    style STORAGE fill:#ffe1e1
```

```bash
# 1. Check server health
curl http://localhost:8080/health

# 2. Create an entity
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'

# 3. Create an index
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'

# 4. Query by index
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"city","value":"Berlin"}],"return":"entities"}'

# 5. View metrics
curl http://localhost:8080/metrics
```

**💡 Learn More:**
- 🚀 **[10-Minute Quickstart](docs/EXAMPLES_QUICKSTART.md)** - Hello World and CRUD operations
- 📚 **[Examples Index](docs/EXAMPLES_INDEX.md)** - Browse 37+ examples by feature
- 🎓 **[Learning Paths](docs/EXAMPLES_INDEX.md#-learning-paths)** - Guided paths for different roles

---

## Core Capabilities

### Architecture Overview

```mermaid
graph TB
    subgraph "Client Layer"
        C1[REST API]
        C2[GraphQL]
        C3[gRPC]
        C4[Wire Protocol]
        C5[Native SDKs]
    end
    
    subgraph "API & Server Layer"
        S1[HTTP Server]
        S2[Authentication]
        S3[Rate Limiting]
        S4[Load Shedding]
    end
    
    subgraph "Query Layer"
        Q1[AQL Parser]
        Q2[Query Optimizer]
        Q3[Execution Engine]
        Q4[Function Libraries]
        Q5[CTE Cache]
        Q6[Semantic Cache]
    end
    
    subgraph "Transaction & Concurrency Layer"
        T1[MVCC]
        T2[Transaction Manager]
        T3[SAGA Coordinator]
        T4[Deadlock Detection]
        T5[WAL Management]
    end
    
    subgraph "Index Layer"
        I1[Vector HNSW]
        I2[Graph]
        I3[Secondary]
        I4[Spatial]
        I5[Fulltext]
        I6[GPU Acceleration]
        I7[SIMD Optimization]
    end
    
    subgraph "Storage Layer"
        ST1[RocksDB LSM-tree]
        ST2[Key Schema]
        ST3[Compression]
        ST4[WAL]
        ST5[Snapshot Management]
        ST6[Compaction]
    end
    
    subgraph "Cross-Cutting Concerns"
        X1[Security]
        X2[Replication]
        X3[Sharding]
        X4[Monitoring]
        X5[CDC]
    end
    
    C1 & C2 & C3 & C4 & C5 --> S1
    S1 --> S2 --> S3 --> S4
    S4 --> Q1 --> Q2 --> Q3
    Q3 --> Q4 & Q5 & Q6
    Q3 --> T1
    T1 --> T2 --> T3
    T2 --> T4 & T5
    T3 --> I1 & I2 & I3 & I4 & I5
    I1 & I2 --> I6 & I7
    I1 & I2 & I3 & I4 & I5 --> ST1
    ST1 --> ST2 & ST3 & ST4 & ST5 & ST6
    ST1 -.-> X1 & X2 & X3 & X4 & X5
    
    style I1 fill:#e1f5ff
    style I2 fill:#e1f5ff
    style I3 fill:#e1f5ff
    style I4 fill:#e1f5ff
    style I5 fill:#e1f5ff
    style ST1 fill:#ffe1e1
    style X1 fill:#fff3cd
    style X2 fill:#fff3cd
    style X3 fill:#fff3cd
    style X4 fill:#fff3cd
    style X5 fill:#fff3cd
```

### Multi-Model Database
- **Relational**: SQL-like queries with secondary indexes
- **Graph**: BFS, Dijkstra, A* traversals with path constraints
- **Vector**: HNSW and FAISS for similarity search (GPU-accelerated)
- **Document**: JSON storage with flexible schema
- **Time-Series**: Gorilla compression, continuous aggregates

```mermaid
graph LR
    subgraph "Unified Storage"
        LSM[RocksDB LSM-Tree]
    end
    
    subgraph "Data Models"
        REL[Relational Model<br/>Tables & Rows]
        GRAPH[Graph Model<br/>Nodes & Edges]
        VECTOR[Vector Model<br/>Embeddings]
        DOC[Document Model<br/>JSON Documents]
        TS[Time-Series<br/>Metrics & Events]
    end
    
    REL --> LSM
    GRAPH --> LSM
    VECTOR --> LSM
    DOC --> LSM
    TS --> LSM
    
    style LSM fill:#ffe1e1
    style REL fill:#e1ffe1
    style GRAPH fill:#e1ffe1
    style VECTOR fill:#e1ffe1
    style DOC fill:#e1ffe1
    style TS fill:#e1ffe1
```

### Transaction Support

```mermaid
sequenceDiagram
    participant Client
    participant TxManager as Transaction Manager
    participant MVCC as MVCC Engine
    participant Storage as RocksDB Storage
    
    Client->>TxManager: BEGIN TRANSACTION
    TxManager->>MVCC: Get Snapshot (timestamp)
    MVCC-->>TxManager: Snapshot ID
    TxManager-->>Client: Transaction Handle
    
    Client->>TxManager: READ (key)
    TxManager->>MVCC: Read at Snapshot
    MVCC->>Storage: Get versioned data
    Storage-->>MVCC: Data with version
    MVCC-->>TxManager: Consistent read
    TxManager-->>Client: Data
    
    Client->>TxManager: WRITE (key, value)
    TxManager->>MVCC: Check conflicts
    MVCC-->>TxManager: No conflicts
    TxManager->>Storage: Write with version
    Storage-->>TxManager: Written
    TxManager-->>Client: OK
    
    Client->>TxManager: COMMIT
    TxManager->>MVCC: Validate & commit
    MVCC->>Storage: Apply changes atomically
    Storage-->>MVCC: Success
    MVCC-->>TxManager: Committed
    TxManager-->>Client: Transaction Complete
```

- Full ACID guarantees with snapshot isolation
- Write-write conflict detection
- Atomic updates across all index types

### Security & Compliance

```mermaid
graph TB
    subgraph "Client Layer"
        CLIENT[Client Application]
    end
    
    subgraph "Transport Security"
        TLS[TLS 1.3<br/>Certificate Validation]
        MTLS[Mutual TLS<br/>Client Certificates]
    end
    
    subgraph "Authentication & Authorization"
        AUTH[Authentication<br/>JWT/OAuth2]
        RBAC[Role-Based Access Control<br/>Permissions Matrix]
        POLICY[Policy Engine<br/>Apache Ranger]
    end
    
    subgraph "Application Security"
        RATELIMIT[Rate Limiting<br/>DDoS Protection]
        AUDIT[Audit Logging<br/>SIEM Integration]
        INPUT[Input Validation<br/>SQL Injection Prevention]
    end
    
    subgraph "Data Security"
        ENCRYPT[Field-Level Encryption<br/>AES-256-GCM]
        HSM[Hardware Security Module<br/>Key Management]
        MASKING[Data Masking<br/>PII Protection]
    end
    
    subgraph "Storage Security"
        STORAGE[Encrypted Storage<br/>At-Rest Encryption]
        BACKUP[Encrypted Backups<br/>Secure Recovery]
    end
    
    CLIENT --> TLS
    TLS --> MTLS
    MTLS --> AUTH
    AUTH --> RBAC
    RBAC --> POLICY
    POLICY --> RATELIMIT
    RATELIMIT --> INPUT
    INPUT --> AUDIT
    AUDIT --> ENCRYPT
    ENCRYPT --> HSM
    HSM --> MASKING
    MASKING --> STORAGE
    STORAGE --> BACKUP
    
    style TLS fill:#ffe1e1
    style AUTH fill:#ffe1e1
    style ENCRYPT fill:#ffe1e1
    style STORAGE fill:#ffe1e1
```

- TLS 1.3 with mTLS support
- Role-Based Access Control (RBAC)
- Field-level encryption
- Audit logging with SIEM integration

### Distribution & Scaling

```mermaid
graph TB
    subgraph "Client Applications"
        APP[Applications]
    end
    
    subgraph "Routing Layer"
        SR[Shard Router<br/>VCC-URN Partitioning]
        SM[Shard Manager<br/>Metadata & Health]
        REBAL[Auto Rebalancer<br/>Load Distribution]
    end
    
    subgraph "ThemisDB Cluster - RAID Modes"
        subgraph "MIRROR Mode RF=2"
            subgraph "Shard 1"
                S1P[Primary Node]
                S1R[Replica Node]
            end
            
            subgraph "Shard 2"
                S2P[Primary Node]
                S2R[Replica Node]
            end
        end
        
        subgraph "PARITY Mode 4+2"
            S3[Data Shard 1]
            S4[Data Shard 2]
            S5[Data Shard 3]
            S6[Data Shard 4]
            P1[Parity Shard 1]
            P2[Parity Shard 2]
        end
    end
    
    subgraph "Observability"
        MON[Monitoring<br/>Metrics & Health]
    end
    
    APP --> SR
    SR --> SM
    SM --> REBAL
    
    SR --> S1P & S2P
    S1P -.Replication.-> S1R
    S2P -.Replication.-> S2R
    
    SR --> S3 & S4 & S5 & S6
    S3 & S4 & S5 & S6 -.Parity.-> P1 & P2
    
    SM --> MON
    REBAL -.Auto-Balance.-> S1P & S2P & S3 & S4
    
    style SR fill:#e1f5ff
    style S1P fill:#e1ffe1
    style S2P fill:#e1ffe1
    style S3 fill:#e1ffe1
    style S4 fill:#e1ffe1
    style S5 fill:#e1ffe1
    style S6 fill:#e1ffe1
    style P1 fill:#fff3cd
    style P2 fill:#fff3cd
```

- VCC-URN based sharding with consistent hashing (Enterprise)
- RAID-like redundancy modes: MIRROR, STRIPE, PARITY, GEO_MIRROR (Enterprise)
- Auto-rebalancing with zero-downtime migration (Enterprise)
- Multi-region deployment support (Enterprise)

**[→ View All Features](docs/de/features/features_overview.md)**

---

## Editions

| Edition | License | Features | Use Case |
|---------|---------|----------|----------|
| 🔹 **Minimal** | Open Source (MIT) | Core database only | Embedded systems, IoT, edge devices |
| 🆓 **Community** | Open Source (MIT) | Full-featured single-node | Development, startups, single-server |
| 🔒 **Enterprise** | Commercial | + Horizontal scaling, HA, replication | Large-scale production deployments |

**[→ Minimal Edition Details](docs/MINIMAL_EDITION.md)** | **[→ Enterprise Edition Details](docs/reports/ENTERPRISE.md)**

---

## Documentation

**Getting Started:**
- 🚀 [Quick Start](#quick-start) - Get up and running in 5 minutes
- 🐳 [Docker Deployment](docs/de/deployment/DOCKER_DEPLOYMENT.md) - Container-based deployment
- 🔧 [Building from Source](docs/de/guides/guides_build_strategy.md) - Compile from source code

**Core Concepts:**
- 🏗️ [Architecture Overview](docs/de/architecture/ARCHITECTURE_OVERVIEW.md) - System design and components
- 💾 [Multi-Model Design](docs/de/architecture/architecture_base_entity.md) - Unified storage architecture
- 🔄 [Transaction Management](docs/de/features/features_transactions.md) - ACID and MVCC details
- 🔍 [AQL Query Language](docs/de/aql/aql_syntax.md) - Advanced Query Language syntax
- 🔀 [Git/GitOps Research](docs/research/git_gitops_themis_vergleich.md) - Version control concepts comparison

**Features:**
- 🎯 [Vector Search](docs/de/features/features_vector_ops.md) - Similarity search and embeddings
- 🕸️ Graph Operations - Graph traversals and algorithms
- 📈 [Time-Series Engine](docs/de/features/features_time_series.md) - Time-series data handling
- 🔐 [Security & Compliance](docs/de/security/security_implementation.md) - Security features

**Operations:**
- ⚙️ [Configuration Guide](docs/en/guides/guides_configuration.md) - Server configuration
- 📊 [Monitoring & Metrics](docs/de/observability/observability_prometheus.md) - Prometheus and Grafana
- 💾 [Backup & Recovery](docs/de/guides/guides_deployment.md#backup--recovery) - Data protection
- ⚡ [Performance Tuning](docs/de/performance/performance_memory.md) - Optimization tips

**Development:**
- 🤝 [Contributing](CONTRIBUTING.md) - How to contribute
- 🌿 [Branching Strategy](docs/BRANCHING_STRATEGY.md) - Git Flow workflow
- 📖 [API Reference](docs/api/API_REFERENCE.md) - REST and GraphQL APIs
- 📦 [Client SDKs](clients/README.md) - Available client libraries

**LLM/LoRA System:**
- ✅ [**LLM Core Status (Master)**](docs/LLM_CORE_STATUS_MASTER.md) - **Single source of truth** for implementation status
- 📊 [Comprehensive Audit Report](docs/LLM_CORE_AUDIT_REPORT.md) - Detailed code audit findings
- 🔍 [Decision Matrix](docs/LLM_CORE_DECISION_MATRIX.md) - Resolution of conflicting documentation
- 📋 [Progress Checklist](docs/LLM_LORA_CHECKLIST.md) - Detailed task tracking
- 📚 [Archived Docs](docs/ARCHIVED/README.md) - Historical documentation (superseded)
- ✅ **Status**: Core 100% production-ready, Integration 95% complete

> **📚 Full Documentation:** [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)

---

## Performance

> **Test Environment:** Release build, Windows x64, 20 cores @ 3696 MHz

| Operation | Throughput | Latency (avg) |
|-----------|:----------:|:-------------:|
| 📝 Entity PUT | 45,000 ops/s | 0.02 ms |
| 📖 Entity GET | 120,000 ops/s | 0.008 ms |
| 🔍 Indexed Query | 3.4M queries/s | 0.29 μs |
| 🕸️ Graph Traverse | 9.56M ops/s | 0.105 μs |
| 🎯 Vector Search | 59.7M queries/s | 0.017 μs |
| 📊 Vector Insert (384D) | 411k vectors/s | 2.44 μs |

> **Note:** Benchmarks represent optimal conditions. Actual performance varies based on hardware, data size, and workload.

### CHIMERA Suite - Scientific Benchmark Framework

ThemisDB performance is evaluated using the **CHIMERA Suite** (_Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_) - an industry-leading, vendor-neutral benchmark framework for multi-model databases with AI integration.

**Key Features:**
- 🔬 IEEE/ACM compliant scientific methodology
- 🎯 Multi-model workload testing (Graph, Vector, Relational, Document)
- 🤖 Native AI/LLM benchmark support (inference, LoRA, RAG)
- 🌐 Vendor-neutral, color-blind friendly reporting
- 📊 Statistical rigor with confidence intervals

**📊 [CHIMERA Suite Documentation](benchmarks/chimera/README.md)** | **[Complete Benchmark Results](benchmarks/BENCHMARK_DETAILED_RESULTS.md)**

---

## Community & Support

| Resource | Description | Link |
|----------|-------------|------|
| 📚 **Documentation** | Complete guides and API reference | [Docs Site](https://makr-code.github.io/ThemisDB/) |
| 🚀 **Production Ops** | Deployment, monitoring, troubleshooting | [Operations Guide](docs/OPERATIONS.md) |
| 🐛 **Issues** | Report bugs or request features | [GitHub Issues](https://github.com/makr-code/ThemisDB/issues) |
| 💬 **Discussions** | Community Q&A and discussions | [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions) |
| 🤝 **Contributing** | How to contribute to ThemisDB | [Contributing Guide](CONTRIBUTING.md) |
| 🔒 **Security** | Responsible disclosure policy | [Security Policy](SECURITY.md) |

---

## License

**Community Edition:** Released under the [MIT License](LICENSE) - Free to use, modify, and distribute.

**Enterprise Edition:** Available under commercial license with additional features (horizontal sharding, advanced analytics, HA/replication).

**Enterprise Inquiries:** sales@themisdb.com

---

## Acknowledgments

ThemisDB builds upon excellent open-source projects:

- **RocksDB** - High-performance LSM-Tree storage engine
- **FAISS** - Efficient similarity search library
- **llama.cpp** - LLM inference engine (optional)
- **ArangoDB** - Multi-model architecture inspiration
- **CozoDB** - Hybrid relational-graph-vector design inspiration

**[→ Complete Attribution & Dependencies](docs/de/legal/ATTRIBUTIONS.md)**  
**[→ Implementation Origins & Code Attribution](IMPLEMENTATION_ORIGINS.md)**

---

## Contributing & Community

We welcome contributions! Please see our:
- 🤝 [Contributing Guide](CONTRIBUTING.md) - Development workflow and guidelines
- 📋 [Code of Conduct](CODE_OF_CONDUCT.md) - Community standards
- 💬 [Support](SUPPORT.md) - How to get help
- 🔒 [Security Policy](SECURITY.md) - Reporting security issues

---

<div align="center">
  
**Built with ❤️ for the database community**

[⭐ Star us on GitHub](https://github.com/makr-code/ThemisDB) · [📖 Read the Docs](https://makr-code.github.io/ThemisDB/) · [🤝 Contribute](CONTRIBUTING.md)

</div>
