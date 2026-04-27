# Self-Refine: Iterative Refinement with Self-Feedback

**Metadaten:**
- Author(en): Aman Madaan, Niket Tandon, Prakhar Gupta, Skyler Hallinan, Luyu Gao, Sarah Wiegreffe, Uri Alon, Nouha Dziri, Shrimai Prabhumoye, Yiming Yang, Shashank Gupta, Bodhisattwa Prasad Majumder, Katherine Hermann, Sean Welleck, Amir Yazdanbakhsh, Peter Clark; and Noah Shinn, Federico Cassano, Ashwin Gopinath, Karthik Narasimhan, Shunyu Yao (Reflexion)
- Konferenz/Journal: NeurIPS 2023 (Self-Refine) · NeurIPS 2023 (Reflexion)
- Jahr: 2023
- Link: [Self-Refine arXiv:2303.17651](https://arxiv.org/abs/2303.17651) · [Reflexion arXiv:2303.11366](https://arxiv.org/abs/2303.11366)
- Zitierweise: `madaan2023selfrefine`, `shinn2023reflexion`
- Tags: `self-refine`, `reflexion`, `reflection`, `constitutional-ai`, `socratic`, `hallucination-guard`, `iterative-refinement`, `verbal-rl`, `llm`
- ThemisDB-Versionen: v1.5.0+ (implemented in `src/prompt_engineering/reflection_tuner.cpp`)
- Status: [x] Fully Implemented

## 📋 Executive Summary

Self-Refine (Madaan et al., NeurIPS 2023) and Reflexion (Shinn et al., NeurIPS 2023) establish the paradigm of **iterative LLM self-improvement**: the model generates a response, critiques it from its own perspective, and revises based on the critique — without external labels or weight updates. Both works demonstrate substantial quality gains on code generation, QA, dialogue, and mathematical reasoning. ThemisDB implements this paradigm as `ReflectionTuner` with four configurable strategies (SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC), a pluggable `IReflectionProvider` interface, a self-aware context extractor (`SelfAwareContext`), and a production-safety `ReflectionHallucinationGuard`.

## 🎯 Key Findings

- **Three-phase loop (Self-Refine)**: Generate → Feedback → Refine, iterated until a convergence threshold or max iterations is reached; the same model handles all three phases.
- **Quality gains**: +14–30% on dialogue generation, +7% on GSM8K maths, +15% on code-review tasks versus single-pass generation.
- **Convergence**: Typically 2–3 iterations for most tasks; diminishing returns beyond 4 iterations.
- **Reflexion — verbal reinforcement learning**: Instead of gradient updates, the agent stores past mistakes as natural-language reflections in an episodic memory context, enabling stronger drift correction than Self-Refine.
- **Reflexion on sequential tasks**: +22% on AlfWorld (decision-making), +17% on HumanEval (code); strongest gains where past errors directly inform the next episode.
- **Constitutional AI (Bai et al., 2022) — integrated as CONSTITUTIONAL strategy**: Principle-guided self-critique; each response is checked against an explicit list of principles without human feedback.

## �� Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/reflection_tuner.cpp` (`ReflectionTuner`, four strategies, `SelfAwareContext`, `ReflectionHallucinationGuard`)
- [x] Prompt Engineering → `src/prompt_engineering/llm_reflection_adapter.cpp` (`ILLMProviderReflectionAdapter`: bridges `ILLMProvider` → `IReflectionProvider`)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_integration.cpp` (`afterExecution()` optional reflection pass via `IntegrationConfig::enable_reflection_tuning`)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_metrics.cpp` (4 reflection counters + Prometheus export + snapshot/restore)

### What Was Adopted?

1. **Self-Refine generate→critique→revise cycle**: `ReflectionTuner::refine()` iterates `IReflectionProvider::{generate, critique, revise}` with configurable `max_iterations` and `convergence_threshold` — a direct mapping of the paper's algorithm.
2. **Reflexion episodic memory**: `ReflectionResult::iteration_trace` records the full critique–revision sequence; `SelfAwareContext` injects the trace into subsequent iteration prompts, mirroring Reflexion's episodic buffer.
3. **Constitutional principles**: `ReflectionConfig::constitutional_principles` holds a list of compliance rules; `DynamicReflectionPromptBuilder` formats each principle as a critique checkpoint, reproducing Constitutional AI's "critique → revision against principles" cycle.
4. **Convergence detection**: Loop breaks when `score >= convergence_threshold` or `|score_delta| < min_delta_improvement`, matching both papers' stopping criteria.
5. **Score-based best-response tracking**: `ReflectionResult::best_response` always holds the highest-scoring response seen across all iterations, ensuring regression safety if a later iteration degrades quality.

### How Was It Adapted?

| Paper Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Same model for all phases | `IReflectionProvider` with 4 methods (generate/critique/revise/score) | Flexibility: different models per phase if needed (e.g., fast model for critique) |
| GPT-4 as evaluator | `IReflectionScorer` interface; fallback to `PromptEvaluator::score()` | Provider-agnostic; no cloud lock-in |
| Reflexion episodic buffer | `ReflectionResult.iteration_trace` + `SelfAwareContext` injection | Explicit trace for ThemisDB observability integration |
| Constitutional principles (Anthropic) | Configurable `constitutional_principles` array in `ReflectionConfig` | Domain-specific compliance: privacy law, administrative regulation |
| No hallucination protection | `ReflectionHallucinationGuard` (ThemisDB addition) | Production safety: loop must not amplify fabricated content |
| No self-confidence signal | `SelfAwareContext` confidence/uncertainty marker extraction (ThemisDB addition) | Adaptive critique intensity based on model's own linguistic confidence signals |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Delta | Reason |
|--------|-------------|-----------------|-------|--------|
| Quality improvement (dialogue) | +14–30% | +5–15% on AQL prompt explanations | -0–15 pp | Narrower domain; absolute gain smaller |
| Typical convergence | 2–3 iterations | 3 iterations (max_iterations default) | 0 | Default tuned to paper observation |
| Refinement latency per iteration (no LLM) | n/a | < 0.5 ms P99 (prompt construction only) | n/a | In-process heuristic fallback |
| Full 3-iteration cycle (no LLM) | n/a | < 1 ms P99 | n/a | No I/O; pure string processing |
| `SelfAwareContext::fromResponse()` (512 tokens) | n/a | < 0.1 ms | n/a | Linear marker scan; O(n·m) where m = 18 markers |

## ⚠️ Limitations & Open Questions

- Self-Refine and Reflexion benefit substantially from strong LLMs (≥ 7B); weaker models produce uninformative critiques.
  - ThemisDB solution: Template-based fallback in `IReflectionProvider::critique()` that operates without an LLM.
- The reflection loop can amplify adversarial inputs if an attacker crafts a critique-steering injection.
  - ThemisDB solution: `PromptInjectionDetector` gate before every `generate()` call; `ReflectionHallucinationGuard` aborts on marker detection.
- Constitutional principles require domain curation; there are no safe universal defaults.
  - ThemisDB solution: `config/prompts/constitutional_principles.yaml` for centralized management with per-tenant override support.
- No empirical benchmark of ThemisDB's SOCRATIC strategy against paper baselines.
  - Open: Build a QA benchmark on ThemisDB domain question pairs to validate SOCRATIC vs. SELF_REFINE quality.

## 🔬 Validation

- [x] Code reviewed against paper algorithms (Self-Refine and Reflexion)
- [x] Unit tests written (38 tests, AC-01..AC-38 in `tests/test_reflection_tuner.cpp`)
- [x] Mock providers implemented: `ConstantMockProvider`, `ImprovingMockProvider`, `HallucinatingCritiqueProvider`, `DivertingMockProvider`
- [ ] Benchmark executed (ReflectionTuner quality lift on ThemisDB domain QA)
- [x] Documentation updated (`src/prompt_engineering/ROADMAP.md` Phase 3+4; `FUTURE_ENHANCEMENTS.md`)
- [ ] Module README linked with paper reference
- [x] implementation_influence index updated

## 📚 Related Work

- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — ProTeGi optimizes prompts; Self-Refine refines responses — complementary loops
- [Yao et al. (2023) — Tree of Thoughts](yao_tree_of_thoughts_2023.md) — ToT expands the thought space; Self-Refine refines individual reasoning paths
- [Bai et al. (2022) — Constitutional AI](https://arxiv.org/abs/2212.08073) — CONSTITUTIONAL strategy source
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/reflection_tuner.cpp`](../../../src/prompt_engineering/reflection_tuner.cpp)
- [`src/prompt_engineering/llm_reflection_adapter.cpp`](../../../src/prompt_engineering/llm_reflection_adapter.cpp)
- [`src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)

---
**Last Updated:** 2026-04-27
**Next Review:** 2026-10-31
