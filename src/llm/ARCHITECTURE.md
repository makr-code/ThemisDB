# LLM Module - Architecture Guide

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

Version: 1.0
Last Updated: 2026-05-31
Module Path: src/llm/

## 1. Overview

The LLM module provides inference execution, routing, model and adapter lifecycle control, streaming, policy enforcement, and runtime safety surfaces.

## 2. Architecture Surfaces

| Surface | Source files |
|---|---|
| Core inference engines | src/llm/async_inference_engine.cpp, src/llm/inference_engine_enhanced.cpp |
| Scheduling and queueing | src/llm/shared_worker_pool.cpp, src/llm/continuous_batch_scheduler.cpp |
| Routing and orchestration | src/llm/model_router.cpp, src/llm/ai_orchestrator.cpp |
| Model and plugin lifecycle | src/llm/llm_plugin_manager.cpp, src/llm/model_loader.cpp, src/llm/model_downloader.cpp |
| Adapter and LoRA lifecycle | src/llm/multi_lora_manager.cpp, src/llm/adapter_load_balancer.cpp, src/llm/lora_router.cpp |
| Streaming and response shaping | src/llm/streaming_handler.cpp, src/llm/openai_compat_adapter.cpp |
| Policy and safety controls | src/llm/prompt_policy.cpp, src/llm/llm_security_utils.cpp, src/llm/production_validator.cpp |
| Caching and resource controls | src/llm/llm_response_cache.cpp, src/llm/kv_cache_buffer.cpp, src/llm/token_quota_manager.cpp |

## 3. Runtime Control Flow

1. Request enters engine submit path.
2. Policy/guard checks run before backend inference call.
3. Router and scheduling choose model/worker execution path.
4. Plugin/backend executes inference with cache/adapter/resource controls.
5. Result is emitted as full response or stream callback frames.

## 4. Integration Boundaries

| Direction | Integration |
|---|---|
| Used by | API handlers, orchestration layers, AI runtime features |
| Uses | LLM backends/plugins, optional acceleration stacks, module-local safety controls |
| Exposes | inference APIs, routing hooks, adapter lifecycle hooks, streaming callbacks |

## 5. Concurrency Model

- Shared worker and scheduler components coordinate concurrent inference jobs.
- Engine and manager components coordinate lifecycle state for models/adapters/plugins.
- Streaming callbacks and cancellation paths are coordinated with request lifecycle state.

## 6. Known Limits

- Some distributed and federated execution paths depend on deployment wiring and are not default-on.
- Runtime behavior can vary across backend and acceleration configurations.
- Cross-node and topology-sensitive benchmark coverage remains an ongoing hardening area.

## 7. Sourcecode Verification (Module: llm/architecture)

- Verified files:
  - src/llm/async_inference_engine.cpp
  - src/llm/inference_engine_enhanced.cpp
  - src/llm/shared_worker_pool.cpp
  - src/llm/continuous_batch_scheduler.cpp
  - src/llm/model_router.cpp
  - src/llm/llm_plugin_manager.cpp
  - src/llm/multi_lora_manager.cpp
  - src/llm/streaming_handler.cpp
  - src/llm/openai_compat_adapter.cpp
  - src/llm/prompt_policy.cpp
  - src/llm/llm_security_utils.cpp
  - src/llm/token_quota_manager.cpp
- Verified interfaces and behavior:
  - request submit/schedule/execute flow
  - routing and lifecycle integration points
  - streaming and policy guard control surfaces
