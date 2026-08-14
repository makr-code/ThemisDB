# Wave A/B Remediation Execution Master Board

**Generated:** 2026-08-14  
**Status:** ⏸️ Execution Phase Starting (A1 on critical path)  
**Timeline:** Q3–Q4 2026  

---

## Executive Summary

Wave A/B remediation addresses runtime reliability (Wave A) and performance consolidation (Wave B) across 8 batches spanning 5 critical modules + 3 performance-critical services. Strict gate progression enforces: A1 → A2-A5 parallel → Wave A Exit Criteria → B1-B3 parallel → Wave B Exit Criteria.

**Critical Path:** Transaction (A1) build/run verification → Replication (A2) geo placement → Sharding (A5) multi-shard stress gates.

---

## Wave A Execution — Runtime Reliability (Q3–Q4 2026)

### Batch A1 — Transaction Build/Run Verification + Chaos Evidence

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Test Verification | Phase 2 build verification: `test_transaction_distributed_phase2.cpp` (9 tests) | TBD | 🔴 PENDING | 2026-08-21 | None |
| Test Verification | Phase 2 run verification: all 9 tests pass | TBD | 🔴 PENDING | 2026-08-21 | Build pass |
| Test Verification | Phase 2 run verification: `test_transaction_saga_compensation_phase2.cpp` (12 tests) | TBD | 🔴 PENDING | 2026-08-21 | Build pass |
| Test Verification | Phase 3 build verification: `test_transaction_fault_injection_phase3.cpp` (14 tests) | TBD | 🔴 PENDING | 2026-08-28 | Phase 2 pass |
| Test Verification | Phase 3 run verification: all 14 tests pass | TBD | 🔴 PENDING | 2026-08-28 | Phase 3 build pass |
| Chaos Evidence | Coordinator crash-recovery: WAL replay validation (AC-6) | TBD | 🔴 NOT_STARTED | 2026-09-04 | Phase 2+3 pass |
| Chaos Evidence | SAGA compensation idempotency under retry storm (AC-8/AC-10) | TBD | 🔴 NOT_STARTED | 2026-09-04 | Phase 2+3 pass |
| Chaos Evidence | Circuit breaker activation after 5 consecutive failures (AC-10) | TBD | 🔴 NOT_STARTED | 2026-09-04 | Phase 2+3 pass |
| Gate Validation | `release_critical` CI green on `develop` for transaction module | TBD | 🔴 PENDING | 2026-09-04 | All chaos evidence pass |
| Documentation | Close evidence block in `src/transaction/ROADMAP.md` Phase 2 section | TBD | 🔴 NOT_STARTED | 2026-09-04 | All verification complete |

**A1 Exit Criteria:**
- [ ] All Phase 2 tests build and pass with exit 0
- [ ] All Phase 3 tests build and pass with exit 0
- [ ] Coordinator crash-recovery chaos scenario documents 5s WAL replay bound + in-doubt resolution
- [ ] SAGA compensation proves idempotency under 10+ concurrent retries
- [ ] Circuit breaker gate verified: no retries after 5 consecutive failures
- [ ] `release_critical` CI green on `develop`
- [ ] Evidence signed off in `src/transaction/ROADMAP.md`

---

### Batch A2 — Replication Geo Placement + WAL Lag Controls

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Design | Geographic placement policy API specification | TBD | ⏸️ BLOCKED | 2026-09-04 | A1 exit |
| Implementation | Async cross-region WAL shipping with durability guarantees | TBD | ⏸️ BLOCKED | 2026-09-11 | A1 exit + Design |
| Testing | Lag alert integration test: alert fires when regional lag > threshold | TBD | ⏸️ BLOCKED | 2026-09-18 | Implementation |
| Verification | Failover diagnostic improvements: root-cause tracing for replication lag | TBD | ⏸️ BLOCKED | 2026-09-18 | Implementation |
| Gate Validation | `release_critical` CI green on `develop` for replication module | TBD | ⏸️ BLOCKED | 2026-09-25 | All verification pass |

**A2 Entry:** A1 exit criteria PASS (Blocker: A1 → A2)

**A2 Exit Criteria:**
- [ ] Geographic placement policy API is documented and callable
- [ ] WAL shipping maintains RPO ≤ 5s across regions
- [ ] Lag alerts fire within 500ms of threshold breach
- [ ] Failover diagnostics trace replication lag root causes
- [ ] `release_critical` CI green on `develop`

