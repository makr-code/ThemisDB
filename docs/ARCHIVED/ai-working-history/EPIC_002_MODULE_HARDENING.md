# EPIC-002: High-Gap Module Hardening (llm, server, query, sharding, index)

**Status:** 🔵 Planned  
**Target Release:** v2.0.0 Stable (2026-09-30)  
**Timeline:** 8 Weeks (W35-W42, 2026-08-25 to 2026-10-18)  
**Owner:** Module Hardening Committee  
**Scope:** 22.085 Total Findings → 15.000 Findings (-32% reduction)

## Epic Summary

Systematically harden the 5 highest-gap-density modules through targeted input validation, exception safety, and distributed consistency improvements. Focus on CRITICAL + HIGH severity findings to enable production-grade v2.0.0 release.

**Module Targets:**
- **llm:** HIGH 18.885 → 12.000 (-36%)
- **server:** HIGH 14.066 → 9.000 (-36%)
- **query:** HIGH 12.959 → 8.000 (-38%)
- **sharding:** HIGH 7.800 → 5.000 (-36%)
- **index:** HIGH 6.503 → 4.000 (-38%)

**Total Gap Reduction:** 59.213 HIGH → 38.000 HIGH (-35.8%, ~21.213 gaps fixed)

## Success Criteria

- ✅ **Module-Specific Targets:** Each module achieves target HIGH gap reduction
- ✅ **Input Validation:** RequestValidator, InputValidator, RangeQueryValidator deployed
- ✅ **Exception Safety:** QueryExecutor RAII wrapper, exception guarantee tests
- ✅ **Distributed Consistency:** 2PC timeout hardening, FK validation verified
- ✅ **Test Coverage:** 50+ new/updated tests across all modules
- ✅ **Performance:** No regression in TPC-H workloads (< 2%)
- ✅ **Production Ready:** All modules certified for v2.0.0 stable release

## Key Deliverables

| Module | Focus | Validator | Test Cases | Gaps Fixed |
|--------|-------|-----------|-----------|-----------|
| **llm** | Input validation, tensor bounds | InputValidator | 15+ | 450/1.011 |
| **server** | HTTP request, connection safety | RequestValidator | 15+ | 900/2.279 |
| **query** | Exception safety, resource cleanup | QueryExecutor | 15+ | 850/2.288 |
| **sharding** | 2PC timeout, FK validation | TimeoutGuard | 10+ | 2.500/7.800 |
| **index** | Range query, partition safety | RangeQueryValidator | 10+ | 2.200/6.503 |

## Dependencies

- **Prerequisite:** EPIC-001 v1.5.0 release complete (Sprint 8-9 done)
- **Parallel:** EPIC-003 Phase 6 Scanner (starts W40, overlaps weeks 40-42)
- **Supports:** EPIC-005 Distributed Consistency (uses 2PC hardening)

## Sub-Issues

### [002-A] LLM Module - Multi-LoRA Manager Input Validation (1.011 HIGH)
- **Title:** `LLM: multi_lora_manager.cpp Input Validation & Bounds Checking`
- **Type:** Enhancement / Bug Fix
- **Labels:** `module-hardening`, `llm`, `input-validation`, `bounds-checking`
- **Estimated Effort:** 30 hours
- **Focus Files:** multi_lora_manager.cpp, llama_wrapper.cpp
- **Acceptance Criteria:**
  - InputValidator wrapper class designed and integrated
  - Bounds checking for tensor operations implemented
  - 450/1.011 HIGH gaps remediated (45% target)
  - 15+ property-based tests for tensor validation
  - Zero false negatives in validation test suite
  - Code review + security audit

### [002-B] Server Module - HTTP Request & Connection Safety (2.279 HIGH)
- **Title:** `Server: http_server.cpp Request Validation & Connection Safety`
- **Type:** Enhancement / Bug Fix
- **Labels:** `module-hardening`, `server`, `http`, `request-validation`
- **Estimated Effort:** 35 hours
- **Focus Files:** http_server.cpp, query_api_handler.cpp, voice_api_handler.cpp
- **Acceptance Criteria:**
  - RequestValidator class for HTTP method/header/body validation
  - Connection pooling with timeout + leak detection
  - 900/2.279 HIGH gaps remediated (40% target)
  - RequestValidator fuzzing tests added
  - HTTP/1.1, HTTP/2, HTTP/3 compliance verified
  - Load testing with 1000+ concurrent connections

