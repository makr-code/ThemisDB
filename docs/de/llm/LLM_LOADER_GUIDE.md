# vLLM-ähnlicher LLM Loader für ThemisDB - Implementierungsleitfaden

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** LLM Infrastructure  
**Sprache:** Deutsch

---

## 📋 Executive Summary

Dieser Leitfaden beschreibt, was benötigt wird, um einen **vLLM-ähnlichen LLM Loader** für ThemisDB zu implementieren. Ein solcher Loader ermöglicht das effiziente Laden, Verwalten und Serving von Large Language Models direkt aus ThemisDB heraus, ähnlich wie vLLM es für generische LLM-Inferenz macht.

**Kernziele:**
- Effizientes Laden von LLM-Modellen und LoRA-Adaptern
- Integration mit ThemisDB's Speicher- und Caching-Infrastruktur
- Unterstützung für Multi-LoRA Inferenz
- GPU-beschleunigtes Model Serving
- Nahtlose Integration mit bestehender ThemisDB-Architektur

---

## 🏗️ Architektur-Übersicht

### High-Level Architektur

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        ThemisDB LLM Loader System                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                    ThemisDB Core APIs                              │    │
│  │  /api/llm/load_model  /api/llm/inference  /api/llm/adapters       │    │
│  └─────────────────────────────┬──────────────────────────────────────┘    │
│                                │                                             │
│  ┌─────────────────────────────▼──────────────────────────────────────┐    │
│  │                      LLM Loader Manager                            │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐     │    │
│  │  │ Model Cache  │  │ LoRA Manager │  │ Inference Scheduler  │     │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────┘     │    │
│  └─────────────────────────────┬──────────────────────────────────────┘    │
│                                │                                             │
│  ┌─────────────────────────────▼──────────────────────────────────────┐    │
│  │                    Backend Abstraction Layer                       │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐     │    │
│  │  │   llama.cpp  │  │    GGML      │  │   Transformers       │     │    │
│  │  │   Backend    │  │   Backend    │  │   Backend            │     │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────┘     │    │
│  └─────────────────────────────┬──────────────────────────────────────┘    │
│                                │                                             │
│  ┌─────────────────────────────▼──────────────────────────────────────┐    │
│  │                    ThemisDB Storage Integration                     │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐     │    │
│  │  │ Model Blobs  │  │ Adapter Meta │  │ Inference Cache      │     │    │
│  │  │ (RocksDB)    │  │ (Entities)   │  │ (Semantic Cache)     │     │    │
│  │  └──────────────┘  └──────────────┘  └──────────────────────┘     │    │
│  └──────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Komponenten-Vergleich: vLLM vs. ThemisDB LLM Loader

| Komponente | vLLM | ThemisDB LLM Loader |
|------------|------|---------------------|
| **Model Storage** | Filesystem (HuggingFace Cache) | ThemisDB RocksDB + Entities |
| **Inference Engine** | vLLM Custom (PagedAttention) | llama.cpp / GGML / Transformers |
| **GPU Backend** | CUDA/ROCm | ThemisDB Acceleration (CUDA/Vulkan/HIP/etc.) |
| **Memory Management** | PagedAttention KV Cache | ThemisDB mimalloc + RocksDB Block Cache |
| **Batching** | Continuous Batching | Request Queue + TBB Thread Pool |
| **LoRA Support** | Native Multi-LoRA | Adapter Registry + Dynamic Loading |
| **API** | OpenAI-compatible | ThemisDB REST/GraphQL + Custom |
| **Caching** | KV Cache only | Semantic Cache + Embedding Cache |

---

## 📚 Benötigte Bibliotheken

### 1. Inference Engines (Backend Options)

#### Option A: llama.cpp (Empfohlen für C++ Integration)
```cmake
# CMakeLists.txt
FetchContent_Declare(
  llama_cpp
  GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
  GIT_TAG        b1698  # Stable release
)
```

**Vorteile:**
- ✅ Native C/C++ implementation
- ✅ Sehr effizient (GGML quantization)
- ✅ Multi-Backend (CPU, CUDA, Metal, Vulkan, etc.)
- ✅ Kleine Memory-Footprint
- ✅ Aktive Community
- ✅ Unterstützt GGUF format (Llama, Mistral, etc.)

**Integration:**
```cpp
#include "llama.h"

class LlamaCppBackend {
public:
    void loadModel(const std::string& model_path) {
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = 32;  // GPU acceleration
        
        model_ = llama_load_model_from_file(model_path.c_str(), model_params);
        
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 4096;
        ctx_params.n_batch = 512;
        
        ctx_ = llama_new_context_with_model(model_, ctx_params);
    }
    
    std::string inference(const std::string& prompt, int max_tokens = 512) {
        // Tokenize
        std::vector<llama_token> tokens = tokenize(prompt);
        
        // Generate
        std::string result;
        for (int i = 0; i < max_tokens; ++i) {
            llama_token new_token = llama_sample_token(ctx_, tokens);
            if (new_token == llama_token_eos(model_)) break;
            
            result += llama_token_to_piece(ctx_, new_token);
            tokens.push_back(new_token);
        }
        
        return result;
    }
    
private:
    llama_model* model_ = nullptr;
    llama_context* ctx_ = nullptr;
};
```

#### Option B: GGML (Low-Level)
```cmake
FetchContent_Declare(
  ggml
  GIT_REPOSITORY https://github.com/ggerganov/ggml.git
  GIT_TAG        master
)
```

**Vorteile:**
- ✅ Maximale Kontrolle über Inferenz
- ✅ Sehr schnell und effizient
- ✅ Unterstützt Custom Models

**Nachteile:**
- ❌ Mehr Boilerplate Code
- ❌ Weniger high-level APIs

#### Option C: Transformers C++ (Experimental)
```cmake
# Verwendet ONNX Runtime + Transformers models
find_package(onnxruntime REQUIRED)
```

**Vorteile:**
- ✅ ONNX Standard
- ✅ Breite Model-Unterstützung
- ✅ Optimized für verschiedene Backends

**Nachteile:**
- ❌ Größere Dependencies
- ❌ Weniger optimiert als llama.cpp für LLMs

### 2. GPU Acceleration Libraries

ThemisDB hat bereits Multi-Backend GPU Support. Diese können genutzt werden:

