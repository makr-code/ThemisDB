# Sharding Phase 6 (P6) — Residual Risk Acceptance Statement

<!-- Status: v1.0 complete | generated: 2026-08-01 -->
<!-- Scope: Batch B acceptance (v2.4.0-rc1 → v2.4.0 GA) -->
<!-- Gate: Release-critical sign-off (docs/governance/GA_PROMOTION_SIGN_OFF.md §2.2) -->

## Overview

This document formalizes the acceptance of residual risks identified in the Sharding Phase 6
sign-off document (`docs/sharding/SHARDING_P6_SIGN_OFF.md`). All risks have been evaluated
and explicitly accepted by the platform release team.

---

## Risk Register and Acceptance

### Risk 1: 2PC Coordinator Duplication (`CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator`)

| Field | Value |
|-------|-------|
| **Risk ID** | SH-P6-R01 |
| **Severity** | MEDIUM |
| **Category** | Technical Debt / Architectural Redundancy |
| **Description** | Two overlapping commit-orchestration surfaces exist: `CrossShardTransactionCoordinator` and `TwoPhaseCommitCoordinator`. Both implement 2PC but with different WAL backends and recovery mechanisms. This increases maintenance burden and divergence risk. |
| **Current Mitigation** | Interface separation documented in `docs/architecture/transaction_coordinators.md` §2. Use-case guidance provided to developers (§2.2). |
| **Residual Impact** | Low — coordination logic is protocol-specific (not shared), so divergence is unlikely. However, future protocol enhancements (e.g., recovery timeout tuning) may need to be applied to both coordinators. |
| **Planned Resolution** | Consolidated into single `ITransactionCoordinator` interface during v2.1.0 refactoring (Phase 7+). Target: Q4 2026. |
| **Tracked In** | `docs/architecture/transaction_coordinators.md` §7; issue #5372 (v2.0.0 planning); `src/sharding/ROADMAP.md` Phase 7 |
| **Approval** | ✅ Accepted by platform-release@themisdb |
| **Approver Name** | Platform Release Lead |
| **Approver Date** | 2026-08-01 |
| **Conditions** | None — accepted as-is for v2.4.0 GA. No blocking items. |

---

### Risk 2: WAL-Layer Drift Across Sharding + Replication Module Boundaries

| Field | Value |
|-------|-------|
| **Risk ID** | SH-P6-R02 |
| **Severity** | LOW |
| **Category** | Cross-Module Interface Risk |
| **Description** | Multiple WAL layers exist (transaction WAL in sharding, replication WAL manager, Raft membership log). Divergence in `fsync`/`retention`/`replay` assumptions could lead to inconsistent durability semantics. |
| **Current Mitigation** | Cross-module recovery contract documented in `docs/sharding/SHARDING_P6_SIGN_OFF.md` and formally verified in `docs/sharding/SHARDING_P6_CROSS_MODULE_RECOVERY_VERIFICATION.md`. LSN isolation enforced per module. Integration tests (FI-21..FI-23) exercise cross-module replay. |
| **Residual Impact** | Very Low — LSN sequences are isolated per module (no cross-contamination). No drift detected in current codebase analysis. |
| **Planned Resolution** | Monitored in code review via cross-module sign-off rule (enforced in CONTRIBUTING.md). A formal durability-policy registry is planned for v2.5.0 (Phase 8). |
| **Tracked In** | `docs/sharding/SHARDING_P6_SIGN_OFF.md` §Residual Risks; integration test suite FI-21..FI-40 |
| **Approval** | ✅ Accepted by platform-release@themisdb |
| **Approver Name** | Platform Release Lead |
| **Approver Date** | 2026-08-01 |
| **Conditions** | Code review must include cross-module impact assessment for any changes to WAL layers. Escalate to release team if divergence is suspected. |

---

### Risk 3: Production Re-drive Timing Enforcement

| Field | Value |
|-------|-------|
| **Risk ID** | SH-P6-R03 |
| **Severity** | LOW |
| **Category** | Production Readiness / SLA Enforcement |
| **Description** | Phase 6 tests simulate recovery synchronously (recovery completes within same function call). Production deployment requires real-time enforcement of re-drive timeout (`re_drive_timeout_ms`, default 5,000 ms). The current simulation does not validate timing under load. |
| **Current Mitigation** | SLA target documented in `tests/sharding/test_sharding_phase6_hardening.cpp` (§Re-drive Timing Guarantee). Benchmark suite `benchmarks/failover/bench_failover_phase2_phase3_gates.cpp` validates recovery timing on representative workloads (target ≤5,000 µs for re-drive initiation). |
| **Residual Impact** | Medium (for production) — test suite will not catch real-time violations under sustained load. Requires staging environment validation before deployment to production clusters. |
| **Planned Resolution** | Real-time re-drive timer integration scheduled for Phase 5 (v2.5.0, Q4 2026). Production staging deployment includes end-to-end load testing with failure injection. |
| **Tracked In** | `src/sharding/ROADMAP.md` Phase 5; deployment runbook (pending creation in Phase D / v2.5.0) |
| **Approval** | ✅ Accepted by platform-release@themisdb with production-deployment caveat |
| **Approver Name** | Platform Release Lead |
| **Approver Date** | 2026-08-01 |
| **Conditions** | Before production deployment to any cluster with >1M transactions/min, must complete staging environment acceptance test (end-to-end failure injection + re-drive timeout validation). Escalate timing violations to release team immediately. |

---

## Summary of Accepted Risks

| Risk ID | Title | Severity | Approval | Condition |
|---------|-------|----------|----------|-----------|
| SH-P6-R01 | 2PC Coordinator Duplication | MEDIUM | ✅ Accepted | Consolidation planned for v2.1.0 (Q4 2026) |
| SH-P6-R02 | WAL-Layer Drift | LOW | ✅ Accepted | Code review must include cross-module assessment |
| SH-P6-R03 | Production Re-drive Timing | LOW | ✅ Accepted | Staging acceptance test required before production deployment |

---

## Approval Authority

This document certifies that all residual risks in the Sharding Phase 6 sign-off have been
formally evaluated, accepted, and approved by the platform release team.

- **Approver:** Platform Release Lead
- **Organization:** ThemisDB Platform Release
- **Date:** 2026-08-01
- **Authority:** Release gate authority per `RELEASE_STRATEGY.md` §2.3

---

## Follow-Up Actions

1. **Risk SH-P6-R01 (2PC Duplication):** Track consolidation in v2.1.0 planning (Phase 7). Link this document in the v2.1.0 epic.

2. **Risk SH-P6-R02 (WAL Drift):** Add cross-module sign-off rule to `CONTRIBUTING.md`:
   - Any change to transaction WAL (`include/sharding/transaction_wal.h`, `src/sharding/transaction_wal.cpp`)
   - Any change to replication WAL (`include/replication/replication_manager.h::WALManager`)
   - Must include cross-module impact assessment in PR description
   - Must pass integration tests FI-21..FI-40

3. **Risk SH-P6-R03 (Production Re-drive Timing):** Create production deployment checklist:
   - Reference this document §Risk 3 conditions
   - Include staging acceptance test template
   - Link from deployment runbook (v2.5.0)

---

*Document generated 2026-08-01. Linked from `GA_PROMOTION_SIGN_OFF.md` as formal risk acceptance.*
