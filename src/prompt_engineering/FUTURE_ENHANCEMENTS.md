<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/prompt_engineering/ -->

# Prompt Engineering Module - Future Enhancements
<!-- Links: src/prompt_engineering/README.md · src/prompt_engineering/ROADMAP.md · src/prompt_engineering/ARCHITECTURE.md · docs/de/prompt_engineering/README.md -->

## Scope

This document covers planned enhancements to ThemisDB's prompt engineering subsystem, which manages LLM prompt templates, chain-of-thought construction, RAG prompt assembly, and system prompt versioning. It targets the gap between the current production-ready state of the core components and advanced future capabilities such as a typed DSL, CoT tracing, and automated quality regression.

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

### [x] Structured Prompt Template DSL
**Priority:** High
**Target Version:** v0.9.0
**Status:** ✅ Implemented (v2.1.0)

Replace ad-hoc string interpolation in `prompt_manager.cpp` with a typed template DSL that supports typed variable slots (string, list, document-chunk), conditional blocks, and loop constructs. The DSL compiles to a `CompiledPromptTemplate` object that is validated at publish time rather than at render time.

**Implemented Components:**
- `SlotType` enum — `STRING`, `LIST`, `DOCUMENT_CHUNK`.
- `SlotDefinition` struct — `name`, `type`, `required`, `default_value`; `toJson()`.
- `PromptContextValue` — typed runtime value; `fromString()`, `fromList()`, `fromChunks()`; `toString()`, `asBool()`.
- `PromptContext` — `unordered_map<string, PromptContextValue>`.
- `IPromptTemplate` — abstract interface: `source()`, `slots()`, `render(ctx)`, `validate(ctx) noexcept`.
- `CompiledPromptTemplate` — implements `IPromptTemplate`; holds compiled AST; `toJson()`.
- `PromptTemplateCompiler` — stateless; `compile(source, slots)` → recursive-descent lex+parse.
- `PromptTemplateCompileError`, `PromptTemplateMissingSlotError`, `PromptTemplateTypeMismatchError`.
- `PromptTemplateValidator` — structural JSON validator for serialised `PromptTemplate` documents; `validate(json)`, `validate(string)`; `require_id` flag.
- DSL syntax: `{var}` / `{{ var }}` slots; `{% if var %}...{% else %}...{% endif %}`; `{% for item in list %}...{% endfor %}`.
- Backward-compatible with existing `{placeholder}` convention from `PromptManager::injectContext()`.
- 30 unit tests (AC-1 through AC-30); registered in `tests/CMakeLists.txt` as `test_prompt_template_compiler_focused`.

**Performance Targets (met):**
- Compile a 4 KB template: < 50 ms.
- Render with a 2 KB context: < 1 ms P99.

---

### [x] Chain-of-Thought Step Tracer
**Priority:** High
**Target Version:** v0.9.0
**Status:** ✅ Implemented (v1.7.0)

Instrument chain-of-thought prompt construction so that each reasoning step is individually traced.  The implementation enables offline analysis of which CoT steps contribute to answer quality and their per-step construction latency.

**Implemented Components:**
- `IChainOfThoughtTracer` — pluggable interface; `onStepBegin(StepId, label) noexcept`, `onStepEnd(StepId, content, duration) noexcept`.
- `CoTSpanRecord` — span value type: step_index, label, content, token_count (BPE approx), duration, start_time; `toJson()`.
- `RecordingCoTTracer` — concrete in-memory tracer for testing; `spans()`, `reset()`, `toJson()`.
- `CoTTraceCollector` — fan-out tracer forwarding to N children; `addTracer()`, `removeTracer()`, `totalStepsTraced()`, `toJson()`.
- `ChainOfThoughtBuilder::attachTracer()` / `detachTracer()` / `hasTracer()`.
- `build()` fires per-step callbacks; exceptions in tracer implementations are caught and suppressed.
- 30 unit tests (AC-1 through AC-30); CI: `cot-tracer-ci.yml`.

**Performance Targets:**
- Tracing overhead per CoT step: <0.2 ms.

---

