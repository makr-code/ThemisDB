# Prompt Engineering Module - Future Enhancements

## Scope

This document covers planned enhancements to ThemisDB's prompt engineering subsystem, which manages LLM prompt templates, chain-of-thought construction, RAG prompt assembly, and system prompt versioning. It targets the gap between the current Alpha/experimental state of `prompt_manager.cpp`, `prompt_optimizer.cpp`, and `prompt_version_control.cpp` and a robust, observable prompt lifecycle suitable for production legal-domain inference.

## Design Constraints

- Prompt templates must be versioned and immutable once published; changes must produce a new version tracked by `prompt_version_control.cpp`.
- The module must not introduce hard dependencies on a specific LLM provider; model-specific behaviour must be encapsulated behind the `IPromptRenderer` interface.
- Prompt construction latency must not exceed 5 ms (P99) on the critical inference path to stay within the latency budget of the RAG pipeline.
- Prompt content that may include PII must be routed through `utils/pii_detector.cpp` before transmission to any external endpoint.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `PromptManager::render(template_id, context)` | `prompt_engineering_integration.cpp`, RAG module | Returns rendered string; throws on missing variables |
| `PromptVersionControl::publish(template)` | CI/CD pipeline, admin API | Immutable publish; version hash stored in metadata DB |
| `PromptEvaluator::score(prompt, response)` | `feedback_collector.cpp`, `self_improvement_orchestrator.cpp` | Returns quality score 0–1 |
| `PromptPerformanceTracker::record()` | `prompt_engineering_metrics.cpp` | Per-template token count, latency, cost |
| `MetaPromptGenerator::generate(task_spec)` | `self_improvement_orchestrator.cpp` | Synthesises new templates from task descriptions |

## Planned Features

### [ ] Structured Prompt Template DSL
**Priority:** High
**Target Version:** v0.9.0

Replace ad-hoc string interpolation in `prompt_manager.cpp` with a typed template DSL that supports typed variable slots (string, list, document-chunk), conditional blocks, and loop constructs. The DSL compiles to a `CompiledPromptTemplate` object that is validated at publish time rather than at render time.

**Implementation Notes:**
- Add `prompt_template_compiler.cpp` with a recursive-descent parser for the DSL; expose `CompiledPromptTemplate::render(PromptContext&)`.
- Update `prompt_version_control.cpp` to store the compiled AST alongside the raw template source for faster re-render.
- `prompt_manager.cpp` must fall back to legacy string-substitution for templates without a DSL version field (backward compat).
- Integrate variable-type validation with `utils/input_validator.cpp` to catch mismatched context variables at publish time.

**Performance Targets:**
- Template compilation (publish time): <50 ms for a 4 KB template.
- Compiled template render latency: <1 ms P99 for a 2 KB context.

---

### [ ] Chain-of-Thought Step Tracer
**Priority:** High
**Target Version:** v0.9.0

Instrument chain-of-thought prompt construction so that each reasoning step is individually traced via `utils/tracing.cpp`. This enables offline analysis of which CoT steps contribute to answer quality and which introduce hallucination risk in the legal-domain context.

**Implementation Notes:**
- Extend `prompt_engineering_integration.cpp` with a `CoTTraceCollector` that emits one OpenTelemetry span per CoT step, carrying `step_index`, `token_count`, and `template_id` attributes.
- Wire `CoTTraceCollector` into the existing `utils/tracing.cpp` span context so traces propagate correctly through the RAG pipeline.
- `prompt_performance_tracker.cpp` must aggregate per-step token counts for cost attribution by legal matter ID.
- Store CoT traces in the timeseries module (`timeseries/tsstore.h`) for retention and aggregation.

**Performance Targets:**
- Tracing overhead per CoT step: <0.2 ms.
- Trace storage: <500 bytes per step after ZSTD compression (`utils/zstd_codec.cpp`).

---

### [ ] Automated Prompt Quality Regression Suite
**Priority:** Medium
**Target Version:** v0.10.0

Build a regression harness around `prompt_evaluator.cpp` that runs on every template version publish to detect quality regressions. The harness compares the new template's `PromptEvaluator` score against the previous published version on a fixed golden-set of legal-domain prompts.

**Implementation Notes:**
- Add `prompt_regression_runner.cpp` that loads golden-set fixtures from `tests/prompt_engineering/golden/` and calls `PromptEvaluator::score()` for each.
- Integrate with `feedback_collector.cpp` to pull real-world human-feedback scores as additional regression signal.
- Block publish in `prompt_version_control.cpp` if the mean regression-suite score drops >5% vs. the current published version.
- Emit regression results as structured log entries via `utils/logger.cpp` with template ID, version, and per-fixture delta scores.

