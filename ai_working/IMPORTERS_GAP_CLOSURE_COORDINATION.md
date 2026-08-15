# Importers Module Gap Closure Coordination Framework

**Start Date:** 2026-08-15  
**Target Completion:** 2026-10-15 (10 weeks)  
**Scope:** 282 total gaps (44 Critical, 151 High, 82 Medium, 5 Low) across 27 files  
**Model:** 6-phase coordinated sub-agent execution with parallel phases 3-5

---

## Executive Summary

The importers module has accumulated 282 code quality gaps spanning null dereference, concurrency issues, performance anti-patterns, and documentation drift. This coordination document orchestrates a 6-phase remediation campaign using specialized sub-agents (gap-verifier, themisdb-implementer, task, themisdb-reviewer).

**Success Criteria:**
- 100% CRITICAL gaps resolved (44 items)
- ≥90% HIGH gaps resolved (≥135 of 151 items, ≤16 deferred)
- ≥60% MEDIUM/LOW resolved (≥52 of 87 items)
- All changes pass `release_critical` CI and benchmark gates (IMRG-01..06)
- Zero new warnings introduced

---

## Phase Status Dashboard

| Phase | Name | Status | Target | Agent | Output Artifact |
|-------|------|--------|--------|-------|-----------------|
| **1** | Triage & Validation | ⏳ **QUEUED** | W1 (Aug 15-22) | gap-verifier | IMPORTERS_PHASE1_GAP_TRIAGE.md |
| **2** | CRITICAL Fixes | ⏹ **PENDING** | W2-3 (Aug 22-Sep 5) | themisdb-implementer | IMPORTERS_PHASE2_CRITICAL_FIXES_COMPLETE.md |
| **3** | HIGH Batch A1 | ⏹ **PENDING** | W4-5 (Sep 5-19) | themisdb-implementer | IMPORTERS_PHASE3_HIGH_BATCH_A1_COMPLETE.md |
| **4** | HIGH Batch A2 | ⏹ **PENDING** | W4-5 (Sep 5-19) | themisdb-implementer | IMPORTERS_PHASE4_HIGH_BATCH_A2_COMPLETE.md |
| **5** | MEDIUM/LOW | ⏹ **PENDING** | W6-8 (Sep 19-Oct 3) | task/implementer | IMPORTERS_PHASE5_MEDIUM_LOW_COMPLETE.md |
| **6** | Review & Docs | ⏹ **PENDING** | W8-10 (Oct 3-15) | themisdb-reviewer | IMPORTERS_PHASE6_FINAL_CLOSURE_CERTIFICATE.md |

---

## Gaps by Severity and Complexity

### Critical (44 gaps)

**Tier-1 (Simple, 1-2 files, <100 LOC each):**
- [ ] blocking_no_timeout (postgres line 373) — add timeout to mutex_lock in ProgressCallback
- [ ] no_timeout (postgres line 373) — condition_variable timeout
- [ ] smart_ptr_misuse (postgres line 2452) — raw new → std::make_unique

**Tier-2 (Moderate, 3-8 files, 100-300 LOC):**
- [ ] null_dereference across postgres (356, 359, 360, 362, 363, 367, 400, 401, 402) — 9 items in handle assignment
- [ ] data_race (postgres 2104, 2106) — custom_type_map_ access without lock
- [ ] resource_leaked_in_exception — add try-catch guards to connector pooling paths
- [ ] smart_ptr_misuse (mysql 6 items, mongo 3 items) — raw new/delete patterns

**Tier-3 (Complex, 8+ files, >300 LOC):**
- [ ] blocking_no_timeout across kafka, oracle, s3 — standardize timeout semantics in all connectors
- [ ] data_race in schema inference/validation — centralized mutex for shared state

**Files in Scope (CRITICAL):**
- postgres_importer.cpp (5 CRITICAL)
- mysql_importer.cpp (6 CRITICAL)
- flatfile_importer.cpp (7 CRITICAL)
- huggingface_ingestion_plugin.cpp (7 CRITICAL)
- deterministic_matcher.cpp (3 CRITICAL)
- gui_import_wizard.cpp (2 CRITICAL)
- s3_importer.cpp (3 CRITICAL)
- kafka_importer.cpp (2 CRITICAL)
- oracle_importer.cpp (2 CRITICAL)
- sqlite_importer.cpp (2 CRITICAL)
- mongo_importer.cpp (3 CRITICAL)
- data_quality.cpp (1 CRITICAL)

