# Remaining 38 Modules — Batch 7+ Remediation Inventory & Scheduling

**Status:** 📋 Inventory Phase (Post-Wave-B gate)  
**Timeline:** 2026-10-02 (Wave B entry) → 2026-Q1 2027 (completion)  
**Model:** Each batch follows Wave A/B acceptance + evidence closure pattern  

---

## Inventory Summary

After Wave A (A1–A5) and Wave B (B1–B3) completion, **38 remaining modules** require remediation across Q4 2026 and Q1 2027.

### High-Level Categorization

| Priority | Count | Target Q | Rationale |
|----------|-------|----------|-----------|
| **CRITICAL** | 8 | Q4 2026 | GA release blockers; high risk; performance gates |
| **HIGH** | 15 | Q4 2026 → Q1 2027 | Feature completeness; security; operational readiness |
| **MEDIUM** | 10 | Q1 2027 | Hardening; cleanup; observability enhancements |
| **LOW** | 5 | Q1 2027 | Experimental; optional acceleration; nice-to-have |

---

## Critical Priority Modules (Q4 2026)

### Tier 1: GA Release Blockers (Must Complete Before Release)

| Batch | Module | Status | Target | Rationale | Tasks |
|-------|--------|--------|--------|-----------|-------|
| 7 | `ethics_ai` | Phase B in progress | 2026-10-31 | EU AI Act compliance (Art. 13/22) | ChainVisualizer finalization, NormEvidence validation, CSEP test completeness (8+ focused tests required) |
| 7 | `security` | Phase 3 in progress | 2026-10-31 | Vault/HSM/PKI integration | RLS real-workload validation, policy conflict edge cases, concurrent policy updates |
| 7 | `audit` | Phase 2 hardening | 2026-10-31 | Trustworthiness + compliance | Integrity verification, high-volume export under sustained load, SBOM audit trail |
| 7 | `observability` | Phase 3 expansion | 2026-10-31 | Operator diagnostics | Distributed tracing, high-cardinality stress, exporter reliability, alert correlation |

### Tier 2: Performance-Gate Blockers (Must Lock Benchmarks)

| Batch | Module | Status | Target | Rationale | Tasks |
|-------|--------|--------|--------|-----------|-------|
| 8 | `server` | P5-S01/S02 closed | 2026-10-15 | Top-risk module; connection pool + coordination hardening | Protocol versioning, connection lifecycle chaos, cascading failure recovery |
| 8 | `llm` | P5-L01/L02 closed | 2026-10-15 | Top-risk module; speculative decode + distributed optimization | Speculative token inference quality, distributed e2e latency gates, token count consistency |
| 8 | `tensor` | Distributed integration | 2026-11-15 | Distributed compute backbone | Federated tensor summaries, cross-shard reduce, fault tolerance under worker loss |

### Tier 3: High-Risk Distributed Paths

| Batch | Module | Status | Target | Rationale | Tasks |
|-------|--------|--------|--------|-----------|-------|
| 8 | `distributed_knowledge` | Phase 2 completion | 2026-11-15 | Distributed graph traversal | Exact-path consistency, cross-shard join correctness, cascade behavior |
| 9 | `distributed_tensor` | Federated sharding | 2026-11-30 | Tensor sharding + replication | Rebalance transparency, fault tolerance, correctness under node failures |

---

## High Priority Modules (Q4 2026 → Q1 2027)

### Feature Completeness

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 9 | `network` | Phase 3 | 2026-11-30 | Circuit breaker + bulkhead isolation, protocol overhead reduction, connection pool tuning |
| 9 | `scheduler` | Phase 2 | 2026-11-30 | Task prioritization, backpressure handling, resource-aware scheduling |
| 9 | `importers` | Phase 2 | 2026-11-30 | SQL/CSV/Parquet/Avro pipeline hardening, error recovery, streaming ingestion |
| 10 | `exporters` | Phase 2 | 2026-12-15 | Parquet/Avro/S3/BigQuery output pipelines, high-volume durability, retry semantics |
| 10 | `governance` | Phase 2 | 2026-12-15 | Policy validation, enforcement automation, audit trail integrity |
| 10 | `llm_wiki` | Phase B cache | 2026-12-31 | (Note: Phase A in Wave B; Phase B cache layer: RocksDB hit-rate > 80%, sustained 1000 QPS) |
| 10 | `rag` | Phase B finalization | 2026-12-31 | BM25+ ranking, HNSW index, RRF fusion; production readiness |

