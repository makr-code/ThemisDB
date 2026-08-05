# ThemisDB Phases 1A–5 Implementation Orchestration

**Document:** Master execution plan for post-GA closure work  
**Status:** 🟡 ACTIVE — Scope-freeze baseline (2026-08-05)  
**Target Completion:** Phase 1A (2026-08-11), Phases 2–5 (2026-09–2027-01)  
**Owner:** AI Delivery Agent + Human Release Approver

---

## Executive Summary

This document orchestrates the execution of Phases 1A through 5 in strict sequence with clear gate dependencies. Phase 1A (GA sign-off) must complete before any Phase 2–5 work is promoted to the `community` release lane. Phases 2–5 can execute in parallel against `develop` once their per-phase gates pass.

### Key Timeline

| Phase | Name | Duration | Target End | Dependencies |
|-------|------|----------|-----------|--------------|
| 1A | GA Promotion Sign-Off Closure | 1-2 weeks | 2026-08-11 | NONE (sequential gate) |
| 2 | Performance & Scalability Readiness | 6-8 weeks | 2026-09-30 | Phase 1A complete |
| 3 | Plugin Externalization & Monetization | 8-10 weeks | 2026-10-31 | Phase 1A complete (parallel OK) |
| 4 | Observability & Operations Hardening | 6-8 weeks | 2026-10-15 | Phase 1A complete (parallel OK) |
| 5 | LLM Wiki Enterprise Plugin Hardening | 8-10 weeks | 2026-10-31 | Phase 1A complete (parallel OK) |

---

## Phase 1A — GA Promotion Sign-Off Closure

### Scope

