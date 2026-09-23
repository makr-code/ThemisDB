# Changelog — LLM Wiki Module

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

All notable changes to the LLM Wiki module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2.4.0-rc2] — 2026-09-22 – Phase 3-4 Complete: Wave B In-Progress

### Current Status

| Component | Status | Evidence |
|---|---|---|
| Phase 1 — API Contract | ✅ COMPLETE | `include/llm_wiki/llm_wiki_plugin_interface.h` implemented and frozen |
| Phase 2 — Core Implementation | ✅ COMPLETE | `LLMWikiPluginImpl`, workspace lifecycle, ingestion/query baseline wired |
| Phase 3 — Error Handling & Edge Cases | ✅ COMPLETE | Guardrails, partial-failure, checksum/recovery, edition-gate enforcement |
| Phase 4 — Test Suite | ✅ COMPLETE | 16 focused/integration/wave-gate test artifacts, all passing |
| Phase 5 — Performance Hardening | 🔄 IN PROGRESS | Benchmark baselines and representative-hardware validation in progress |
| Phase 6 — Documentation & GA Acceptance | 🔄 PENDING | Awaiting Wave B/D evidence and governance sign-off |

### Wave B Status

- [~] Complete wiki-routing cost-signal expansion, provenance propagation, and route-selection regression coverage (Target: Q4 2026)
- [~] Close security/governance runtime gates (allowlist evidence gate, deny-path determinism, drift checks) with persisted audit artefacts (Target: Q4 2026)
- [~] Finalize policy-loader hot-reload safety and rollback-trigger/canary promotion tests for YAML process orchestration (Target: Q1 2027)

### Delivered Artefacts (Phase 3-4)

| Artefact | Path | Gate |
|---|---|---|
| Guardrail pattern taxonomy | `src/llm_wiki/guardrail_patterns.h` | 60+ injected-command patterns; 5-category classification |
| Workspace state management | `src/llm_wiki/workspace_state_manager.h` | Atomic write-replace + log-based recovery |
| Edition gating enforcement | `src/llm_wiki/edition_gate.h/.cpp` | Edition-specific access control |
| YAML process orchestration | `src/llm_wiki/process/llm_wiki_process_policy.yaml` | Versioned policy for stage timing, gates, ML knobs |
| Process policy manager | `src/llm_wiki/process_policy_manager.cpp` | Runtime validation + hot-reload safety |
| Focused test suite | `tests/llm/test_llm_wiki_*.cpp` (16 artifacts) | Phase 4 gate coverage: LWP-01 through LWP-20+ |
| Integration tests | `tests/integration/test_llm_wiki_soak.cpp` | Sustained operation gates; no corruption; recall ≥ 0.9 |
| Benchmark gates | `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` | p95 latency, throughput, stability validation |

### Test Coverage Summary

- **Total Test Artifacts:** 16 (focused + integration + wave-gate)
- **All Tests Passing:** ✅ Verified
- **Benchmark Artifacts:** 2 (dedicated gates + throughput validation)
- **Test Timeout:** 120s standard per Phase 4+ gate specification

### Known Limitations (See MODULE_GAPS.md for details)

- No representative hardware performance baselines (p95/p99) for Wave B exit (in progress)
- Wikipedia ingest throughput evidence on CI/HW lanes pending
- Schema migration runner with compatibility matrix planned for Phase 6

---

## [2.3.0] — 2026-08-10 – Phase 3 Error Handling & Edge Cases Delivery

### Phase 3 Deliverables ✅ COMPLETE

- [x] Guardrail pattern detection for prompt injection prevention
  - 60+ attack patterns across 5 categories (shell, code, encoding, privilege, control-flow)
  - File: `src/llm_wiki/guardrail_patterns.h`
  - All patterns validated through focused tests (LWP-05, LWP-17 through LWP-20)

- [x] Workspace state management with atomic persistence
  - Checksum validation and atomic write-replace semantics
  - Log-based recovery from corruption
  - File: `src/llm_wiki/workspace_state_manager.h/.cpp`
  - Validation: LWP-06 (checksum validation), LWP-09 through LWP-16 (lifecycle)