### Operational Readiness

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 9 | `maintenance` | Runbook delivery | 2026-11-30 | Access-model promotion, replication lag/failover, sharding repair, voice incident triage, GPU fallback |
| 9 | `cdc` | Phase 2 hardening | 2026-11-30 | Change data capture consistency, delivery guarantees, replay semantics |
| 10 | `configuration` | Validation + reload | 2026-12-15 | Zero-downtime config reload, policy validation, schema versioning |

### Security Hardening

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 10 | `user_storage_encrypted` | Phase B finalization | 2026-12-31 | Encryption key rotation, zero-knowledge proof, quantum-safe crypto integration |
| 10 | `performance` | Profiling + tuning | 2026-12-31 | Bottleneck identification, optimization guide, performance SLA documentation |

---

## Medium Priority Modules (Q1 2027)

### Hardening + Cleanup

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 11 | `utils` | Phase 2 | 2027-01-31 | Utility library completeness, edge case hardening, performance optimization |
| 11 | `index` | Phase 3 | 2027-01-31 | Index selection + stats, query optimization hints, index rebuild safety |
| 11 | `execution` | Phase 3 | 2027-01-31 | Executor thread pool tuning, adaptive parallelism, memory pressure handling |
| 11 | `config` | Validation scope | 2027-01-31 | Config schema validation, deprecation path, backwards compatibility |
| 11 | `metadata` | Catalog consistency | 2027-01-31 | Schema versioning, migration path, concurrent DDL safety |

### Observability + Diagnostics

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 12 | `cache` | Eviction policies | 2027-02-28 | Cache hit-rate optimization, adaptive TTL, memory pressure response |
| 12 | `temporal` | Time-series aggregation | 2027-02-28 | Downsampling, retention policies, cardinality management |
| 12 | `timeseries` | Phase 3 (optional public plugin) | 2027-02-28 | Time-series kernel optimization, compression, replication |
| 12 | `vector_search` | HNSW tuning | 2027-02-28 | Index tuning, recall vs latency tradeoff, dynamic reindexing |
| 12 | `retrieval` | E2E integration | 2027-02-28 | Sparse + dense hybrid search, ranking fusion, filtering safety |

---

## Low Priority Modules (Q1 2027)

### Experimental + Optional Acceleration

| Batch | Module | Status | Target | Key Tasks |
|-------|--------|--------|--------|-----------|
| 13 | `image_analysis` | Phase 1 | 2027-03-31 | CLIP-based embedding, image classification, batch processing |
| 13 | `content` | Phase 1 | 2027-03-31 | Document extraction, metadata inference, chunking strategy |
| 13 | `plugins` | SDK finalization | 2027-03-31 | Public SDK API freeze, plugin versioning, compatibility matrix |
| 13 | `toolbox` | Utility scripts | 2027-03-31 | CLI tools, debugging utilities, operational scripts |
| 13 | `projects` | Demo scenarios | 2027-03-31 | Reference implementations, integration examples, training materials |

---

## Detailed Batch 7–13 Execution Plan

### Batch 7 — Ethics AI + Security + Audit + Observability (Q4 2026)

**Entry Gate:** Wave B exit criteria PASS (October 2, 2026)  
**Target Completion:** 2026-10-31  
**Parallel Execution:** All 4 modules in parallel (no inter-module blocking)  

#### B7-1: Ethics AI (EU AI Act Compliance)

**Acceptance Criteria:**
- [ ] ChainVisualizer fully functional: traces decision chain from input → output
- [ ] NormEvidence comprehensive: documents ethical norm reasoning per decision
- [ ] CSEP test suite ≥8 focused tests (Art. 13/22 coverage)
- [ ] Audit trail: immutable log of all ethics checks + decisions
- [ ] Documentation: ethics audit runbook for operators

**Key Tasks:**
- T7-1-A: Finalize ChainVisualizer (if Phase B not complete)
- T7-1-B: Validate NormEvidence against EU AI Act requirements
- T7-1-C: Create CSEP-focused test suite (8+ tests)
- T7-1-D: Implement audit trail integration
- T7-1-E: Documentation + runbook

**Owner:** TBD  
**Est. Effort:** 120 hours (2–3 weeks)  
**Evidence Location:** `src/ethics_ai/BATCH_7_CLOSURE_REPORT.md`

