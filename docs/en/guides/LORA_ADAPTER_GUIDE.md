# ThemisDB LoRA Adapter Management Guide

**Version:** 1.3.3+  
**Last Updated:** 2026-04-06

## Introduction

This guide covers ThemisDB's LoRA (Low-Rank Adaptation) adapter management system, enabling efficient fine-tuning and hot-swapping of LLM adapters without reloading base models.

## Quick Start

### Basic Configuration

```yaml
# config/llm_config.yaml
lora:
  max_lora_slots: 8
  max_lora_vram_mb: 512
  lora_ttl_minutes: 30
  enable_multi_lora_batch: true
  enable_adapter_fusion: true
  
  # Preload adapters on startup
  preload:
    - id: "general"
      path: "/models/lora/general.bin"
      scale: 1.0
      pin: true  # Keep in memory
```

### C++ API

```cpp
#include "llm/multi_lora_manager.h"

// Configure LoRA manager
MultiLoRAManager::Config config;
config.max_lora_slots = 8;
config.max_lora_vram_mb = 512;
config.enable_multi_lora_batch = true;

MultiLoRAManager manager(config);
```

## Core Operations

### Loading LoRA Adapters

```cpp
// Load a LoRA adapter
bool success = manager.loadLoRA(
    "math-lora",                    // Unique ID
    "/models/lora/math.bin",        // Path to LoRA file
    "llama-2-7b",                   // Base model ID
    1.0f                            // Scale factor
);

if (success) {
    std::cout << "LoRA loaded successfully\n";
}
```

### Unloading LoRA Adapters

```cpp
// Unload when no longer needed
manager.unloadLoRA("math-lora");

// Force unload even if pinned
manager.unloadLoRA("math-lora", true);
```

### Pinning Adapters

Prevent frequently-used adapters from being evicted:

```cpp
// Pin adapter in memory
manager.pinLoRA("general-lora");

// Unpin when usage pattern changes
manager.unpinLoRA("general-lora");
```

### Listing Loaded Adapters

```cpp
auto loras = manager.listLoRAs();
for (const auto& lora : loras) {
    std::cout << "ID: " << lora.lora_id << "\n"
              << "  Path: " << lora.path << "\n"
              << "  VRAM: " << lora.vram_bytes / (1024*1024) << " MB\n"
              << "  Uses: " << lora.use_count << "\n"
              << "  Active: " << (lora.is_active ? "yes" : "no") << "\n";
}
```

## Advanced Features

### Multi-LoRA Batch Inference

Process multiple requests with different LoRAs in a single batch:

```cpp
std::vector<std::pair<InferenceRequest, std::string>> requests;

// Math request with math-lora
InferenceRequest math_req;
math_req.prompt = "Solve: 2x + 5 = 13";
requests.push_back({math_req, "math-lora"});

// Code request with code-lora
InferenceRequest code_req;
code_req.prompt = "Write a Python sort function";
requests.push_back({code_req, "code-lora"});

// Chat request with general-lora
InferenceRequest chat_req;
chat_req.prompt = "How are you?";
requests.push_back({chat_req, "general-lora"});

// Process all in one batch
auto responses = manager.batchInferenceMultiLoRA(requests, model_context);

for (size_t i = 0; i < responses.size(); ++i) {
    std::cout << "Response " << i << ": " << responses[i].text << "\n";
    std::cout << "  LoRA used: " << responses[i].lora_used << "\n";
    std::cout << "  Latency: " << responses[i].latency_ms << "ms\n";
}
```

### LoRA Fusion

Combine multiple LoRA adapters into one:

```cpp
std::vector<std::string> source_loras = {
    "math-lora",
    "code-lora",
    "reasoning-lora"
};

// Weighted combination
std::vector<float> weights = {0.4f, 0.4f, 0.2f};

bool fused = manager.fuseLoRAs(
    source_loras,
    "combined-lora",     // New fused adapter ID
    weights
);

if (fused) {
    std::cout << "LoRA fusion successful\n";
    
    // Use the fused adapter
    InferenceRequest req;
    req.prompt = "Complex math problem with code solution";
    req.lora_id = "combined-lora";
    
    auto response = plugin.generate(req);
}
```

