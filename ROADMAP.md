# ThemisDB Project Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

**Version:** 2.3  
**Last Updated:** 2026-06-08  
**Scope:** Aggregated roadmap across tracked modules in `src/` (scanner baseline currently tracks 65 modules; Phase 1-5 Gap Analysis Complete)

> For module-specific details see each module's `src/<module>/ROADMAP.md`.

---

## Overview

ThemisDB is a high-performance multi-model database with native AI/LLM integration. This top-level roadmap aggregates the status and planned work across tracked source modules. The project follows a hybrid systems strategy across storage, graph, vector, retrieval, and LLM infrastructure.

**Overall Timeline:** Q1 2026 – Q4 2027  
**Current Release:** v1.9.0-beta

---

## Root Governance: Terminology and Traceability

- **Feature:** a delivered or currently shipping capability mapped to a release milestone in this roadmap.
- **Enhancement:** planned follow-up work not yet shipped; tracked in `FUTURE_ENHANCEMENTS.md` and module-level `src/<module>/FUTURE_ENHANCEMENTS.md`.
- **Breaking Change:** incompatible API/ABI/configuration change; must be listed in this file (`## Breaking Changes`) and in `CHANGELOG.md`.

Traceability rules:

- Release scope starts in roadmap milestones (for example `## Milestone: v1.9.0`).
- Open enhancement backlog stays in `FUTURE_ENHANCEMENTS.md`.
- `CHANGELOG.md` entries must reference milestone scope and, where applicable, the related enhancement/backlog item.
- `RELEASE_STRATEGY.md` defines milestone naming and release-type alignment with `VERSION`/`RELEASE_TYPE`.
- `VERSIONING.md` defines the canonical release-type vocabulary (`alpha`, `beta`, `rc`, `stable`) and pre-release suffix rules.
- `COPILOT_INSTRUCTIONS.md` defines AI/agent update rules for root-governance and release/versioning document consistency.
- `FEATURE_ENHANCEMENT.md` is a generated maturity snapshot and is not the canonical planning backlog.

---

## Hybrid CPU/GPU Execution Model (Planning Baseline)

The target retrieval architecture in ThemisDB is not only layered, but also **execution-tiered**.
Planning and implementation should therefore assume a hybrid CPU/GPU model with explicit boundaries rather than a generic “move everything to GPU” strategy.

### Preferred Execution Boundaries

#### GPU-first or GPU-friendly
Best candidates for selective acceleration:
- ANN Frontdoor
- dense vector similarity and top-k search
- Tensor Mid-Layer summaries, fingerprints, routing, and contraction-style refinement
- bounded graph kernels such as batched frontier expansion
- LLM / LoRA inference and adapter-heavy generation paths

#### CPU-first
Best retained on CPU unless benchmark evidence proves otherwise:
- Graph Truth Layer final validation
- provenance and evidence-chain traversal
- ACL / permission enforcement
- policy-aware constraints
- exact multi-hop graph checks
- distributed coordination, shard-truth reconciliation, and recovery-critical control paths

### Operational Rollout Principle
The recommended implementation sequence is:
1. accelerate ANN candidate generation
2. add selective Tensor Mid-Layer acceleration
3. add bounded graph-kernel acceleration only where batch structure exists
4. preserve Graph Truth Layer as exact and CPU-first
5. size LLM / LoRA infrastructure based on upstream selectivity, not in isolation

### Planning Consequence
This execution model must be reflected in:
- benchmark design
- query planner design
- hardware profiles
- distributed tensor artifact planning
- module prioritization for `index`, `gpu`, `acceleration`, `graph`, `query`, and `sharding`

---

## Module Status Summary — Evidence-Based (from Gap Scanner v3 + Rescan 2026-05-27)

