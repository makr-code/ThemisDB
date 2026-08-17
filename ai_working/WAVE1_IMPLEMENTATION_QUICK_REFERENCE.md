# Wave 1 Implementation Quick Reference (v1.1.0–v1.2.0)

**Audience:** Implementing Agent  
**Purpose:** Minimal reference for each Wave 1 item — design, scope, tests, gates  
**Target Timeline:** 8–10 weeks  

---

## A-01 · JWT JWKS Cache Thread-Safety

| Aspect | Detail |
|--------|--------|
| **Files** | `src/auth/jwt_validator.{h,cpp}` |
| **Issue** | Race condition on JWKS cache → undefined behavior, auth bypass |
| **Fix** | Add `mutable std::shared_mutex jwks_mutex_` |
| **Reads** | `std::shared_lock` |
| **Writes** | `std::unique_lock` (refresh path) |
| **Security** | Use `CRYPTO_memcmp` for key comparison (no early-exit timing leak) |
| **Test** | 16 threads × 10K validate() calls with concurrent refresh; no UB with TSan |
| **Phase** | 1: design (1d), 2: impl (2d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-02 · LDAP DN and Filter Injection Prevention

| Aspect | Detail |
|--------|--------|
| **Files** | `src/auth/ldap_authenticator.{h,cpp}` |
| **Issue** | LDAP injection via DN and filter string concatenation |
| **Fix** | Implement `escapeLdapDn()` and `escapeLdapFilter()` (RFC 4515/4514) |
| **Test** | Fuzz: injection payloads `)(uid=*)(|(uid=*`, `*)(uid=*))(|(uid=*` — verify escaped safely |
| **Phase** | 1: design (1d), 2: impl (2d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-03 · Constant-Time Recovery Code / Session ID Comparison

| Aspect | Detail |
|--------|--------|
| **Files** | `src/auth/mfa_authenticator.cpp`, `src/auth/rate_limiter_backend.cpp` |
| **Issue** | Timing side-channel on recovery code / session ID comparison (early exit) |
| **Fix** | Replace `std::find()` with full-traversal loop + `CRYPTO_memcmp` |
| **Test** | Timing test: measure std-dev latency across 10K trials (match vs non-match); assert < 500ns difference |
| **Phase** | 1: design (1d), 2: impl (1d), 3: error (1d), 4: tests (1d), 5: obs (0.5d), 6: doc (0.5d) |

---

## A-04 · Chimera — Production ThemisDB Adapter Integration

| Aspect | Detail |
|--------|--------|
| **Files** | `src/chimera/themisdb_adapter.{hpp,cpp}` |
| **Issue** | Chimera adapter uses in-process simulation; production paths guarded by `NOT_IMPLEMENTED` |
| **Fix** | Wire real engines: `setQueryEngine()`, `setVectorIndex()`, `setGraphIndex()` |
| **Error** | Replace `NOT_IMPLEMENTED` with `ThemisError::BackendUnavailable` when null |
| **Capability** | Update `has_capability()` to reflect actual engine availability |
| **Test** | Inject real engine; verify null-engine path fails deterministically |
| **Phase** | 1: design (2d), 2: impl (3d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-05 · Chimera — MongoDB / Qdrant / Neo4j: Replace In-Process Simulation

| Aspect | Detail |
|--------|--------|
| **Files** | `src/chimera/{mongodb,qdrant,neo4j}_adapter.cpp` |
| **Issue** | Adapters use in-process hash-map simulation instead of real driver calls |
| **Fix** | Feature-gate + real SDK calls: `THEMIS_ENABLE_MONGOCXX`, `THEMIS_ENABLE_QDRANT`, `THEMIS_ENABLE_NEO4J_BOLT` |
| **MongoDB** | Gate on `THEMIS_ENABLE_MONGOCXX`; wire `mongocxx::client` session |
| **Qdrant** | Gate on `THEMIS_ENABLE_QDRANT`; use gRPC client from `qdrant.proto` |
| **Neo4j** | Gate on `THEMIS_ENABLE_NEO4J_BOLT`; use Bolt v4 C++ client |
| **Simulation** | Retain path + mark with `STUB/SIMULATION NOTE` (purpose, activation, delta, removal) |
| **Test** | Contract test against real Docker-compose instance; existing unit tests must still pass |
| **Performance** | No additional linear copy overhead per batch vs simulation |
| **Phase** | 1: design (2d), 2: impl (4d), 3: error (2d), 4: tests (3d), 5: obs (1d), 6: doc (1d) |

---

## A-06 · GPU Query Accelerator: Replace 5 CPU Fallback Stubs

| Aspect | Detail |
|--------|--------|
| **Files** | `src/gpu/query_accelerator.cpp` |
| **Issue** | 5 functions with CPU fallback stubs (not real GPU dispatch) |
| **Fix** | Replace stubs with real CUDA/HIP kernel dispatch |
| **Test** | GPU availability check + fallback error handling when no GPU |
| **Performance** | Speedup >= 2x vs CPU baseline (typical for these ops) |
| **Phase** | 1: design (2d), 2: impl (3d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-07 · GPU Vector Index: CUDA and HIP Backends

| Aspect | Detail |
|--------|--------|
| **Files** | `src/index/gpu_vector_index.cpp` |
| **Issue** | No production GPU backend dispatch (CPU stub only) |
| **Fix** | CUDA + HIP kernel library; backend selection contract |
| **Performance** | p99 distance latency gate from benchmarks/wave7/ baseline |
| **Test** | GPU parity: CUDA vs HIP results match within FP tolerance |
| **Test** | Fallback error paths when no GPU available |
| **Phase** | 1: design (2d), 2: impl (4d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-08 · Geospatial: CUDA and OpenCL Production Backend

| Aspect | Detail |
|--------|--------|
| **Files** | `src/geo/` (all geo query implementation) |
| **Issue** | CPU-only; CUDA and OpenCL not production-wired |
| **Fix** | CUDA kernel library + OpenCL kernel library for distance/containment |
| **Test** | CPU vs GPU correctness: antipodes, equator crossing, edge cases |
| **Performance** | Batch distance computation throughput gate from benchmarks/wave7/ |
| **Phase** | 1: design (2d), 2: impl (4d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-09 · AQL — Post-Generation AQL Validation

| Aspect | Detail |
|--------|--------|
| **Files** | `src/aql/` (new validator + integration with executor) |
| **Issue** | Generated AQL not validated before execution; syntax/semantic errors slip to runtime |
| **Fix** | Post-generation validator before executor hand-off |
| **Validation** | Syntax check + semantic checks + timeout policy validation |
| **Test** | Malformed query rejection; valid query acceptance; timeout enforcement |
| **Phase** | 1: design (1d), 2: impl (2d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (1d) |

---

## A-10 · AQL — Thread Leak in `LLMTimeoutManager::executeWithTimeout()`

| Aspect | Detail |
|--------|--------|
| **Files** | `src/aql/llm_timeout_manager.cpp` |
| **Issue** | Thread lifecycle not managed properly (likely missing `detach()` or `join()` path) |
| **Fix** | Correct thread lifecycle: ensure all threads joined or detached |
| **Test** | Run executor 1K times under leak detector (valgrind, AddressSanitizer); verify no thread accumulation |
| **Phase** | 1: design (1d), 2: impl (2d), 3: error (1d), 4: tests (2d), 5: obs (1d), 6: doc (0.5d) |

---

## Implementation Sequence (Recommended)

### Weeks 1–2: Parallel Security Track
- **Parallel:** A-01 (JWT), A-02 (LDAP), A-03 (constant-time)
- **Rationale:** Highest risk; no dependencies; can proceed in parallel

### Weeks 2–3: Production Wiring
- **Sequential:** A-04 (Chimera ThemisDB), A-06 (GPU accelerator)
- **Rationale:** A-04 enables A-06 integration

### Weeks 3–4: GPU Backends + Safety
- **Parallel:** A-07 (GPU vector index), A-08 (geo backends), A-09 (AQL validation), A-10 (AQL thread leak)
- **Rationale:** A-07 + A-08 independent; A-09 + A-10 independent

### Weeks 5–6: Chimera Expansion
- **Sequential:** A-05 (Chimera MongoDB/Qdrant/Neo4j)
- **Rationale:** Depends on A-04 foundation; longer cycle (3–4 days impl + tests)

---

## Quality Checklist Per Item

### Before Phase 1 Design
- [ ] Issue exists in GitHub (linked from ROADMAP.md + FUTURE_ENHANCEMENTS.md)
- [ ] Module ROADMAP.md item status known (`[ ]` or `[~]`)

### After Phase 2 Implementation
- [ ] Code compiles with no warnings (clang-tidy + -Wall -Wextra)
- [ ] No UB detected by AddressSanitizer, TSan, or valgrind
- [ ] Feature gates (`THEMIS_ENABLE_*`) compile correctly (both ON and OFF)

### After Phase 4 Tests
- [ ] Unit tests: production path + all error paths ✓
- [ ] Integration tests: real backend or injected test-double ✓
- [ ] Regression tests: prior stub unreachable in production ✓
- [ ] Performance: measured against Wave 7 baseline ✓

### Before Phase 6 Acceptance
- [ ] `src/<module>/ROADMAP.md` marked `[x]`
- [ ] `ROADMAP.md` marked `[x]`
- [ ] `FUTURE_ENHANCEMENTS.md` marked `[x]`
- [ ] PR linked to v1.1.0 or v1.2.0 milestone

---

## Related Documentation

- `ROADMAP.md` — System-level roadmap with v1.1.0/v1.2.0 milestones
- `FUTURE_ENHANCEMENTS.md` — Full Wave A item details + 6-phase template
- `EXECUTION_BASELINE_2026_Q3.md` — Consolidated board (this is the canonical reference)
- `src/<module>/ROADMAP.md` — Module-level completion status
- `benchmarks/wave7/` — Performance baseline for quality gates

---
