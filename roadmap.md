# ThemisDB Project Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

**Version:** 2.0  
**Last Updated:** 2026-02-27  
**Scope:** Aggregated roadmap across all 44 modules in `src/`

> For module-specific details see each module's `src/<module>/ROADMAP.md`.

---

## Overview

ThemisDB is a high-performance multi-model database with native AI/LLM integration. This top-level roadmap aggregates the status and planned work across all 44 source modules. The project follows a phased approach: stabilise core infrastructure first, then harden distributed and AI layers, and finally deliver operational excellence at hyperscale.

**Overall Timeline:** Q1 2026 – Q4 2027  
**Current Release:** v1.5.0-dev

---

## Module Status Summary

| Module | Status | Individual Roadmap |
|--------|--------|--------------------|
| **acceleration** | 🚧 Pre-production hardening | [src/acceleration/ROADMAP.md](src/acceleration/ROADMAP.md) |
| **analytics** | ✅ Production-ready | [src/analytics/ROADMAP.md](src/analytics/ROADMAP.md) |
| **api** | 🟡 Beta — Core HTTP complete, gRPC/OpenAPI in progress | [src/api/ROADMAP.md](src/api/ROADMAP.md) |
| **aql** | ✅ Production-ready | [src/aql/ROADMAP.md](src/aql/ROADMAP.md) |
| **auth** | ✅ Production-ready | [src/auth/ROADMAP.md](src/auth/ROADMAP.md) |
| **base** | ✅ Production-ready | [src/base/ROADMAP.md](src/base/ROADMAP.md) |
| **cache** | ✅ Production-ready | [src/cache/ROADMAP.md](src/cache/ROADMAP.md) |
| **cdc** | ✅ Production-ready | [src/cdc/ROADMAP.md](src/cdc/ROADMAP.md) |
| **chimera** | 🔴 Alpha — ThemisDB adapter functional; vendor adapters planned | [src/chimera/ROADMAP.md](src/chimera/ROADMAP.md) |
| **config** | ✅ Production-ready | [src/config/ROADMAP.md](src/config/ROADMAP.md) |
| **content** | 🟡 Beta — MIME/text extraction complete, OCR/audio planned | [src/content/ROADMAP.md](src/content/ROADMAP.md) |
| **core** | 🟡 Beta — DI framework and adapters functional | [src/core/ROADMAP.md](src/core/ROADMAP.md) |
| **exporters** | ✅ Production-ready | [src/exporters/ROADMAP.md](src/exporters/ROADMAP.md) |
| **geo** | 🟡 Beta — CPU queries tested; GPU backend with CPU fallback | [src/geo/ROADMAP.md](src/geo/ROADMAP.md) |
| **governance** | 🟡 Beta — GDPR/HIPAA rules functional; OPA planned | [src/governance/ROADMAP.md](src/governance/ROADMAP.md) |
| **gpu** | 🟡 Beta — Device management complete; multi-node coordination in progress | [src/gpu/ROADMAP.md](src/gpu/ROADMAP.md) |
| **graph** | 🟡 Beta — Cost-based optimiser, constrained path finding, parallel traversal, distributed shards, GPU fallback functional; EXPLAIN AQL output planned | [src/graph/ROADMAP.md](src/graph/ROADMAP.md) |
| **importers** | 🟡 Beta — PostgreSQL import functional; additional sources planned | [src/importers/ROADMAP.md](src/importers/ROADMAP.md) |
| **index** | ✅ Production-ready | [src/index/ROADMAP.md](src/index/ROADMAP.md) |
| **ingestion** | ✅ Production-ready | [src/ingestion/ROADMAP.md](src/ingestion/ROADMAP.md) |
| **llm** | ✅ Production-ready (v1.16.0) | [src/llm/ROADMAP.md](src/llm/ROADMAP.md) |
| **metadata** | ✅ Production-ready | [src/metadata/ROADMAP.md](src/metadata/ROADMAP.md) |
| **network** | ✅ Production-ready | [src/network/ROADMAP.md](src/network/ROADMAP.md) |
| **observability** | ✅ Production-ready | [src/observability/ROADMAP.md](src/observability/ROADMAP.md) |
| **performance** | ✅ Production-ready | [src/performance/ROADMAP.md](src/performance/ROADMAP.md) |
| **plugins** | ✅ Production-ready | [src/plugins/ROADMAP.md](src/plugins/ROADMAP.md) |
| **prompt_engineering** | ✅ Production-ready (v1.x) | [src/prompt_engineering/ROADMAP.md](src/prompt_engineering/ROADMAP.md) |
| **query** | ✅ Production-ready | [src/query/ROADMAP.md](src/query/ROADMAP.md) |
| **rag** | ✅ Production-ready | [src/rag/ROADMAP.md](src/rag/ROADMAP.md) |
| **replication** | ✅ Production-ready | [src/replication/ROADMAP.md](src/replication/ROADMAP.md) |
| **scheduler** | ✅ Production-ready (v1.5.0) | [src/scheduler/ROADMAP.md](src/scheduler/ROADMAP.md) |
| **search** | ✅ Production-ready (v1.2.0+) | [src/search/ROADMAP.md](src/search/ROADMAP.md) |
| **security** | ✅ Production-ready | [src/security/ROADMAP.md](src/security/ROADMAP.md) |
| **server** | ✅ Production-ready | [src/server/ROADMAP.md](src/server/ROADMAP.md) |
| **sharding** | 🟡 Beta — Consensus and repair engine complete; advanced observability in progress | [src/sharding/ROADMAP.md](src/sharding/ROADMAP.md) |
| **storage** | 🟡 Beta — RocksDB MVCC complete; production hardening in progress | [src/storage/ROADMAP.md](src/storage/ROADMAP.md) |
| **temporal** | 🟡 Beta — Core conflict resolution complete; production hardening in progress | [src/temporal/ROADMAP.md](src/temporal/ROADMAP.md) |
| **themis** | 🟡 Beta — Module loading and edition management complete; v1.7.0 migration planned | [src/themis/ROADMAP.md](src/themis/ROADMAP.md) |
| **timeseries** | ✅ Production-ready | [src/timeseries/ROADMAP.md](src/timeseries/ROADMAP.md) |
| **training** | ✅ Production-ready (v1.x) | [src/training/ROADMAP.md](src/training/ROADMAP.md) |
| **transaction** | ✅ Production-ready | [src/transaction/ROADMAP.md](src/transaction/ROADMAP.md) |
| **updates** | ✅ Production-ready | [src/updates/ROADMAP.md](src/updates/ROADMAP.md) |
| **utils** | ✅ Production-ready | [src/utils/ROADMAP.md](src/utils/ROADMAP.md) |
| **voice** | ✅ Production-ready | [src/voice/ROADMAP.md](src/voice/ROADMAP.md) |

