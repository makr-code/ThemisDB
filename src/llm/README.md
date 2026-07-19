# ThemisDB LLM Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The LLM module provides inference runtime, routing, model/adapter lifecycle management, and LLM-oriented orchestration surfaces used by ThemisDB AI features.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| async_inference_engine.cpp | asynchronous inference submission and completion handling |
| inference_engine_enhanced.cpp | multi-model orchestration and enhanced runtime controls |
| shared_worker_pool.cpp | shared worker scheduling for LLM execution paths |
| model_router.cpp | rule-based model routing and selection |
| llm_plugin_manager.cpp | plugin/backend lifecycle control |
| multi_lora_manager.cpp | LoRA adapter load/switch/unload lifecycle |
| streaming_handler.cpp | streaming token framing and callback paths |
| prompt_policy.cpp | prompt safety/policy enforcement helpers |
| token_quota_manager.cpp | per-model/per-request quota enforcement |
| production_validator.cpp | runtime validation and production-safety checks |

## Scope

In scope:
- inference runtime and orchestration
- model and adapter lifecycle operations
- routing and scheduling for LLM execution
- streaming output and prompt/policy controls
- runtime safety, quota, and observability helpers

Out of scope:
- persistence internals and storage engine behavior
- HTTP gateway implementation details outside LLM runtime adapters
- non-LLM domain modules unrelated to inference/orchestration

## Known Limitations

- Some advanced distributed/federated paths require deployment wiring and are not universally default-enabled.
- Runtime behavior can vary by selected backend and available acceleration stack.
- Benchmark coverage is broad but still evolving for all cross-node production scenarios.

## Runtime Fallback and Verification Status (validated: 2026-07-19)

- Doxygen `@file` coverage is complete across the module source and public headers.
- EmbeddedLLM fallback mode now preserves the single-text embedding contract for
  `embedBatch(...)` by returning one normalized embedding per input text.
- Backend-dependent extension points remain documented as optional runtime
  bridges/fallbacks; they either delegate to injected implementations or fail
  closed / degrade safely when the corresponding upstream capability is absent.
- Release-signoff work that is broader than these source-level fallback fixes
  remains tracked in [ROADMAP.md](ROADMAP.md) and
  [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md).

Historical broad-spectrum scan inventories are retained in [MODULE_GAPS.md](MODULE_GAPS.md),
but they are not the source of truth for the targeted runtime fallback coverage
documented in this README.

## Sourcecode Verification (Module: llm/readme)

- Verified files:
  - src/llm/async_inference_engine.cpp
  - src/llm/inference_engine_enhanced.cpp
  - src/llm/shared_worker_pool.cpp
  - src/llm/model_router.cpp
  - src/llm/llm_plugin_manager.cpp
  - src/llm/multi_lora_manager.cpp
  - src/llm/streaming_handler.cpp
  - src/llm/prompt_policy.cpp
  - src/llm/token_quota_manager.cpp
  - src/llm/production_validator.cpp
- Verified behavior surfaces:
  - request submission, scheduling, routing, and streaming
  - plugin/adapter lifecycle behavior
  - policy/quota/validation control surfaces
- Note:
  - Forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md.
  - Historical implementation record remains in CHANGELOG.md.
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`

## Installation

This module is built as part of ThemisDB. See the root CMakeLists.txt for build configuration.
