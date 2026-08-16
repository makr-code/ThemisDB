# Server Module Gap Closure — Master Plan (2026-08-16)

**Status:** Parallel 4-agent execution in progress  
**Plan Version:** 1.0  
**Target Completion:** ~2.5 hours wall-clock time (vs ~18+ sequential)

---

## Executive Summary

Close **2,172 verified gaps** in ThemisDB server module using proven parallel multi-agent model from Query (2026-08-16) and Process (2026-08-06) module closures.

### Gap Statistics
- **Total Verified Gaps:** 2,172
- **CRITICAL (186):** Exception safety, null-dereference, resource leaks
- **HIGH (468):** Hardcoded paths, copy overhead, data races, timeouts, audit gaps
- **MEDIUM (1,013):** Modernization, doc drift, logging
- **LOW (8):** Edge cases and cleanup
- **Actionable (Critical+High):** 654 findings

### Affected Files: 111 across server module

---

## Batch Architecture (File-Level Isolation)

### Batch 1: CRITICAL (Agent 1 — themisdb-reviewer)
**Assigned Files (32):** Exception-safety and null-safety critical paths

**Primary Focus Files:**
1. `http_server.cpp` (28 CRITICAL, 109 HIGH, 94 MEDIUM) 
2. `query_api_handler.cpp` (34 CRITICAL, 40 HIGH, 75 MEDIUM)
3. `mqtt_session.cpp` (8 CRITICAL)
4. `http3_session.cpp` (7 CRITICAL)
5. `task_scheduler_api_handler.cpp` (6 CRITICAL)
6. `monitoring_api_handler.cpp` (6 CRITICAL)
7. `shard_repair_api_handler.cpp` (7 CRITICAL)
8–32. [25 additional files with high critical density]

**Gap Categories (Priority Order):**
1. null_dereference (118) → add nullptr checks
2. uncaught_exception (131) → try-catch with cleanup
3. resource_leaked_in_exception (64) → RAII/smart_ptr
4. data_race (76) → mutex guards
5. explicit_delete / manual_cleanup (53) → smart pointers

**Est. Timeline:** ~15 min  
**Verification:** Compile + server-phase5-hardening tests pass

---

### Batch 2: HIGH-A Infrastructure (Agent 2 — gap-verifier)
**Assigned Files (38):** Infrastructure hardening (paths, copies, races)

**Primary Focus Files:**
1. `postgres_session.cpp` (131 gaps, 7 CRITICAL, 18 HIGH)
2. `import_wizard_builder.cpp` (118 gaps, 0 CRITICAL, 2 HIGH)
3. `llm_api_handler.cpp` (67 gaps, 15 CRITICAL, 39 HIGH) — HIGH-portion
4. `mcp_server.cpp` (63 gaps, 3 CRITICAL, 34 HIGH) — HIGH-portion
5. `replication_topology_api_handler.cpp` (21 gaps, 6 CRITICAL, 7 HIGH) — HIGH-portion
6–38. [33 additional files]

**Gap Categories (Priority Order):**
1. hardcoded_path (258) → config/env vars
2. copy_overhead (139) → string_view, move
3. unnecessary_copy (108) → const&
4. db_connection_leak (53) → connection pooling
5. string_concat_loop (94) → stringstream

**Est. Timeline:** ~45 min  
**Verification:** Compile + API handler tests pass

---

### Batch 3: HIGH-B Operational (Agent 3 — doc-orchestrator)
**Assigned Files (35):** Observability and operational contracts

**Primary Focus Files:**
1. `rate_limiter_v2.cpp` (24 gaps, 4 CRITICAL, 20 HIGH)
2. `async_job_api_handler.cpp` (21 gaps, 2 CRITICAL, 17 HIGH)
3. `entity_api_handler.cpp` (33 gaps, 4 CRITICAL, 12 HIGH)
4. `changefeed_api_handler.cpp` (27 gaps, 2 CRITICAL, 5 HIGH)
5. `auth_middleware.cpp` (16 gaps, 2 CRITICAL, 5 HIGH)
6–35. [30 additional files]

**Gap Categories (Priority Order):**
1. no_timeout (19) → deadline/timeout params
2. missing_audit_log (32) → audit emitters
3. missing_correlation_id (30) → correlation threading
4. missing_latency_metric (27) → timer/metrics
5. no_retry_logic (32) → idempotent retry + backoff
6. missing_health_check (12) → liveness probes

**Est. Timeline:** ~40 min  
**Verification:** Compile + audit/metric instrumentation verified

---

### Batch 4: MEDIUM Modernization (Agent 4 — general-purpose)
**Assigned Files (6+):** All remaining MEDIUM/LOW findings across all other files

**Gap Categories (Priority Order):**
1. missing_vector_reserve (7) → pre-allocate
2. o_n_squared (13) → fix nested loops
3. smart_ptr_misuse (13) → unique/shared ptr fixes
4. delete_without_nullptr (47) → set nullptr after delete
5. uninitialized_access (21) → init all members
6. range_temporary (19) → fix temporary binding
7. iterator_invalidation (10) → iterator safety
8. legacy_or_compat_path (27) → mark/remove legacy
9. stale_doc_section_reference (13) → update links
10. [+35 more categories, all MEDIUM/LOW]

**Est. Timeline:** ~30 min  
**Verification:** Compile + regression tests pass

---

## Integration Strategy (Sequential Merge)

After all agents complete, merge in strict order to avoid conflicts:

```
Order: Batch1 (Agent1) → Batch2 (Agent2) → Batch3 (Agent3) → Batch4 (Agent4)

Why sequential:
- File-level isolation prevents merge conflicts
- Each batch depends on previous build state
- Verification gates catch regressions early
```

