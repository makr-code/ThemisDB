> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/llm/ROADMAP.md -->

# LLM Module — Public Header Roadmap

**Module Path:** `include/llm/`
**Canonical implementation roadmap:** [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)

---

## Overview

This document tracks public API contract stability, planned header additions, and header-level breaking changes for `include/llm/`. For feature roadmap items that affect both implementation and headers see the canonical roadmap:

→ [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)

---

## Current Status

production LLM runtime with async inference, continuous batching, adapter management, routing, VRAM management, and policy/safety controls. All production-required public headers are present and `#pragma once` guarded.

The header API surface is **stable** for all types introduced in v1.x.

---

## Completed ✅

- [x] `async_inference_engine.h` — inference and scheduling contract
- [x] `continuous_batch_scheduler.h` — inference and scheduling contract
- [x] `shared_worker_pool.h` — inference and scheduling contract
- [x] `batch_generator.h` — inference and scheduling contract
- [x] `block_table.h` — inference and scheduling contract
- [x] `context_window_budget.h` — inference and scheduling contract
- [x] `ai_orchestrator.h` — routing and orchestration contract
- [x] `model_router.h` — routing and orchestration contract
- [x] `embedded_llm.h` — routing and orchestration contract
- [x] `docs_assistant.h` — routing and orchestration contract
- [x] `adapter_registry.h` — adapter and model lifecycle contract
- [x] `adapter_deployment_manager.h` — adapter and model lifecycle contract
- [x] `adapter_load_balancer.h` — adapter and model lifecycle contract
- [x] `adapter_compatibility.h` — adapter and model lifecycle contract
- [x] `aql_train_parser.h` — adapter and model lifecycle contract
- [x] `active_vram_allocator.h` — memory and gpu contract
- [x] `adaptive_vram_allocator.h` — memory and gpu contract
- [x] `distributed_training_coordinator.h` — memory and gpu contract
- [x] `ai_decision_auditor.h` — safety, policy, and observability contract
- [x] `byzantine_detector.h` — safety, policy, and observability contract
- [x] `constitutional_reasoning_engine.h` — safety, policy, and observability contract
- [x] `decision_record_yaml_processor.h` — safety, policy, and observability contract

---

## In Progress 🚧

- [I] Header-level unit test coverage for all public interfaces (tracked via module issue backlog)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] Audit all headers for missing `[[nodiscard]]` on factory and error-returning methods (Target: Q3 2026)
- [ ] Verify `#pragma once` guard consistency across all headers in a CI step (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] Align header-level type documentation with OpenAPI spec where applicable (Target: Q4 2026)
- [ ] Consolidate deprecated symbol annotations with `[[deprecated("...")]]` where needed (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [P] Header-level unit tests (tracked in module issue backlog)

---

## References

- Canonical implementation roadmap: [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
