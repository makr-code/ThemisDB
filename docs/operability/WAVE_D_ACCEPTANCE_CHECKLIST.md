# Wave D Acceptance Checklist

**Document Type:** Wave D Exit Gate Checklist  
**Target Completion:** Q1 2027 (v2.4.x / v2.5.0-rc1)  
**Status:** 🟡 Phase 1 In Progress  
**Last Updated:** 2026-08-24

---

## Phase 1: Prerequisite Verification (Target: 2026-08-15)

### Wave A-C Exit Criteria Status
- [~] **Wave A Exit Criteria** — Deterministic chaos evidence, fail-closed behavior, `release_critical` CI green, p95/p99 baselines
  - [~] Transaction module: 83 tests registered `release_critical`; chaos/recovery evidence indexed in `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`; hardware execution pending Q4 2026
  - [x] Sharding module: Multi-shard exact-path gate, rebalance hardening, distributed write stress ✅ COMPLETE 2026-08-17
  - [x] Replication module: Geographic placement policy, async WAL shipping, failover diagnostics ✅ COMPLETE 2026-08-18
  - [~] Voice module: Test suites (VOICE-CHAOS-01..12, stream validation, anti-spoof, teardown) delivered; representative-hardware baselines pending Q4 2026
  - [~] GPU module: `include/gpu/cuda_raii.h` RAII guards created 2026-08-24 (CudaStreamGuard, CudaEventGuard, CudaDeviceMemoryGuard); KernelSLAGuard confirmed at 11 sites; GPU-TIMEOUT/EXHAUST/FALLBACK suites registered `release_critical`; hardware baselines pending Q4 2026
  - [~] `release_critical` CI gate: registered for Transaction/Voice/GPU; green confirmation pending hardware run
  - [~] Representative hardware p95/p99 baselines: Sharding ✅ Replication ✅; Transaction/Voice/GPU pending Q4 2026

- [~] **Wave B Exit Criteria** — 4-layer retrieval chain stability, Access Model benchmarks, RocksDB gates
  - [x] Search module: LayeredRetrievalOrchestrator 4-layer integration complete ✅ COMPLETE 2026-08-17/18
  - [x] Access Model: Phase 5-6 observability, concurrency/e2e tests, GATE-ACM-01..06 closed ✅ COMPLETE 2026-08-17
  - [~] LLM Wiki: Integration tests delivered (LWP-INT-01..05, 16 tests); `WAVE_B_CLOSURE_EVIDENCE_BUNDLE.md` created 2026-08-24; RocksDB hardware evidence pending Q4 2026

- [x] **Wave C Exit Criteria** — Security integration, Audit reliability, CI policy gates ✅ ALL PASS 2026-08-18
  - [x] Security module: Vault/HSM/PKI integration validation complete ✅
  - [x] Audit module: Integrity + high-volume export reliability under sustained load ✅
  - [x] CI policy gates: Private/public boundaries, edition/license checks, SBOM, fail-closed community builds ✅

### GA Batch D Sign-Off
- [~] Batch D gates D-1 through D-10 all marked PASS
  - [x] D-1: Operations/SLA runbook suites wired ✅
  - [x] D-2: 99.99% SLA gate (RTO ≤ 5000 µs) ✅
  - [x] D-3: Chaos/fault-recovery gate (cluster rejoin ≤ 2000 µs) ✅
  - [x] D-4: Security overhead gate (auth p99 ≤ 150 µs) ✅
  - [x] D-5: Wave 5/6 regression suites retained ✅
  - [x] D-6: Top-risk modules no new CRITICAL findings ✅
  - [x] D-7: Public API & failure-behaviour docs aligned ✅
  - [x] D-8: Release governance docs synchronized ✅
  - [x] D-9: Doxygen 100% public API coverage audit ✅
  - [x] D-10: Research Soll-Ist matrix complete ✅