---

### Batch A3 — Voice Fail-Closed Hardening

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Implementation | Stream validation: reject malformed/oversized input | TBD | ⏸️ BLOCKED | 2026-09-04 | A1 exit |
| Implementation | Session lifecycle fail-closed behavior under errors | TBD | ⏸️ BLOCKED | 2026-09-04 | A1 exit |
| Testing | Anti-spoof/liveness regression tests under adversarial input | TBD | ⏸️ BLOCKED | 2026-09-11 | Implementation |
| Verification | Multi-session teardown safety: no resource leaks under cascading errors | TBD | ⏸️ BLOCKED | 2026-09-18 | Implementation |
| Gate Validation | `release_critical` CI green on `develop` for voice module | TBD | ⏸️ BLOCKED | 2026-09-25 | All verification pass |

**A3 Entry:** A1 exit criteria PASS (Blocker: A1 → A3)

**A3 Exit Criteria:**
- [ ] All malformed stream formats are rejected before processing
- [ ] Stream size limits enforced; oversized streams error with bounded resource consumption
- [ ] Liveness check maintains accuracy under 1000 concurrent spoofed sessions
- [ ] Anti-spoof regressions: zero false positives across 10K test sessions
- [ ] Multi-session teardown leaves zero dangling resources

---

### Batch A4 — GPU CUDA Safety + RAII + Timeouts

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Implementation | Audit CUDA API calls: identify unchecked error returns | TBD | ⏸️ BLOCKED | 2026-09-04 | A1 exit |
| Implementation | RAII wrapper lifecycle: kernel launch/completion/cleanup | TBD | ⏸️ BLOCKED | 2026-09-11 | A1 exit + Audit |
| Implementation | Kernel timeout enforcement: fail-closed on timeout | TBD | ⏸️ BLOCKED | 2026-09-11 | RAII wrappers |
| Testing | CPU fallback path: clean degradation on every GPU error | TBD | ⏸️ BLOCKED | 2026-09-18 | Timeout enforcement |
| Gate Validation | `release_critical` CI green on `develop` for gpu module | TBD | ⏸️ BLOCKED | 2026-09-25 | All testing pass |

**A4 Entry:** A1 exit criteria PASS (Blocker: A1 → A4)

**A4 Exit Criteria:**
- [ ] All CUDA API calls checked; unchecked calls wrapped in assert/exception
- [ ] RAII wrappers prove deterministic cleanup on all paths (success, timeout, error)
- [ ] Kernel timeout ≤ 30s enforced; exceeding triggers CPU fallback
- [ ] CPU fallback path produces identical output as GPU path (on representative hardware)
- [ ] Zero GPU resource leaks under 1000 consecutive kernel timeouts

---

### Batch A5 — Sharding Multi-Shard + Rebalance + Distributed Write Stress

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Implementation | Multi-shard exact-path gating: writes to all 3 shards verify consistency | TBD | ⏸️ BLOCKED | 2026-09-04 | A1 exit |
| Implementation | Topology change auto-rebalance: shard add/remove without write downtime | TBD | ⏸️ BLOCKED | 2026-09-11 | A1 exit + Multi-shard |
| Implementation | Latency-aware routing: prefer low-latency shards; fall back on timeout | TBD | ⏸️ BLOCKED | 2026-09-11 | Topology change |
| Testing | Distributed write stress: 1M writes across 3 shards; verify exact-once delivery | TBD | ⏸️ BLOCKED | 2026-09-18 | Latency routing |
| Verification | Long-run stress test: 24h continuous distributed writes; zero corruption | TBD | ⏸️ BLOCKED | 2026-09-25 | Write stress pass |
| Gate Validation | `release_critical` CI green on `develop` for sharding module | TBD | ⏸️ BLOCKED | 2026-10-02 | All verification pass |

**A5 Entry:** A1 exit criteria PASS (Blocker: A1 → A5)

**A5 Exit Criteria:**
- [ ] Multi-shard exact-path gate: 1M writes → 1M reads; all data verified
- [ ] Topology change completes without write downtime; rebalance finalizes within 300s
- [ ] Latency routing switches to fallback within 1s of primary shard timeout
- [ ] 24h stress test: zero data corruption; zero message loss; zero orphaned locks

