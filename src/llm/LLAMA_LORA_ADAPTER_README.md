<!-- Status: current | validated: 2026-11-10 | Source: src/llm/multi_lora_manager.cpp, src/llm/llama_wrapper.cpp (llama.cpp b7974) -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md · AUDIT.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# LoRA Adapter Implementation for ThemisDB

> **Scope:** LoRA serving against the pinned llama.cpp **b7974** API (`MultiLoRAManager`,
> `LlamaWrapper`) and the adapter GGUF metadata contract (`include/llm/llama_lora_adapter.h`).
> For GGUF model loading see [GGUF_LOADER_README.md](GGUF_LOADER_README.md).
> For the LoRA training framework see `src/llm/lora_framework/`.

## Overview

LoRA/AdaLoRA adapters are distributed **independently of the base model** and loaded
**dynamically at runtime** as standalone GGUF adapter files. They are never merged into the
base-model weights. AdaLoRA adapters are exported as an equivalent plain-LoRA GGUF.

## llama.cpp b7974 API used

`MultiLoRAManager` and `LlamaWrapper` call the public functions from `<llama.h>` directly:

```cpp
llama_adapter_lora* llama_adapter_lora_init(llama_model* model, const char* path_lora);
int32_t llama_set_adapter_lora(llama_context* ctx, llama_adapter_lora* adapter, float scale);
int32_t llama_rm_adapter_lora(llama_context* ctx, llama_adapter_lora* adapter); // -1: not attached
void    llama_clear_adapter_lora(llama_context* ctx);
```

`llama_adapter_lora_free()` is deprecated in b7974 and is **not** used: adapter memory is
owned by the `llama_model` it was initialised with and is released together with that model.

## Ownership and lifecycle

| Step | Call | Notes |
|------|------|-------|
| Register | `MultiLoRAManager::loadLoRA(id, path, base_model_id, scale)` | Path must be inside the trusted adapter directory; the GGUF KV metadata is read and exposed as `LoRAInfo::metadata["gguf"]`. |
| Hot-load | `initializeLoRAWithModel(id, model, BaseModelIdentity)` | Verifies GGUF KV (see below), then `llama_adapter_lora_init`. Idempotent per (adapter, model instance); handles of the same file on the same model instance are reused. |
| Apply per request | `applyLoRA(id, ctx)` | `llama_set_adapter_lora(ctx, handle, scale)`; signature stable (used by continuous batching). |
| Detach | `removeLoRA(id, ctx)` | `llama_rm_adapter_lora`; "not attached" is not an error. |
| Unload | `unloadLoRA(id)` | Detaches from all contexts it was applied to and drops the handle reference. Memory is freed with the model. |
| Model reload | `invalidateModelBindings(model or nullptr)` | Called by `LlamaWrapper::unloadModel()`. Handles bound to a previous model instance (detected via `BaseModelIdentity::generation`) are re-initialised on next use. |

`LlamaWrapper::generate()` binds the requested adapter per request with an RAII guard
(`ScopedRequestLoRA`), both in regular and speculative decoding:

- **Fail-closed:** an unknown adapter, a failed hot-load (incompatible/invalid GGUF) or a failed
  apply returns `InferenceResponse{success=false, error_message=...}`; the request is never
  served by the bare base model. `lora_used` is only set when the adapter was applied.
- After the request (also on exceptions) the adapter is detached, so the next request on the
  same context does not inherit it.
- The response cache key is `prompt|model_id|lora=<id>@<scale>` (or `|lora=<none>`).
  `AsyncInferenceEngine` and `InferenceEngineEnhanced` use adapter-qualified keys as well;
  `InferenceEngineEnhanced` rejects requests that name an unregistered adapter.

The manager is reachable for server wiring via `LlamaWrapper::loraManager()` /
`EmbeddedLLM::loraManager()` (non-owning; valid for the lifetime of the wrapper), e.g. to
install `MultiLoRAManager::setAdapterMetadataVerifier()`.

## Adapter GGUF metadata contract

All adapter/training metadata lives in the GGUF KV store. Before `llama_adapter_lora_init`
the manager reads it with the ggml `gguf.h` API (`gguf_init_from_file` with `no_alloc=true`,
RAII-freed) and enforces:

| Key | Rule |
|-----|------|
| `adapter.type` | must be `"lora"` |
| `general.architecture` | required; must equal the loaded model's `general.architecture` |
| `themis.base_model.gguf` | when present and the base-model file name is known: must match |
| `themis.base_model.sha256` | when present and the base-model sha256 is configured: must match (case-insensitive) |
| `themis.base_model.name` | mismatch is logged only |
| (optional) | `setAdapterMetadataVerifier()` callback, e.g. provenance signature check |

`general.*`, `adapter.*` and `themis.*` keys are exposed as JSON in `LoRAInfo::metadata["gguf"]`.

## Export / import (distribution between shards)