### Cross-Shard LoRA Sharing

Export and import LoRAs between shards:

```cpp
// On source shard
auto serialized = manager.exportLoRA("math-lora");

// Transfer to target shard (e.g., via RPC)
rpc_client.transferLoRA(target_shard, serialized);

// On target shard
bool imported = manager.importLoRA(
    "math-lora",
    serialized,
    "llama-2-7b"  // Base model ID
);
```

## Memory Management

### LRU Cache Behavior

The manager automatically evicts least-recently-used adapters when:
- Number of loaded adapters exceeds `max_lora_slots`
- VRAM usage exceeds `max_lora_vram_mb`

```cpp
// Check current memory usage
auto stats = manager.getStats();
std::cout << "Loaded: " << stats.loaded_count << "/" << stats.max_slots << "\n";
std::cout << "VRAM: " << stats.vram_used_mb << "/" << stats.vram_limit_mb << " MB\n";
std::cout << "Cache hits: " << stats.cache_hits << "\n";
std::cout << "Cache misses: " << stats.cache_misses << "\n";
std::cout << "Evictions: " << stats.evictions << "\n";
```

### Controlling Eviction

```cpp
// Pinned adapters won't be evicted
manager.pinLoRA("critical-lora");

// Set custom TTL for specific adapter
manager.setLoRATTL("temporary-lora", std::chrono::minutes(10));

// Manual eviction
manager.evictLRU();  // Evict least recently used
```

## Integration with LLM Plugin

### llama.cpp Integration

```cpp
#include "llm/llamacpp_plugin.h"

LlamaCppPlugin::Config plugin_config;
plugin_config.n_gpu_layers = 32;
plugin_config.multi_lora_config.max_lora_slots = 8;
plugin_config.multi_lora_config.enable_multi_lora_batch = true;

LlamaCppPlugin plugin(plugin_config);

// Load base model
plugin.loadModel("/models/llama-2-7b.gguf");

// Load LoRA
plugin.loadLoRA("math-lora", "/models/lora/math.bin", 1.0f);

// Generate with LoRA
InferenceRequest request;
request.prompt = "Calculate 15 * 23";
request.lora_id = "math-lora";

auto response = plugin.generate(request);
std::cout << response.text << "\n";
```

## Performance Optimization

### Batch Size Tuning

```cpp
// Optimal batch sizes
config.max_loras_per_batch = 4;  // Don't mix too many LoRAs

// Group requests by LoRA for efficiency
std::map<std::string, std::vector<InferenceRequest>> grouped;
for (const auto& req : all_requests) {
    grouped[req.lora_id].push_back(req);
}
```

### VRAM Budget

```cpp
// Conservative for shared GPU
config.max_lora_vram_mb = 256;

// Aggressive for dedicated GPU
config.max_lora_vram_mb = 2048;

// Monitor and adjust
auto stats = manager.getStats();
if (stats.evictions > 100) {
    // Too many evictions, increase budget
    manager.updateConfig(new_config);
}
```

### Preloading

```cpp
// Preload frequently-used adapters at startup
std::vector<std::string> frequent_loras = {
    "general", "math", "code"
};

for (const auto& lora_id : frequent_loras) {
    manager.loadLoRA(lora_id, "/models/lora/" + lora_id + ".bin", 
                    "base-model", 1.0f);
    manager.pinLoRA(lora_id);  // Prevent eviction
}
```

## Monitoring

### Prometheus Metrics

```
# Cache performance
themis_lora_cache_hits_total
themis_lora_cache_misses_total
themis_lora_evictions_total

# Memory usage
themis_lora_vram_used_bytes
themis_lora_loaded_adapters

# Operations
themis_lora_switches_total
themis_lora_loads_total
themis_lora_fusion_total
```

### Grafana Dashboard

