> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# llama_cpp Plugin — Architecture Guide

<!-- Status: current | validated: 2026-04-07 | Primary: src/llama_cpp/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

**Version:** 1.0
**Last Updated:** 2026-04-07
**Module Path:** `src/llama_cpp/`

---

## 1. Overview

The `llama_cpp` plugin wraps the existing `LlamaWrapper` / `ILLMPlugin` infrastructure
for dynamic loading via the plugin system. It provides the full `ILLMPlugin` contract
(generate, RAG, embed, LoRA lifecycle) as a loadable shared library or static target.

---

## 2. Design Principles

- **Interface-complete** — all `ILLMPlugin` methods are implemented; no pure-virtual
  method left as a link-time error.
- **Stub-first** — `loadModel("")` succeeds without a model file; `generate()` returns
  a detectable echo stub; tests never require a model.
- **Thread-safe lifecycle** — `loadModel`, `unloadModel`, `generate`, `loadLoRA`,
  `unloadLoRA`, `listLoRAs`, `getModelInfo` are all guarded by `std::mutex`.
- **LoRA registry** — duplicate `lora_id` is replaced (not accumulated), consistent with
  the LLM module's LoRA lifecycle.
- **Stats** — `inference_count` and `error_count` are monotonic; `getPerformanceStats()`
  is callable without model load.

---

## 3. Component Diagram

```
┌──────────────────────────────────────────────────────────┐
│   ILLMPlugin   (include/llm/llm_plugin_interface.h)      │
└────────────────────────┬─────────────────────────────────┘
                         │ implements
             ┌───────────▼──────────────┐
             │     LlamaCppPlugin        │
             │  ┌──────────────────────┐ │
             │  │  std::mutex mutex_   │ │  guards all state
             │  └──────────────────────┘ │
             │  ┌──────────────────────┐ │
             │  │  vector<LoRAEntry>   │ │  {id, path, scale}
             │  └──────────────────────┘ │
             │  inference_count_         │
             │  error_count_             │
             └───────────────────────────┘
```

---

## 4. Key Data Flows

### 4.1 generate(request)

```
LlamaCppPlugin::generate(request)
  ├─ lock mutex_  (snapshot model_loaded_, policy_fn_)
  ├─ if (policy_fn_ && !policy_fn_(request, reason))
  │     → error response { success=false, "denied by policy: reason" } + ++error_count_
  ├─ if (!model_loaded_) → error response + ++error_count_
  ├─ ++inference_count_
  ├─ if (cancellation_token && *token == true) → { success=false, "Request cancelled" }
  ├─ THEMIS_LLM_ENABLED + wrapper_: delegate to LlamaWrapper::generate()
  │     stream_callback invoked via invokeStreamCallback(token) [retry ≤3, bad_alloc non-retryable]
  ├─ stub path: response.text = "[stub:" + prompt[:40] + "]"
  └─ return response { success=true/false }
```

### 4.2 generateRAG(request, context_docs)

```
LlamaCppPlugin::generateRAG(rag_context, request)
  ├─ lock mutex_  (snapshot model_loaded_, context_length_)
  ├─ early-out: if (!snap_model_loaded) → error + ++error_count_
  ├─ release lock
  ├─ RAGContextAssembler: chunk + rank + truncate (uses snap_context_length)
  ├─ read rag_mode from request.metadata (caller-owned, lock-free)
  └─ delegate to generate(augmented_request)
        [policy gate fires inside generate()]
```

### 4.3 LoRA Lifecycle

```
loadLoRA(path, id, scale)
  ├─ lock mutex_
  ├─ remove existing entry with same id (replace semantics)
  └─ push_back {id, path, scale}

unloadLoRA(id)
  ├─ lock mutex_
  └─ erase if found (returns false if not found)
```

---

## 5. Capabilities

| Capability | v2.0.0 |
|---|---|
| `supports_streaming` | `false` |
| `supports_lora` | `true` |
| `supports_embeddings` | `true` (zero vector in stub) |
| `supports_rag` | `true` |
| `supports_function_call` | `false` |

---

## 6. Stats

| Stat | JSON Key | Description |
|---|---|---|
| Memory | `model_loaded` | bool |
| Memory | `model_id` | string |
| Memory | `lora_count` | int |
| Performance | `inference_count` | monotonic uint64 |
| Performance | `error_count` | monotonic uint64 |

---

## 7. Dynamic Loading

The `THEMIS_LLM_PLUGIN()` macro at the bottom of `include/llama_cpp/llama_cpp_plugin.h`
generates `themis_llm_create()` and `themis_llm_destroy()` with C linkage and
`THEMIS_PLUGIN_EXPORT` visibility, enabling `dlopen` / `LoadLibrary` based loading.

---

## 8. Thread Safety

`LlamaCppPlugin` is **thread-safe** for all public methods via `std::mutex mutex_`.

---

## 9. Testing Strategy

| Type | Files | Count |
|---|---|---|
| Unit (stub mode) | `src/llama_cpp/tests/test_llama_cpp_plugin.cpp` | 30 |