```cpp
// Bestehende ThemisDB Acceleration Backends
#include "acceleration/compute_backend.h"
#include "acceleration/plugin_loader.h"

namespace themis {
namespace llm {

class LLMGPUBackend {
public:
    LLMGPUBackend(acceleration::BackendType backend_type) {
        // Nutze bestehende ThemisDB GPU Infrastructure
        backend_ = acceleration::createBackend(backend_type);
    }
    
    void offloadLayersToGPU(int num_layers) {
        // GPU layer offloading für LLM
    }
    
private:
    std::unique_ptr<acceleration::IVectorBackend> backend_;
};

} // namespace llm
} // namespace themis
```

**Verfügbare Backends in ThemisDB:**
1. **CUDA** - NVIDIA GPUs (beste Performance)
2. **Vulkan** - Cross-platform (Windows, Linux, macOS)
3. **HIP** - AMD GPUs
4. **Metal** - Apple Silicon
5. **OpenCL** - Legacy GPU support
6. **DirectX** - Windows DirectML
7. **OneAPI** - Intel GPUs
8. **ZLUDA** - CUDA emulation auf AMD

### 3. Model Format Libraries

#### GGUF/GGML Format Support
```cpp
// llama.cpp provides GGUF support out of the box
#include "ggml.h"
#include "llama.h"

// Models: Llama-2, Mistral, Mixtral, etc. in GGUF format
```

#### HuggingFace Safetensors
```cmake
# Safetensors C++ library
FetchContent_Declare(
  safetensors_cpp
  GIT_REPOSITORY https://github.com/hzhou/safetensors_cpp.git
)
```

```cpp
#include "safetensors.hpp"

class SafetensorsLoader {
public:
    void loadModel(const std::string& model_path) {
        auto tensors = safetensors::load(model_path);
        // Convert to llama.cpp or GGML format
    }
};
```

### 4. LoRA Adapter Support

#### PEFT (Python - for training)
```python
# Training side (creates LoRA adapters)
from peft import LoraConfig, get_peft_model, TaskType

lora_config = LoraConfig(
    task_type=TaskType.CAUSAL_LM,
    r=8,  # LoRA rank
    lora_alpha=16,
    lora_dropout=0.1,
    target_modules=["q_proj", "v_proj"]
)
```

#### llama.cpp LoRA Support
```cpp
// llama.cpp supports LoRA adapters natively
llama_model_apply_lora_from_file(
    model_,
    "adapters/legal-qa-v1/adapter_model.bin",
    1.0f  // scaling factor
);
```

### 5. Tokenization Libraries

#### SentencePiece (Llama, Mistral)
```cmake
find_package(sentencepiece REQUIRED)
```

```cpp
#include <sentencepiece_processor.h>

class SPTokenizer {
public:
    void load(const std::string& model_path) {
        processor_.Load(model_path);
    }
    
    std::vector<int> encode(const std::string& text) {
        std::vector<int> ids;
        processor_.Encode(text, &ids);
        return ids;
    }
    
    std::string decode(const std::vector<int>& ids) {
        std::string text;
        processor_.Decode(ids, &text);
        return text;
    }
    
private:
    sentencepiece::SentencePieceProcessor processor_;
};
```

#### Tiktoken (GPT models) - Python binding needed
```python
import tiktoken

# For integration with C++, use pybind11
```

### 6. Networking & API Libraries (Already in ThemisDB)

```cpp
// ThemisDB already has these
#include <crow/crow.h>           // HTTP server
#include <nlohmann/json.hpp>     // JSON handling
#include <grpc/grpc.h>           // gRPC (optional)
```

### 7. Memory Management

```cpp
// ThemisDB bereits nutzt:
#include <mimalloc.h>  // Efficient memory allocator

// Für Tensor Memory:
#include <ggml.h>      // ggml_context für Tensor Management
```

### 8. Concurrency & Threading

```cpp
// ThemisDB bereits nutzt:
#include <tbb/tbb.h>              // Intel TBB
#include <tbb/concurrent_queue.h>  // For request batching
```

---

## 🔧 Implementierung: ThemisDB LLM Loader

### 1. Model Loader Klasse

```cpp
// include/llm/llm_model_loader.h
#pragma once

#include <string>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "llama.h"
#include "storage/storage_engine.h"

namespace themis {
namespace llm {

struct ModelConfig {
    std::string model_id;
    std::string model_path;          // Path in ThemisDB storage
    std::string model_format;        // "gguf", "safetensors", etc.
    int n_ctx = 4096;                // Context length
    int n_gpu_layers = 32;           // GPU offload layers
    int n_batch = 512;               // Batch size
    float rope_freq_base = 10000.0f; // RoPE base frequency
    bool use_mmap = true;            // Memory-map model file
    bool use_mlock = false;          // Lock model in RAM
    nlohmann::json metadata;         // Additional metadata
};

struct InferenceRequest {
    std::string prompt;
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    float top_k = 40.0f;
    std::vector<std::string> stop_sequences;
    std::optional<std::string> lora_adapter_id;  // Optional LoRA
    nlohmann::json metadata;
};

struct InferenceResult {
    std::string generated_text;
    int tokens_generated;
    int prompt_tokens;
    float inference_time_ms;
    nlohmann::json metadata;
};

class LLMModelLoader {
public:
    explicit LLMModelLoader(storage::StorageEngine* storage);
    ~LLMModelLoader();
    
    // Model Management
    bool loadModel(const ModelConfig& config);
    bool unloadModel(const std::string& model_id);
    bool isModelLoaded(const std::string& model_id) const;
    std::vector<std::string> getLoadedModels() const;
    
    // LoRA Adapter Management
    bool loadLoRAAdapter(const std::string& model_id, 
                        const std::string& adapter_id,
                        const std::string& adapter_path,
                        float scaling = 1.0f);
    bool unloadLoRAAdapter(const std::string& model_id,
                          const std::string& adapter_id);
    std::vector<std::string> getLoadedAdapters(const std::string& model_id) const;
    
    // Inference
    InferenceResult inference(const std::string& model_id,
                            const InferenceRequest& request);
    
    // Async Inference (returns request_id)
    std::string inferenceAsync(const std::string& model_id,
                              const InferenceRequest& request,
                              std::function<void(InferenceResult)> callback);
    
    // Model Storage Integration
    bool storeModelInThemisDB(const std::string& model_id,
                             const std::string& local_path);
    bool loadModelFromThemisDB(const std::string& model_id,
                              const std::string& local_cache_path);
    
    // Statistics
    nlohmann::json getModelStats(const std::string& model_id) const;
    nlohmann::json getAllStats() const;
    
private:
    struct ModelInstance {
        std::string model_id;
        llama_model* model = nullptr;
        llama_context* context = nullptr;
        ModelConfig config;
        std::map<std::string, float> lora_adapters;  // adapter_id -> scaling
        std::mutex mutex;
        
        // Statistics
        size_t total_requests = 0;
        size_t total_tokens_generated = 0;
        double total_inference_time_ms = 0.0;
    };
    
    storage::StorageEngine* storage_;
    std::map<std::string, std::unique_ptr<ModelInstance>> models_;
    mutable std::shared_mutex models_mutex_;
    
    // Request queue for batching
    tbb::concurrent_queue<std::pair<std::string, InferenceRequest>> request_queue_;
    
    // Helper methods
    llama_model* loadLlamaCppModel(const ModelConfig& config);
    std::vector<llama_token> tokenize(llama_context* ctx, 
                                      const std::string& text);
    std::string detokenize(llama_context* ctx,
                          const std::vector<llama_token>& tokens);
};

} // namespace llm
} // namespace themis
```