#### B7-2: Security (Vault/HSM/PKI Integration)

**Acceptance Criteria:**
- [ ] Vault integration: auth, secret retrieval, rotation
- [ ] HSM failover: provider failover, transparent recovery
- [ ] RLS production workload: real query workloads with row-level filtering
- [ ] Policy conflicts: concurrent policy updates, conflict resolution tested
- [ ] `release_critical` CI: green on `develop`

**Key Tasks:**
- T7-2-A: Implement Vault client integration (if not complete)
- T7-2-B: HSM failover testing under chaos (coordinator crash, network partition)
- T7-2-C: RLS production-style scenario (100K rows, 1000 concurrent queries)
- T7-2-D: Policy conflict edge cases (race conditions, stale cache)
- T7-2-E: CI gate validation

**Owner:** TBD  
**Est. Effort:** 160 hours (3–4 weeks)  
**Evidence Location:** `src/security/BATCH_7_CLOSURE_REPORT.md`

#### B7-3: Audit (Integrity + Export Reliability)

**Acceptance Criteria:**
- [ ] Integrity verification: audit trail cryptographic consistency check (Merkle tree or chainable hashes)
- [ ] High-volume export: 10M audit records exported under sustained load (100 records/sec)
- [ ] Export durability: guaranteed delivery (no loss, no dups) under network failures
- [ ] SBOM audit: artifact tracking (code → compiled binary → deployed)

**Key Tasks:**
- T7-3-A: Implement integrity verification (Merkle tree or equivalent)
- T7-3-B: High-volume export stress test (10M records, 100 rec/sec, 24h)
- T7-3-C: Export retry logic + durability guarantees
- T7-3-D: SBOM artifact tracking integration

**Owner:** TBD  
**Est. Effort:** 140 hours (2–3 weeks)  
**Evidence Location:** `src/audit/BATCH_7_CLOSURE_REPORT.md`

#### B7-4: Observability (Distributed Tracing + Correlation)

**Acceptance Criteria:**
- [ ] Distributed tracing: trace follows request across modules (server → query → execution → storage)
- [ ] High-cardinality stress: 10K concurrent traces; no memory leak
- [ ] Exporter reliability: traces delivered to backend under network degradation
- [ ] Alert correlation: related alerts grouped intelligently (root cause inference)

**Key Tasks:**
- T7-4-A: Distributed tracing instrumentation (OpenTelemetry or native)
- T7-4-B: High-cardinality stress test (10K traces concurrent)
- T7-4-C: Exporter failover + retry logic
- T7-4-D: Alert correlation engine

**Owner:** TBD  
**Est. Effort:** 150 hours (3 weeks)  
**Evidence Location:** `src/observability/BATCH_7_CLOSURE_REPORT.md`

---

### Batch 8 — Server + LLM + Tensor Top-Risk Modules (Q4 2026)

**Entry Gate:** B7-1 through B7-4 exit criteria PASS (October 31, 2026)  
**Target Completion:** 2026-11-15  
**Parallel Execution:** All 3 modules in parallel  

#### B8-1: Server (P5-S01/S02 Hardening)

**Acceptance Criteria:**
- [ ] P5-S01: Protocol versioning + compatibility testing
- [ ] P5-S02: Connection lifecycle chaos (connect/drop/reconnect patterns)
- [ ] Cascading failure recovery: if database unavailable, graceful degradation
- [ ] Connection pool stress: 10K concurrent connections; deterministic cleanup on failure

**Key Tasks:**
- T8-1-A: Protocol versioning implementation (backward compatibility)
- T8-1-B: Connection chaos test suite (20+ scenarios)
- T8-1-C: Cascading failure recovery paths
- T8-1-D: Connection pool hardening + stress test

**Owner:** TBD  
**Est. Effort:** 180 hours (3–4 weeks)  
**Evidence Location:** `src/server/BATCH_8_CLOSURE_REPORT.md`

#### B8-2: LLM (P5-L01/L02 Distributed Optimization)

**Acceptance Criteria:**
- [ ] P5-L01: Speculative decode correctness (inference quality regression < 1%)
- [ ] P5-L02: Distributed end-to-end latency gate (p95 ≤ 500ms @ 100 concurrent inference)
- [ ] Token count consistency: all replicas produce same token count
- [ ] Batch inference: multi-model load balancing + queue management

