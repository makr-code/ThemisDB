> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/rag/ARCHITECTURE.md -->

# RAG Module — Public Header Architecture

**Module Path:** `include/rag/`
**Implementation:** `../../src/rag/`
**Canonical architecture doc:** [`../../src/rag/ARCHITECTURE.md`](../../src/rag/ARCHITECTURE.md)

---

## 1. Overview

The `include/rag/` directory contains the **public C++ header contract** for ThemisDB's retrieval-augmented generation — retrieval fusion and ranking, context assembly, evaluation and quality control, ingestion, and safety surfaces. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/rag/ARCHITECTURE.md`](../../src/rag/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::rag`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Retrieval and ranking` | `hybrid_retriever.h`, `reranker.h`, `replug_retriever.h`... |
| `Context assembly and orchestration` | `rag_context_assembler.h`, `streaming_retriever.h`, `multi_step_rag.h`... |
| `Evaluation and quality control` | `rag_judge.h`, `faithfulness_evaluator.h`, `quality_control_pipeline.h`... |
| `Safety, guardrails, and learning` | `prompt_injection_detector.h`, `bias_detector.h`, `fairness_detector.h`... |
| `Integration and metrics` | `rag_ingestion_bridge.h`, `rag_integration_helpers.h`, `llm_integration.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_ONNX` | onnx_model_loader.h | ONNX runtime for local model inference |
| `THEMIS_ENABLE_DISTRIBUTED` | distributed_rag_evaluator.h | Distributed RAG evaluation backend |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/rag/ARCHITECTURE.md`](../../src/rag/ARCHITECTURE.md)
- Module overview: [`../../src/rag/README.md`](../../src/rag/README.md)
- Roadmap: [`../../src/rag/ROADMAP.md`](../../src/rag/ROADMAP.md)
- Future enhancements: [`../../src/rag/FUTURE_ENHANCEMENTS.md`](../../src/rag/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