- [x] Batch E gates E-1 through E-5 all marked PASS
  - [x] E-1: CDC Phase 5-6 complete ✅
  - [x] E-2: Prompt Engineering Phase 3-6 benchmarks created ✅
  - [x] E-3: Geo Phase 5-6 complete ✅
  - [x] E-4: Chimera Phase 5 benchmarks created ✅
  - [x] E-5: Graph Phase 5 benchmarks created ✅

- [x] Content Batch 5 gates CMT-7504/7505/7506 ✅ COMPLETE 2026-08-24
  - [x] CMT-7504-04: Documentation linkset framework evidenced ✅
  - [x] CMT-7505-03/04: Test coverage correlation complete (27 test files) ✅
  - [x] CMT-7506: GA sign-off section §8.1 populated ✅ (two-reviewer approval pending — non-blocking)

- [~] GA Batch D Section 9 human sign-off status
  - [ ] Release approver signature required: ❌ AWAITING
  - [x] Deferred items (DEF-01..04) documented and accepted: ✅

### Operability Infrastructure Setup
- [x] `docs/operability/` directory created
- [x] `docs/operability/WAVE_D_ROADMAP.md` established
- [x] `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` established (this file)
- [x] 5 operator runbooks published: `RUNBOOK_ACCESS_MODEL_PROMOTION.md`, `RUNBOOK_GPU_FALLBACK_PERFORMANCE.md`, `RUNBOOK_REPLICATION_LAG_FAILOVER.md`, `RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md`, `RUNBOOK_VOICE_INCIDENT_TRIAGE.md`
- [ ] `docs/operability/README.md` created with directory index
- [ ] Wave D markers added to root ROADMAP.md (template: Phase-6B for observability modules)

---

## Phase 2: Observability Expansion (Target: 2026-09-15)

### 2A: Distributed Tracing SDK Wrapper Pattern
- [ ] `include/observability/distributed_tracing.h` created
  - [ ] `DistributedTraceSpan` class with baggage, timing, parent-child linking
  - [ ] W3C Trace Context propagation support
  - [ ] Thread-safe span creation and closure
  - [ ] Doxygen documentation complete
- [ ] Transaction module integration
  - [ ] `src/transaction/coordinator.cpp` instrumented (beginTxn, commitTxn, rollbackTxn)
  - [ ] SAGA retry span tracking
  - [ ] Byzantine/timeout failure trace patterns documented
  - [ ] `tests/transaction/test_tracing_integration.cpp` created (≥10 test cases)
- [ ] Sharding module integration
  - [ ] `src/sharding/shard_router.cpp` instrumented (routeWrite, rebalanceTopology)
  - [ ] Shard discovery latency spans
  - [ ] Topology change baggage logging
  - [ ] `tests/sharding/test_tracing_integration.cpp` created (≥10 test cases)
- [ ] Replication module integration
  - [ ] `src/replication/wal_shipper.cpp` instrumented (shipToReplica, failover)
  - [ ] Cross-region lag quantile spans
  - [ ] Lag-alert decision trace annotations
  - [ ] `tests/replication/test_tracing_integration.cpp` created (≥10 test cases)
- [ ] Evidence: `src/observability/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md` completed
  - [ ] Span creation correctness verified
  - [ ] Baggage propagation cross-module verified
  - [ ] Parent-child linking verified
  - [ ] Zero performance regression vs. Wave 7 baseline (≤2% overhead)

### 2B: High-Cardinality Metrics Collection
- [ ] `src/observability/metrics_collector.cpp` extended with:
  - [ ] Shard-level write latency histogram (p50/p95/p99)
  - [ ] Shard-level read latency histogram
  - [ ] Replication lag gauge (p95/p99 by replica)
  - [ ] WAL shipping throughput (events/sec by shard pair)
  - [ ] Transaction retry counter (by reason)
  - [ ] GPU fallback counter (by cause)
- [ ] Cardinality limits defined and enforced
  - [ ] Shard ID label cardinality ≤ cluster topology size
  - [ ] Replica ID label cardinality ≤ replica set size
  - [ ] Failure reason labels (enumerated set)
- [ ] `tests/observability/test_metrics_cardinality.cpp` created
  - [ ] Cardinality explosion scenario verified (10k dimensions → aggregation)
  - [ ] Memory usage bounded under sustained collection