Complete the blocking human governance gate in `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9 before promoting v2.4.0 GA to the `community` release lane.

### Deliverables

1. ✅ Section 9 human sign-off completed by release approver
2. ✅ All Batch D gates (D-1 through D-10) re-verified on current `develop` HEAD
3. ✅ Wave 7 + Wave 9 hard gate evidence confirmed PASS
4. ✅ Release tag `v2.4.0` created on `community` branch
5. ✅ CHANGELOG.md entry published for v2.4.0 GA
6. ✅ research/implementation_influence/by_module.md Soll-Ist matrix verified

### Acceptance Criteria

- [ ] `develop` HEAD passes full `release_critical` CTest suite
- [ ] All Wave 7 hard gates (GATE-W7-01..06) confirmed PASS
- [ ] All Wave 8 hard gates (GATE-W8-01..04) confirmed PASS
- [ ] All Wave 9 hard gates (GATE-W9-01..06) confirmed PASS
- [ ] Security Lead reviews and accepts GA_SANITIZER_EVIDENCE_BUNDLE.md
- [ ] Security Lead reviews and accepts GA_PENTEST_EVIDENCE_BUNDLE.md
- [ ] No new CRITICAL findings in server, llm, or sharding module gap registers
- [ ] CHANGELOG.md `[Unreleased]` moved to `[2.4.0]` entry
- [ ] VERSIONING.md version table updated to v2.4.0 GA stable status
- [ ] ROADMAP.md updated with Phase 0-6 completion markers
- [ ] research/implementation_influence/by_module.md Soll-Ist matrix verified (6 modules)
- [ ] docs/DOXYGEN_COVERAGE_REPORT.md confirms >99% header coverage
- [ ] `develop` → `community` merge approved
- [ ] Tag `v2.4.0` created on `community` merge commit
- [ ] Release artefact built from v2.4.0 tag (not develop HEAD)

### Verification Checklist

**Pre-Promotion Verification:**
- [ ] Run full `release_critical` suite on develop HEAD
- [ ] Archive Wave 7/8/9 gate evidence with timestamp
- [ ] Security Lead sign-off on sanitizer/pentest bundles
- [ ] Confirm module gap registers (server/llm/sharding) have zero new CRITICAL findings
- [ ] Review CHANGELOG.md for accuracy and completeness
- [ ] Verify VERSIONING.md correctly reflects GA status
- [ ] Check research/implementation_influence/by_module.md Soll-Ist (6 modules, 21+ aspects)
- [ ] Doxygen audit confirms >99% coverage

**Post-Promotion Verification:**
- [ ] Tag `v2.4.0` is created and reachable
- [ ] `community` branch is synchronized with v2.4.0 tag
- [ ] Release build succeeds from v2.4.0 tag
- [ ] Binary/package artefact is published
- [ ] RELEASE_NOTES_v2.4.0.md posted (if applicable)

### Known Blockers & Mitigation

| Blocker | Impact | Mitigation |
|---------|--------|-----------|
| Human sign-off delay | Promotion blocked indefinitely | Escalate to release approver by 2026-08-08 |
| Wave 9 gate regression | CI failure blocks promotion | Re-run Wave 9 with fresh baseline; if persistent, trigger root-cause investigation |
| Security review pending | Cannot clear sanitizer/pentest evidence | Coordinate with Security Lead immediately; provide evidence review SLA |
| DOXYGEN coverage gap | Cannot confirm >99% header coverage | Run `doxygen Doxyfile.audit` and validate on current HEAD |

---

## Phase 2 — Performance & Scalability Readiness (6-8 weeks)

**Starts after Phase 1A completes. Parallel with Phases 3–5 allowed.**

### Scope

Deliver query optimization backlog focused on measurable latency reduction, plan-cache efficiency, and load-balancing hardening under Wave 7 regression gates.

### Key Components

1. **Query Optimization Backlog**
   - Plan-cache efficiency (cache hit-rate ≥ 95%, memory footprint < 100 MB)
   - Cost-model refinement for complex AQL queries
   - Connection pooling tuning for cluster-wide scenarios
   - Load-balancing strategy validation

2. **Benchmark Infrastructure**
   - Plan-cache hit-rate measurement tool
   - Cost-model performance profiling
   - Resource pooling efficiency benchmarks
   - Wave-7 non-regression baseline

3. **Testing Coverage**
   - ~130 new performance-focused tests
   - Integration tests for query plan caching
   - Cost-model validation tests
   - Load-balancing scenario tests

### Deliverables

- [ ] Query optimization backlog itemized with measurable latency targets
- [ ] Plan-cache efficiency benchmarks (cache hit-rate, memory footprint)
- [ ] Cost-model refinement for complex AQL queries (p99 latency < 200 ms)
- [ ] Connection pooling tuning for cluster-wide scenarios (≥ 2× throughput vs. baseline)
- [ ] Load-balancing strategy validation under Wave 7 load
- [ ] 130+ new performance-focused tests passing
- [ ] Wave-7 non-regression proof on final candidate

### Acceptance Criteria (Gates)

- [ ] All optimizations gated on measured query latency with Wave-7 non-regression proof
- [ ] Cache-hit-rate ≥ 95% on repeat query patterns
- [ ] p99 query latency < 200 ms for complex multi-shard queries
- [ ] Resource pooling shows ≥ 2× throughput improvement vs. baseline
- [ ] All 130+ performance tests pass with TIMEOUT 300
- [ ] Wave 7 gates remain PASS (no regression)
- [ ] Code review approved
- [ ] Merged to develop

### Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Query plan cache misses under load | Medium | Latency regression | Measure cache hit patterns; add cache warming strategy |
| Resource contention in pooling | Medium | Throughput plateau | Profile under load; implement dynamic pool sizing |
| Wave-7 regression | Low | Promotion blocked | Run Wave 7 baseline frequently; gate on metrics |
| Plan-cache memory overhead | Low | OOM on memory-constrained systems | Cap cache size; implement LRU eviction policy |

---

## Phase 3 — Plugin Externalization & Monetization (8-10 weeks)

**Starts after Phase 1A completes. Parallel with Phases 2, 4, 5 allowed.**

### Scope

Move Wave-1 private plugins to commit-pinned submodules and finalize public/private boundaries with manifest schema extensions and CI policy enforcement.

### Wave-1 Private Plugins

1. `themisdb_ethic_ai` → `plugins/private/themisdb_ethic_ai/` (ethics_ai plugin)
2. `themisdb_storage` → `plugins/private/themisdb_storage/` (user_storage_encrypted, azure_blob_storage, s3_blob_storage)
3. `themisdb_importer` → `plugins/private/themisdb_importer/` (mysql_importer, mongo_importer, kafka_importer, s3_importer)
4. `themisdb_llm_wiki` → `plugins/private/themisdb_llm_wiki/` (LLM Wiki tool)

### Optional Public Externalizations

- `themisdb_geo` → `plugins/themisdb_geo/` (optional via `THEMIS_EXTERNALIZE_GEO_PLUGIN`)
- `themisdb_timeseries` → `plugins/themisdb_timeseries/` (optional via `THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN`)

### Deliverables

- [ ] All Wave-1 private repo `.gitmodules` entries pinned to release-ready commits
- [ ] Manifest schema extended with visibility, allowed_editions, license_feature, compatibility metadata
- [ ] Community/Minimal lane CI gates added to prevent private credential leakage
- [ ] Geo/TimeSeries optional plugin registration tested (with/without submodule checkout)
- [ ] Contributor onboarding guide published for public-vs-private-enabled checkouts
- [ ] Edition gating enforced at runtime (Status::PermissionDenied for unauthorized editions)
- [ ] All plugin visibility/edition tests passing

### Acceptance Criteria (Gates)

- [ ] All private submodule pins ratified (commit hashes archived)
- [ ] Community lane configure/build/test succeeds **without** private sources
- [ ] Enterprise lane configure/build/test succeeds **with all** Wave-1 plugins
- [ ] Edition gating enforced at runtime
- [ ] CI/CD policy checks active for all private-related changes
- [ ] Contributor guide published and validated by 2+ community members
- [ ] All plugin visibility tests passing
- [ ] Code review approved
- [ ] Merged to develop

### Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Private credential leakage in Community build | Medium | Security incident | Implement CI policy checks; run negative tests frequently |
| Submodule sync drift across editions | Medium | Build failure for some branches | Daily submodule update validation; SETUP.md guidance |
| Manifest schema incompatibility | Low | Plugin load failure | Maintain backward-compatible defaults; test migration path |
| Edition gating not enforced | Low | Unauthorized feature access | Runtime checks + integration tests |

---

## Phase 4 — Observability & Operations Hardening (6-8 weeks)

**Starts after Phase 1A completes. Parallel with Phases 2, 3, 5 allowed.**

### Scope

Complete observability, auditability, and SLO-signal coverage for production operations with full runbook suite and 99.99% SLA validation.

### Key Components

1. **Error Classification & Context**
   - Error code taxonomy with operational semantics (retry-safe, transient, permanent)
   - Context propagation throughout release-critical paths
   - Error-to-SLO-signal mapping

2. **Audit Logging**
   - Compliance instrumentation for security-sensitive operations
   - Audit trail persistence
   - Log rotation and retention policies

3. **SLO Signal Export**
   - Latency percentiles (p50, p95, p99)
   - Error rate and error type breakdowns
   - Availability percentiles (9s counting)
   - Resource utilization signals (CPU, memory, I/O)

4. **Runbook Suite**
   - Installation and configuration guide
   - Failover procedures with RTO targets
   - Recovery procedures with RPO targets
   - Upgrade and rollback procedures
   - Troubleshooting guide

5. **SLA Validation**
   - 99.99% uptime (RTO ≤ 5000 µs, chaos-validated)
   - Cluster rejoin ≤ 2000 µs under fault injection
   - Auth security overhead ≤ 150 µs p99

### Deliverables

- [ ] Error code taxonomy expanded with operational semantics (retry-safe, transient, permanent)
- [ ] Audit logging instrumentation complete for security-sensitive operations
- [ ] SLO-signal export configured for Prometheus/monitoring
- [ ] Runbook suite: install, configure, failover, recovery, upgrade, rollback
- [ ] 99.99% SLA validation test confirms sustained uptime under chaos
- [ ] Operations team reviews and validates runbooks
- [ ] Operator handoff drills completed
- [ ] All instrumentation tests passing

### Acceptance Criteria (Gates)

- [ ] All release-critical paths have SLO signals exported
- [ ] Error taxonomy covers all release-critical error codes
- [ ] Audit logging active in production paths
- [ ] Runbooks validated by operations team (signature or approval)
- [ ] 99.99% SLA gate (GATE-W9-04) passes: RTO ≤ 5000 µs
- [ ] Chaos/fault-recovery gate (GATE-W9-03) passes: cluster rejoin ≤ 2000 µs
- [ ] Security overhead gate (GATE-W9-02) passes: auth p99 ≤ 150 µs
- [ ] All observability tests passing
- [ ] Code review approved
- [ ] Merged to develop

### Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Audit logging overhead impacts p99 latency | Medium | SLO violation | Measure instrumentation impact; use async logging where safe |
| Operations team readiness delay | Medium | Runbook validation blocked | Start operator handoff drills early; provide training materials |
| SLA target unachievable under chaos | Low | Promotion blocked | Root-cause investigation; may require infrastructure changes |
| Error context propagation overhead | Low | Latency regression | Use thread-local storage; lazy evaluation |

---

## Phase 5 — LLM Wiki Enterprise Plugin Hardening (8-10 weeks)

**Starts after Phase 1A completes. Parallel with Phases 2, 3, 4 allowed.**

### Scope

Complete Phases 2–6 implementation for the LLM Wiki plugin, transitioning from Phase 1–2 baseline (Python MVP + Phase A) to production-hardened Phase B backend with RocksDB, persistent caching, and Wikipedia ingestion validation.

### Current Status (Baseline)

- ✅ Phase 1: `ILLMWikiPlugin` public C++ SDK interface defined
- ✅ Phase 2 (partial): Private plugin shared library with Phase A (JSON indexing + FNV hash)
- 🟡 Phase 2B: RocksDB Phase B activation pending
- [ ] Phase 3: Error handling & edge cases
- [ ] Phase 4: Tests (LWP-01..LWP-20)
- [ ] Phase 5: Performance hardening
- [ ] Phase 6: Documentation & acceptance

### Key Components

1. **Phase B Activation**
   - RocksDB-native BM25 index
   - HNSW vector search
   - Reciprocal Rank Fusion (RRF) for hybrid search
   - Phase A → B migration path with automatic index rebuild
   - p95 query latency < 100 ms at 50k chunks

2. **Persistent Embedding Cache**
   - RocksDB column family for embeddings
   - Keyed on `(doc_id + sha256(content))`
   - Cache miss triggers re-embedding
   - ≥ 99% hit rate on re-ingest

3. **C++ Workspace Orchestrator Hardening**
   - Production lifecycle tests (create, ingest, query, delete)
   - Concurrency safety validation
   - Recovery from partial failures
   - Append-only log + atomic state persistence

4. **Wikipedia Ingestion Production Validation**
   - Throughput ≥ 5k articles/s
   - Memory usage profiling
   - Streaming ingestion with backpressure
   - Sub-feature license gating (`llm_wiki_wikipedia`)

5. **RBAC Multi-Tenant Namespaces**
   - Per-workspace RBAC
   - Tenant isolation verification
   - Cross-tenant query rejection

### Deliverables

- [ ] Phase B index migration complete with automatic rebuild
- [ ] Persistent embedding cache (RocksDB column family) achieving ≥ 99% hit rate
- [ ] C++ workspace orchestrator production tests (LWP-09..LWP-16)
- [ ] Wikipedia dump ingestion benchmarks (≥ 5k articles/s throughput)
- [ ] Operator runbook: install, configure, ingest, query, upgrade, rollback
- [ ] Migration guide: Python MVP → C++ plugin with index format compatibility
- [ ] RBAC multi-tenant namespace support
- [ ] All LWP tests passing (LWP-01..LWP-20 + integration tests)

### Acceptance Criteria (Gates)

- [ ] Phase B query latency < 100 ms at 50k chunks (vs. Phase A < 200 ms at 5k chunks)
- [ ] Embedding cache hit rate ≥ 99% on re-ingest of unchanged docs
- [ ] Wikipedia ingestion throughput ≥ 5k articles/s
- [ ] All LWP-01..LWP-20 tests pass with TIMEOUT 120
- [ ] LWP-INT-01..LWP-INT-04 integration tests pass (live RocksDB fixture)
- [ ] LWP-WIKI-01..02 Wikipedia dump smoke tests pass with sub-feature gate
- [ ] LWP-GATE-01 edition-gate negative test passes
- [ ] LWP-PERF-01 p95 query latency benchmark passes
- [ ] Operator runbook validated by operations team
- [ ] Code review approved
- [ ] Merged to develop

### Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Phase B index migration data loss | Low | Catastrophic | Implement backup + rollback strategy; validate on staging |
| Embedding cache thrashing under load | Medium | Latency regression | Monitor cache hit patterns; implement adaptive sizing |
| RocksDB tuning complexity | Medium | Suboptimal performance | Profile on target hardware; document tuning parameters |
| Wikipedia ingestion OOM on large dumps | Medium | Process crash | Implement streaming with backpressure; tune batch sizes |
| RBAC implementation complexity | Low | Schedule slip | Prioritize workspace isolation first; RBAC as follow-up |

---

## Central Execution Board (Gates & Risk Register)

### Live Gate Status

| Phase | Gate | Status | Evidence | Target |
|-------|------|--------|----------|--------|
| 1A | Human sign-off (Section 9) | 🔴 PENDING | Awaiting approver | 2026-08-11 |
| 1A | Wave 7/8/9 regression proof | 🟡 EVIDENCE PRESENT | `benchmarks/wave{7,8,9}/` | 2026-08-11 |
| 1A | CI: release_critical suite | 🟡 EVIDENCE PRESENT | `.github/workflows/09-pr-gates_release-critical-tests.yml` | 2026-08-11 |
| 2 | Wave-7 non-regression | 🟤 PENDING | Not started (Phase 2 TBD) | 2026-09-30 |
| 2 | Plan-cache ≥ 95% hit-rate | 🟤 PENDING | Not started (Phase 2 TBD) | 2026-09-30 |
| 2 | p99 query latency < 200 ms | 🟤 PENDING | Not started (Phase 2 TBD) | 2026-09-30 |
| 3 | Private submodule pins ratified | 🟤 PENDING | Not started (Phase 3 TBD) | 2026-10-31 |
| 3 | Community lane CI passing | 🟤 PENDING | Not started (Phase 3 TBD) | 2026-10-31 |
| 3 | Edition gating enforced | 🟤 PENDING | Not started (Phase 3 TBD) | 2026-10-31 |
| 4 | All SLO signals exported | 🟤 PENDING | Not started (Phase 4 TBD) | 2026-10-15 |
| 4 | 99.99% SLA gate (GATE-W9-04) | 🟡 EVIDENCE PRESENT | `benchmarks/wave9/WAVE9_BENCHMARK_COVERAGE.md` | 2026-10-15 |
| 4 | Runbooks validated | 🟤 PENDING | Not started (Phase 4 TBD) | 2026-10-15 |
| 5 | Phase B latency < 100 ms @ 50k | 🟤 PENDING | Not started (Phase 5 TBD) | 2026-10-31 |
| 5 | Embedding cache ≥ 99% hit rate | 🟤 PENDING | Not started (Phase 5 TBD) | 2026-10-31 |
| 5 | Wikipedia ≥ 5k articles/s | 🟤 PENDING | Not started (Phase 5 TBD) | 2026-10-31 |

### Risk Register

| Risk ID | Phase | Risk | Probability | Impact | Current Mitigation | Owner |
|---------|-------|------|-------------|--------|-------------------|-------|
| R-1A-1 | 1A | Human sign-off delay | High | BLOCKS ALL | Escalate by 2026-08-08 | Release Approver |
| R-1A-2 | 1A | Wave 9 regression | Low | High | Rerun with fresh baseline | QA Lead |
| R-2-1 | 2 | Cache misses under load | Medium | Medium | Measure patterns; cache warming | Perf Team |
| R-3-1 | 3 | Credential leakage | Medium | Critical | CI policy checks; negative tests | Infra Team |
| R-4-1 | 4 | Instrumentation overhead | Medium | Medium | Async logging; measure impact | Ops Team |
| R-5-1 | 5 | Phase B data migration | Low | Critical | Backup + rollback; staging validation | LLM Team |

---

## Parallel Workstream Task Distribution

### Recommended Team Assignments

| Team | Phase | Duration | Notes |
|------|-------|----------|-------|
| A (Release) | 1A | 1-2 weeks | Human sign-off + validation + tag creation |
| B (Query/Perf) | 2 | 6-8 weeks | Starts after 1A; plan-cache, cost-model, pooling |
| C (Infra/DevOps) | 3 | 8-10 weeks | Starts after 1A; plugin externalization + CI gates |
| D (Ops/SRE) | 4 | 6-8 weeks | Starts after 1A; observability + runbooks + SLA proof |
| E (LLM) | 5 | 8-10 weeks | Starts after 1A; Phase B activation + embedding cache + Wikipedia |

### Gate Sequencing (Mandatory)

```
develop (Phase 1A gates complete)
    ├─► Phase 2 gates pass ─► merge to develop
    ├─► Phase 3 gates pass ─► merge to develop
    ├─► Phase 4 gates pass ─► merge to develop
    └─► Phase 5 gates pass ─► merge to develop

  After ALL phases complete and gate evidence archived:
    ├─► Promotion candidate branch created
    └─► Human sign-off on full test suite
        └─► Tag v2.4.1 (or next release version)
            └─► Merge to community
                └─► Release candidate tagged