**Key Tasks:**
- T8-2-A: Speculative decode quality validation (against baseline)
- T8-2-B: Distributed inference latency gates + benchmarking
- T8-2-C: Token consistency verification
- T8-2-D: Batch inference load balancing

**Owner:** TBD  
**Est. Effort:** 200 hours (4 weeks)  
**Evidence Location:** `src/llm/BATCH_8_CLOSURE_REPORT.md`

#### B8-3: Tensor (Distributed Integration)

**Acceptance Criteria:**
- [ ] Federated tensor summaries: correctness across shards (CRC/hash match)
- [ ] Cross-shard reduce: merge tensors from multiple shards with correct semantics
- [ ] Fault tolerance: worker loss → automatic failover + re-computation
- [ ] Performance gates: cross-shard communication ≤ 10% overhead

**Key Tasks:**
- T8-3-A: Federated summaries algorithm + verification
- T8-3-B: Cross-shard reduce implementation + gate
- T8-3-C: Worker fault tolerance + recovery
- T8-3-D: Performance profiling + optimization

**Owner:** TBD  
**Est. Effort:** 170 hours (3–4 weeks)  
**Evidence Location:** `src/tensor/BATCH_8_CLOSURE_REPORT.md`

---

### Batch 9 — Distributed Knowledge + Tier-2 High-Priority (Q4–Q1 2026–2027)

**Entry Gate:** B8-1 through B8-3 exit criteria PASS (November 15, 2026)  
**Target Completion:** 2026-12-15  
**Parallel Execution:** 6 modules in parallel  

| Module | Acceptance Criteria | Key Task | Est. Effort |
|--------|-------------------|----------|------------|
| `distributed_knowledge` | Exact-path consistency (1M graph traversals), cross-shard join correctness, cascade behavior verified | Graph traversal verification, join correctness proofs, cascading failure testing | 150h |
| `network` | Circuit breaker + bulkhead isolation working, protocol overhead < 5%, connection pool optimal tuning | CB activation tests, overhead profiling, pool tuning | 120h |
| `scheduler` | Task prioritization working, backpressure control activated, resource-aware scheduling preventing overload | Prioritization validation, backpressure test, resource limits enforcement | 130h |
| `importers` | SQL/CSV/Parquet/Avro pipelines hardened, error recovery deterministic, streaming ingestion under load | Pipeline completion tests, error recovery chaos, streaming stress (1M rec/sec) | 140h |
| `cdc` | CDC consistency guaranteed (no loss, no dups), replay semantics deterministic, delivery SLA met | Consistency proofs, replay correctness, delivery guarantee stress test | 150h |
| `maintenance` | Runbooks published: access promotion, replication lag/failover, sharding repair, voice triage, GPU fallback | 5 runbooks (1 per area), operability review, update cycle process | 100h |

**Total Effort:** ~790 hours (11–13 weeks)  
**Evidence Location:** `src/*/BATCH_9_CLOSURE_REPORT.md` (per module)

---

### Batch 10 — Feature Completeness + Security Hardening (Q4–Q1 2026–2027)

**Entry Gate:** B9 exit criteria PASS (December 15, 2026)  
**Target Completion:** 2026-12-31  
**Parallel Execution:** 7 modules in parallel  

| Module | Target | Key Deliverables |
|--------|--------|-----------------|
| `exporters` | 2026-12-15 | Parquet/Avro/S3/BigQuery output pipelines, high-volume durability, retry semantics, SLA compliance |
| `governance` | 2026-12-15 | Policy validation automation, enforcement audit trail, conflict resolution, edition-license checks |
| `llm_wiki` | 2026-12-31 | Phase B cache: RocksDB integration, hit-rate > 80%, sustained 1000 QPS, latency p95 ≤ 100ms |
| `rag` | 2026-12-31 | BM25+ ranking finalization, HNSW index tuning, RRF fusion, production readiness validation |
| `user_storage_encrypted` | 2026-12-31 | Encryption key rotation, zero-knowledge proof validation, quantum-safe crypto integration (if available) |
| `performance` | 2026-12-31 | Bottleneck identification, optimization guide, SLA documentation, profiling tools |
| `configuration` | 2026-12-15 | Zero-downtime config reload, schema versioning, backwards compatibility, policy validation |