- [ ] Evidence: `src/observability/PHASE2B_METRICS_COLLECTION_VERIFICATION.md`
  - [ ] Metrics correlation with Wave 7-9 gate evidence verified
  - [ ] Operator interpretation guide documented

### 2C: OpenTelemetry Exporter Reliability Test Suite
- [ ] `tests/observability/test_otel_exporter_stress.cpp` created
  - [ ] Test 1: 1000 spans/sec × 60 sec, zero drops verified
  - [ ] Test 2: Network failure + recovery, buffering verified
  - [ ] Test 3: Cardinality explosion (10k metric dimensions), memory bounded
  - [ ] Test 4: Concurrent exporters, no duplicates verified
  - [ ] Test 5: Graceful shutdown, zero data loss verified
  - [ ] Wired into `release_critical;observability` label
- [ ] `tests/observability/test_high_cardinality_metrics_stress.cpp` created
  - [ ] Test 1: 50k metric dimensions/2 min, aggregation verified
  - [ ] Test 2: Metric scrape under lock contention, p99 latency measured
  - [ ] Test 3: Metric rotation during active emission, continuity verified
  - [ ] Wired into `release_critical;observability` label
- [ ] Evidence: `src/observability/PHASE2C_EXPORTER_RELIABILITY_VERIFICATION.md`
  - [ ] Test results: all 8 test scenarios pass
  - [ ] Performance baselines: sustained throughput, recovery latency documented
  - [ ] Resource usage: memory/CPU utilization under peak load documented

### 2D: Operator Remediation Hints Documentation
- [ ] `docs/operability/remediation_hints/TRANSACTION_REMEDIATION.md`
  - [ ] Byzantine vs. timeout detection (trace pattern decision tree)
  - [ ] SAGA retry storm indicators and mitigation (backoff tuning guide)
- [ ] `docs/operability/remediation_hints/SHARDING_REMEDIATION.md`
  - [ ] Exact-path gate failure recovery (topology refresh triggers)
  - [ ] Rebalance stall detection and intervention (manual override steps)
- [ ] `docs/operability/remediation_hints/REPLICATION_REMEDIATION.md`
  - [ ] Lag spike causes and recovery (WAL backpressure, network diagnosis)
  - [ ] Failover decision thresholds and manual override (command reference)
- [ ] Voice module hints (placeholder + outline created)
- [ ] GPU module hints (placeholder + outline created)
- [ ] Evidence: `docs/operability/PHASE2D_REMEDIATION_HINTS_COMPLETE.md`
  - [ ] All hints reviewed by respective module owners
  - [ ] Cross-references to tracing/metrics verified

---

## Phase 3: Runbook Development (Target: 2026-10-15)

### 3.1: Access-Model Promotion Workflow
- [x] `docs/operability/RUNBOOK_ACCESS_MODEL_PROMOTION.md` skeleton created
- [ ] Content complete:
  - [ ] Promotion prerequisites checklist
  - [ ] Dry-run procedure with validation
  - [ ] Canary rollout steps
  - [ ] Full rollout procedure
  - [ ] Rollback detection criteria and steps
  - [ ] Troubleshooting table (≥5 common issues)
  - [ ] Evidence/logging checklist

### 3.2: Replication Lag/Failover Detection & Recovery
- [x] `docs/operability/RUNBOOK_REPLICATION_LAG_FAILOVER.md` skeleton created
- [ ] Content complete:
  - [ ] Lag metric interpretation guide
  - [ ] Alert threshold configuration
  - [ ] Root cause identification procedures
  - [ ] Failover trigger conditions and safety checks
  - [ ] Recovery verification steps
  - [ ] Incident post-mortem template
  - [ ] Troubleshooting table (≥6 lag spike scenarios)

