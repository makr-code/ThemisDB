> **Hinweis:** Vage Eintraege ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Storage Module - Future Enhancements

## Scope
- Reliability hardening for RocksDB/MVCC/WAL/backup/PITR execution paths.
- Blob backend resilience and migration safety across local and cloud backends.
- Performance and lifecycle hardening for write-heavy and mixed analytical workloads.

## Design Constraints
- [ ] No storage write path may bypass audit/safety checks in production mode (Target: ongoing)
- [ ] Migration operations must stay copy-then-delete with deterministic rollback semantics (Target: Q4 2026)
- [ ] Public storage APIs remain additive-only in active major lines (Target: ongoing)
- [ ] Durability and recovery behavior must be verifiable under fault-injection scenarios (Target: Q4 2026)
- [ ] Long-running background jobs must support bounded shutdown and safe interruption (Target: Q4 2026)

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `RocksDBWrapper` lifecycle and write APIs | StorageEngine, managers | close/open/write invariants and safety checks |
| `MVCCStore` transaction APIs | query/storage paths | snapshot consistency and retry behavior |
| `WALStorage` append/replay paths | recovery and PITR | deterministic replay under partial failure |
| `BlobRedundancyManager` | blob backend orchestration | failover/re-replication correctness |
| `BackupManager` / `PITRManager` | operations and restore flows | restore integrity and bounded recovery time |
| `StorageAuditLogger` | write/delete/maintenance paths | traceability without sensitive leakage |

## Implementation Notes

### Concurrency and Lifecycle Hardening
**Priority:** High
**Target:** Q3-Q4 2026

- Expand no-operation-after-close verification across wrapper entry points and background tasks.
- Harden concurrent write + maintenance interactions (compaction, pruning, migration).
- Keep graceful shutdown behavior deterministic under heavy operation counts.

### Durability and Recovery Hardening
**Priority:** High
**Target:** Q4 2026

- Strengthen durability defaults and operator guardrails for non-sync configurations.
- Extend WAL/PITR replay validation for interruption and restart edge cases.
- Add explicit recovery acceptance criteria tied to reproducible tests.

### Blob and Tier Reliability Hardening
**Priority:** Medium
**Target:** Q4 2026

- Expand multi-backend failover and re-replication chaos coverage.
- Validate tier migration correctness with concurrent read/write pressure.
- Add clearer observability around migration lag and failure states.

### Advanced Backend Hardening
**Priority:** Medium
**Target:** Q1 2027

- Improve persistent vector index backend depth and operational guarantees.
- Complete tensor-storage backend and benchmark hardening milestones.
- Tighten online schema migration guardrails for large datasets.

## Test Strategy
- Focused regression suites for concurrency, shutdown, and durability.
- Fault-injection matrix for backend outages, replay interruptions, and migration failures.
- Recovery drills for backup/PITR with consistency verification.
- Performance regression checks for write amplification and compaction pressure.

## Performance Targets
- Maintain stable p99 latency and throughput envelopes under mixed workloads.
- Keep compaction and migration overhead within release regression budgets.
- Keep audit/signature overhead bounded for high-ingest scenarios.

## Security / Reliability
- Fail closed on invalid encryption/signature/runtime safety configuration.
- Avoid partial visibility of multi-step storage operations.
- Ensure recoverability and auditability of critical write paths.

## Risk Backlog

### Risk 1: Blob failover edge-case data loss
**Severity:** High
**Signal:** Multi-backend outage/recovery sequence leaves blobs temporarily unavailable or inconsistent.
**Mitigation:** expanded chaos tests and deterministic re-replication validation.

### Risk 2: Durability misconfiguration drift
**Severity:** Medium
**Signal:** Non-sync or permissive runtime modes remain enabled in production-like deployments.
**Mitigation:** stricter defaults, startup warnings, and deployment policy checks.

### Risk 3: Lifecycle race regressions
**Severity:** Medium
**Signal:** Concurrent operations overlap with close/shutdown paths causing instability.
**Mitigation:** guard invariants, stress tests, and explicit lifecycle gates.

## Adoption Scenarios

### Scenario A: Reliability-first lane
- Prioritize durability/recovery hardening and replay guarantees.
- Promote only after fault-injection and recovery drills pass.

### Scenario B: Throughput-first lane
- Prioritize write-path performance and compaction predictability.
- Promote only with validated regression budgets.

### Scenario C: Extensibility-first lane
- Prioritize vector/tensor/persistent backend maturation.
- Promote only with safety and lifecycle guarantees in place.
