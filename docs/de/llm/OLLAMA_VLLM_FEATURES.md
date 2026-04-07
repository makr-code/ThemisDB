# Ollama & vLLM Inspired Features for ThemisDB v1.3.0

**Date:** December 2025  
**Status:** Implementation Guide  
**Inspired by:** Ollama (lazy loading) + vLLM (multi-LoRA)

---

## 🎯 Overview

ThemisDB v1.3.0 LLM plugin system now incorporates best practices from two leading LLM serving frameworks:

1. **Ollama**: Lazy model loading with intelligent caching
2. **vLLM**: Efficient multi-LoRA adapter management

This document explains these features and how to use them.

---

## 🦙 Ollama-Inspired: Lazy Model Loading

### Concept

**Ollama's approach:** Models are loaded on-demand when first requested and automatically unloaded when not in use. This enables:
- Running multiple models without loading all at once
- Automatic memory management
- Fast switching between frequently-used models (cached)
- Slow first request, fast subsequent requests

### ThemisDB Implementation

**`LazyModelLoader`** class provides:
- **On-demand loading**: Models load only when actually needed
- **LRU caching**: Keep N most recently used models in memory
- **TTL-based eviction**: Unload models after inactivity period
- **Memory limits**: Respect VRAM/RAM constraints
- **Pinning**: Keep important models always loaded

### Usage Example

```cpp
#include "llm/model_loader.h"

// Initialize lazy loader
LazyModelLoader::Config config;
config.max_vram_mb = 24576;      // 24 GB VRAM budget
config.max_models = 3;           // Keep up to 3 models loaded
config.model_ttl = std::chrono::seconds(1800);  // 30 min TTL

LazyModelLoader loader(config);

// First request: Model loads lazily (slower)
auto* model1 = loader.getOrLoadModel(
    "mistral-7b",
    "/models/mistral-7b-instruct-q4.gguf"
);
// ~2-3 seconds to load

// Second request: Model already loaded (fast)
auto* model1_again = loader.getOrLoadModel("mistral-7b", "");
// ~0ms, cache hit!

// Load another model
auto* model2 = loader.getOrLoadModel(
    "llama-3-8b",
    "/models/llama-3-8b-instruct-q4.gguf"
);

// Pin important model (won't be evicted)
loader.pinModel("mistral-7b");

// Automatic eviction when cache is full
// If 4th model is requested, LRU model (not pinned) is evicted
```

### Configuration

```yaml
# config/llm_config.yaml
llm:
  model_loading:
    # Lazy loading (Ollama-style)
    enable_lazy_load: true
    max_models: 3
    max_vram_mb: 24576
    max_ram_mb: 65536
    model_ttl_seconds: 1800
    
    # Preload models at startup (optional)
    preload:
      - model_id: "mistral-7b"
        path: "/models/mistral-7b-instruct-q4.gguf"
        pin: true  # Keep loaded always
```

### Benefits

| Scenario | Without Lazy Loading | With Lazy Loading |
|----------|---------------------|-------------------|
| 3 models, only 1 used | All 3 loaded (18 GB VRAM) | Only 1 loaded (6 GB VRAM) |
| Switch between 2 models | Reload each time (~3s) | Both cached (<1ms) |
| 10 models, use 3 | Cannot fit in memory | 3 active, 7 unloaded |
| Model not used for 30min | Still loaded | Auto-evicted |

---

## 🚀 vLLM-Inspired: Multi-LoRA Management

### Concept

**vLLM's approach:** Multiple LoRA adapters can be loaded simultaneously and efficiently switched during inference. This enables:
- Serving multiple fine-tuned variants of same base model
- Per-request LoRA selection
- Batched inference with different LoRAs
- Memory-efficient adapter storage

### ThemisDB Implementation

**`MultiLoRAManager`** class provides:
- **Multi-LoRA support**: Load many adapters simultaneously
- **Lazy LoRA loading**: Adapters load on-demand
- **Efficient switching**: Change LoRA per request
- **Batch inference**: Different LoRAs in same batch (if backend supports)
- **Adapter fusion**: Merge multiple LoRAs into one
- **Cross-shard transfer**: Share LoRAs between shards