```promql
# Cache hit rate
rate(themis_lora_cache_hits_total[5m]) / 
(rate(themis_lora_cache_hits_total[5m]) + rate(themis_lora_cache_misses_total[5m]))

# VRAM utilization
themis_lora_vram_used_bytes / themis_lora_vram_limit_bytes

# Average adapter switches per second
rate(themis_lora_switches_total[1m])
```

## Troubleshooting

### High Cache Miss Rate

```cpp
// Increase cache size
config.max_lora_slots = 16;

// Increase VRAM budget
config.max_lora_vram_mb = 1024;

// Pin frequently-used adapters
for (const auto& lora_id : frequent_loras) {
    manager.pinLoRA(lora_id);
}
```

### Out of Memory

```cpp
// Reduce cache size
config.max_lora_slots = 4;

// Stricter VRAM limit
config.max_lora_vram_mb = 256;

// Shorter TTL
config.lora_ttl = std::chrono::minutes(10);

// Force cleanup
manager.clearCache();
```

### Slow Adapter Switching

```cpp
// Preload adapters
manager.loadLoRA("adapter1", path1, base, 1.0f);
manager.loadLoRA("adapter2", path2, base, 1.0f);

// Both are now hot in cache, switching is instant

// Check switching overhead
auto start = std::chrono::steady_clock::now();
manager.getLoRA("adapter2");
auto end = std::chrono::steady_clock::now();
auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
```

## Best Practices

1. **Pin frequently-used adapters** to avoid cache thrashing
2. **Group requests by LoRA** for batch inference efficiency
3. **Monitor cache hit rate** and adjust cache size accordingly
4. **Use fusion** for complex multi-skill tasks
5. **Set appropriate TTLs** based on usage patterns
6. **Test VRAM limits** before production deployment
7. **Profile adapter loading** times for your hardware
8. **Document adapter dependencies** and base models

## Examples

### Use Case 1: Customer Support Bot

```cpp
// Load domain-specific adapters
manager.loadLoRA("support-general", "/lora/support_general.bin", base, 1.0f);
manager.loadLoRA("support-technical", "/lora/support_tech.bin", base, 1.0f);
manager.loadLoRA("support-billing", "/lora/support_billing.bin", base, 1.0f);

// Route requests based on classification
std::string select_adapter(const std::string& query) {
    if (contains_technical_terms(query)) return "support-technical";
    if (contains_billing_terms(query)) return "support-billing";
    return "support-general";
}

InferenceRequest req;
req.prompt = customer_query;
req.lora_id = select_adapter(customer_query);

auto response = plugin.generate(req);
```

### Use Case 2: Multi-Language Translation

```cpp
// Load language-specific adapters
manager.loadLoRA("translate-en-de", "/lora/en_de.bin", base, 1.0f);
manager.loadLoRA("translate-en-fr", "/lora/en_fr.bin", base, 1.0f);
manager.loadLoRA("translate-en-es", "/lora/en_es.bin", base, 1.0f);

// Batch translate to multiple languages
std::vector<std::pair<InferenceRequest, std::string>> batch;
for (const auto& lang : {"de", "fr", "es"}) {
    InferenceRequest req;
    req.prompt = "Translate to " + std::string(lang) + ": " + text;
    req.lora_id = "translate-en-" + std::string(lang);
    batch.push_back({req, req.lora_id});
}

auto translations = manager.batchInferenceMultiLoRA(batch, context);
```

## Next Steps

- Review [Multi-LoRA Integration](../de/connectors/VLLM_MULTI_LORA_INTEGRATION.md)
- Check [LoRA Training Framework](../de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md)
- Read [Full Implementation Report](../../RAID_LORA_IMPLEMENTATION_REPORT.md)
- Explore [Benchmark Results](../../benchmarks/bench_raid_lora.cpp)

## References

- LoRA Paper: https://arxiv.org/abs/2106.09685
- vLLM Multi-LoRA: https://docs.vllm.ai/
- llama.cpp: https://github.com/ggerganov/llama.cpp