```

### Sequential Requirement

- **Phase 1A MUST complete before any Phase 2–5 work is promoted to community**
- **Phases 2–5 can merge to develop independently once their per-phase gates pass**
- **Final promotion to community requires ALL phases complete + human sign-off**

---

## Success Criteria Summary

### Phase 1A Success

✅ Done when:
- Human sign-off in Section 9 of GA_PROMOTION_SIGN_OFF.md
- All Batch D gates (D-1..D-10) show PASS status
- Tag v2.4.0 created on community branch
- Release artefact published

### Phase 2 Success

✅ Done when:
- Query optimization backlog closed (130+ tests)
- Plan-cache ≥ 95% hit-rate measured
- p99 query latency < 200 ms for complex queries
- Wave 7 non-regression proof archived

### Phase 3 Success

✅ Done when:
- All Wave-1 private submodule pins ratified
- Community lane CI passes without private sources
- Enterprise lane CI passes with all plugins
- Contributor guide published and validated

### Phase 4 Success

✅ Done when:
- All SLO signals exported and validated
- Error taxonomy complete with operational semantics
- Runbooks validated by operations team
- 99.99% SLA gate passes

### Phase 5 Success

✅ Done when:
- Phase B latency < 100 ms @ 50k chunks achieved
- Embedding cache ≥ 99% hit rate on re-ingest
- Wikipedia ingestion ≥ 5k articles/s
- All LWP tests passing + operator runbook validated

---

## Escalation Points

### Immediate Escalation Required

- **Phase 1A:** If human sign-off not completed by 2026-08-10 → Escalate to release approver
- **Any Phase:** If gate shows FAIL status → Investigate root cause immediately + update risk register

### Weekly Sync Required

- Update gate status table above
- Review risk register + mitigation effectiveness
- Confirm team assignments remain aligned
- Adjust timeline if blockers identified

---

## References

- `ROADMAP.md` — canonical source of truth for release readiness
- `docs/governance/GA_PROMOTION_SIGN_OFF.md` — GA sign-off checklist and evidence
- `FUTURE_ENHANCEMENTS.md` — open enhancement backlog
- `research/implementation_influence/by_module.md` — research backbone
- `.github/workflows/09-pr-gates_release-critical-tests.yml` — CI release gate definition
- `BRANCHING_STRATEGY.md` — branch governance and promotion lanes
