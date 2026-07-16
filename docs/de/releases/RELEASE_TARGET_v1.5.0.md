# ThemisDB v1.5.0 — Release Target Definition

**Document Type:** Release Target (Pre-Release Planning)
**Milestone:** v1.5.0
**Target Release Date:** 2026-02-03
**Previous Version:** v1.4.1-dev-alpha
**Release Type:** Feature Release
**Status:** ✅ Released
**Release Aggregation Document:** [`docs/de/releases/RELEASE_NOTES_v1.5.0.md`](./RELEASE_NOTES_v1.5.0.md)

---

## 🎯 Milestone Overview

ThemisDB v1.5.0 is a major feature release focused on three strategic pillars:

1. **GPU Hardening** — Production-ready GPU kernel isolation (WASM sandbox), multi-node GPU coordination, and acceleration-layer CPU/GPU parity validation.
2. **Security & Compliance** — QueryMaskingPolicy for PII field masking, full PKCS#11 HSM stack, RFC 3161 Timestamp Authority, HSM-backed update signing, and audit-log data-loss protection.
3. **Geo & Acceleration Completeness** — Full GeoJSON RFC 7946 support, in-memory R-tree spatial index, ST_UNION/ST_DIFFERENCE geometry operations, and baseline CPU-vs-GPU throughput benchmarks.

Additional scope areas: RPC production implementation, CRDT expansion, shard repair/anti-entropy, write-amplification optimisations, voice module hardening, and cross-module performance regression CI.

---

## 📦 Major Deliverables

The following deliverables are **in scope** for v1.5.0. Each entry includes the owning module, the target issue or PR, and its completion criterion.

### 1. GPU Module — WASM Kernel Sandbox

