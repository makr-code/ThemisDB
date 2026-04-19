# LoRa Adapter Application Guide

**Status**: ✅ Implemented  
**Version**: 1.3.0+  
**Priority**: P0 - Critical  
**Related**: implementation-history/REMAINING_GAPS_SUMMARY.md §3 (Historical)

---

## Overview

This document describes how to apply LoRa (Low-Rank Adaptation) adapters to loaded models in ThemisDB's LLM inference engine. LoRa adapters allow fine-tuning of large language models efficiently by applying learned weight modifications without changing the base model.

## Architecture

### Components

1. **LlamaCppInferenceEngine**: Core inference engine that manages model loading and adapter application
2. **LoRAStorageService**: Backend service for storing and retrieving adapter weights
3. **Adapter Tracking**: Internal system for managing multiple active adapters
4. **Format Conversion**: Automatic conversion between adapter formats (safetensors, GGUF)

### Integration with llama.cpp

The implementation provides a framework ready for llama.cpp integration. The following llama.cpp functions are used (when available):

```cpp
// Initialize adapter from file
int llama_lora_adapter_init(llama_model* model, const char* path);

// Apply adapter to context with scaling
int llama_lora_adapter_set(llama_context* ctx, int adapter_id, float scale);

// Remove adapter from context
int llama_lora_adapter_remove(int adapter_id);

// Clear all adapters from context
void llama_lora_adapter_clear(llama_context* ctx);
```

## Usage

### Basic Adapter Application

```cpp
#include "llm/llamacpp_inference_engine.h"

using namespace themis::llm;

// Configure inference engine
LlamaCppInferenceEngine::Config config;
config.n_ctx = 4096;
config.n_threads = 4;
config.lora_storage = lora_storage_service;  // Pre-configured storage

LlamaCppInferenceEngine engine(config);

// Load base model
engine.loadModel("models/mistral-7b.gguf", "mistral-7b");

// Load and apply a LoRa adapter with default scale (1.0)
bool success = engine.loadAndApplyLoRAAdapter("legal-qa-adapter-v1");

if (success) {
    std::cout << "Adapter applied successfully!" << std::endl;
}

// Run inference (now uses the adapted model)
InferenceRequest request;
request.prompt = "Explain contract law basics";
InferenceResponse response = engine.infer(request);
```

### Adapter Scaling

Adapters can be applied with different scaling factors to control their influence:

```cpp
// Apply adapter with custom scale
// scale = 0.0  → No effect (equivalent to base model)
// scale = 0.5  → Half strength
// scale = 1.0  → Full strength (default)
// scale = 2.0  → Double strength

engine.loadAndApplyLoRAAdapter("medical-adapter", 0.7f);  // 70% strength
```

### Multiple Adapters

Apply multiple adapters simultaneously:

```cpp
std::vector<std::pair<std::string, float>> adapters = {
    {"legal-qa-adapter", 1.0f},
    {"formal-writing-adapter", 0.5f},
    {"technical-terms-adapter", 0.3f}
};

bool all_applied = engine.applyMultipleAdapters(adapters);

if (all_applied) {
    std::cout << "All adapters applied!" << std::endl;
}
```

### Managing Active Adapters

```cpp
// Check if adapter is active
if (engine.isAdapterActive("legal-qa-adapter")) {
    std::cout << "Adapter is active" << std::endl;
}

// Get list of all active adapters
auto active = engine.getActiveAdapters();
for (const auto& adapter_id : active) {
    std::cout << "Active: " << adapter_id << std::endl;
}

// Remove specific adapter
engine.removeAdapter("legal-qa-adapter");

// Clear all adapters
engine.clearAllAdapters();
```

### Validation

Validate that an adapter is correctly applied:

```cpp
bool valid = engine.validateAdapterApplication("legal-qa-adapter");

if (!valid) {
    std::cerr << "Adapter validation failed!" << std::endl;
}
```

### Loading from ThemisDB Storage

```cpp
// Load adapter directly from ThemisDB storage backend
bool loaded = engine.loadAdapterFromThemisDB("adapter-123");

if (loaded) {
    // Adapter is now active
    assert(engine.isAdapterActive("adapter-123"));
}
```

## Format Support

### Supported Formats

| Format       | Status | Notes |
|-------------|--------|-------|
| GGUF        | ✅ Native | llama.cpp native format |
| safetensors | ✅ Converted | Automatically converted to GGUF |
| PyTorch     | ⚠️ Future | Requires additional conversion |
| pickle      | ⚠️ Future | Requires additional conversion |

### Automatic Conversion

The engine automatically handles format conversion:

```cpp
// Adapter stored in safetensors format
auto weights = storage->loadAdapter("my-adapter");
// weights.format == "safetensors"

// Engine automatically converts to GGUF for llama.cpp
engine.loadAndApplyLoRAAdapter("my-adapter");  // Works seamlessly
```

