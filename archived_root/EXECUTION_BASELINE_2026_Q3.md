# ThemisDB Execution Baseline — Q3 2026 Consolidation

**Status:** Draft Canonical Execution Board  
**Created:** 2026-07-17  
**Scope:** Align Wave A (v1.1.0-v1.2.0) and Wave B (v1.5.0-v1.8.0) delivery against production readiness gates  
**Governance:** Implements BRANCHING_STRATEGY.md (develop→edition branches), RELEASE_STRATEGY.md (milestone/tag mapping)

---

## Overview

This document consolidates open backlog items from:
- **FUTURE_ENHANCEMENTS.md** — Root stub/simulation replacement matrix (Waves A + B)
- **ROADMAP.md** — System readiness gates, EPIC phases, production milestones
- **src/*/ROADMAP.md + src/*/FUTURE_ENHANCEMENTS.md** — Module-level open items

**Objective:** Create one canonical execution board (no duplicates, no planning drift) with clear release targeting and delivery phase sequencing.

---

## Delivery Strategy

### Release Governance
- **Target Branch:** `develop` (integration, default for all new work)
- **Edition Promotion:** Via RELEASE_STRATEGY.md milestone→tag mapping
  - Minimal: `minimal-vX.Y.Z(-prerelease)`
  - Community: `vX.Y.Z(-alphaN|-betaN|-rcN)`
  - Enterprise: `enterprise-vX.Y.Z(-rcN)`
  - Hyperscaler: `hyperscaler-vX.Y.Z(-rcN)`
  - Military: `military-vX.Y.Z(-rcN)`

### Quality Gates
- **Production readiness:** No silent fallbacks to CPU/simulation paths
- **Test coverage:** Unit + integration + property-based for all stub replacements
- **Security review:** Required for auth, crypto, ingestion adapters
- **Performance gating:** Measured against Phase 5-7 benchmark baseline (benchmarks/wave7/)

---

## WAVE 1 — Release-Blocking (Q3 2026, Target: v1.1.0–v1.2.0)

**Rationale:** These items contain **active security risk** or **simulation-path critical** issues that must be resolved before production GA.

### A-01 · `auth` — JWT JWKS Cache Thread-Safety  
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3825  
**Type:** Security (race condition)  
**Scope:** `src/auth/jwt_validator.cpp` — Add `std::shared_mutex` for concurrent token validation  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: `shared_mutex` lock contract documented
- [ ] Implementation: JWKS cache guarded by `shared_lock` (read) / `unique_lock` (write)
- [ ] Tests: 16-thread stress × 10K validate() calls with concurrent refresh
- [ ] Security: `CRYPTO_memcmp` used for key comparison (no early-exit timing leak)
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-02 · `auth` — LDAP DN and Filter Injection Prevention  
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3826  
**Type:** Security (LDAP injection)  
**Scope:** `src/auth/ldap_authenticator.cpp` — RFC 4515/4514 escaping for DN and search filters  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: `escapeLdapDn()` and `escapeLdapFilter()` functions per RFC
- [ ] Implementation: String concatenation → escaped variants
- [ ] Tests: Fuzz test with injection payloads; unit tests for escape correctness
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-03 · `auth` — Constant-Time Recovery Code / Session ID Comparison  
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3833  
**Type:** Security (timing side-channel)  
**Scope:** `src/auth/mfa_authenticator.cpp`, `src/auth/rate_limiter_backend.cpp`  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Full-traversal loop with `CRYPTO_memcmp` (no early exit)
- [ ] Implementation: Replace `std::find()` across both files
- [ ] Tests: Timing test with std-dev < 500ns for all code paths
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-04 · `chimera` — Production ThemisDB Adapter Integration  
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3842  
**Type:** Feature (simulation→production)  
**Scope:** `src/chimera/themisdb_adapter.cpp` — Wire real query/vector/graph engines  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Injection contract for `setQueryEngine()`, `setVectorIndex()`, `setGraphIndex()`
- [ ] Implementation: Replace `NOT_IMPLEMENTED` guards with `ThemisError::BackendUnavailable`
- [ ] Tests: Integration test with real engine stubs; verify null-engine path fails deterministically
- [ ] Tests: Verify `has_capability()` reflects actual engine availability at runtime
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-05 · `chimera` — MongoDB / Qdrant / Neo4j: Replace In-Process Simulation  
**Priority:** 🟠 High | **Target:** v1.2.0 | **Issue:** #3843  
**Type:** Feature (simulation→production)  
**Scope:** `src/chimera/{mongodb,qdrant,neo4j}_adapter.cpp` — Feature-gated real driver calls  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Feature gates: `THEMIS_ENABLE_MONGOCXX`, `THEMIS_ENABLE_QDRANT`, `THEMIS_ENABLE_NEO4J_BOLT`
- [ ] Implementation: MongoDB (`mongocxx::client`), Qdrant (gRPC), Neo4j (Bolt v4)
- [ ] Simulation: Retained path with explicit `STUB/SIMULATION NOTE` marker (purpose, activation, delta, removal target)
- [ ] Tests: Contract test against real Docker-compose instance + existing unit tests (simulation path)
- [ ] Performance: Verify no additional linear copy overhead per batch vs. simulation
- [ ] PR: Linked to ROADMAP.md v1.2.0 milestone

