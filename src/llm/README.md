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

## Gap Status (L0.5 Verified - 2026-06-25)

**Gap Summary**: 3,821 verified gaps across 146 source files
- **CRITICAL**: 1,029 gaps (26.9%) - Require immediate attention for production safety
- **HIGH**: 1,937 gaps (50.7%) - High-priority fixes for stability and performance
- **MEDIUM**: 854 gaps (22.4%) - Medium-priority improvements
- **LOW**: 1 gap (0.0%)

**Top Issue Categories**:
- LLM AI Safety: 1,910 findings (model integrity, prompt injection, LLM output validation)
- Performance: 391 findings (query optimization, inefficient algorithms, copy overhead)
- Data Races & Concurrency: 321 findings (synchronization issues, thread safety)
- Resource Management: 125 findings (leaks, manual cleanup, GPU memory)
- Observability: 93 findings (missing instrumentation, hardcoded values)

**Remediation Status**:
- [ ] Review CRITICAL gaps by module component
- [ ] Correlate with test coverage analysis
- [ ] Open GitHub issues for tracking
- [ ] Target: Q3 2026 remediation sprint

For detailed breakdown, see [MODULE_GAPS.md](MODULE_GAPS.md) and root [ROADMAP.md](../../ROADMAP.md).

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
