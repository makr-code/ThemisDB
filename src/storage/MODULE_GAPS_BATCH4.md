# storage — MODULE_GAPS.md (Batch 4 Wave A Analysis)

**Batch:** Tier 3 Batch 4  
**Wave:** A (Runtime Reliability First)  
**Module:** `src/storage` (1485 gaps identified)  
**Last Updated:** 2026-08-14  
**Status:** Gap categorization in progress (IMPL vs DOC phase)

## Gap Summary

| Metric | Value |
|---|---|
| **Total Gaps** | ~1485 |
| **Implementation Gaps (IMPL)** | ~900 (60%) |
| **Documentation Gaps (DOC)** | ~585 (40%) |
| **Critical Severity** | ~120 |
| **High Severity** | ~310 |
| **Medium Severity** | ~1055 |

## Gap Categorization: IMPL vs DOC

### Implementation Gaps (IMPL) — Code/Logic Gaps: ~900

**Categories:**
1. **Memory Safety:** ~180 gaps
   - Uninitialized variable access (MVCC snapshot lifecycle)
   - Buffer boundary checks (tiered storage tier migration)
   - Pointer arithmetic in WAL ring buffer
   - GPU memory handling (encrypted blob CUDA fallback)
   - Severity: HIGH → MEDIUM (most under guards)

2. **Concurrency & Thread Safety:** ~240 gaps
   - Lock ordering violations (WAL lock vs compaction lock; need formal verification)
   - Double-checked locking anti-pattern in backup manager
   - Race conditions in tiered storage access tracking
   - MVCC snapshot reference counting (potential leak under concurrent deletes)
   - Severity: CRITICAL → HIGH (require focused testing)

3. **Error Handling & Recovery:** ~280 gaps
   - Missing error propagation in crash recovery (WAL replay)
   - Incomplete exception safety in backup atomicity
   - Silent failure swallowing in compaction cleanup
   - Missing retry logic in cloud-backend PITR restore
   - Severity: HIGH → MEDIUM (mostly fallback paths)

4. **Resource Management:** ~150 gaps
   - File descriptor leaks (backup staging files not cleaned)
   - Connection pool exhaustion (tiered storage cold backend)
   - Compaction stall under memory pressure
   - VRAM allocation overflow in encrypted blob backend
   - Severity: MEDIUM → LOW (manageable with limits)

5. **Performance & Scalability:** ~150 gaps
   - O(n²) compaction scheduling in large key ranges
   - Repeated MVCC map lookups in write path
   - Redundant data copies in tiered migration
   - Unoptimized blob integrity checking
   - Severity: MEDIUM → LOW (under investigation)

### Documentation Gaps (DOC) — Documentation/Evidence: ~585

**Categories:**
1. **API Contract Documentation:** ~140 gaps
   - Missing determinism guarantees (WAL replay order, PITR timestamp accuracy)
   - Incomplete error code mapping (all 12 StorageErrorCode cases not fully documented)
   - Implicit assumptions in MVCC isolation level documentation
   - Severity: HIGH (affects module contract clarity)

2. **Thread-Safety Documentation:** ~120 gaps
   - Missing lock ordering diagram (MVCC, WAL, compaction hierarchy)
   - Incomplete concurrency model specification (which operations are lock-free?)
   - Undocumented blocking points (timeouts, deadlock risk)
   - Severity: HIGH (critical for integration teams)

3. **Failure Path Documentation:** ~150 gaps
   - Incomplete crash recovery sequence documentation
   - Missing backup restore failure scenarios
   - Undocumented tiered storage promotion failure handling
   - Incomplete PITR edge-case documentation
   - Severity: MEDIUM (affects operator runbooks)

4. **Behavior Contract Documentation:** ~100 gaps
   - Incomplete tiered storage migration semantics
   - Missing erasure-coded backend durability guarantees
   - Undocumented blob integrity verification process
   - Severity: MEDIUM (affects integration tests)

5. **Example & Integration Documentation:** ~75 gaps
   - Missing example code for MVCC snapshot lifecycle
   - Incomplete WAL configuration guide
   - Undocumented backup/restore workflow examples
   - Severity: LOW (affects developer onboarding)

## Wave A (Runtime Reliability) Focus Areas

