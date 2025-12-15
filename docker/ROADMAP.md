# ThemisDB Roadmap (Docker Hub)

**Current Version:** v1.0.2  
**Last Updated:** December 14, 2025

---

## Vision

ThemisDB is evolving into a **high-performance, multi-model database platform** with native GPU acceleration, distributed architecture, and advanced analytics capabilities.

Our goal: **Best-in-class performance** while maintaining **simplicity for developers** and **enterprise-grade reliability**.

---

## Completed (v1.0.x - 2025)

### ✅ Foundation (v1.0.0 - November 2025)

**Core Database:**
- LSM Tree storage engine (RocksDB)
- Multi-model support (Key-Value, Document, Vector, Graph, Geospatial, Full-Text)
- ACID transactions with MVCC
- Query optimizer

**Distribution:**
- Horizontal sharding (auto-rebalancing)
- Multi-master replication
- RAID-like redundancy (MIRROR, STRIPE, PARITY, GEO)
- Gossip protocol for cluster membership

**Performance:**
- GPU acceleration (10 backends: CUDA, Vulkan, ROCm, DirectX, OpenCL, HIP, OneAPI, ZLUDA, FAISS, TBB)
- SIMD optimizations (AVX2/NEON)
- Parallel query execution

**Analytics:**
- OLAP engine (CUBE, ROLLUP, window functions)
- Complex Event Processing (CEP)
- Process mining

**Client SDKs:**
- 7 languages: Python, JavaScript, Rust, Go, Java, C#, Swift
- Feature parity across all SDKs

### ✅ Docker & Release (v1.0.1 - December 2025)

**Docker Hub:**
- Multi-architecture images (amd64, arm64)
- Automated CI/CD pipeline
- ~150MB compressed image size
- Health checks & monitoring

**Release Management:**
- SLSA Level 1 compliance
- SBOM (Software Bill of Materials)
- Automated GitHub releases
- Enterprise-grade packaging

### ✅ Build Stability (v1.0.2 - December 2025)

**Windows Support:**
- Fixed MSVC RocksDB linking issues
- Stable Windows Release builds

**QNAP Support:**
- QNAP-optimized Docker image
- Ubuntu 20.04 base (GLIBC 2.31)
- SSE4.2 CPU baseline compatibility

---

## In Progress

### 🔧 v1.1.0 - Optimization Release (Q1 2026)

**Focus:** Maximize existing library capabilities before adding new dependencies

**Philosophy:** "Optimize what we have, add only what we need"

#### Core Optimizations (9-11 weeks)

**RocksDB Advanced Features** (3 weeks)
- ✅ TTL (Time-To-Live) for automatic data retention
- ✅ Incremental backups (space-efficient, GDPR-compliant)
- ✅ Statistics export to OpenTelemetry

**TBB Parallel Algorithms** (3 weeks)
- ✅ Parallel sort (2-4x speedup on large result sets)
- ✅ Concurrent containers (lock-free caches)
- ✅ Flow graphs for complex pipelines

**Arrow Data Lake Integration** (2 weeks)
- ✅ Parquet export for data warehouses
- ✅ Compute kernels for SIMD aggregations

**CUDA as Core Component** (1 week)
- ✅ Move CUDA from enterprise to core
- ✅ Adaptive GPU usage (fallback to CPU)
- ✅ Optimized memory transfers

**vLLM Co-Location** (1 week)
- ✅ Resource coordination with vLLM
- ✅ Shared GPU/RAM management
- ✅ Optimized RAG workflows

**Memory Performance** (1 day)
- ✅ mimalloc integration
- ✅ 20-40% memory allocation speedup
- ✅ Drop-in replacement for system allocator

#### Expected Impact

- **3-10x performance** on existing features
- **+6% dependencies** (16 instead of 15 libraries)
- **Zero breaking changes** (backwards compatible)

#### New Variants

- **OLTP Variant:** Optimized for transactions
- **OLAP Variant:** Analytics workloads
- **vLLM Variant:** AI/ML co-location

---

## Planned

### 🚀 v1.2.0 - Enterprise Features (Q2-Q3 2026)

**vLLM AI Support** (8-12 weeks)
- LoRA adapter management
- Advanced FAISS integration
- Hybrid search (vector + keyword)
- Embedding cache

**Geo-Spatial PostGIS Compatibility** (6-9 weeks)
- GEOS library integration
- PROJ coordinate transformations
- cuSpatial GPU acceleration
- PostGIS function compatibility

**IoT/TimescaleDB Features** (5-7 weeks)
- Hypertables for time-series
- Arrow compute aggregates
- Parquet archiving
- Continuous aggregates

#### Dependencies

- **+18% libraries** (19 instead of 16)
- New: vLLM, GEOS, PROJ
- Zero removal of existing features

### 🌟 v1.3.0+ - Enterprise Scale (Q4 2026+)

**Production Hardening:**
- Multi-datacenter deployments
- Kubernetes operator with controller
- Advanced monitoring & alerting
- Production-grade security audits