### Usage Example

```cpp
#include "llm/multi_lora_manager.h"

// Initialize multi-LoRA manager
MultiLoRAManager::Config config;
config.max_lora_slots = 16;          // Up to 16 LoRAs
config.max_lora_vram_mb = 2048;      // 2 GB for all LoRAs
config.enable_multi_lora_batch = true;  // vLLM-style batching

MultiLoRAManager lora_mgr(config);

// Load multiple LoRA adapters for same base model
lora_mgr.loadLoRA("legal-qa", "/loras/legal-qa-v1.bin", "mistral-7b");
lora_mgr.loadLoRA("medical-diag", "/loras/medical-v1.bin", "mistral-7b");
lora_mgr.loadLoRA("code-assist", "/loras/code-v1.bin", "mistral-7b");

// Request 1: Use legal LoRA
InferenceRequest req1;
req1.prompt = "What is liability law?";
req1.lora_adapter_id = "legal-qa";

// Request 2: Use medical LoRA
InferenceRequest req2;
req2.prompt = "Symptoms of diabetes?";
req2.lora_adapter_id = "medical-diag";

// Request 3: Use code LoRA
InferenceRequest req3;
req3.prompt = "Write a Python function";
req3.lora_adapter_id = "code-assist";

// Option A: Sequential (simple)
auto response1 = plugin->generate(req1);  // With legal-qa LoRA
auto response2 = plugin->generate(req2);  // With medical-diag LoRA
auto response3 = plugin->generate(req3);  // With code-assist LoRA

// Option B: Batched (vLLM-style, if supported)
std::vector<std::pair<InferenceRequest, std::string>> batch = {
    {req1, "legal-qa"},
    {req2, "medical-diag"},
    {req3, "code-assist"}
};
auto responses = lora_mgr.batchInferenceMultiLoRA(batch, model_context);
```

### Configuration

```yaml
# config/llm_config.yaml
llm:
  lora_management:
    # Multi-LoRA (vLLM-style)
    enable_multi_lora: true
    max_lora_slots: 16
    max_lora_vram_mb: 2048
    lora_ttl_seconds: 1800
    
    # Advanced features
    enable_multi_lora_batch: true   # Different LoRAs in same batch
    enable_adapter_fusion: true     # Merge multiple LoRAs
    
    # Preload adapters (optional)
    preload:
      - lora_id: "legal-qa-v1"
        path: "/loras/legal-qa-v1.bin"
        base_model: "mistral-7b"
        pin: true
      
      - lora_id: "medical-diagnosis-v1"
        path: "/loras/medical-v1.bin"
        base_model: "mistral-7b"
        pin: true
```

### Benefits

| Scenario | Without Multi-LoRA | With Multi-LoRA |
|----------|-------------------|-----------------|
| 5 domain-specific variants | Need 5 full models (30 GB) | 1 model + 5 LoRAs (6.5 GB) |
| Switch between domains | Reload model (~3s) | Switch LoRA (~5ms) |
| Batch with mixed domains | Process separately | Single batch (faster) |
| 20 LoRA variants | Cannot fit | 16 active, 4 on-demand |

---

## 🔄 Combined Workflow

### Real-World Example: Multi-Domain RAG System

```cpp
// Setup
LazyModelLoader model_loader(model_config);
MultiLoRAManager lora_manager(lora_config);

// Lazy load base model
auto* model = model_loader.getOrLoadModel(
    "mistral-7b",
    "/models/mistral-7b-instruct-q4.gguf"
);

// Load domain-specific LoRAs on-demand
// (They load lazily when first request comes in)

// Request 1: Legal query
{
    RAGContext context;
    context.query = "Contract law question";
    context.documents = /* legal documents from vector search */;
    
    InferenceRequest req;
    req.prompt = context.query;
    req.lora_adapter_id = "legal-qa-v1";  // Loads lazily if not cached
    
    auto response = plugin->generateRAG(context, req);
}

// Request 2: Medical query (different LoRA)
{
    RAGContext context;
    context.query = "Medical diagnosis question";
    context.documents = /* medical records from vector search */;
    
    InferenceRequest req;
    req.prompt = context.query;
    req.lora_adapter_id = "medical-diag-v1";  // Loads lazily
    
    auto response = plugin->generateRAG(context, req);
}

// Both model and LoRAs stay cached for fast subsequent requests
// Automatic eviction after TTL expires
```

