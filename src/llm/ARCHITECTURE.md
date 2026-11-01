---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/llm/embedded_llm.h"
  - "../../include/llm/async_inference_engine.h"
  - "../../include/llm/llm_plugin_interface.h"
  - "../../include/llm/model_router.h"
  - "../../include/llm/prompt_policy.h"
  - "../../include/llm/streaming_handler.h"
  - "embedded_llm.cpp"
  - "async_inference_engine.cpp"
  - "llm_plugin_manager.cpp"
  - "../server/llm_api_handler.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# LLM Module Architecture

## Scope, boundaries, and ownership

`src/llm/` owns model-backed inference runtime surfaces: embedded and asynchronous requests, model/backend selection, prompt policy, streaming response helpers, model/adapter lifecycle, and inference resource support. The module does not own client protocol endpoints, search ranking, or durable database storage.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Embedded inference | [`EmbeddedLLM`](../../include/llm/embedded_llm.h) | Configure model/runtime access and expose generation/embedding callbacks and methods. |
| Async execution | [`AsyncInferenceEngine`](../../include/llm/async_inference_engine.h), [`InferenceHandle`](../../include/llm/inference_handle.h) | Submit/manage asynchronous requests and represent request completion/cancellation state. |
| Backend and routing | [`ILLMPlugin`](../../include/llm/llm_plugin_interface.h), [`LLMPluginManager`](../../include/llm/llm_plugin_manager.h), [`ModelRouter`](../../include/llm/model_router.h) | Register/select backend plugins and route model requests. |
| Prompt and stream formatting | [`PromptPolicy`](../../include/llm/prompt_policy.h), [`StreamingHandler`](../../include/llm/streaming_handler.h) | Apply prompt rules; helper formats SSE/chunk strings and adapts a callback to a supplied sink. |
| Embedded stream callbacks | [`EmbeddedLLM`](../../include/llm/embedded_llm.h) | Pass generated text through callbacks and optionally format callback values as SSE. |
| Runtime resources | [`SharedWorkerPool`](../../include/llm/shared_worker_pool.h), [`TokenQuotaManager`](../../include/llm/token_quota_manager.h), [`GPU memory manager`](../../include/llm/gpu_memory_manager.h) | Schedule work and expose token/GPU resource controls. |

## Request and event flow

1. A caller submits through an embedded or asynchronous inference interface.
2. Where wired, prompt policy and routing determine whether and where the request proceeds; async execution uses worker-pool and request-management components.
3. The selected engine/plugin performs inference. `EmbeddedLLM::generateStreaming` supplies a callback to the inference request; `generateStreamingSSE` formats callback values through `LlamaWrapper::formatStreamTokenAsSSE`.
4. Backend status and request completion are returned through the corresponding synchronous result, handle, or callback.

See [`embedded_llm.cpp`](embedded_llm.cpp), [`async_inference_engine.cpp`](async_inference_engine.cpp), [`llm_plugin_manager.cpp`](llm_plugin_manager.cpp), and [`model_router.cpp`](model_router.cpp). The sequence is configurable; not every request traverses every listed component.

The HTTP route `GET /api/v1/llm/stream` is owned by
`LLMApiHandler::handleStreamInference`. It collects SSE events in a string and
returns a `http::string_body` response after generation completes; it does not
use `StreamingHandler` or write each event to the network as generated. The
route currently accepts `prompt`, `request_id`, and `max_tokens` query
parameters; `max_tokens` defaults to 512 and accepted values are capped at
4096. No dedicated streaming configuration or transport cancellation/
backpressure contract was verified.

The same handler exposes OpenAI-compatible `POST /v1/chat/completions`
streaming when the request body sets `stream: true`. It assigns an inference
callback that appends OpenAI-compatible SSE chunks, then returns the buffered
complete response with a final chunk and `[DONE]`; inference exceptions return
JSON errors. This path also does not write chunks incrementally to the network.
`AsyncInferenceEngine` cancellation/deadline controls apply to requests
submitted through that engine; these synchronous HTTP streaming paths do not
submit through it.

## Failure domains and edge cases

- Invalid prompt-rule regex/configuration is rejected when rules are installed.
- Missing model/plugin, model-load errors, and backend inference errors are distinct from successful empty output.
- Async queue, deadline, cancellation, and resource exhaustion affect request completion; callers must inspect the result/status.
- `EmbeddedLLM` contains a fallback embedding helper; that behavior does not imply a general fallback for text generation or backend failures.

## Explicit non-goals

- Owning HTTP/gRPC/WebSocket endpoint behavior.
- Owning search fusion/ranking or RAG retrieval candidate generation.
- Owning durable storage semantics for state/cache backends.
- Guaranteeing GPU availability, distributed inference, or a specific performance envelope for every build.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Search reranking | Search exposes an injected LLM reranker/backend interface; see [search architecture](../search/ARCHITECTURE.md) and [`search/llm_reranker.h`](../../include/search/llm_reranker.h). |
| Storage-backed state | LLM sources include storage interfaces for selected persisted state/model paths; see [storage architecture](../storage/ARCHITECTURE.md). |
| Query integration | Query has LLM/LoRA function interfaces; this is a caller integration, not ownership of the query engine. See [query architecture](../query/ARCHITECTURE.md). |

## References

- [Module README](README.md) · [API contract](API.md) · [module roadmap](ROADMAP.md)
- [`llm_api_contract.h`](../../include/llm/llm_api_contract.h) · [`embedded_llm.h`](../../include/llm/embedded_llm.h) · [`llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h)