**Performance Targets:**
- Full regression suite (100 golden prompts) runtime: <60 s against a mock LLM stub.
- False-positive regression block rate: <2% over a 30-day rolling window.

---

### [ ] RAG Context Window Budget Manager
**Priority:** High
**Target Version:** v0.9.0

Add a `ContextWindowBudgetManager` to `prompt_engineering_integration.cpp` that enforces per-model token limits. It ranks retrieved document chunks by relevance score, then greedily packs chunks until the token budget is reached, ensuring the system prompt and CoT scaffolding always fit.

**Implementation Notes:**
- Implement `ContextWindowBudgetManager` in a new `context_window_manager.cpp`; consume the model's `max_tokens` from the `meta_prompt_generator.cpp` model registry.
- Token counting must use the model's actual tokenizer (tiktoken-compatible BPE); add `tokenizer_bridge.cpp` wrapping a shared library call.
- Expose a `PromptBudgetExceededError` structured error for callers to handle gracefully (e.g., reduce chunk count).
- Track budget utilization per request in `prompt_engineering_metrics.cpp` for capacity planning.

**Performance Targets:**
- Budget computation for 20 candidate chunks: <2 ms P99.
- Token counting via BPE bridge: <0.5 ms per 512-token chunk.

---

### [ ] Prompt A/B Experimentation Framework
**Priority:** Medium
**Target Version:** v1.0.0

Extend `prompt_version_control.cpp` and `prompt_optimizer.cpp` to support traffic-split A/B experiments between prompt template versions. Experiment assignment is deterministic per `request_id` (hash-based) and results feed into `feedback_collector.cpp` for statistical significance testing.

**Implementation Notes:**
- Add `PromptExperiment` entity to `prompt_version_control.cpp`: stores control/treatment template IDs, traffic split ratio, and start/end timestamps.
- `prompt_manager.cpp::render()` accepts an optional `experiment_context` struct; if present, selects variant via `murmur3(request_id) % 100 < split_pct`.
- `self_improvement_orchestrator.cpp` consumes experiment outcome data to auto-promote the winning variant after reaching statistical significance (p < 0.05, min 200 samples).
- Emit per-variant metrics to `prompt_performance_tracker.cpp` tagged with `experiment_id` and `variant`.

**Performance Targets:**
- Variant selection overhead: <0.1 ms per request.
- Minimum detectable effect size at p < 0.05 with 200 samples: 10% relative score improvement.

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >85% new code | Cover DSL compiler, `ContextWindowBudgetManager`, experiment variant selection |
| Integration | All prompt lifecycle stages | Template compile → publish → render → evaluate → feedback loop |
| Regression | 100% golden-set prompts | Run on every template publish via `prompt_regression_runner.cpp` |
| Performance | P99 < budgets above | Micro-benchmark render, token counting, and CoT tracing paths |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| `render()` latency P99 | ~8 ms (string interp) | <1 ms | Compiled DSL + warm context cache |
| Context packing for 20 chunks | N/A | <2 ms | `ContextWindowBudgetManager` microbenchmark |
| End-to-end prompt assembly (RAG) | ~15 ms | <5 ms | Trace span aggregation in `utils/tracing.cpp` |
| `PromptEvaluator::score()` throughput | N/A | >500 req/s | Batch scoring with mock LLM stub |
| Template publish (compile + validate) | N/A | <50 ms | DSL compiler benchmark on 4 KB template |

## Security / Reliability

- [ ] Rendered prompts containing fields derived from user input must pass through `utils/pii_detector.cpp` before transmission; any detected PII must be pseudonymized via `utils/pii_pseudonymizer.cpp`.
- [ ] Prompt templates loaded from external sources must be stored with an integrity hash in `prompt_version_control.cpp`; hash mismatch at render time must abort execution and emit an audit event via `utils/audit_logger.cpp`.
- [ ] The A/B experimentation framework must not leak experiment assignments across tenant boundaries; `experiment_context` must be scoped to a single tenant ID.
- [?] Clarify whether chain-of-thought traces containing legal case content are subject to e-discovery retention requirements before enabling long-term storage.
- [ ] `ContextWindowBudgetManager` must enforce a hard maximum token cap regardless of model-reported limit to prevent prompt-injection via oversized context chunks.
