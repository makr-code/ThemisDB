# Prompt Engineering Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-05 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable prompt engineering runtime exists for template lifecycle operations, context injection, revision/version control, optimization/evaluation loops, feedback ingestion, and prompt metrics support.

## In Progress

- [x] hardening adversarial/edge-case template and injection validation behavior (Target: Q3 2026) — **COMPLETED 2026-08-08**
- [~] improving optimization/evaluation diagnostics consistency across failure classes (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for prompt engineering hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for concurrent template/version mutation traffic (Target: Q4 2026)
- [ ] expand stress coverage for optimization loops and feedback-heavy scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for prompt incident triage (Target: Q4 2026)
- [ ] introduce deterministic `RewriteEngine` for prompt normalization, policy rewrites, and NL→AQL preprocessing (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for prompt template/version/quality paths (Target: Q1 2027)
- [ ] broaden benchmark depth for module-native advanced prompt workflows (Target: Q1 2027)
- [ ] harden long-run reliability under sustained prompt update/evaluation pressure (Target: Q1 2027)
- [ ] extend `RewriteEngine` to post-generation canonicalization and structured agent/tool output normalization (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze template/versioning/optimization contracts for current major line (Target: Q3 2026) — See PHASE_1_CONTRACT.md
- [x] define explicit error taxonomy for prompt engineering failure classes (Target: Q3 2026) — See ERROR_TAXONOMY_REFERENCE.md and prompt_engineering_errors.h (52 error codes)
- [x] define `RewriteEngine` interfaces (`RewriteDocument`, `RewriteContext`, `RewriteResult`, `RewriteTrace`, `IRewriteRule`) and phase boundaries (Target: Q4 2026) — See rewrite_engine.h

### Phase 2: Core Implementation
- [x] complete hardening for manager/version control and validator internals (Target: Q4 2026) — PHASE_1_CONTRACT.md baseline established
- [x] align optimization/evaluation behavior to bounded runtime contracts (Target: Q4 2026) — baseline contracts defined
- [x] implement deterministic ordered rule execution, rule registration, and YAML-backed low-risk rewrite loading (Target: Q4 2026) — RewriteEngine implementation complete
  - `rewrite_engine.cpp`: Core orchestration with phase ordering, priority-based execution, max-steps prevention
  - `rewrite_rule_base.cpp`: Concrete rule types (RegexRewriteRule, DictionaryRewriteRule, PolicyRewriteRule, SemanticRewriteRule)
  - `rewrite_rule_loader.cpp`: YAML schema validation and lexical rule loading
  - `rewrite_metrics.cpp`: Observability with per-rule and per-phase metrics
- [x] implement `RewriteEngine` executor with deterministic rule evaluation and phase isolation (Target: Q4 2026) — all 4 phases (input normalization, policy, NL→AQL, post-gen) implemented
- [x] implement YAML configuration schema for input normalization rules (Target: Q4 2026) — regex and dictionary rules with schema validation

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for invalid templates, injection mismatches, and version faults (Target: Q4 2026) — **COMPLETED 2026-08-08**
  - `PromptTemplateValidator` extended with injection detection methods
  - SQL, command, path traversal, and template injection patterns detected
  - Adversarial test cases: 40+ test scenarios covering evasion attempts and edge cases
  - Integration: `validate()` automatically checks template content for injection patterns
- [x] unify diagnostics across manager/version/optimizer/evaluator incidents using `PromptEngineeringErrorContext` (Target: Q4 2026)
- [x] enforce rewrite step bounds, phase isolation, malformed-rule rejection, and terminal policy behavior (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for invalid template and concurrent mutation scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for optimization/evaluation workloads (Target: Q4 2026)
- [ ] add unit/integration coverage for rewrite rule ordering, trace generation, normalization idempotence, and NL→AQL preprocessing flows (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for prompt engineering hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
- [ ] benchmark rewrite latency/trace overhead and harden regex/rule execution against adversarial expansion or loop behavior (Target: Q1 2027)

### Phase 6: Documentation and Acceptance
- [~] core prompt_engineering docs aligned to source-verifiable behavior (Target: Q3-Q4 2026) — In progress with Phase 1 deliverables
- [x] roadmap/future planning separated from historical changelog entries
- [x] focused test infrastructure (PE-FT-001..PE-FT-015) implemented at `tests/prompt_engineering/test_prompt_engineering_focused.cpp` (Target: Q3 2026)
- [ ] document and integrate rewrite engine architecture and operational guidance (`docs/architecture/rewrite_engine_architecture.md`) (Target: Q4 2026)

## Production Readiness Checklist

- [x] core prompt engineering surfaces documented and source-verified
- [x] module-level security and failure behavior documented (Phase 1: error taxonomy in prompt_engineering_errors.h and ERROR_TAXONOMY_REFERENCE.md)
- [x] benchmark mapping documented in performance expectations
- [x] focused test infrastructure (PE-FT-001..PE-FT-015) implemented for core surfaces
- [ ] remaining hardening tasks closed for template/versioning/quality edge paths (Phase 3 + Phase 4 work)
- [ ] release benchmark stabilization complete (Phase 5 work)
- [ ] rewrite engine deterministic rule execution, bounded behavior, and audit trace support validated in production-like test profiles (Phase 2–5 work)
- [ ] backward compatibility guarantee documented and reviewed (Phase 1: PHASE_1_CONTRACT.md)

## Known Issues and Limitations

- runtime behavior depends on template quality, injected context shape, optimizer configuration, and downstream model behavior.
- selected adversarial and concurrency-heavy edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced prompt engineering workflows.
- rewrite-driven normalization and policy behavior are not yet implemented and remain roadmap work.

## Breaking Changes

No breaking prompt engineering contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Module Evidence and Validation (Q3 2026 Snapshot)

### Focused Test Infrastructure (PE-FT-001..PE-FT-015)

Implemented focused test suite at `tests/prompt_engineering/test_prompt_engineering_focused.cpp`:

| Test ID | Area | Coverage | Status |
|---------|------|----------|--------|
| PE-FT-001 | PromptManager template lifecycle | create/get/list operations | ✅ PASS |
| PE-FT-002 | Context injection | context map substitution | ✅ PASS |
| PE-FT-003 | Template validation | valid/invalid template checks | ✅ PASS |
| PE-FT-004 | PromptVersionControl | commit/history operations | ✅ PASS |
| PE-FT-005 | FeedbackCollector | feedback recording and stats | ✅ PASS |
| PE-FT-006 | PromptOptimizer | basic optimization loop | ✅ PASS |
| PE-FT-007 | PromptEvaluator | structural evaluation | ✅ PASS |
| PE-FT-008 | PromptEngineeringMetrics | metrics recording | ✅ PASS |
| PE-FT-009 | Error handling | missing template edge case | ✅ PASS |
| PE-FT-010 | Error handling | invalid context injection edge case | ✅ PASS |
| PE-FT-011 | Concurrency | basic concurrent template access | ✅ PASS |
| PE-FT-012 | Injection validation | empty placeholder edge case | ✅ PASS |
| PE-FT-013 | Version control | commit history consistency | ✅ PASS |
| PE-FT-014 | Optimizer diagnostics | diagnostic output validation | ✅ PASS |
| PE-FT-015 | Evaluator consistency | multiple evaluation consistency | ✅ PASS |

### Benchmark Evidence (Existing)

- `benchmarks/prompt_engineering/bench_prompt_engineering.cpp` provides performance validation
- Covers 13 benchmark targets for template operations, context injection, version control, feedback, optimization, and evaluation
- Performance targets documented and tracked (e.g., createTemplate < 10 µs, getTemplate < 1 µs)

### Build Targets

- **Build** (Preset: `community-release-allow-missing-rocksdb`)
  - `module_prompt_engineering_test_prompt_engineering_focused_focused` - focused test executable
  - `bench_prompt_engineering` - benchmark executable
  
- **Test Registry**
  - Focused tests registered with: `themis_register_module_focused_test(MODULE prompt_engineering TIER unit TIMEOUT 120)`
  - CTest labels: `prompt_engineering` for filtering and aggregation

### Validation Summary

- ✅ Roadmap priorities (8 Q3/Q4/Q1 2027 items) synced with ROADMAP.md
- ✅ Future enhancements scope aligned with FUTURE_ENHANCEMENTS.md
- ✅ Core API surfaces tested (template, versioning, feedback, optimization, evaluation, metrics)
- ✅ Error handling and edge cases covered (15 focused test cases)
- ✅ Concurrency sanity check included
- ✅ Benchmark infrastructure exists and measurable against documented targets

## Phase 2: RewriteEngine Core Implementation Evidence (2026-08-05)

### Implementation Deliverables

**Core Engine Implementation**
- `src/prompt_engineering/rewrite_engine.cpp` (495 lines)
  - Phase-ordered deterministic execution (phases 1-4 in strict sequence)
  - Thread-safe rule registration and management with shared_mutex
  - Max-steps loop prevention (default 1000, configurable per context)
  - Rule lookup optimization by phase
  - Trace collection with optional max_trace_entries limit
  - JSON stats export with rule counts, latency, transformation metrics

**Concrete Rule Type Implementations**
- `src/prompt_engineering/rewrite_rule_base.cpp` (409 lines)
  - `RegexRewriteRule`: Precompiled regex patterns with backreference support, max_replacements bound
  - `DictionaryRewriteRule`: Hash-based substitution with case-sensitive/insensitive modes
  - `PolicyRewriteRule`: Custom match/apply callbacks for semantic policy enforcement (terminal mode)
  - `SemanticRewriteRule`: Base class for complex C++-only rule implementations

**YAML Rule Loader**
- `src/prompt_engineering/rewrite_rule_loader.cpp` (328 lines)
  - Schema validation for YAML rule definitions
  - Lexical-only constraint enforcement (regex and dictionary rules only from YAML)
  - All-or-nothing loading semantics (partial failures don't modify registry)
  - Phase/priority/mapping validation with detailed error messages
  - Regex compilation validation at load time

**Observability**
- `src/prompt_engineering/rewrite_metrics.cpp` (230 lines)
  - Per-rule metrics: match/apply/error counts, min/max/avg latency
  - Per-phase metrics: aggregated rules_evaluated/rules_applied/total_latency
  - Global singleton metrics collection with reset capability
  - JSON export with timestamp for integration with monitoring systems

### Test Coverage (RW-P2-01..06)

| Test ID | Coverage | Location | Status |
|---------|----------|----------|--------|
| RW-P2-01 | Rule registration and deduplication | test_rewrite_engine_focused.cpp:45-113 | ✅ PASS |
| RW-P2-02 | Phase ordering enforcement | test_rewrite_engine_focused.cpp:125-160 | ✅ PASS |
| RW-P2-03 | Priority-based execution within phases | test_rewrite_engine_focused.cpp:172-207 | ✅ PASS |
| RW-P2-04 | Max-steps loop prevention | test_rewrite_engine_focused.cpp:219-256 | ✅ PASS |
| RW-P2-05 | Trace correctness and completeness | test_rewrite_engine_focused.cpp:268-330 | ✅ PASS |
| RW-P2-06 | Thread-safety under concurrent register/rewrite | test_rewrite_engine_focused.cpp:342-427 | ✅ PASS |

**Additional Tests**
- Multi-phase execution integration test
- Dictionary rule substitution
- Statistics tracking and JSON export
- 28 focused test cases total covering RW-FT-001..028 scenarios

### Header Files Created

- `include/prompt_engineering/rewrite_rule.h` (238 lines)
  - Interface declarations for all 4 rule types
  - Callback function type aliases for PolicyRewriteRule
  - Doxygen documentation for all public methods

- `include/prompt_engineering/rewrite_rule_loader.h` (62 lines)
  - RewriteRuleLoader class with load/validate/error reporting
  - All-or-nothing semantics documented

### Build Configuration

- `src/prompt_engineering/CMakeLists.txt` created
  - `themis_prompt_engineering_rewrite` library target
  - Linked against nlohmann_json, spdlog, yaml-cpp
  - C++17 standard requirement
  - Integrated with test infrastructure via glob pattern in tests/prompt_engineering/CMakeLists.txt

### Acceptance Criteria - Phase 2 Complete ✅

- ✅ RewriteEngine core implemented and linked
- ✅ All rule types (regex, dictionary, policy, semantic) compilable and functional
- ✅ YAML loading works for lexical rules with schema validation
- ✅ 6 focused RW-P2 tests passing (28 total test cases)
- ✅ No blocking compile/link errors
- ✅ Thread-safety verified under concurrent operations
- ✅ Deterministic phase ordering enforced
- ✅ Observability metrics collected and exported

### Next Steps: Phases 3-6 Parallel Streams

- **Stream B (Secondary):** Phase 3 error paths (RW-P3-01..04) — can proceed immediately
- **Stream C (Tertiary):** Phase 4 comprehensive tests (RW-FT-001..060 expansion)
- **Stream D (Performance):** Phase 5 benchmarking (bench_rewrite_engine.cpp)
- **Stream E (Documentation):** Phase 6 docs (operational guidance, YAML schema reference)

