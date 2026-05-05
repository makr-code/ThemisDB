# LLM Prompt Enhancement Pipeline: ProTeGi + Self-Refine + ToT + DSPy

**Metadaten:**
- Source: Pryzant et al. (2023) — ProTeGi (EMNLP); Madaan et al. (2023) — Self-Refine (NeurIPS); Shinn et al. (2023) — Reflexion (NeurIPS); Yao et al. (2023) — Tree of Thoughts (NeurIPS); Khattab et al. (2023) — DSPy (ICLR 2024); ThemisDB Engineering
- URL: [ProTeGi arXiv:2305.03495](https://arxiv.org/abs/2305.03495) · [Self-Refine arXiv:2303.17651](https://arxiv.org/abs/2303.17651) · [ToT arXiv:2305.10601](https://arxiv.org/abs/2305.10601) · [DSPy arXiv:2310.03714](https://arxiv.org/abs/2310.03714)
- Tags: `prompt-engineering`, `protegi`, `self-refine`, `reflexion`, `tree-of-thoughts`, `dspy`, `prompt-optimization`, `llm`, `self-improvement`, `reflection`
- ThemisDB-Versionen: v1.5.0+ (Self-Refine/Reflexion); v2.0.0+ (ProTeGi, ToT, DSPy)
- Status: [x] Fully Adopted (declaration layer + optimization loops); [ ] Partially Adopted (DSPy compiler — planned v2.2.0)

## 📋 Summary

ThemisDB's prompt engineering module implements a four-layer LLM Prompt Enhancement Pipeline that progressively improves LLM prompt quality from initial template to production-optimized, self-correcting outputs. Each layer addresses a different aspect of quality: (1) **ProTeGi** optimizes the *prompt* itself via textual gradients and beam search; (2) **Self-Refine / Reflexion** improves individual *LLM responses* through iterative critique-and-revision; (3) **Tree of Thoughts** expands the *reasoning space* via multi-path tree search; (4) **DSPy** provides a *declarative interface* that separates program logic from prompt implementation details. All four layers are composable, opt-in, and share the same `ILLMProvider` abstraction — meaning a single registered LLM backend serves all enhancement mechanisms.

## 🎯 Core Principles

1. **Layer independence**: Each enhancement layer can be activated independently. The standard inference path uses CoT only; ProTeGi, ToT, and Reflection are opt-in.
2. **Shared LLM provider abstraction**: `ILLMProvider` (for ProTeGi/MetaPromptGenerator), `IReflectionProvider` (bridged via `ILLMProviderReflectionAdapter`), `IToTThoughtGenerator`/`IToTEvaluator`, and `IDspyLLMProvider` all accept the same registered backend.
3. **Heuristic fallback everywhere**: Every layer ships an in-process heuristic fallback (`HeuristicProTeGiProvider`, template-based `IReflectionProvider`, `HeuristicThoughtGenerator`) so the system operates without an LLM in edge/offline deployments.
4. **Production safety gates**: `PromptInjectionDetector` runs before every generated prompt candidate is committed. `ReflectionHallucinationGuard` terminates the reflection loop on hallucination marker detection or rolling quality divergence.
5. **Versioned output**: Every optimization result creates a new `PromptVersionControl` commit; rollback to any prior version is possible via `PromptVersionControl::rollback()`.
6. **Observable pipeline**: `PromptEngineeringMetrics` (Prometheus-compatible) tracks ProTeGi iteration counts, reflection cycles, ToT search depth, and DSPy forward() latency; all metrics support snapshot/restore for crash-safe recovery.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/prompt_engineering/protegi_optimizer.cpp` — `ProTeGiOptimizer`: textual-gradient beam search over prompt space
- `src/prompt_engineering/reflection_tuner.cpp` — `ReflectionTuner`: response refinement (SELF_REFINE / REFLEXION / CONSTITUTIONAL / SOCRATIC)
- `src/prompt_engineering/tree_of_thoughts.cpp` — `TreeOfThoughtsBuilder`: BFS / DFS / BEAM multi-path reasoning
- `src/prompt_engineering/dspy_module.cpp` — `DspySignature`, `DspyPredict`, `DspyChainOfThought`: declarative I/O typing
- `src/prompt_engineering/self_improvement_orchestrator.cpp` — `SelfImprovementOrchestrator`: orchestrates ProTeGi cycle + A/B testing + feedback loop
- `src/prompt_engineering/prompt_engineering_integration.cpp` — `PromptEngineeringIntegration`: optional Reflection pass in `afterExecution()`
- `src/prompt_engineering/prompt_engineering_metrics.cpp` — Prometheus counters for all enhancement layers
- `src/prompt_engineering/llm_reflection_adapter.cpp` — `ILLMProviderReflectionAdapter`: bridges `ILLMProvider` → `IReflectionProvider`

### What Was Adopted?

**Pipeline architecture — layer overview**

```
┌─────────────────────────────────────────────────────────────────────┐
│              LLM Prompt Enhancement Pipeline (ThemisDB)             │
├─────────────────────────────────────────────────────────────────────┤
│ Layer 0: Template Declaration (DSPy)                                │
│   DspySignature → DspyPredict / DspyChainOfThought                 │
│   Typed I/O; prompt auto-generated from field descriptions          │
├─────────────────────────────────────────────────────────────────────┤
│ Layer 1: Prompt Optimization (ProTeGi)                              │
│   FeedbackCollector errors → mini-batch → textual gradient          │
│   → beam of improved prompt candidates → PromptVersionControl       │
├─────────────────────────────────────────────────────────────────────┤
│ Layer 2: Reasoning Expansion (Tree of Thoughts)                     │
│   TreeOfThoughtsBuilder (BFS/DFS/BEAM) → multi-path exploration     │
│   → IToTEvaluator pruning → best-leaf / majority-vote synthesis     │
├─────────────────────────────────────────────────────────────────────┤
│ Layer 3: Response Refinement (Self-Refine / Reflexion)              │
│   ReflectionTuner → generate → critique → revise (max 3 iter)      │
│   → SelfAwareContext → ReflectionHallucinationGuard → best response │
├─────────────────────────────────────────────────────────────────────┤
│ Cross-cutting: PromptInjectionDetector (all layers)                 │
│                PromptEngineeringMetrics (Prometheus)                │
│                PromptVersionControl (rollback safety)               │
└─────────────────────────────────────────────────────────────────────┘
```

**Layer 0 — DSPy declaration**

```cpp
auto sig = DspySignature("AQLTranslator", "Translates NL to AQL")
    .addInputField({"query",  "Natural language query", DspyFieldType::STRING, true, ""})
    .addInputField({"schema", "DB schema summary",      DspyFieldType::STRING, false, ""})
    .addOutputField({"aql",   "AQL query string",        DspyFieldType::STRING, true, ""});

DspyChainOfThought module(sig);   // auto-appends REASONING output field
module.setLLMProvider(llm_provider);
auto prediction = module.forward({{"query", "find users older than 30"}});
// prediction["aql"]       → generated AQL string
// prediction["reasoning"] → step-by-step explanation
```

**Layer 1 — ProTeGi prompt optimization (periodic, SelfImprovementOrchestrator)**

```cpp
ProTeGiConfig cfg { .beam_width = 4, .mini_batch_size = 8, .max_iterations = 6 };
ProTeGiOptimizer optimizer(cfg);
optimizer.setLLMProvider(llm_provider);  // or use HeuristicProTeGiProvider fallback

// Triggered by SelfImprovementOrchestrator when failure_rate > threshold
auto result = optimizer.optimize(
    current_prompt,
    feedback_collector.getByType(FeedbackType::FAILURE),
    eval_fn);
// result.best_prompt → saved to PromptVersionControl
```

**Layer 2 — Tree of Thoughts (opt-in for complex multi-step tasks)**

```cpp
ToTConfig tot_cfg { .strategy = ToTSearchStrategy::BEAM,
                    .max_depth = 4, .branching_factor = 3, .beam_width = 4 };
TreeOfThoughtsBuilder tot(tot_cfg);
tot.setThoughtGenerator(llm_based_generator);
tot.setEvaluator(prompt_evaluator_adapter);

auto result = tot.search(complex_query_problem);
// result.best_answer → answer from highest-scored leaf node
```

**Layer 3 — ReflectionTuner response refinement (afterExecution hook)**

```cpp
// In PromptEngineeringIntegration::afterExecution()
if (config_.enable_reflection_tuning && reflection_tuner_) {
    ReflectionConfig r_cfg {
        .strategy              = ReflectionStrategy::SELF_REFINE,
        .max_iterations        = 3,
        .convergence_threshold = 0.85,
        .divergence_threshold  = 0.15,
        .include_self_aware_context = true
    };
    auto result = reflection_tuner_->refine(response, task_context, r_cfg);
    // result.best_response → returned to caller if improved
    metrics_->recordReflectionCycle(result.iterations, result.final_score);
}
```

**Cross-cutting: injection gate + metrics**

```cpp
// PromptInjectionDetector runs before every generated candidate (all layers)
if (injection_detector_.detect(candidate_prompt).risk_level >= RiskLevel::MEDIUM) {
    THEMIS_WARN("PromptEnhancementPipeline: injection risk detected; candidate discarded");
    continue;
}

// Prometheus counters (PromptEngineeringMetrics)
metrics_.protegi_iterations_total   // counter
metrics_.reflection_cycles_total    // counter
metrics_.tot_search_depth_histogram // histogram
metrics_.dspy_forward_latency_ms    // histogram
```

### Deviations & Rationale

| Best-Practice Standard | ThemisDB Adaptation | Rationale |
|---|---|---|
| Single optimization technique | Four composable layers | Different tasks benefit from different techniques; composability avoids lock-in |
| Cloud-LLM dependency | `ILLMProvider` abstraction + heuristic fallback in every layer | Edge/offline deployments; llama.cpp local models as production backend |
| Global optimization pipeline | Per-template ProTeGi instances; opt-in Reflection | Multi-tenant: different templates need different optimization cadences |
| Open-loop optimization | Feedback loop via `FeedbackCollector` + `PromptVersionControl` rollback | Production safety: every optimization round creates a new version; rollback possible |
| No safety gate | `PromptInjectionDetector` before every generated candidate | Generated prompts can contain inadvertent injection patterns |
| No hallucination protection | `ReflectionHallucinationGuard` in Reflection layer | Iterative loops can amplify fabricated content without a guard |

## ⚠️ Trade-offs & Limitations

- Activating all four layers multiplies LLM inference cost by `O(beam_width × max_iterations × branching_factor × max_depth)` in the worst case.
  - Mitigation: All layers are opt-in; the standard inference path uses only CoT. ProTeGi runs periodically (triggered by `SelfImprovementOrchestrator`); ToT and Reflection are per-request opt-in.
- ProTeGi requires production failure examples from `FeedbackCollector`; a cold-start deployment has no data to optimize from.
  - Mitigation: `HeuristicProTeGiProvider` provides rule-based candidate generation until sufficient production data accumulates.
- Reflection loops can diverge under adversarial inputs if the critique itself is manipulated.
  - Mitigation: `ReflectionHallucinationGuard` rolling-average divergence detection + `PromptInjectionDetector` gate.
- DSPy compiler (`DspyOptimize`) is not yet implemented; the declaration layer is ready but automatic prompt compilation requires training data.
  - Mitigation: Manual prompts can be used with `DspyPredict` today; compiler planned for v2.2.0 using `FeedbackCollector` training data.

## 🔬 Validation

- [x] ProTeGi: 18 unit tests (PG-01..PG-18); CI: `protegi-optimizer-ci.yml`
- [x] Self-Refine / Reflexion: 38 unit tests (AC-01..AC-38); CI: `reflection-tuner-ci.yml`
- [x] Tree of Thoughts: 30 unit tests (AC-01..AC-30); CI: `tree-of-thoughts-ci.yml`
- [x] DSPy declaration layer: 30 unit tests (AC-01..AC-30); CI: `dspy-module-ci.yml`
- [ ] End-to-end benchmark: full pipeline (all 4 layers) vs. baseline CoT on AQL-translation task
- [x] Documentation updated (`src/prompt_engineering/ROADMAP.md` Phase 5+6; `FUTURE_ENHANCEMENTS.md`)
- [ ] Module README linked
- [x] implementation_influence index updated

## 📚 Related

- [Paper: ProTeGi — Pryzant et al. (2023)](../papers/pryzant_protegi_prompt_optimization_2023.md)
- [Paper: Self-Refine / Reflexion — Madaan & Shinn (2023)](../papers/madaan_self_refine_2023.md)
- [Paper: Tree of Thoughts — Yao et al. (2023)](../papers/yao_tree_of_thoughts_2023.md)
- [Paper: DSPy — Khattab et al. (2023)](../papers/khattab_dspy_2023.md)
- [LLM Integration Scientific Foundations](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)
- [`src/prompt_engineering/ROADMAP.md`](../../../src/prompt_engineering/ROADMAP.md)
- [`src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)

---
**Last Updated:** 2026-04-27