### 3.3: Sharding Topology Change & Rebalance
- [x] `docs/operability/RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md` skeleton created
- [ ] Content complete:
  - [ ] Topology change prerequisites (quorum, shard readiness)
  - [ ] Rebalance execution monitoring
  - [ ] Backpressure handling and stall recovery
  - [ ] Completion validation (consistency checks)
  - [ ] Performance normalization verification
  - [ ] Rollback conditions and procedure
  - [ ] Troubleshooting table (≥5 rebalance failure modes)

### 3.4: Voice Incident Triage & Session Lifecycle
- [x] `docs/operability/RUNBOOK_VOICE_INCIDENT_TRIAGE.md` skeleton created
- [ ] Content complete:
  - [ ] Session lifecycle tracing (setup → active → teardown)
  - [ ] Failure mode catalog (malformed streams, oversized payloads, timeouts)
  - [ ] Liveness/anti-spoof verification procedures
  - [ ] Multi-session concurrent handling and cleanup
  - [ ] Incident response checklist
  - [ ] Troubleshooting table (≥5 session lifecycle issues)

### 3.5: GPU Fallback Detection & Performance Monitoring
- [x] `docs/operability/RUNBOOK_GPU_FALLBACK_PERFORMANCE.md` skeleton created
- [ ] Content complete:
  - [ ] GPU health monitoring (CUDA error patterns, kernel timeout detection)
  - [ ] Fallback trigger procedures and safety gates
  - [ ] CPU baseline performance acceptance criteria
  - [ ] Recovery strategies (GPU re-init, warm-up)
  - [ ] Performance degradation assessment
  - [ ] Troubleshooting table (≥5 GPU failure scenarios)

### Runbook Cross-Reference
- [ ] `docs/operability/README.md` created with directory index and runbook links
- [ ] All runbooks cross-linked and discoverable from central ops hub
- [ ] Runbook version control and update procedures documented

---

## Phase 4: Soak Test Suite & Long-Duration Validation (Target: 2026-11-15)

### 4.1: Telemetry Exporter Resilience (24-hour)
- [x] `benchmarks/wave9/test_wave9_otel_24h_soak.cpp` skeleton created
- [ ] Implementation complete:
  - [ ] Continuous metric/trace emission at realistic rates
  - [ ] Network disruption injection (random intervals)
  - [ ] Exporter recovery verification
  - [ ] Data completeness validation
  - [ ] Streaming result collection (avoid memory bloat)
  - [ ] Wired into Wave 9 benchmark suite with configurable duration

### 4.2: Replication WAL Shipping & Lag Tracking (48-hour)
- [x] `benchmarks/wave9/test_wave9_replication_48h_soak.cpp` skeleton created
- [ ] Implementation complete:
  - [ ] Multi-region WAL shipping simulation
  - [ ] Periodic topology changes (failover injections)
  - [ ] Lag measurement and quantile tracking
  - [ ] Recovery validation after failover
  - [ ] Data consistency proof collection
  - [ ] Evidence: lag baselines, recovery times

### 4.3: Distributed Multi-Shard Writes with Topology Changes
- [x] `benchmarks/wave9/test_wave9_sharding_topology_soak.cpp` skeleton created
- [ ] Implementation complete:
  - [ ] Sustained writes across 8+ shards
  - [ ] Topology change injection (every 30 minutes)
  - [ ] Recovery time measurement and stall detection
  - [ ] Write latency impact tracking
  - [ ] Exactness guarantees verified
  - [ ] Streaming results collection

### 4.4: Mixed Acceleration Workloads (GPU + CPU)
- [x] `benchmarks/wave9/test_wave9_mixed_acceleration_soak.cpp` skeleton created
- [ ] Implementation complete:
  - [ ] Sustained GPU workloads with fallback injections
  - [ ] Fallback recovery latency measurement
  - [ ] Performance normalization tracking
  - [ ] CPU baseline establishment under mixed load
  - [ ] Graceful shutdown with final report

### Soak Test Integration
- [ ] All 4 soak tests wired into Wave 9 benchmark manifest
- [ ] Configurable duration override (default 24/48 hours, override to 2 hours for CI)
- [ ] Periodic health checks and failure injection framework
- [ ] Graceful shutdown and evidence report generation

---

## Phase 5: Validation & Documentation (Target: 2026-12-15)

