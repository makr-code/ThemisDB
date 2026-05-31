> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Storage Module Roadmap

## Current Status
Production-grade RocksDB-based storage stack with MVCC, WAL, backup/PITR, multi-backend blob storage, encryption/signature controls, and operational tooling in active use.

## In Progress
- [~] Storage hardening wave for concurrency, durability defaults, and migration safety (Target: Q3 2026)
  - [ ] Finalize remaining storage fault-injection and chaos coverage for failover and blob recovery paths (Target: Q3 2026)
  - [ ] Tighten durability and shutdown behavior checks for high-concurrency write paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Harden tiered-storage migration verification (copy-then-delete invariants, rollback semantics, observability) (Target: Q4 2026)
- [ ] Expand distributed rate and storage coordination tests for mixed shard/network failure patterns (Target: Q4 2026)
- [ ] Introduce stricter production durability presets and operator guardrails (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Persistent high-performance vector index backend beyond flat-scan baseline (Target: Q1 2027)
- [ ] Extended online schema migration safety checks for large datasets and long-running backfills (Target: Q1 2027)
- [ ] Tensor-storage phase hardening with backend and benchmark completion (Target: Q1 2027)

## Implementation Phases

### Phase 1: Concurrency and Lifecycle Safety
- [ ] Validate no-operation-after-close invariants across all RocksDB wrapper entry points (Target: Q3 2026)
- [ ] Expand stress tests for transaction + background maintenance interactions (Target: Q3 2026)

### Phase 2: Durability and Recovery
- [ ] Strengthen default durability profile guidance and runtime warnings for non-sync writes (Target: Q4 2026)
- [ ] Extend PITR/WAL replay validation under partial-failure scenarios (Target: Q4 2026)

### Phase 3: Blob and Tier Migration Reliability
- [ ] Add end-to-end chaos scenarios for backend outage, recovery, and re-replication (Target: Q4 2026)
- [ ] Validate migration correctness under concurrent read/write pressure (Target: Q4 2026)

### Phase 4: Performance and Capacity Hardening
- [ ] Re-baseline write amplification, compaction pressure, and cache hit targets on representative workloads (Target: Q1 2027)
- [ ] Add bounded-overhead guarantees for audit/signature paths under peak ingest (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [ ] Keep storage docs source-aligned after each hardening cycle with explicit sourcecode verification evidence (Target: ongoing)
- [ ] Keep completed roadmap work exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: focused storage tests, WAL/PITR suites, blob backend tests, performance benchmarks
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some chaos/fault-injection scenarios for multi-backend blob failover remain incomplete.
- Persistent vector-index backend depth is still limited beyond current in-memory baseline.
- Some long-horizon migration and distributed failure envelopes require additional evidence.

## Breaking Changes
- Storage public APIs in active major lines remain additive-first.
- Key schema and migration contracts require explicit compatibility handling before any breaking evolution.
