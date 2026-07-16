### Context

This issue implements the roadmap item 'OpenAI-Compatible `/v1/chat/completions` Adapter' for the llm domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: OpenAI-Compatible `/v1/chat/completions` Adapter

### Goal

Deliver the scoped changes for OpenAI-Compatible `/v1/chat/completions` Adapter in src/llm/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### OpenAI-Compatible `/v1/chat/completions` Adapter
**Priority:** High
**Target Version:** v1.7.0

Add an `OpenAICompatAdapter` that translates OpenAI Chat Completions API requests to `InferenceRequest` structs and routes them to `InferenceEngineEnhanced`. This allows existing OpenAI API clients (LangChain, LlamaIndex, OpenAI Python SDK) to target ThemisDB's local inference engine without code changes.

**Implementation Notes:**
- Add `openai_compat_adapter.cpp` in `src/llm/`; parse `POST /v1/chat/completions` JSON body into `InferenceRequest` including `messages`, `temperature`, `max_tokens`, `stream`, `stop`, and `tools` fields.
- Map the `messages` array to the internal prompt format using the same chat template logic already used by `InferenceEngineEnhanced`; support `system`, `user`, and `assistant` roles.
- `stream: true` routes to `IInferenceEngine::submitStreaming()` and emits SSE chunks in the `data: {"choices":[{"delta":{"content":"..."}}]}` format.
- Function/tool calling: serialize tool definitions as grammar constraints via `grammar.cpp` to enforce valid JSON output; return `tool_calls` in the response when the model outputs a function-call JSON object.
- Add API key validation via `PolicyEngine::checkInferencePermission()` before request processing; return HTTP 401 on denied requests.

**Performance Targets:**
- Non-streaming request overhead (adapter serialization/deserialization) ≤ 2 ms vs direct `submitRequest()` call.
- Compatible with OpenAI SDK smoke tests: `openai.ChatCompletion.create()` with `stream=False` and `stream=True` must both succeed against a local ThemisDB instance.

---

### Acceptance Criteria

- [ ] Add `openai_compat_adapter.cpp` in `src/llm/`; parse `POST /v1/chat/completions` JSON body into `InferenceRequest` including `messages`, `temperature`, `max_tokens`, `stream`, `stop`, and `tools` fields.
- [ ] Map the `messages` array to the internal prompt format using the same chat template logic already used by `InferenceEngineEnhanced`; support `system`, `user`, and `assistant` roles.
- [ ] `stream: true` routes to `IInferenceEngine::submitStreaming()` and emits SSE chunks in the `data: {"choices":[{"delta":{"content":"..."}}]}` format.
- [ ] Function/tool calling: serialize tool definitions as grammar constraints via `grammar.cpp` to enforce valid JSON output; return `tool_calls` in the response when the model outputs a function-call JSON object.
- [ ] Add API key validation via `PolicyEngine::checkInferencePermission()` before request processing; return HTTP 401 on denied requests.
- [ ] Non-streaming request overhead (adapter serialization/deserialization) ≤ 2 ms vs direct `submitRequest()` call.
- [ ] Compatible with OpenAI SDK smoke tests: `openai.ChatCompletion.create()` with `stream=False` and `stream=True` must both succeed against a local ThemisDB instance.

### Relationships

- Roadmap row: #77 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#openai-compatible-v1chatcompletions-adapter
- Source key: roadmap:77:llm:v1.7.0:openai-compatible-v1chatcompletions-adapter

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:77:llm:v1.7.0:openai-compatible-v1chatcompletions-adapter -->
<!-- roadmap-ref: row=77;module=llm;target=v1.7.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#openai-compatible-v1chatcompletions-adapter -->