### Critical Path 1: WAL/Durability Safety (IMPL + DOC)
- [ ] **IMPL Gap:** Verify WAL replay idempotence under sustained write load (proof of determinism)
- [ ] **IMPL Gap:** Close async WAL write path for cross-region replication (coordinate with replication module)
- [ ] **DOC Gap:** Document WAL ordering invariants and recovery determinism proof
- [ ] **Test Gate:** WAL-01 to WAL-06 focused tests (crash recovery with partial writes, replay idempotence)
- [ ] **Benchmark Gate:** WAL throughput ≥100k ops/s, latency p99≤500µs
- **Target:** Q3 2026 | **Severity:** CRITICAL

### Critical Path 2: MVCC Isolation & Concurrency (IMPL + DOC)
- [ ] **IMPL Gap:** Eliminate lock ordering violations (formal proof via TS analysis)
- [ ] **IMPL Gap:** Verify MVCC snapshot reference counting under concurrent deletes
- [ ] **DOC Gap:** Document lock ordering hierarchy and prove no circular waits
- [ ] **DOC Gap:** Document MVCC isolation level and read consistency guarantees
- [ ] **Test Gate:** MVCC-01 to MVCC-08 focused tests (concurrent reads/writes, snapshot lifecycle)
- [ ] **Benchmark Gate:** MVCC read p99≤100µs, write p99≤500µs, multi-thread contention <5% overhead
- **Target:** Q3 2026 | **Severity:** CRITICAL

### Critical Path 3: Crash Recovery Determinism (IMPL + DOC)
- [ ] **IMPL Gap:** Verify unclean shutdown recovery is deterministic (chaos injection tests)
- [ ] **IMPL Gap:** Close exception safety in recovery path (atomic or no-op principle)
- [ ] **DOC Gap:** Document recovery sequence and rollback semantics
- [ ] **DOC Gap:** Document failure scenarios and deterministic outcomes
- [ ] **Test Gate:** Recovery-01 to Recovery-06 focused tests (unclean shutdown, partial writes, corruption detection)
- [ ] **Benchmark Gate:** Recovery time ≤5s (100GB dataset), no data loss verification
- **Target:** Q3 2026 | **Severity:** CRITICAL

### Critical Path 4: Backup/PITR Atomicity & Restorability (IMPL + DOC)
- [ ] **IMPL Gap:** Verify backup is atomic under concurrent writes
- [ ] **IMPL Gap:** Close backup restoration failure handling (explicit error on any corruption)
- [ ] **IMPL Gap:** Verify PITR timestamp accuracy (±100ms bound)
- [ ] **DOC Gap:** Document backup/restore determinism and edge cases
- [ ] **DOC Gap:** Document PITR timestamp resolution and accuracy bounds
- [ ] **Test Gate:** Backup-01 to Backup-08 focused tests (concurrent backups, restore under load, PITR edge cases)
- [ ] **Benchmark Gate:** Backup time ≤30s (10GB dataset), restore time ≤20s
- **Target:** Q3–Q4 2026 | **Severity:** HIGH

### Critical Path 5: Tiered Storage Safety & Transparency (IMPL + DOC)
- [ ] **IMPL Gap:** Verify tiered migration never blocks foreground I/O
- [ ] **IMPL Gap:** Verify cold-to-hot promotion latency ≤100ms (1MB object)
- [ ] **DOC Gap:** Document tiered storage migration semantics (age-based, access-frequency-based)
- [ ] **DOC Gap:** Document tier configuration and performance expectations
- [ ] **Test Gate:** Tiering-01 to Tiering-06 focused tests (migration under load, promotion latency, access tracking accuracy)
- [ ] **Benchmark Gate:** Migration decision p99≤50µs, promotion p99≤100ms, no write stall
- **Target:** Q4 2026 | **Severity:** HIGH

## Wave A Closure Status

### Test Evidence Gates (Batch 4, Wave A)
- [ ] **STR-01 to STR-16:** Contract-hardening focused tests (WAL, MVCC, recovery, PITR)
- [ ] **STR-CHAOS-01 to STR-CHAOS-06:** Chaos injection tests (unclean shutdown, write storm, concurrent recovery)
- [ ] **STR-TIERING-01 to STR-TIERING-06:** Tiered storage focused tests (migration, promotion, no-block guarantee)
- **Target:** Q3 2026 | **Status:** In Progress

