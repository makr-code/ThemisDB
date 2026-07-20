# Next-Phase 30/60/90 Backlog (Production-Readiness Execution)

**Branch target:** `develop`  
**Cadence:** weekly Friday phase-gate (build/test/bench/security)  
**Execution mode:** larger delivery blocks (A→E), no micro-fix sequencing

---

## Day 0-30 (Kickoff + Block A/C/D Start)

### Block A — P3-01 + P3-02 (Graph optimizer + cache)
- [ ] P3-01 Query optimizer hardening (plan cache + cost model)
- [ ] P3-02 Cache efficiency (multi-tier/LRU/weighted eviction)
- [ ] Baseline and delta evidence recorded (latency/cache-hit)
- [ ] Gate: no regression vs existing graph suite, Wave-7 stays PASS

### Block C — P5-S01 + P5-S02 (Server hardening)
- [ ] Wire protocol retry/backoff hardening
- [ ] HTTP timeout + graceful shutdown hardening
- [ ] Fault-injection gate established and documented
- [ ] Gate: transient recovery stable, no shutdown/connection leaks

### Block D — P5-L01 + P5-L02 (LLM hardening)
- [ ] Exception safety closure on critical paths
- [ ] Leak-free lifecycle for load/unload/cache
- [ ] Baseline memory profile + post-fix comparison archived
- [ ] Gate: robust failure paths, memory gate green

### Cross-cutting kickoff
- [ ] Run `scripts/next_phase_kickoff.py` with available wave7/baseline artifacts
- [ ] Open/refresh issue set for P3, P5-S, P5-L, P6, AQL-Phase-2
- [ ] Confirm owners and acceptance gates per block

---

## Day 31-60 (Block B + E Start)

### Block B — P3-03 + P3-04 (Resource pooling + scheduling)
- [ ] Resource pooling hardening (connection/thread/buffer)
- [ ] Load-balancing + scheduling hardening
- [ ] Integration and regression verification against existing graph flow
- [ ] Gate: no performance regressions, deterministic behavior under load

### Block E — P6-01 + P6-02 + P6-03 (Sharding consistency + fault injection)
- [ ] 2PC/3PC consistency and recovery hardening
- [ ] Failover/rebalancing risk closure
- [ ] Wave-8 soak/partition/degradation scenarios executed
- [ ] Gate: recovery SLO met, no core consistency violations

### AQL 2.0 parallel lane
- [ ] Phase 2 DDL implementation and gate tests
- [ ] Follow-up sequencing for geospatial/FTS
- [ ] Breaking-change tracking kept in sync

---

## Day 61-90 (Promotion Readiness Window)

### System-wide readiness closure
- [ ] Cluster-level chaos/fault injection checklist items closed
- [ ] 99.99% uptime validation evidence captured
- [ ] Penetration test report attached to release decision package
- [ ] GPU-CI + cross-backend consistency fully green/stable

### Governance and promotion gates
- [ ] Weekly gate history complete and auditable
- [ ] No promotion without fulfilled gate criteria
- [ ] Promotion target path confirmed:
  - [ ] v1.9.0-beta (post P3/P5 + Wave-7/8)
  - [ ] v2.0.0-rc1
  - [ ] v2.0.0 GA

---

## Block Execution Order (Binding)

1. **Block A:** P3-01 + P3-02  
2. **Block B:** P3-03 + P3-04  
3. **Block C:** P5-S01 + P5-S02  
4. **Block D:** P5-L01 + P5-L02  
5. **Block E:** P6-01 + P6-02 + P6-03

After each block:
- [ ] Full gate-check completed (build/test/bench/security)
- [ ] Risks and regressions triaged before next block starts