---

## 📊 Performance Comparison

### Scenario: 5 Domain-Specific Models

**Traditional approach (5 separate models):**
```
Memory: 5 × 6 GB = 30 GB VRAM
Cold start: 5 × 3s = 15 seconds
Switching: 3 seconds per domain change
Concurrent domains: Limited by VRAM
```

**Ollama + vLLM approach (1 model + 5 LoRAs):**
```
Memory: 6 GB (model) + 160 MB (LoRAs) = 6.16 GB VRAM
Cold start: 3s (model) + 50ms (LoRA) = 3.05 seconds
Switching: 5ms per LoRA change
Concurrent domains: Up to 16 LoRAs simultaneously
```

**Savings:**
- **79% less VRAM** (6.16 GB vs 30 GB)
- **600x faster switching** (5ms vs 3s)
- **5x more domains** possible in same memory

---

## 🛠️ Integration with Existing Code

### Update LlamaWrapper

The existing `LlamaWrapper` can be enhanced to use these new managers:

```cpp
class LlamaWrapper : public ILLMPlugin {
private:
    std::unique_ptr<LazyModelLoader> model_loader_;
    std::unique_ptr<MultiLoRAManager> lora_manager_;
    
public:
    bool loadModel(const std::string& model_path, const json& config) override {
        // Use lazy loader instead of immediate loading
        auto* model = model_loader_->getOrLoadModel(
            extractModelId(model_path),
            model_path,
            config
        );
        return model != nullptr;
    }
    
    bool loadLoRA(const std::string& lora_id, 
                  const std::string& lora_path,
                  float scale) override {
        // Use multi-LoRA manager
        return lora_manager_->loadLoRA(lora_id, lora_path, 
                                       current_model_id_, scale);
    }
    
    InferenceResponse generate(const InferenceRequest& request) override {
        // Apply LoRA if specified
        if (request.lora_adapter_id) {
            lora_manager_->applyLoRA(*request.lora_adapter_id, context_);
        }
        
        // Actual inference...
        
        return response;
    }
};
```

---

## 📈 Monitoring & Statistics

### Model Loader Stats

```cpp
auto stats = model_loader.getCacheStats();
// {
//   "cache_hits": 127,
//   "cache_misses": 15,
//   "hit_rate": 0.894,
//   "evictions": 3
// }

auto memory = model_loader.getMemoryStats();
// {
//   "vram_used_mb": 12288,
//   "vram_max_mb": 24576,
//   "vram_usage_pct": 50.0,
//   "models_loaded": 2,
//   "models_max": 3
// }
```

### LoRA Manager Stats

```cpp
auto stats = lora_manager.getCacheStats();
// {
//   "cache_hits": 453,
//   "cache_misses": 27,
//   "hit_rate": 0.944,
//   "switches": 480,
//   "evictions": 5
// }

auto memory = lora_manager.getMemoryStats();
// {
//   "vram_used_mb": 512,
//   "vram_max_mb": 2048,
//   "vram_usage_pct": 25.0,
//   "loras_loaded": 8,
//   "loras_max": 16
// }
```

---

## 🔮 Future Enhancements

### Phase 2 (Post v1.3.0)

1. **Async model loading**: Background loading without blocking
2. **Model quantization on-the-fly**: Convert models during load
3. **LoRA composition**: Combine multiple LoRAs mathematically
4. **Distributed LoRA registry**: Share LoRAs across cluster (from AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md)

---

## 📚 References

- **Ollama**: https://github.com/ollama/ollama
- **vLLM**: https://github.com/vllm-project/vllm
- **LoRA Paper**: "LoRA: Low-Rank Adaptation of Large Language Models" (Hu et al., 2021)
- **ThemisDB Architecture**: [AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md)

---

**Version:** ThemisDB v1.3.0  
**Last Updated:** April 2026  
**Status:** Implemented (Stub), Full Integration Pending