---

## Wave A Exit Criteria Validation

**Gate to Wave B → Proceed Only If All A1-A5 Checkboxes ✓**

| Criterion | Status | Evidence |
|-----------|--------|----------|
| A1 build/run verification complete | 🔴 PENDING | src/transaction/ROADMAP.md Phase 2+3 section |
| A2 geographic placement + WAL shipping delivered | 🔴 PENDING | src/replication/ROADMAP.md |
| A3 voice fail-closed hardening complete | 🔴 PENDING | src/voice/ROADMAP.md |
| A4 GPU CUDA safety + fallback verified | 🔴 PENDING | src/gpu/ROADMAP.md |
| A5 sharding multi-shard + stress complete | 🔴 PENDING | src/sharding/ROADMAP.md |
| Deterministic chaos evidence for all 5 modules | 🔴 PENDING | Batch delivery evidence blocks in module ROADMAPs |
| Fail-closed behavior verified (all paths) | 🔴 PENDING | Batch delivery evidence blocks |
| `release_critical` CI green on `develop` (all 5 modules) | 🔴 PENDING | CI run reports |
| Representative-hardware p95/p99 baselines refreshed | 🔴 PENDING | benchmarks/ reports |

---

## Wave B Execution — Performance Consolidation (Q3–Q4 2026)

**Status:** ⏸️ Waiting for Wave A Exit Criteria PASS  
**Entry:** Strict gate requires all A1-A5 PASS + Wave A criteria verified

### Batch B1 — Search 4-Layer Orchestrator Integration

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Design | 4-layer orchestrator API spec (ANN/Tensor/Graph/LLM integration points) | TBD | ⏸️ BLOCKED | 2026-10-02 | Wave A exit |
| Implementation | ANN layer integration + gate | TBD | ⏸️ BLOCKED | 2026-10-09 | Wave A exit + Design |
| Implementation | Tensor → Graph → LLM wiring + gates | TBD | ⏸️ BLOCKED | 2026-10-16 | ANN integration |
| Testing | E2E 4-layer chain: p95/p99 latency + memory gates | TBD | ⏸️ BLOCKED | 2026-10-23 | Full wiring |
| Gate Validation | Benchmark gates locked: p95 ≤ 500ms, p99 ≤ 1s, peak memory ≤ 2GB | TBD | ⏸️ BLOCKED | 2026-10-30 | E2E testing |

**B1 Entry:** Wave A exit + all A1-A5 PASS (Blocker: Wave A exit → B1)

---

### Batch B2 — Access Model Phase 5–6 Observability + Benchmarks

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Implementation | Phase 5 observability: metrics, traces, state inspection | TBD | ⏸️ BLOCKED | 2026-10-02 | Wave A exit |
| Implementation | Phase 6 concurrency + end-to-end tests | TBD | ⏸️ BLOCKED | 2026-10-09 | Phase 5 |
| Testing | GATE-ACM-01..06 benchmark closure: all gates reproducible on representative hardware | TBD | ⏸️ BLOCKED | 2026-10-23 | Phase 6 tests |
| Verification | Benchmark gates: p95 ≤ 250ms per gate; zero regression across 5 runs | TBD | ⏸️ BLOCKED | 2026-10-30 | Benchmark testing |

**B2 Entry:** Wave A exit + all A1-A5 PASS (Blocker: Wave A exit → B2)

---

### Batch B3 — LLM Wiki Phase B RocksDB Gates + Cache + Latency

| Phase | Task | Owner | Status | Target | Blocker |
|-------|------|-------|--------|--------|---------|
| Implementation | RocksDB retrieval integration + gate (cache hit rate > 80%) | TBD | ⏸️ BLOCKED | 2026-10-02 | Wave A exit |
| Testing | Query latency gate: p95 ≤ 100ms, p99 ≤ 250ms under 1000 QPS | TBD | ⏸️ BLOCKED | 2026-10-09 | RocksDB integration |
| Verification | Sustained load test: 24h @ 1000 QPS; stable cache hit rate + latency | TBD | ⏸️ BLOCKED | 2026-10-23 | Query testing |
| Gate Validation | Benchmark gates locked: reproducible across 3 runs on representative hardware | TBD | ⏸️ BLOCKED | 2026-10-30 | Sustained test pass |

