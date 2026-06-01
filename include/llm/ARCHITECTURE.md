> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/llm/ARCHITECTURE.md -->

# LLM Module — Public Header Architecture

**Module Path:** `include/llm/`
**Implementation:** `../../src/llm/`
**Canonical architecture doc:** [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md)

---

## 1. Overview

The `include/llm/` directory contains the **public C++ header contract** for ThemisDB's LLM inference execution, routing, model and adapter lifecycle, streaming, policy enforcement, and runtime safety surfaces. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::llm`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Inference and scheduling` | `async_inference_engine.h`, `continuous_batch_scheduler.h`, `shared_worker_pool.h`... |
| `Routing and orchestration` | `ai_orchestrator.h`, `model_router.h`, `embedded_llm.h`... |
| `Adapter and model lifecycle` | `adapter_registry.h`, `adapter_deployment_manager.h`, `adapter_load_balancer.h`... |
| `Memory and GPU` | `active_vram_allocator.h`, `adaptive_vram_allocator.h`, `distributed_training_coordinator.h` |
| `Safety, policy, and observability` | `ai_decision_auditor.h`, `byzantine_detector.h`, `constitutional_reasoning_engine.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP` | active_vram_allocator.h, adaptive_vram_allocator.h | GPU VRAM management (CUDA or HIP) |
| `THEMIS_ENABLE_LLM_DISTRIBUTED` | distributed_training_coordinator.h | Distributed training coordinator |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md)
- Module overview: [`../../src/llm/README.md`](../../src/llm/README.md)
- Roadmap: [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)
- Future enhancements: [`../../src/llm/FUTURE_ENHANCEMENTS.md`](../../src/llm/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