**Phase 2 Batching:**
- Batch A: postgres (5) + mysql (6) = 11 CRITICAL
- Batch B: flatfile (7) + huggingface (7) + deterministic_matcher (3) + gui_import_wizard (2) = 19 CRITICAL
- Batch C: s3 (3) + kafka (2) + oracle (2) + sqlite (2) + mongo (3) + data_quality (1) = 13 CRITICAL

### High (151 gaps)

**Distribution by file:**
- postgres_importer.cpp: 31 HIGH
- mysql_importer.cpp: 15 HIGH
- mongo_importer.cpp: 12 HIGH
- flatfile_importer.cpp: 10 HIGH
- s3_importer.cpp: 12 HIGH
- kafka_importer.cpp: 12 HIGH
- oracle_importer.cpp: 8 HIGH
- sqlite_importer.cpp: 9 HIGH
- mdm_engine.cpp: 10 HIGH
- schema_inference.cpp: 4 HIGH
- Other files: 28 HIGH

**Phase 3 (Batch A1):** postgres (31) + mysql (15) + mongo (12) = 58 HIGH gaps  
**Phase 4 (Batch A2):** flatfile (10) + s3 (12) + kafka (12) + oracle (8) + sqlite (9) + schema_inference (4) = 55 HIGH gaps

### Medium (82 gaps)

**Categories:**
- map_vs_unordered_map (13 items)
- repeated_search (7 items)
- nested_loop_find (16 items)
- uninitialized_access (11 items)
- pointer_arithmetic_unbounded (10 items)
- hardcoded_path (7 items)
- resource_leaked_in_exception (13 items)
- Other patterns (5 items)

**Phase 5 Batching:**
- Batch M1: Data structures (map→unordered_map, vector reserve) — 28 items
- Batch M2: Algorithms (nested loops→hash, repeated search→cache) — 22 items
- Batch M3: Documentation & cross-file consistency — 32 items

### Low (5 gaps)

- module_doc_linkset_drift (2 items)
- stale_doc_section_reference (1 item)
- unstructured_log (1 item)
- no_retry_logic (1 item)

**Phase 5:** Integrated into Batch M3 (documentation and cross-file consistency)

---

## Key Dependencies and Blockers

| Blocker | Impact | Mitigation | Status |
|---------|--------|-----------|--------|
| RocksDB missing in Community builds | WikiIndexStore Phase B, persistent cache blocked | Feature-gate behind THEMIS_WIKI_PHASE_B (already done) | ✅ RESOLVED |
| Connector availability variance | Tests may fail if connectors unavailable | Use mock/stub connectors in tests | ⏳ TBD Phase 1 |
| Benchmark instability | Release gates (IMRG-01..06) may fluctuate | Use ±5% regression budget, established baseline | ⏳ TBD Phase 2 |
| Cross-module coordination | Failover/Updates dependencies | Failover Phase 2+3 ✅, Updates ✅ (from ROADMAP.md) | ✅ RESOLVED |
| Concurrency correctness | Multiple connectors share state | Standardize mutex/lock semantics in Phase 2 | ⏳ TBD Phase 2 |

---

## Quality Gate Definitions

### Acceptance Criteria per Phase

**Phase 1 Triage Exit Gate:**
- [ ] All 282 gaps classified (True Positive / False Positive / Deferred)
- [ ] Severity reassessment confirmed or updated with rationale
- [ ] Complexity matrix populated (Tier-1/2/3 distribution)
- [ ] Blockers and dependencies documented
- [ ] Zero gaps left unclassified

**Phase 2 CRITICAL Exit Gate:**
- [ ] 100% (44/44) CRITICAL gaps fixed or explicitly deferred (with documented reason)
- [ ] All fixes compile without new warnings
- [ ] Focused regression tests pass for all modified files
- [ ] Benchmark gates (IMRG-01..06) stable (no regression >±5%)
- [ ] Commit messages link to gap categories

**Phase 3 HIGH A1 Exit Gate:**
- [ ] ≥80% (47/58) postgres/mysql/mongo HIGH gaps fixed
- [ ] Integration tests pass
- [ ] p99 latency within ±5% of baseline
- [ ] No regressions in connector availability

**Phase 4 HIGH A2 Exit Gate:**
- [ ] ≥80% (44/55) flatfile/s3/kafka/oracle/sqlite/schema HIGH gaps fixed
- [ ] All fixes compile and pass module tests
- [ ] No regression in connector fallback paths
- [ ] Release gates stable