### [x] Automated Prompt Quality Regression Suite
**Priority:** Medium
**Target Version:** v0.10.0
**Status:** ✅ Implemented (v1.8.0)

Regression harness around `PromptEvaluator` that compares candidate vs. baseline
outputs on a fixed golden-set and optional human-feedback fixtures.

**Implemented Components:**
- `RegressionFixture` — evaluation pair: prompt_text, expected_output, source (`"golden"` / `"feedback"`); `toJson()` / `fromJson()`.
- `RegressionConfig` — `max_regression_pct` (5 %), `min_fixtures`, `confidence_level`, `block_on_regression`.
- `RegressionResult` — `delta_pct`, `is_regression`, `blocked`, `inconclusive`, `statistically_significant`, per-fixture `FixtureDelta`s; `toJson()`.
- `PromptRegressionRunner` — fixture management; `loadFeedbackFixtures()` (imports `USER_POSITIVE` entries from `FeedbackCollector`); `run()` computes scores via `PromptEvaluator::evaluateSingle()` and Welch t-test; `setLogCallback()` for structured JSON log events.
- 30 unit tests (AC-1 through AC-30); CI: `prompt-regression-runner-ci.yml`.

---

### [x] RAG Context Window Budget Manager

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

### [x] Reflection Tuning with Dynamic Self-Aware Prompting
**Priority:** High
**Target Version:** v1.5.0
**Status:** ✅ Implemented (v1.5.0)

Implement an iterative self-critique and revision cycle (`ReflectionTuner`) that improves LLM responses through structured reflection, grounded in:

- Madaan et al. (NeurIPS 2023) "Self-Refine: Iterative Refinement with Self-Feedback"
- Shinn et al. (NeurIPS 2023) "Reflexion: Language Agents with Verbal Reinforcement Learning"
- Bai et al. (Anthropic 2022) "Constitutional AI: Harmlessness from AI Feedback"
- Li et al. (2023) "Reflection-Tuning: Recycling Data for Better Instruction Tuning"

The "self-aware" component dynamically adapts critique prompts based on the model's own self-reported confidence and linguistic uncertainty markers extracted from its previous response.

**Implementation Notes:**
- `ReflectionTuner` in `include/prompt_engineering/reflection_tuner.h` + `src/prompt_engineering/reflection_tuner.cpp`.
- `IReflectionProvider` interface with `generate`, `critique`, `revise`, `score` methods; fallback template-and-heuristic mode when no provider is attached.
- `DynamicReflectionPromptBuilder` generates strategy-specific prompts (SELF_REFINE / REFLEXION / CONSTITUTIONAL / SOCRATIC) and injects `SelfAwareContext`.
- `SelfAwareContext::fromResponse()` scans for linguistic confidence/uncertainty markers; confidence ratio drives adaptive critique emphasis.
- `ReflectionHallucinationGuard` uses two mechanisms to prevent hallucination amplification: marker scan (Golem.de, 2026-03: "Selbstkritik bis hin zur Halluzination") and rolling-average quality divergence detection.
- `ReflectionConfig` exposes all tuning parameters: strategy, `max_iterations`, `convergence_threshold`, `min_delta_improvement`, `divergence_threshold`, `divergence_window`, `constitutional_principles`, `include_self_aware_context`.
- `ReflectionResult::toJson()` emits the complete trace for logging and observability.

**Performance Targets:**
- Single reflection iteration (prompt construction only): <0.5 ms P99.
- Full 3-iteration cycle without an LLM backend: <1 ms P99.
- `SelfAwareContext::fromResponse()` for a 512-token response: <0.1 ms.

**Security / Reliability:**
- The `ReflectionHallucinationGuard` fires on any of 15 known hallucination marker phrases found in the critique and on rolling quality divergence, preventing the reflection loop from amplifying fabricated content.
- Constitutional principles must be curated by the application; the module does not impose defaults to avoid silently constraining output in unexpected domains.