## Best Practices

### 1. Adapter Lifecycle Management

```cpp
// Load model once
engine.loadModel(model_path, model_name);

// Apply adapters for specific tasks
engine.loadAndApplyLoRAAdapter("task-specific-adapter");

// Run inference
auto response = engine.infer(request);

// Clean up when done
engine.removeAdapter("task-specific-adapter");
```

### 2. Error Handling

```cpp
try {
    if (!engine.loadAndApplyLoRAAdapter("my-adapter", 1.0f)) {
        spdlog::error("Failed to load adapter");
        // Handle error (adapter not found, format error, etc.)
    }
} catch (const std::exception& e) {
    spdlog::error("Exception: {}", e.what());
}
```

### 3. Resource Cleanup

Adapters are automatically cleaned up when:
- The model is unloaded: `engine.unloadModel()`
- The engine is destroyed: `~LlamaCppInferenceEngine()`
- Adapters are explicitly removed: `engine.removeAdapter()` or `clearAllAdapters()`

Temporary files are automatically deleted.

### 4. Performance Considerations

```cpp
// ✅ Good: Apply multiple adapters at once
engine.applyMultipleAdapters({
    {"adapter1", 1.0f},
    {"adapter2", 0.5f}
});

// ❌ Avoid: Repeatedly loading/unloading adapters
for (int i = 0; i < 100; i++) {
    engine.loadAndApplyLoRAAdapter("adapter1");  // Redundant
    engine.removeAdapter("adapter1");
}
```

## Advanced Usage

### Custom Adapter Scales for Domain Mixing

```cpp
// Mix multiple domain adapters with different strengths
std::vector<std::pair<std::string, float>> adapters = {
    {"legal-terms", 0.8f},        // Strong legal terminology
    {"business-writing", 0.5f},   // Moderate business style
    {"formal-tone", 0.3f}         // Light formal tone
};

engine.applyMultipleAdapters(adapters);

// Generate with mixed domain knowledge
InferenceRequest request;
request.prompt = "Draft a business contract for software licensing";
auto response = engine.infer(request);
```

### Adapter Swapping for Different Tasks

```cpp
void processRequest(const std::string& task_type, const std::string& prompt) {
    // Clear previous adapters
    engine.clearAllAdapters();
    
    // Apply task-specific adapter
    if (task_type == "legal") {
        engine.loadAndApplyLoRAAdapter("legal-qa");
    } else if (task_type == "medical") {
        engine.loadAndApplyLoRAAdapter("medical-diagnosis");
    } else if (task_type == "code") {
        engine.loadAndApplyLoRAAdapter("code-assistant");
    }
    
    // Run inference
    InferenceRequest request;
    request.prompt = prompt;
    auto response = engine.infer(request);
}
```

## Troubleshooting

### Adapter Not Found

```
Error: Failed to load adapter: adapter-123
```

**Solution**: Verify adapter exists in storage:
```cpp
if (lora_storage->exists("adapter-123")) {
    // Adapter exists, other issue
} else {
    // Adapter doesn't exist in storage
}
```

### Format Conversion Failed

```
Error: Failed to convert adapter to llama.cpp format
```

**Solution**: Check adapter format:
```cpp
auto weights = lora_storage->loadAdapter("adapter-123");
std::cout << "Format: " << weights->format << std::endl;
// Ensure format is supported (gguf or safetensors)
```

### Adapter Not Active After Loading

```
Warning: Adapter loaded but not active
```

**Solution**: Ensure model is loaded first:
```cpp
// ✅ Correct order
engine.loadModel(model_path, model_name);
engine.loadAndApplyLoRAAdapter("adapter-123");

// ❌ Wrong order
engine.loadAndApplyLoRAAdapter("adapter-123");  // Fails - no model loaded
engine.loadModel(model_path, model_name);
```

### Memory Issues with Multiple Adapters

```
Error: Failed to allocate memory for adapter
```

**Solution**: Limit number of concurrent adapters:
```cpp
// Remove old adapters before adding new ones
engine.clearAllAdapters();
engine.loadAndApplyLoRAAdapter("new-adapter");
```

## API Reference

### Core Methods

#### `loadAndApplyLoRAAdapter`

```cpp
bool loadAndApplyLoRAAdapter(
    const std::string& adapter_id,
    float scale = 1.0f
);
```

Load adapter from storage and apply to model.

**Parameters**:
- `adapter_id`: Unique identifier of the adapter
- `scale`: Scaling factor (0.0 to 2.0+), default 1.0

**Returns**: `true` if successful, `false` otherwise

---

#### `applyMultipleAdapters`

```cpp
bool applyMultipleAdapters(
    const std::vector<std::pair<std::string, float>>& adapters
);
```