### [002-C] Query Module - Exception Safety & Resource Management (2.288 HIGH)
- **Title:** `Query: query_engine.cpp Exception Handling & Resource Management`
- **Type:** Enhancement / Bug Fix
- **Labels:** `module-hardening`, `query`, `exception-safety`, `raii`
- **Estimated Effort:** 35 hours
- **Focus Files:** query_engine.cpp, aql_translator.cpp
- **Acceptance Criteria:**
  - QueryExecutor RAII wrapper for query lifecycle
  - Exception guarantee tests (strong, basic, no-throw)
  - 850/2.288 HIGH gaps remediated (37% target)
  - Zero uncaught exceptions in query path
  - Resource leaks eliminated (valgrind clean)
  - 15+ exception safety tests added

### [002-D] Sharding Module - 2PC Timeout & FK Validation (7.800 HIGH)
- **Title:** `Sharding: Cross-Shard FK Validation & Timeout Handling`
- **Type:** Enhancement / Bug Fix
- **Labels:** `module-hardening`, `sharding`, `2pc`, `distributed-consistency`
- **Estimated Effort:** 35 hours
- **Focus Files:** cross_shard_transaction.cpp, two_phase_commit_coordinator.cpp
- **Acceptance Criteria:**
  - CrossShardForeignKeyValidator timeout hardening
  - Explicit timeout enforcement in prepare/commit phases
  - 2.500/7.800 HIGH gaps remediated (32% target)
  - 2PC timeout verified under network delays
  - FK validation works cross-shard with timeout guarantees
  - 10+ distributed consistency tests

### [002-E] Index Module - Range Query & Partition Safety (6.503 HIGH)
- **Title:** `Index: Range Query Safety & Partitioning Consistency`
- **Type:** Enhancement / Bug Fix
- **Labels:** `module-hardening`, `index`, `range-queries`, `partitioning`
- **Estimated Effort:** 30 hours
- **Focus Files:** index_adaptive.cpp, index_lsm.cpp
- **Acceptance Criteria:**
  - RangeQueryValidator for partition boundary checks
  - Explicit bounds checking for range scans
  - 2.200/6.503 HIGH gaps remediated (34% target)
  - RangeQueryValidator tested with synthetic workloads
  - Partition key validation prevents off-by-one errors
  - 10+ range consistency tests added

## Metrics & KPIs

| Metric | Baseline | Target | Success |
|--------|----------|--------|---------|
| Total HIGH Gaps | 59.213 | 38.000 | -35.8% |
| llm HIGH Gaps | 18.885 | 12.000 | -36% |
| server HIGH Gaps | 14.066 | 9.000 | -36% |
| query HIGH Gaps | 12.959 | 8.000 | -38% |
| sharding HIGH Gaps | 7.800 | 5.000 | -36% |
| index HIGH Gaps | 6.503 | 4.000 | -38% |
| Test Coverage | 326 | 350+ | +24 tests |
| Performance Regression | TBD | < 2% | Within bounds |

## References & Evidence

- **Source:** ROADMAP.md §Module Status Summary
- **Scanner Output:** ai_working/HIGH_GAP_SPRINT_BACKLOG_2026-05-25.md
- **Module Hotspots:** Identified in gap_scan_report_2026-06-13.md
- **Related Issues:** #5195-#5201 (Module-level inventories)

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Validator perf overhead | Medium | Medium | Benchmark validators, optimize critical paths |
| Hidden regressions | Medium | High | Full TPC-H workload testing |
| Distributed consistency issues | Low | High | Property-based tests, Raft audit |
| 2PC timeout tuning | Medium | High | Network simulation testing |

## Timeline & Milestones

```
W35-W36: [002-A] LLM module hardening
W36-W37: [002-B] Server module hardening  
W37-W38: [002-C] Query module hardening
W38-W39: [002-D] Sharding module hardening
W39-W40: [002-E] Index module hardening
W40-W42: Integration testing, performance verification
```

---

**Last Updated:** 2026-07-05  
**Created By:** AI Deep-Dive Implementation Team  
**Related Epics:** EPIC-001 (prerequisite), EPIC-003 (parallel), EPIC-005 (depends on sharding hardening)
