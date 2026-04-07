# LLM Plugin Development Guide

**Version:** 1.0.0 (ThemisDB v1.3.0)  
**Date:** December 2025  
**Status:** Implementation Guide

---

## 📋 Overview

This guide explains how to develop LLM plugins for ThemisDB, based on the architecture defined in [AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md).

ThemisDB's plugin-based LLM architecture enables:
- **Multiple LLM backends** (llama.cpp, vLLM, custom implementations)
- **LoRA adapter management** for domain-specific fine-tuning
- **Distributed reasoning** across sharded deployments
- **Zero-copy integration** with ThemisDB's vector storage

---

## 🏗️ Architecture

### Plugin Interface Hierarchy

```
ILLMPlugin (llm/llm_plugin_interface.h)
    ↓
LlamaWrapper (reference implementation)
    ↓
Your Custom Plugin
```

### Key Components

1. **ILLMPlugin** - Base interface all plugins must implement
2. **LLMPluginManager** - Coordinates multiple LLM backends
3. **LlamaWrapper** - Reference implementation using llama.cpp
4. **LLMPluginAdapter** - Bridges to ThemisDB's unified plugin system

---

## 🚀 Quick Start

### 1. Create Your Plugin Class

```cpp
// my_llm_plugin.h
#include "llm/llm_plugin_interface.h"

namespace themis {
namespace llm {

class MyLLMPlugin : public ILLMPlugin {
public:
    MyLLMPlugin();
    ~MyLLMPlugin() override;
    
    // Implement required interface methods
    bool loadModel(const std::string& model_path, const json& config) override;
    void unloadModel() override;
    std::optional<ModelInfo> getModelInfo() const override;
    bool isModelLoaded() const override;
    
    bool loadLoRA(const std::string& lora_id, const std::string& lora_path, float scale) override;
    bool unloadLoRA(const std::string& lora_id) override;
    std::vector<LoRAInfo> listLoRAs() const override;
    
    InferenceResponse generate(const InferenceRequest& request) override;
    InferenceResponse generateRAG(const RAGContext& rag_context, const InferenceRequest& request) override;
    std::vector<float> embed(const std::string& text) override;
    
    LLMCapabilities getCapabilities() const override;
    json getMemoryStats() const override;
    json getPerformanceStats() const override;
    
    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    bool importLoRA(const std::string& lora_id, const std::vector<uint8_t>& data) override;
    
private:
    // Your implementation details
};

} // namespace llm
} // namespace themis
```

### 2. Implement Core Methods

#### Model Loading

```cpp
bool MyLLMPlugin::loadModel(const std::string& model_path, const json& config) {
    // 1. Validate model path
    if (!std::filesystem::exists(model_path)) {
        spdlog::error("Model file not found: {}", model_path);
        return false;
    }
    
    // 2. Load your model (using your backend)
    // Example: my_model_ = your_backend::load_model(model_path);
    
    // 3. Configure GPU offloading if supported
    if (config.contains("n_gpu_layers")) {
        // Configure GPU layers
    }
    
    // 4. Populate ModelInfo
    model_info_.name = "my-model-7b";
    model_info_.path = model_path;
    model_info_.architecture = "llama";
    model_info_.parameter_count = 7000000000;  // 7B
    model_info_.context_length = 4096;
    model_info_.vram_required_mb = 4096;
    
    model_loaded_ = true;
    spdlog::info("Model loaded: {}", model_path);
    return true;
}
```

#### Inference

```cpp
InferenceResponse MyLLMPlugin::generate(const InferenceRequest& request) {
    if (!model_loaded_) {
        throw std::runtime_error("No model loaded");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 1. Tokenize prompt
    // auto tokens = tokenize(request.prompt);
    
    // 2. Apply LoRA if requested
    if (request.lora_adapter_id) {
        // Apply LoRA adapter
    }
    
    // 3. Generate tokens
    // std::string generated_text = your_backend::generate(tokens, request.max_tokens);
    
    // 4. Build response
    InferenceResponse response;
    response.text = generated_text;
    response.model_used = model_info_.name;
    response.tokens_generated = count_tokens(generated_text);
    
    auto end = std::chrono::high_resolution_clock::now();
    response.inference_time_ms = 
        std::chrono::duration<float, std::milli>(end - start).count();
    response.tokens_per_second = 
        response.tokens_generated / (response.inference_time_ms / 1000.0f);
    
    return response;
}
```

#### RAG Integration