---

### A-06 · `gpu` — `query_accelerator.cpp`: Replace 5 CPU Fallback Stubs  
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3844  
**Type:** Feature (CPU fallback→GPU dispatch)  
**Scope:** `src/gpu/query_accelerator.cpp` — 5 functions with CPU fallback stubs  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Identify 5 stub functions; document GPU dispatch contract
- [ ] Implementation: Replace stubs with real CUDA/HIP kernel dispatch
- [ ] Tests: GPU availability check + fallback error handling when no GPU
- [ ] Performance: Measure speedup vs CPU baseline; gate on ≥2x (typical for these ops)
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-07 · `index` — GPU Vector Index: CUDA and HIP Backends  
**Priority:** 🟠 High | **Target:** v1.2.0 | **Issue:** #3845  
**Type:** Feature (backend dispatch)  
**Scope:** `src/index/gpu_vector_index.cpp` — CUDA and HIP production backends  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Backend selection contract; performance targets (p99 latency, throughput gates)
- [ ] Implementation: CUDA kernel library + HIP portability layer
- [ ] Tests: Cross-platform GPU parity test (CUDA vs HIP results match); fallback error paths
- [ ] Performance: Measure p99 distance computation latency; gate on baseline target from Wave 7 benchmarks
- [ ] PR: Linked to ROADMAP.md v1.2.0 milestone

---

### A-08 · `geo` — CUDA and OpenCL Production Backend  
**Priority:** 🟠 High | **Target:** v1.2.0 | **Issue:** #3846  
**Type:** Feature (backend dispatch)  
**Scope:** `src/geo/` — CUDA and OpenCL production backends for geospatial queries  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Backend selection; distance/containment kernel contract
- [ ] Implementation: CUDA kernel library + OpenCL kernel library
- [ ] Tests: Cross-platform correctness test (CPU vs GPU results match); edge cases (antipodes, equator crossing)
- [ ] Performance: Measure throughput for batch distance computation; gate on baseline from benchmarks/wave7/
- [ ] PR: Linked to ROADMAP.md v1.2.0 milestone

---

### A-09 · `aql` — Post-Generation AQL Validation  
**Priority:** 🟠 High | **Target:** v1.1.0 | **Issue:** #3847  
**Type:** Feature (safety hardening)  
**Scope:** `src/aql/` — Validate generated AQL before execution  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Validation contract (syntax, semantic checks, timeout policy)
- [ ] Implementation: Post-generation validator before executor hand-off
- [ ] Tests: Malformed query rejection; valid query acceptance; timeout enforcement
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

### A-10 · `aql` — Thread Leak in `LLMTimeoutManager::executeWithTimeout()`  
**Priority:** 🔴 Critical | **Target:** v1.1.0 | **Issue:** #3848  
**Type:** Bug (resource leak, thread lifecycle)  
**Scope:** `src/aql/llm_timeout_manager.cpp` — Fix thread lifecycle management  
**Acceptance:**
- [x] Issue tracking created
- [ ] Design: Thread lifecycle contract; cancellation semantics
- [ ] Implementation: Fix thread leak (likely missing `detach()` or `join()` path)
- [ ] Tests: Run executor 1K times under memory leak detector (valgrind/AddressSanitizer); verify no thread accumulation
- [ ] PR: Linked to ROADMAP.md v1.1.0 milestone

---

## WAVE 2 — Near-Term Hardening (Q3 2026 – Q1 2027, Target: v1.5.0–v1.8.0)

**Rationale:** Production integration and module-level stub completions; enables feature delivery and operational scaling.

### B-01 · `acceleration` — CUDA Kernel Completion for Vector Similarity Search  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3849  
**Type:** Feature  
**Acceptance:** [ ] Kernel library complete; [ ] Parity with CPU path; [ ] Integration tests

