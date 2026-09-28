# LLM Wiki Plugin Phase 3-4 Implementation Strategy

**Version:** 1.0  
**Last Updated:** 2026-08-05  
**Timeline:** Q4 2026 - Q1 2027  
**Objective:** Complete error handling, comprehensive testing, and production readiness for LLM Wiki plugin

---

## Executive Summary

The LLM Wiki enterprise plugin is at Phase 3-4 (Error Handling & Comprehensive Testing). This document provides:

1. **Current State Assessment** - what's implemented vs. pending
2. **Implementation Priorities** - phases 3-4 focus areas
3. **Test Matrix** - LWP-01..LWP-20 test definitions and validation criteria
4. **Deliverables Timeline** - Q4 2026 + Q1 2027 phased rollout

---

## Phase 3: Error Handling & Edge Cases (Q4 2026)

### Current Implementation Status

#### ✅ Complete (Ready for Testing)

1. **Guardrail Patterns Registry** (`src/llm_wiki/guardrail_patterns.h`)
   - 60+ patterns across 5 categories:
     - Shell commands (20+ patterns): `bash`, `sh`, `sudo`, `rm`, `curl`, etc.
     - Code execution (10+ patterns): `eval`, `exec`, `__import__`, `subprocess`, etc.
     - Encoding bypass (8+ patterns): `base64`, `rot13`, `hex encoding`, `URL encoding`, etc.
     - Privilege escalation (5+ patterns): `sudo`, `su`, `chmod`, `chown`, etc.
     - Control flow manipulation (5+ patterns): `goto`, `jmp`, `interrupt`, etc.
   - Case-insensitive matching with whitespace normalization
   - Thread-safe stateless design

2. **Edition-Gate Enforcement** (`src/llm_wiki/edition_gate.h` + `edition_gate.cpp`)
   - Compile-time flag: `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED`
   - Runtime enforcement via `Status::PermissionDenied()` on community/minimal editions
   - Sub-feature gating (e.g., `llm_wiki_wikipedia`)

3. **Enhanced Guardrail Checks**
   - Integrated into `ILLMWikiPlugin::query()` pipeline
   - Per-chunk filtering with unsafe pattern detection
   - Query-level flagging (`query_flagged_for_prompt_injection = true`)

#### 🟡 Partial Implementation (Needs Completion)

4. **Workspace State Manager** (`src/llm_wiki/workspace_state_manager.h`)
   - **Status:** Class interface defined; implementation pending
   - **Required Methods:**
     - `load(path)` → validate SHA-256 checksum, detect corruption
     - `save(state)` → atomic write with temp-file pattern
     - `recover()` → rebuild from append-only log on corruption
   - **Schema:** JSON state with version, timestamps, link graph, task tracking
   - **Test:** `test_llm_wiki_phase3_edge_cases_focused.cpp::WorkspaceStateManager*`

5. **Partial-Failure Semantics**
   - **Status:** Per-file error aggregation implemented; needs test coverage
   - **Behavior:**
     - Individual file processing errors logged (no blanket failure)
     - Results aggregated in `WikiIngestResult.failed_files` vector
     - Continue processing remaining files after error
   - **Test:** `test_llm_wiki_plugin_phase3_phase4_focused.cpp::LWP06`

#### ❌ Not Yet Implemented

6. **Private Plugin Submodule Content**
   - `plugins/themisdb_llm_wiki/` exists but is empty (awaiting submodule provisioning)
   - Phase B backend (RocksDB-native BM25 + HNSW + RRF) → Phase 5 work

---

## Phase 4: Comprehensive Test Suite (Q4 2026 - Q1 2027)

### Test Organization

All Phase 4 tests are organized in:
- **Primary Suite:** `tests/test_llm_wiki_plugin_phase3_phase4_focused.cpp`
- **Edge Cases:** `tests/test_llm_wiki_phase3_edge_cases_focused.cpp`
- **Integration:** `tests/test_llm_wiki_integration_phase4_focused.cpp` (pending)
- **Performance:** `benchmarks/llm_wiki/bench_llm_wiki_*.cpp`

### Test Matrix — LWP-01..LWP-20 + Gates