```cpp
InferenceResponse MyLLMPlugin::generateRAG(
    const RAGContext& rag_context,
    const InferenceRequest& request
) {
    // 1. Format context with retrieved documents
    std::ostringstream prompt;
    prompt << "Context:\n";
    
    for (const auto& doc : rag_context.documents) {
        prompt << doc.content << "\n\n";
    }
    
    prompt << "Question: " << rag_context.query << "\n";
    prompt << "Answer:";
    
    // 2. Create modified request
    InferenceRequest rag_request = request;
    rag_request.prompt = prompt.str();
    
    // 3. Generate with context
    auto response = generate(rag_request);
    
    // 4. Add RAG metadata
    response.metadata["rag_enabled"] = true;
    response.metadata["num_documents"] = rag_context.documents.size();
    
    return response;
}
```

### 3. Register Your Plugin

```cpp
// In your application startup code
#include "llm/llm_plugin_manager.h"
#include "my_llm_plugin.h"

void initializeLLM() {
    auto plugin = std::make_unique<MyLLMPlugin>();
    
    // Load model
    json config = {
        {"n_gpu_layers", 32},
        {"n_ctx", 4096}
    };
    plugin->loadModel("/path/to/model.gguf", config);
    
    // Register with manager
    LLMPluginManager::instance().registerPlugin("my_llm", std::move(plugin));
}
```

---

## 📚 Interface Reference

### Model Management

#### `loadModel(model_path, config)`
Load a model from disk.

**Parameters:**
- `model_path`: Path to model file (e.g., .gguf, .safetensors)
- `config`: JSON configuration object

**Common config options:**
```json
{
    "n_gpu_layers": 32,        // GPU offload layers
    "n_ctx": 4096,             // Context window size
    "n_batch": 512,            // Batch size
    "n_threads": 8,            // CPU threads
    "max_vram_mb": 14336,      // VRAM limit
    "use_mmap": true           // Memory-map model file
}
```

**Returns:** `true` if successful

---

#### `unloadModel()`
Unload current model and free resources.

---

#### `getModelInfo()`
Get information about the loaded model.

**Returns:** `std::optional<ModelInfo>` containing:
```cpp
struct ModelInfo {
    std::string name;              // "mistral-7b-instruct"
    std::string path;              // "/models/mistral-7b.gguf"
    std::string format;            // "gguf"
    std::string architecture;      // "llama", "mistral", "gpt"
    size_t parameter_count;        // 7000000000 (7B)
    size_t context_length;         // 4096
    size_t vram_required_mb;       // 4096
};
```

---

### LoRA Management

#### `loadLoRA(lora_id, lora_path, scale)`
Load a LoRA adapter.

**Parameters:**
- `lora_id`: Unique identifier for this adapter
- `lora_path`: Path to LoRA weights file
- `scale`: LoRA scaling factor (default: 1.0)

**Example:**
```cpp
plugin->loadLoRA("legal-qa-v1", "/loras/legal-qa.bin", 1.0f);
```

---

#### `unloadLoRA(lora_id)`
Unload a LoRA adapter from memory.

---

#### `listLoRAs()`
List all loaded LoRA adapters.

**Returns:** `std::vector<LoRAInfo>`

---

### Inference

#### `generate(request)`
Generate text from a prompt.

**Request structure:**
```cpp
InferenceRequest request;
request.prompt = "What is ThemisDB?";
request.max_tokens = 512;
request.temperature = 0.7f;
request.top_p = 0.9f;
request.lora_adapter_id = "legal-qa-v1";  // Optional
```

**Response structure:**
```cpp
InferenceResponse {
    std::string text;              // Generated text
    int tokens_generated;          // Number of tokens
    float inference_time_ms;       // Latency
    float tokens_per_second;       // Throughput
    std::string model_used;        // Model name
    std::optional<std::string> lora_used;
};
```

---

#### `generateRAG(rag_context, request)`
Generate with retrieved document context.

**RAG Context:**
```cpp
RAGContext context;
context.query = "What are the legal requirements?";
context.documents = {
    {.content = "Document 1...", .source = "doc1.pdf", .relevance_score = 0.95},
    {.content = "Document 2...", .source = "doc2.pdf", .relevance_score = 0.87}
};
```

---

#### `embed(text)`
Generate vector embedding for text.

**Returns:** `std::vector<float>` (typically 768 or 1024 dimensions)

---

### Capabilities

#### `getCapabilities()`
Report plugin capabilities.

**Example:**
```cpp
LLMCapabilities MyLLMPlugin::getCapabilities() const {
    LLMCapabilities caps;
    caps.supports_instruct = true;
    caps.supports_lora = true;
    caps.supports_streaming = true;
    caps.gpu_accelerated = true;
    caps.supports_cuda = true;
    return caps;
}
```

