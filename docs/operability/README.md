# Wave D: Operability & Runbooks

**Directory Purpose:** Wave D program coordination, operator runbooks, and distributed tracing/observability hardening  
**Target Release:** Q1 2027 (ThemisDB v2.4.x GA + v2.5.0-rc1)  
**Status:** 🟡 Phase 1-2 In Progress (updated 2026-08-24)  

---

## Quick Navigation

### Program Coordination
- **[WAVE_D_ROADMAP.md](WAVE_D_ROADMAP.md)** — Detailed Wave D implementation plan (Phases 1-5)
- **[WAVE_D_ACCEPTANCE_CHECKLIST.md](WAVE_D_ACCEPTANCE_CHECKLIST.md)** — Exit gate checklist and success metrics

### Operator Runbooks (Phase 3 — Target: 2026-10-15)
1. **[RUNBOOK_ACCESS_MODEL_PROMOTION.md](RUNBOOK_ACCESS_MODEL_PROMOTION.md)** — Access-model promotion workflow & rollback
2. **[RUNBOOK_REPLICATION_LAG_FAILOVER.md](RUNBOOK_REPLICATION_LAG_FAILOVER.md)** — Replication lag detection & failover recovery
3. **[RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md](RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md)** — Sharding topology changes & rebalancing
4. **[RUNBOOK_VOICE_INCIDENT_TRIAGE.md](RUNBOOK_VOICE_INCIDENT_TRIAGE.md)** — Voice incident triage & session lifecycle
5. **[RUNBOOK_GPU_FALLBACK_PERFORMANCE.md](RUNBOOK_GPU_FALLBACK_PERFORMANCE.md)** — GPU fallback detection & performance

### Remediation Hints (Phase 2D — Target: 2026-09-15)
- **remediation_hints/TRANSACTION_REMEDIATION.md** — Byzantine vs. timeout diagnosis, SAGA retry storms
- **remediation_hints/SHARDING_REMEDIATION.md** — Exact-path gate failures, rebalance stalls
- **remediation_hints/REPLICATION_REMEDIATION.md** — Lag spike causes, failover thresholds
- **remediation_hints/VOICE_REMEDIATION.md** — Session lifecycle issues (Phase 3)
- **remediation_hints/GPU_REMEDIATION.md** — CUDA failure modes, fallback recovery (Phase 3)

### Implementation Verification & Evidence
- **evidence/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md** — Distributed tracing integration results
- **evidence/PHASE2B_METRICS_COLLECTION_VERIFICATION.md** — High-cardinality metrics verification
- **evidence/PHASE2C_EXPORTER_RELIABILITY_VERIFICATION.md** — OpenTelemetry exporter stress test results
- **evidence/PHASE2D_REMEDIATION_HINTS_COMPLETE.md** — Remediation hints review sign-off
- **evidence/WAVE_D_SOAK_TEST_RESULTS.md** — Long-duration soak test evidence (Phase 4)

### Sign-Off & Closure
- **[WAVE_D_SIGN_OFF.md](WAVE_D_SIGN_OFF.md)** — Program completion sign-off with ops/architecture approvals (Phase 5)

---

## Wave D Program Overview

Wave D focuses on **Operability Hardening** — enabling operators to reliably run ThemisDB at scale with confidence. Unlike Wave A (reliability), Wave B (performance), and Wave C (security), Wave D emphasizes **visibility**, **runbook completeness**, and **sustained-load resilience**.

### Key Deliverables

#### 1. Observability Expansion (Phase 2 — Target: 2026-09-15)
- **Distributed Tracing:** W3C Trace Context propagation across transaction/sharding/replication with baggage (shard ID, transaction ID)
- **High-Cardinality Metrics:** Shard-level latencies, replication lag quantiles, retry/fallback counters (cardinality-safe)
- **Exporter Reliability:** Sustained 1000s spans/sec, network failure recovery, zero data loss under stress
- **Remediation Hints:** Decision trees for Byzantine vs. timeout detection, lag spike diagnosis, rebalance stall recovery

#### 2. Operator Runbooks (Phase 3 — Target: 2026-10-15)
Five comprehensive runbooks published with step-by-step procedures, decision trees, and troubleshooting tables:
- Access-model promotion workflow & rollback
- Replication lag/failover detection & recovery
- Sharding topology change & rebalance
- Voice incident triage & session lifecycle
- GPU fallback detection & performance monitoring

#### 3. Soak Tests (Phase 4 — Target: 2026-11-15)
Long-duration validated tests proving sustained operation:
- **Telemetry Exporter:** 24-hour continuous metric/trace emission with network disruption recovery
- **Replication:** 48-hour multi-region WAL shipping with periodic failover injection
- **Sharding:** Distributed writes across 8+ shards with topology changes (every 30 min)
- **Acceleration:** Mixed GPU + CPU fallback cycles with performance normalization tracking

#### 4. Integration & Governance (Phase 5 — Target: 2026-12-15)
- Observability integration with Wave 7-9 benchmark gates
- Runbook UAT and operator sign-off
- Soak test evidence collection (baselines, failure modes, recovery times)
- Root ROADMAP.md and GA sign-off updates
- Program closure and release approval