- `exportLoRA(id)` returns the exact bytes of the registered standalone adapter GGUF (checked
  for the `GGUF` magic and size ≤ 2 × `max_lora_vram_mb`). No base-model weights, no merging.
- `importLoRA(id, bytes, base_model_id)` validates the payload (size, magic, parseable GGUF,
  `adapter.type == "lora"`, architecture present), writes it atomically to
  `<lora_base_dir>/imported/<id>/<themis.adapter.version|unversioned>/adapter.gguf` and
  registers it for dynamic loading. Requires `Config::lora_base_dir`; existing ids are rejected.
- The safetensors distribution package (`adapter_model.safetensors` + `__metadata__`) is
  validated and converted to GGUF by the training/orchestration layer before registration.

## Legacy shim `llama_lora_adapter.cpp`

`src/llm/llama_lora_adapter.cpp` still exports the pre-b7974 symbol names resolved via
`dlsym`/`GetProcAddress` together with the `themis_lora_inject_api_functions()` test hook.
Those symbols do not exist in b7974, so the shim reports the API as unavailable in
production. It is no longer used by the serving path; its removal is pending
(see `ai_working/E2E_LORA_BLOCKED_ITEMS.md`).

## Supported Adapter File Formats

| Format | Role | Notes |
|--------|------|-------|
| LoRA adapter GGUF (`general.type=adapter`, `adapter.type=lora`) | Serving | Only format passed to `llama_adapter_lora_init`; metadata contract above |
| SafeTensors (`adapter_model.safetensors` + `adapter_config.json`) | Distribution | Validated and converted to GGUF before registration; never loaded by llama.cpp directly |
| Pickle checkpoints (`.bin`, `.pt`, `.pkl`, `.ckpt`) | — | Rejected (code-execution risk) |

Path validation (trusted-directory enforcement) applies to **all** formats.

## Security & Path Validation

> **AUDIT.md F1-1 / F1-2 / F2-1:** LoRA adapter paths are validated against the configured trusted
> adapter directory (`MultiLoRAManager::isLoRAPathTrusted`) before any file I/O; imported adapters
> are written under `<lora_base_dir>/imported/`. The GGUF KV metadata is verified against the
> loaded base model before `llama_adapter_lora_init`. See [SECURITY.md](SECURITY.md) and
> [AUDIT.md](AUDIT.md) for the full threat model.

## Failure Modes

| Condition | Result | Log level |
|-----------|--------|-----------|
| Adapter id unknown in the request | `success=false`, `"Unknown LoRA adapter '<id>'"` | `ERROR` |
| GGUF unreadable / `adapter.type != lora` / architecture or base-model mismatch | `initializeLoRAWithModel` → `false`; request `success=false` | `ERROR` |
| `llama_adapter_lora_init` returns null | `initializeLoRAWithModel` → `false`; request `success=false` | `ERROR` |
| `llama_set_adapter_lora` non-zero | `applyLoRA` → `false`; request `success=false` | `ERROR` |
| `llama_rm_adapter_lora` returns -1 | adapter was not attached; treated as removed | `DEBUG` |
| Export: file missing / not GGUF / too large | empty byte vector | `ERROR` |
| Import: invalid id, payload or missing `lora_base_dir` | `false` | `ERROR` |

## Compatibility

### llama.cpp Versions

- **Pinned**: llama.cpp `b7974` (`cmake/Dependencies.cmake`). The serving path uses the b7974
  function names directly; a llama.cpp upgrade that renames the adapter API requires updating
  `multi_lora_manager.cpp`.

### ThemisDB Integration

- **MultiLoRAManager**: Full integration with vLLM-style multi-LoRA support
- **LlamaWrapper**: Compatible with lazy model loading

## Performance

### Zero-Copy Design

- Direct pointer passing to llama.cpp
- No intermediate copies of adapter weights
- Minimal overhead (<1ms for adapter application)

### Memory Efficiency

- Adapters loaded once and shared across contexts
- LRU cache for frequently used adapters
- Memory tracking and automatic eviction

## Logging

The implementation provides detailed logging at multiple levels:

```
INFO:  API detection results and initialization status
DEBUG: Detailed function call traces and parameter values
WARN:  Non-critical issues and fallback behavior
ERROR: Critical failures with recovery instructions
```

## Error Codes

| Return Value | Meaning |
|--------------|---------|
| 0 | Success |
| -1 | General error (see logs) |
| Non-zero | llama.cpp specific error code |

## Security Considerations

1. **Path Validation**: Adapter paths must be validated against the trusted directory before loading (see AUDIT.md F1-1/F1-2/F2-1); see **Security & Path Validation** section above
2. **Null Checks**: All pointers checked before dereferencing
3. **Error Propagation**: Errors from llama.cpp properly propagated
4. **Memory Safety**: No manual memory management of adapter data
5. **Integrity Verification**: `lora_security_validator.cpp` enforces SHA-256 manifest check before any adapter is registered in `AdapterRegistry`

## Implemented Enhancements (since v1.3.1)