---

### Statistics

#### `getMemoryStats()`
Get current memory usage.

**Example return:**
```json
{
    "vram_model_mb": 4096,
    "vram_lora_mb": 128,
    "vram_total_mb": 4224,
    "lora_cache_size": 3
}
```

---

#### `getPerformanceStats()`
Get performance metrics.

**Example return:**
```json
{
    "total_inferences": 1250,
    "total_tokens_generated": 125000,
    "avg_inference_time_ms": 287.5,
    "avg_tokens_per_inference": 100,
    "cache_hit_rate": 0.73
}
```

---

## 🔧 Advanced Features

### Distributed LoRA Transfer

For multi-shard deployments (see [AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md)):

```cpp
// Export LoRA for transfer to another shard
std::vector<uint8_t> MyLLMPlugin::exportLoRA(const std::string& lora_id) {
    // Serialize LoRA weights to binary format
    // Include metadata: version, checksum, etc.
    std::vector<uint8_t> data;
    // ... serialize your LoRA ...
    return data;
}

// Import LoRA from another shard
bool MyLLMPlugin::importLoRA(
    const std::string& lora_id,
    const std::vector<uint8_t>& data
) {
    // Deserialize and load LoRA weights
    // Verify checksum
    // Load into memory
    return true;
}
```

---

### Streaming Generation

For token-by-token streaming:

```cpp
InferenceRequest request;
request.prompt = "Tell me a story";
request.stream_callback = [](const std::string& token) {
    std::cout << token << std::flush;
};

plugin->generate(request);
```

---

### Zero-Copy Integration

For maximum performance with GPU-resident vector data:

```cpp
// If your plugin supports zero-copy (CUDA Unified Memory)
LLMCapabilities caps = plugin->getCapabilities();
if (caps.supports_zero_copy) {
    // Pass GPU pointers directly to plugin
    // No CPU-GPU copies needed
}
```

---

## 📦 Packaging Your Plugin

### Plugin Manifest (plugin.json)

```json
{
    "name": "my-llm-plugin",
    "version": "1.0.0",
    "description": "My custom LLM backend for ThemisDB",
    "type": "LLM",
    "author": "Your Name",
    
    "binary_linux": "libthemis_llm_myplugin.so",
    "binary_windows": "themis_llm_myplugin.dll",
    "binary_macos": "libthemis_llm_myplugin.dylib",
    
    "capabilities": {
        "supports_lora": true,
        "supports_streaming": true,
        "gpu_accelerated": true
    },
    
    "config_schema": {
        "type": "object",
        "properties": {
            "model_path": {"type": "string"},
            "n_gpu_layers": {"type": "integer", "default": 32},
            "n_ctx": {"type": "integer", "default": 4096}
        },
        "required": ["model_path"]
    }
}
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(themis_llm_myplugin)

add_library(themis_llm_myplugin SHARED
    my_llm_plugin.cpp
)

target_link_libraries(themis_llm_myplugin PRIVATE
    themis_core
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    # Your backend dependencies
)

target_include_directories(themis_llm_myplugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

install(TARGETS themis_llm_myplugin
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION bin/themis/plugins
)
```

---

## 🧪 Testing Your Plugin

### Unit Tests

```cpp
#include <gtest/gtest.h>
#include "my_llm_plugin.h"

TEST(MyLLMPluginTest, LoadModel) {
    MyLLMPlugin plugin;
    
    bool loaded = plugin.loadModel("/path/to/test/model.gguf");
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(plugin.isModelLoaded());
    
    auto info = plugin.getModelInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->format, "gguf");
}

TEST(MyLLMPluginTest, BasicInference) {
    MyLLMPlugin plugin;
    plugin.loadModel("/path/to/test/model.gguf");
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    request.max_tokens = 10;
    
    auto response = plugin.generate(request);
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
}
```

---

## 📖 Examples

See:
- **Reference Implementation:** `src/llm/llama_wrapper.cpp`
- **Architecture Design:** `docs/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md`
- **LLM Integration Guide:** `docs/llm/NATIVE_LLM_INTEGRATION_CONCEPT.md`

---

## 🆘 Support

For questions or issues:
1. Check the [LLM Documentation](./README.md)
2. Review the [Architecture Document](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md)
3. Open an issue on GitHub

---

**Last Updated:** April 2026  
**ThemisDB Version:** v1.3.0  
**Status:** Active Development
