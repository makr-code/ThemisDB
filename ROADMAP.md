# ThemisDB Project Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

**Version:** 2.1  
**Last Updated:** 2026-04-13  
**Scope:** Aggregated roadmap across all 58 modules in `src/`

> For module-specific details see each module's `src/<module>/ROADMAP.md`.

---

## Overview

ThemisDB is a high-performance multi-model database with native AI/LLM integration. This top-level roadmap aggregates the status and planned work across all 58 source modules. The project follows a phased approach: stabilise core infrastructure first, then harden distributed and AI layers, and finally deliver operational excellence at hyperscale.

**Overall Timeline:** Q1 2026 – Q4 2027  
**Current Release:** v1.8.1-rc2

---

## Module Status Summary

| Module | Status | Individual Roadmap |
|--------|--------|--------------------|
| **acceleration** | ✅ Production-ready (v1.8.0) — AiHardwareDispatcher v1.0 (NPU priority chain), NCCL/RCCL mergeTopK, CUDA ANN/geospatial kernels, Vulkan compute pipeline; AC-4 filter operator and HardwareAccelerator tests complete | [src/acceleration/ROADMAP.md](src/acceleration/ROADMAP.md) |
| **analytics** | ✅ Production-ready | [src/analytics/ROADMAP.md](src/analytics/ROADMAP.md) |
| **api** | ✅ Production-ready — REST/gRPC/WebSocket/OpenAPI 3.x complete; GraphQL v1.x limitations version-gated | [src/api/ROADMAP.md](src/api/ROADMAP.md) |
| **aql** | ✅ Production-ready | [src/aql/ROADMAP.md](src/aql/ROADMAP.md) |
| **auth** | ✅ Production-ready | [src/auth/ROADMAP.md](src/auth/ROADMAP.md) |
| **base** | ✅ Production-ready | [src/base/ROADMAP.md](src/base/ROADMAP.md) |
| **cache** | ✅ Production-ready | [src/cache/ROADMAP.md](src/cache/ROADMAP.md) |
| **cdc** | ✅ Production-ready | [src/cdc/ROADMAP.md](src/cdc/ROADMAP.md) |
| **chaos** | ✅ Production-ready — FaultInjector (5 fault types), ChaosScheduler (cron + event trigger), deterministic chaos scheduling | [src/chaos/ROADMAP.md](src/chaos/ROADMAP.md) |
| **chimera** | 🟡 Beta — ThemisDB adapter functional; vendor adapters implemented in simulation mode | [src/chimera/ROADMAP.md](src/chimera/ROADMAP.md) |
| **config** | ✅ Production-ready | [src/config/ROADMAP.md](src/config/ROADMAP.md) |
| **content** | ✅ Production-ready — 13 format processors with >80% coverage; benchmark thresholds met; security hardening (zip-bomb, path, upload) verified | [src/content/ROADMAP.md](src/content/ROADMAP.md) |
| **core** | ✅ Production-ready — ConcernsContext DI, pluggable adapters, tracing/metrics/cache/secrets/feature-flags operational | [src/core/ROADMAP.md](src/core/ROADMAP.md) |
| **ethics_ai** | ✅ Production-ready (v0.2.0) — PhilosophyLoader (YAML rich thesis objects), EthicsEvaluator (configurable weights), ChainVisualizer (DOT/Mermaid export) | [src/ethics_ai/ROADMAP.md](src/ethics_ai/ROADMAP.md) |
| **exporters** | ✅ Production-ready | [src/exporters/ROADMAP.md](src/exporters/ROADMAP.md) |
| **distributed_knowledge** | ✅ Production-ready (v1.0.0) — RAID-5 knowledge sharding: 4 layers (11A–11D) fully wired; DK-1…DK-8 + DK-OR all complete; 13 OR tests + 5 OR benchmarks; DecisionRecord traceability (S-16) | [src/distributed_knowledge/ROADMAP.md](src/distributed_knowledge/ROADMAP.md) |
| **failover** | ✅ Production-ready — AutoFailoverManager (Raft-based, quorum), DisasterRecoveryManager (7-step DR plan with step hooks and dry_run) | [src/failover/ROADMAP.md](src/failover/ROADMAP.md) |
| **geo** | ✅ Production-ready — CPU spatial queries stable; GPU dispatch with documented CPU fallback; WGS-84 boundaries explicitly documented | [src/geo/ROADMAP.md](src/geo/ROADMAP.md) |
| **governance** | ✅ Production-ready — Policy engine incl. GDPR/HIPAA/CCPA/PCI/SOC2, OPA integration, model governance operational | [src/governance/ROADMAP.md](src/governance/ROADMAP.md) |
| **gpu** | ✅ Production-ready — Device management, P2P transfer, NVLink topology-aware scheduling complete; hardware capability benchmarks verified | [src/gpu/ROADMAP.md](src/gpu/ROADMAP.md) |
| **graph** | ✅ Production-ready — Cost-based optimiser, constrained path finding, distributed execution, EXPLAIN endpoint operational; GPU traversal kernels pending for full CUDA path | [src/graph/ROADMAP.md](src/graph/ROADMAP.md) |
| **importers** | ✅ Production-ready (v2.1) — Multi-source import pipeline incl. FK-preserving PostgreSQL importer and v1.x production-ready adapters | [src/importers/ROADMAP.md](src/importers/ROADMAP.md) |
| **index** | ✅ Production-ready — exportIndexStats to metadata module (Issue #1866) complete; multi-tenancy isolation, online rebuild, GPU oversubscription all operational | [src/index/ROADMAP.md](src/index/ROADMAP.md) |
| **ingestion** | ✅ Production-ready | [src/ingestion/ROADMAP.md](src/ingestion/ROADMAP.md) |
| **llama_cpp** | ✅ Production-ready (v2.2.0) — `LlamaWrapper` real inference (generate/embed/exportLoRA/importLoRA), streaming, batch inference, PluginManager hot-plug registrar | [src/llama_cpp/ROADMAP.md](src/llama_cpp/ROADMAP.md) |
| **llm** | ✅ Production-ready (v1.19.0) — LLM+RAID bridge: `getLLMStats()` ShardStats, `updateShardLLMLoad()` + LEAST_LOADED routing, `remote_draft_shard_id` | [src/llm/ROADMAP.md](src/llm/ROADMAP.md) |
| **maintenance** | ✅ Production-ready (v1.1.0) — Orchestration, schedule persistence, window enforcement, health aggregation complete | [src/maintenance/ROADMAP.md](src/maintenance/ROADMAP.md) |
| **metadata** | ✅ Production-ready | [src/metadata/ROADMAP.md](src/metadata/ROADMAP.md) |
| **network** | ✅ Production-ready | [src/network/ROADMAP.md](src/network/ROADMAP.md) |
| **observability** | ✅ Production-ready | [src/observability/ROADMAP.md](src/observability/ROADMAP.md) |
| **onnx_clip** | ✅ Production-ready (v0.2.0) — all v0.2.0 items done: Prometheus metrics, model integrity check (SHA-256), 26 unit tests | [src/onnx_clip/ROADMAP.md](src/onnx_clip/ROADMAP.md) |
| **performance** | ✅ Production-ready | [src/performance/ROADMAP.md](src/performance/ROADMAP.md) |
| **plugins** | ✅ Production-ready | [src/plugins/ROADMAP.md](src/plugins/ROADMAP.md) |
| **process** | ✅ Production-ready — BPMN/EPK/VCC-VPB import, Graph-RAG, ProcessLinker, HNSW + full-text retrieval operational; ARIS-XML import (AML v9/v10) + AgenticRAG iterative Q&A (2026-04-17) | [src/process/ROADMAP.md](src/process/ROADMAP.md) |
| **projects** | ✅ Production-ready (v1.0.0) — Project lifecycle state machine, snapshot versioning, structural diff/merge, template instantiation (`BLANK/ANALYTICS/ML_PIPELINE/REPORT`), concurrent collaboration session management | [src/projects/ROADMAP.md](src/projects/ROADMAP.md) |
| **prompt_engineering** | ✅ Production-ready (v1.x) | [src/prompt_engineering/ROADMAP.md](src/prompt_engineering/ROADMAP.md) |
| **query** | ✅ Production-ready | [src/query/ROADMAP.md](src/query/ROADMAP.md) |
| **rag** | ✅ Production-ready | [src/rag/ROADMAP.md](src/rag/ROADMAP.md) |
| **replication** | ✅ Production-ready | [src/replication/ROADMAP.md](src/replication/ROADMAP.md) |
| **rpc_grpc** | ✅ Production-ready (v0.0.2) — `GRPCServer` + `GRPCPlugin` fully functional gRPC server; service registry integration | [src/rpc_grpc/ROADMAP.md](src/rpc_grpc/ROADMAP.md) |
| **scheduler** | ✅ Production-ready (v1.5.0) | [src/scheduler/ROADMAP.md](src/scheduler/ROADMAP.md) |
| **search** | ✅ Production-ready (v1.2.0+) | [src/search/ROADMAP.md](src/search/ROADMAP.md) |
| **security** | ✅ Production-ready | [src/security/ROADMAP.md](src/security/ROADMAP.md) |
| **server** | ✅ Production-ready | [src/server/ROADMAP.md](src/server/ROADMAP.md) |
| **sharding** | ✅ Production-ready — mTLS RPC integration, WAL/consensus recovery, consistent-hash routing (>10K ops/s), chaos-engineering suite all verified | [src/sharding/ROADMAP.md](src/sharding/ROADMAP.md) |
| **stable_diffusion** | ✅ Production-ready (v2.3.0) — `SDCppGenerator` (stable-diffusion.cpp C API), real PNG encoder (IDAT/CRC32/Adler32), img2img, batch generation, thread-safe; `SDPluginAdapter`+`SDPluginRegistrar` PluginManager hot-plug integration | [src/stable_diffusion/ROADMAP.md](src/stable_diffusion/ROADMAP.md) |
| **storage** | ✅ Production-ready (v1.8.0) — RocksDB-based persistent storage incl. MVCC/WAL/backup-PITR/NVMe/erasure coding/2PC | [src/storage/ROADMAP.md](src/storage/ROADMAP.md) |
| **temporal** | ✅ Production-ready (v1.2.0 C++ engine) — System-versioned + bi-temporal queries, time-travel, temporal joins, index acceleration | [src/temporal/ROADMAP.md](src/temporal/ROADMAP.md) |
| **themis** | ✅ Production-ready — All core components migrated to `src/themis/`; Wire Protocol V2 delivered; integration tests added (v1.8.0) | [src/themis/ROADMAP.md](src/themis/ROADMAP.md) |
| **timeseries** | ✅ Production-ready | [src/timeseries/ROADMAP.md](src/timeseries/ROADMAP.md) |
| **toolbox** | ✅ Production-ready — System-wide integration layer: `IngestionToolbox`, `ToolboxBuilder`, `ContentToolboxBridge`, `ToolboxRegistry` (process-global); text-processing primitives (chunker, normalizer, fingerprinter, quality scorer, language detector) | [src/toolbox/ROADMAP.md](src/toolbox/ROADMAP.md) |
| **training** | ✅ Production-ready (v1.x) | [src/training/ROADMAP.md](src/training/ROADMAP.md) |
| **transaction** | ✅ Production-ready | [src/transaction/ROADMAP.md](src/transaction/ROADMAP.md) |
| **updates** | ✅ Production-ready | [src/updates/ROADMAP.md](src/updates/ROADMAP.md) |
| **utils** | ✅ Production-ready | [src/utils/ROADMAP.md](src/utils/ROADMAP.md) |
| **user_storage_encrypted** | ✅ Production-ready (v0.1.0) — Argon2id KDF, gocryptfs backend, AES-256-GCM encrypted user storage; stdin key delivery | [src/user_storage_encrypted/ROADMAP.md](src/user_storage_encrypted/ROADMAP.md) |
| **voice** | ✅ Production-ready | [src/voice/ROADMAP.md](src/voice/ROADMAP.md) |
| **whisper** | ✅ Production-ready (v2.1.0) — Thread-safe; FFmpeg audio chunk reader (MP3/OGG); CompositeAudioChunkReader; `WhisperPluginAdapter`+`WhisperPluginRegistrar` PluginManager hot-plug integration; 44+12 tests | [src/whisper/ROADMAP.md](src/whisper/ROADMAP.md) |

**Legend:** ✅ Production-ready · 🟡 Beta · 🔴 Alpha · 🚧 In active hardening · *(58 modules total)*

---

## Milestone: v1.5.0

> **Release Target Document:** [`docs/de/releases/RELEASE_TARGET_v1.5.0.md`](docs/de/releases/RELEASE_TARGET_v1.5.0.md)
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
| CHIMERA Suite Branding | benchmarks | Rebranded benchmark framework; `CHIMERA_RESULTS_*` naming; docs + CI updated |
| API Versioning and Compatibility Strategy | server / api | `Accept-Version` / `API-Version` headers, deprecation policy, `APIVersionManager` |
| Query Result Pagination | query / server | Cursor / keyset / offset pagination; `PaginatedResponse`; 17 tests |
| Plugin Metrics and Monitoring | plugins | `PluginMetrics`; P95/P99 latency; Prometheus integration |
| Schema Manager | storage | Runtime schema, field type, and index metadata introspection |
| Independent Health / Error Service | server | Dedicated port 9090; `/health`, `/readiness`, `/error-summary` |
| [#3471](https://github.com/makr-code/ThemisDB/pull/3471) | tests / benchmarks | Coverage audit: 6 benchmarks + 21 unit test files |
| [#3472–#3484](https://github.com/makr-code/ThemisDB/pull/3484) | docs (all modules) | Full 44-module documentation audit and sync |
| [#3480](https://github.com/makr-code/ThemisDB/pull/3480) | ci | Documentation validation CI workflow |
| [#3485](https://github.com/makr-code/ThemisDB/pull/3485) | rag / research | RAG scientific foundations (40 IEEE citations) |
| [#84](https://github.com/makr-code/ThemisDB/issues/84) | observability | Root Cause Analyzer — `RootCauseAnalyzer` with `analyzeIssue`, `findCorrelations`, `buildCausalGraph` |
| Documentation Archival System | docs | Formal archival process; 70+ documents moved to `docs/implementation-history/` |
| Retroactive Release Building System | ci / docs | Reproducible binary builds from historical version tags |

**Breaking change:** `themis` module initialisation code migrated from `src/utils/` / `src/base/` to `src/themis/`.

---

## Milestone: v1.8.0

> **Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.8.0.md`](docs/de/releases/RELEASE_NOTES_v1.8.0.md)
> **Issues:** [#4300](https://github.com/makr-code/ThemisDB/issues/4300)

Key PRs and features included in v1.8.0:

| PR / Feature | Module | Purpose |
|---|--------|---------|
| [#4279](https://github.com/makr-code/ThemisDB/pull/4279), [#4270](https://github.com/makr-code/ThemisDB/pull/4270) | auth | JWT scope enforcement — `JWTClaims.scopes`, `role_scope_map_`, OAuth2 `scope`/`scp` |
| [#4280](https://github.com/makr-code/ThemisDB/pull/4280) | security | `ArrowUserRegistrationPlugin` — Apache Arrow-backed user store, SHA-256 auth (Issue #99) |
| [#4283](https://github.com/makr-code/ThemisDB/pull/4283), [#4292](https://github.com/makr-code/ThemisDB/pull/4292) | acceleration | CRL / OCSP certificate revocation in `PluginSecurityVerifier` (Issue #38) |
| [#4281](https://github.com/makr-code/ThemisDB/pull/4281) | transaction | Serializable Snapshot Isolation — `IsolationLevel::SerializableSnapshot`, 38 tests (Issue #122) |
| SAGA | transaction | SAGA Orchestration Engine — execute/validate/getStatus/template management, 23 tests |
| [#4285](https://github.com/makr-code/ThemisDB/pull/4285) | server | Versioned API Routing — `RouteVersionRouter`, `/v1/` + `/v2/` (bulk NDJSON, SSE, async jobs), 37 tests |
| PredictivePrefetcher | cache | Markov-chain + 24-bucket ToD weighting, RocksDB persistence, A/B toggle, 14 tests |
| [#4250](https://github.com/makr-code/ThemisDB/pull/4250) | cache | Warmup Parallel Bulk Load (Issue #244) |
| Geo Clustering | geo | DBSCAN + K-means clustering engine, 20 tests (Issue #4003) |
| [#4299](https://github.com/makr-code/ThemisDB/pull/4299) | graph | `DistributedGraphManager` read-path `std::shared_mutex` upgrade |
| PolicyManager | governance | Hot-reload with `reloadPolicies()`, double-buffer swap, `PolicyValidator`, 7 tests |
| HuggingFace Hub | exporters | 429 back-off, `Retry-After` parsing, `ExporterMetrics`, 5 tests |
| [#4289](https://github.com/makr-code/ThemisDB/pull/4289) | performance | `HardwareAccelerator` — AC-4 filter operator completeness, 45 tests (Issue #85) |
| [#4284](https://github.com/makr-code/ThemisDB/pull/4284) | analytics | `ExporterFactory` — concrete Arrow / Parquet / Feather / JSON exporters (Issue #3868) |
| [#4297](https://github.com/makr-code/ThemisDB/pull/4297) | analytics | `JoinExporter` — cross-collection hash-join with PII redaction |
| [#4291](https://github.com/makr-code/ThemisDB/pull/4291) | analytics | `CEPEngine` deadlock fix — release window lock before user callbacks |
| [#4266](https://github.com/makr-code/ThemisDB/pull/4266), [#4267](https://github.com/makr-code/ThemisDB/pull/4267) | themis | Wire Protocol V2 — RFC 7540 §6.3 / §5.3.1 full compliance |
| [#4253](https://github.com/makr-code/ThemisDB/pull/4253) | config | SIGHUP hot-reload — inotify / kqueue / ReadDirectoryChangesW |
| [#4265](https://github.com/makr-code/ThemisDB/pull/4265) | sharding | `GpuErasureCoderOpenCL` encode/decode/batchEncode (Issue #105) |
| [#4257](https://github.com/makr-code/ThemisDB/pull/4257) | performance | Intelligent Prefetching System (Issue #192) |
| [#4258](https://github.com/makr-code/ThemisDB/pull/4258) | query | Materialized Views & Incremental Maintenance (Issue #195) |
| [#4271](https://github.com/makr-code/ThemisDB/pull/4271), [#4273](https://github.com/makr-code/ThemisDB/pull/4273) | network | UDP ingestion server + Bandwidth Management / QoS (Issue #190) |
| [#4288](https://github.com/makr-code/ThemisDB/pull/4288) | importers | MySQL / MariaDB importer |
| [#4290](https://github.com/makr-code/ThemisDB/pull/4290) | ci | GitHub Actions 138-workflow reorganisation into 9 functional categories |

**Breaking changes:** ZSTD replaces zlib in `StreamWriter`; unversioned HTTP paths redirect 301 to `/v1/`; CI workflow files relocated (see `.github/WORKFLOW_REGISTRY.md`).

---

## Milestone Delta (2026-04-13)

Recently merged PRs and documentation aligned to their target milestones:

| Milestone | PR | Scope |
|---|---|---|
| v1.9.0 | [#4478](https://github.com/makr-code/ThemisDB/pull/4478) | chimera - streaming result sets, prepared statements, connection pool adapter interfaces |
| v1.9.0 | [#4484](https://github.com/makr-code/ThemisDB/pull/4484) | governance - ISO 27001 and HIPAA compliance rule evaluators |
| v1.9.1 | [#4474](https://github.com/makr-code/ThemisDB/pull/4474) | auth - register missing focused test targets |
| v1.10.0 | [#4512](https://github.com/makr-code/ThemisDB/pull/4512) | server - MQTT client TLS support |
| v2.0.0 | [#4477](https://github.com/makr-code/ThemisDB/pull/4477) | cdc - replay/filter/batch-commit coordinator interfaces |
| v2.0.0 | [#4569](https://github.com/makr-code/ThemisDB/pull/4569) | query - v2.0.0 port for issue #3528 |
| v2.0.0 | [#4570](https://github.com/makr-code/ThemisDB/pull/4570) | storage - v2.0.0 port for issue #3536 |
| v2.1.0 | [#4555](https://github.com/makr-code/ThemisDB/pull/4555) | stable_diffusion - batch generation, img2img, thread-safety |
| v2.1.0 | [#4556](https://github.com/makr-code/ThemisDB/pull/4556) | llama_cpp - streaming, batch inference, PluginManager hot-plug registrar |
| v2.4.0 | [#4511](https://github.com/makr-code/ThemisDB/pull/4511) | search - conversational/federated/streaming search interfaces |

Selected 2026-04-12/13 production items (target: v1.9.0 unless noted):

| Module | Item |
|--------|------|
| cache | `RequestCoalescer` Singleflight (promise/shared_future inflight map, 14 tests RC-01…RC-14) |
| analytics | `IStreamingJoin` / `HashJoin` / `IntervalJoin` (composite-key hash table, inner/left-outer, LRU pruning, 15 tests SJ-01…SJ-15) |
| storage | `StreamingIngestManager` (ring-buffer + flush-thread, ≥1 M events/s), `ColumnarCache` (LRU + PinGuard RAII) |
| timeseries | `TsStreamCursor` (lazy paginated iterator, page_size=4 096), `TSStore::putBatch` (zero-copy via single `WriteBatch`) |
| temporal | `TemporalCompressor` LZ4 support |
| performance | `LockFreeHistogram<T>` header-only (atomic buckets, P50/P90/P99), LIRS/RCU fixes |
| acceleration | `AiHardwareDispatcher` v1.0 (NPU priority chain), NCCL/RCCL `mergeTopK` |
| network | `IoUringBatchedSender` (single `io_uring_enter()` for N WireProtocolBatcher flushes) |
| utils | UUID v7 (RFC 9562), streaming ZSTD (`zstd_compress_stream`/`zstd_decompress_stream`) |
| maintenance | MVCC_CLEANUP + STORAGE_COMPACTION wired in `http_server.cpp` |
| index | Concurrent-unique sentinel locking fix, `SecondaryIndexMetadataCache` |
| stable_diffusion | `SDCppGenerator` v2.2.0 (real PNG encoder, img2img, 51 tests A-Q) |
| whisper | `WhisperPlugin` v2.1.0 (thread-safe, `FfmpegAudioChunkReader`, `CompositeAudioChunkReader`, 36 tests A-L) |
| sharding | Paxos WAL durability (`handlePrepare`/`handleAccept`→`wal_->logPromise()`/`logAccept()`, 10 tests PSR-01…PSR-10); `ShardRPCClient::writeEntity()` gRPC cross-shard writes |
| process | `ProcessLinker` hard-delete + secondary index; `BpmnSerializer` state-machine tokenizer (no-regex, 11 tests PM-01…PM-11) |
| ethics_ai | `PhilosophyLoader` rich YAML, `EthicsEvaluator::Config` weights, `ChainVisualizer` DOT/Mermaid, 8 tests CV-01…CV-08 |

Superseded PR mapping:

- [#4507](https://github.com/makr-code/ThemisDB/pull/4507) superseded by [#4569](https://github.com/makr-code/ThemisDB/pull/4569)
- [#4515](https://github.com/makr-code/ThemisDB/pull/4515) superseded by [#4570](https://github.com/makr-code/ThemisDB/pull/4570)

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
| Typed DSL for structured prompt authoring | prompt_engineering | ✅ Shipped | `IPromptTemplate`, `IRAGContextBudgetManager`, `IPromptQualityEvaluator`, `IPromptABFramework` (2026-04-19) |
| `MqttClientService` + `MqttCDCTransport` | server | 🚧 In progress | Boost.Asio async I/O, RPCServiceRegistry |
| ISO 27001 + HIPAA compliance evaluators | governance | ✅ Shipped (#4484) | |
| Chimera streaming result sets | chimera | ✅ Shipped (#4478) | Prepared statements, connection pool adapter interfaces |
| MQTT client TLS support | server | 🚧 In progress (#4512, targets v1.10.0) | |

**Breaking changes planned for v1.9.0:** None anticipated; minor API additions only.

**v1.9.0 Acceptance Criteria:**
- All items marked `✅ Shipped` in the table above merged and green in CI
- `MqttClientService` integration tests passing
- `prompt_engineering` token budget enforcer unit tests ≥ 90% coverage
- No P0/P1 open bugs against the milestone
- Release notes and migration guide updated

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
- [x] Versioned endpoint routing `/v1/`, `/v2/` with deprecation headers (Issue: #1506) (Target: Q3 2026)
- [x] SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue: #1507) (Target: Q3 2026)

#### 1.3 CDC — WebSocket & Streaming Transport
- [x] WebSocket transport for changefeed subscriptions (Target: Q2 2026)
- [x] Kafka integration for event streaming/importers (Target: Q3 2026)
- [I] Kinesis integration for event streaming (Target: Q3 2026)

#### 1.4 Chimera — Vendor Adapter Implementations
- [x] PostgreSQL adapter (Issue: alpha) (Target: Q3 2026)
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

#### 1.8 Ingestion — Distributed & Cloud Sources
- [x] Kafka consumer source connector (Issue: #1892) (Target: Q3 2026)
- [x] S3/GCS/Azure Blob object-storage source (Issue: #1893) (Target: Q3 2026)
- [x] OAuth 2.0 token refresh within connectors (Issue: #2408) (Target: Q3 2026)

#### 1.9 Sharding — Observability & Repair
- [x] Advanced metrics and distributed tracing (`sharding/operational_metrics.cpp`, `observability/distributed_flame_graph.cpp`, `observability/ebpf_tracer.cpp`)
- [I] Automated shard rebalancing (Target: Q3 2026)

#### 1.10 Storage — Production Hardening
- [x] Benchmark-driven performance optimisation (`tests/test_storage_latency_bench.cpp`)
- [x] Backup/PITR integration tests (`tests/test_backup_restore_integration.cpp`)

#### 1.11 High-Priority API Modernization Epic (Review 03/2026)
- [x] GraphQL API incl. subscriptions production-ready, documented, and tested (Target: v1.7.0–v1.8.0)
- [x] WebSocket CDC for real-time changefeeds (`/v2/changes`, `/v2/cdc/stream`) (Target: v1.7.0–v1.8.0)
- [x] Versioned API routing (`/v2/`) with legacy compatibility (`/v1/` + redirects) (Target: v1.8.0)
- [x] LLM API streaming (SSE/chunked) + OpenAI-compatible `/v1/chat/completions` with regression tests (Target: v1.7.0)
- [x] Kafka consumer importer + S3-compatible source connectors production-ready (Target: v1.7.0–v1.8.0)
- [x] Geo functionality production-ready: R-tree index, spatial JOIN, temporal-spatial queries, benchmarks (Target: v1.5.0–v1.8.0)
- [x] OpenTelemetry full integration + custom metric types integrated (Target: v1.6.0)

---

### Phase 2: AI/LLM Ecosystem Expansion (Q2–Q3 2026) — 📋 Planned

Focus: Deepen AI capabilities across prompt engineering, training, RAG, and analytics.

#### 2.1 Prompt Engineering
- [x] Token counting and context-window budget enforcement (Target: Q2 2026) — `ContextWindowBudgetManager` + `IRAGContextBudgetManager` (2026-04-19)
- [x] Typed template DSL with compile-time placeholder validation (Target: Q2 2026) — `CompiledPromptTemplate` + `IPromptTemplate` + `IPromptQualityEvaluator` + `IPromptABFramework` (2026-04-19)
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

#### 2.6 LoRA Foundation — Loops 1–4 + Dataset (Target: Q3 2026)
*Defined in: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md`*
- [x] IMPL-A1: Golden dataset CLI + `DatabaseDomainAutoLabeler` — Inputs: query logs + FeedbackCollector; Outputs: JSONL label + confidence ≥ 0.7 (Target: Q3 2026)
  (`include/training/database_domain_auto_labeler.h` + `src/training/database_domain_auto_labeler.cpp`, 8 tests in `tests/test_database_domain_auto_labeler.cpp`. `DomainType` extended in `include/training/auto_labeler.h`.)
- [x] IMPL-A2: Loop 1–4 explicit orchestration in `ContinuousLearningOrchestrator` — `LoopPhase` enum, `triggerLoop()`, guardrails; all 4 loops named and testable (Target: Q3 2026)
  (`include/rag/continuous_learning_orchestrator.h` + `src/rag/continuous_learning_orchestrator.cpp`. `getMissRate()`, `getProfileDrift()`, `newEntryCount()` accessors added. 10 tests appended to `tests/test_continuous_learning_orchestrator.cpp`.)
- [x] IMPL-A3: `exportGradient()` + `applyGlobalDelta()` + `FEDERATED_ROUND_START` — bridge between LoRA pipeline and Layer 11B (Implemented: 2026-04-17)
  (`include/training/incremental_lora_trainer.h` + `include/rag/continuous_learning_orchestrator.h`. 5 tests ILT-EG-01..03, ILT-AG-01..02 + 3 CLO-FED tests.)

#### 2.7 LLM Optimization Layers 5–10 (Target: Q3–Q4 2026)
*Defined in: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md`*
- [x] IMPL-B5: `TransactionSemanticAdvisor` — batch-affinity hints, `analyzeBatch()` ≤ 10 ms (Implemented: 2026-04-17)
  (`include/transaction/transaction_semantic_advisor.h` + 8 tests TSA-01..08.)
- [x] IMPL-B6: `SchemaDeadWeightDetector` — 180-day window, seasonality, 0 GDPR false-negatives (Implemented: 2026-04-17)
  (`include/storage/schema_dead_weight_detector.h` + 10 tests SDWD-01..10.)
- [x] IMPL-B7: `IntentClassifier` — SQL-injection/exfiltration, precision ≥ 80 % v1.0 → ≥ 92 % post-LoRA (Implemented: 2026-04-17)
  (`include/security/intent_classifier.h` + 8 tests IC-01..08.)
- [x] IMPL-B8: `WorkloadFingerprintEngine` — OLTP/OLAP/Batch, similarity-match ≥ 80 % accuracy (Implemented: 2026-04-17)
  (`include/server/workload_fingerprint_engine.h` + 8 tests WFE-01..08.)
- [x] IMPL-B9: `ExplainabilityReasonBuilder` — causal chain for 100 % of autonomous decision types (Implemented: 2026-04-17)
  (`include/rag/explainability_reason_builder.h` + 10 tests ERB-01..10.)
- [x] IMPL-B10: `StorageLayoutAdvisor` — Row/Columnar/Hybrid, ≥ +50 % compression for time-series (Implemented: 2026-04-17)
  (`include/storage/storage_layout_advisor.h` + 10 tests SLA-01..10.)

#### 2.8 Distributed Knowledge — Layer 11 (Implemented: 2026-04-17)
*Defined in: `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md` · `src/distributed_knowledge/ROADMAP.md`*
- [x] DK-1: Build system + 25 unit tests for `distributed_knowledge` module (Implemented: 2026-04-17)
- [x] DK-2: Layer 11A — GossipProtocol `registerCustomHandler()` + `routeByDomain()` (Implemented: 2026-04-17)
- [x] DK-3: Layer 11B — FedAvg + DP aggregation wired to `IncrementalLoRATrainer` (Implemented: 2026-04-17)
- [x] DK-4: Layer 11C — `QueryFederation` RAG-aware merge, Recall@10 ≥ +15 % vs. shard-local (Implemented: 2026-04-17)
- [x] DK-5: Layer 11D — `CrossShardFeedbackSync` wired to `FeedbackCollector` + RLAIF (Implemented: 2026-04-17)
- [x] DK-6: End-to-end integration (7 scenarios) + privacy invariant test (Implemented: 2026-04-17)
- [x] DK-7: Admin API + SphincsPlus audit + `CrossBorderTransferPolicy` (Implemented: 2026-04-17)
- [x] DK-8: Performance benchmarks — `triggerAggregation()` ≤ 500 ms, `merge()` ≤ 20 ms (Implemented: 2026-04-17)
- [x] DK-OR: Operational Resilience hardening — backpressure, timeouts, GDPR erase, ZeroTrust (Implemented: 2026-04-17)

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
- [I] End-to-end distributed trace correlation across all 58 modules (Target: Q4 2026)
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

#### 4.5 Maintenance — Advanced Orchestration
- [x] Explicit per-task DAG dependency graph with topological sort (Target: v1.2.0) — `MaintenanceTaskDependency` + `resolveTaskExecutionOrder` (Kahn's algorithm) in `database_maintenance_orchestrator.h/cpp` ✅
- [x] Replica consistency check integration with sharding/replication module (Target: v1.2.0) — `ShardRepairEngine::runConsistencyCheck()` + `makeReplicaValidationHandler()` factory in `maintenance_task_handler_impls.h` ✅
- [x] StorageCompaction integration with `CompactionManager` (Target: v1.2.0) — `StorageCompactionHandler` in `maintenance_task_handler_impls.h` wired to `CompactionManager::compactAll()` ✅

#### 4.6 Process — Semantic Search & LLM Integration
- [x] Auto-generate process model embeddings via LLM module on import (Target: Q2 2026)
- [x] Full-text inverted index over process model descriptions (Target: Q2 2026)
- [x] AgenticRAG integration for iterative process question answering (Target: Q3 2026) — `ProcessAgenticRag` in `include/process/process_agentic_rag.h` (2026-04-17)
- [x] EPK ARIS-XML import (Target: Q3 2026) — `EpkArisXmlImporter` in `include/process/epk_aris_xml_importer.h`, AML v9/v10 (2026-04-17)

#### 4.7 CLI Tooling — Unified Management Interface
- [x] `themisctl` — unified ThemisDB CLI for server operations (Target: Q1 2026)
  - Commands: `health`, `version`, `query`, `get`, `put`, `delete`, `schema`, `branch`, `snapshot`, `admin`
  - Environment variable support: `THEMIS_HOST`, `THEMIS_PORT`, `THEMIS_TOKEN`
  - Raw JSON output mode (`--json`), auth token forwarding (`--token`), configurable timeout
  - In-process httplib unit tests (arg parsing, HTTP round-trips, error handling)
  - CMake target: `themisctl`; install component: `tools`
- [x] Shell completion scripts for `themisctl` (Target: Q2 2026)
  - Bash: `tools/completion/themisctl.bash` — installed to `share/bash-completion/completions/`
  - Zsh:  `tools/completion/_themisctl`      — installed to `share/zsh/site-functions/`
  - Fish: `tools/completion/themisctl.fish`  — installed to `share/fish/vendor_completions.d/`
  - Covers all commands and sub-commands; `config set` offers known key completions
- [x] `themisctl config` sub-command — read/write server config via API (Target: Q2 2026)
  - `config get` — GET `/config`, pretty-printed JSON
  - `config set key=value ...` — POST `/config` hot-reload patch (dotted key → nested JSON)
  - Supported keys: `logging.level`, `logging.format`, `request_timeout_ms`, `features.*`, `cdc_retention_hours`
  - 9 unit tests for config get/set/error paths
- [x] `themisctl repl` — interactive REPL mode with command history (Target: Q2 2026)
  - Shell-style tokenizer with single/double quote support (`tokenizeLine`)
  - GNU Readline integration when available (`THEMISCTL_ENABLE_READLINE`); plain getline() fallback
  - History persisted to `~/.themisctl_history`; exits on `exit`, `quit`, or EOF (Ctrl-D)
  - 9 tokenizer unit tests
- [x] `themisctl config` schema validation — dry-run + diff output (Target: Q3 2026) — `themisctl config validate [key=value ...]` → POST `/config/validate`; diff display in `tools/themisctl.cpp` ✅
- [x] AgentRAG integration — `themisctl rag query [--collection C] [--top-k N] [--lora ID] <nl-question>` → POST `/api/v1/llm/rag`; answer + retrieval metadata display in `tools/themisctl.cpp` (2026-04-17) ✅

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

### Phase 5.5: QTS / QNAP Admin UI (Q2 2026) — 🚧 Phase 2 Complete

Focus: Lightweight web admin UI for ThemisDB on QNAP Container Station (QTS).

#### 5.5.1 Phase 1 — Sidecar Admin UI MVP

- [x] Static single-page admin UI (HTML/CSS/vanilla JS, no build step) — `docker/admin-ui/app/`
- [x] nginx sidecar container with reverse proxy `/api/* → ThemisDB:8080` — `docker/admin-ui/nginx.conf`
- [x] Admin UI Docker image (`docker/admin-ui/Dockerfile`) — nginx:1.25-alpine
- [x] QNAP Container Station compose file — `docker-compose.qnap.yml`
  - ThemisDB from Docker Hub (`makrcode/themisdb:latest`) on port 18765
  - Admin UI sidecar on port 18766
  - Bridge network `themis-net`; named volumes for data + logs
- [x] Dashboard: health status, version, uptime, request count, DB size
- [x] Collections browser: list with document count + size
- [x] AQL query editor (Ctrl+Enter to execute)
- [x] Backup/Restore UI (`POST /admin/backup`, `POST /admin/restore`)
- [x] Monitoring: raw Prometheus metrics viewer (`GET /metrics`)
- [x] German setup & operations guide — `docs/de/admin_tools/qts-inline-admin.md`
- [x] English setup & operations guide — `docs/en/admin_tools/qts-inline-admin.md`

#### 5.5.2 Phase 2 — Security Hardening ✅ (v1.1.0, 2026-04-16)

- [x] TLS termination via QNAP reverse proxy or Let's Encrypt — `docker/admin-ui/nginx.ssl.conf` (HTTP→HTTPS redirect + TLS 1.2/1.3 hardening); `docker-compose.qnap.yml` port 18767 + cert volume hints
- [x] Admin UI authentication: session cookie + CSRF token — login overlay in `index.html`; auth state machine + Bearer token + sessionStorage + CSRF nonce (`X-CSRF-Token`) in `app.js`; 401 interception → re-shows login; logout flow (DELETE /auth/sessions/{id})
- [x] CORS/Origin header validation in nginx — `map $http_origin $cors_allowed` block; 403 on disallowed origins
- [x] Audit log mount (bind `/var/log/themis` as named volume) — `themis-logs:/var/log/themis:ro` on admin-ui in `docker-compose.qnap.yml`
- [x] Rate limiting for admin endpoints in nginx (`limit_req_zone`) — `zone=admin_api 30r/m` + `zone=admin_login 5r/m` (burst=10/3); HTTP 429 with JSON body
- [x] MFA enforcement for admin role — `THEMIS_MFA_REQUIRED_ROLES=admin,operator` env var hint in `docker-compose.qnap.yml`

#### 5.5.3 Phase 3 — QPKG Native Integration (Target: Q4 2026, optional)

- [ ] QPKG package wrapping ThemisDB + Admin UI
  - Inputs: QPKG build toolchain, QTS version matrix (5.x)
  - Outputs: `.qpkg` installable via QTS App Center
  - Tests: smoke install on QTS 5.1 + 5.2 test images
- [ ] Native QTS menu shortcut and inline frame embedding
- [ ] Automatic update mechanism via QPKG version check
- [ ] Dependency declaration (Container Station, qpkg.cfg)

**Acceptance Criteria (Phase 1):**
- Admin UI accessible at `http://<QNAP-IP>:18766` after `docker compose -f docker-compose.qnap.yml up -d`
- Dashboard shows live ThemisDB health and stats within 5 s
- No external JS/CSS dependencies (fully self-contained SPA)
- nginx serves static files ≤ 10 ms (P95), proxy latency adds ≤ 2 ms overhead

---

### Phase 5.5: Centralized Performance Benchmarking Framework (Q2 2026) — ✅ Complete

**Status:** Implemented and validated (2026-05-10)

Focus: Establish unified benchmark methodology and measurement infrastructure across all performance test suites.

#### 5.5.1 Benchmark Policy & Measurement Helpers ✅

**Implementation:** `tests/test_performance_helpers.h`

Centralized `BenchmarkPolicy` class providing:
- **Configurable Runs:** `independentRuns()` defaults to 5 iterations (env: `THEMIS_BENCH_RUNS`)
- **Warmup Cycles:** `warmupIterations()` defaults to 100 (env: `THEMIS_BENCH_WARMUP_ITERS`)
- **Measurement Utilities:**
  - `LatencyMeasurement`: High-resolution timer (nanosecond precision)
  - `sampleLatencyMs<Fn>()`: Template for repeated runs + percentile extraction
  - `percentileValue<T>()`: Compute p50/p95/p99 from samples

**Edition Support:**
- Community: Ethics benchmarks disabled by default (override: `-DTHEMIS_DEV_ETHICS_AI_OVERRIDE=ON`)
- Hyperscaler: Full feature set enabled with license requirement

#### 5.5.2 Benchmark Suite Integration ✅

| Suite | Tests | Status | Coverage |
|-------|-------|--------|----------|
| `SchedulerBenchmark` | 5 | ✅ PASSED | Throughput, batch scheduling, quota rejection, stats latency |
| `WirePerfBenchmark` | 9 | ✅ PASSED | Protocol metrics, pool efficiency, compression, cycle validation |
| `EthicsAIBenchmarkTests` | 6 | ✅ PASSED | SLA validation (all < documented targets) |
| `PerformanceAllocatorTest` | 1 | ✅ PASSED | Memory allocation p95 latency |
| `InferencePerformanceTest` | 14 | ✅ **EXECUTED** | Full suite (latency, throughput, memory, concurrency scaling) with metrics collection |
| **Total** | **35** | **✅ 29 PASSED + 14 EXEC** | Full coverage with 43 benchmark tests validated |

#### 5.5.3 Performance Metrics Validation ✅

**Ethics SLA Compliance (all targets met):**
```
PB01: MakeDecision (1 school)           <  500 ms ✅
PB02: MakeDecision (2 schools)          <  500 ms ✅
PB03: ComputeConfidence (100 args)      <    1 ms ✅
PB04: ComputeConsensus (100 args)       <    1 ms ✅
PB05: VectorSemanticSearch              <    5 ms ✅
PB06: BuildContext (standalone)         <    1 s  ✅
```

**Scheduler Performance (samples):**
- `getStats()` call cost: 19 ns (10,000 measurements, p95/p99 gates active)
- Throughput benchmarks use 5-run repeated sampling with warmup normalization

#### 5.5.4 Build Artifacts ✅

All benchmark binaries compile successfully in both Community and Hyperscaler editions:
- `themis_tests.exe` (aggregate, 35+ benchmark tests)
- `bench_llm_continuous_batch_scheduler.exe` (5 tests)
- `test_wire_perf_benchmark.exe` (9 tests)
- `test_ethics_ai_benchmark.exe` (6 tests)

#### 5.5.5 Acceptance Criteria ✅

- [x] Centralized policy implemented in shared header
- [x] All 5 benchmark suites integrated with policy (warmup + repeated runs)
- [x] **All 14 Inference Performance tests executed with full metrics collection**
- [x] Environment variable overrides functional (`THEMIS_BENCH_RUNS`, `THEMIS_BENCH_WARMUP_ITERS`)
- [x] 43 benchmark tests validated (29 PASSED + 14 EXECUTED with metrics)
- [x] Ethics benchmarks passing SLA validation in both editions
- [x] Inference concurrency scaling measured (1/2/4/8 threads: 683K → 1.51M tokens/sec)
- [x] Measurement methodology compliant with `PERFORMANCE_EXPECTATIONS.md`
- [x] CI-ready (env vars support for GitHub Actions/local testing)

**Results Summary:**
- Throughput scaling: 1→2→4→8 threads shows 2.2x improvement with 4 threads, plateauing at 8 threads
- Concurrent consistency: CV=0.166 (16.6% variability, acceptable for simulation)
- All 43 tests use centralized BenchmarkPolicy with deterministic warmup + repeated sampling

**Next Steps (Optional):**
- Rollout policy to additional benchmark files (index, database, storage performance suites)
- Community vs. Hyperscaler performance overhead analysis
- Integration into GitHub Actions CI pipeline

---

### Phase 6: Documentation, SDK & Ecosystem (Q2–Q4 2027) — 📋 Planned

Focus: Developer experience, official SDKs, and community ecosystem.

#### 6.1 SDKs
- [I] Python SDK from OpenAPI spec (Issue: #1507) (Target: Q2 2027)
- [I] JavaScript/TypeScript SDK (Issue: #1507) (Target: Q2 2027)
- [I] Go client library (Issue: #1507) (Target: Q2 2027)

#### 6.2 Documentation
- [I] Interactive API reference (Swagger UI / Redoc) (Target: Q2 2027)
- [I] Module-level architecture decision records (ADRs) for all 58 modules (Target: Q3 2027)
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

### Phase 7: Tensor-Native Index & Zero-Copy Inference (Q3 2026 – Q4 2027) — 📋 Planned

Focus: Tensor-Train (TT) compressed ANN indexing as a first-class SOC module
parallel to HNSW/FAISS, with a zero-copy bridge to llama.cpp for RAG/FLARE
inference and an AdaLoRA adapter sovereignty layer.

**Scientific basis:**
Oseledets 2011 (TT-SVD); Holtz et al. 2012 (TT-rounding); Malkov & Yashunin 2020 (HNSW);
Dettmers et al. 2023 (NF4); Zhang et al. 2023 (AdaLoRA); Bigoni et al. 2016 (compressed-domain queries).

**Research documentation:**
- `research/TENSOR_NETWORK_DATABASE_ARXIV_DRAFT.md`
- `research/ADALORA_TT_BRIDGE_ARXIV_DRAFT.md`
- `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`
- `research/papers/tensor_networks_themisdb.md`
- `research/best_practices/tensor_train_storage.md`

#### 7.1 Storage — Tensor-Native Storage Engine (Phase 8, Q3 2026)
- [~] `TensorTrainDecomposer` — TT-SVD (Oseledets 2011); LAPACK `dgesvd`; cuSOLVER under `THEMIS_ENABLE_CUDA` (Target: Q3 2026)
- [~] `TensorNetworkStorageEngine` — RocksDB-backed TT-core persistence; key schema `__ttn__:<tenant>:<collection>:<field>:G<k>:<version>` (Target: Q3 2026)
- [~] `TTQuantizer` — INT8/NF4 quantization of TT cores per-core channel-wise scaling (Target: Q3 2026)
- [~] `TensorRouter` — κ compressibility metric; decides TENSOR_TRAIN / HNSW / HYBRID per data profile (Target: Q3 2026)
- [~] `GgmlTensorBridge` — header spec complete; full mmap implementation (Target: Q1 2027)

#### 7.2 Tensor Index — SOC Module src/tensor/ (Phase 1 complete, Phase 2 Q4 2026)
- [x] `ITensorIndex` interface — add/search/norm/innerProduct/save/load (Target: Q3 2026)
- [x] `FlatTensorIndex` — Phase-1 linear-scan reference implementation (Target: Q3 2026)
- [x] `TensorIndexManager` — lifecycle registry, routing, tenant isolation (Target: Q3 2026)
- [x] `HnswTTBridge` — HYBRID two-layer index (HNSW nav + TT re-rank) header + skeleton (Target: Q3 2026)
- [ ] hnswlib integration in `HnswTTBridge::HnswLayer` (Target: Q4 2026)
- [ ] RocksDB persistence for `FlatTensorIndex` and `HnswTTBridge` (Target: Q4 2026)
- [ ] CMakeLists.txt `themis_tensor` library target (Target: Q4 2026)
- [ ] Test suite `tests/tensor/` — 30 unit tests TTX-01..30 (Target: Q4 2026)

#### 7.3 Query — Tensor Algebra Query Engine (Phase 9, Q4 2026)
- [~] `TensorContractionEngine` — in-compressed-domain inner-products, norms, contractions O(d·n·r³) (Target: Q4 2026)
- [~] AQL built-ins: `TENSOR_SIMILARITY`, `TENSOR_NORM`, `TENSOR_SLICE`, `TENSOR_COMPRESS` (Target: Q4 2026)
- [ ] `TensorAwareQueryOptimizer` — `TENSOR_CONTRACTION` plan-node in EXPLAIN (Target: Q1 2027)
- [~] `TensorRagCostModel` — 5-phase RAG cost model with `TENSOR_RAG` WorkloadType (Target: Q4 2026)

#### 7.4 Graph — Cross-Tensor Redundancy Mapping (Phase 8, Q2 2027)
- [~] `TensorFingerprintGraph` — Frobenius-norm-hash + MinHash 128-function LSH; CDC-changefeed integration (Target: Q2 2027)
- [~] `TensorDeduplicationManager` — single-instance TT storage; delta-TT residuals; similarity threshold 0.999 (Target: Q2 2027)

#### 7.5 Training — AdaLoRA ↔ TT Bridge (Q2–Q4 2027)
- [~] `AdaLoRATTBridge::exportLayer()` — convert AdaLoRA (B, Λ, A) triplet to TTTrain (Target: Q2 2027)
- [~] `AdaLoRATTBridge::importFromTT()` — reconstruct (B, A) from TT approximation (Target: Q2 2027)
- [ ] `AdaLoRATTBridge::findSimilarAdapters()` — wire TensorFingerprintGraph (Target: Q3 2027)
- [ ] Just-in-time adapter loading via GGML bridge null-pointer protocol (Target: Q3 2027)

#### 7.6 Boundary Analysis & Cost Model
- [x] HNSW/FAISS/TT boundary analysis: κ compressibility threshold; dim/n phase diagram (Target: Q3 2026)
- [x] RAG retrieval cost model: 5-phase C_RAG formula; TTFT comparison (150–400ms vs. 40–90ms) (Target: Q3 2026)
- [x] Research arXiv drafts: tensor networks in multi-model DBs; AdaLoRA↔TT bijection theorem (Target: Q3 2026)

---

## Production Readiness Checklist

### Per-Module Requirements (applied to all 58 modules)
- [x] Module has `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`
- [x] Current Status section with maturity indicator (Alpha / Beta / Production-ready)
- [x] Unit test coverage target defined
- [x] Integration tests implemented or planned
- [x] Performance benchmarks defined
- [x] Security audit completed or scheduled
- [x] API stability guaranteed or documented as unstable
- [x] Prometheus metrics exported where applicable

### System-Wide Requirements
- [x] All 58 modules integrated into the CMake build system
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
| 2 | acceleration | Vulkan compute shaders (distance kernels) not yet implemented | ✅ Fixed (v1.8.0) |
| 3 | chimera | Only ThemisDB self-benchmark adapter; third-party adapters pending | 📋 Planned |
| 4 | content | PDF extraction and OCR require optional third-party libraries | 📋 Planned |
| 5 | ingestion | libcurl stubs not yet replaced with real perform calls in `api_connector.cpp` | 🚧 In progress |
| 6 | ingestion | OAuth 2.0 token refresh within connectors unclear (Issue: #2408) | ❓ Unclear |
| 7 | sharding | Advanced distributed observability metrics incomplete | 🚧 In progress |
| 8 | storage | Production hardening (backup integration tests) in progress | 🚧 In progress |
| 9 | themis | Core module code still in `src/utils/` and `src/base/`; migration to `src/themis/` planned for v1.7.0 | 📋 Planned |
| 10 | config | Legacy config migration tooling not yet implemented | 📋 Planned |
| 11 | training | Multi-GPU distributed training coordination not implemented | 📋 Planned |
| 12 | prompt_engineering | Token counting / context-window budget enforcement not implemented | ✅ Done v1.7.0 |
| 13 | process | Embedding-based similarity search requires pre-computed embeddings; auto-generation not yet implemented | 🚧 In progress |
| 14 | process | BPMN parser uses regex (not DOM/SAX); deeply nested sub-process pools may not parse correctly | ⚠️ Known limitation |
| 15 | maintenance | Explicit per-task DAG dependency graph not yet implemented; tasks execute in list order | ✅ Resolved v1.2.0 |

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
- [AUDIT.md](AUDIT.md) — Security and compliance audit record
- [CHANGELOG.md](CHANGELOG.md) — Release history
- [CONTRIBUTING.md](CONTRIBUTING.md) — Contribution guidelines
- [SECURITY.md](SECURITY.md) — Security policy and vulnerability reporting
- [src/README.md](src/README.md) — Source directory overview
- [src/ROADMAP.md](src/ROADMAP.md) — Module-level roadmap index