1. **Adapter Verification**: ✅ Implemented in `lora_security_validator.cpp` (SHA-256 + trusted manifest)
2. **Hot-Reloading**: ✅ Implemented via `AdapterRegistry::hotLoad()` (`src/llm/adapter_registry.cpp`, `include/llm/adapter_registry.h`)
3. **Multi-Adapter Composition**: ✅ Implemented in `multi_lora_manager.cpp` (vLLM-style multi-LoRA support)
4. **Quantized adapter format (GGUF-ST)**: ✅ Partial — quantized base models are fully supported via `model_quantization_pipeline.cpp`; GGUF-ST adapter weights (LoRA deltas) are loaded by `GGUFSTAdapter` (`include/llm/gguf_st_adapter.h`); raw quantized-weight LoRA training is not yet implemented (Target: Q3 2026, see [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md))
5. **Test Injection API**: ✅ `themis_lora_inject_api_functions()` added 2026-05-06; tests LORA-INJ-01..03

## Related Components

| Component | Role |
|-----------|------|
| `multi_lora_manager.cpp` | High-level vLLM-style multi-LoRA manager; primary caller of this shim |
| `llama_wrapper.cpp` | llama.cpp context/model lifecycle management |
| `adapter_registry.cpp` | Runtime hot-load registry with trusted-path enforcement |
| `lora_security_validator.cpp` | SHA-256 integrity verification before adapter registration |
| `lora_framework/` | LoRA training and GGUF/safetensors storage infrastructure |
| `include/llm/gguf_st_adapter.h` | GGUF-ST format loader for adapter weights |

## Core LLM Documentation

| Document | Description |
|----------|-------------|
| [README.md](README.md) | Module overview, configuration surfaces (AdapterRegistry knobs) |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Component diagrams, LoRA hot-load flow |
| [SECURITY.md](SECURITY.md) | Threat model, LoRA trust model, path-injection mitigations |
| [AUDIT.md](AUDIT.md) | S0/S1/S2 findings (F1-1, F1-2, F2-1 — LoRA path validation) |
| [ROADMAP.md](ROADMAP.md) | LoRA hot-loading status (`[x]`, Issue #1929, #1935) |
| [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) | `LoraSecurityValidator` cert-store integration, quantized adapter format backlog |
| [GGUF_LOADER_README.md](GGUF_LOADER_README.md) | GGUF model file loading |

## External References

- [llama.cpp LoRA Documentation](https://github.com/ggerganov/llama.cpp)
- [ThemisDB LoRA Integration Guide](../../docs/en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md)
- [LoRA Adapter Application Guide](../../docs/en/lora/LORA_ADAPTER_APPLICATION_GUIDE.md)
- [GGUF-ST Format](../../docs/llm_orchestration/GGUF_SUPPORT.md)

## Changelog

### v1.4.0 (2026-11-10)
- Serving migrated to the llama.cpp b7974 LoRA API (`llama_adapter_lora_init`, `llama_set_adapter_lora`, `llama_rm_adapter_lora`); adapters owned by the model, no deprecated free.
- GGUF KV verification before hot-load; metadata exposed on `LoRAInfo`.
- Fail-closed per-request binding with detach after each request; adapter-qualified response-cache keys.
- `exportLoRA`/`importLoRA` transfer the standalone adapter GGUF.
- `LlamaWrapper::loraManager()` / `EmbeddedLLM::loraManager()` accessors.

### v1.3.3 (2026-05-06)
- `themis_lora_inject_api_functions(init, set, remove, clear, free)` extern "C" API added
- `g_lora_api_override_active` override path bypasses dlsym for test isolation
- Null arguments revert to dlsym-detected path
- Tests LORA-INJ-01..03 added in `tests/llm/test_gpu_lora_integration.cpp`

### v1.3.2 (2026-04-21)
- Path injection fixes F1-1, F1-2, F2-1: trusted-directory validation enforced at call site
- Path-only legacy setter of the shim renamed to avoid collision with the integer-handle overload
- STUB/SIMULATION documentation block added at top of source file

### v1.3.1 (2026-01-26)
- Initial real implementation replacing stub
- Dynamic API detection and loading
- Full LoRA lifecycle support
- Cross-platform compatibility
- Backward compatible overloads

### v1.3.0
- Stub implementation returning -1
- Prevented real LoRA functionality

## Review / Audit Trail

| Date | Reviewer | Scope | Result |
|------|----------|-------|--------|
| 2026-04-21 | Copilot | `llama_lora_adapter.cpp` path injection (F1-1, F1-2, F2-1) | Fixed — trusted-directory enforcement at `AdapterRegistry` / call site |
| 2026-05-06 | Copilot | Test injection API (`themis_lora_inject_api_functions`) | Added — LORA-INJ-01..03 tests pass |
| 2026-05-13 | Copilot | Documentation sync with `llama_lora_adapter.cpp` v0.0.47 | Updated — scope boundary, formats, failure modes, test injection API, corrected future enhancements, cross-references added |

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