**Phase 5 MEDIUM/LOW Exit Gate:**
- [ ] ≥60% (52/87) MEDIUM/LOW gaps fixed
- [ ] No behavioral regressions from optimizations
- [ ] Documentation gaps closed and cross-linked
- [ ] Benchmarks show no regression

**Phase 6 Final Exit Gate:**
- [ ] Zero outstanding CRITICAL gaps (100% closure)
- [ ] ≥90% HIGH gaps resolved (≤16 deferred with documented reasons)
- [ ] ≥60% MEDIUM/LOW closure (≥52 items)
- [ ] All reviews completed and approved
- [ ] `release_critical` CI green
- [ ] ROADMAP and FUTURE_ENHANCEMENTS synchronized
- [ ] Certification certificate signed

---

## Progress Tracking Protocol

### Daily Standup Artifacts
- Commit messages must reference gap categories and phase number
- Each agent dispatch logged with agent_id in `IMPORTERS_GAP_WEEKLY_STATUS_W<N>.md`
- Blockers escalated within 24h of discovery

### Weekly Consolidation (Friday)
- [ ] Triage completion rate (Phase 1)
- [ ] Fix throughput: gaps closed / agent-hours (Phases 2-5)
- [ ] Test pass rate and benchmark variance
- [ ] Risk/blocker updates
- [ ] Next week priorities

### Example Commit Messages
```
IMPORTERS-P2-BATCH-A: Fix 11 CRITICAL gaps (postgres, mysql)

Categories addressed:
- null_dereference (9 items): handle null checks in postgres import
- data_race (2 items): add mutex to custom_type_map_ access
- blocking_no_timeout (3 items): add timeouts to mutex_lock

Files modified: postgres_importer.cpp, mysql_importer.cpp
Tests: IMPI-01..11 PASS; IMRG-01..06 stable
Benchmarks: p99 ±2% of baseline
```

---

## Communication & Escalation

**Phase Kickoff:** Detailed agent task spec prepared in ai_working/ before dispatch  
**Mid-phase Updates:** Weekly consolidation report generated Friday EOD  
**Phase Completion:** Artifact signed off before next phase launches  
**Escalation:** Blocker reported immediately with mitigation options  

**Contact Points:**
- gap-verifier: Phase 1 findings → IMPORTERS_PHASE1_GAP_TRIAGE.md
- themisdb-implementer: Phase 2-4 implementations → Batch completion reports
- task agents: Phase 5 bulk operations → Batch completion reports
- themisdb-reviewer: Phase 6 review → Final certification

---

## Timeline & Milestones

| Week | Phase | Milestones | Gate |
|------|-------|-----------|------|
| W1 (Aug 15-22) | 1 | Phase 1 triage complete; all gaps classified | CRITICAL: zero unclassified |
| W2-3 (Aug 22-Sep 5) | 2 | All CRITICAL fixes merged | CRITICAL: 100% closure |
| W4-5 (Sep 5-19) | 3-4 | Phase 3+4 parallel HIGH fixes merged | HIGH: ≥80% closure |
| W6-8 (Sep 19-Oct 3) | 5 | MEDIUM/LOW fixes and optimizations merged | MEDIUM/LOW: ≥60% closure |
| W8-10 (Oct 3-15) | 6 | Code review, CI validation, documentation complete | FINAL: GA-ready |

---

## Files Modified Inventory (Progressive)

**Phase 1 Output:** Complete inventory of affected 27 files  
**Phase 2 Output:** postgres_importer.cpp, mysql_importer.cpp, + 10 others  
**Phase 3-4 Output:** flatfile, s3, kafka, oracle, sqlite, schema_inference, + others  
**Phase 5 Output:** Cross-file optimizations and documentation  
**Phase 6 Output:** ROADMAP.md, FUTURE_ENHANCEMENTS.md, BUILD_STATUS.md

---

## Success Metrics (Post-Completion)

- [ ] Gap coverage: 60%+ of 282 gaps remediated (~170 gaps)
- [ ] CRITICAL: 100% (44/44)
- [ ] HIGH: ≥90% (≥135/151)
- [ ] MEDIUM/LOW: ≥60% (≥52/87)
- [ ] Test coverage: 100% of modified code paths
- [ ] Benchmark stability: IMRG-01..06 all PASS with <±5% variance
- [ ] No regressions: `release_critical` CI green
- [ ] Code quality: Zero new warnings, RAII compliance verified
- [ ] Documentation: Cross-linked and source-verifiable