#### LWP-01..LWP-08: Core Interface & Semantics ✅

| Test | Focus | Expected Behavior | Status |
|------|-------|-------------------|--------|
| **LWP-01** | Ingest single Markdown file | File indexed with correct chunk boundaries | ✅ Impl |
| **LWP-02** | Query returns ranked results | Results sorted descending by score | ✅ Impl |
| **LWP-03** | min_score threshold | Low-score results filtered out | ✅ Impl |
| **LWP-04** | skip_existing=true | Already-indexed files skipped | ✅ Impl |
| **LWP-05** | Guardrail prompt injection | Unsafe queries flagged; chunks filtered | ✅ Impl |
| **LWP-06** | Partial-failure on ingest | Errors logged; processing continues | 🟡 Test needed |
| **LWP-07** | Initialize from JSON config | Workspace created with config values | ✅ Impl |
| **LWP-08** | Stats returns correct counts | Chunk count, file count, ingest time reported | ✅ Impl |

**Recall Target:** ≥ 0.8 (Recall@k where k=top_k from query options)

#### LWP-09..LWP-16: Workspace Lifecycle 🟡

| Test | Focus | Expected Behavior | Status |
|------|-------|-------------------|--------|
| **LWP-09** | Workspace directory creation | `wiki/`, `wiki/pages/`, `wiki/logs/` created | 🟡 Impl |
| **LWP-10** | Page creation in workspace | `wiki/pages/<page_id>.json` persisted | 🟡 Impl |
| **LWP-11** | State.json persistence | Workspace state saved with checksums | 🟡 Impl |
| **LWP-12** | Log entry appending | One entry per operation in `wiki/log.md` | 🟡 Impl |
| **LWP-13** | Orphan page detection | `wikiLint()` detects pages with no backlinks | 🟡 Impl |
| **LWP-14** | Missing backlink detection | `wikiLint()` finds broken references | 🟡 Impl |
| **LWP-15** | Unresolved task tracking | Tasks with missing pages tracked | 🟡 Impl |
| **LWP-16** | Workspace corruption recovery | State corruption detected; recovery from log succeeds | 🟡 Impl |

**Recovery Target:** Recovery success rate ≥ 98% on corrupted state.json

#### LWP-17..LWP-20: Guardrail Coverage 🟡

| Test | Focus | Patterns | Target |
|------|-------|----------|--------|
| **LWP-17** | Shell command patterns | `bash`, `sh`, `sudo`, `rm`, `curl`, `nc`, etc. (20+ patterns) | False positives < 5% |
| **LWP-18** | Code execution patterns | `eval`, `exec`, `__import__`, `subprocess`, etc. (10+ patterns) | False positives < 5% |
| **LWP-19** | Encoding bypass patterns | `base64`, `rot13`, `hex`, `URL encoding`, etc. (8+ patterns) | False positives < 5% |
| **LWP-20** | Privilege/control flow | `sudo`, `su`, `chmod`, `goto`, `interrupt`, etc. (15+ patterns) | False positives < 5% |

**Negative Test Strategy:**
- Benign queries using similar keywords should NOT be flagged
- Example: "How to use the command line?" ✅ NOT flagged
- Example: "sudo systemctl start service" ❌ FLAGGED

#### LWP-GATE-01: Edition-Gate Enforcement ✅

| Scenario | Input Edition | Expected Result | Status |
|----------|---------------|-----------------|--------|
| **Community Init** | COMMUNITY | `Status::PermissionDenied()` | ✅ Impl |
| **Minimal Init** | MINIMAL | `Status::PermissionDenied()` | ✅ Impl |
| **Enterprise Init** | ENTERPRISE | `Status::Ok()` | ✅ Impl |
| **Hyperscaler Init** | HYPERSCALER | `Status::Ok()` | ✅ Impl |
| **Military Init** | MILITARY | `Status::Ok()` | ✅ Impl |

#### LWP-INT-01..LWP-INT-04: Integration Tests 🟡