**Legend:** ✅ Production-ready · 🟡 Beta · 🔴 Alpha · 🚧 In active hardening

---

## Milestone: v1.5.0

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.5.0.md`](docs/de/releases/RELEASE_NOTES_v1.5.0.md)

Key PRs included in v1.5.0:

| PR | Module | Feature |
|----|--------|---------|
| [#3049](https://github.com/makr-code/ThemisDB/pull/3049) | geo | Geo CPU/GPU throughput benchmarks |
| [#3050](https://github.com/makr-code/ThemisDB/pull/3050) | security | QueryMaskingPolicy (PII field masking) |
| [#3051](https://github.com/makr-code/ThemisDB/pull/3051) | gpu | WASMKernelSandbox (GPU kernel isolation) |
| [#1383](https://github.com/makr-code/ThemisDB/issues/1383) | acceleration | CUDA ANN + geospatial kernels |
| [#1384](https://github.com/makr-code/ThemisDB/issues/1384) | acceleration | Vulkan compute shader pipeline |
| [#1390](https://github.com/makr-code/ThemisDB/issues/1390) | acceleration | Cross-backend L2 distance validation |
| [#3420](https://github.com/makr-code/ThemisDB/pull/3420) | updates | Update history log |
| [#3421](https://github.com/makr-code/ThemisDB/pull/3421) | updates | Blue/green deployment support |
| [#3422](https://github.com/makr-code/ThemisDB/pull/3422) | replication/updates | CoordinatedUpdateManager |
| [#3424](https://github.com/makr-code/ThemisDB/pull/3424) | chimera | CI benchmark baseline |
| [#3425](https://github.com/makr-code/ThemisDB/pull/3425) | gpu | Multi-node GPU coordination production-ready |
| [#3426](https://github.com/makr-code/ThemisDB/pull/3426) | performance | Memory pressure monitor (Phase 3) |
| [#3427](https://github.com/makr-code/ThemisDB/pull/3427) | query | Per-query resource limits |
| [#3428](https://github.com/makr-code/ThemisDB/pull/3428) | replication | CRDT FLAG_EW + FLAG_DW types |
| [#3434](https://github.com/makr-code/ThemisDB/pull/3434) | voice | Real-time meeting transcription |
| [#3435](https://github.com/makr-code/ThemisDB/pull/3435) | performance | PMU cache-miss analysis |
| [#3437](https://github.com/makr-code/ThemisDB/pull/3437) | performance/ci | Cross-module performance regression CI |
| [#3438](https://github.com/makr-code/ThemisDB/pull/3438) | security/updates | HSM-backed SigningService |
| [#3442](https://github.com/makr-code/ThemisDB/pull/3442) | voice | STT/TTS benchmarks |
| [#3444](https://github.com/makr-code/ThemisDB/pull/3444) | voice | Language detection + auto-locale |
| [#3445–#3450](https://github.com/makr-code/ThemisDB/pull/3450) | rpc | Full RPC production implementation |
| [#3453–#3462](https://github.com/makr-code/ThemisDB/pull/3462) | security | PKCS#11 HSM + RFC 3161 TSA full stack |
| [#3463](https://github.com/makr-code/ThemisDB/pull/3463) | security/observability | Audit log fsync + rotation + mirror |
| [#3464](https://github.com/makr-code/ThemisDB/pull/3464) | sharding | Hardware migration / NodeIdentity persistence |

---

## Milestone: v1.7.0

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.7.0.md`](docs/de/releases/RELEASE_NOTES_v1.7.0.md)
> **Issues:** [#3486](https://github.com/makr-code/ThemisDB/issues/3486) · [#3073](https://github.com/makr-code/ThemisDB/issues/3073)

Key PRs and features included in v1.7.0:

| PR / Feature | Module | Purpose |
|---|--------|---------|
| Config Architecture Reorganization | config | Hierarchical `config/` directories + `ConfigPathResolver` backward-compat layer |
| Multi-GPU Vector Indexing API (v2.4) | gpu / index | `MultiGPUVectorIndex` scaffolding: partition strategies, fan-out/merge, CPU-backed |
| Git-Like Features Integration | storage / server | SnapshotManager, PITR REST API, MergeEngine 3-way merge |
| HybridSearch production hardening | search | Configurable metric, strict validation, `SearchStats`, exception safety |
| Distributed Query Optimizer | query | Dynamic shard row estimates, predicate selectivity, latency hooks |
| FAISS ADC distance tables | index | ~40% faster `IndexIVFPQ` search |
| [#3471](https://github.com/makr-code/ThemisDB/pull/3471) | tests / benchmarks | Coverage audit: 6 benchmarks + 21 unit test files |
| [#3472–#3484](https://github.com/makr-code/ThemisDB/pull/3484) | docs (all modules) | Full 44-module documentation audit and sync |
| [#3480](https://github.com/makr-code/ThemisDB/pull/3480) | ci | Documentation validation CI workflow |
| [#3485](https://github.com/makr-code/ThemisDB/pull/3485) | rag / research | RAG scientific foundations (40 IEEE citations) |

**Breaking change:** `themis` module initialisation code migrated from `src/utils/` / `src/base/` to `src/themis/`.

---

## Implementation Phases

### Phase 1: Foundation Hardening (Q1–Q2 2026) — 🚧 In Progress

Focus: Bring all remaining Beta/Alpha modules to production grade. Eliminate known gaps in
cross-backend consistency, error handling, and resource management.

#### 1.1 Acceleration Module — CUDA/Vulkan Kernel Completion
- [P] CUDA ANN + geospatial kernels production-ready (Issue: #1383) (Target: Q2 2026)
- [P] Vulkan compute shader pipeline (Issue: #1384) (Target: Q2 2026)
- [P] Cross-backend L2 distance consistency validation (Issue: #1390) (Target: Q2 2026)
- [I] Runtime device detection and capability negotiation (Issue: #1374) (Target: Q2 2026)

#### 1.2 API — OpenAPI & gRPC Surface
- [I] OpenAPI 3.x spec completeness for all endpoints (Issue: #1491) (Target: Q2 2026)
- [I] Versioned endpoint routing `/v1/`, `/v2/` with deprecation headers (Issue: #1506) (Target: Q3 2026)
- [x] SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue: #1507) (Target: Q3 2026)

#### 1.3 CDC — WebSocket & Streaming Transport
- [I] WebSocket transport for changefeed subscriptions (Target: Q2 2026)
- [I] Kafka/Kinesis integration for event streaming (Target: Q3 2026)

#### 1.4 Chimera — Vendor Adapter Implementations
- [I] PostgreSQL adapter (Issue: alpha) (Target: Q3 2026)
- [I] MongoDB adapter (Target: Q3 2026)
- [x] Weaviate adapter (Target: Q4 2026)

#### 1.5 Content — Binary Format Support
- [I] PDF text extraction (Target: Q2 2026)
- [I] OCR integration for image-embedded text (Target: Q3 2026)
- [I] Audio transcription pipeline (Target: Q3 2026)

#### 1.6 Core — Production DI Hardening
- [I] Full OpenTelemetry adapter coverage (Target: Q2 2026)
- [I] Production readiness checklist completion (Target: Q2 2026)

#### 1.7 Geo — GPU Kernel Completion
- [P] Geo CPU/GPU throughput benchmarks (`bench_geo_cpu_gpu.cpp`) (PR: #3049) (Target: v1.5.0) ✅
- [I] ST_BUFFER/ST_UNION/ST_DIFFERENCE CUDA kernels (Target: Q2 2026)
- [I] Full PostGIS ST_* function parity (Target: Q3 2026)

#### 1.8 Ingestion — Distributed & Cloud Sources
- [x] Kafka consumer source connector (Issue: #1892) (Target: Q3 2026)
- [I] S3/GCS/Azure Blob object-storage source (Issue: #1893) (Target: Q3 2026)
- [!] OAuth 2.0 token refresh within connectors (Issue: #2408) (Target: Q3 2026)

#### 1.9 Sharding — Observability & Repair
- [x] Advanced metrics and distributed tracing (`sharding/operational_metrics.cpp`, `observability/distributed_flame_graph.cpp`, `observability/ebpf_tracer.cpp`)
- [I] Automated shard rebalancing (Target: Q3 2026)

#### 1.10 Storage — Production Hardening
- [x] Benchmark-driven performance optimisation (`tests/test_storage_latency_bench.cpp`)
- [x] Backup/PITR integration tests (`tests/test_backup_restore_integration.cpp`)

---

### Phase 2: AI/LLM Ecosystem Expansion (Q2–Q3 2026) — 📋 Planned

Focus: Deepen AI capabilities across prompt engineering, training, RAG, and analytics.

#### 2.1 Prompt Engineering
- [?] Token counting and context-window budget enforcement (Target: Q2 2026)
- [?] Typed template DSL with compile-time placeholder validation (Target: Q2 2026)
- [?] Batch A/B test runner with configurable traffic splits (Target: Q3 2026)
- [?] RLHF integration for prompt quality improvement (Target: Q4 2026)

#### 2.2 Training
- [?] Multi-GPU distributed training coordination (Target: Q2 2026)
- [?] Automated hyperparameter search (LoRA rank, learning rate sweep) (Target: Q2 2026)
- [?] Adapter serving integration with LLM inference layer (Target: Q3 2026)
- [?] Active learning loop for most-informative sample selection (Target: Q3 2026)
- [?] Domain adaptation beyond legal (medical, financial) (Target: Q4 2026)

#### 2.3 RAG — Advanced Retrieval
- [I] Adaptive retrieval depth based on query complexity (Target: Q2 2026)
- [I] Multi-hop reasoning with intermediate knowledge graph traversal (Target: Q3 2026)
- [I] Retrieval confidence calibration and hallucination detection improvements (Target: Q3 2026)

#### 2.4 AQL — Extended Language Features
- [I] Streaming NL responses for long AQL explanations (Issue: #2012) (Target: Q2 2026)
- [I] AQL query validation and linting before LLM submission (Issue: #1525) (Target: Q2 2026)
- [x] Few-shot example library for improved NL-to-AQL accuracy (Issue: #1521) (Target: Q3 2026)

#### 2.5 Analytics — GPU-Accelerated OLAP
- [P] GPU-accelerated OLAP aggregations via CUDA (Issue: #1469) (Target: Q3 2026)
- [I] Zero-copy Arrow data transfer optimisations (Issue: #1471) (Target: Q3 2026)
- [I] Arrow Flight RPC support for remote analytics (Issue: #1472) (Target: Q3 2026)
- [x] Predictive analytics and time-series forecasting (Issue: #1473)

---

### Phase 3: Distributed Systems Maturity (Q3–Q4 2026) — 📋 Planned

Focus: Hyperscale distributed operations, multi-region support, and advanced consensus.

#### 3.1 Replication — Multi-Region
- [I] Geographic replica placement policies (Target: Q3 2026)
- [I] Asynchronous cross-region WAL shipping with configurable lag limits (Target: Q4 2026)

#### 3.2 Sharding — Global Distribution
- [I] Automatic shard rebalancing on cluster topology changes (Target: Q3 2026)
- [I] Cross-datacenter shard placement and latency-aware routing (Target: Q4 2026)
- [I] Global secondary indexes across shards (Target: Q4 2026)

#### 3.3 Graph — Distributed Traversal
- [I] Cross-shard graph query execution (Target: Q3 2026)
- [I] Distributed Betweenness Centrality (Target: Q4 2026)

#### 3.4 Storage — Tiered & Cloud-Native
- [I] Tiered storage: hot/warm/cold with automatic data migration (Target: Q3 2026)
- [I] Cloud-native blob backend improvements (S3/GCS/Azure) (Target: Q4 2026)

#### 3.5 Network — Protocol Hardening
- [I] HTTP/3 QUIC production enablement (Target: Q3 2026)
- [I] Zero-copy socket I/O for high-throughput workloads (Target: Q4 2026)

---

### Phase 4: Observability & Operational Excellence (Q4 2026) — 📋 Planned

Focus: Enterprise-grade monitoring, alerting, and automated operations.

#### 4.1 Observability — Extended Tracing
- [I] End-to-end distributed trace correlation across all 44 modules (Target: Q4 2026)
- [I] Anomaly-driven alerting with root cause analysis hints (Target: Q4 2026)
- [I] Continuous profiling integration (eBPF / perf) (Target: Q4 2026)

#### 4.2 Scheduler — Intelligent Retention
- [I] ML-based retention policy recommendations (Target: Q4 2026)
- [I] Cost-aware task prioritisation (Target: Q4 2026)

#### 4.3 Updates — Advanced Migration
- [x] Schema migration dry-run with impact analysis report (Target: Q4 2026) — `validateMigration` regression tests added (PR: #3433)
- [x] Blue-green deployment support for zero-downtime major upgrades (PR: #3421) ✅

#### 4.4 Config — Full Migration Tooling
- [I] Automated legacy config migration script with dry-run mode (Issue: #1661) (Target: Q4 2026)
- [I] Integration with JSON Schema / YAML schema validation (Issue: #1666) (Target: Q4 2026)

---

### Phase 5: Security Hardening & Compliance (Q1 2027) — 📋 Planned

Focus: Zero-trust, advanced compliance, and penetration-tested security posture.

#### 5.1 Security
- [P] `QueryMaskingPolicy` — dynamic PII field masking of query results (PR: #3050) (Target: v1.5.0) ✅
- [I] Zero-trust continuous verification framework (Issue: #1541) (Target: Q1 2027)
- [x] HSM integration for production key management (PKCS#11 real provider in `src/security/hsm_provider_pkcs11.cpp`, stub fail-fast guards in `src/security/hsm_provider.cpp`, security metrics and checker in `include/security/`, deployment docs in `docs/security/HSM_PRODUCTION_SETUP.md`; build with `-DTHEMIS_ENABLE_HSM_REAL=ON`; Phase 2 complete; acceptance criteria: PKCS#11 signing/key-management tests passing, no stub code path in ENTERPRISE/HYPERSCALER production builds, CI enforced)
- [I] Automated SOC 2 Type II evidence collection (Target: Q1 2027)

#### 5.2 Auth — Advanced Protocols
- [P] Fine-grained ABAC with OPA policy expressions (Issue: #1538) (Target: Q1 2027)
- [I] Certificate-based mTLS authentication (Issue: #2370) (Target: Q1 2027)
- [I] SAML 2.0 SP/IdP-initiated SSO completion (Target: Q1 2027)

#### 5.3 Governance
- [I] OPA (Open Policy Agent) integration (Target: Q1 2027)
- [I] Automated CCPA/CPRA data subject rights fulfilment (Target: Q1 2027)

#### 5.4 Acceleration — Security Audit
- [P] Plugin/driver interaction security hardening (Issue: #1394) (Target: Q1 2027)
- [I] Shader integrity verification (Issue: #1384) (Target: Q1 2027)

---

### Phase 6: Documentation, SDK & Ecosystem (Q2–Q4 2027) — 📋 Planned

Focus: Developer experience, official SDKs, and community ecosystem.

#### 6.1 SDKs
- [I] Python SDK from OpenAPI spec (Issue: #1507) (Target: Q2 2027)
- [I] JavaScript/TypeScript SDK (Issue: #1507) (Target: Q2 2027)
- [I] Go client library (Issue: #1507) (Target: Q2 2027)

#### 6.2 Documentation
- [I] Interactive API reference (Swagger UI / Redoc) (Target: Q2 2027)
- [I] Module-level architecture decision records (ADRs) for all 44 modules (Target: Q3 2027)
- [I] End-to-end tutorial series (20+ guides) (Target: Q3 2027)

#### 6.3 Plugin Ecosystem
- [P] `WASMKernelSandbox` — isolated execution environment for untrusted GPU kernel blobs (PR: #3051) (Target: v1.5.0) ✅
- [I] Plugin marketplace manifest standard (Issue: #1556) (Target: Q2 2027)
- [I] WASM-based plugin isolation for untrusted code (Issue: #1572) (Target: Q3 2027)
- [I] Remote plugin loading from authenticated registry (Issue: #1562) (Target: Q4 2027)

#### 6.4 Analytics & ML Ecosystem
- [I] Multi-language NLP support (beyond English/German) (Issue: #1478) (Target: Q3 2027)
- [I] Federated learning for privacy-preserving cross-institution training (Target: Q4 2027)
- [I] Model distillation from large to small adapters (Target: Q4 2027)

---

## Production Readiness Checklist

### Per-Module Requirements (applied to all 44 modules)
- [x] Module has `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`
- [x] Current Status section with maturity indicator (Alpha / Beta / Production-ready)
- [x] Unit test coverage target defined
- [x] Integration tests implemented or planned
- [x] Performance benchmarks defined
- [x] Security audit completed or scheduled
- [x] API stability guaranteed or documented as unstable
- [x] Prometheus metrics exported where applicable

### System-Wide Requirements
- [x] All 44 modules integrated into the CMake build system
- [x] Edition matrix (MINIMAL / COMMUNITY / ENTERPRISE / HYPERSCALER) enforced at build time
- [x] Docker image builds for all supported editions
- [x] CI pipeline covers core module matrix
- [~] GPU CI pipeline covers acceleration, gpu, geo, index modules
- [~] Cross-backend consistency tests for all accelerated modules
- [ ] Chaos engineering / fault injection testing at cluster level
- [ ] 99.99% uptime SLA validation (load + fault injection)
- [ ] Security penetration test report

---

## Known Cross-Module Issues & Limitations

| # | Module(s) | Description | Status |
|---|-----------|-------------|--------|
| 1 | acceleration | L2 distance consistency across CUDA/HIP/Vulkan/CPU backends | ✅ Fixed |
| 2 | acceleration | Vulkan compute shaders (distance kernels) not yet implemented | 🚧 In progress |
| 3 | chimera | Only ThemisDB self-benchmark adapter; third-party adapters pending | 📋 Planned |
| 4 | content | PDF extraction and OCR require optional third-party libraries | 📋 Planned |
| 5 | ingestion | libcurl stubs not yet replaced with real perform calls in `api_connector.cpp` | 🚧 In progress |
| 6 | ingestion | OAuth 2.0 token refresh within connectors unclear (Issue: #2408) | ❓ Unclear |
| 7 | sharding | Advanced distributed observability metrics incomplete | 🚧 In progress |
| 8 | storage | Production hardening (backup integration tests) in progress | 🚧 In progress |
| 9 | themis | Core module code still in `src/utils/` and `src/base/`; migration to `src/themis/` planned for v1.7.0 | 📋 Planned |
| 10 | config | Legacy config migration tooling not yet implemented | 📋 Planned |
| 11 | training | Multi-GPU distributed training coordination not implemented | 📋 Planned |
| 12 | prompt_engineering | Token counting / context-window budget enforcement not implemented | 📋 Planned |

---

## Breaking Changes

| Version | Module | Change |
|---------|--------|--------|
| v1.7.0 | themis | Module initialisation code migrated from `src/utils/` and `src/base/` to `src/themis/` |
| v2.0.0 | acceleration | GPU kernel API will stabilise; pre-v2 interfaces should be treated as unstable |
| v2.0.0 | api | `/v1/` versioned endpoints become the stable surface; unversioned endpoints deprecated |

---

## References

- [ARCHITECTURE.md](ARCHITECTURE.md) — Full system architecture documentation
- [README.md](README.md) — Project overview and quick start
- [CHANGELOG.md](CHANGELOG.md) — Release history
- [CONTRIBUTING.md](CONTRIBUTING.md) — Contribution guidelines
- [src/README.md](src/README.md) — Source directory overview
- [src/ROADMAP.md](src/ROADMAP.md) — Module-level roadmap index