**Advanced Features:**
- Graph Neural Networks (GNN)
- Multi-vLLM load balancing
- Advanced query optimizer v2
- DuckDB OLAP integration (optional)

**Security & Compliance:**
- RE2 regex engine (no ReDoS)
- Advanced encryption (homomorphic, ZKP)
- Penetration testing Phase 2
- Additional certifications (SOC 2, ISO 27001)

---

## Docker Image Roadmap

### Current Images

| Tag | Architecture | Status |
|-----|--------------|--------|
| `latest` | amd64, arm64 | ✅ Active |
| `v1.0.2` | amd64, arm64 | ✅ Stable |
| `qnap` | amd64 | ✅ Active |

### Planned Images (v1.1.0)

| Tag | Purpose | Size Target |
|-----|---------|-------------|
| `v1.1.0-oltp` | Transaction-optimized | ~160MB |
| `v1.1.0-olap` | Analytics-optimized | ~170MB |
| `v1.1.0-vllm` | AI/ML co-location | ~180MB |
| `v1.1.0-full` | All features | ~200MB |

### Future Images (v1.2.0+)

- Minimal variant (~100MB, core features only)
- GPU-optimized (CUDA runtime included)
- Enterprise variant (all compliance features)

---

## Performance Targets

### v1.0.2 (Current)

**Baseline:**
- Transaction throughput: 10K TPS
- Vector search: 1M vectors in <100ms (L2 distance)
- Graph traversal: 10K nodes/sec
- Full-text search: 1M docs in <50ms

### v1.1.0 (Q1 2026)

**Goals:**
- Transaction throughput: 30-50K TPS (+3-5x)
- Vector search: <30ms on 1M vectors (+3x)
- Graph traversal: 50K nodes/sec (+5x)
- Full-text search: <20ms (+2.5x)
- Memory overhead: -30% (mimalloc)

### v1.2.0 (Q2-Q3 2026)

**Goals:**
- Transaction throughput: 100K TPS (+10x from v1.0)
- Vector search: <10ms with GPU (+10x)
- Hybrid search: Vector + keyword in single query
- Geo-spatial: PostGIS-compatible performance

---

## Community & Contributions

### SDK Publishing (v1.1.0)

**Python:**
- PyPI package
- Conda-forge distribution
- Type stubs included

**JavaScript:**
- npm package
- TypeScript definitions
- React/Vue integrations

**Go:**
- Go module
- pkg.go.dev documentation

**Rust:**
- crates.io package
- Comprehensive docs.rs

**Java/C#/Swift:**
- Maven Central / NuGet / Swift Package Manager

### Documentation (Ongoing)

- Interactive tutorials
- Video walkthroughs
- Architecture deep-dives
- Migration guides from other databases

---

## Deployment Targets

### Current Support

✅ Docker / Docker Compose  
✅ Kubernetes (manifests)  
✅ QNAP Container Station  
✅ Bare metal (Linux, Windows, macOS)  
✅ Raspberry Pi / ARM devices

### Planned (2026)

🔧 Kubernetes Operator (v1.1.0)  
🔧 Helm charts (v1.1.0)  
🚀 AWS ECS / Fargate (v1.2.0)  
🚀 Google Cloud Run (v1.2.0)  
🚀 Azure Container Instances (v1.2.0)

---

## Timeline

```
2025 Q4          2026 Q1          2026 Q2-Q3       2026 Q4+
───────────────────────────────────────────────────────────
v1.0.0-1.0.2     v1.1.0           v1.2.0           v1.3.0+
✅ Foundation    🔧 Optimization  🚀 Enterprise    🌟 Scale
                                  Features
```

**Milestones:**
- ✅ v1.0.0 - November 30, 2025
- ✅ v1.0.1 - December 11, 2025 (Docker Hub)
- ✅ v1.0.2 - December 14, 2025 (Stability)
- 🔧 v1.1.0 - March 2026 (Optimization)
- 🚀 v1.2.0 - June-September 2026 (Enterprise)
- 🌟 v1.3.0 - December 2026+ (Scale)

---

## Feedback & Requests

We're listening! Help shape the future of ThemisDB:

**GitHub Discussions:** https://github.com/makr-code/ThemisDB/discussions  
**Feature Requests:** https://github.com/makr-code/ThemisDB/issues/new?template=feature_request.md  
**Bug Reports:** https://github.com/makr-code/ThemisDB/issues/new?template=bug_report.md

**Community Priorities:**
- Performance benchmarks vs PostgreSQL/MySQL
- Cloud-native deployment patterns
- Integration with popular frameworks
- Real-world use case examples

---

## Stay Updated

**Changelog:** [CHANGELOG.md](https://github.com/makr-code/ThemisDB/blob/main/CHANGELOG.md)  
**Releases:** [GitHub Releases](https://github.com/makr-code/ThemisDB/releases)  
**Docker Hub:** [themisdb/themisdb](https://hub.docker.com/r/themisdb/themisdb)  
**Documentation:** [Full Docs](https://github.com/makr-code/ThemisDB)

---

**Last Updated:** December 14, 2025  
**Next Review:** March 2026 (v1.1.0 release)
