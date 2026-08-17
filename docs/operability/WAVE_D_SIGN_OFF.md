# Wave D Operability Hardening — Sign-Off Document

**Status:** PENDING HUMAN SIGN-OFF
**Target:** Q1 2027
**Prerequisite:** Wave C Security Production Validation complete (Vault/HSM/PKI, audit hardening, CI policy gates)

---

## Sign-Off Gate

> **⚠️ Human sign-off required before closing the Wave D gate.**
> AI agents cannot self-approve this sign-off.
> A human maintainer must review and approve all evidence below before this
> gate is considered closed.

---

## Batch D1 — Distributed Tracing Integration

**Status:** 🔲 Open

**Evidence required:**
- [ ] `src/observability/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md` exists with
  verified trace span emission from `TransactionCoordinator::commit()`,
  `ShardRouter::routeWrite()`/`rebalanceTopology()`, `WALShipper::shipToReplica()`,
  and the failover detection path.
- [ ] W3C `traceparent` baggage propagated across all module boundaries (verified
  via distributed trace sample in evidence file).
- [ ] Parent-child span linking confirmed for multi-hop distributed writes.

**Reviewer notes:**
<!-- Human reviewer notes here -->

**Approved by:** _(human sign-off required)_
**Date approved:** _(pending)_

---

## Batch D2 — High-Cardinality Metrics + Exporter Reliability

**Status:** 🔲 Open

**Evidence required:**
- [ ] `src/observability/PHASE2B_METRICS_COLLECTION_VERIFICATION.md` exists with:
  - Per-shard write/read latency histograms (p50/p95/p99 per shard ID)
  - Replication lag gauges
  - WAL shipping throughput
  - Transaction retry counters
  - GPU fallback counters
- [ ] `src/observability/PHASE2C_EXPORTER_RELIABILITY_VERIFICATION.md` exists with:
  - 5-test exporter stress suite results: 1000 spans/sec sustained 60 s ✓
  - Network failure + recovery ✓
  - Cardinality explosion handling ✓
  - Concurrent exporters ✓
  - Shutdown flush ✓
- [ ] Cardinality bounds documented (shard/replica labels topology-bounded; failure-reason labels enumerated)

**Reviewer notes:**
<!-- Human reviewer notes here -->

**Approved by:** _(human sign-off required)_
**Date approved:** _(pending)_

---

## Batch D3 — Operator Runbooks × 5

**Status:** ✅ Complete — all 5 runbooks already exist in `docs/operability/`

**Evidence:**
- [x] `docs/operability/RUNBOOK_ACCESS_MODEL_PROMOTION.md`
- [x] `docs/operability/RUNBOOK_REPLICATION_LAG_FAILOVER.md`
- [x] `docs/operability/RUNBOOK_SHARDING_TOPOLOGY_CHANGE.md`
- [x] `docs/operability/RUNBOOK_VOICE_INCIDENT_TRIAGE.md`
- [x] `docs/operability/RUNBOOK_GPU_FALLBACK_PERFORMANCE.md`

**Remaining cross-link work:**
- [ ] Each runbook cross-links the relevant trace span names from Batch D1
  (can only be added after Batch D1 evidence is complete)

**Approved by:** _(pending Batch D1 cross-link update)_

---

## Batch D4 — Long-Duration Soak Tests

**Status:** 🔲 Open (tests exist; full soak run not yet executed)

**Test files created:**
- [x] `tests/integration/test_replication_soak_60min.cpp` — labels: `wave_d;soak;not_release_critical`
- [x] `tests/integration/test_sharding_distributed_write_soak.cpp` — labels: `wave_d;soak;not_release_critical`
- [x] `tests/integration/test_telemetry_soak.cpp` — labels: `wave_d;soak;not_release_critical`

**Evidence required for sign-off:**
- [ ] Full 60-minute `test_replication_soak_60min` run result:
  - Replication lag p99 ≤ 500 ms ✓ (evidence: CI run link or log attachment)
- [ ] Full soak `test_sharding_distributed_write_soak` run result:
  - No topology divergence across all 4 shards ✓
- [ ] Full 10-minute `test_telemetry_soak` run result:
  - Exporter buffer never full at 1000 events/sec ✓
  - Zero event loss ✓

**Reviewer notes:**
<!-- Attach soak test run logs or CI artifact links here -->

**Approved by:** _(human sign-off required)_
**Date approved:** _(pending)_

---

## Batch D5 — Wave D Gate Closure

**Status:** 🔲 Open — pending Batches D1–D4 completion and human sign-off

**Gate closure checklist:**
- [ ] Batch D1 approved (see above)
- [ ] Batch D2 approved (see above)
- [ ] Batch D3 runbook cross-links added
- [ ] Batch D4 full soak evidence attached (see above)
- [ ] `ROADMAP.md` Wave D items updated to `[x]`
- [ ] `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 (Batch D) updated with completion status
- [ ] Human maintainer approves this document (signature below)

---

## Final Sign-Off

> By approving this document, the signing maintainer certifies that all Wave D
> Operability Hardening batches (D1–D4) have been completed, evidence has been
> reviewed, and ThemisDB is ready to proceed past the Wave D gate.

**Gate approved by:** _(human maintainer — name + GitHub username)_
**Date:** _(date of approval)_
**Commit / PR reference:** _(PR number or commit SHA)_

---

## References

- `docs/operability/WAVE_D_ROADMAP.md` — Full Wave D execution plan
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — Detailed acceptance criteria (D-1..D-10)
- `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 — GA Batch D gate
- `ROADMAP.md` — Root Wave A→D gate model