---

### B-02 · `analytics` — `ExporterFactory` Stub Replacement  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3850  
**Type:** Feature (✅ verified 2026-06-18)  
**Status:** ✅ DONE  

---

### B-04 · `api` — gRPC API Surface: Wire Stub Implementations  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3851  
**Type:** Feature (API completeness)  
**Scope:** `src/api/` — Full gRPC handler wiring (Query, Insert, Update, Delete, etc.)  
**Acceptance:**
- [ ] Design: Handler contract per gRPC service
- [ ] Implementation: Wire all stub handlers to production paths
- [ ] Tests: E2E gRPC call flow for all major operations
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

### B-07 · `ingestion` — `LLMIngestionAdapter` Phase 2: Wire llama.cpp  
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3852  
**Type:** Feature (LLM integration)  
**Scope:** `src/ingestion/llm_ingestion_adapter.cpp` — Real llama.cpp inference wiring  
**Acceptance:**
- [ ] Design: Model loading and inference contract
- [ ] Implementation: Replace mock with real llama.cpp backend
- [ ] Tests: Integration test with real model; fallback error paths
- [ ] PR: Linked to ROADMAP.md v1.6.0 milestone

---

### B-08 · `ingestion` — Connector Mock Paths: Production Wiring  
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3853  
**Type:** Feature (connector wiring)  
**Scope:** `src/ingestion/` — S3, Kafka, Object Storage, ODBC, CDC connectors  
**Acceptance:**
- [ ] Design: Connector interface contract per backend
- [ ] Implementation: Replace mock paths with real SDK calls
- [ ] Tests: Integration test against real services (Docker compose)
- [ ] Performance: Throughput gating per connector type
- [ ] PR: Linked to ROADMAP.md v1.6.0 milestone

---

### B-09 · `llm` — `LoraSecurityValidator`: Certificate Store Integration  
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3854  
**Type:** Feature (security)  
**Scope:** `src/llm/lora_security_validator.cpp` — Real certificate store integration  
**Acceptance:**
- [ ] Design: Certificate validation contract
- [ ] Implementation: Replace mock with system cert store / custom validator
- [ ] Tests: Certificate chain validation tests; revocation scenarios
- [ ] PR: Linked to ROADMAP.md v1.6.0 milestone

---

### B-10 · `llm` — `LLMDeploymentPlugin`: RocksDB Model Storage  
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3855  
**Type:** Feature (persistence)  
**Scope:** `src/llm/llm_deployment_plugin.cpp` — Real RocksDB model persistence  
**Acceptance:**
- [ ] Design: Model serialization and storage contract
- [ ] Implementation: Replace mock with RocksDB backend
- [ ] Tests: Model persistence round-trip; recovery scenarios
- [ ] PR: Linked to ROADMAP.md v1.6.0 milestone

---

### B-11 · `llama_cpp` — Real `generate()` Inference via LlamaWrapper  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3856  
**Type:** Feature (LLM inference)  
**Scope:** `src/llama_cpp/llama_wrapper.cpp` — Production llama.cpp inference  
**Acceptance:**
- [ ] Design: Inference pipeline contract (model load, tokenize, forward, decode)
- [ ] Implementation: Replace mock with real llama.cpp library
- [ ] Tests: Integration test with real model; token streaming; error paths
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

### B-12 · `query` — `QueryOptimizer`: Wire Real MetadataShard and Statistics  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3857  
**Type:** Feature (optimizer correctness)  
**Scope:** `src/query/query_optimizer.cpp` — Real metadata shard wiring  
**Acceptance:**
- [ ] Design: Metadata retrieval contract
- [ ] Implementation: Replace mock statistics with real shard queries
- [ ] Tests: Optimizer correctness tests with varied statistics
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

### B-13 · `query` — `QueryFederation`: Real Shard Determination Logic  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3858  
**Type:** Feature (sharding correctness)  
**Scope:** `src/query/query_federation.cpp` — Real shard determination  
**Acceptance:**
- [ ] Design: Shard selection contract
- [ ] Implementation: Replace mock with real shard manager lookup
- [ ] Tests: Shard distribution correctness; fallback error paths
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

### B-14 · `query` — `CTESubquery`: Replace Phase 1 Stub  
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #3859  
**Type:** Feature (CTE support)  
**Scope:** `src/query/cte_subquery.cpp` — Production CTE execution  
**Acceptance:**
- [ ] Design: CTE lifecycle contract
- [ ] Implementation: Replace stub with real CTE materialization/streaming
- [ ] Tests: CTE correctness tests; performance gating
- [ ] PR: Linked to ROADMAP.md v1.7.0 milestone

