### Context

This issue implements the roadmap item '`AIOrchestrator`: Tool Call Parsing' for the llm domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `AIOrchestrator`: Tool Call Parsing

### Goal

Deliver the scoped changes for `AIOrchestrator`: Tool Call Parsing in src/llm/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `AIOrchestrator`: Tool Call Parsing
**Priority:** Medium
**Target Version:** v1.8.0

`ai_orchestrator.cpp` line 494 has: "TODO(extensible): parse tool calls from `result.text` using `react_agent`". Without tool call parsing, the orchestrator cannot dispatch function calls returned by the LLM, making the ReAct loop incomplete.

**Implementation Notes:**
- `[ ]` Add tool call extraction to `AIOrchestrator::execute()` at line 494: parse the structured JSON block from `result.text` using `nlohmann::json`; dispatch to the registered tool via `AQLReActAgent::dispatchTool()`.
- `[ ]` Handle malformed tool call JSON gracefully: log a warning and continue with the raw text result rather than crashing.

---


**Priority:** High
**Target Version:** v1.7.0

Add token-streaming support so that callers receive generated tokens incrementally rather than waiting for the full response. This is required by interactive chat applications and the planned OpenAI-compatible API passthrough adapter. Both `AsyncInferenceEngine` and `InferenceEngineEnhanced` must support streaming via a new `submitStreaming()` method.

**Implementation Notes:**
- Add `IInferenceEngine::submitStreaming(InferenceRequest, TokenCallback)` to the engine interface; `TokenCallback` is `std::function<void(std::string_view token, bool is_final)>`.
- In `async_inference_engine.cpp`, invoke the callback from the worker thread after each llama.cpp token decode step; the callback must be thread-safe (called from the worker, consumed by the HTTP layer).
- In `inference_engine_enhanced.cpp`, integrate streaming with `continuous_batch_scheduler.cpp`; each batch step flushes decoded tokens for all in-flight requests to their respective callbacks.
- Return an `InferenceHandle` from `submitStreaming()` so callers can still call `cancel()` to abort mid-stream; on cancellation, the token callback receives a final call with `is_final=true` and an empty token.
- SSE framing (`data: {token}\n\n`) is applied at the HTTP layer, not inside the engine; the engine emits raw token strings.

**Performance Targets:**
- Time-to-first-token (TTFT) ≤ 200 ms p99 for prompt lengths ≤ 512 tokens on a single A10G GPU.
- Streaming overhead (vs non-streaming) ≤ 2 % of total tokens/sec throughput.

---

### Acceptance Criteria

- [ ] Add tool call extraction to `AIOrchestrator::execute()` at line 494: parse the structured JSON block from `result.text` using `nlohmann::json`; dispatch to the registered tool via `AQLReActAgent::dispatchTool()`.
- [ ] Handle malformed tool call JSON gracefully: log a warning and continue with the raw text result rather than crashing.
- [ ] Add `IInferenceEngine::submitStreaming(InferenceRequest, TokenCallback)` to the engine interface; `TokenCallback` is `std::function<void(std::string_view token, bool is_final)>`.
- [ ] In `async_inference_engine.cpp`, invoke the callback from the worker thread after each llama.cpp token decode step; the callback must be thread-safe (called from the worker, consumed by the HTTP layer).
- [ ] In `inference_engine_enhanced.cpp`, integrate streaming with `continuous_batch_scheduler.cpp`; each batch step flushes decoded tokens for all in-flight requests to their respective callbacks.
- [ ] Return an `InferenceHandle` from `submitStreaming()` so callers can still call `cancel()` to abort mid-stream; on cancellation, the token callback receives a final call with `is_final=true` and an empty token.
- [ ] SSE framing (`data: {token}\n\n`) is applied at the HTTP layer, not inside the engine; the engine emits raw token strings.
- [ ] Time-to-first-token (TTFT) ≤ 200 ms p99 for prompt lengths ≤ 512 tokens on a single A10G GPU.
- [ ] Streaming overhead (vs non-streaming) ≤ 2 % of total tokens/sec throughput.

### Relationships

- Roadmap row: #181 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#aiorchestrator-tool-call-parsing
- Source key: roadmap:181:llm:v1.8.0:aiorchestrator-tool-call-parsing

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:181:llm:v1.8.0:aiorchestrator-tool-call-parsing -->
<!-- roadmap-ref: row=181;module=llm;target=v1.8.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#aiorchestrator-tool-call-parsing -->