> ⚠️ **CRITICAL UPDATE (2026-05-27):** Latest gap-scan run reports **185,190 total gaps** across 27 scanner categories (CRITICAL 5,980 | HIGH 143,326 | MEDIUM 35,884 | Actionable 149,306). Historical and latest baselines are kept for trend tracking and planning prioritization.
>
> See comprehensive planning and rescan documentation in `ai_working/`:
> - [PHASE_5_IMPLEMENTATION_COMPLETE.md](ai_working/PHASE_5_IMPLEMENTATION_COMPLETE.md)
> - [PHASE_1_4_IMPROVEMENTS.md](ai_working/PHASE_1_4_IMPROVEMENTS.md)
> - [PHASE_6_SCANNER_DESIGN.md](ai_working/PHASE_6_SCANNER_DESIGN.md)
> - [IMPLEMENTATION_ROADMAP.md](ai_working/IMPLEMENTATION_ROADMAP.md)
> - [EXECUTIVE_DASHBOARD.md](ai_working/EXECUTIVE_DASHBOARD.md)
> - [GAP_SCAN_RESCAN_REPORT_2026-05-25.md](ai_working/GAP_SCAN_RESCAN_REPORT_2026-05-25.md)

**Latest Gap Scanner Results Summary (Rescan 2026-05-27):**
- Total gaps (current canonical snapshot): **185,190** across 27 categories
- CRITICAL: 5,980 | HIGH: 143,326 | MEDIUM: 35,884
- Actionable (C+H): 149,306
- Delta vs 194,852 baseline: **-9,662 total**, **-5,798 critical**
- Modules scanned: 65

### Execution-Model-Relevant Module Reality Check
The hybrid retrieval target architecture depends most directly on the maturity of the following modules:
- **index** — active work
- **gpu** — active work
- **graph** — active work
- **acceleration** — blocked / not ready
- **query** — active work
- **sharding** — blocked / not ready

This means the architecture is documented as a valid target state, but production rollout must remain staged.
In particular:
- ANN and Tensor acceleration are the most realistic early execution targets
- Graph Truth finalization must remain conservative and CPU-first
- distributed tensor retrieval depends on `query`, `sharding`, and `acceleration` maturity, not just document design

---