### 2. Implementation (src/llm/llm_model_loader.cpp)

```cpp
#include "llm/llm_model_loader.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace themis {
namespace llm {

LLMModelLoader::LLMModelLoader(storage::StorageEngine* storage)
    : storage_(storage) {
    llama_backend_init(false);  // Initialize llama.cpp backend
}

LLMModelLoader::~LLMModelLoader() {
    // Cleanup all loaded models
    std::unique_lock lock(models_mutex_);
    for (auto& [model_id, instance] : models_) {
        if (instance->context) {
            llama_free(instance->context);
        }
        if (instance->model) {
            llama_free_model(instance->model);
        }
    }
    llama_backend_free();
}

bool LLMModelLoader::loadModel(const ModelConfig& config) {
    std::unique_lock lock(models_mutex_);
    
    if (models_.find(config.model_id) != models_.end()) {
        LOG_WARNING << "Model already loaded: " << config.model_id;
        return false;
    }
    
    // Create model instance
    auto instance = std::make_unique<ModelInstance>();
    instance->model_id = config.model_id;
    instance->config = config;
    
    // Load llama.cpp model
    instance->model = loadLlamaCppModel(config);
    if (!instance->model) {
        LOG_ERROR << "Failed to load model: " << config.model_id;
        return false;
    }
    
    // Create context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config.n_ctx;
    ctx_params.n_batch = config.n_batch;
    ctx_params.n_threads = std::thread::hardware_concurrency();
    
    instance->context = llama_new_context_with_model(instance->model, ctx_params);
    if (!instance->context) {
        LOG_ERROR << "Failed to create context for model: " << config.model_id;
        llama_free_model(instance->model);
        return false;
    }
    
    models_[config.model_id] = std::move(instance);
    
    LOG_INFO << "Model loaded successfully: " << config.model_id;
    return true;
}

InferenceResult LLMModelLoader::inference(const std::string& model_id,
                                         const InferenceRequest& request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get model instance
    std::shared_lock lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        throw std::runtime_error("Model not loaded: " + model_id);
    }
    
    auto& instance = it->second;
    std::lock_guard model_lock(instance->mutex);
    
    // Apply LoRA adapter if specified
    if (request.lora_adapter_id.has_value()) {
        auto lora_it = instance->lora_adapters.find(*request.lora_adapter_id);
        if (lora_it == instance->lora_adapters.end()) {
            throw std::runtime_error("LoRA adapter not loaded: " + *request.lora_adapter_id);
        }
        // LoRA already applied during loadLoRAAdapter()
    }
    
    // Tokenize prompt
    auto tokens = tokenize(instance->context, request.prompt);
    int prompt_tokens = tokens.size();
    
    // Run inference
    std::vector<llama_token> generated_tokens;
    
    // Evaluate prompt
    llama_eval(instance->context, tokens.data(), tokens.size(), 0);
    
    // Generate tokens
    for (int i = 0; i < request.max_tokens; ++i) {
        // Sample next token
        llama_token new_token = llama_sample_token_greedy(
            instance->context,
            nullptr  // Use default sampling
        );
        
        // Check for EOS
        if (new_token == llama_token_eos(instance->model)) {
            break;
        }
        
        generated_tokens.push_back(new_token);
        
        // Evaluate new token
        llama_eval(instance->context, &new_token, 1, tokens.size() + i);
        
        // Check stop sequences
        std::string current_text = detokenize(instance->context, generated_tokens);
        for (const auto& stop : request.stop_sequences) {
            if (current_text.find(stop) != std::string::npos) {
                goto generation_done;
            }
        }
    }
    
generation_done:
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    // Update statistics
    instance->total_requests++;
    instance->total_tokens_generated += generated_tokens.size();
    instance->total_inference_time_ms += duration.count();
    
    // Build result
    InferenceResult result;
    result.generated_text = detokenize(instance->context, generated_tokens);
    result.tokens_generated = generated_tokens.size();
    result.prompt_tokens = prompt_tokens;
    result.inference_time_ms = duration.count();
    result.metadata["model_id"] = model_id;
    if (request.lora_adapter_id.has_value()) {
        result.metadata["lora_adapter"] = *request.lora_adapter_id;
    }
    
    return result;
}

bool LLMModelLoader::loadLoRAAdapter(const std::string& model_id,
                                    const std::string& adapter_id,
                                    const std::string& adapter_path,
                                    float scaling) {
    std::shared_lock lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        LOG_ERROR << "Model not loaded: " << model_id;
        return false;
    }
    
    auto& instance = it->second;
    std::lock_guard model_lock(instance->mutex);
    
    // Load LoRA adapter using llama.cpp
    int result = llama_model_apply_lora_from_file(
        instance->model,
        adapter_path.c_str(),
        scaling,
        nullptr,  // No base model override
        1         // Number of threads
    );
    
    if (result != 0) {
        LOG_ERROR << "Failed to load LoRA adapter: " << adapter_path;
        return false;
    }
    
    instance->lora_adapters[adapter_id] = scaling;
    
    LOG_INFO << "LoRA adapter loaded: " << adapter_id 
             << " for model: " << model_id;
    return true;
}

bool LLMModelLoader::storeModelInThemisDB(const std::string& model_id,
                                         const std::string& local_path) {
    // Read model file
    std::ifstream file(local_path, std::ios::binary);
    if (!file) {
        LOG_ERROR << "Failed to open model file: " << local_path;
        return false;
    }
    
    std::vector<char> model_data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    
    // Store as blob in ThemisDB
    std::string entity_id = "llm_model:" + model_id;
    nlohmann::json metadata = {
        {"type", "llm_model"},
        {"model_id", model_id},
        {"format", "gguf"},
        {"size_bytes", model_data.size()},
        {"stored_at", std::time(nullptr)}
    };
    
    // Use storage engine to store
    return storage_->putEntity(
        entity_id,
        std::string(model_data.begin(), model_data.end()),
        metadata.dump()
    );
}

bool LLMModelLoader::loadModelFromThemisDB(const std::string& model_id,
                                          const std::string& local_cache_path) {
    std::string entity_id = "llm_model:" + model_id;
    
    // Retrieve from ThemisDB
    auto entity = storage_->getEntity(entity_id);
    if (!entity.has_value()) {
        LOG_ERROR << "Model not found in ThemisDB: " << model_id;
        return false;
    }
    
    // Write to local cache
    std::ofstream file(local_cache_path, std::ios::binary);
    if (!file) {
        LOG_ERROR << "Failed to write model cache: " << local_cache_path;
        return false;
    }
    
    file.write(entity->blob.data(), entity->blob.size());
    file.close();
    
    LOG_INFO << "Model loaded from ThemisDB to cache: " << local_cache_path;
    return true;
}

llama_model* LLMModelLoader::loadLlamaCppModel(const ModelConfig& config) {
    llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = config.n_gpu_layers;
    params.use_mmap = config.use_mmap;
    params.use_mlock = config.use_mlock;
    
    return llama_load_model_from_file(config.model_path.c_str(), params);
}

std::vector<llama_token> LLMModelLoader::tokenize(
    llama_context* ctx,
    const std::string& text) {
    
    std::vector<llama_token> tokens(text.size() + 1);
    int n_tokens = llama_tokenize(
        llama_get_model(ctx),
        text.c_str(),
        text.size(),
        tokens.data(),
        tokens.size(),
        true,   // add_bos
        false   // special tokens
    );
    
    tokens.resize(n_tokens);
    return tokens;
}

std::string LLMModelLoader::detokenize(
    llama_context* ctx,
    const std::vector<llama_token>& tokens) {
    
    std::string result;
    for (llama_token token : tokens) {
        const char* piece = llama_token_to_piece(
            llama_get_model(ctx),
            token
        );
        result += piece;
    }
    return result;
}

nlohmann::json LLMModelLoader::getModelStats(const std::string& model_id) const {
    std::shared_lock lock(models_mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return nlohmann::json::object();
    }
    
    auto& instance = it->second;
    std::lock_guard model_lock(instance->mutex);
    
    double avg_time = instance->total_requests > 0
        ? instance->total_inference_time_ms / instance->total_requests
        : 0.0;
    
    double avg_tokens = instance->total_requests > 0
        ? static_cast<double>(instance->total_tokens_generated) / instance->total_requests
        : 0.0;
    
    return {
        {"model_id", instance->model_id},
        {"total_requests", instance->total_requests},
        {"total_tokens_generated", instance->total_tokens_generated},
        {"total_inference_time_ms", instance->total_inference_time_ms},
        {"avg_inference_time_ms", avg_time},
        {"avg_tokens_per_request", avg_tokens},
        {"loaded_lora_adapters", instance->lora_adapters.size()},
        {"config", instance->config.metadata}
    };
}

} // namespace llm
} // namespace themis
```

