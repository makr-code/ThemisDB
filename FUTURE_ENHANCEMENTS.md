# ThemisDB — Open Stub Replacement Matrix

<!-- Status: current | generated: 2026-04-16 | source: src/*/FUTURE_ENHANCEMENTS.md + src/ROADMAP.md -->
<!-- Primary (Quelle der Wahrheit): src/ROADMAP.md -->
<!-- Datum: 2026-04-16 -->

> **Purpose:** This document is the canonical "Open Stub Replacement Matrix" for ThemisDB.
> It consolidates every open stub, mock, and documented simulation path across all source modules,
> extracted from `src/*/FUTURE_ENHANCEMENTS.md` and sorted by release risk.
>
> **Scope:** `src/` and `include/` modules. Plugin stubs in `plugins/` follow the same pattern
> but are tracked separately in each plugin's `FUTURE_ENHANCEMENTS.md`.
>
> **Filename convention:** `.MD` (uppercase) for root-level strategy documents.
> Module-level files use `FUTURE_ENHANCEMENTS.md` (uppercase name, lowercase extension).

---

## Table of Contents

1. [Legend and Priority System](#legend-and-priority-system)
2. [Statistics](#statistics)
3. [Implementation Phases](#implementation-phases)
4. [Wave A — Critical / Immediate (≤ v1.4.0)](#wave-a--critical--immediate--v140)
5. [Wave B — High / Near-term (v1.5.0 – v1.8.0)](#wave-b--high--near-term-v150--v180)
6. [Wave C — Medium / Long-term (v1.9.0+)](#wave-c--medium--long-term-v190)
7. [Cross-Cutting Epics](#cross-cutting-epics)
8. [Definition of Done](#definition-of-done)
9. [Governance and Tracking](#governance-and-tracking)

---

## Legend and Priority System

| Symbol | Status |
|--------|--------|
| `[ ]` | Open — not started |
| `[~]` | In Progress |
| `[x]` | Done |
| `[!]` | Blocked / needs clarification |
| `[P]` | Pull Request open |
| `[I]` | Tracking Issue open |

| Priority | Meaning |
|----------|---------|
| 🔴 Critical | Security/data-loss risk; blocks GA |
| 🟠 High | Required for production readiness; blocks next minor release |
| 🟡 Medium | Significant improvement; plan within 2 minor releases |
| 🟢 Low | Enhancement or cleanup; schedule opportunistically |

| Wave | Target Range | Calendar |
|------|-------------|----------|
| A | ≤ v1.4.0 | Q2 2026 |
| B | v1.5.0 – v1.8.0 | Q3 2026 – Q1 2027 |
| C | v1.9.0+ | Q2 2027+ |

---

## Statistics

| Category | Count |
|----------|-------|
| Total open stub/simulation items | **42** |
| 🔴 Critical (security/data-loss) | 3 |
| 🟠 High (production blockers) | 22 |
| 🟡 Medium | 12 |
| 🟢 Low / future | 5 |
| Items with linked `src/ROADMAP.md` issue | 42 |

> Full backlog (276 items incl. features): see `src/ROADMAP.md`.
> This document covers only the **stub-replacement** subset (label `stub-replacement`).

---

## Implementation Phases

Every stub replacement **must** follow these six phases before marking `[x]`:

### Phase 1 — Design / API Contract
- Define or confirm the production interface (header, namespace, method signatures).
- Identify compile-time feature gates (`THEMIS_ENABLE_*`).
- Document activation conditions that switch from stub path to production path.

### Phase 2 — Core Implementation
- Replace stub body with production logic.
- Wire real backend / SDK / service call.
- Keep the stub path available only under an explicit test-double injection point.

### Phase 3 — Error Handling and Edge Cases
- Timeout, retry, partial failure, backend unavailable.
- Structured error codes (reuse `errors.h` or module-specific error registry).
- No silent fallbacks to stub path in production builds.

### Phase 4 — Tests
- Unit tests: production path + all documented error paths.
- Integration tests: real backend or injected controlled test-double.
- Regression tests: prior stub/fallback behaviour no longer reachable in production.
- Performance gate: measured against targets stated in each item below.

### Phase 5 — Observability / Security Hardening
- Prometheus counter / gauge for the new code path.
- Structured audit log entry where the stub previously silently succeeded.
- Security review: cert validation, input validation, sandbox boundaries.

### Phase 6 — Documentation and Acceptance
- Update `src/<module>/ROADMAP.md`: mark item `[x]`.
- Update `src/<module>/AUDIT.md`: close open item.
- Update this file: mark item `[x]`.
- PR description must reference the issue from `src/ROADMAP.md`.

---

## Wave A — Critical / Immediate (≤ v1.4.0)

> Calendar: Q2 2026. These items block the v1.4.0 release or contain active security risk.

---

### A-01 · `auth` — `JWTValidator` JWKS Cache Thread-Safety
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3825
**Stub location:** `src/auth/jwt_validator.cpp` — JWKS cache updated without mutex protection.
**Risk:** Race condition on concurrent token validation → undefined behaviour, potential auth bypass.

**Affected files:**
- `src/auth/jwt_validator.cpp`
- `include/auth/jwt_validator.h`

**Implementation:**
- `[ ]` Add `mutable std::shared_mutex jwks_mutex_` to `JWTValidator`.
- `[ ]` All JWKS reads: `std::shared_lock`; all writes (refresh): `std::unique_lock`.
- `[ ]` Verify `CRYPTO_memcmp` is used for key comparison (no early-exit timing leak).

**Tests:** Multi-threaded stress test: 16 threads × 10 000 validate() calls with concurrent refresh.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#1-thread-safety-add-mutex-to-jwtvalidator-jwks-cache)

---

### A-02 · `auth` — LDAP DN and Filter Injection Prevention
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3826
**Stub location:** `src/auth/ldap_authenticator.cpp` — DN and search-filter strings assembled by naive string concatenation.
**Risk:** LDAP injection via username field → unauthorised login, directory enumeration.

**Affected files:**
- `src/auth/ldap_authenticator.cpp`
- `include/auth/ldap_authenticator.h`

**Implementation:**
- `[ ]` Implement `escapeLdapDn(input)` and `escapeLdapFilter(input)` using RFC 4515 / RFC 4514 escaping rules.
- `[ ]` Replace all string concatenation with escaped variants before passing to `ldap_search_ext_s`.
- `[ ]` Add fuzz test targeting the escape functions.

**Tests:** Unit: injection payloads `)(uid=*)(|(uid=*`, `*)(uid=*))(|(uid=*` — assert escaped safely.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#3-ldap-dn-and-filter-injection-prevention)

---

### A-03 · `auth` — Constant-Time Recovery Code and Session ID Comparison
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3833
**Stub location:** `src/auth/mfa_authenticator.cpp` line 173 — `std::find` over recovery codes returns early on first match.
**Risk:** Timing side-channel leaks which recovery code slot is valid.

**Affected files:** `src/auth/mfa_authenticator.cpp`

**Implementation:**
- `[ ]` Replace `std::find` with full-traversal loop using `CRYPTO_memcmp`; return result only after all entries checked.
- `[ ]` Apply same pattern to session-ID comparison in `rate_limiter_backend.cpp`.

**Tests:** Timing test: measure std-dev of compare latency across 10 000 trials for matching vs. non-matching codes — assert < 500 ns std-dev difference.
**Detail:** [→ src/auth/FUTURE_ENHANCEMENTS.md](src/auth/FUTURE_ENHANCEMENTS.md#4-constant-time-comparison-for-recovery-codes-and-session-ids)

---

### A-04 · `chimera` — Production ThemisDB Adapter Integration
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3842
**Stub location:** `src/chimera/themisdb_adapter.cpp` — all engine-backed paths (`query_engine_`, `vector_index_`, `graph_index_`) guarded by `NOT_IMPLEMENTED` returns when optional engine pointers are null.
**Risk:** Chimera benchmark harness silently uses in-process simulation; production integration was never exercised.

**Affected files:**
- `src/chimera/themisdb_adapter.cpp`
- `include/chimera/themisdb_adapter.hpp`

**Design Constraints (from `src/chimera/FUTURE_ENHANCEMENTS.md`):**
- No ABI-unstable breaks in `include/chimera/themisdb_adapter.hpp` without migration note.
- Feature-claims in `has_capability/get_capabilities` must match actual behaviour.
- Engine-specific paths must fail deterministically when backend is unavailable.

**Implementation:**
- `[ ]` Wire `query_engine_` to real `IQueryEngine` instance injected via `setQueryEngine()`.
- `[ ]` Wire `vector_index_` to real `IVectorIndex` via `setVectorIndex()`.
- `[ ]` Wire `graph_index_` to real `IGraphIndex` via `setGraphIndex()`.
- `[ ]` Replace `NOT_IMPLEMENTED` guards with structured `ThemisError::BackendUnavailable`.
- `[ ]` Update `has_capability()` to reflect actual engine availability at runtime.

**Tests:** Integration: inject real engine stubs via `setQueryEngine`; assert `NOT_IMPLEMENTED` error on null-engine path.
**Detail:** [→ src/chimera/FUTURE_ENHANCEMENTS.md](src/chimera/FUTURE_ENHANCEMENTS.md#production-themisdb-adapter-integration)

---

### A-05 · `chimera` — MongoDB / Qdrant / Neo4j: Replace In-Process Simulation
**Priority:** 🟠 High | **Target:** v1.2.0 | **Issue:** #3843
**Stub location:** `src/chimera/` — MongoDB, Qdrant, and Neo4j adapters use in-process hash-map simulation instead of real driver calls.

**Affected files:**
- `src/chimera/mongodb_adapter.cpp`
- `src/chimera/qdrant_adapter.cpp`
- `src/chimera/neo4j_adapter.cpp`

**Implementation:**
- `[ ]` MongoDB: gate on `THEMIS_ENABLE_MONGOCXX`; wire `mongocxx::client` session under the `IDatabaseAdapter` contract.
- `[ ]` Qdrant: gate on `THEMIS_ENABLE_QDRANT`; use gRPC client generated from `qdrant.proto`.
- `[ ]` Neo4j: gate on `THEMIS_ENABLE_NEO4J_BOLT`; use Bolt v4 C++ client.
- `[ ]` Simulation path retained and explicitly documented with `STUB/SIMULATION NOTE`.

**Tests:** Each adapter: contract test against real instance in Docker compose CI; simulation path: existing unit tests must still pass unchanged.
**Performance:** Streaming: no additional linear copy overhead per batch vs. simulation.
**Detail:** [→ src/chimera/FUTURE_ENHANCEMENTS.md](src/chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers)

---

### A-06 · `gpu` — `query_accelerator.cpp`: Replace 5 CPU Fallback Stubs
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3856
**Stub location:** `src/gpu/query_accelerator.cpp` — 5 `#ifdef THEMIS_ENABLE_CUDA` blocks contain stub comments, not real kernel dispatches.

| Line | Operation | Current | Target |
|------|-----------|---------|--------|
| 230 | Sort dispatch | CPU `std::stable_sort` | `thrust::stable_sort_by_key` |
| 277 | Sort by key | CPU fallback | Thrust device sort |
| 325 | Reduce | CPU loop | `cub::DeviceReduce::Sum/Max/Min` |
| 383 | Hash join | CPU nested loop | 2-phase GPU hash join kernel |
| 445 | BLAS matmul | CPU BLAS | `cublasSgemv` (FP32) / `cublasHgemm` (FP16) |

**Affected files:**
- `src/gpu/query_accelerator.cpp`
- `src/gpu/query_accelerator_hip.cpp` (HIP equivalents)
- `include/themis/gpu/query_accelerator.h`

**Implementation:**
- `[ ]` Sort (line 277): `#ifdef THEMIS_ENABLE_CUDA` — device alloc via `GpuMemoryManager`, `thrust::stable_sort_by_key`, device free.
- `[ ]` Reduce (line 325): `cub::DeviceReduce::Sum`/`Max`/`Min`; allocate temp storage from `GpuMemoryPool`.
- `[ ]` Hash join (line 383): build hash table on device, probe from device memory; reuse `memory_pool.cpp`.
- `[ ]` BLAS (line 445): dispatch `cublasSgemv`/`cublasHgemm`; cuBLAS handle lifecycle via `GpuModule`.
- `[ ]` Add `THEMIS_ENABLE_HIP` equivalents: `hipblas` / `rocThrust` / `hipcub`.

**Tests:** CUDA/CPU parity tests for all 5 operations at 1 K, 100 K, 10 M rows.
**Performance:**
- Sort 10 M int64: ≥ 5× vs. CPU `std::stable_sort` on RTX 3080.
- Hash join 2 × 1 M rows: ≥ 8× vs. CPU nested loop.

**Detail:** [→ src/gpu/FUTURE_ENHANCEMENTS.md](src/gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch)

---

### A-07 · `index` — GPU Vector Index: CUDA and HIP Backends
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3857
**Stub location:** `src/index/advanced_vector_index.cpp` — `#ifdef THEMIS_ENABLE_CUDA` and `#ifdef THEMIS_ENABLE_HIP` paths exist but dispatch to CPU HNSW fallback.

**Affected files:**
- `src/index/advanced_vector_index.cpp`
- `src/index/gpu_search_cuda.cpp`
- `src/index/gpu_search_hip.cpp`
- `include/index/index_manager.h`

**Implementation:**
- `[ ]` CUDA path: wire `cuVS`/`RAFT` approximate k-NN when `THEMIS_ENABLE_CUDA` + `THEMIS_ENABLE_CUVS`.
- `[ ]` HIP path: wire `rocThrust`-based k-NN when `THEMIS_ENABLE_HIP`.
- `[ ]` HIP VRAM clear validation: `hipMemset` zero-on-free in `GPUMemoryPool::release()` (#1878).
- `[ ]` GPU memory safety: validate VRAM budget before alloc; emit `gpu_oom_total` counter on rejection.

**Tests:**
- Hardware-in-the-loop tests gated on `THEMIS_GEO_CUDA=ON`.
- CPU/GPU recall parity: recall@10 ≥ 0.95 for the same dataset on both paths.
- GPU memory safety: force `cudaErrorMemoryAllocation`; assert graceful degradation to CPU.

**Detail:** [→ src/index/FUTURE_ENHANCEMENTS.md](src/index/FUTURE_ENHANCEMENTS.md#gpu-vector-index-cuda-and-hip-backend-implementation)

---

### A-08 · `geo` — CUDA and OpenCL Production Backend
**Priority:** 🟠 High | **Target:** v1.4.0 | **Issue:** #3858
**Stub location:** `src/geo/gpu_backend_stub.cpp` — `GpuBackendRegistry` entry is described as "Simple internal registry stub"; production `gpu_backend_production.cpp` is not registered.

**Affected files:**
- `src/geo/cpu_backend.cpp` (line 914: registry stub)
- `src/geo/gpu_backend_stub.cpp`
- `src/geo/gpu_backend_cuda.cu`
- `src/geo/device_detector.cpp`

**Implementation:**
- `[ ]` Register `GpuBackendRegistry` entry pointing to real `GpuBatchBackend` on startup.
- `[ ]` `GpuBatchBackend::stBuffer()` — replace CPU fallback with audit log + GPU metrics counter.
- `[ ]` `ST_UNION` / `ST_DIFFERENCE` CUDA kernels: deferred to v2.2.0; retain CPU Greiner-Hormann with explicit `STUB/SIMULATION NOTE`.
- `[ ]` Circuit-breaker on CUDA error → structured audit entry → CPU fallback (never silently).

**Tests:**
- Force `cudaErrorNoDevice` via mock; assert fallback to `boost_cpu_exact_backend.cpp` + audit entry.
- GPU contains 1 M points: ≤ 50 ms on A10G.

**Detail:** [→ src/geo/FUTURE_ENHANCEMENTS.md](src/geo/FUTURE_ENHANCEMENTS.md#cuda-and-opencl-implementation-in-gpu_backend_productioncpp)

---

### A-09 · `aql` — Post-Generation AQL Validation
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3859
**Stub location:** `src/aql/llm_aql_handler.cpp` — `translateNLToAQL()` returns LLM output without AST-level validation; invalid AQL reaches the query engine.

**Affected files:**
- `src/aql/llm_aql_handler.cpp`
- `include/aql/llm_aql_handler.h`

**Implementation:**
- `[ ]` After LLM generation, run `AQLParser::parse()` on the result.
- `[ ]` On parse error: retry with corrective prompt (max 2 retries); on persistent failure return structured `AQLError::InvalidSyntax`.
- `[ ]` Emit `aql_validation_failures_total` counter with `reason` label.

**Tests:** Unit: inject malformed AQL from mock LLM; assert retry + structured error after 2 retries.
**Detail:** [→ src/aql/FUTURE_ENHANCEMENTS.md](src/aql/FUTURE_ENHANCEMENTS.md#1--post-generation-aql-validation-in-translatenlttoaql)

---

### A-10 · `aql` — Thread Leak in `LLMTimeoutManager::executeWithTimeout()`
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3860
**Stub location:** `src/aql/llm_aql_handler.cpp` — detached thread spawned for timeout enforcement; thread outlives the manager on destruction.

**Affected files:** `src/aql/llm_aql_handler.cpp`

**Implementation:**
- `[ ]` Replace `std::thread` + `detach()` with `std::async(std::launch::async, ...)` and `std::future::wait_for()`.
- `[ ]` On timeout: set atomic cancellation flag; calling thread returns `AQLError::Timeout`.
- `[ ]` No orphaned threads: `~LLMTimeoutManager()` joins or cancels.

**Tests:** Valgrind/ASAN: no leaked threads after 1 000 timeout invocations.
**Detail:** [→ src/aql/FUTURE_ENHANCEMENTS.md](src/aql/FUTURE_ENHANCEMENTS.md#2--eliminate-thread-leak-in-llmtimeoutmanagerexecutewithtimeout)

---

## Wave B — High / Near-term (v1.5.0 – v1.8.0)

> Calendar: Q3 2026 – Q1 2027. These items are required for production hardening.

---

### B-01 · `acceleration` — CUDA Kernel Completion for Vector Similarity Search
**Priority:** 🟠 High | **Target:** v1.7.0 | **Issue:** #3863
**Stub location:** `src/acceleration/ai_hardware_dispatcher.cpp` and `src/acceleration/vllm_resource_manager.cpp` — CUDA vector similarity path (`THEMIS_ENABLE_CUDA`) dispatches to CPU HNSW fallback.

**Affected files:**
- `src/acceleration/ai_hardware_dispatcher.cpp`
- `src/acceleration/graphics_backends.cpp`

**Implementation:**
- `[ ]` Implement `CudaVectorSimilarityBackend::search()` using FAISS GPU `IndexFlatL2` / `GpuIndexIVFFlat`.
- `[ ]` Add `THEMIS_ENABLE_CUDA` compile gate; CPU path unchanged when gate is off.
- `[ ]` `VLLMResourceManager`: replace mock CPU/RAM monitoring with `sysinfo()` (Linux) / `GetSystemInfo()` (Windows).

**Tests:** Recall@10 ≥ 0.90 GPU vs. CPU on ANN-benchmarks dataset; throughput ≥ 10 000 QPS on RTX-class GPU.
**Detail:** [→ src/acceleration/FUTURE_ENHANCEMENTS.md](src/acceleration/FUTURE_ENHANCEMENTS.md#cuda-kernel-completion-for-vector-similarity-search)

---

### B-02 · `analytics` — `ExporterFactory` Stub Replacement
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3868
**Stub location:** `src/analytics/analytics_export.cpp` line 728 — `createExporter()` always returns the same stub exporter regardless of `ExportFormat`; Parquet/Feather silently unavailable.

**Affected files:** `src/analytics/analytics_export.cpp`

**Implementation:**
- `[ ]` Switch on `format`; return `ParquetExporter`, `FeatherExporter`, `CSVExporter` as appropriate.
- `[ ]` When `THEMIS_HAS_ARROW` is not defined: return `std::unexpected(ExportError::FormatUnavailable)` with clear message.
- `[ ]` Unit test: assert `createExporter(ExportFormat::FMT_ARROW_PARQUET)` returns non-stub type when `THEMIS_HAS_ARROW` is defined.

**Tests:** All 5 export formats: round-trip write/read; assert no data loss.
**Detail:** [→ src/analytics/FUTURE_ENHANCEMENTS.md](src/analytics/FUTURE_ENHANCEMENTS.md#1--exporterfactory-stub-replacement)

---

### B-03 · `analytics` — `KNNRegressorModel::predictOneReg()` Stub — ✅ Completed alongside LRModel fix
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3968

> **Status:** `KNNModel::predictOneReg` was already implemented (inverse-distance-weighted mean).
> `LRModel::predictOneReg` returned `0.0` (stub); replaced — see below.

### B-03b · `analytics` — `LRModel::predictOneReg()` Stub ✅ Done (2026-04-16)
**Priority:** 🟡 Medium | **Target:** v1.8.0
**Stub location:** `src/analytics/automl.cpp` line 832 — `return 0.0` unconditionally.

**Fix:** Compute the expected class value using `LogisticRegression::predictProbaOne(x)`:
`v = Σ c * P(class=c)` for all classes c. For binary classification this equals P(class=1).

**Tests added:** `LRModelRegressorTest::PredictOneRegNotZeroStub` and `PredictOneRegRangeMonotonic`
in `tests/analytics/test_automl.cpp`.

**Affected files:** `src/analytics/automl.cpp`

**Implementation:**
- `[ ]` Implement real k-NN regression: Euclidean distance to `k` nearest training points; weighted average of labels.
- `[ ]` Wire to existing `KNNModel` implementation already present in `automl.cpp`.

**Tests:** MAE < 0.05 on held-out synthetic dataset; performance: ≤ 1 ms per prediction at k=5, N=10 000.
**Detail:** [→ src/analytics/FUTURE_ENHANCEMENTS.md](src/analytics/FUTURE_ENHANCEMENTS.md#10--automlcpp--knnregressormodelpredictoreg-stub)

---

### B-04 · `api` — gRPC API Surface: Wire Stub Implementations
**Priority:** 🟠 High | **Target:** v2.0.0 | **Issue:** #3879
**Stub location:** `src/api/themisdb_grpc_service.cpp` and `src/api/grpc_server.cpp` — gRPC RPC methods return `grpc::Status::OK` with empty responses or `UNIMPLEMENTED`.

**Affected files:**
- `src/api/themisdb_grpc_service.cpp`
- `src/api/grpc_server.cpp`
- `include/api/themisdb_grpc_service.h`

**Implementation:**
- `[ ]` Wire each RPC method to its corresponding service handler (query engine, ingestion, admin).
- `[ ]` Propagate `ThemisError` → appropriate gRPC status code (see `grpc_error_mapper.cpp`).
- `[ ]` Add per-RPC Prometheus counters: `grpc_requests_total{method, status}`.
- `[ ]` TLS: enforce `fail_closed` (no insecure fallback); certificate hot-reload.

**Tests:** End-to-end gRPC test: `ExecuteAQL`, `IngestDocument`, `GetDocument`, `DeleteCollection` RPCs; test `UNIMPLEMENTED` RPCs no longer reachable.
**Detail:** [→ src/api/FUTURE_ENHANCEMENTS.md](src/api/FUTURE_ENHANCEMENTS.md#grpc-api-surface--wire-stub-implementations)

---

### B-05 · `content` — Abuse Detection Stub Replacement
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3889
**Stub location:** `src/content/content_security.cpp` line 150 and line 421 — abuse detection always returns `PASS`; CSAM / spam content not detected.

**Affected files:**
- `src/content/content_security.cpp`
- `include/content/ocr_processor.h` (image hash interface)

**Implementation:**
- `[ ]` Define `IAbuseDetector` interface: `detect(data, metadata) → AbuseDetectionResult{action, reason, hash}`.
- `[ ]` Implement `PhotoDNAAbuseDetector`: perceptual hash comparison against configurable blocklist YAML.
- `[ ]` Implement `TextAbuseDetector`: pattern/regex blocklist from `config/security/abuse_patterns.yaml`; `BLOCK` and `FLAG` actions.
- `[ ]` Wire both into `ContentSecurity::check()` at stub line 150.
- `[ ]` Audit log every detection via `AuditLogger::logEvent()` (content hash, detector type, action).

**Tests:** `BLOCK` path: content rejected; `FLAG` path: content stored with flag metadata. Both paths audited.
**Detail:** [→ src/content/FUTURE_ENHANCEMENTS.md](src/content/FUTURE_ENHANCEMENTS.md#abuse-detection-stub-replacement)

---

### B-06 · `governance` — OPA Adapter: HTTP Client Stub Replacement
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4004
**Stub location:** `src/governance/opa_adapter.cpp` line 218 — `OpaAdapter::evaluate()` uses a hard-coded HTTP simulation when `THEMIS_ENABLE_OPA` is not defined.

**Affected files:** `src/governance/opa_adapter.cpp`, `include/governance/opa_adapter.h`

**Implementation:**
- `[ ]` Under `THEMIS_ENABLE_OPA`: wire libcurl or gRPC call to real OPA REST API (`/v1/data/...`).
- `[ ]` On OPA unavailable: emit `governance_opa_fallback_total` counter; fall back to native evaluation with logged warning.
- `[ ]` Add mTLS option for OPA endpoint (`opa_tls_cert_path` config key).

**Tests:** Mock HTTP server returns allow/deny; integration test against real OPA in Docker CI.
**Detail:** [→ src/governance/FUTURE_ENHANCEMENTS.md](src/governance/FUTURE_ENHANCEMENTS.md)

---

### B-07 · `ingestion` — `LLMIngestionAdapter` Phase 2: Wire llama.cpp
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3904
**Stub location:** `src/ingestion/` — `LLMIngestionAdapter` Phase 1 used `NullTextGenerationBackend`; Phase 2 must wire real llama.cpp backend.

**Affected files:**
- `src/ingestion/llm_ingestion_adapter.cpp`
- `include/ingestion/inference_backend.h`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_LLAMA_CPP && THEMIS_ENABLE_LLM`.
- `[ ]` Inject real `ITextGenerationBackend` backed by `LlamaCppPlugin`.
- `[ ]` `NullTextGenerationBackend` remains available only under the `STUB/SIMULATION NOTE` contract for test injection.
- `[ ]` Batch size and timeout configurable via `ingestion_config.yaml`.

**Tests:**
- Unit: inject `NullTextGenerationBackend`; assert ingestion completes without LLM.
- Integration: tiny GGUF model in CI; assert non-empty entity extraction result.

**Detail:** [→ src/ingestion/FUTURE_ENHANCEMENTS.md](src/ingestion/FUTURE_ENHANCEMENTS.md#llmingestionadapter-phase-2-wire-llamacpp)

---

### B-08 · `ingestion` — Connector Mock Paths: Production Wiring
**Priority:** 🟡 Medium | **Target:** v1.7.0 – v1.8.0 | **Issues:** multiple
**Stub locations:**

| Connector | File | Stub Location | Production Target |
|-----------|------|---------------|-------------------|
| S3 | `src/ingestion/s3_connector.cpp:326` | `ingestFromMock()` path | AWS SDK `s3_client->GetObject()` |
| S3 | `src/ingestion/s3_connector.cpp:492` | Listing stub | AWS SDK `ListObjectsV2` |
| Kafka | `src/ingestion/kafka_connector.cpp:238` | `ingestFromMock()` | librdkafka `RdKafka::Consumer` |
| Object Storage | `src/ingestion/object_storage_connector.cpp:273` | Mock path | GCS / Azure SDK |
| Database | `src/ingestion/database_connector.cpp:458` | Mock ODBC path | Real ODBC via `THEMIS_ENABLE_ODBC` |
| CDC | `src/ingestion/cdc_connector.cpp:563` | Mock CDC path | Debezium / real DB WAL |

**Implementation per connector:**
- `[ ]` S3: wire under `THEMIS_ENABLE_AWS_SDK`; mock-injection path retained for unit tests.
- `[ ]` Kafka: wire under `THEMIS_ENABLE_KAFKA`; mock-injection path retained for unit tests.
- `[ ]` Object Storage: wire under `THEMIS_ENABLE_GCS` / `THEMIS_ENABLE_AZURE`.
- `[ ]` Database: wire full ODBC under `THEMIS_ENABLE_ODBC`.
- `[ ]` CDC: wire Debezium events or WAL-based CDC under `THEMIS_ENABLE_CDC`.
- `[ ]` All mock paths annotated with `STUB/SIMULATION NOTE` (already done for format; verify completeness).

**Tests:** Each connector: 28–32 unit tests via mock-injection (no cloud credentials required); Docker-compose integration tests for S3 (MinIO) and Kafka.

---

### B-09 · `llm` — `LoraSecurityValidator`: Certificate Store Integration
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3906
**Stub location:** `src/llm/multi_lora_manager.cpp` line 392 and 462 — `LoraSecurityValidator` validates LoRA adapter files but the certificate chain is verified against a hard-coded base64 hash comparison, not a real cert store.

**Affected files:**
- `src/llm/multi_lora_manager.cpp`
- `include/llm/lora_framework/lora_metrics.h`

**Implementation:**
- `[ ]` Integrate OpenSSL `X509_STORE` for certificate chain validation.
- `[ ]` Load trusted CA bundle from `config/security/lora_trusted_cas.pem` (configurable path).
- `[ ]` On cert validation failure: reject LoRA load; emit `lora_cert_rejected_total` counter.
- `[ ]` CRL check: use OCSP stapling when available; fall back to CRL distribution point.

**Tests:** Valid cert chain → load succeeds. Expired / revoked cert → load rejected + counter incremented.
**Detail:** [→ src/llm/FUTURE_ENHANCEMENTS.md](src/llm/FUTURE_ENHANCEMENTS.md#lorasecurityvalidator-certificate-store-integration)

---

### B-10 · `llm` — `LLMDeploymentPlugin`: RocksDB Model Storage
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4011
**Stub location:** `src/llm/lora_framework/lora_storage_service_themisdb.cpp` — model metadata persisted in in-memory map; survives only for process lifetime.

**Affected files:**
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
- `include/llm/active_vram_allocator.h`

**Implementation:**
- `[ ]` Replace in-memory map with RocksDB KV store keyed by model ID.
- `[ ]` Atomic write via `rocksdb::WriteBatch`.
- `[ ]` On restart: restore map from RocksDB scan.

**Tests:** Crash recovery: write 10 models, kill process, restart, assert all 10 recoverable.
**Detail:** [→ src/llm/FUTURE_ENHANCEMENTS.md](src/llm/FUTURE_ENHANCEMENTS.md#llmdeploymentplugin-rocksdb-model-storage)

---

### B-11 · `llama_cpp` — Real `generate()` Inference via LlamaWrapper
**Priority:** 🟠 High | **Target:** Q3 2026 | **Issue:** (llama_cpp module)
**Stub location:** `src/llama_cpp/llama_cpp_plugin.cpp` — `generate()` returns echo stub; `embed()` returns 384-dim zero vector.

**Affected files:**
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `include/llama_cpp/llama_cpp_plugin.h`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_LLAMA_CPP`.
- `[ ]` `generate()`: delegate to `LlamaWrapper::generate()`; increment `inference_count_`.
- `[ ]` `embed()`: delegate to `LlamaWrapper::embed()`; return real 384-dim (or model-defined-dim) vector.
- `[ ]` Stub mode (`loadModel("")`) must remain functional; all stub paths annotated.
- `[ ]` `exportLoRA()` / `importLoRA()`: serialize/deserialize LoRA weights (GGUF-compatible); magic-byte validation before deserialisation.

**Tests:** Integration test with tiny GGUF model in CI fixtures. Perf: ≤ 200 ms for 50-token prompt on RTX 3090 equivalent.
**Detail:** [→ src/llama_cpp/FUTURE_ENHANCEMENTS.md](src/llama_cpp/FUTURE_ENHANCEMENTS.md#1-real-llamacpp-inference-via-llamawrapper-target-q3-2026)

---

### B-12 · `query` — `QueryOptimizer`: Wire Real MetadataShard and Statistics
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3918
**Stub location:** `src/query/query_optimizer.cpp` — `MetadataShard`, Prometheus handle, and column statistics are injected as null/mock objects; cost model uses constant estimates.

**Affected files:**
- `src/query/query_optimizer.cpp`
- `include/query/query_optimizer.h`

**Implementation:**
- `[ ]` Accept `IMetadataShard*` via constructor; assert non-null in production builds.
- `[ ]` Pull column statistics (cardinality, min/max/histogram) from `MetadataShard::getStats()`.
- `[ ]` Emit `query_optimizer_plan_cost_estimate` gauge to Prometheus.

**Tests:** Cost-based join order test: 3-table join; assert optimizer chooses lower-cost plan when statistics favour it.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#queryoptimizer-wire-real-metadatashard-prometheus-and-statistics)

---

### B-13 · `query` — `QueryFederation`: Real Shard Determination Logic
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3919
**Stub location:** `src/query/cte_subquery.cpp` — `QueryFederation::determineShard()` broadcasts to all shards regardless of query predicates.

**Affected files:**
- `src/query/cte_subquery.cpp`
- `include/query/cross_cluster_federation.h`

**Implementation:**
- `[ ]` Implement shard-key extraction from `WHERE` predicates.
- `[ ]` Map shard-key value to shard ID via consistent-hash ring from `IShardManager`.
- `[ ]` Fan-out only to relevant shards; merge results with `MergeSort`.

**Tests:** Shard routing test: query with exact shard-key match routes to exactly 1 shard; range query routes to correct subset.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#queryfederation-real-shard-determination-logic)

---

### B-14 · `query` — `CTESubquery`: Replace Phase 1 Stub
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #4025
**Stub location:** `src/query/cte_subquery.cpp` — CTE materialisation Phase 1 caches results in `std::unordered_map<string, json>`; no spill-to-disk, no streaming.

**Affected files:** `src/query/cte_subquery.cpp`, `include/query/cte_subquery.h`

**Implementation:**
- `[ ]` Spill to RocksDB when in-memory size exceeds `cte_memory_limit_mb` config.
- `[ ]` Streaming CTE: yield rows incrementally instead of materialising full result.

**Tests:** 10 M row CTE: assert spill occurs; result identical to in-memory path.
**Detail:** [→ src/query/FUTURE_ENHANCEMENTS.md](src/query/FUTURE_ENHANCEMENTS.md#ctesubquery-replace-phase-1-stub)

---

### B-15 · `rag` — `LLMIntegration` / `LLMJudgeIntegration`: Replace Mock Mode
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3925
**Stub location:** `src/rag/llm_judge_integration.cpp` — mock mode returns fixed evaluation scores; used in production RAG evaluation when real LLM is unavailable.

**Affected files:**
- `src/rag/llm_judge_integration.cpp`
- `include/rag/llm_judge_integration.h`
- `include/rag/hybrid_retriever.h`

**Implementation:**
- `[ ]` Remove implicit mock-mode fallback from production build path.
- `[ ]` Provide explicit `LLMJudgeMock` class injectable only in tests.
- `[ ]` Production path: call real `ILLMPlugin::generate()` via `LLMIntegration`.
- `[ ]` On LLM unavailable: return `RAGError::JudgeUnavailable` (not silent mock scores).

**Tests:** Integration: real LLM client via `openai_compat_adapter.cpp`; assert non-trivial scores. Unit: inject `LLMJudgeMock`.
**Detail:** [→ src/rag/FUTURE_ENHANCEMENTS.md](src/rag/FUTURE_ENHANCEMENTS.md#llmintegration-and-llmjudgeintegration-replace-stubmock-mode-with-real-engine)

---

### B-16 · `security` — `ArrowUserRegistrationPlugin`: Implement Apache Arrow Integration
**Priority:** 🟠 High | **Target:** v1.8.0 | **Issue:** #3930
**Stub location:** `src/security/` — `ArrowUserRegistrationPlugin` is a complete stub; user store backed by in-memory vector.

**Affected files:**
- `src/security/hsm_provider.cpp` (lines 32+)
- `src/security/hsm_provider_pkcs11.cpp` (lines 56+)
- `src/security/timestamp_authority.cpp` (lines 28+)

**Implementation:**
- `[ ]` Arrow plugin: back user store with Apache Arrow `RecordBatch`; persist via `ArrowFileWriter`.
- `[ ]` HSM provider: wire to real PKCS#11 token under `THEMIS_ENABLE_HSM`; stub path annotated with `STUB/SIMULATION NOTE`.
- `[ ]` Timestamp authority: wire to real RFC 3161 TSA HTTP endpoint; stub path annotated.
- `[ ]` PKIClient stub (`src/utils/pki_client.cpp`): replace base64-hash fallback with real X.509 certificate verification via OpenSSL.

**Tests:** HSM: SoftHSM2 in CI Docker; Arrow: round-trip 1 000 users, restart, verify.
**Detail:** [→ src/security/FUTURE_ENHANCEMENTS.md](src/security/FUTURE_ENHANCEMENTS.md#arrowuserregistrationplugin-implement-apache-arrow-integration)

---

### B-17 · `server` — `HttpServer`: Initialize Real `ShardingManager`
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4032
**Stub location:** `src/server/grpc_web_proxy_handler.cpp` and `src/api/http_server.cpp` — `ShardingManager` pointer is null; shard-aware routing is skipped.

**Affected files:** `src/api/http_server.cpp`, `src/server/`

**Implementation:**
- `[ ]` Inject real `IShardingManager` via `HttpServer::setShardingManager()`.
- `[ ]` Route shard-key requests to correct shard; non-shard requests to local handler.

**Tests:** Integration: 3-shard cluster; key-based request routes to correct shard.
**Detail:** [→ src/server/FUTURE_ENHANCEMENTS.md](src/server/FUTURE_ENHANCEMENTS.md#httpserver-initialize-real-shardingmanager)

---

### B-18 · `sharding` — `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3936
**Stub location:** `src/sharding/cloud_backup.cpp` lines 70, 220, 326 — `GpuErasureCoderOpenCL::encode()` / `decode()` / `repair()` return CPU-fallback no-ops.

**Affected files:** `src/sharding/cloud_backup.cpp`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_OPENCL`.
- `[ ]` Implement Reed-Solomon encode/decode/repair using OpenCL kernels.
- `[ ]` CPU path retained as `GpuErasureCoder` default when OpenCL unavailable.

**Tests:** Erasure encode + introduce 2 shard failures + repair: assert bitwise identical recovery.
**Detail:** [→ src/sharding/FUTURE_ENHANCEMENTS.md](src/sharding/FUTURE_ENHANCEMENTS.md#gpuerasurecoderopencl-implement-opencl-encodedecode)

---

### B-19 · `training` — `ProvenanceTracker`: Replace AQL Template Stubs
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #3951
**Stub location:** `src/training/provenance_tracker.cpp` — AQL queries templated with placeholder collection names; `knowledge_graph_enricher.cpp` uses hard-coded stub enrichment.

**Affected files:**
- `src/training/provenance_tracker.cpp`
- `src/training/knowledge_graph_enricher.cpp`

**Implementation:**
- `[ ]` Wire `ProvenanceTracker` to real `IQueryExecutor` via constructor injection.
- `[ ]` Collection names resolved from `TrainingConfig::provenance_collection`.
- `[ ]` `KnowledgeGraphEnricher`: wire to real `IGraphWriter` from `IngestionToolbox`.

**Tests:** Integration: ingest 5 training examples, enrich graph, query provenance; assert graph edges exist.
**Detail:** [→ src/training/FUTURE_ENHANCEMENTS.md](src/training/FUTURE_ENHANCEMENTS.md#provenancetracker-replace-aql-template-stubs-with-live-connection)

---

### B-20 · `utils` — `PKIClient`: Replace Fallback Stub Verification
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** #4049
**Stub location:** `src/utils/pki_client.cpp` — certificate verification falls back to base64 hash comparison when OpenSSL verification fails; this is a stub, not a secure fallback.

**Affected files:** `src/utils/pki_client.cpp`, `include/utils/pki_client.h`

**Implementation:**
- `[ ]` Remove base64-hash fallback from production build.
- `[ ]` On OpenSSL verification failure: return `PKIError::VerificationFailed` (hard error).
- `[ ]` CRL / OCSP check added to `PKIClient::verify()`.

**Tests:** Self-signed cert (no trusted CA): verify returns error, not fallback success.
**Detail:** [→ src/utils/FUTURE_ENHANCEMENTS.md](src/utils/FUTURE_ENHANCEMENTS.md#pkiclient-replace-fallback-stub-verification)

---

### B-21 · `performance` — Advanced Cache Manager Stub
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** (performance module)
**Stub location:** `src/performance/advanced_cache_manager.cpp` line 92 — ML-based prefetch predictor returns fixed access pattern.

**Affected files:** `src/performance/advanced_cache_manager.cpp`

**Implementation:**
- `[ ]` Replace fixed pattern with sliding-window access frequency model.
- `[ ]` Gate on `THEMIS_ENABLE_ML_CACHE`; fixed pattern retained as compile-time fallback.

**Tests:** Prefetch hit rate ≥ 60% on realistic access trace.
**Detail:** [→ src/performance/FUTURE_ENHANCEMENTS.md](src/performance/FUTURE_ENHANCEMENTS.md#advanced-cache-optimization)

---

### B-22 · `content` — `TextProcessor::generateEmbedding` Stub
**Priority:** 🟡 Medium | **Target:** v1.8.0 | **Issue:** (content module)
**Stub location:** `src/content/text_processor.cpp` line 207 — `generateEmbedding()` returns a random or zero vector.

**Affected files:** `src/content/text_processor.cpp`

**Implementation:**
- `[ ]` Gate on `THEMIS_ENABLE_EMBEDDING`.
- `[ ]` Delegate to `IEmbeddingBackend` injected via `TextProcessor::setEmbeddingBackend()`.
- `[ ]` Default to zero-vector stub only when backend is null (test-only path).

**Tests:** Cosine similarity between related documents > 0.7; unrelated documents < 0.3.
**Detail:** [→ src/content/FUTURE_ENHANCEMENTS.md](src/content/FUTURE_ENHANCEMENTS.md#embedding-generation-pipeline-text--vector)

---

## Wave C — Medium / Long-term (v1.9.0+)

> Calendar: Q2 2027+. Scheduled opportunistically; no release-blocking status.

---

### C-01 · `security` — SPHINCS+ Production Implementation (liboqs)
**Priority:** 🟡 Medium | **Target:** v2.0.0+
**Stub location:** `src/security/post_quantum_crypto.cpp` lines 923–1000 — `SphincsPlus` class uses OpenSSL Ed25519 simulation for SPHINCS+-SHA2-256s/256f; documented `STUB/SIMULATION NOTE`.
**Activation:** Replace when `liboqs` ≥ 0.10.0 is available in `vcpkg.json`.

**Implementation:**
- `[ ]` Add `liboqs` to `vcpkg.json` and `CMakeLists.txt` under `THEMIS_ENABLE_LIBOQS`.
- `[ ]` Replace `OQS_SIG_sphincs_sha2_256s_sign()` / `verify()` with real liboqs calls.
- `[ ]` Retain Ed25519 simulation under `STUB/SIMULATION NOTE` for environments without liboqs.

**Tests:** CPU/liboqs sign+verify parity; known-answer tests from NIST PQC test vectors.

---

### C-02 · `acceleration` — OpenGL Compute Shader Backend: 5 Remaining Stubs
**Priority:** 🟢 Low | **Target:** v2.0.0 | **Issue:** #4065
**Stub location:** `src/acceleration/graphics_backends.cpp` — OpenGL compute shader backend has 5 unimplemented kernels.

**Implementation:**
- `[ ]` Implement 5 compute shaders (`vector_add`, `matrix_mul`, `reduce_sum`, `scan`, `sort`) in GLSL 4.3 compute.
- `[ ]` Gate on `THEMIS_ENABLE_OPENGL`.

---

### C-03 · `analytics` — Windows Platform Stubs
**Priority:** 🟢 Low | **Target:** v2.0.0 | **Issue:** #4068
**Stub location:** `src/analytics/olap.cpp:53` and `src/analytics/process_mining.cpp:24` — `#if defined(_WIN32)` stubs emit `spdlog::error` but return empty results.

**Implementation:**
- `[ ]` Port SIMD paths to Windows-compatible intrinsics (`<intrin.h>`).
- `[ ]` Add Windows CI job; set stub-count gate ≤ 0 for non-Windows builds.

---

### C-04 · `whisper` — Real Diarisation Backend
**Priority:** 🟢 Low | **Target:** v2.1.0+
**Stub location:** `src/whisper/whisper_plugin.cpp` line 39 — diarisation returns preset stub fixtures.

**Implementation:**
- `[ ]` Integrate `pyannote.audio` inference via subprocess or embedded Python.
- `[ ]` Gate on `THEMIS_ENABLE_DIARISATION`.

**Tests:** 5 unit tests with stub returning preset fixtures (keep); integration test with real audio file.

---

### C-05 · `performance` — Phase 4 PMU Counters: Non-Linux Stub Coverage
**Priority:** 🟢 Low | **Target:** v1.9.0 | **Issue:** #4086
**Stub location:** `src/performance/phase4/pmu_counters.cpp` — PMU counter reads return 0 on non-Linux platforms.

**Implementation:**
- `[ ]` macOS: use `kperf` framework for cycle / instruction counters.
- `[ ]` Windows: use `QueryThreadCycleTime` / `QueryPerformanceCounter`.

---

### C-06 · `llm` — `VisionEncoder`: Checksum Verification ✅ Done (2026-04-16)
**Priority:** 🟢 Low | **Was:** `src/llm/vision_encoder.cpp` line 117 — `// TODO: Implement checksum verification`.

**Fix:** Implemented SHA-256 sidecar file verification using `ModuleHashVerifier::computeSHA256()`.
Convention: `<model_path>.sha256` sidecar contains the expected hex digest.
On mismatch: `std::runtime_error` with file path and both hashes.
On missing sidecar: warns and continues (non-fatal, matches existing behaviour when verification config is absent).
Gate: only runs when `config_->isModelVerificationEnabled()` && `mv.verify_checksums` are both true.

---

## Cross-Cutting Epics

### Epic: Stub/Mock Replacement
**Label:** `epic:stub-replacement` | **Target:** v1.8.0
Covers all items with label `stub-replacement` in `src/ROADMAP.md`.
Most critical items: A-06 (GPU sort/join/BLAS), A-07 (index GPU), A-08 (geo GPU), B-04 (gRPC), B-05 (abuse detection), B-07 (LLM ingestion), B-09 (LoRA certs), B-15 (RAG mock).

### Epic: Security Hardening
**Label:** `epic:security-hardening` | **Target:** v1.8.0
Affects: `auth`, `security`, `server`, `llm`, `utils`, `sharding`, `storage`.
Critical items: A-01 (JWT mutex), A-02 (LDAP injection), A-03 (timing), B-09 (LoRA certs), B-16 (Arrow/HSM), B-20 (PKI stub).

### Epic: GPU Compute
**Label:** `epic:gpu-compute` | **Target:** v1.7.0 – v1.8.0
Covers all GPU backend stubs: A-06, A-07, A-08, B-01, B-18.
All GPU items require `THEMIS_ENABLE_CUDA` / `THEMIS_ENABLE_HIP` / `THEMIS_ENABLE_OPENCL` gates.
CPU fallback must remain available and tested independently.

### Epic: Thread-Safety
**Label:** `epic:thread-safety` | **Target:** v1.8.0
All exclusive-mutex read paths upgraded to `std::shared_mutex`:
`analytics` (#41–45), `plugins` (#193), `maintenance` `schedules_mutex_` (#185), `graph` `DistributedGraphManager` (#174), `config` `ConfigEncryptedStore` (#164).

---

## Definition of Done

A stub replacement item is **Done** (`[x]`) when ALL of the following are true:

1. **No stub in production path**: the `#ifdef`-guarded or null-check stub body is unreachable in a production build (i.e., with all feature gates enabled).
2. **Stub retained for tests only**: where a controlled test-double is needed, it is injectable via constructor/setter and annotated with `STUB/SIMULATION NOTE:`.
3. **Tests green**: all Phase 4 tests pass in CI (unit + integration + regression).
4. **Performance gate met**: measured metric meets the target stated in this document.
5. **Observability added**: at least one Prometheus counter or gauge for the new production path.
6. **No new security vulnerabilities**: `parallel_validation` (CodeQL + Code Review) passes.
7. **Docs updated**: `src/<module>/ROADMAP.md` item marked `[x]`; `src/<module>/AUDIT.md` open item closed; this file updated.

---

## Governance and Tracking

| Document | Purpose |
|----------|---------|
| `src/ROADMAP.md` | Master backlog (276 items, all modules) |
| `src/<module>/FUTURE_ENHANCEMENTS.md` | Module-level detail, acceptance criteria, API sketches |
| `src/<module>/AUDIT.md` | Per-module audit trail, open items, security findings |
| `FUTURE_ENHANCEMENTS.md` (this file) | Root-level stub replacement matrix, wave prioritisation |
| GitHub Issues `#3825–#4092` | One issue per backlog item; label `stub-replacement` |

**Issue template:**
```
## Summary
<Item title from this matrix>

## Module
`src/<module>/`

## Priority / Target Version
<Wave> · <Priority> · <Target Version>

## Stub Location
<file>:<line> — <description>

## Acceptance Criteria
- [ ] (copy `- [ ]` items from this matrix)

## Labels
stub-replacement, module:<name>, <priority-label>
```

**Release gate:** No `🔴 Critical` or Wave-A items may remain open when cutting a minor release.

---

*Last updated: 2026-04-16 | Generated from: `src/ROADMAP.md` + `src/*/FUTURE_ENHANCEMENTS.md`*