| **base** | 🟢 PRODUCTION | 12 | 0 | Stable, safe for production |
| **config** | 🟢 PRODUCTION | 18 | 1 | Stable configuration layer |
| **utils** | 🟢 PRODUCTION | 28 | 2 | Solid utility functions |
| **cache** | 🟢 PRODUCTION | 35 | 3 | Cache layer functional |
| **plugins** | 🟢 PRODUCTION | 42 | 4 | Plugin system operational |
| **auth** | 🟡 HARDENING | 145 | 35 | Fixing input validation, hardcoded secrets |
| **api** | 🟡 HARDENING | 156 | 38 | Error handling gaps being fixed |
| **governance** | 🟡 HARDENING | 168 | 41 | Policy validation in progress |
| **metadata** | 🟡 HARDENING | 124 | 31 | NULL checks, error propagation needed |
| **cdc** | 🟡 HARDENING | 137 | 33 | Exception handling improvements underway |
| **chaos** | 🟡 HARDENING | 142 | 34 | Test coverage expansion in progress |
| **aql** | 🟡 HARDENING | 151 | 37 | Parser edge cases being addressed |
| **core** | 🟡 HARDENING | 167 | 40 | DI container robustness improvements |
| **maintenance** | 🟡 HARDENING | 133 | 32 | Schedule validation being fixed |
| **analytics** | 🟡 HARDENING | 128 | 31 | Numeric stability improvements |
| **rpc_grpc** | 🟡 HARDENING | 142 | 34 | Timeout patterns, error handling being added |
| **temporal** | 🟡 HARDENING | 159 | 38 | Time precision logic being improved |
| **storage** | 🔴 ACTIVE WORK | 799 | 271 | **DO NOT USE IN PRODUCTION** — MVCC, transaction safety gaps |
| **index** | 🔴 ACTIVE WORK | 678 | 230 | **DO NOT USE IN PRODUCTION** — Bounds checks, query correctness |
| **query** | 🔴 ACTIVE WORK | 675 | 229 | **DO NOT USE IN PRODUCTION** — NULL checks, exception safety |
| **security** | 🚨 BLOCKED | 669 | 227 | **SECURITY AUDIT REQUIRED** — Hardcoded secrets, input validation gaps |
| **content** | 🔴 ACTIVE WORK | 525 | 178 | Format validation, path traversal issues |
| **network** | 🔴 ACTIVE WORK | 520 | 176 | Timeout patterns, retry logic missing |
| **importers** | 🔴 ACTIVE WORK | 481 | 163 | Error handling, format variants |
| **exporters** | 🔴 ACTIVE WORK | 456 | 155 | Output consistency gaps |
| **geo** | 🔴 ACTIVE WORK | 412 | 139 | Numerical precision, bounds checks |
| **gpu** | 🔴 ACTIVE WORK | 487 | 165 | CUDA error handling gaps |
| **ingestion** | 🔴 ACTIVE WORK | 468 | 159 | Data validation, error recovery |
| **transaction** | 🔴 ACTIVE WORK | 512 | 174 | Deadlock detection, rollback safety |
| **failover** | 🔴 ACTIVE WORK | 434 | 147 | Quorum logic, recovery timing |
| **projects** | 🔴 ACTIVE WORK | 445 | 151 | State machine validation gaps |
| **graph** | 🔴 ACTIVE WORK | 489 | 166 | Path finding correctness issues |
| **search** | 🔴 ACTIVE WORK | 501 | 170 | Ranking precision, recall gaps |
| **scheduler** | 🔴 ACTIVE WORK | 478 | 162 | Scheduling logic, cancellation |
| **process** | 🔴 ACTIVE WORK | 523 | 177 | Workflow orchestration gaps |
| **acceleration** | 🚨 BLOCKED | 612 | 207 | **NOT READY** — GPU kernel edge cases, cross-backend consistency |
| **onnx_clip** | 🚨 BLOCKED | 445 | 151 | **NOT READY** — Model loading, tensor validation |
| **stable_diffusion** | 🚨 BLOCKED | 468 | 159 | **NOT READY** — Image generation edge cases |
| **replication** | 🚨 BLOCKED | 534 | 181 | **NOT READY** — Consistency under failures |
| **distributed_knowledge** | 🚨 BLOCKED | 587 | 199 | **NOT READY** — RAID-5 reconstruction gaps |
| **rag** | 🚨 BLOCKED | 498 | 169 | **NOT READY** — Retrieval quality issues |
| **training** | 🚨 BLOCKED | 521 | 177 | **NOT READY** — LoRA fine-tuning correctness |
| **voice** | 🚨 BLOCKED | 456 | 155 | **NOT READY** — Audio processing gaps |
| **whisper** | 🚨 BLOCKED | 478 | 162 | **NOT READY** — Transcription accuracy issues |
| **llama_cpp** | 🚨 BLOCKED | 512 | 174 | **NOT READY** — Inference correctness, memory safety |
| **chimera** | 🚨 BLOCKED | 534 | 181 | **NOT READY** — Multi-vendor adapters incomplete, simulation mode |
| **ethics_ai** | 🚨 BLOCKED | 467 | 158 | **NOT READY** — Philosophy evaluation logic gaps |
| **document** | 🚨 BLOCKED | 445 | 151 | **NOT READY** — Format handling incomplete |
| **observability** | 🚨 BLOCKED | 512 | 174 | **NOT READY** — Metrics accuracy, tracing gaps |
| **prompt_engineering** | 🚨 BLOCKED | 489 | 166 | **NOT READY** — Template edge cases |
| **themis** | 🚨 BLOCKED | 556 | 188 | **NOT READY** — Wire protocol robustness issues |
| **llm** | 🚨 BLOCKED | 3,664 | 1,245 | **DO NOT USE IN PRODUCTION** — Exception safety, memory management, unimplemented paths |
| **sharding** | 🚨 BLOCKED | 2,051 | 696 | **DO NOT USE IN PRODUCTION** — Consistency guarantees, failover logic, unimplemented paths |
| **server** | 🚨 BLOCKED | 4,139 | 1,407 | **DO NOT USE IN PRODUCTION** — Missing timeouts, retry logic, error handling, stubs |

