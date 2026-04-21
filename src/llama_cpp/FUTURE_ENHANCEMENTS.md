> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Future Enhancements — llama_cpp Plugin

## Scope

Planned enhancements beyond v2.0.0. Core implementation in
`src/llama_cpp/llama_cpp_plugin.cpp`.

---

## Design Constraints

- `ILLMPlugin` interface must remain the stable ABI; new capabilities added as optional
  methods with default implementations (e.g., `exportLoRA`, streaming callback).
- Stub mode (no model file, `loadModel("")`) must remain functional after every enhancement.
- Thread-safety guarantee (`std::mutex`) must be maintained across all new code paths.
- All calls to the real `LlamaWrapper` must remain conditional on `model_loaded_ == true`.
- `inference_count_` and `error_count_` must be incremented on every code path that calls
  or fails to call the underlying model.

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ILLMPlugin::generateStream(request, token_callback)` | Streaming API endpoint | New optional method; `LlamaCppPlugin` calls `LlamaWrapper::generateStream()` |
| `ILLMPlugin::generateBatch(requests)` | Batch inference API | Returns `vector<InferenceResponse>` |
| `ILLMPlugin::exportLoRA(lora_id)` | LoRA management API | Returns serialised adapter bytes |
| `ILLMPlugin::importLoRA(data, lora_id)` | LoRA management API | Deserialises and registers adapter |

---

## Planned Features

### 1. Real llama.cpp Inference via LlamaWrapper (Target: Q3 2026)

**Problem:** `generate()` returns an echo stub in v2.0.0.

**Solution:** Add a real `LlamaWrapper*` member, initialised in `loadModel()`. Gate all
inference calls on `THEMIS_ENABLE_LLAMA_CPP` compile flag (consistent with
`THEMIS_ENABLE_WHISPER` pattern).

**Inputs:** `InferenceRequest { prompt, max_tokens, temperature, top_p }`.
**Outputs:** `InferenceResponse { text, tokens_generated, latency_ms }`.
**Constraints:** `LlamaWrapper` must be initialised before any `generate()` call;
double-init must be safe (unload + reload).
**Errors:** Model load failure → `loadModel()` returns `false`; generate on unloaded model
→ error response.
**Tests:** Integration test with a tiny GGUF model in CI fixtures.
**Perf target:** ≤ 200 ms for 50-token prompt on RTX 3090 equivalent.

---

### 2. Streaming Token Output (Target: Q3 2026)

**Problem:** `generate()` blocks until the entire response is generated.

**Solution:** Add `generateStream(request, callback)` to `ILLMPlugin`.
`LlamaCppPlugin::generateStream()` calls `LlamaWrapper::generateStream()` and forwards
each token to the callback on the calling thread.

**Constraints:** Callback must not throw; exceptions from the callback must be caught and
recorded as `error_count_++`.
**Tests:** 2 unit tests with a mock `LlamaWrapper` emitting 5 tokens.
**Perf target:** ≤ 30 ms first-token latency on stub.

---

### 3. Real Embedding Model (Target: Q3 2026)

**Problem:** `embed()` returns a fixed 384-dim zero vector.

**Solution:** `LlamaWrapper::embed()` provides real embedding vectors when an embedding
model is loaded. `LlamaCppPlugin::embed()` will delegate to it.

**Constraints:** Embedding model may be different from the generation model; support
loading both simultaneously.
**Tests:** 3 unit tests: cosine similarity between related vs. unrelated texts.
**Perf target:** ≤ 5 ms per 512-token text on CPU.

---

### 4. exportLoRA / importLoRA (Target: Q4 2026)

**Problem:** `exportLoRA` returns empty and `importLoRA` returns false.

**Solution:** Serialize the LoRA weight matrices to a binary format (GGUF-compatible or
custom); `importLoRA` deserialises and hot-loads via `LlamaWrapper::loadLoRA()`.

**Security:** Serialised LoRA bytes are validated (magic bytes, size bounds) before
deserialisation to prevent injection attacks.
**Tests:** Round-trip test: export → import → same weights.

---

### 5. Function / Tool Calling (Target: Q4 2026)

**Problem:** `supports_function_call` is `false` in v2.0.0.

**Solution:** Add `LlamaCppPlugin::callTool(request, tool_schema)` using JSON schema
grammar-constrained generation (consistent with LLM module's grammar.cpp).

**Constraints:** Grammar validation required before compilation; recursive grammars bounded
by depth limit (same constraint as LLM module).
**Tests:** 5 unit tests with JSON schema fixtures.

---

### 6. Structured Error on Unloaded Model — Replace Silent Stub Echo (Target: Q3 2026)

**Source:** `AI_ML_IMPACT_ASSESSMENT.md §7, Gap 1 (Severity: High/S1)`

**Problem:** When `LlamaCppPlugin::generate()` is called without a loaded model
(wrapper_ == nullptr), it returns a stub response with `success=true` and text
`"[stub:<prompt_prefix>]"`.  Callers cannot distinguish this from a real inference
result at the `InferenceResponse` level; silent incorrect output may propagate into
RAG pipelines and AQL results.

**Solution:** Replace the silent stub path with a typed failure:
- Set `response.success = false` and populate `response.error_message` with a
  human-readable `"Model not loaded — call loadModel() before generate()"` string.
- Introduce `InferenceErrorCode::MODEL_NOT_LOADED` in the shared inference types so
  calling code can programmatically distinguish this error from network or OOM errors.
- Retain the build-configuration fallback stub path (guarded by a separate
  `THEMIS_LLAMA_CPP_STUB_MODE` compile flag used exclusively in unit-test builds).

**Inputs:** `InferenceRequest` (unchanged); `wrapper_` state (nullptr vs. loaded).
**Outputs:** `InferenceResponse { success=false, error_code=MODEL_NOT_LOADED, error_message }`.
**Constraints:** Existing unit tests that rely on the stub response must be updated to
set the new compile flag or inject a mock `LlamaWrapper`.
**Errors:** No new runtime errors; changes the contract of existing stub path.
**Tests:** 2 unit tests — one asserting `success=false` with `MODEL_NOT_LOADED` code
when model is not loaded; one asserting the stub continues to work under
`THEMIS_LLAMA_CPP_STUB_MODE`.
**Perf target:** No performance impact (error path only).

---

## Security / Reliability

- All new inference paths must pass through `PolicyEngine::checkInferencePermission()`
  before queueing (consistent with LLM module security policy).
- `importLoRA` must validate size bounds before heap allocation.
- `generateStream` callbacks must never receive pointers to stack-allocated token data
  that may be invalidated after the streaming call returns.