| Field | Value |
|-------|-------|
| **Module** | gpu |
| **PR** | [#3051](https://github.com/makr-code/ThemisDB/pull/3051) |
| **Files** | `include/themis/gpu/wasm_kernel_sandbox.h`, `src/gpu/wasm_kernel_sandbox.cpp`, `tests/test_gpu_wasm_kernel_sandbox.cpp` |
| **Completion Criterion** | `WASMKernelSandbox` validates, resource-caps, and dispatches third-party GPU kernel blobs; all test cases pass; feature-gate `THEMIS_ENABLE_WASM_SANDBOX` verified |

### 2. Security Module — QueryMaskingPolicy

| Field | Value |
|-------|-------|
| **Module** | security |
| **PR** | [#3050](https://github.com/makr-code/ThemisDB/pull/3050) |
| **Files** | `include/security/query_masking_policy.h`, `src/security/query_masking_policy.cpp`, `tests/test_query_masking_policy.cpp` |
| **Completion Criterion** | Field-level PII masking applied to all query results before delivery; role-based bypass functional; integrated into `QueryApiHandler`; all test cases pass |

### 3. Geo Module — CPU/GPU Throughput Benchmarks

| Field | Value |
|-------|-------|
| **Module** | geo |
| **PR** | [#3049](https://github.com/makr-code/ThemisDB/pull/3049) |
| **Files** | `benchmarks/bench_geo_cpu_gpu.cpp`, `benchmarks/baselines/` |
| **Completion Criterion** | Google Benchmark suite covers `batchIntersects`, `exactIntersects`, `geodesicDistance` across `cpu_exact`, `cpu_approx`, and `gpu_spatial` backends at batch sizes 256/1024/4096; baseline CSV committed |

### 4. Acceleration Module — CUDA ANN + Geospatial Kernels

| Field | Value |
|-------|-------|
| **Module** | acceleration |
| **Issue** | [#1383](https://github.com/makr-code/ThemisDB/issues/1383) |
| **Completion Criterion** | Batch `intersects` and distance dispatch via `GpuBatchBackend` production-ready; full GPU regression suite passes; numeric parity with CPU baseline confirmed |

### 5. Acceleration Module — Vulkan Compute Shader Pipeline

| Field | Value |
|-------|-------|
| **Module** | acceleration |
| **Issue** | [#1384](https://github.com/makr-code/ThemisDB/issues/1384) |
| **Completion Criterion** | Initial Vulkan backend wired into `ISpatialComputeBackend`; shader integrity verification complete; basic compute pipeline test passes |

### 6. Acceleration Module — Cross-Backend L2 Distance Validation

| Field | Value |
|-------|-------|
| **Module** | acceleration |
| **Issue** | [#1390](https://github.com/makr-code/ThemisDB/issues/1390) |
| **Completion Criterion** | Regression test suite confirms CPU/GPU numeric parity for L2 distance across all enabled backends; baseline committed |

### 7. Geo Module — Full GeoJSON RFC 7946 Support

| Field | Value |
|-------|-------|
| **Module** | geo |
| **Files** | `include/utils/geo/ewkb.h`, `src/utils/geo/ewkb.cpp`, `include/geo/spatial_backend.h` |
| **Completion Criterion** | All RFC 7946 geometry types parse without error; coordinates outside WGS84 range raise `std::runtime_error`; `-DTHEMIS_GEO_COMPAT_LAX=1` compatibility mode available |

### 8. Geo Module — In-Memory R-Tree Spatial Index

| Field | Value |
|-------|-------|
| **Module** | geo |
| **Completion Criterion** | R-tree index supports insert, delete, and spatial range queries; correctness tests pass; basic throughput benchmark included |

### 9. Geo Module — ST_UNION and ST_DIFFERENCE

| Field | Value |
|-------|-------|
| **Module** | geo |
| **Completion Criterion** | `ST_UNION` and `ST_DIFFERENCE` operations produce correct results for point, linestring, and polygon geometries; edge cases (empty geometries, invalid input) handled |

### 10. Search Module — New Search Components

| Field | Value |
|-------|-------|
| **Module** | search |
| **Completion Criterion** | At minimum 5 additional search component types integrated; all new components have unit tests and documentation |

### 11. Sharding Module — Shard Repair / Anti-Entropy Engine

| Field | Value |
|-------|-------|
| **Module** | sharding |
| **Completion Criterion** | Anti-entropy engine detects and repairs divergent replicas; at least basic integration test confirms repair convergence |

### 12. Security Module — Full PKCS#11 HSM + RFC 3161 TSA Stack

| Field | Value |
|-------|-------|
| **Module** | security |
| **PRs** | [#3453](https://github.com/makr-code/ThemisDB/pull/3453)–[#3462](https://github.com/makr-code/ThemisDB/pull/3462) |
| **Files** | `include/security/pkcs11_provider.h`, `include/security/pkcs11_initializer.h`, `include/security/timestamp_authority.h` |
| **Completion Criterion** | PKCS#11 wrapper, initializer, and production failsafe all pass tests against SoftHSMv2; RFC 3161 request/response verified with at least one public TSA backend; stub hard-fails in production builds |

### 13. Security Module — HSM-Backed SigningService

| Field | Value |
|-------|-------|
| **Module** | security / updates |
| **PR** | [#3438](https://github.com/makr-code/ThemisDB/pull/3438) |
| **Completion Criterion** | Update bundles signed via PKCS#11 hardware-backed operations; soft-key path rejected by verifier in production builds |

### 14. Security / Observability — Audit Log Data-Loss Protection

| Field | Value |
|-------|-------|
| **Module** | security / observability |
| **PR** | [#3463](https://github.com/makr-code/ThemisDB/pull/3463) |
| **Completion Criterion** | Writes `fsync`-ed immediately; log rotation at configurable size/age; secondary mirror path functional; data-loss scenario tests pass |

### 15. Updates Module — Update History Log

| Field | Value |
|-------|-------|
| **Module** | updates |
| **PR** | [#3420](https://github.com/makr-code/ThemisDB/pull/3420) |
| **Completion Criterion** | Persistent audit trail records operator identity, source/target version, timestamp, and outcome; retained across restarts |

### 16. Updates Module — Blue/Green Deployment Support

| Field | Value |
|-------|-------|
| **Module** | updates |
| **PR** | [#3421](https://github.com/makr-code/ThemisDB/pull/3421) |
| **Completion Criterion** | Phase 4 of zero-downtime upgrade path functional; automatic rollback on failure; integration test demonstrates traffic routing switch |

### 17. Replication / Updates — CoordinatedUpdateManager

| Field | Value |
|-------|-------|
| **Module** | replication / updates |
| **PR** | [#3422](https://github.com/makr-code/ThemisDB/pull/3422) |
| **Completion Criterion** | Multi-node replication-safe update sequencing ensures all replicas apply updates in consensus order; test with simulated 3-node cluster |

### 18. GPU Module — Multi-Node GPU Coordination Production-Ready

| Field | Value |
|-------|-------|
| **Module** | gpu |
| **PR** | [#3425](https://github.com/makr-code/ThemisDB/pull/3425) |
| **Completion Criterion** | Device management and topology discovery hardened; marked production-ready in `src/gpu/ROADMAP.md` |

### 19. Performance Module — Memory Pressure Monitor (Phase 3)

| Field | Value |
|-------|-------|
| **Module** | performance |
| **PR** | [#3426](https://github.com/makr-code/ThemisDB/pull/3426) |
| **Completion Criterion** | Memory pressure callbacks (cache eviction, GC, reject-new-writes) trigger at configurable thresholds; wired into build system; test suite passes |

### 20. Performance Module — PMU Cache-Miss Analysis & Regression CI

| Field | Value |
|-------|-------|
| **Module** | performance / ci |
| **PRs** | [#3435](https://github.com/makr-code/ThemisDB/pull/3435), [#3437](https://github.com/makr-code/ThemisDB/pull/3437) |
| **Completion Criterion** | L1/L2/LLC miss rates exposed per query; CI job fails on metric regression beyond configurable tolerance; baseline stored under `benchmarks/baselines/` |

### 21. Query Module — Per-Query Resource Limits

| Field | Value |
|-------|-------|
| **Module** | query |
| **PR** | [#3427](https://github.com/makr-code/ThemisDB/pull/3427) |
| **Completion Criterion** | `max_rows`, `max_memory_bytes`, `timeout_ms` enforced at execution time; HTTP 429/408 returned on violation; configurable via request header and global config |

### 22. Replication Module — CRDT FLAG Types

| Field | Value |
|-------|-------|
| **Module** | replication |
| **PR** | [#3428](https://github.com/makr-code/ThemisDB/pull/3428) |
| **Completion Criterion** | `FLAG_EW` and `FLAG_DW` types integrate with `CRDTManager` merge pipeline; serialisation round-trip tests pass |

### 23. Voice Module — Real-Time Meeting Transcription, STT/TTS Benchmarks, Language Detection

| Field | Value |
|-------|-------|
| **Module** | voice |
| **PRs** | [#3434](https://github.com/makr-code/ThemisDB/pull/3434), [#3439](https://github.com/makr-code/ThemisDB/pull/3439), [#3442](https://github.com/makr-code/ThemisDB/pull/3442), [#3444](https://github.com/makr-code/ThemisDB/pull/3444) |
| **Completion Criterion** | Meeting transcription + action-item extraction functional; unit-test coverage ≥ 80%; STT latency and TTS generation-speed benchmarks committed; audio security audit (#3443) complete |

### 24. RPC Layer — Full Production Implementation

| Field | Value |
|-------|-------|
| **Module** | rpc |
| **PRs** | [#3445](https://github.com/makr-code/ThemisDB/pull/3445)–[#3450](https://github.com/makr-code/ThemisDB/pull/3450) |
| **Completion Criterion** | All stub RPC methods replaced with real RocksDB-backed implementations (GET, PUT, DELETE, Batch); full integration test coverage |

### 25. Sharding Module — Hardware Migration / NodeIdentity Persistence

| Field | Value |
|-------|-------|
| **Module** | sharding |
| **PR** | [#3464](https://github.com/makr-code/ThemisDB/pull/3464) |
| **Completion Criterion** | `NodeIdentity` survives reboots and hardware replacement; safe endpoint replacement API functional; no false `NODE_LOST` events in integration test |

### 26. Storage Module — Write-Amplification Optimisation

| Field | Value |
|-------|-------|
| **Module** | storage |
| **Completion Criterion** | Measurable reduction in write amplification factor under sustained write workload; benchmark result committed to baselines |

### 27. Index Module — FAISS Quantizer Integration

| Field | Value |
|-------|-------|
| **Module** | index |
| **Completion Criterion** | FAISS quantizer wired into vector index pipeline; search recall and throughput benchmarks committed |

### 28. Chimera Module — CI Benchmark Baseline

| Field | Value |
|-------|-------|
| **Module** | chimera |
| **PR** | [#3424](https://github.com/makr-code/ThemisDB/pull/3424) |
| **Completion Criterion** | `run_ci_benchmarks` integration-test target seeds repeatable baseline under `benchmarks/baselines/chimera_baseline.json`; CI comparison job operational |

---

## ✅ Completion Criteria Summary

A deliverable is **complete** for v1.5.0 when ALL of the following are satisfied:

| # | Criterion | Detail |
|---|-----------|--------|
| C-01 | All unit and integration tests pass in CI | No failing tests on the feature branch; zero skipped tests in critical paths |
| C-02 | No P0/P1 regressions vs v1.4.x | All existing API contracts maintained; no unplanned breaking changes |
| C-03 | Security-sensitive features reviewed | Crypto, masking, sandboxing, HSM, and audit paths security-reviewed and signed off |
| C-04 | Performance-critical features benchmarked | Baseline stored under `benchmarks/baselines/`; no regression beyond threshold |
| C-05 | Documentation present | Doxygen/comment blocks in headers; CHANGELOG.md updated; module ROADMAP.md updated |
| C-06 | Migration guide for breaking changes | Every breaking change has a migration note in CHANGELOG.md and RELEASE_NOTES |
| C-07 | Roadmap item linkage | Every PR traced to at least one roadmap item in `ROADMAP.md` or module `ROADMAP.md` |

---

## 📋 Minimum Code Readiness Criteria

The following **must** hold for all code merged into v1.5.0:

- **No stub implementations** in production paths — every public interface is backed by real logic or has an explicit `STUB/SIMULATION NOTE` comment with removal plan.
- **No unresolved TODO/FIXME** in files changed by v1.5.0 PRs unless tracked in an open GitHub issue.
- **Thread-safety** — all new public API types that are intended for shared use are documented as thread-safe or not.
- **RAII / smart pointers** — new code avoids raw `new`/`delete`; uses `std::unique_ptr` or `std::shared_ptr`.
- **`#pragma once`** in all new headers (no `#ifndef` guards unless retrofitting existing files).
- **Const-correctness** — member functions that do not modify state are marked `const`.
- **Error handling** — no silent failures; errors propagated via exceptions or return values as appropriate to the module pattern.
- **Feature gates respected** — GPU/WASM/HSM features compile cleanly when their respective `THEMIS_ENABLE_*` flag is absent.

---

## 📚 Minimum Documentation Readiness Criteria

- **Header documentation** — every public class and non-trivial function has a Doxygen-style comment block.
- **CHANGELOG.md** — at minimum one entry per merged PR noting the module, change type (Added/Fixed/Security/Breaking), and PR reference.
- **Module ROADMAP.md** — all completed roadmap items marked `[x]`; newly in-progress items marked `[~]`.
- **Release notes entry** — every deliverable in this document has a corresponding section in `docs/de/releases/RELEASE_NOTES_v1.5.0.md`.
- **Migration guide** — breaking changes documented in both CHANGELOG.md and RELEASE_NOTES with concrete before/after code examples where applicable.
- **API documentation** — new REST endpoints documented in OpenAPI spec or inline HTTP handler comments.

---

## 🧪 QA Requirements

### Test Coverage Targets

| Module | Minimum Test Coverage | Note |
|--------|-----------------------|------|
| gpu (WASMKernelSandbox) | ≥ 80% line coverage | Sandbox execution paths including timeout/OOM |
| security (QueryMasking, HSM, TSA) | ≥ 85% line coverage | Security-critical paths require thorough coverage |
| geo (RFC 7946, R-tree, ST_UNION/DIFF) | ≥ 80% line coverage | Edge cases: empty geometries, invalid coordinates |
| rpc | ≥ 80% line coverage | All GET/PUT/DELETE/Batch paths |
| voice | ≥ 80% line coverage | Minimum target per PR #3439 |
| acceleration (CUDA/Vulkan/L2 parity) | CI pass | Numeric parity regression suite required |
| All other v1.5.0 modules | ≥ 75% line coverage | Default threshold |

### Regression Testing

- **v1.4.x compatibility regression suite** must pass in full before release sign-off.
- **Geo geometry regression suite** — all pre-existing geometry tests from v1.4.x must pass under the new RFC 7946 parser.
- **RPC regression suite** — all client-facing RPC calls from v1.4.x must produce identical results in v1.5.0.
- **CRDT merge regression** — existing CRDT types (`GSet`, `2PSet`, `LWWElementSet`, `ORSet`) must serialise/merge identically.

### Security QA

| Item | Requirement |
|------|-------------|
| QueryMaskingPolicy | Penetration test: confirm masked fields are never present in response body for non-privileged roles |
| WASMKernelSandbox | Fuzz test: submit malformed kernel blobs; verify no crashes or escapes |
| PKCS#11 HSM | Tested against SoftHSMv2 (CI) and at least one hardware token type (manual) |
| RFC 3161 TSA | Timestamp round-trip verified against FreeTSA public endpoint |
| Audit log fsync | Power-loss simulation confirms no log data loss |

---

## 📈 Performance Requirements

### Benchmark Baselines

The following benchmarks must be committed to `benchmarks/baselines/` before v1.5.0 can be declared ready:

| Benchmark | File | Metric |
|-----------|------|--------|
| Geo CPU/GPU throughput | `bench_geo_cpu_gpu.cpp` | `batchIntersects`, `exactIntersects`, `geodesicDistance` at 256/1024/4096 pairs |
| PMU cache-miss | `include/performance/pmu_cache_miss_analyzer.h` | L1/L2/LLC miss rates per query under standard workload |
| Chimera adapter | `tests/chimera/test_chimera_ci_benchmarks.cpp` | Adapter throughput under synthetic load |
| STT/TTS latency | `benchmarks/bench_voice_stt_tts.cpp` | STT latency p50/p95/p99; TTS generation speed (words/sec) |

### Regression Thresholds

| Metric | Max Allowed Regression |
|--------|------------------------|
| Geo batch intersects throughput | ≤ 5% vs v1.4.x baseline |
| RPC GET/PUT p99 latency | ≤ 10% vs v1.4.x baseline |
| CRDT merge throughput | ≤ 5% vs v1.4.x baseline |
| Vector index search recall | ≤ 1% absolute vs v1.4.x |
| Write amplification factor | Must decrease (not increase) vs v1.4.x |

### CI Performance Gate

The cross-module performance regression CI introduced in PR #3437 must be active and passing. Any metric regressing beyond the thresholds above causes the release build to fail.

---

## 🚫 Out-of-Scope Items

The following items have been **explicitly excluded** from v1.5.0. They are tracked for future milestones.

| Item | Reason for Exclusion | Target Milestone |
|------|----------------------|-----------------|
| Voice SIP/WebRTC integration | PR #3431 WIP; insufficient time for QA | v1.6.0 |
| Build modularisation | PR #3429 WIP; high risk, insufficient review time | v1.6.0 |
| Chimera adapter capabilities matrix | Documentation-only; low risk; no blocking dependency | v1.5.x patch |
| Pre-flight health checks update | Incomplete; no blocking dependency on release | v1.5.x patch |
| Spherical geometry support (WGS-84 ellipsoid) | Not yet designed; complex correctness requirements | v2.3.0 |
| GPU-accelerated DBSCAN/k-means (headers) | Depends on CUDA kernel maturity work | v2.3.0 |
| Multi-GPU Vector Indexing API | Scaffolding only; full partition strategy requires more design | v1.7.0 |
| Distributed Query Optimizer (full) | Latency hook design incomplete | v1.7.0 |
| FAISS ADC distance tables (~40% speedup) | Dependent on indexing refactor | v1.7.0 |
| Config hierarchical reorganization | Architectural change; requires migration tooling | v1.7.0 |
| Git-Like PITR REST API | SnapshotManager design incomplete | v1.7.0 |
| Root Cause Analyzer (`RootCauseAnalyzer`) | LLM integration dependency not stabilised | v1.7.0 |

---

## 🔮 Future Roadmap Candidates (Post-v1.5.0)

Items raised during v1.5.0 planning that are worth capturing for later cycles:

| Item | Notes | Candidate Milestone |
|------|-------|---------------------|
| Vulkan compute shader — full geometry pipeline | #1384 initial Vulkan wiring; full geometry pipeline deferred | v1.7.0–v1.8.0 |
| CUDA ANN GPU-only path (no CPU fallback) | Requires sustained GPU availability in CI | v1.7.0 |
| Geo streaming ingestion with R-tree live updates | Depends on CDC streaming pipeline | v1.8.0 |
| WASM sandbox multi-kernel composition | WASMKernelSandbox single-kernel only in v1.5.0 | v1.9.0 |
| RFC 3161 TSA — offline/air-gapped mode | Requires local TSA server component | v1.8.0 |
| Voice speaker diarisation | STT pipeline dependency | v1.8.0 |
| Per-query resource limits — enforcement via cgroup v2 | Requires Linux cgroup integration | v2.0.0 |
| CRDT convergence proofs (formal verification) | Research effort | TBD |

---

## 🚦 Release Sign-Off Matrix

The following roles must sign off before v1.5.0 can be tagged and shipped:

| Role | Sign-Off Condition | Status |
|------|--------------------|--------|
| Engineering Lead | All C-01–C-07 completion criteria satisfied for in-scope deliverables | ✅ Signed off (2026-02-03) |
| Security Reviewer | All security QA items in the Security QA table completed | ✅ Signed off (2026-02-03) |
| QA Lead | Regression suite passes; all ⚠️ QA flags in QA Status table resolved | ✅ Signed off (2026-02-03) |
| Documentation Owner | All documentation readiness criteria met | ✅ Signed off (2026-02-03) |
| Release Manager | Performance baselines committed; CI passing; changelog updated | ✅ Signed off (2026-02-03) |

---

## 🔗 Related Documents

| Document | Link |
|----------|------|
| Release Aggregation (post-release summary) | [`docs/de/releases/RELEASE_NOTES_v1.5.0.md`](./RELEASE_NOTES_v1.5.0.md) |
| Release Candidate Checklist | [`docs/de/releases/RC_CHECKLIST.md`](./RC_CHECKLIST.md) |
| Top-Level Project Roadmap | [`ROADMAP.md`](../../../ROADMAP.md) |
| Geo Module Roadmap | [`src/geo/ROADMAP.md`](../../../src/geo/ROADMAP.md) |
| GPU Module Roadmap | [`src/gpu/ROADMAP.md`](../../../src/gpu/ROADMAP.md) |
| Security Module Roadmap | [`src/security/ROADMAP.md`](../../../src/security/ROADMAP.md) |
| Acceleration Module Roadmap | [`src/acceleration/ROADMAP.md`](../../../src/acceleration/ROADMAP.md) |
| Future GPU Support | [`docs/FUTURE_GPU_SUPPORT.md`](../../FUTURE_GPU_SUPPORT.md) |
| Version Comparison v1.3.4 → v1.5.0 | [`docs/de/releases/VERSION_COMPARISON_v1.3.4_vs_v1.5.0.md`](../../reports/VERSION_COMPARISON_v1.3.4_vs_v1.5.0.md) |

---

*This document was produced as part of the v1.5.0 milestone planning (Issue: makr-code/ThemisDB#[release-target]). It serves as the definitive pre-release target definition and stakeholder alignment record for v1.5.0. For post-release actuals, see the Release Aggregation document linked above. Last updated: 2026-04-15.*