**Legend:** 🟢 PRODUCTION · 🟡 HARDENING · 🟤 ACTIVE WORK · 🚨 BLOCKED/NOT READY

---

## Hybrid Retrieval Rollout Priorities

### Priority 1 — ANN Frontdoor Acceleration
Scope:
- HNSW / DiskANN candidate generation
- GPU-assisted vector search where batch size and hardware justify it

### Priority 2 — Tensor Mid-Layer Refinement
Scope:
- tensor summaries
- routing tensors
- fingerprints
- shard relevance estimation
- summary-first retrieval

### Priority 3 — Bounded Graph Kernels
Scope:
- frontier-style expansion
- batched neighborhood exploration
- prepared graph math over bounded structures

Constraint:
These kernels are auxiliary acceleration paths, not replacements for Graph Truth semantics.

### Priority 4 — Graph Truth Finalization
Scope:
- exact relation validation
- provenance
- evidence chains
- ACL / policy checks
- exact multi-hop logic

Constraint:
This layer remains CPU-first unless specific bounded kernels are proven beneficial without weakening correctness or governance clarity.

---

## Planning Implications for Benchmarks and Issues

The roadmap and issue system should continue to reflect the following planning assumptions:

1. GPU work should be prioritized where arithmetic density and batching dominate.
2. CPU-first execution should be preserved where exactness, provenance, and governance dominate.
3. Planner work must explicitly model when summary-first retrieval is enough and when exact graph loading is required.
4. Hardware profiles and benchmark matrices must evaluate CPU SIMD, GPU dispatch, mmap-backed artifacts, and cross-shard transfer costs together.
5. Distributed tensor work must be staged behind realistic `query`, `sharding`, `index`, `graph`, and `acceleration` readiness.

---

## Milestone: v1.9.0

> **Target:** Q2 2026 · **Status:** 🚧 In Progress  
> **Issues:** Tracked per-module in individual `src/<module>/CHANGELOG.md [Unreleased]` sections

Key features planned and partially shipped for v1.9.0:

| Feature | Module | Status | Notes |
|---------|--------|--------|-------|
| `RequestCoalescer` Singleflight | cache | ✅ Shipped | promise/shared_future inflight map; 14 tests RC-01…RC-14 |
| `IStreamingJoin` / `HashJoin` / `IntervalJoin` | analytics | ✅ Shipped | Composite-key hash table, inner/left-outer, LRU pruning; 15 tests SJ-01…SJ-15 |
| `StreamingIngestManager` | storage | ✅ Shipped | Ring-buffer + flush-thread, ≥1 M events/s |
| `ColumnarCache` | storage | ✅ Shipped | LRU + PinGuard RAII |
| `TsStreamCursor` | timeseries | ✅ Shipped | Lazy paginated iterator, page_size=4 096 |
| `TSStore::putBatch` | timeseries | ✅ Shipped | Zero-copy batch write via single `WriteBatch` |
| `TemporalCompressor` LZ4 | temporal | ✅ Shipped | |
| `LockFreeHistogram<T>` | performance | ✅ Shipped | Header-only, atomic buckets, P50/P90/P99 |
| LIRS / RCU race fixes | performance | ✅ Shipped | |
| `AiHardwareDispatcher` v1.0 | acceleration | ✅ Shipped | NPU priority chain |
| NCCL/RCCL `mergeTopK` | acceleration | ✅ Shipped | |
| `IoUringBatchedSender` | network | ✅ Shipped | Single `io_uring_enter()` for N WireProtocolBatcher flushes |
| UUID v7 (RFC 9562) | utils | ✅ Shipped | `generate_uuid_v7()` |
| Streaming ZSTD | utils | ✅ Shipped | `zstd_compress_stream`/`zstd_decompress_stream` |
| MVCC_CLEANUP + STORAGE_COMPACTION | maintenance | ✅ Shipped | Wired in `http_server.cpp` |
| Concurrent-unique sentinel lock | index | ✅ Shipped | |
| `SecondaryIndexMetadataCache` | index | ✅ Shipped | |
| Paxos WAL durability | sharding | ✅ Shipped | `logPromise()`/`logAccept()`; 10 tests PSR-01…PSR-10 |
| `ShardRPCClient::writeEntity()` | sharding | ✅ Shipped | gRPC `ReplicateData` RPC for cross-shard writes |
| `ProcessLinker` hard-delete + secondary index | process | ✅ Shipped | Hard-delete via `db_.del()`, `obj_idx` prefix scan |
| `BpmnSerializer` state-machine tokenizer | process | ✅ Shipped | No-regex, CDATA, 11 tests PM-01…PM-11 |
| Typed DSL for structured prompt authoring | prompt_engineering | ✅ Shipped | `IPromptTemplate`, `IRAGContextBudgetManager`, `IPromptQualityEvaluator`, `IPromptABFramework` |
| `MqttClientService` + `MqttCDCTransport` | server | 🚧 In progress | Boost.Asio async I/O, RPCServiceRegistry |
| ISO 27001 + HIPAA compliance evaluators | governance | ✅ Shipped | |
| Chimera streaming result sets | chimera | ✅ Shipped | Prepared statements, connection pool adapter interfaces |
| MQTT client TLS support | server | 🚧 In progress | targets v1.10.0 |