- [x] Edition-gated access control enforcement
  - Per-edition capability matrix (Community excluded, Enterprise+, Military)
  - File: `src/llm_wiki/edition_gate.h/.cpp`
  - Validation: LWP-07 (edition gates allowed/denied paths)

- [x] Error handling and partial-failure semantics
  - Explicit error codes for wiki-specific failures
  - Graceful handling of invalid input, state corruption, policy violations
  - File: `src/llm_wiki/guardrail_patterns.h`, workspace_state_manager.cpp
  - Validation: LWP-08 (error handling tests)

- [x] YAML process policy loader with hot-reload safety
  - Versioned process orchestration policy
  - Runtime invariant validation on load
  - File: `src/llm_wiki/process/llm_wiki_process_policy.yaml`, process_policy_manager.cpp
  - Validation: Policy schema enforced, fail-closed on invalid policy

### Status: ✅ PHASE 3 COMPLETE

---

## [2.2.0] — 2026-07-15 – Phase 2 Core Implementation & Python MVP

### Phase 2 Deliverables ✅ COMPLETE

- [x] LLMWikiPluginImpl implementation
  - Wikipedia ingestion bridge
  - Query + ingest operations with guardrail checks
  - File: `plugins/private/themisdb_llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp`

- [x] Workspace lifecycle management
  - Create, delete, query, and status operations
  - Checksum-based state validation
  - File: `src/llm_wiki/workspace_state_manager.h/.cpp`

- [x] Python MVP CLI interface
  - Index, query, workspace management commands
  - File: `scripts/llm_wiki_mvp.py`

- [x] Plugin manifest and edition visibility
  - File: `plugins/private/themisdb_llm_wiki/plugin.json`
  - Edition gating: Community (no LLM Wiki), Enterprise+ (full access)

### Status: ✅ PHASE 2 COMPLETE

---

## [2.1.0] — 2026-06-20 – Phase 1 API Contract Finalization

### Phase 1 Deliverables ✅ FUNCTIONALLY COMPLETE

- [x] ILLMWikiPlugin SDK interface
  - Public C++ contract for wiki backends
  - Factory export: `themisdb_llm_wiki_create`
  - File: `include/llm_wiki/llm_wiki_plugin_interface.h`

- [x] Module boundary and plugin manifest
  - Edition-gated capabilities definition
  - File: `plugins/private/themisdb_llm_wiki/plugin.json`

- [x] Edition-gate contract enforcement
  - Community: No LLM Wiki
  - Enterprise/Hyperscaler/Military: Full LLM Wiki with guardrails

- [x] API methods frozen
  - `createWorkspace()`, `deleteWorkspace()`, `ingestPage()`, `queryWorkspace()`, `getWorkspaceStatus()`
  - All methods documented with Doxygen

### Status: ✅ PHASE 1 FUNCTIONALLY COMPLETE

---

## Version History

| Version | Date | Status | Phase(s) | Wave |
|---|---|---|---|---|
| 2.4.0-rc2 | 2026-09-22 | In Progress | 3-4 ✅, 5-6 🔄 | Wave B |
| 2.3.0 | 2026-08-10 | Complete | Phase 3 | Wave A → B transition |
| 2.2.0 | 2026-07-15 | Complete | Phase 2 | Wave A |
| 2.1.0 | 2026-06-20 | Complete | Phase 1 | Wave A |

---

## Notes

- Roadmap phases (1-6) and completion status are canonical references in `ROADMAP.md`
- Implementation phases and detailed gap tracking available in `ARCHITECTURE.md` and `MODULE_GAPS.md`
- Future enhancements and performance targets documented in `FUTURE_ENHANCEMENTS.md`
- Wave B evidence and governance gates documented in `ROADMAP.md` (root repository level)
- Performance expectations and benchmarks documented in `PERFORMANCE_EXPECTATIONS.md`
- Production requirements and operational constraints documented in `PRODUCTION_REQUIREMENTS.md`
