<!-- Status: current | validated: 2026-05-13 | Source: src/llm/llama_lora_adapter.cpp v0.0.47 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md · AUDIT.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# LoRA Adapter Implementation for ThemisDB

> **Scope:** This document covers `llama_lora_adapter.cpp` — the dynamic llama.cpp LoRA API shim.
> For GGUF model loading see [GGUF_LOADER_README.md](GGUF_LOADER_README.md).
> For the LoRA training framework see `src/llm/lora_framework/`.
> For the high-level multi-LoRA manager see `multi_lora_manager.cpp` and [README.md](README.md).

## Overview

This file (`llama_lora_adapter.cpp`) provides real LoRA (Low-Rank Adaptation) adapter support for ThemisDB's LLM integration with llama.cpp. It replaces the previous stub implementation with a production-ready dynamic API loader.

## Features

### 1. Dynamic API Detection
- **Runtime Detection**: Automatically detects if llama.cpp was compiled with LoRA support
- **Graceful Degradation**: Falls back safely when LoRA API is not available
- **Cross-Platform**: Works on Windows, Linux, and macOS

### 2. Modern LoRA API Support

The implementation provides the full llama.cpp LoRA adapter lifecycle:

```cpp
// Load adapter from file
void* llama_lora_adapter_init(llama_model* model, const char* path_lora);

// Apply adapter with scaling
int llama_lora_adapter_set_with_scale(llama_context* ctx, void* adapter, float scale);

// Remove adapter
int llama_lora_adapter_remove(llama_context* ctx, void* adapter);

// Clear all adapters
int llama_lora_adapter_clear(llama_context* ctx);

// Free adapter handle
void llama_lora_adapter_free(void* adapter);

// Check API availability
bool themis_llama_lora_available();
```

### 3. Backward Compatibility

**Note:** The legacy path-only signature has been renamed to avoid collision with the modern integer-handle overload:

```cpp
// Legacy path-only signature (backward-compat; always returns -1 because
// model* is unavailable at this call site — use MultiLoRAManager instead)
int llama_lora_adapter_set_path(struct llama_context* ctx, const char* adapter_path);

// MultiLoRAManager signature (integer handle cast from void*)
int llama_lora_adapter_set(llama_context* ctx, int adapter_index, float scale);
```

## Architecture

### Dynamic Loading

The implementation uses platform-specific dynamic library loading:

**Windows:**
```cpp
GetProcAddress(GetModuleHandle(nullptr), "llama_lora_adapter_init")
```

**Unix/Linux/macOS:**
```cpp
dlsym(RTLD_DEFAULT, "llama_lora_adapter_init")
```

### Thread Safety

- Uses `std::call_once` for one-time initialization
- Thread-safe function pointer initialization
- No global state modifications after initialization

### Error Handling

- Comprehensive error checking at each API call
- Detailed logging via spdlog
- Clear error messages with actionable instructions
- Graceful degradation when API unavailable

### Test Injection API

`themis_lora_inject_api_functions()` allows tests to override the dlsym-detected function pointers
without requiring a real llama.cpp LoRA build. Call it before any LoRA API call:

```cpp
// Inject mock LoRA functions for unit tests (LORA-INJ-01..03)
themis_lora_inject_api_functions(
    my_mock_init,    // llama_lora_adapter_init replacement
    my_mock_set,     // llama_lora_adapter_set replacement
    my_mock_remove,  // llama_lora_adapter_remove replacement
    my_mock_clear,   // llama_lora_adapter_clear replacement
    my_mock_free     // llama_lora_adapter_free replacement
);

// Pass nullptr for all to revert to dlsym-detected path
themis_lora_inject_api_functions(nullptr, nullptr, nullptr, nullptr, nullptr);
```

When `g_lora_api_override_active` is true (any non-null init+set pair was injected), the override
path entirely bypasses dlsym detection. The flag is declared in the anonymous namespace of
`llama_lora_adapter.cpp`; do not access it directly from test code — use `themis_lora_inject_api_functions`.

Tests: `tests/llm/test_gpu_lora_integration.cpp` LORA-INJ-01..03 (added 2026-05-06).

## Usage

### From MultiLoRAManager

```cpp
// Load adapter (path must be inside the trusted adapter directory — see SECURITY.md / AUDIT.md F1-1)
bool success = lora_manager->loadLoRA(
    "adapter_id",
    "/trusted/adapters/adapter.bin",
    "base_model_id",
    1.0f  // scale
);

// Apply adapter
bool applied = lora_manager->applyLoRA("adapter_id", context);

// Remove adapter
bool removed = lora_manager->removeLoRA("adapter_id", context);
```