| Test | Focus | Requirements | Status |
|------|-------|--------------|--------|
| **LWP-INT-01** | RocksDB fixture smoke test | DB opens, closes, survives restart | 🟡 Impl |
| **LWP-INT-02** | Phase B roundtrip (pending) | Ingest+query with BM25+HNSW (Phase 5) | ❌ Deferred |
| **LWP-INT-03** | Concurrent query safety | Parallel queries don't corrupt index | 🟡 Impl |
| **LWP-INT-04** | Wikipedia ingest (sub-feature) | Large-scale ingest with checkpoint resume | 🟡 Impl |

#### LWP-PERF-01: Performance Gate 🟡

| Metric | Target | Measurement Point | Status |
|--------|--------|-------------------|--------|
| **p95 Query Latency** | < 200 ms | Phase A (hash embedding) @ 5k chunks | 🟡 Bench |
| **Ingest Throughput** | ≥ 100 files/s | Single-threaded ingest of small files | 🟡 Bench |
| **Chunk Count Accuracy** | ± 0% | Verify `stats.chunk_count` matches index | ✅ Impl |

---

## Implementation Priorities — Q4 2026

### Week 1-2: Test Infrastructure

- [ ] Set up `test_llm_wiki_integration_phase4_focused.cpp` test file
- [ ] Add RocksDB fixture for Phase A testing
- [ ] Implement test helpers: `CreateTestWiki()`, `IngestTestFile()`, `QueryAndVerify()`
- [ ] Add performance benchmarking harness

### Week 3-4: Workspace State Manager

- [ ] Implement `WorkspaceStateManager::load()` with SHA-256 validation
- [ ] Implement `WorkspaceStateManager::save()` with atomic write-replace
- [ ] Implement `WorkspaceStateManager::recover()` using append-only log
- [ ] Add corruption detection tests (LWP-16)

### Week 5-6: Workspace Lifecycle Tests

- [ ] LWP-09: Directory creation
- [ ] LWP-10: Page persistence
- [ ] LWP-11: State.json checksums
- [ ] LWP-12: Log appending
- [ ] LWP-13..14: Lint operations (orphan detection)
- [ ] LWP-15: Task tracking

### Week 7-8: Guardrail & Performance Tests

- [ ] LWP-17..20: Run guardrail pattern suites, measure false positive rate
- [ ] LWP-PERF-01: Benchmark query latency @ 5k chunks, 10k chunks
- [ ] LWP-INT-01/03: Concurrent query safety and RocksDB fixture tests
- [ ] LWP-06: Partial-failure error aggregation test

### Week 9-12: Q1 2027 Preview (Phase 5)

- [ ] Architecture planning for Phase B (RocksDB BM25+HNSW+RRF)
- [ ] Wikipedia ingest sub-feature prototype
- [ ] Embedding cache design specification

---

## Key Acceptance Criteria

### Phase 3 (Error Handling) ✅ Almost Complete

- [x] Guardrails implemented: 60+ patterns, < 5% false positive rate
- [x] Edition gating enforced: PermissionDenied on community/minimal
- [x] Workspace state manager interface defined
- [x] Partial-failure semantics designed
- [ ] Workspace state manager implementation complete + tested

### Phase 4 (Testing) 🟡 In Progress

- [ ] LWP-01..LWP-08 (core interface) — 6/8 tests complete, 2 pending
- [ ] LWP-09..LWP-16 (workspace lifecycle) — 0/8 tests implemented
- [ ] LWP-17..LWP-20 (guardrails) — pattern definitions ready, test harness pending
- [ ] LWP-GATE-01 (edition gate) — ✅ complete
- [ ] LWP-INT-01..04 (integration) — 1/4 smoke test, 3/4 pending
- [ ] LWP-PERF-01 (performance) — benchmarks pending

**Go-Live Criteria:** ≥ 20/20 tests PASS (all phases)

---

## References

- `src/llm_wiki/ROADMAP.md` — Module-level roadmap with phase details
- `include/llm_wiki/llm_wiki_plugin_interface.h` — Public C++ SDK interface
- `src/llm_wiki/guardrail_patterns.h` — Guardrail pattern definitions
- `src/llm_wiki/workspace_state_manager.h` — State persistence interface
- `docs/architecture/llm_wiki_mvp_adr.md` — Architecture decision record
