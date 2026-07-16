> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/rag/FUTURE_ENHANCEMENTS.md -->

# RAG Module — Public Header Future Enhancements

**Module Path:** `include/rag/`
**Canonical implementation enhancements:** [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/rag/`. Runtime retriever orchestration, evaluation scheduling, continuous-learning loop, and LLM judge routing work remain tracked in:

→ [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Retriever headers must define stable strategy contracts; index and vector-store internals remain opaque.
- `[x]` `IVectorizer` must remain the public extension point for custom embedding backends.
- `[x]` Evaluation headers must model deterministic scoring contracts; LLM judge calls require explicit timeout handling.
- `[x]` Safety and injection-detection headers must fail closed on adversarial or unsupported inputs.
- `[x]` Continuous-learning headers must expose signal-provider registration without leaking training loop internals.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `HybridRetriever` retrieve API | `hybrid_retriever.h` | RAG generation pipeline | ✅ Stable |
| `FaithfulnessEvaluator::score()` | `faithfulness_evaluator.h` | Quality-control pipeline | ✅ Stable |
| `QualityControlPipeline` run API | `quality_control_pipeline.h` | Evaluation orchestration | ✅ Stable |
| `ContinuousLearningOrchestrator` loop | `continuous_learning_orchestrator.h` | ML telemetry and monitoring | ✅ Stable |
| `PromptInjectionDetector::detect()` | `prompt_injection_detector.h` | Security middleware | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Align `IVectorizer` and `vectorizer_interface.h` with `include/llm/` embedding model contracts for consistent backend selection.
- Document async timeout handling and error propagation requirements for LLM judge client headers.
- Add explicit stability annotations to experimental `agentic_rag.h` and `multimodal_rag.h` APIs.

### Medium-Term (Q4 2026)

- Introduce `rag_policy.h` to provide per-pipeline retrieval resource quotas and access-policy contract.
- Expose benchmark-reference precision/recall and latency targets for retrieval and evaluation hot paths.
- Deprecate any redundant single-evaluator APIs superseded by the `QualityControlPipeline` composition model.

### Long-Term

- Unify retrieval result types behind a shared passage-context envelope across all retriever variants.
- Add extension hooks for embedders to inject custom evaluation backends alongside the built-in judge ensemble.
- Provide pipeline explain outputs via `quality_control_pipeline.h` to expose evaluation decision traces to consumers.
