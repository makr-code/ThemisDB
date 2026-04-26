> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Prompt Engineering Module Roadmap
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/prompt_engineering/README.md · src/prompt_engineering/ARCHITECTURE.md · src/prompt_engineering/FUTURE_ENHANCEMENTS.md · docs/de/prompt_engineering/README.md -->

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.5.0 – Full lifecycle management for LLM prompt templates is production-ready. Version control, A/B testing, feedback collection, self-improvement orchestrator, Prometheus metrics, prompt injection attack detection, chain-of-thought prompt construction, RAG prompt assembly, system prompt management, context-window budget enforcement, and **Reflection Tuning with dynamic self-aware prompting** are all implemented.
v2.0.0 – Full lifecycle management for LLM prompt templates is production-ready. Version control, A/B testing, feedback collection, self-improvement orchestrator, Prometheus metrics, prompt injection attack detection, chain-of-thought prompt construction, RAG prompt assembly, system prompt management, context-window budget enforcement, Tree-of-Thoughts multi-path reasoning, ProTeGi textual-gradient optimizer, and DSPy-compatible prompt declaration layer are all implemented.

## Completed ✅
- [x] PromptManager – CRUD with RocksDB persistence and YAML bulk-load
- [x] Thread-safe reads/writes via TBB `concurrent_hash_map`
- [x] Context injection (`{placeholder}` variable substitution)
- [x] `buildContextFromSchema()` – populate variables from SchemaManager snapshot
- [x] **Template validation** – `validateTemplate()` with `ValidationResult` (errors + warnings); integrated into `createTemplate()` and `loadFromYAML()`
- [x] FeedbackCollector – 10 feedback types, aggregate stats, failure pattern analysis
- [x] **FeedbackCollector scalability** – `getFeedbackPaged()` chunked API, `detectOutliers()` Z-score anomaly detection, FNV-1a audit checksum on every entry
- [x] PromptEvaluator – semantic similarity, exact match, partial match, relevance scoring
- [x] **PromptEvaluator statistical significance** – proper Welch's two-sample t-test replacing naive 5% threshold
- [x] **PromptEvaluator embedding interface** – `IEmbeddingProvider` interface; `setEmbeddingProvider()` / cosine similarity / graceful Jaccard fallback
- [x] PromptOptimizer – iterative improvement with pluggable eval/improvement functions
- [x] MetaPromptGenerator – LLM-assisted prompt rewriting
- [x] **MetaPromptGenerator LLM integration** – `ILLMProvider` interface; `setLLMProvider()` / graceful fallback
- [x] Git-like version control (branches, commits, diffs, parent tracking)
- [x] A/B testing with statistical significance (p-value via standard normal CDF)
- [x] **A/B test statistics** – replaced hardcoded z-score table with `std::erfc`-based normal CDF
- [x] Self-improvement orchestrator with configurable trigger thresholds
- [x] **SelfImprovementOrchestrator eval_fn** – wired to real `PromptEvaluator`; heuristic fallback only when no evaluator available
- [x] Background worker thread for periodic auto-optimization
- [x] Prometheus-compatible metrics export
- [x] **Metrics persistence** – `snapshotToJson()` / `restoreFromJson()` for crash-safe recovery
- [x] **Threshold alerting** – `AlertConfig` / `AlertCallback` hooks firing on failure rate and hallucination count breaches
- [x] Integration facade combining all subsystems
- [x] **Prompt injection attack detection** – `PromptInjectionDetector` with 10 built-in patterns, keyword/syntax scoring, `detect()`, `detectInResponse()`, `sanitize()`, pluggable custom patterns (Issue: #2428, PR: #2534)
- [x] Multi-modal prompt support (image descriptions alongside text) (Target: Q3 2026) (Issue: #2429)
- [x] **Tree-of-Thoughts reasoning** – `TreeOfThoughtsBuilder` with BFS/DFS/BEAM search, pluggable `IToTThoughtGenerator` and `IToTEvaluator`, pruning, and answer synthesis (Target: Q1 2026)
- [x] **ProTeGi textual-gradient optimizer** – `ProTeGiOptimizer` implementing automatic prompt optimisation via natural-language gradients, mini-batch critique, and beam search (Target: Q1 2026)
- [x] **DSPy-compatible prompt declaration layer** – `DspySignature`, `DspyPredict`, `DspyChainOfThought`, `EchoDspyLLMProvider`, and `DspyMissingFieldError` (Target: Q1 2026)
- [x] **Chain-of-thought prompt construction** – `ChainOfThoughtBuilder` with step delimiters, auto-numbering, zero-shot/few-shot/wrap helpers
- [x] **RAG prompt builder** – `RAGPromptBuilder` with budget-aware chunk selection, source citations, template injection, and full-prompt assembly
- [x] **System prompt manager** – `SystemPromptManager` with built-in and custom role support, context-variable rendering, and JSON serialisation
- [x] **Context-window budget enforcement** – `ContextWindowBudgetManager` with pluggable `ITokenCounter`, `CharDivisionCounter` BPE approximation, greedy chunk selection, `PromptBudgetExceededError`, and utilisation callback
- [x] **Reflection Tuning with dynamic self-aware prompting** – `ReflectionTuner` with four strategies (SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC), `IReflectionProvider` interface, `DynamicReflectionPromptBuilder`, `SelfAwareContext`, `ReflectionHallucinationGuard`

## In Progress 🚧
- [x] Token counting and context-window budget enforcement (Target: Q2 2026)
- [x] Typed template DSL (`PromptTemplateCompiler`, `CompiledPromptTemplate`, `IPromptTemplate`) (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [?] Prompt diff visualization in web UI
- [?] Batch A/B test runner with configurable traffic splits
- [?] Import/export prompt library to JSON or YAML
- [?] Per-language prompt template variants (i18n)
- [?] Latency SLO tracking per prompt template

### Long-term (6-12 months)
- [?] Reinforcement learning from human feedback (RLHF) integration
- [?] Cross-model prompt portability scoring (GPT-4 vs. LLaMA compatibility)
- [?] Automated regression detection when base model is upgraded
- [?] Prompt chaining and DAG execution
- [?] Community prompt library with versioned sharing

## Implementation Phases

### Phase 1: Template Management & Evaluation (Status: Completed ✅)
- [x] PromptManager – CRUD with RocksDB persistence and YAML bulk-load
- [x] Context injection (`{placeholder}` variable substitution) and `buildContextFromSchema()`
- [x] Chain-of-thought (CoT) prompt support with step delimiters (`ChainOfThoughtBuilder`)
- [x] RAG prompt construction helpers (retrieved context injection) (`RAGPromptBuilder`)
- [x] System prompt management and per-role override (`SystemPromptManager`)
- [x] FeedbackCollector, PromptEvaluator, PromptOptimizer, MetaPromptGenerator
- [x] Git-like version control (branches, commits, diffs)
- [x] A/B testing with statistical significance (p-value)
- [x] Prometheus-compatible metrics export

### Phase 2: Typed DSL & Context Budget (Status: Completed ✅)
- [x] **Typed Template DSL** – `PromptTemplateCompiler`, `IPromptTemplate`, `CompiledPromptTemplate`, `PromptTemplateValidator`, `PromptContextValue` (Target: Q2 2026)
- [x] Context window budget manager – enforce token limits before dispatch (Target: Q2 2026)
- [x] Prompt injection attack detection layer (Target: Q2 2026)
- [x] Multi-modal prompt support (image descriptions alongside text) (Target: Q3 2026)

### Phase 3: Reflection Tuning & Dynamic Self-Aware Prompting (Status: Completed ✅)
- [x] `ReflectionTuner` — iterative generate→critique→revise cycle with four strategies
- [x] `IReflectionProvider` interface — pluggable LLM backend; fallback template/heuristic mode
- [x] `DynamicReflectionPromptBuilder` — strategy-specific, self-aware critique and revision prompts
- [x] `SelfAwareContext` — linguistic confidence/uncertainty extraction; adaptive prompt injection
- [x] `ReflectionHallucinationGuard` — marker scan + rolling-average divergence detection
- [x] 38 unit tests; CI: `reflection-tuner-ci.yml`

### Phase 4: Integration, Adapter & Observability (Status: Completed ✅)
- [x] `ILLMProviderReflectionAdapter` — bridges `ILLMProvider` → `IReflectionProvider`; adapter pattern, `IReflectionScorer` interface, heuristic fallback
- [x] Reflection metrics in `PromptEngineeringMetrics` — 4 new counters + Prometheus export + snapshot/restore
- [x] `PromptEngineeringIntegration` wired: `setReflectionTuner()`, `setMetrics()`, optional reflection pass in `afterExecution()`
- [x] `IntegrationConfig::enable_reflection_tuning` / `reflection_max_iterations`
- [x] 28 focused integration tests; CI: `reflection-integration-ci.yml`
- [x] German docs updated (README, missing-implementations.md)

### Phase 5: Tracing, Regression & Experiments (Status: In Progress 🚧)
- [x] CoT execution tracer — `IChainOfThoughtTracer` + `RecordingCoTTracer` + `CoTTraceCollector`; `ChainOfThoughtBuilder::attachTracer()`; per-step latency attribution; 30 tests; CI: `cot-tracer-ci.yml`
- [x] Prompt regression suite — `PromptRegressionRunner`; golden-set fixtures; `FeedbackCollector` integration; `delta_pct`/`is_regression`/`blocked`; structured log callback; 30 tests; CI: `prompt-regression-runner-ci.yml`
- [x] A/B experiment framework — `PromptABExperimentFramework`; deterministic MurmurHash3-32 variant assignment; Welch t-test significance; auto winner promotion; `WinnerCallback`; 30 tests; CI: `prompt-ab-experiment-ci.yml`
- [x] Import/export prompt library — `PromptLibraryIO`; JSON + YAML (via yaml-cpp); FNV-1a checksum; `PromptLibraryBundle`; file round-trip; 30 tests; CI: `prompt-library-io-ci.yml`
- [?] Per-language prompt template variants (i18n support)

### Phase 6: Advanced Reasoning & Optimization (Status: Completed ✅)
- [x] **Tree-of-Thoughts reasoning** – `TreeOfThoughtsBuilder` with BFS/DFS/BEAM search strategies, pluggable `IToTThoughtGenerator` and `IToTEvaluator`, depth-bounded pruning, and answer synthesis (30 tests, CI: tree-of-thoughts-ci.yml)
- [x] **ProTeGi textual-gradient optimizer** – `ProTeGiOptimizer` and `IProTeGiLLMProvider` implementing automatic prompt optimisation via natural-language gradients, mini-batch critique, and beam search (18 tests, CI: protegi-optimizer-ci.yml)
- [x] **DSPy-compatible prompt declaration layer** – `DspySignature`, `DspyPredict`, `DspyChainOfThought`, `EchoDspyLLMProvider`, `IDspyLLMProvider`, and `DspyMissingFieldError` (30 tests, CI: dspy-module-ci.yml)

## Production Readiness Checklist
- [x] Template validation with detailed error reporting
- [x] Feedback paging API for large archives
- [x] Audit trail (checksum) on feedback entries
- [x] Pluggable LLM interface for MetaPromptGenerator
- [x] Pluggable embedding interface for PromptEvaluator
- [x] Welch's t-test for statistical significance
- [x] Proper normal CDF for A/B test z-test p-values
- [x] Metrics snapshot/restore for crash recovery
- [x] Threshold-based alerting with pluggable callbacks
- [x] All prompt_engineering sources compiled in the build
- [x] Prompt injection attack detection layer (`PromptInjectionDetector`)
- [x] Chain-of-thought, RAG prompt builder, and system prompt manager implemented
- [x] Context-window budget enforcement (`ContextWindowBudgetManager`) with pluggable token counter and `PromptBudgetExceededError`
- [x] **Reflection Tuning** (`ReflectionTuner`, `IReflectionProvider`, `DynamicReflectionPromptBuilder`, `SelfAwareContext`, `ReflectionHallucinationGuard`) — dynamic self-aware LLM prompting with divergence guard
- [x] Tree-of-Thoughts reasoning (`TreeOfThoughtsBuilder`) with BFS/DFS/BEAM, pruning, and answer synthesis
- [x] ProTeGi textual-gradient optimizer (`ProTeGiOptimizer`) with mini-batch critique and beam search
- [x] DSPy-compatible prompt declaration layer (`DspySignature`, `DspyPredict`, `DspyChainOfThought`)
- [x] Unit tests coverage > 80%
- [x] Integration tests (version control round-trip, A/B statistical significance)
- [x] Performance benchmarks (optimization loop latency, concurrent access)
- [x] Security audit (prompt injection risk addressed via PromptInjectionDetector)
- [x] Documentation complete
- [x] API stability guaranteed

## Known Issues & Limitations
- Token counting and context-window management is out of scope; callers must manage limits.
- Full LLM-based evaluation in `optimizePrompt()` requires callers to execute the prompt and supply a custom `eval_fn`; the built-in fallback uses `PromptEvaluator` structural similarity as a proxy.

## Breaking Changes
- PromptTemplate schema is stable from v1.x; new optional fields only.
- `FeedbackType` enum may gain new values; exhaustive switches in callers should use a default case.
- `PromptManager::createTemplate()` now returns an empty-id sentinel on validation failure (id.empty() == true); callers should check the returned id before use.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `attackCategoryName` – Gibt lesbaren Namen einer AdversarialAttackCategory zurück
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.