### Step 1: Wait for all agents to complete
- Monitor agent_ids:
  - server-batch1-critical
  - server-batch2-higha
  - server-batch3-highb
  - server-batch4-medium

### Step 2: Merge Batch 1 → Batch 2
```bash
# Batch 1 committed to develop
# Batch 2 pulled develop (includes B1 changes)
# Merge B2 on top of B1
```

### Step 3: Merge Batch 2 → Batch 3
```bash
# B2 now on develop
# B3 pulled develop (includes B1+B2)
# Merge B3 on top of B1+B2
```

### Step 4: Merge Batch 3 → Batch 4
```bash
# B3 now on develop
# B4 pulled develop (includes B1+B2+B3)
# Merge B4 on top of B1+B2+B3
```

### Step 5: Final Verification
- Full rebuild: `cmake --preset linux-debug && cmake --build --preset linux-debug`
- Server test suite: `ctest -R test_server --preset linux-debug`
- Release-gate benchmarks: `cmake --preset linux-release && ctest -R bench_server --preset linux-release`

---

## Verification Checklist

### Per-Batch Verification (During Execution)

**Batch 1 (CRITICAL):**
- [ ] All 32 files compile without error
- [ ] Zero null-ptr dereferences remain (verified via asan/ubsan)
- [ ] All exception paths have proper cleanup
- [ ] test_server_phase5_hardening passes (16 tests)
- [ ] test_server_contract_hardening_focused passes (20 tests)

**Batch 2 (HIGH-A):**
- [ ] All 38 files compile cleanly on top of B1
- [ ] No hardcoded paths remain in API paths
- [ ] No unnecessary copies in hot paths
- [ ] No db connection leaks (verified via leak detector)
- [ ] test_api_integration passes
- [ ] test_api_gateway passes

**Batch 3 (HIGH-B):**
- [ ] All 35 files compile cleanly on top of B1+B2
- [ ] All timeout paths have deadlines
- [ ] All security paths emit audit logs
- [ ] Retry logic follows Phase 5 patterns (exponential backoff + budget)
- [ ] test_server_activation_profile passes
- [ ] Correlation IDs properly threaded

**Batch 4 (MEDIUM):**
- [ ] All remaining files compile without error
- [ ] All modern C++ patterns applied consistently
- [ ] No raw new/delete in public APIs
- [ ] Vector pre-allocation where applicable
- [ ] test_server_integration_complete passes
- [ ] Release-gate benchmarks (SVR-01..SVR-08) PASS

### Final Integration Verification

- [ ] Full repo builds: `cmake --preset linux-release`
- [ ] All server tests pass: `ctest -R test_server -j 4`
- [ ] All server benchmarks pass: `ctest -R bench_server`
- [ ] Zero compiler warnings (or expected warnings only)
- [ ] CodeQL scan: no new high/critical findings
- [ ] Module gap-scan rerun: gap count reduced by ≥90%

---

## Expected Outcomes

### Before Closure
- 2,172 verified gaps across 111 files
- 654 actionable (Critical+High) gaps
- Multiple categories of safety/quality debt

### After Closure
- >95% gap reduction (≥2,063 gaps closed)
- All 186 CRITICAL findings eliminated
- All 468 HIGH findings eliminated or documented
- Modern C++ practices applied to MEDIUM findings
- Production-ready server stack

### Deliverables
- Updated source files (all 111 files potentially modified)
- Enhanced test coverage for edge cases
- Improved observability (audit logs, metrics, correlation IDs)
- Updated documentation (API contracts, operational guides)
- Single PR with batched commits

---

## Timeline Estimate

| Phase | Est. Duration | Cumulative |
|-------|---------------|-----------|
| Batch 1 (CRITICAL) | 15 min | 15 min |
| Batch 2 (HIGH-A) | 45 min | 60 min |
| Batch 3 (HIGH-B) | 40 min | 100 min |
| Batch 4 (MEDIUM) | 30 min | 130 min |
| Integration & Verification | 15 min | 145 min |
| **Total (Wall-Clock)** | — | **~2.5 hours** |

**Comparison:** Sequential approach would take ~18+ hours (111 files × 10 min/file avg)

---

## Risk & Rollback

### Risks
- **Build Failure:** Any batch fails to compile → detected immediately, rollback that batch
- **Test Regression:** Any gate test fails → re-run, identify gap, fix, re-commit
- **Merge Conflict:** Batches touch same file → unlikely (file-level isolation), if occurs → manual resolution

### Rollback Strategy
- Each batch is independent; can rollback individual batch without affecting others
- If all 4 batches fail: revert all commits, investigate root cause, restart
- If 1-3 batches fail: keep passing batches, fix failing batch, re-run

---

## Success Criteria

1. ✅ All 2,172 gaps triaged and categorized by severity
2. ✅ 95%+ gap reduction (≥2,063 closed)
3. ✅ Zero CRITICAL gaps remain
4. ✅ Zero HIGH gaps remain (or explicitly documented/wont-fix)
5. ✅ Build passes without errors
6. ✅ All server tests pass (100%)
7. ✅ Release-gate benchmarks pass
8. ✅ No new CodeQL findings
9. ✅ PR ready for merge to develop

---

## References

- **Source:** `src/server/MODULE_GAPS.md` (2,172 verified gaps)
- **Precedent:** Query module (2026-08-16, 203+ gaps, 4-agent parallel, ~2 hr total)
- **Precedent:** Process module (2026-08-06, 101 files, 72+ tests, Phase 1-6 complete)
- **Roadmap:** `src/server/ROADMAP.md` (Phase 5 complete, P0 security remediation in progress)
- **API Contract:** `include/server/server_api_contract.h` (frozen, 12+ error classes)
