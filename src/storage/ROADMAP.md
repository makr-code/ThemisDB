# Storage Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable storage runtime exists for durable persistence, MVCC/WAL lifecycle behavior, backup/PITR flows, blob/tiering behavior, and storage audit/integrity surfaces.

## In Progress

- [~] hardening failure-path behavior under sustained write/load and maintenance overlap (Target: Q3 2026)
- [~] improving diagnostics consistency across storage, replay, and recovery stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for storage hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under heavy WAL replay and compaction pressure (Target: Q4 2026)
- [ ] expand stress coverage for blob/tiering and PITR edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for recovery and maintenance incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for write/replay/recovery-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for mount-latency and storage allocator edge paths (Target: Q1 2027)
- [ ] harden long-run reliability under sustained mixed read/write pressure (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity — 3.4 Storage** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

- [ ] **Tiered storage (hot/warm/cold) with automatic data migration**: implement age- and
  access-frequency-based tier migration; data that has not been accessed within a configurable
  window automatically demoted from hot (NVMe) → warm (HDD) → cold (object storage) (Target: Q4 2026)
  - Inputs: tier policy config (`storage.tier.hot_max_age_s`, `storage.tier.warm_max_age_s`,
    `storage.tier.cold_backend`); access-frequency tracker
  - Acceptance: migration runs without blocking foreground I/O; cold-to-hot promotion completes
    within 100 ms for objects ≤ 1 MB; `storage_tier_migration_total` counter wired; `release_critical` green
- [ ] **Cloud-native S3/GCS/Azure backend hardening**: harden the existing cloud backend adapter
  for sustained production use: retry with exponential backoff, integrity checksums (MD5/CRC32C),
  multipart upload for objects > 100 MB, SDK version pinning (Target: Q4 2026)
  - Acceptance: 72-hour soak at 1 000 object writes/s with zero data-integrity errors;
    multipart upload tested for 1 GB object; SDK versions pinned in vcpkg manifest

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze storage wrapper/engine/recovery contracts for current major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for durability and recovery incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for WAL/MVCC and backup/PITR internals (Target: Q4 2026)
- [ ] align tiered/blob/redundancy behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for replay faults, storage pressure, and recovery errors (Target: Q4 2026)
- [ ] unify diagnostics across persistence, maintenance, and recovery incident classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for replay, PITR, and tiered/blob edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for mixed write + maintenance workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for storage hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core storage module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core storage surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for durability/recovery edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on storage configuration, backend profile, and workload shape.
- selected replay/recovery/tiering edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced storage workloads.

## Breaking Changes

No breaking storage contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.