### Direct API Usage

```cpp
// Check if LoRA API is available
if (themis_llama_lora_available()) {
    // Load adapter (path must be inside the trusted adapter directory)
    void* adapter = llama_lora_adapter_init(model, "/trusted/adapters/adapter.bin");
    
    if (adapter) {
        // Apply with scale factor
        llama_lora_adapter_set_with_scale(context, adapter, 1.0f);
        
        // ... perform inference ...
        
        // Remove and free
        llama_lora_adapter_remove(context, adapter);
        llama_lora_adapter_free(adapter);
    }
}
```

## Supported Adapter File Formats

| Format | Extension | Notes |
|--------|-----------|-------|
| llama.cpp native binary | `.bin` | Direct pass-through to `llama_lora_adapter_init` |
| GGUF-ST (GGUF + SafeTensors hybrid) | `.gguf` | Load via `GGUFSTAdapter` from `include/llm/gguf_st_adapter.h`; ThemisDB-specific manifest with SHA-256 integrity signature; see `docs/llm_orchestration/GGUF_SUPPORT.md` |
| SafeTensors | `.safetensors` | Converted to `.bin` before passing to llama.cpp; handled by `AdapterRegistry` |

Path validation (trusted-directory enforcement) applies to **all** formats.

## Security & Path Validation

> **AUDIT.md F1-1 / F1-2 / F2-1 (fixed 2026-04-21):** LoRA adapter paths received from API callers
> or deserialized remote sources **must** be validated against the configured trusted adapter directory
> before being passed to `llama_lora_adapter_init`.  The shim itself does **not** enforce this — path
> validation is the responsibility of `AdapterRegistry` / `lora_security_validator.cpp`.
> See [SECURITY.md](SECURITY.md) and [AUDIT.md](AUDIT.md) for the full threat model.

## Failure Modes

| Condition | Return value | Log level | Message |
|-----------|-------------|-----------|---------|
| LoRA API not available (dlsym miss) | `nullptr` / `-1` | `ERROR` | `"LoRA API not available in this llama.cpp build"` |
| Null `llama_model*` passed to `init` | `nullptr` | `ERROR` | `"llama_lora_adapter_init: null model provided"` |
| Empty or null adapter path | `nullptr` / `-1` | `ERROR` | `"null or empty adapter path"` |
| `llama_lora_adapter_init` returns null | `nullptr` | `ERROR` | `"Failed to load LoRA adapter from: <path>"` |
| Non-zero return from `adapter_set` | non-zero int | `ERROR` | `"Failed to apply LoRA adapter (error: <n>)"` |
| Legacy `set_path` signature called | `-1` | `ERROR` | `"Legacy signature not supported … use MultiLoRAManager"` |
| `llama_lora_adapter_remove` unavailable; `set` available | calls `set(ctx, adapter, 0.0f)` | `WARN` | fallback behaviour |
| Null `adapter` passed to `free` | *(no-op)* | `DEBUG` | `"null adapter (nothing to free)"` |

## Configuration

### Building llama.cpp with LoRA Support

To enable LoRA functionality, llama.cpp must be compiled with LoRA support:

```bash
# CMake option
cmake -DLLAMA_LORA=ON ..

# Or for vcpkg
vcpkg install llama-cpp[lora]
```

### Checking LoRA Availability

At runtime, check if LoRA support is available:

```cpp
if (!themis_llama_lora_available()) {
    spdlog::warn("LoRA API not available - rebuild llama.cpp with LLAMA_LORA=ON");
}
```

## Compatibility

### llama.cpp Versions

- **Minimum**: llama.cpp b1000+ (with LoRA adapter API)
- **Recommended**: Latest stable release
- **Fallback**: Gracefully degrades on older versions

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

### v1.3.3 (2026-05-06)
- `themis_lora_inject_api_functions(init, set, remove, clear, free)` extern "C" API added
- `g_lora_api_override_active` override path bypasses dlsym for test isolation
- Null arguments revert to dlsym-detected path
- Tests LORA-INJ-01..03 added in `tests/llm/test_gpu_lora_integration.cpp`

### v1.3.2 (2026-04-21)
- Path injection fixes F1-1, F1-2, F2-1: trusted-directory validation enforced at call site
- `llama_lora_adapter_set_path` renamed from `llama_lora_adapter_set` (path-only overload) to avoid collision with the integer-handle overload used by `MultiLoRAManager`
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