**Test Strategy:**
- 38 unit tests covering AC-1 through AC-20 in `tests/test_reflection_tuner.cpp`.
- Four mock provider types: `ConstantMockProvider`, `ImprovingMockProvider`, `HallucinatingCritiqueProvider`, `DivertingMockProvider`.
- CI: `.github/workflows/reflection-tuner-ci.yml`, multi-platform (GCC-12/14, Clang-15).

---

### [x] Prompt A/B Experimentation Framework
**Priority:** Medium
**Target Version:** v1.0.0
**Status:** ✅ Implemented (v1.9.0)

Public A/B experiment framework for prompt template variants with deterministic
per-request traffic splitting and automated winner promotion.

**Implemented Components:**
- `ExperimentVariant` — `CONTROL` / `TREATMENT`.
- `ExperimentContext` — routing struct: `experiment_id` + `request_id`.
- `ExperimentStatus` — `RUNNING`, `WINNER_CONTROL`, `WINNER_TREATMENT`, `INCONCLUSIVE`, `COMPLETED`.
- `PromptExperiment` — descriptor: `split_pct`, `min_samples`, `confidence_level`; `toJson()` / `fromJson()`.
- `ExperimentOutcome` — single scored observation; `toJson()`.
- `ExperimentSummary` — per-variant counts, mean scores, `delta_pct`, `p_value`, `significant`, `winner_version_id`; `toJson()`.
- `PromptABExperimentFramework` — MurmurHash3-32 variant assignment; Welch two-sample t-test; auto-promotes winner at `min_samples` + `p < alpha`; exception-safe `WinnerCallback`.
- 30 unit tests (AC-1 through AC-30); CI: `prompt-ab-experiment-ci.yml`.

---

### [x] Prompt Library Import/Export
**Priority:** Medium
**Target Version:** v1.0.0
**Status:** ✅ Implemented (v2.0.0)

Cross-environment portability for prompt template collections via JSON and YAML serialisation.

**Implemented Components:**
- `PromptLibraryBundle` — self-contained snapshot: `name`, `description`, `version`, `format_version`, `created_at`, `checksum`, `templates`; `toJson()` / `fromJson()`.
- `ExportFormat` — `JSON` or `YAML`.
- `ImportResult` — `success`, `templates_loaded`, `error_message`, `checksum_valid`.
- `ExportResult` — `success`, `templates_written`, `error_message`.
- `PromptLibraryIO` — all-static:
  - `exportToJson()` / `exportToYaml()` — auto-compute checksum; pretty-printed JSON or yaml-cpp output.
  - `exportToFile(bundle, path, fmt)` — auto-detects YAML from `.yaml`/`.yml` extension.
  - `importFromJson()` / `importFromYaml()` — parse-safe; return `nullopt` on error.
  - `importFromFile(path, bundle)` → `ImportResult`; auto-detects format; validates checksum.
  - `computeChecksum()` / `verifyChecksum()` — FNV-1a 64-bit over sorted template JSON; 16-character lowercase hex.
- 30 unit tests (AC-1 through AC-30); CI: `prompt-library-io-ci.yml`.

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

## Scientific References

The planned enhancements are grounded in the following peer-reviewed literature and industry research:

### Structured Prompt Template DSL

[1] L.-H. Beurer-Kellner et al., "Prompting Is Programming: A Query Language for Large Language Models," in *Proc. PLDI 2023*, pp. 1946–1969, 2023. [DOI: 10.1145/3591300] Available: https://arxiv.org/abs/2212.06094

[2] A. Dohan et al., "Language Model Cascades," *arXiv preprint arXiv:2207.10342*, 2022. Available: https://arxiv.org/abs/2207.10342

[3] J. White et al., "A Prompt Pattern Catalog to Enhance Prompt Engineering with ChatGPT," *arXiv preprint arXiv:2302.11382*, 2023. Available: https://arxiv.org/abs/2302.11382

### Chain-of-Thought Step Tracer

[4] J. Wei et al., "Chain-of-Thought Prompting Elicits Reasoning in Large Language Models," in *Proc. NeurIPS*, vol. 35, pp. 24824–24837, 2022. Available: https://arxiv.org/abs/2201.11903