### Integration & Verification
- [ ] Observability implementation integration tests
  - [ ] Trace context propagation across module boundaries verified
  - [ ] Metrics collection under chaos conditions verified
  - [ ] Wave 7-9 gate compatibility confirmed
- [ ] Runbook UAT with operations team
  - [ ] All 5 runbooks reviewed by ops lead
  - [ ] Feedback incorporated and procedures validated
  - [ ] UAT sign-off document obtained
- [ ] Soak test evidence collection
  - [ ] All 4 soak tests executed (24-48 hour minimum runs)
  - [ ] Failure modes documented and recovery verified
  - [ ] Performance baselines and outliers noted
  - [ ] Evidence artifacts collected: logs, metrics, traces

### ROADMAP & Governance Synchronization
- [ ] Root `ROADMAP.md` updated
  - [ ] Wave D section markers changed from [~] to [x] (in progress → complete)
  - [ ] Phase 2-5 completion dates recorded
  - [ ] Program-level success criteria cross-check
- [ ] `FUTURE_ENHANCEMENTS.md` updated
  - [ ] Wave D program sequencing section marked complete
  - [ ] Deferred items (if any) documented in Wave D+ planning
- [ ] `docs/governance/GA_PROMOTION_SIGN_OFF.md` updated
  - [ ] Wave D closure gate added to promotion checklist (new section or appendix)
  - [ ] Evidence artifacts indexed

### Wave D Sign-Off Document
- [ ] `docs/operability/WAVE_D_SIGN_OFF.md` created
  - [ ] Executive summary: program scope, phases, completion status
  - [ ] Gate verification matrix (all Wave D exit criteria marked PASS)
  - [ ] Evidence index (observability integration, runbooks, soak test results)
  - [ ] Operations team acceptance signature
  - [ ] Architecture lead acceptance signature
  - [ ] Release manager approval (for GA promotion gating)

---

## 6. Success Metrics

| Category | Gate | Target | Status |
|----------|------|--------|--------|
| **Observability** | Distributed tracing coverage | Transaction/Sharding/Replication ✅ | ❌ PENDING |
| | High-cardinality metrics | Shard/replica/failure-reason labels | ❌ PENDING |
| | Exporter stress tests | 1000 spans/sec sustained, 50k metric dims | ❌ PENDING |
| **Runbooks** | Completeness | 5 runbooks published | ❌ PENDING |
| | Accuracy | Operations UAT sign-off | ❌ PENDING |
| | Cross-linking | All runbooks discoverable from ops hub | ❌ PENDING |
| **Soak Tests** | Telemetry exporter | 24-hour minimum, 0 data loss | ❌ PENDING |
| | Replication | 48-hour minimum, lag tracking verified | ❌ PENDING |
| | Sharding | Topology changes without stall | ❌ PENDING |
| | Acceleration | GPU fallback cycles + recovery | ❌ PENDING |
| **Integration** | Wave 7-9 compat | All benchmark gates compatible | ❌ PENDING |
| | Governance sync | ROADMAP/GA gates updated | ❌ PENDING |
| | Sign-off | Operations + Architecture + Release approval | ❌ PENDING |

---

## 7. Dependencies & Blockers

| Dependency | Status | Impact | Mitigation |
|------------|--------|--------|-----------|
| Wave A-C exit criteria | 🟡 IN-PROGRESS (background verify) | Must complete before Wave D official start | Parallelize Phase 2 prep work now |
| GA Batch D sign-off | 🔴 AWAITING (Section 9) | Required for v2.4.0 GA promotion | Track sign-off path separately |
| Observability SDK finalization | 🟢 READY (infrastructure exists) | Enables Phase 2A span instrumentation | Start Phase 2A parallel to verification |
| Runbook UAT scheduling | 🟡 PENDING | Phase 3 completion blocker | Schedule early (Week 1 Phase 3) |

---

**Document Owner:** platform-release@themisdb  
**Last Updated:** 2026-08-15  
**Next Checklist Review:** End of Phase 1 (2026-08-31)