### 3. HTTP API Integration

```cpp
// src/server/llm_endpoints.cpp
#include "llm/llm_model_loader.h"
#include <crow/crow.h>

namespace themis {
namespace server {

void registerLLMEndpoints(crow::SimpleApp& app, llm::LLMModelLoader& loader) {
    // Load model
    CROW_ROUTE(app, "/api/llm/load_model")
        .methods("POST"_method)
        ([&loader](const crow::request& req) {
            auto body = nlohmann::json::parse(req.body);
            
            llm::ModelConfig config;
            config.model_id = body["model_id"];
            config.model_path = body["model_path"];
            config.n_ctx = body.value("n_ctx", 4096);
            config.n_gpu_layers = body.value("n_gpu_layers", 32);
            
            bool success = loader.loadModel(config);
            
            return crow::response(
                success ? 200 : 500,
                nlohmann::json{{"success", success}}.dump()
            );
        });
    
    // Inference
    CROW_ROUTE(app, "/api/llm/inference")
        .methods("POST"_method)
        ([&loader](const crow::request& req) {
            auto body = nlohmann::json::parse(req.body);
            
            std::string model_id = body["model_id"];
            
            llm::InferenceRequest inference_req;
            inference_req.prompt = body["prompt"];
            inference_req.max_tokens = body.value("max_tokens", 512);
            inference_req.temperature = body.value("temperature", 0.7f);
            
            if (body.contains("lora_adapter_id")) {
                inference_req.lora_adapter_id = body["lora_adapter_id"];
            }
            
            try {
                auto result = loader.inference(model_id, inference_req);
                
                nlohmann::json response = {
                    {"generated_text", result.generated_text},
                    {"tokens_generated", result.tokens_generated},
                    {"prompt_tokens", result.prompt_tokens},
                    {"inference_time_ms", result.inference_time_ms}
                };
                
                return crow::response(200, response.dump());
            } catch (const std::exception& e) {
                nlohmann::json error = {
                    {"error", e.what()}
                };
                return crow::response(500, error.dump());
            }
        });
    
    // Load LoRA adapter
    CROW_ROUTE(app, "/api/llm/load_lora")
        .methods("POST"_method)
        ([&loader](const crow::request& req) {
            auto body = nlohmann::json::parse(req.body);
            
            std::string model_id = body["model_id"];
            std::string adapter_id = body["adapter_id"];
            std::string adapter_path = body["adapter_path"];
            float scaling = body.value("scaling", 1.0f);
            
            bool success = loader.loadLoRAAdapter(
                model_id, adapter_id, adapter_path, scaling
            );
            
            return crow::response(
                success ? 200 : 500,
                nlohmann::json{{"success", success}}.dump()
            );
        });
    
    // Get model stats
    CROW_ROUTE(app, "/api/llm/stats/<string>")
        ([&loader](const std::string& model_id) {
            auto stats = loader.getModelStats(model_id);
            return crow::response(200, stats.dump());
        });
}

} // namespace server
} // namespace themis
```

---

## 📦 Deployment

### Docker Integration