**B3 Entry:** Wave A exit + all A1-A5 PASS (Blocker: Wave A exit → B3)

---

## Wave B Exit Criteria Validation

**Gate to Batch 7+ → Proceed Only If All B1-B3 Checkboxes ✓**

| Criterion | Status | Evidence |
|-----------|--------|----------|
| B1 4-layer retrieval chain p95/p99 locked | 🔴 PENDING | benchmarks/wave_b1_gates/ |
| B2 access model GATE-ACM-01..06 locked | 🔴 PENDING | benchmarks/wave_b2_gates/ |
| B3 LLM Wiki RocksDB + latency gates locked | 🔴 PENDING | benchmarks/wave_b3_gates/ |
| All gates reproducible on representative hardware | 🔴 PENDING | CI benchmark run reports |
| No regressions across 5 consecutive runs per gate | 🔴 PENDING | Benchmark evidence blocks |

---

## Remaining 38 Modules (Batch 7+) — Post-Wave-B Remediation

**Status:** 📋 Inventory pending Wave A/B completion  
**Target:** Q4 2026 – Q1 2027  
**Model:** Each batch follows Wave A/B acceptance + evidence closure pattern

### Inventory & Categorization

| Priority | Module Count | Target Quarter | Key Modules |
|----------|--------------|-----------------|------------|
| CRITICAL | 8 | Q4 2026 | search (real wiring), llm_wiki (caching), access_model (observability) |
| HIGH | 15 | Q4 2026 – Q1 2027 | ethics_ai, security, audit, observability, network |
| MEDIUM | 10 | Q1 2027 | utils, index, executor, config, metadata |
| LOW | 5 | Q1 2027 | plugins, maintenance, experimental features |

**Next Step:** Post Wave B completion, inventory remaining 38 modules and schedule Batches 7–15 with same acceptance gate model.

---

## Program Success Criteria (Final)

By end Q1 2027, all below must be PASS:

- [ ] All distributed and acceleration paths fail closed
- [ ] Major modules have benchmark-backed p95/p99 baselines on representative hardware
- [ ] Recovery/failover paths have deterministic chaos evidence
- [ ] Security controls have production-style integration validation
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks

---

## Next Immediate Actions (Week of 2026-08-14)

### Action 1: Transaction (A1) Build Verification
- [ ] Run: `cmake --preset community-release && cmake --build --target test_transaction_distributed_phase2`
- [ ] Verify: exit 0
- [ ] Owner: TBD
- [ ] Target: 2026-08-21

### Action 2: Transaction Phase 2 Test Execution
- [ ] Run: `ctest --preset community-release -R transaction_distributed_phase2 -V`
- [ ] Verify: All 9 tests pass
- [ ] Owner: TBD
- [ ] Target: 2026-08-21

### Action 3: Transaction Phase 3 Build + Run
- [ ] Build + run `test_transaction_saga_compensation_phase2.cpp` (12 tests)
- [ ] Build + run `test_transaction_fault_injection_phase3.cpp` (14 tests)
- [ ] Owner: TBD
- [ ] Target: 2026-08-28

### Action 4: Chaos Evidence Planning (AC-6, AC-8, AC-10)
- [ ] Define chaos scenarios: coordinator crash, SAGA retry storm, circuit breaker
- [ ] Schedule: 2026-09-04 evidence collection
- [ ] Owner: TBD
- [ ] Target: Planning draft by 2026-08-28

---

## Risk Register

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Transaction tests fail on `develop` | Blocks all Wave A | Medium | Immediate triage + local build validation |
| CUDA hardware unavailable for A4 | A4 cannot proceed | Low | Self-hosted runner or CPU-only parity fallback |
| RocksDB missing for B3 | B3 blocked | Low | Feature gate behind `THEMIS_WIKI_PHASE_B` |
| Performance regression in A5 stress test | Delays Wave A exit | Medium | Baseline comparison + profiling budget |
| GitHub Actions workflow timeout | Blocks CI gates | Low | Increase timeout; parallelize if needed |

---

## Document Maintenance

**Update Cadence:** Weekly (every Friday)  
**Owner:** TBD (Wave Orchestrator)  
**Sync Points:** Module ROADMAPs, CI run reports, benchmark evidence blocks

**Last Updated:** 2026-08-14  
**Next Sync:** 2026-08-21