### Benchmark Gates (Batch 4, Wave A)
- [ ] **SGRG-01:** WAL throughput ≥100k ops/s (≥95th percentile)
- [ ] **SGRG-02:** MVCC read latency p99≤100µs
- [ ] **SGRG-03:** MVCC write latency p99≤500µs
- [ ] **SGRG-04:** Checkpoint latency p99≤10ms
- [ ] **SGRG-05:** Tiering decision latency p99≤50µs
- [ ] **SGRG-06:** Compaction check latency p99≤100µs
- **Target:** Q3 2026 | **Status:** In Progress

### `release_critical` CI Coverage (Batch 4, Wave A)
- [ ] Storage module: 100% Gate coverage on `develop` branch
- [ ] No regressions in WAL/MVCC/recovery hot paths
- [ ] Benchmark gates passing at ≥95th percentile
- **Target:** Q3 2026 | **Status:** Ongoing

## Priority Assessment and Action Plan

### P0 — Wave A Gate Blockers (resolve by Q3 2026 end)
1. **Lock ordering verification** (MVCC/WAL/compaction hierarchy) → Formal proof + focused tests
2. **WAL replay idempotence** (crash recovery determinism) → Chaos injection + proof
3. **MVCC isolation level verification** (concurrent reads/writes) → Concurrent read stress + verification
4. **Backup restoration failure handling** (no silent corruption) → Explicit error on any bad checksum
5. **Tiered storage no-block guarantee** (foreground I/O not blocked) → Concurrent I/O stress + latency monitoring

### P1 — Post-Wave-A Hardening (Q4 2026)
1. Cloud backend reliability hardening (retry logic, checksums, multipart upload)
2. PITR edge-case handling (timestamp resolution, range boundaries)
3. Erasure-coded backend durability verification
4. Compaction stall prevention under memory pressure

### P2 — Post-Wave-B Optimization (Q1 2027)
1. p95/p99 baseline refresh on representative hardware
2. Performance optimization for tiered storage migration (reduce CPU overhead)
3. Documentation depth expansion for operator runbooks

## Known Issues & Limitations

1. **Thread-safety proof:** Manual lock ordering verification (need formal TS analysis or Clang thread-safety annotations)
2. **Crash recovery testing:** Limited hardware availability for unclean shutdown scenarios; rely on chaos injection
3. **Tiered storage promotion latency:** Cold-to-warm promotion may exceed 100ms under high load; need tuning
4. **Concurrent backup limitations:** Only one concurrent backup per database instance (serialized through backup lock)
5. **PITR timestamp resolution:** Limited to millisecond precision; microsecond precision requires architectural change

## Cross-Module Dependencies

| Dependency | Module | Nature | Wave |
|---|---|---|---|
| Async WAL shipping | replication | Required for Batch A2 delivery | Wave A |
| Field-level encryption keys | security | Optional; affects encrypted blob backend | Wave C |
| CUDA memory management | acceleration | Optional; GPU-accelerated compaction | Wave B |
| Compliance audit trail | governance | Required for audit logging integration | Wave C |
| Access tracking signals | access_model | Optional; promotion hints for tiered storage | Wave B |

## Batch 4 Contribution to Program Success

This module contributes to **Wave A (Runtime Reliability)** by:
1. ✅ Ensuring ACID guarantees remain valid under fail-closed scenarios
2. ✅ Proving crash recovery determinism via chaos injection
3. ✅ Delivering benchmark-backed durability guardrails
4. ✅ Documenting explicit failure semantics for integration teams

**Gate Status for Wave A Exit:** 🟡 In Progress (P0 items resolve by Q3 2026 end)

---

**Next Steps:**
1. Execute P0 gap resolution (lock ordering, WAL idempotence, MVCC isolation) by EOQ3 2026
2. Deliver focused test gates (STR-01..STR-16, STR-CHAOS-01..06) by EOQ3 2026
3. Benchmark gates must pass at ≥95th percentile by EOQ3 2026
4. `release_critical` CI must remain green throughout Wave A execution