```dockerfile
# Dockerfile.llm
FROM themisdb/themisdb:latest

# Install llama.cpp dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Build llama.cpp
WORKDIR /opt
RUN git clone https://github.com/ggerganov/llama.cpp.git && \
    cd llama.cpp && \
    mkdir build && cd build && \
    cmake .. -DLLAMA_CUDA=ON && \
    make -j$(nproc)

# Copy ThemisDB with LLM support
COPY build/themis_server /usr/local/bin/
COPY config/llm_config.yaml /etc/themis/

# Model cache directory
VOLUME /models

EXPOSE 8765 8080

CMD ["themis_server", "--config", "/etc/themis/llm_config.yaml"]
```

### Docker Compose with vLLM

```yaml
# docker-compose-themis-llm.yml
version: '3.8'

services:
  themisdb-llm:
    build:
      context: .
      dockerfile: Dockerfile.llm
    runtime: nvidia
    environment:
      - CUDA_VISIBLE_DEVICES=0
      - THEMIS_LLM_ENABLED=1
      - THEMIS_LLM_GPU_LAYERS=32
    ports:
      - "8765:8765"
      - "8080:8080"
    volumes:
      - ./models:/models
      - themis_data:/data
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]

volumes:
  themis_data:
```

---

## 🔌 Kommunikationsprotokolle: ThemisDB ↔ vLLM

### Übersicht der Protokolloptionen

Bei der Integration von ThemisDB mit vLLM (als externe Komponente) gibt es mehrere Kommunikationsoptionen:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    ThemisDB ↔ vLLM Kommunikation                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Option 1: HTTP/JSON (Standard)                                         │
│  ┌──────────────┐   HTTP POST    ┌──────────────┐                      │
│  │  ThemisDB    │───────────────→ │    vLLM      │                      │
│  │              │   JSON Request  │   OpenAI API │                      │
│  │              │ ←───────────────│              │                      │
│  └──────────────┘   JSON Response └──────────────┘                      │
│  Latenz: ~1-2ms overhead | Bandbreite: Mittel                           │
│                                                                          │
│  Option 2: gRPC/Protobuf (Binär)                                        │
│  ┌──────────────┐   gRPC Call    ┌──────────────┐                      │
│  │  ThemisDB    │───────────────→ │    vLLM      │                      │
│  │              │   Protobuf      │   gRPC Server│                      │
│  │              │ ←───────────────│              │                      │
│  └──────────────┘   Protobuf Resp └──────────────┘                      │
│  Latenz: ~0.2-0.5ms overhead | Bandbreite: Hoch                         │
│                                                                          │
│  Option 3: Shared Memory (Zero-Copy)                                    │
│  ┌──────────────┐   SHM Pointer  ┌──────────────┐                      │
│  │  ThemisDB    │───────────────→ │    vLLM      │                      │
│  │              │   mmap         │   (same host)│                      │
│  │              │ ←───────────────│              │                      │
│  └──────────────┘   Signal/Semaphore└────────────┘                      │
│  Latenz: ~0.05-0.1ms | Bandbreite: Maximal                              │
│                                                                          │
│  Option 4: Unix Domain Sockets (Binär)                                  │
│  ┌──────────────┐   UDS Write    ┌──────────────┐                      │
│  │  ThemisDB    │───────────────→ │    vLLM      │                      │
│  │              │   Binary Stream │   (same host)│                      │
│  │              │ ←───────────────│              │                      │
│  └──────────────┘   Binary Stream └──────────────┘                      │
│  Latenz: ~0.1-0.3ms | Bandbreite: Sehr hoch                             │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### Option 1: HTTP/JSON (Standard vLLM API)

**Aktueller Stand:** Dies ist die Standard-Integration, wie in `docker-compose-vllm.yml` gezeigt.

```cpp
// ThemisDB sendet JSON über HTTP
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class VLLMHTTPClient {
public:
    std::string inference(const std::string& prompt, int max_tokens = 512) {
        nlohmann::json request = {
            {"model", "mistralai/Mistral-7B-v0.1"},
            {"prompt", prompt},
            {"max_tokens", max_tokens},
            {"temperature", 0.7}
        };
        
        // HTTP POST zu vLLM
        std::string response = httpPost(
            "http://vllm:8000/v1/completions",
            request.dump()
        );
        
        auto result = nlohmann::json::parse(response);
        return result["choices"][0]["text"];
    }
    
private:
    std::string httpPost(const std::string& url, const std::string& body) {
        CURL* curl = curl_easy_init();
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        return response;
    }
};
```

**Performance:**
- ✅ **Einfach zu implementieren** - Standard HTTP/JSON
- ✅ **Kompatibel mit vLLM OpenAI API**
- ⚠️ **JSON Parsing Overhead** - ~1-2ms pro Request
- ⚠️ **Höhere Bandbreite** - JSON ist größer als Binär

### Option 2: gRPC/Protobuf (Binär, Empfohlen für Produktion)

**Warum binär?** Binäre Protokolle reduzieren Serialisierungs-Overhead und Bandbreite.

#### Protobuf Definition

```protobuf
// vllm_service.proto
syntax = "proto3";

package themis.llm;

service VLLMService {
    rpc Inference(InferenceRequest) returns (InferenceResponse);
    rpc InferenceStream(InferenceRequest) returns (stream InferenceToken);
}

message InferenceRequest {
    string model_id = 1;
    bytes prompt_tokens = 2;        // Binär: Tokenisierte Prompt
    int32 max_tokens = 3;
    float temperature = 4;
    float top_p = 5;
    optional string lora_adapter = 6;
    
    // Effizient: Direkt Tokens statt String
    repeated int32 token_ids = 7;
}

message InferenceResponse {
    bytes generated_tokens = 1;     // Binär: Generierte Tokens
    string generated_text = 2;      // Optional: Text für Debugging
    int32 tokens_generated = 3;
    int32 prompt_tokens = 4;
    float inference_time_ms = 5;
}

message InferenceToken {
    int32 token_id = 1;
    string token_text = 2;
    bool is_finished = 3;
}
```

#### ThemisDB gRPC Client