[5] X. Wang et al., "Self-Consistency Improves Chain of Thought Reasoning in Language Models," in *Proc. ICLR 2023*, 2023. Available: https://arxiv.org/abs/2203.11171

[6] T. Kojima et al., "Large Language Models are Zero-Shot Reasoners," in *Proc. NeurIPS*, vol. 35, pp. 22199–22213, 2022. Available: https://arxiv.org/abs/2205.11916

### RAG Context Window Budget Management

[7] P. Lewis et al., "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," in *Proc. NeurIPS*, vol. 33, pp. 9459–9474, 2020. Available: https://arxiv.org/abs/2005.11401

[8] Y. Gao et al., "Retrieval-Augmented Generation for Large Language Models: A Survey," *arXiv preprint arXiv:2312.10997*, 2023. Available: https://arxiv.org/abs/2312.10997

[9] Z. Shi et al., "REPLUG: Retrieval-Augmented Black-Box Language Models," *arXiv preprint arXiv:2301.12652*, 2023. Available: https://arxiv.org/abs/2301.12652

### Prompt A/B Experimentation Framework

[10] Y. Zhou et al., "Large Language Models Are Human-Level Prompt Engineers," in *Proc. ICLR 2023*, 2023. Available: https://arxiv.org/abs/2211.01910

[11] R. Pryzant et al., "Automatic Prompt Optimization with 'Gradient Descent' and Beam Search," in *Proc. EMNLP 2023*, pp. 7957–7968, 2023. Available: https://arxiv.org/abs/2305.03495

### Automated Quality Regression

[12] C.-Y. Lin, "ROUGE: A Package for Automatic Evaluation of Summaries," in *Proc. Workshop on Text Summarization Branches Out*, pp. 74–81, 2004. Available: https://aclanthology.org/W04-1013

[13] K. Papineni et al., "BLEU: A Method for Automatic Evaluation of Machine Translation," in *Proc. ACL 2002*, pp. 311–318, 2002. [DOI: 10.3115/1073083.1073135]

[14] L. Zheng et al., "Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena," in *Proc. NeurIPS*, vol. 36, 2023. Available: https://arxiv.org/abs/2306.05685

### Prompt Injection Security

[15] K. Greshake et al., "Not What You've Signed Up For: Compromising Real-World LLM-Integrated Applications with Indirect Prompt Injection," in *Proc. AISec@CCS 2023*, pp. 79–90, 2023. Available: https://arxiv.org/abs/2302.12173

[16] F. Perez and I. Ribeiro, "Ignore Previous Prompt: Attack Techniques For Language Models," *arXiv preprint arXiv:2211.09527*, 2022. Available: https://arxiv.org/abs/2211.09527

### Reflection Tuning & Self-Aware Dynamic Prompting

[17] A. Madaan et al., "Self-Refine: Iterative Refinement with Self-Feedback," in *Proc. NeurIPS*, vol. 36, 2023. Available: https://arxiv.org/abs/2303.17651

[18] N. Shinn et al., "Reflexion: Language Agents with Verbal Reinforcement Learning," in *Proc. NeurIPS*, vol. 36, 2023. Available: https://arxiv.org/abs/2303.11366

[19] Y. Bai et al., "Constitutional AI: Harmlessness from AI Feedback," *arXiv preprint arXiv:2212.08073*, 2022. Available: https://arxiv.org/abs/2212.08073

[20] M. Li et al., "Reflection-Tuning: Recycling Data for Better Instruction Tuning," *arXiv preprint arXiv:2310.11716*, 2023. Available: https://arxiv.org/abs/2310.11716

[21] S. Ji et al., "Survey of Hallucination in Natural Language Generation," *ACM Computing Surveys*, vol. 55, no. 12, pp. 1–38, 2023. [DOI: 10.1145/3571730] Available: https://arxiv.org/abs/2202.03629

[22] Golem.de, "Reflection Tuning bei KI: Selbstkritik bis hin zur Halluzination," *Golem.de*, March 2026. Available: https://www.golem.de/news/reflection-tuning-bei-ki-selbstkritik-bis-hin-zur-halluzination-2603-206734.html