---

### B-15 · `rag` — `LLMIntegration` / `LLMJudgeIntegration`: Replace Mock Mode  
**Priority:** 🟠 High | **Target:** v1.6.0 | **Issue:** #3860  
**Type:** Feature (RAG production)  
**Scope:** `src/rag/` — Real LLM integration for retrieval and ranking  
**Acceptance:**
- [ ] Design: LLM call contract (model selection, prompt templating, response parsing)
- [ ] Implementation: Replace mock with real llm module integration
- [ ] Tests: E2E RAG pipeline tests; LLM error handling
- [ ] PR: Linked to ROADMAP.md v1.6.0 milestone

---

### B-16 · `security` — `ArrowUserRegistrationPlugin`: Apache Arrow Integration  
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #3861  
**Type:** Feature (Apache Arrow integration)  
**Scope:** `src/security/arrow_user_registration_plugin.cpp` — Real Arrow interop  
**Acceptance:**
- [ ] Design: Arrow schema contract
- [ ] Implementation: Replace mock with real Arrow integration
- [ ] Tests: Arrow serialization round-trip tests
- [ ] PR: Linked to ROADMAP.md v1.7.0 milestone

---

### B-17 · `server` — `HttpServer`: Initialize Real `ShardingManager`  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3862  
**Type:** Feature (server initialization)  
**Scope:** `src/server/http_server.cpp` — Real ShardingManager wiring  
**Acceptance:**
- [ ] Design: ShardingManager initialization contract
- [ ] Implementation: Replace mock with real manager initialization
- [ ] Tests: Server startup integration tests
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

### B-18 · `sharding` — `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode  
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #3863  
**Type:** Feature (GPU erasure coding)  
**Scope:** `src/sharding/gpu_erasure_coder_opencl.cpp` — Production OpenCL kernels  
**Acceptance:**
- [ ] Design: Erasure coding algorithm contract
- [ ] Implementation: Real OpenCL kernel library
- [ ] Tests: Parity with CPU path; error injection recovery tests
- [ ] PR: Linked to ROADMAP.md v1.7.0 milestone

---

### B-19 · `training` — `ProvenanceTracker`: Replace AQL Template Stubs  
**Priority:** 🟡 Medium | **Target:** v1.7.0 | **Issue:** #3864  
**Type:** Feature (model training provenance)  
**Scope:** `src/training/provenance_tracker.cpp` — Real AQL template expansion  
**Acceptance:**
- [ ] Design: Template expansion contract
- [ ] Implementation: Replace mock AQL generation with real engine
- [ ] Tests: Provenance tracking correctness
- [ ] PR: Linked to ROADMAP.md v1.7.0 milestone

---

### B-20 · `utils` — `PKIClient`: Replace Fallback Stub Verification  
**Priority:** 🟠 High | **Target:** v1.5.0 | **Issue:** #3865  
**Type:** Feature (PKI verification)  
**Scope:** `src/utils/pki_client.cpp` — Real certificate verification  
**Acceptance:**
- [ ] Design: PKI verification contract
- [ ] Implementation: Replace fallback with real cert verification
- [ ] Tests: Certificate validation tests; revocation checking
- [ ] PR: Linked to ROADMAP.md v1.5.0 milestone

---

## Milestone / Release Tag Mapping

### v1.1.0 (Critical Security Fixes + Simulation Replacement)
**Target:** Q3 2026 (8-10 weeks)  
**Items:** A-01, A-02, A-03, A-04, A-06, A-09, A-10  
**Tag:** `v1.1.0` (Community), `enterprise-v1.1.0`, `military-v1.1.0`, `hyperscaler-v1.1.0`  
**Release Type:** `stable` (per RELEASE_STRATEGY.md)

---

### v1.2.0 (GPU Backend Dispatch + Chimera Expansion)
**Target:** Q3 2026 – Q4 2026 (4-6 weeks post-v1.1.0)  
**Items:** A-05, A-07, A-08  
**Tag:** `v1.2.0` (Community), `enterprise-v1.2.0`, `military-v1.2.0`, `hyperscaler-v1.2.0`  
**Release Type:** `stable` (per RELEASE_STRATEGY.md)

---

### v1.5.0 (API Completion + Production Connectors)
**Target:** Q4 2026 – Q1 2027  
**Items:** B-01, B-04, B-11, B-12, B-13, B-17, B-20  
**Tag:** `v1.5.0` (Community), `enterprise-v1.5.0`, `military-v1.5.0`, `hyperscaler-v1.5.0`  
**Release Type:** `stable` (per RELEASE_STRATEGY.md)