```cpp
// src/llm/vllm_grpc_client.h
#pragma once

#include <grpcpp/grpcpp.h>
#include "vllm_service.grpc.pb.h"
#include <memory>
#include <string>

namespace themis {
namespace llm {

class VLLMgRPCClient {
public:
    VLLMgRPCClient(const std::string& server_address)
        : stub_(themis::llm::VLLMService::NewStub(
            grpc::CreateChannel(server_address, 
                              grpc::InsecureChannelCredentials())
        )) {}
    
    InferenceResponse inference(const InferenceRequest& request) {
        InferenceResponse response;
        grpc::ClientContext context;
        
        // Binäre gRPC Kommunikation
        grpc::Status status = stub_->Inference(&context, request, &response);
        
        if (!status.ok()) {
            throw std::runtime_error("gRPC error: " + status.error_message());
        }
        
        return response;
    }
    
    // Streaming inference (für lange Generierungen)
    void inferenceStream(const InferenceRequest& request,
                        std::function<void(const InferenceToken&)> callback) {
        grpc::ClientContext context;
        
        std::unique_ptr<grpc::ClientReader<InferenceToken>> reader(
            stub_->InferenceStream(&context, request)
        );
        
        InferenceToken token;
        while (reader->Read(&token)) {
            callback(token);  // Streaming tokens
            if (token.is_finished()) break;
        }
    }
    
private:
    std::unique_ptr<themis::llm::VLLMService::Stub> stub_;
};

} // namespace llm
} // namespace themis
```

#### Nutzung in ThemisDB

```cpp
// ThemisDB verwendet binäre Kommunikation
VLLMgRPCClient vllm_client("localhost:50051");

// Erstelle binäre Request
InferenceRequest request;
request.set_model_id("mistral-7b");

// WICHTIG: Bereits tokenisierte Daten (binär)
std::vector<int32_t> token_ids = tokenizer_.encode(prompt);
for (int32_t id : token_ids) {
    request.add_token_ids(id);
}

request.set_max_tokens(512);
request.set_temperature(0.7f);

// Binärer gRPC Call
auto response = vllm_client.inference(request);

// Ergebnis: Binäre Tokens
const auto& generated_bytes = response.generated_tokens();
// Dekodiere lokal oder nutze Text
std::string result = response.generated_text();
```

**Performance-Vergleich:**

| Metrik | HTTP/JSON | gRPC/Protobuf | Verbesserung |
|--------|-----------|---------------|--------------|
| Serialisierung | ~0.8-1.2ms | ~0.1-0.2ms | **6x schneller** |
| Payload Size | 100% | 30-40% | **60-70% kleiner** |
| Bandbreite (1000 req/s) | ~50 MB/s | ~15-20 MB/s | **3x effizienter** |
| Latenz (Network) | 1-2ms | 0.2-0.5ms | **4x schneller** |
| CPU Usage | Hoch (JSON parse) | Niedrig | **50% weniger** |

### Option 3: Shared Memory (Zero-Copy, Maximal Performance)

**Nur für same-host Deployment!** ThemisDB und vLLM laufen auf derselben Maschine.

```cpp
// src/llm/vllm_shm_client.h
#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <cstring>

namespace themis {
namespace llm {

struct SHMInferenceRequest {
    char model_id[64];
    int32_t token_ids[4096];        // Vorallokiert
    int32_t num_tokens;
    int32_t max_tokens;
    float temperature;
    bool ready;                     // Signal für vLLM
};

struct SHMInferenceResponse {
    int32_t generated_tokens[4096];
    int32_t num_generated;
    float inference_time_ms;
    bool ready;                     // Signal für ThemisDB
};

class VLLMShmClient {
public:
    VLLMShmClient() {
        // Erstelle Shared Memory Segment
        shm_fd_ = shm_open("/vllm_themis_shm", O_CREAT | O_RDWR, 0666);
        ftruncate(shm_fd_, sizeof(SHMInferenceRequest) + 
                          sizeof(SHMInferenceResponse));
        
        // Map in Speicher
        void* ptr = mmap(0, sizeof(SHMInferenceRequest) + 
                           sizeof(SHMInferenceResponse),
                        PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
        
        request_ = static_cast<SHMInferenceRequest*>(ptr);
        response_ = reinterpret_cast<SHMInferenceResponse*>(
            static_cast<char*>(ptr) + sizeof(SHMInferenceRequest)
        );
        
        // Semaphoren für Synchronisation
        req_sem_ = sem_open("/vllm_req_sem", O_CREAT, 0666, 0);
        resp_sem_ = sem_open("/vllm_resp_sem", O_CREAT, 0666, 0);
    }
    
    std::vector<int32_t> inference(const std::string& model_id,
                                   const std::vector<int32_t>& tokens,
                                   int max_tokens = 512) {
        // ZERO-COPY: Schreibe direkt in Shared Memory
        std::strncpy(request_->model_id, model_id.c_str(), 63);
        std::memcpy(request_->token_ids, tokens.data(), 
                   tokens.size() * sizeof(int32_t));
        request_->num_tokens = tokens.size();
        request_->max_tokens = max_tokens;
        request_->ready = true;
        
        // Signal vLLM
        sem_post(req_sem_);
        
        // Warte auf Antwort
        sem_wait(resp_sem_);
        
        // ZERO-COPY: Lese direkt aus Shared Memory
        std::vector<int32_t> result(
            response_->generated_tokens,
            response_->generated_tokens + response_->num_generated
        );
        
        response_->ready = false;
        return result;
    }
    
    ~VLLMShmClient() {
        munmap(request_, sizeof(SHMInferenceRequest) + 
                        sizeof(SHMInferenceResponse));
        close(shm_fd_);
        sem_close(req_sem_);
        sem_close(resp_sem_);
    }
    
private:
    int shm_fd_;
    SHMInferenceRequest* request_;
    SHMInferenceResponse* response_;
    sem_t* req_sem_;
    sem_t* resp_sem_;
};

} // namespace llm
} // namespace themis
```

**vLLM Server-Side (Python)**