Apply multiple adapters simultaneously.

**Parameters**:
- `adapters`: Vector of (adapter_id, scale) pairs

**Returns**: `true` if all adapters applied successfully

---

#### `removeAdapter`

```cpp
bool removeAdapter(const std::string& adapter_id);
```

Remove specific adapter from model.

**Parameters**:
- `adapter_id`: Identifier of adapter to remove

**Returns**: `true` if removed, `false` if not active

---

#### `clearAllAdapters`

```cpp
void clearAllAdapters();
```

Remove all active adapters from model.

---

#### `isAdapterActive`

```cpp
bool isAdapterActive(const std::string& adapter_id) const;
```

Check if adapter is currently active.

**Parameters**:
- `adapter_id`: Identifier of adapter to check

**Returns**: `true` if active, `false` otherwise

---

#### `getActiveAdapters`

```cpp
std::vector<std::string> getActiveAdapters() const;
```

Get list of all currently active adapters.

**Returns**: Vector of adapter identifiers

---

#### `validateAdapterApplication`

```cpp
bool validateAdapterApplication(const std::string& adapter_id);
```

Validate that adapter is correctly applied.

**Parameters**:
- `adapter_id`: Identifier of adapter to validate

**Returns**: `true` if validation passes

---

#### `loadAdapterFromThemisDB`

```cpp
bool loadAdapterFromThemisDB(const std::string& adapter_id);
```

Load adapter directly from ThemisDB storage (convenience method).

**Parameters**:
- `adapter_id`: Identifier of adapter in ThemisDB

**Returns**: `true` if loaded and applied successfully

## Testing

### Running Tests

```bash
# Build test executable
cmake --build . --target test_lora_adapter_application

# Run tests
./test_lora_adapter_application

# Run specific test
./test_lora_adapter_application --gtest_filter=LoRAAdapterApplicationTest.LoadAndApplySingleAdapter
```

### Test Coverage

The test suite includes:
- ✅ Basic adapter loading (single adapter)
- ✅ Multiple adapter management
- ✅ Adapter scaling (different scale values)
- ✅ Format conversion (GGUF, safetensors)
- ✅ Adapter removal and cleanup
- ✅ Error handling (non-existent adapters, no model loaded)
- ✅ Resource cleanup (temp files, memory)
- ✅ Integration with storage backend
- ✅ Complete workflow testing

## Security Considerations

### 1. Adapter Validation

Always validate adapters before applying:

```cpp
// Verify adapter metadata
auto metadata = lora_storage->loadMetadata(adapter_id);
if (metadata) {
    // Check base model compatibility
    if (metadata->base_model != current_model_name) {
        std::cerr << "Adapter not compatible with current model!" << std::endl;
        return false;
    }
}
```

### 2. Secure Storage

Adapters should be stored securely:
- Enable encryption: `storage_config.enable_encryption = true`
- Use HSM/Vault for key management
- Verify digital signatures

### 3. Resource Limits

Set limits on adapter memory usage:

```cpp
// Limit number of concurrent adapters
const size_t MAX_ADAPTERS = 5;

if (engine.getActiveAdapters().size() >= MAX_ADAPTERS) {
    std::cerr << "Too many active adapters!" << std::endl;
    engine.clearAllAdapters();  // Clean up before adding new ones
}
```

## Future Enhancements

### Planned Features

1. **Adapter Composition**: Merge multiple adapters into a single adapter
2. **Adapter Hot-Swapping**: Change adapters without reloading model
3. **Adapter Caching**: Keep frequently-used adapters in memory
4. **Adapter Versioning**: Support multiple versions of same adapter
5. **Adapter Metrics**: Track adapter usage and performance

### Roadmap

| Feature | Version | Status |
|---------|---------|--------|
| Basic Adapter Application | 1.3.0 | ✅ Complete |
| Multiple Adapter Support | 1.3.0 | ✅ Complete |
| Format Conversion | 1.3.0 | ✅ Complete |
| Adapter Composition | 1.4.0 | 📋 Planned |
| Hot-Swapping | 1.4.0 | 📋 Planned |
| Adapter Caching | 1.5.0 | 📋 Planned |

## Related Documentation

- [LoRa Storage Backend](LORA_STORAGE_BACKEND_COMPLETION.md)
- [LoRa Training Guide](../../llm_orchestration/lora_production_training.md)
- [LLM Plugin Interface](../include/llm/llm_plugin_interface.h)
- [REMAINING_GAPS_SUMMARY.md](../implementation-history/REMAINING_GAPS_SUMMARY.md) §3 (Historical)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: `/docs/`
- Examples: `/examples/`

---

**Last Updated**: 2026-04-06  
**Author**: ThemisDB Development Team  
**License**: MIT
