> **Status:** 2026-04-19 – Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos ggf. korrigiert.

# LoRA Adapter Implementation for ThemisDB

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

Maintains compatibility with existing code through overloaded signatures:

```cpp
// Legacy signature (compatibility with old stub)
int llama_lora_adapter_set(llama_context* ctx, const char* adapter_path);

// MultiLoRAManager signature (integer handle)
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

## Usage

### From MultiLoRAManager

```cpp
// Load adapter
bool success = lora_manager->loadLoRA(
    "adapter_id",
    "/path/to/adapter.bin",
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
    // Load adapter
    void* adapter = llama_lora_adapter_init(model, "/path/to/adapter.bin");
    
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

1. **Path Validation**: Adapter paths are validated before loading
2. **Null Checks**: All pointers checked before dereferencing
3. **Error Propagation**: Errors from llama.cpp properly propagated
4. **Memory Safety**: No manual memory management of adapter data

## Future Enhancements

The following items listed below have been implemented since v1.3.1:

1. **Adapter Verification**: Cryptographic verification of adapter files — ✅ Implemented in `lora_security_validator.cpp` (SHA-256 + trusted manifest, see `src/llm/lora_security_validator.cpp`)
2. **Hot-Reloading**: Dynamic adapter updates without context recreation — ✅ Implemented via `AdapterRegistry::hotLoad()` (see `src/llm/adapter_registry.cpp` and `include/llm/adapter_registry.h`)
3. **Multi-Adapter Composition**: Simultaneous application of multiple adapters — ✅ Implemented in `multi_lora_manager.cpp` (vLLM-style multi-LoRA support)
4. **Quantization**: Support for quantized adapter formats — <!-- TODO: verify --> partial; quantized base models are supported via `model_quantization_pipeline.cpp`; quantized adapter weights format support is not yet fully implemented

## Related Components

- `multi_lora_manager.cpp`: High-level LoRA management (vLLM-style)
- `llama_wrapper.cpp`: llama.cpp integration layer
- `lora_framework/`: LoRA training and storage infrastructure

## References

- [llama.cpp LoRA Documentation](https://github.com/ggerganov/llama.cpp)
- [ThemisDB LoRA Integration Guide](../../docs/en/llm/LLM_LORA_LLAMACPP_INTEGRATION.md)
- [LoRA Adapter Application Guide](../../docs/en/lora/LORA_ADAPTER_APPLICATION_GUIDE.md)

## Changelog

### v1.3.1 (2026-01-26)
- Initial real implementation replacing stub
- Dynamic API detection and loading
- Full LoRA lifecycle support
- Cross-platform compatibility
- Backward compatible overloads

### Previous (v1.3.0)
- Stub implementation returning -1
- Prevented real LoRA functionality
