# DSPy: Compiling Declarative Language Model Calls into Self-Improving Pipelines

**Metadaten:**
- Author(en): Omar Khattab, Arnav Singhvi, Paridhi Maheshwari, Zhiyuan Zhang, Keshav Santhanam, Sri Vardhamanan A, Saiful Haq, Ashutosh Sharma, Thomas T. Joshi, Hanna Moazam, Heather Miller, Matei Zaharia, Christopher Potts
- Konferenz/Journal: ICLR 2024 (Oral)
- Jahr: 2023/2024
- Link: [arXiv:2310.03714](https://arxiv.org/abs/2310.03714) · [GitHub: stanfordnlp/dspy](https://github.com/stanfordnlp/dspy)
- Zitierweise: `khattab2023dspy`
- Tags: `dspy`, `declarative-llm`, `prompt-optimization`, `pipeline-compilation`, `typed-signature`, `predict`, `chain-of-thought`, `module-composition`, `llm`
- ThemisDB-Versionen: v2.0.0+ (declaration layer in `src/prompt_engineering/dspy_module.cpp`; compiler planned v2.2.0)
- Status: [x] Partially Implemented (declaration layer ✅; compiler `DspyOptimize` ⏳ planned v2.2.0)

## 📋 Executive Summary

DSPy (Declarative Self-Improving Language Programs) introduces a **programming paradigm for LLM pipelines**: instead of hand-writing prompts, developers declare typed input/output signatures (`DspySignature`) and compose predefined modules (`Predict`, `ChainOfThought`, `ReAct`, `Retrieve`). A **compiler** (`dspy.compile`) translates these declarations into optimized prompts using training examples and a metric function — cleanly separating *what* the model should do (program logic) from *how* it should do it (prompt details). ThemisDB implements the DSPy-compatible declaration layer (`DspySignature`, `DspyPredict`, `DspyChainOfThought`) in C++; the compiler back-end (`DspyOptimize`) is planned for v2.2.0.

## 🎯 Key Findings

- **Signatures instead of prompts**: `question: str -> answer: str` is a complete DSPy signature; the system derives an effective prompt automatically.
- **Composable modules**: `Predict(sig)`, `ChainOfThought(sig)`, `Retrieve(k=3)`, `ReAct(tools)` compose like software components; pipelines are Python/C++ programs, not prompt strings.
- **Compilation = prompt optimization**: `dspy.compile(program, trainset, metric)` runs automatic prompt optimization via Bootstrap Few-Shot or ProTeGi-style textual gradients.
- **Teleprompter (teacher–student)**: A strong model generates demonstrations for a weaker model; the compiler distills this knowledge into few-shot prompts for deployment.
- **ICLR 2024 Oral**: Top-scored paper; >18,000 GitHub stars (April 2024); adopted by Stanford, Databricks, JetBlue.
- **+11% F1 on multi-hop QA** vs. manually tuned prompts; gains scale with trainset size and metric quality.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/dspy_module.cpp` (`DspySignature`, `DspyPredict`, `DspyChainOfThought`, `IDspyLLMProvider`, `EchoDspyLLMProvider`, `DspyMissingFieldError`)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_template_compiler.cpp` (DSL compiler handles `DspySignature`-style typed slots)
- [x] Prompt Engineering → `src/prompt_engineering/chain_of_thought.cpp` (`DspyChainOfThought` is an adapter over `ChainOfThoughtBuilder`)
- [ ] Prompt Engineering → `src/prompt_engineering/dspy_optimizer.cpp` (`DspyOptimize` compiler — planned v2.2.0)

### What Was Adopted?

1. **Typed I/O signatures**: `DspySignature::addInputField(DspyField{name, description, type, required, default})` and `addOutputField()` mirror DSPy's `dspy.Signature` with field-level semantics (`STRING`, `REASONING`, `CODE`, `LIST`).
2. **Predict module**: `DspyPredict::forward(DspyContext)` renders the signature to a prompt, calls `IDspyLLMProvider::generate()`, parses output fields, and returns a `DspyPrediction` — the C++ equivalent of `dspy.Predict(sig)(input)`.
3. **ChainOfThought module**: `DspyChainOfThought` extends `DspyPredict` by auto-appending a `REASONING`-type output field and injecting a "Let's think step by step" preamble — matching `dspy.ChainOfThought(sig)`.
4. **Missing-field error handling**: `DspyMissingFieldError` is raised when a required output field is absent from the LLM response, enabling structured error recovery in pipeline callers.
5. **Prompt template generation**: `DspySignature::toPromptTemplate()` generates a base prompt from field names and descriptions, analogous to DSPy's internal `Signature.instructions` generation.

### How Was It Adapted?

| DSPy Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Python `dspy.Signature` | C++ `DspySignature` with fluent builder API | Native C++ integration; no Python binding required |
| `dspy.Predict(sig)` | `DspyPredict` with `IDspyLLMProvider` interface | Provider abstraction consistent with the rest of `src/prompt_engineering/` |
| `dspy.ChainOfThought(sig)` | `DspyChainOfThought` subclasses `DspyPredict` | Reuses `ChainOfThoughtBuilder` infrastructure; single source of truth for CoT logic |
| Global `dspy.settings.lm` | `DspyPredict::setLLMProvider()` per instance | Multi-tenant: different providers per template possible |
| `dspy.compile()` | `DspyOptimize` — planned v2.2.0 | Phase 1 delivers the declaration layer; compiler needs `FeedbackCollector` training data |
| Python `dict`-based I/O | `DspyContext` (map<string,string>) + typed `DspyField` | Type safety; `DspyMissingFieldError` replaces silent `KeyError` |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Delta | Reason |
|--------|-------------|-----------------|-------|--------|
| Quality after compilation (multi-hop QA) | +11% F1 vs. manual prompts | +5–10% on AQL translation (after compiler, v2.2.0) | -1–6 pp | AQL is a narrower task; absolute F1 gain smaller |
| `DspyPredict::forward()` latency (no LLM) | n/a | < 0.5 ms P99 | n/a | In-process string rendering and parsing |
| Unit-test coverage (declaration layer) | n/a | 30 tests (AC-01..AC-30) | n/a | Full declaration layer covered |
| Compiler (`DspyOptimize`) quality gain | +11–40% F1 | geplant Q3 2026 | n/a | Requires training data from `FeedbackCollector` |

## ⚠️ Limitations & Open Questions

- The DSPy compiler requires training examples and a metric function — not always available in production deployments.
  - ThemisDB solution: `FeedbackCollector` + `PromptRegressionRunner` organically accumulate training data for eventual compiler use.
- Output field parsing from free-form LLM text is fragile.
  - ThemisDB solution: `DspyMissingFieldError` + structured JSON output forcing via `src/prompt_engineering/structured_output.cpp`.
- DSPy signatures cannot express conditional control flow (e.g., `if confidence < 0.7: retrieve_more`).
  - ThemisDB solution: Hybrid approach — `DspySignature` for simple I/O modules; `PromptTemplateCompiler` DSL for complex conditional logic.
- The v2.2.0 compiler is not yet implemented; ProTeGi-teleprompter integration is pending.
  - Open: Define `BootstrapFewShot` and `ProTeGiTeleprompter` teleprompters using existing `FeedbackCollector` and `ProTeGiOptimizer` infrastructure.

## 🔬 Validation

- [x] Code reviewed against DSPy paper design (`DspySignature`, `DspyPredict`, `DspyChainOfThought`)
- [x] Unit tests written (30 tests, AC-01..AC-30 in `tests/test_dspy_module.cpp`)
- [ ] Benchmark executed (DSPy-compiled vs. manual AQL prompts, F1 comparison)
- [x] Documentation updated (`src/prompt_engineering/ROADMAP.md` Phase 6)
- [ ] Module README linked with paper reference
- [x] implementation_influence index updated

## 📚 Related Work

- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — DSPy compiler uses ProTeGi-style textual gradients as a teleprompter
- [Tree of Thoughts — Yao et al. (2023)](yao_tree_of_thoughts_2023.md) — ToT integrable as a DSPy reasoning module
- [Zhou et al. (2022) — APE](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape) — APE: first step toward automatic prompt generation; DSPy extends this to full pipelines
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/dspy_module.cpp`](../../../src/prompt_engineering/dspy_module.cpp)
- [`src/prompt_engineering/prompt_template_compiler.cpp`](../../../src/prompt_engineering/prompt_template_compiler.cpp)

---
**Last Updated:** 2026-04-27
**Next Review:** 2026-10-31