---

## Implementation Phases

### Phase 1: Foundation Hardening (Q1–Q2 2026) — 🚧 In Progress

Focus: Bring all remaining Beta/Alpha modules to production grade. Eliminate known gaps in cross-backend consistency, error handling, and resource management.

#### 1.1 Acceleration Module — CUDA/Vulkan Kernel Completion
- [P] CUDA ANN + geospatial kernels production-ready (Issue: #1383) (Target: Q2 2026)
- [P] Vulkan compute shader pipeline (Issue: #1384) (Target: Q2 2026)
- [P] Cross-backend L2 distance consistency validation (Issue: #1390) (Target: Q2 2026)
- [I] Runtime device detection and capability negotiation (Issue: #1374) (Target: Q2 2026)

#### 1.2 API — OpenAPI & gRPC Surface
- [I] OpenAPI 3.x spec completeness for all endpoints (Issue: #1491) (Target: Q2 2026)
- [x] Versioned endpoint routing `/v1/`, `/v2/` with deprecation headers (Issue: #1506) (Target: Q3 2026)
- [x] SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue: #1507) (Target: Q3 2026)

#### 1.3 CDC — WebSocket & Streaming Transport
- [x] WebSocket transport for changefeed subscriptions (Target: Q2 2026)
- [x] Kafka integration for event streaming/importers (Target: Q3 2026)
- [I] Kinesis integration for event streaming (Target: Q3 2026)

#### 1.4 Chimera — Vendor Adapter Implementations
- [x] PostgreSQL adapter (Target: Q3 2026)
- [x] MongoDB adapter (Target: Q3 2026)
- [x] Weaviate adapter (Target: Q4 2026)

#### 1.5 Content — Binary Format Support
- [I] PDF text extraction (Target: Q2 2026)
- [I] OCR integration for image-embedded text (Target: Q3 2026)
- [I] Audio transcription pipeline (Target: Q3 2026)

#### 1.6 Core — Production DI Hardening
- [x] Full OpenTelemetry adapter coverage (Target: Q2 2026)
- [I] Production readiness checklist completion (Target: Q2 2026)

#### 1.7 Geo — GPU Kernel Completion
- [P] Geo CPU/GPU throughput benchmarks (`bench_geo_cpu_gpu.cpp`) (PR: #3049) (Target: v1.5.0) ✅
- [I] ST_BUFFER/ST_UNION/ST_DIFFERENCE CUDA kernels (Target: Q2 2026)
- [I] Full PostGIS ST_* function parity (Target: Q3 2026)

---

### Phase 2: AI/LLM Ecosystem Expansion (Q2–Q3 2026) — 📋 Planned

Focus: Deepen AI capabilities across prompt engineering, training, RAG, and analytics.

#### 2.1 Prompt Engineering
- [x] Token counting and context-window budget enforcement (Target: Q2 2026)
- [x] Typed template DSL with compile-time placeholder validation (Target: Q2 2026)
- [?] Batch A/B test runner with configurable traffic splits (Target: Q3 2026)
- [?] RLHF integration for prompt quality improvement (Target: Q4 2026)

#### 2.2 Training
- [?] Multi-GPU distributed training coordination (Target: Q2 2026)
- [?] Automated hyperparameter search (Target: Q2 2026)
- [?] Adapter serving integration with LLM inference layer (Target: Q3 2026)
- [?] Active learning loop for most-informative sample selection (Target: Q3 2026)
- [?] Domain adaptation beyond legal (Target: Q4 2026)

#### 2.3 RAG — Advanced Retrieval
- [I] Adaptive retrieval depth based on query complexity (Target: Q2 2026)
- [I] Multi-hop reasoning with intermediate knowledge graph traversal (Target: Q3 2026)
- [I] Retrieval confidence calibration and hallucination detection improvements (Target: Q3 2026)

#### 2.4 Analytics — GPU-Accelerated OLAP
- [P] GPU-accelerated OLAP aggregations via CUDA (Issue: #1469) (Target: Q3 2026)
- [I] Zero-copy Arrow data transfer optimisations (Issue: #1471) (Target: Q3 2026)
- [I] Arrow Flight RPC support for remote analytics (Issue: #1472) (Target: Q3 2026)

#### 2.5 Hybrid Retrieval Architecture Track
This track should continue to prefer the following rollout order:
- ANN frontdoor acceleration first
- Tensor Mid-Layer refinement second
- bounded graph kernels third
- Graph Truth exactness preserved throughout

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

---

### Phase 4: Observability & Operational Excellence (Q4 2026) — 📋 Planned

Focus: Enterprise-grade monitoring, alerting, and automated operations.

- [I] End-to-end distributed trace correlation across all 58 modules (Target: Q4 2026)
- [I] Anomaly-driven alerting with root cause analysis hints (Target: Q4 2026)
- [I] Continuous profiling integration (eBPF / perf) (Target: Q4 2026)

---

### Phase 5: Security Hardening & Compliance (Q1 2027) — 📋 Planned

Focus: Zero-trust, advanced compliance, and penetration-tested security posture.

- [P] `QueryMaskingPolicy` — dynamic PII field masking of query results (PR: #3050) (Target: v1.5.0) ✅
- [I] Zero-trust continuous verification framework (Issue: #1541) (Target: Q1 2027)
- [I] Automated SOC 2 Type II evidence collection (Target: Q1 2027)
- [P] Plugin/driver interaction security hardening (Issue: #1394) (Target: Q1 2027)
- [I] Shader integrity verification (Issue: #1384) (Target: Q1 2027)

---

### Phase 6: Documentation, SDK & Ecosystem (Q2–Q4 2027) — 📋 Planned

Focus: Developer experience, official SDKs, and community ecosystem.

- [I] Python SDK from OpenAPI spec (Issue: #1507) (Target: Q2 2027)
- [I] JavaScript/TypeScript SDK (Issue: #1507) (Target: Q2 2027)
- [I] Go client library (Issue: #1507) (Target: Q2 2027)
- [I] Interactive API reference (Swagger UI / Redoc) (Target: Q2 2027)
- [I] Module-level architecture decision records (ADRs) for all modules (Target: Q3 2027)
- [I] End-to-end tutorial series (Target: Q3 2027)

---

### Phase 7: Tensor-Native Index & Zero-Copy Inference (Q3 2026 – Q4 2027) — 📋 Planned

Focus: Tensor-Train (TT) compressed ANN indexing as a first-class SOC module parallel to HNSW/FAISS, with a zero-copy bridge to llama.cpp for RAG/FLARE inference and an AdaLoRA adapter sovereignty layer.

This phase must remain aligned with the hybrid execution principles above:
- tensor-native acceleration is valuable
- exact graph truth is still not replaced
- benchmark evidence is required before broad GPU generalization
