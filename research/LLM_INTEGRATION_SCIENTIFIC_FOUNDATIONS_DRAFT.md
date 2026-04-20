# Scientific Foundations and Systems Integration for LLM-Native Database Workloads

**Status**: Draft  
**Version**: 0.2 (migrated from docs/research)  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.LG)

---

## Abstract

This paper consolidates the scientific and systems foundations for integrating large language models into a hybrid database stack. Instead of presenting model-level novelty, it studies the operational coupling of prompting, retrieval, adapter tuning, and local inference in a production-oriented architecture. The ThemisDB implementation combines prompt engineering pipelines, LoRA-based adaptation, llama.cpp deployment, and benchmark-driven optimization. The paper contributes a repository-grounded evidence map, a reproducibility protocol, and claim boundaries that separate implemented platform capabilities from still-open research hypotheses.

## I. Introduction

Database-native LLM integration requires more than plugging an API into query execution. Practical systems must handle prompt lifecycle, model portability, retrieval grounding, adapter management, and safety constraints under real operational limits.

### Contributions

1. A unified architecture view of ThemisDB's LLM integration layers (prompting, LoRA, inference, evaluation).
2. A repository-backed evidence registry for reproducible claims.
3. A claim-boundary framework that prevents overstating current capabilities.

## II. Related Work

- Foundation models and prompting (few-shot, chain-of-thought, self-refinement).
- Retrieval-augmented generation pipelines.
- Parameter-efficient tuning (LoRA/QLoRA) and local inference systems.
- Gap: most prior work isolates one layer; this work emphasizes full-stack integration constraints.

## III. System Model / Architecture

- Prompt layer: templates, validation, optimization, regression checks.
- Retrieval layer: context selection and budget control for grounded inference.
- Adaptation layer: LoRA training and runtime selection.
- Inference layer: llama.cpp runtime, speculative decoding, cache-aware execution.
- Observability layer: benchmarking and quality/performance tracking.

## IV. Method / Design

- Define integration contracts across modules (input/output, quality gates, rollback).
- Use evidence-first engineering: each architectural claim maps to code/docs/tests.
- Evaluate via reproducible workflows and explicit limitations.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `src/prompt_engineering/prompt_optimizer.cpp` | optimizer logic | Prompt optimization pipeline exists | Ready |
| E2 | `src/prompt_engineering/prompt_quality_evaluator.cpp` | quality scoring | Automated quality evaluation path exists | Ready |
| E3 | `src/prompt_engineering/rag_prompt_builder.cpp` | context assembly | Prompt grounding via retrieval context is implemented | Ready |
| E4 | `config/prompts/scientific_prompts.yaml` | prompt config | Domain-specific prompt templates are maintained | Ready |
| E5 | `docs/en/llm/LORA_TRAINING_GUIDE.md` | training process | LoRA/QLoRA workflow is documented for deployment | Ready |
| E6 | `docs/en/llm/LLAMA_CPP_MIGRATION.md` | runtime migration | On-device llama.cpp integration path exists | Ready |
| E7 | `docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md` | decoding optimization | Speculative decoding integration plan/implementation exists | Ready |
| E8 | `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md` | attention optimization | Flash attention path is specified for performance | Ready |

## VI. Experimental Methodology

### A. Setup
- Fixed model set (same GGUF variants across runs)
- Fixed prompt suites (scientific, legal, technical)
- Repeated runs with warm-up and median reporting

### B. Workloads
- W1: grounded Q&A with retrieval context
- W2: long-context summarization under context budget
- W3: domain-adapter routing and response quality consistency

### C. Metrics
- Latency (TTFT, p95, p99)
- Throughput (requests/s, tokens/s)
- Quality (faithfulness/relevance via rubric-based evaluation)
- Stability (error rate, fallback activation)

## VII. Results (Planned Consolidation)

Initial subsystem evidence is strong, but end-to-end comparative runs across all workload classes are still being consolidated for camera-ready reporting.

## VIII. Discussion

The practical value lies in integration reliability and auditability, not a single novel model component.

### Claim Boundaries

**Supported claims:**
- Prompt engineering, LoRA training guidance, and local inference integration are implemented and documented.

**Deferred claims:**
- Universal quality gains across all domains.
- Cross-hardware performance generalization without additional benchmark series.

## IX. Reproducibility & Artifact

- Artifact family: ThemisDB repository (develop branch)
- Re-run sources: prompt_engineering module + llm docs/bench guides
- Reproducibility gaps: consolidated benchmark script and unified result tables still being harmonized

## X. Limitations, Risk, Ethics

- Prompt-injection and jailbreak risks remain critical.
- Domain-sensitive deployments require policy and compliance controls.
- Quality metrics can be dataset-sensitive and should not be overgeneralized.

## XI. Conclusion

ThemisDB already contains substantial LLM integration infrastructure. The next publication-quality step is rigorous, unified benchmarking across quality, latency, and reliability dimensions under fixed evaluation protocol.

## References

- See foundational bibliography in migrated source: `docs/research/LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md`.
