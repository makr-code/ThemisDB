# Block E Quick Reference — P6-01/P6-02/P6-03 Sharding Hardening

**Status:** 🎯 READY FOR KICKOFF (pending Block D CI gates)  
**Duration:** 3 weeks (2026-08-10 to 2026-08-31)  
**Teams:** E (P6-01/02), F (P6-03)  
**Total Tests:** 92+ (32 consistency + 20 failover + 40+ chaos)

---

## One-Line Summaries

| Phase | Objective | Test Count | Success Criterion |
|-------|-----------|------------|-------------------|
| **P6-01** | 2PC/3PC transaction consistency across shards | 32 | Zero divergence in any scenario |
| **P6-02** | Failover & rebalancing recovery | 20 | RTO≤30s, RPO≤5s, zero data loss |
| **P6-03** | Wave 8: 40+ distributed chaos scenarios | 40+ | 99.99% uptime, consistent state |

---

## Key Test Profiles

### P6-01: Consistency Checks

**2PC (16 cases):** happy path → abort → timeout → crash → recovery → burst  
**3PC (16 cases):** precommit phase → dual-coordinator → partition → rebalance

### P6-02: Failover Targets

**Failover (12):** leader crash → follower crash → cascade → re-election  
**Rebalancing (8):** add/remove shard → RTO/RPO verification

### P6-03: Wave 8 Chaos

**Network (10):** partition → latency → packet loss → asymmetric → flaky  
**Node (10):** crash → recovery → GC pause → memory pressure → disk stall  
**Combined (12):** partition+crash → cascade+rebalance → write burst+partition  
**Soak (8):** 1-hour soak → read/write sweep → hotspot → large txns → GC tuning

---

## Acceptance Gates (Blocking Promotion)

### Per-Phase
- [ ] P6-01: 32 tests pass, zero consistency violations
- [ ] P6-02: 20 tests pass, RTO/RPO targets met
- [ ] P6-03: 40+ scenarios pass, 99.99% uptime

### Final (v2.0.0-rc1)
- [ ] All 92+ tests passing
- [ ] No performance regression vs Wave 7
- [ ] Security scan clean
- [ ] ROADMAP + docs updated

---

## Execution Sequence

```
Week 1 (Aug 10-16):  P6-01 design + infra setup + initial tests
                     P6-03 chaos harness + network scenarios
Week 2 (Aug 17-23):  P6-02 failover tests + RTO/RPO tuning
                     P6-03 node + combined faults
Week 3 (Aug 24-31):  Integration + soak tests + gate verification
```

---

## Documentation Tasks

- [ ] `src/sharding/ROADMAP.md` — Phase 6 completion sections
- [ ] `tests/sharding/P6_01_TRANSACTION_CONSISTENCY.md` — 32-case matrix
- [ ] `tests/sharding/P6_02_FAILOVER_RECOVERY.md` — RTO/RPO targets
- [ ] `benchmarks/wave8/WAVE8_SPECIFICATION.md` — 40+ fault catalog
- [ ] `CHANGELOG.md` — Block E delivery summary
- [ ] `docs/architecture/TRANSACTION_2PC_3PC_VERIFICATION.md` — design update

---

## CI Commands (Verification)

```bash
# Build Block D tests
cmake --preset community-release
ctest --preset community-release -L "llm;hardening;phase5"

# Bench baseline (Wave 7)
./benchmarks/wave7/bench_w7a_release_critical_signoff
./benchmarks/wave7/bench_w7b_endurance_soak

# Security gate
codeql_checker [files]
```

---

## File Checklist

### To Create (Block E)
- [ ] `tests/sharding/test_p6_01_transaction_consistency.cpp` (32 tests)
- [ ] `tests/sharding/test_p6_02_failover_recovery.cpp` (20 tests)
- [ ] `benchmarks/wave8/wave8_fault_injection_suite.cpp` (40+ scenarios)
- [ ] 3 design docs (P6_01, P6_02, WAVE8_SPECIFICATION)

### To Update
- [ ] `tests/sharding/CMakeLists.txt` — register new test targets
- [ ] `benchmarks/CMakeLists.txt` — register Wave 8 suite
- [ ] `ROADMAP.md` — Block E status
- [ ] `CHANGELOG.md` — delivery notes

---

## Risk Summary

| Risk | Mitigation |
|------|-----------|
| Txn log divergence | Quorum verification + WAL checksums |
| RTO > 30s | Leader election optimization |
| Cascading failures | Quorum requirements |
| Memory leaks | ASan + long-running monitoring |

---

**Quick Link:** Full plan at `ai_working/BLOCK_D_GATE_VERIFICATION_AND_BLOCK_E_PLAN.md`