```python
# vllm_shm_server.py
import mmap
import posix_ipc
import struct
import numpy as np
from vllm import LLM, SamplingParams

class VLLMShmServer:
    def __init__(self):
        # Öffne Shared Memory
        self.shm = posix_ipc.SharedMemory('/vllm_themis_shm')
        self.mem = mmap.mmap(self.shm.fd, self.shm.size)
        
        # Semaphoren
        self.req_sem = posix_ipc.Semaphore('/vllm_req_sem')
        self.resp_sem = posix_ipc.Semaphore('/vllm_resp_sem')
        
        # vLLM Engine
        self.llm = LLM(model="mistralai/Mistral-7B-v0.1")
    
    def run(self):
        while True:
            # Warte auf Request
            self.req_sem.acquire()
            
            # ZERO-COPY: Lese direkt aus Shared Memory
            model_id = self.mem[0:64].decode('utf-8').strip('\x00')
            num_tokens = struct.unpack('i', self.mem[4160:4164])[0]
            token_ids = struct.unpack(f'{num_tokens}i', 
                                     self.mem[64:64+num_tokens*4])
            max_tokens = struct.unpack('i', self.mem[4164:4168])[0]
            
            # Inference
            sampling_params = SamplingParams(max_tokens=max_tokens)
            outputs = self.llm.generate(
                prompt_token_ids=[list(token_ids)],
                sampling_params=sampling_params
            )
            
            generated_ids = outputs[0].outputs[0].token_ids
            
            # ZERO-COPY: Schreibe direkt in Shared Memory (Response Bereich)
            response_offset = 8192  # Nach Request Bereich
            struct.pack_into(f'{len(generated_ids)}i', self.mem, 
                           response_offset, *generated_ids)
            struct.pack_into('i', self.mem, 
                           response_offset + 16384, len(generated_ids))
            
            # Signal ThemisDB
            self.resp_sem.release()

if __name__ == '__main__':
    server = VLLMShmServer()
    server.run()
```

**Performance:**
- ✅ **Maximale Performance** - Keine Kopien, kein Netzwerk
- ✅ **Latenz ~0.05-0.1ms** - Nur Semaphore Overhead
- ✅ **Zero-Copy** - Direkte Memory-Zugriffe
- ❌ **Nur same-host** - Nicht für verteilte Systeme
- ⚠️ **Komplex** - Shared Memory Management

### Option 4: Unix Domain Sockets (Binär, Guter Kompromiss)

```cpp
// src/llm/vllm_uds_client.h
#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <cstring>

namespace themis {
namespace llm {

class VLLMUdsClient {
public:
    VLLMUdsClient(const std::string& socket_path) {
        sock_ = socket(AF_UNIX, SOCK_STREAM, 0);
        
        struct sockaddr_un addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socket_path.c_str(), 
                    sizeof(addr.sun_path) - 1);
        
        connect(sock_, (struct sockaddr*)&addr, sizeof(addr));
    }
    
    std::vector<int32_t> inference(const std::vector<int32_t>& tokens,
                                   int max_tokens = 512) {
        // Binäres Protokoll: [num_tokens][token_ids...][max_tokens]
        int32_t num_tokens = tokens.size();
        
        // Sende Header
        write(sock_, &num_tokens, sizeof(int32_t));
        
        // Sende Tokens (binär)
        write(sock_, tokens.data(), tokens.size() * sizeof(int32_t));
        
        // Sende Config
        write(sock_, &max_tokens, sizeof(int32_t));
        
        // Empfange Antwort (binär)
        int32_t num_generated;
        read(sock_, &num_generated, sizeof(int32_t));
        
        std::vector<int32_t> result(num_generated);
        read(sock_, result.data(), num_generated * sizeof(int32_t));
        
        return result;
    }
    
    ~VLLMUdsClient() {
        close(sock_);
    }
    
private:
    int sock_;
};

} // namespace llm
} // namespace themis
```

**Performance:**
- ✅ **Binäres Protokoll** - Effizient
- ✅ **Niedrige Latenz** - ~0.1-0.3ms
- ✅ **Hohe Bandbreite** - Kernel-optimiert
- ✅ **Einfacher als Shared Memory**
- ❌ **Nur same-host**

### Empfehlung: Welches Protokoll wann?

```
┌──────────────────────────────────────────────────────────────────┐
│                   Protokoll-Entscheidungsbaum                     │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ThemisDB und vLLM auf derselben Maschine?                       │
│         │                                                         │
│         ├─ JA ──→ Hohe Performance erforderlich?                 │
│         │           │                                             │
│         │           ├─ JA ──→ Shared Memory (Zero-Copy)          │
│         │           │         Latenz: 0.05-0.1ms ⭐⭐⭐⭐⭐           │
│         │           │                                             │
│         │           └─ NEIN ──→ Unix Domain Sockets              │
│         │                       Latenz: 0.1-0.3ms ⭐⭐⭐⭐          │
│         │                                                         │
│         └─ NEIN ──→ Verteiltes System                            │
│                     │                                             │
│                     ├─ Produktion? ──→ gRPC/Protobuf (Binär)     │
│                     │                   Latenz: 0.2-0.5ms ⭐⭐⭐⭐   │
│                     │                                             │
│                     └─ Development/Testing ──→ HTTP/JSON         │
│                                               Latenz: 1-2ms ⭐⭐⭐   │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### Performance-Vergleich: Alle Protokolle

**Test Setup:**
- Prompt: 512 Tokens
- Generation: 100 Tokens
- Model: Mistral-7B
- Hardware: i7-12700K, RTX 4090

| Protokoll | Latenz (Kommunikation) | Bandbreite | Payload Size | Komplexität | Same-Host Only |
|-----------|------------------------|------------|--------------|-------------|----------------|
| **HTTP/JSON** | 1-2ms | 50 MB/s | 100% | ⭐ | ❌ |
| **gRPC/Protobuf** | 0.2-0.5ms | 150 MB/s | 30-40% | ⭐⭐ | ❌ |
| **Unix Domain Sockets** | 0.1-0.3ms | 300 MB/s | 25% | ⭐⭐⭐ | ✅ |
| **Shared Memory** | 0.05-0.1ms | 1000+ MB/s | 25% | ⭐⭐⭐⭐⭐ | ✅ |

**Hinweis:** Die Kommunikationslatenz ist minimal im Vergleich zur Inferenzzeit (~50-500ms). Bei hohem Durchsatz (>100 req/s) wird binäre Kommunikation wichtig.

---

## 🔍 Vergleich: Eigener Loader vs. vLLM Integration

### Wann eigener LLM Loader?

**Vorteile:**
- ✅ Tiefe Integration mit ThemisDB Storage
- ✅ Einheitliches API
- ✅ Direkter Zugriff auf ThemisDB Caches
- ✅ Kleinerer Memory Footprint
- ✅ Keine externe Dependency

**Nachteile:**
- ❌ Mehr Entwicklungsaufwand
- ❌ Weniger Features als vLLM
- ❌ Kein PagedAttention
- ❌ Einfacheres Batching

### Wann vLLM als externes System?

**Vorteile:**
- ✅ Production-ready
- ✅ PagedAttention (höhere Throughput)
- ✅ Sophisticated Batching
- ✅ OpenAI-compatible API
- ✅ Aktive Community

**Nachteile:**
- ❌ Externe Dependency
- ❌ Separater Prozess
- ❌ Mehr Ressourcen

### Empfehlung: Hybrid Approach

```
┌────────────────────────────────────────┐
│         ThemisDB Ecosystem              │
├────────────────────────────────────────┤
│                                         │
│  ┌──────────────────────────────────┐  │
│  │  ThemisDB Native LLM Loader      │  │
│  │  - Lightweight inference         │  │
│  │  - Direct DB integration         │  │
│  │  - For: embeddings, small models │  │
│  └──────────────────────────────────┘  │
│                                         │
│  ┌──────────────────────────────────┐  │
│  │  vLLM Integration                │  │
│  │  - Heavy inference               │  │
│  │  - Via HTTP/gRPC                 │  │
│  │  - For: Large models, production │  │
│  └──────────────────────────────────┘  │
│                                         │
└────────────────────────────────────────┘
```

---

## 📊 Performance Benchmarks

### Expected Performance (Native Loader)

**Test Setup:**
- Model: Mistral-7B-GGUF (Q4_K_M)
- GPU: NVIDIA RTX 4090
- RAM: 64GB
- Context: 4096 tokens

| Metric | Performance |
|--------|-------------|
| **Model Load Time** | ~2-3 seconds |
| **Prompt Processing (512 tokens)** | ~50ms |
| **Generation Speed** | 40-60 tokens/sec |
| **Memory Usage** | ~5GB VRAM (Q4 quantization) |
| **Concurrent Requests** | 4-8 (depending on context size) |

### vLLM Performance (Reference)

| Metric | Performance |
|--------|-------------|
| **Model Load Time** | ~5-8 seconds |
| **Prompt Processing (512 tokens)** | ~30ms |
| **Generation Speed** | 80-120 tokens/sec |
| **Memory Usage** | ~8GB VRAM (FP16) |
| **Concurrent Requests** | 32-128 (PagedAttention) |

---

## 🚀 Quick Start Example

### 1. Build with LLM Support

```bash
# CMakeLists.txt additions
option(THEMIS_ENABLE_LLM "Enable LLM loader support" ON)

