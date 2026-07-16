# ProcessGPT: Transforming Business Process Management with Generative AI

**Metadaten:**
- Author(en): Kristian Busch, Maximilian Röglinger, Flavia Santoro, Jan vom Brocke
- Konferenz/Journal: IEEE International Conference on Big Data (IEEE Big Data 2023)
- Jahr: 2023
- Link: [IEEE Xplore](https://ieeexplore.ieee.org/document/10386497) · [DOI: 10.1109/BigData59044.2023.10386497]
- Zitierweise: `busch2023processgpt`
- Tags: `process-mining`, `bpmn`, `llm`, `generative-ai`, `nlp-to-bpmn`, `process-generation`, `bpm`
- ThemisDB-Versionen: v1.9.0+; planned in `src/process/` (P1: LLM-to-BPMN Generator, Q2 2026)
- Status: [ ] Not Started · planned Q2 2026

## 📋 Executive Summary

ProcessGPT investigates how large language models can transform business process management by generating BPMN process models from natural language descriptions, supporting process analysis via chat-based interfaces, and recommending process improvements. The paper introduces a framework that bridges the gap between informal process descriptions (as written by domain experts in German administrative contexts) and formal BPMN 2.0 XML models. This directly maps to ThemisDB's planned LLM-to-BPMN generator feature (`src/process/FUTURE_ENHANCEMENTS.md`, P1, Target Q2 2026).

## 🎯 Key Findings

- **NL-to-BPMN generation**: LLMs can generate syntactically valid BPMN 2.0 from natural language descriptions with ~65% structural correctness; prompt engineering significantly improves accuracy.
- **BPMN repair loop**: Iterative refinement — generate BPMN → validate → feed errors back to LLM → regenerate — achieves >85% valid BPMN after 3 iterations.
- **German administrative process descriptions**: The paper includes examples from German public administration, directly applicable to ThemisDB's `Verwaltungsvorgang` use case.
- **Structured prompting beats zero-shot**: Few-shot examples with BPMN syntax snippets raise BPMN structural validity from 45% to 72% in the study.
- **Hybrid approach recommended**: LLM for initial BPMN draft + rule-based BPMN validator + optional human review; fully automated pipeline achieves ~75% acceptance rate.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (planned `process_model_manager.cpp` LLM-to-BPMN)
- [x] LLM module → `src/llm/` (inference call for BPMN generation)
- [x] Prompt engineering → `src/prompt_engineering/` (BPMN generation prompt templates)
- [ ] Importers → `src/importers/` (BPMN XML import after LLM generation)

### What Was Adopted?

1. **Iterative BPMN repair loop**: ThemisDB's planned `BpmnGenerator` calls the LLM, validates the resulting BPMN XML against the ISO/IEC 19510 schema, and re-prompts with validation errors for up to 3 iterations.
2. **Structured output via grammar**: Use ThemisDB's existing grammar-constrained generation (`src/llm/grammar.cpp`) to enforce BPMN XML structure at the token level — stronger than post-hoc validation.
3. **Few-shot BPMN templates**: `PromptTemplate::BPMN_GENERATION` provides 3 few-shot examples (Antragstellung, Fachprüfung, Bescheiderteilung) as context for the LLM.
4. **German administrative vocabulary**: Prompt templates include German administrative terminology (Verwaltungsakt, Bescheid, Antragsteller) to improve LLM domain accuracy.

### How Was It Adapted?

| ProcessGPT Concept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Generic BPMN generation | Scoped to German Verwaltungsvorgänge | ThemisDB's primary use case is German public administration |
| OpenAI API (GPT-4) | llama.cpp local inference (Mistral/LLaMA 3) | Data privacy: citizen process data must not leave the premise |
| Text only input | Text + FIM process blueprint templates | FIM templates provide structural priors, reducing LLM hallucination |
| Single-shot validation | 3-iteration repair with BPMN schema validation | Higher quality bar; ISO/IEC 19510 compliance required |

### Performance Impact

| Metric | ProcessGPT Claim | ThemisDB Target | Status |
|--------|-----------------|-----------------|--------|
| BPMN structural validity (1-shot) | ~65% | >60% (local model) | ⏳ Planned Q2 2026 |
| BPMN validity after 3-iteration repair | >85% | >75% (local model) | ⏳ Planned |
| Latency per generation (incl. repair) | Not reported | <30 s (local 7B model) | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- LLMs hallucinate BPMN element IDs; structural validity ≠ semantic correctness.
  - ThemisDB solution: Post-validation via `ProcessModelManager::validateSemantics()` checks node connectivity and gateway completeness.
- BPMN complexity ceiling: LLMs struggle with >30-node processes.
  - ThemisDB solution: Generate sub-processes independently; link via BPMN Call Activity.
- Local models (7B–13B) underperform GPT-4 on BPMN generation.
  - ThemisDB solution: Fine-tune a legal-domain LoRA adapter on BPMN generation examples; use ThemisDB's `src/training/` pipeline.

## 🔬 Validation

- [ ] Code reviewed against paper
- [ ] Unit tests written (BPMN validity rate)
- [ ] Benchmark executed (vs. manual BPMN authoring)
- [ ] Documentation updated
- [ ] Module README linked (`src/process/README.md`)
- [ ] implementation_influence index updated

## 📚 Related Work

- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md)
- [OMG BPMN 2.0.2 Standard](https://www.omg.org/spec/BPMN/2.0.2/)
- [FIM — FITKO (2024)](verwaltungs_it_ozg_sources.md)
- [`src/process/FUTURE_ENHANCEMENTS.md`](../../../src/process/FUTURE_ENHANCEMENTS.md) (P1)
- [`docs/de/process/STATE_OF_THE_ART.md`](../../de/process/STATE_OF_THE_ART.md)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