**Total Effort:** ~900 hours (13–15 weeks)  
**Evidence Location:** `src/*/BATCH_10_CLOSURE_REPORT.md` (per module)

---

### Batch 11 — Q1 2027 Hardening + Cleanup (January 2027)

**Entry Gate:** B10 exit criteria PASS (December 31, 2026)  
**Target Completion:** 2027-01-31  
**Modules:** utils, index, execution, config, metadata (5 modules, parallel)  

**Summary:** Edge-case hardening, performance optimization, compatibility verification across 5 utility + execution modules.

---

### Batch 12 — Observability + Optimization (February 2027)

**Entry Gate:** B11 exit criteria PASS (January 31, 2027)  
**Target Completion:** 2027-02-28  
**Modules:** cache, temporal, timeseries, vector_search, retrieval (5 modules, parallel)  

**Summary:** Caching policies, time-series optimization, vector-search tuning, e2e retrieval integration.

---

### Batch 13 — Experimental + Finalization (March 2027)

**Entry Gate:** B12 exit criteria PASS (February 28, 2027)  
**Target Completion:** 2027-03-31  
**Modules:** image_analysis, content, plugins, toolbox, projects (5 modules, parallel)  

**Summary:** Experimental feature stabilization, SDK freeze, demo/training materials, CLI finalization.

---

## Consolidated Schedule

| Phase | Batches | Timeline | Entry Gate | Exit Gate |
|-------|---------|----------|-----------|-----------|
| Wave A | A1–A5 | 2026-08-14 → 2026-10-02 | Dev branch | Chaos evidence + p95/p99 baselines |
| Wave B | B1–B3 | 2026-10-02 → 2026-10-31 | Wave A PASS | Performance gates locked |
| **Batch 7** | Ethics/Security/Audit/Obs | 2026-10-02 → 2026-10-31 | Wave B entry | All 4 closures signed off |
| **Batch 8** | Server/LLM/Tensor | 2026-10-31 → 2026-11-15 | B7 PASS | P5 gates + distributed correctness |
| **Batch 9** | Dist.Knowledge/Network/CDC/etc | 2026-11-15 → 2026-12-15 | B8 PASS | 6 modules signed off |
| **Batch 10** | Exporters/Governance/RAG/etc | 2026-12-15 → 2026-12-31 | B9 PASS | 7 modules signed off |
| **Batch 11** | Utils/Index/Exec/Config/Meta | 2027-01-01 → 2027-01-31 | B10 PASS | 5 modules signed off |
| **Batch 12** | Cache/Temporal/TS/Vector/Retrieval | 2027-02-01 → 2027-02-28 | B11 PASS | 5 modules signed off |
| **Batch 13** | Image/Content/Plugins/Tools/Projects | 2027-03-01 → 2027-03-31 | B12 PASS | 5 modules signed off |

**Program Completion:** 2027-03-31  
**Total Remaining Modules:** 38 (7 A/B batches + 8 follow-on batches = 15 total batches across 43 modules)  

---

## Program Success Criteria (Final)

By end Q1 2027, **all below must be PASS:**

- [ ] All distributed and acceleration paths fail closed
- [ ] Major modules have benchmark-backed p95/p99 baselines on representative hardware
- [ ] Recovery/failover paths have deterministic chaos evidence
- [ ] Security controls have production-style integration validation
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks
- [ ] All 38 remaining modules have closure evidence blocks
- [ ] GA release sign-off complete (Section 9 in `docs/governance/GA_PROMOTION_SIGN_OFF.md`)

---

## Risk Register

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Wave A/B blocker prevents scheduled start | Cascading delay to Batches 7–13 | Medium | Escalate immediately; execute parallel workarounds if possible |
| Test flakiness (e.g., timing issues) | Gate fails; retry cycle delays | Medium | Deterministic test design; profile latency bottlenecks; isolate flaky tests |
| Resource unavailability (CUDA hardware, RocksDB) | Gated functionality blocked | Low | Feature gates; CPU-only fallback paths; diagnostic presets |
| Performance regression in stress tests | Fails gate; requires investigation + fix | Medium | Baseline comparison; profiling budget; optimization sprint |
| Operator readiness gaps discovered late | Runbooks incomplete at release | Low | Start runbook drafts early (during Batch 7); operability review at Batch 8 |

---

**Last Updated:** 2026-08-14  
**Next Review:** Upon Wave A/B exit criteria completion (targeting 2026-10-02)
