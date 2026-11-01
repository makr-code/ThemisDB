---
Author: "ThemisDB Team"
Created: "2026-08-05"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering Module Roadmap

## Current Status

The source tree contains implementations for template management, version control, evaluation/optimization, feedback, metrics, and the standalone rewrite engine. The root build adds these sources to `themis_core`. Code presence is not a production-readiness or test-pass claim; release acceptance requires current test, benchmark, and human-review evidence.

The rewrite engine is not invoked automatically by `PromptEngineeringIntegration`. Its current focused test checks three interface/compile contracts and does not exercise rule execution.

## In Progress

- [~] Add behavior-level tests for rewrite rule execution, YAML loading, terminal behavior, tracing, and step exhaustion (Target: Q4 2026).
- [~] Run the focused prompt-engineering test label and record current results for the registered test sources (Target: Q4 2026).
- [~] Replace synthetic rewrite microbenchmarks with cases that exercise `RewriteEngine` and `RewriteRuleLoader` directly (Target: Q4 2026).
- [~] Establish release-profile measurements and baselines for the implemented prompt-engineering cases (Target: Q4 2026).

## Planned Features

- [ ] Integrate rewrite execution at explicitly owned prompt/request boundaries, with downstream validation and policy checks (Target: Q1 2027).
- [ ] Add representative fixtures for optimization/evaluation and long-run mutation workloads (Target: Q1 2027).
- [ ] Expand operator diagnostics based on verified failure behavior (Target: Q1 2027).

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Define public template, evaluation, revision, and rewrite interfaces in `include/prompt_engineering/` (Target: Q3 2026).
- [x] Define 62 `PromptEngineeringErrorCode` enum values and document their limits as identifiers, not guaranteed runtime emissions (Target: Q3 2026).

### Phase 2: Core Implementation
- [x] Implement the template, version, feedback, evaluation, and optimization source paths (Target: Q4 2026).
- [x] Implement the separate rewrite engine, rule types, metrics, and YAML loader; YAML loading is restricted to lexical regex/dictionary rules (Target: Q4 2026).
- [x] Keep the rewrite engine distinct from automatic integration-hook behavior (Target: Q4 2026).

### Phase 3: Error Handling and Edge Cases
- [x] Keep manager field validation distinct from serialized-JSON injection heuristics (Target: Q4 2026).
- [~] Add and verify regression coverage for malformed templates, persistence failures, duplicate rewrite rules, and rewrite termination (Target: Q4 2026).

### Phase 4: Tests
- [x] Register `test_*.cpp` files from `tests/prompt_engineering/CMakeLists.txt` as focused unit targets (Target: Q3 2026).
- [~] Add behavior-level rewrite tests; `test_rewrite_engine_focused.cpp` currently contains only three interface/compile-contract cases (Target: Q4 2026).
- [~] Record a current run of the `prompt_engineering` CTest label; this roadmap does not claim that the tests pass (Target: Q4 2026).

### Phase 5: Performance and Hardening
- [x] Add core microbenchmark cases for template, version, feedback, optimization, evaluation, metrics, and concurrent-write paths (Target: Q4 2026).
- [~] Benchmark actual rewrite implementation behavior; `bench_rewrite_engine.cpp` currently measures local synthetic helpers (Target: Q4 2026).
- [~] Collect release-profile results and compare with reviewed baselines before calling any performance gate complete (Target: Q4 2026).

### Phase 6: Documentation and Acceptance
- [~] Keep API, test, benchmark, and runtime documentation synchronized with source and CMake registration (Target: Q4 2026).
- [ ] Complete human review of module readiness and supporting test/benchmark evidence before a release claim (Target: Q4 2026).

## Production Readiness Checklist

- [x] Core public headers and implementation sources are present.
- [x] Focused module tests and benchmark targets are wired into the repository build.
- [~] Rewrite behavior tests cover execution semantics and error cases (Target: Q4 2026).
- [~] Release-profile benchmarks exercise production implementations and have reviewed baselines (Target: Q4 2026).
- [ ] Human acceptance confirms the release criteria and operational ownership (Target: Q4 2026).

## Known Issues & Limitations

- `PromptEngineeringIntegration` does not automatically invoke `RewriteEngine`; callers must not assume requests are rewritten.
- `test_rewrite_engine_focused.cpp` checks interface contracts only; it is not evidence of rule execution behavior.
- `bench_rewrite_engine.cpp` uses local synthetic helpers and is not evidence of `RewriteEngine` performance.
- The error-code enum does not establish that each code is emitted by current runtime paths.
- Prompt scoring quality depends on caller-provided cases and, when configured, an external embedding provider.
- Prompt substitution and injection heuristics do not replace caller-owned authorization, output validation, or safety policy.

## Breaking Changes

No breaking change is scheduled here. Update the public header contract, migration guidance, and changelog before any breaking API or persisted-format change.

## Historical Evidence

Older PE-FT/RW test IDs and pass tables are historical snapshots, not current test names or results. See [`ISSUE_5663_CLOSURE_EVIDENCE.md`](./ISSUE_5663_CLOSURE_EVIDENCE.md) for the dated closure record. Current test source inventory and commands are in [`../../tests/prompt_engineering/README.md`](../../tests/prompt_engineering/README.md); benchmark source limitations are in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