---

## How to Use This Directory

### For Operators & SREs
1. **Start here:** [RUNBOOK_REPLICATION_LAG_FAILOVER.md](RUNBOOK_REPLICATION_LAG_FAILOVER.md) (most common incident)
2. **Diagnosis:** Check [remediation_hints/REPLICATION_REMEDIATION.md](remediation_hints/REPLICATION_REMEDIATION.md) for lag spike patterns
3. **Reference:** Use [RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md](RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md) for topology work
4. **Training:** Review all 5 runbooks before on-call rotation

### For Architects & Release Managers
1. **Program Status:** Check [WAVE_D_ROADMAP.md](WAVE_D_ROADMAP.md) for current phase and completion estimates
2. **Acceptance Gates:** Verify [WAVE_D_ACCEPTANCE_CHECKLIST.md](WAVE_D_ACCEPTANCE_CHECKLIST.md) for exit criteria status
3. **Sign-Off:** Review [WAVE_D_SIGN_OFF.md](WAVE_D_SIGN_OFF.md) for governance approval (Phase 5)
4. **Evidence:** Check [evidence/](evidence/) folder for integration and soak test results

### For Developers (Observability Team)
1. **Architecture:** See WAVE_D_ROADMAP.md §3 for implementation phases
2. **Implementation Tasks:** Check WAVE_D_ACCEPTANCE_CHECKLIST.md §Phase 2 for deliverables
3. **Testing:** Review test requirements in PHASE2C sections
4. **Verification:** Follow evidence templates when documenting completion

---

## Phase Timeline

```
Phase 1: Prerequisite Verification (Aug 15 - Aug 31)
   │
   ├─ Wave A-C exit criteria verification
   ├─ GA Batch D sign-off status
   └─ Operability infrastructure (this directory)
   
Phase 2: Observability Expansion (Sep 1 - Sep 15)
   │
   ├─ Distributed tracing SDK wrapper pattern
   ├─ High-cardinality metrics collection
   ├─ OpenTelemetry exporter reliability tests
   └─ Operator remediation hints documentation
   
Phase 3: Runbook Development (Oct 1 - Oct 15)
   │
   ├─ 5 runbooks published with complete procedures
   ├─ Decision trees & troubleshooting tables
   └─ Cross-linking & ops hub integration
   
Phase 4: Soak Test Suite (Nov 1 - Nov 15)
   │
   ├─ Telemetry exporter 24-hour soak
   ├─ Replication 48-hour soak with failover
   ├─ Sharding topology change soak
   └─ Mixed acceleration workload soak
   
Phase 5: Validation & Documentation (Dec 1 - Dec 15)
   │
   ├─ Observability integration verification
   ├─ Runbook UAT with ops team
   ├─ Soak test evidence collection
   ├─ Governance synchronization
   └─ Wave D sign-off & release approval
```

**Target Completion:** Q1 2027 (v2.4.x GA + v2.5.0-rc1 readiness)

---

## Dependencies

### Prerequisites (Must be complete before Wave D official start)
- [~] **Wave A Exit Criteria:** Transaction/Voice/GPU in-progress (hardware pending Q4 2026); Sharding/Replication ✅ complete
- [~] **Wave B Exit Criteria:** Search ✅ / AccessModel ✅ complete; LLM Wiki RocksDB hardware evidence pending Q4 2026
- [x] **Wave C Exit Criteria:** Security ✅ / Audit ✅ / CI policy gates ✅ — ALL PASS 2026-08-18
- [ ] **GA Batch D:** Section 9 human sign-off obtained (D-1..D-10 technical gates PASS)

### Supporting Infrastructure (Already available)
- ✅ **Observability Stack:** `src/observability/` with OpenTelemetry, tracer.cpp, metrics_collector.cpp
- ✅ **Benchmark Waves:** Wave 7-9 infrastructure (gates, chaos, SLA measurement)
- ✅ **Runbook Base:** Existing runbooks in `docs/metadata/` and `docs/de/guides/`

---

## Governance & References

- **Root ROADMAP.md:** [§121-132](../../ROADMAP.md#wave-d--operability-hardening-q1-2027) — Wave D overview
- **FUTURE_ENHANCEMENTS.md:** [§81-84](../../FUTURE_ENHANCEMENTS.md#wave-d--operability-hardening-q1-2027) — Program sequencing
- **GA Promotion Sign-Off:** [docs/governance/GA_PROMOTION_SIGN_OFF.md](../governance/GA_PROMOTION_SIGN_OFF.md) — GA batch gates
- **Observability Module:** [src/observability/ROADMAP.md](../../src/observability/ROADMAP.md) — Observability implementation details

---

## Contact & Questions

- **Wave D Program Lead:** platform-release@themisdb
- **Observability Team:** observability-team@themisdb
- **Operations / SRE:** operations-sre@themisdb

---

**Last Updated:** 2026-08-24  
**Next Update:** Phase 2 implementation start (pending Wave A hardware confirmation)