if(THEMIS_ENABLE_LLM)
    # Add llama.cpp
    add_subdirectory(third_party/llama.cpp)
    
    # Add LLM source files
    add_library(themis_llm
        src/llm/llm_model_loader.cpp
        src/llm/llm_interaction_store.cpp
        src/llm/prompt_manager.cpp
    )
    
    target_link_libraries(themis_llm
        llama
        themis_storage
        TBB::tbb
    )
endif()
```

```bash
# Build
cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build -j$(nproc)
```

### 2. Download Model

```bash
# Download GGUF model
wget https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf \
    -O models/mistral-7b-q4.gguf
```

### 3. Load Model

```bash
curl -X POST http://localhost:8765/api/llm/load_model \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "mistral-7b",
    "model_path": "/models/mistral-7b-q4.gguf",
    "n_ctx": 4096,
    "n_gpu_layers": 32
  }'
```

### 4. Run Inference

```bash
curl -X POST http://localhost:8765/api/llm/inference \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "mistral-7b",
    "prompt": "Explain what ThemisDB is in one sentence.",
    "max_tokens": 100,
    "temperature": 0.7
  }'
```

**Response:**
```json
{
  "generated_text": "ThemisDB is a high-performance multi-model database that combines relational, graph, vector, and document capabilities with ACID transactions.",
  "tokens_generated": 28,
  "prompt_tokens": 12,
  "inference_time_ms": 450
}
```

---

## 🔐 Security Considerations

### Model Storage Security

```cpp
// Encrypt model files in ThemisDB
#include "security/encryption.h"

bool LLMModelLoader::storeEncryptedModel(
    const std::string& model_id,
    const std::string& local_path,
    const std::string& encryption_key) {
    
    // Read model
    auto model_data = readFile(local_path);
    
    // Encrypt using ThemisDB security layer
    auto encrypted = security::encrypt(model_data, encryption_key);
    
    // Store encrypted blob
    return storage_->putEntity("llm_model:" + model_id, encrypted);
}
```

### Access Control

```cpp
// Role-based access for LLM endpoints
CROW_ROUTE(app, "/api/llm/inference")
    .methods("POST"_method)
    ([&loader, &auth](const crow::request& req) {
        // Check permissions
        if (!auth.hasPermission(req, "llm:inference")) {
            return crow::response(403, "Forbidden");
        }
        
        // ... inference logic
    });
```

---

## 📚 Weiterführende Ressourcen

### Official Documentation
- [llama.cpp Repository](https://github.com/ggerganov/llama.cpp)
- [GGML Documentation](https://github.com/ggerganov/ggml)
- [vLLM Documentation](https://docs.vllm.ai/)
- [HuggingFace PEFT](https://huggingface.co/docs/peft)

### ThemisDB Documentation
- [ThemisDB LLM Module](./README.md)
- [vLLM Multi-LoRA Integration](../connectors/VLLM_MULTI_LORA_INTEGRATION.md)
- [Plugin Loader Architecture](../../include/acceleration/plugin_loader.h)
- [Semantic Cache](../features/features_semantic_cache.md)

### Community Resources
- [Awesome LLM](https://github.com/Hannibal046/Awesome-LLM)
- [LLM Inference Optimization](https://lilianweng.github.io/posts/2023-01-10-inference-optimization/)

---

## 🎯 Zusammenfassung

### Minimal Required Libraries
```
Core:
  - llama.cpp (Inference Engine)
  - GGML (Tensor operations)
  - nlohmann/json (JSON handling)
  - TBB (Threading)
  
GPU (Optional):
  - CUDA Toolkit (NVIDIA)
  - Vulkan SDK (Cross-platform)
  - HIP (AMD)
  
Utilities:
  - SentencePiece (Tokenization)
  - mimalloc (Memory allocation)
```

### Recommended Approach

**Phase 1: Basic Integration**
1. Integrate llama.cpp als Backend
2. Implementiere LLMModelLoader Klasse
3. HTTP API für load/inference
4. Speicherung in ThemisDB Storage

**Phase 2: Advanced Features**
1. LoRA Adapter Support
2. Request Batching mit TBB
3. Semantic Cache Integration
4. GPU Backend Auswahl

**Phase 3: Production**
1. Async Inference
2. Streaming Responses
3. Model Versioning
4. Monitoring & Metrics

---

**Erstellt:** Dezember 2025  
**Letzte Aktualisierung:** Dezember 2025  
**Maintainer:** ThemisDB LLM Team