---

### v1.6.0 (LLM Integration + Advanced Ingestion)
**Target:** Q1 2027 – Q2 2027  
**Items:** B-07, B-08, B-09, B-10, B-15  
**Tag:** `v1.6.0` (Community), `enterprise-v1.6.0`, `military-v1.6.0`, `hyperscaler-v1.6.0`  
**Release Type:** `stable` (per RELEASE_STRATEGY.md)

---

### v1.7.0 (Advanced Query + Sharding + Training)
**Target:** Q2 2027 – Q3 2027  
**Items:** B-14, B-16, B-18, B-19  
**Tag:** `v1.7.0` (Community), `enterprise-v1.7.0`, `military-v1.7.0`, `hyperscaler-v1.7.0`  
**Release Type:** `stable` (per RELEASE_STRATEGY.md)

---

## Implementation Workflow (Per Committing Agent)

For each Wave 1 or Wave 2 item:

### Phase 1: Design
- [ ] Read module `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`
- [ ] Identify production interface (header, namespace, method signatures)
- [ ] Document feature gates (e.g., `THEMIS_ENABLE_CUDA`)
- [ ] Document activation conditions (when stub path → production path)

### Phase 2: Core Implementation
- [ ] Replace stub body with production logic
- [ ] Wire real backend / SDK / service call
- [ ] Keep stub path available only under explicit test-double injection

### Phase 3: Error Handling & Edge Cases
- [ ] Timeout / retry / partial failure / backend unavailable
- [ ] Structured error codes (reuse `errors.h` or module-specific registry)
- [ ] No silent fallbacks to stub path in production builds

### Phase 4: Tests
- [ ] Unit tests: production path + all documented error paths
- [ ] Integration tests: real backend or injected controlled test-double
- [ ] Regression tests: prior stub/fallback behaviour no longer reachable in production
- [ ] Performance gate: measured against targets from Wave 7 benchmark suite

### Phase 5: Observability / Security
- [ ] Prometheus counter / gauge for the new code path
- [ ] Structured audit log entry (especially for auth, crypto, ingestion)
- [ ] Security review: cert validation, input validation, sandbox boundaries

### Phase 6: Documentation & Acceptance
- [ ] Update `src/<module>/ROADMAP.md`: mark item `[x]`
- [ ] Update `src/<module>/AUDIT.md`: close open item
- [ ] Update `ROADMAP.md`: mark item `[x]`
- [ ] Update `FUTURE_ENHANCEMENTS.md`: mark item `[x]`
- [ ] PR description must reference issue from ROADMAP.md
- [ ] PR linked to target milestone (v1.1.0, v1.2.0, v1.5.0, etc.)

---

## Key Constraints

1. **No Silent Fallbacks in Production**
   - If production path unavailable → deterministic error (not silent CPU/simulation fallback)
   - Simulation path must be explicitly marked with `STUB/SIMULATION NOTE` (purpose, activation, delta, removal target)

2. **Feature Gates**
   - Production paths guarded by `THEMIS_ENABLE_*` when optional hardware (GPU) or external service required
   - Fallback must fail fast if gate=OFF and production path invoked

3. **Thread Safety**
   - All thread-safe stubs (JWKS cache, etc.) must use `std::shared_mutex` or atomic operations
   - No classic data races

4. **Timing Side-Channels**
   - Auth/crypto paths must use `CRYPTO_memcmp` for sensitive comparisons
   - No early exit / length-based early termination

5. **Performance Gating**
   - Each Wave 2+ stub replacement must be measured against Wave 7 benchmark baseline
   - Must not regress latency or throughput below baseline

---

## Related Documentation

- `ROADMAP.md` — Complete roadmap with EPIC phases and system gates
- `FUTURE_ENHANCEMENTS.md` — Full Wave A + Wave B backlog with detailed acceptance criteria
- `BRANCHING_STRATEGY.md` — Branch model and edition mapping
- `RELEASE_STRATEGY.md` — Tag naming and milestone alignment
- `src/*/ROADMAP.md` — Module-level completion status
- `benchmarks/wave7/` — Release gate measurements and performance baselines

---

## Sign-Off

This baseline consolidates all open production-blocking and near-term hardening work into one canonical execution board. No duplicates, no planning drift.

**Next Step:** Assign Wave 1 items to delivery sprints; begin Phase 1 design on A-01 (JWT) through A-10 (AQL thread leak).

---
