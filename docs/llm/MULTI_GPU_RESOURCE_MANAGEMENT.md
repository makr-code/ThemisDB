# Multi-GPU Distribution and Resource Management Guide

**Version:** 1.0  
**Date:** January 19, 2026  
**Status:** Production-Ready

## Overview

ThemisDB implements comprehensive multi-GPU support for LLM inference and LoRA adapter management, including:

- **Tensor Parallelism**: Distribute model layers across multiple GPUs
- **Dynamic Load Balancing**: Automatically balance adapter placement across GPUs
- **Health Monitoring**: Continuous GPU health checks with automatic failover
- **Persistent Pinning**: Keep critical models/adapters in memory with priority protection
- **JIT Eviction**: Intelligent LRU-based eviction when VRAM is constrained

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│            Multi-GPU Resource Management                 │
└─────────────────────────────────────────────────────────┘
                           │
         ┌─────────────────┴─────────────────┐
         │                                    │
┌────────▼────────┐                  ┌───────▼────────┐
│ GPUMemoryManager│                  │AdapterLoadBalancer│
│                 │                  │                 │
│ - Per-GPU stats │◄────────────────►│ - Placement     │
│ - Health checks │                  │ - Migration     │
│ - VRAM tracking │                  │ - Eviction      │
└────────┬────────┘                  └───────┬────────┘
         │                                    │
         ▼                                    ▼
┌─────────────────────────────────────────────────────────┐
│               GPU 0    GPU 1    GPU 2    GPU 3          │
│              ┌────┐  ┌────┐  ┌────┐  ┌────┐            │
│              │Base│  │Base│  │LoRA│  │LoRA│            │
│              │Mdl │  │Mdl │  │ A1 │  │ A2 │            │
│              └────┘  └────┘  └────┘  └────┘            │
└─────────────────────────────────────────────────────────┘
```

## Configuration

### 1. Basic Multi-GPU Setup

```cpp
#include "llm/llama_resource_manager.h"
#include "llm/gpu_memory_manager.h"
#include "llm/adapter_load_balancer.h"

// Configure multi-GPU backend
themis::llm::GPUBackendConfig config;

// GPU Device Selection
config.primary_gpu_id = 0;
config.secondary_gpus = {1, 2, 3};  // Additional GPUs for distribution

// Tensor Parallelism Mode
config.tensor_parallel_mode = GPUBackendConfig::TensorParallelismMode::HYBRID;
config.tensor_split_ratio = 0.5f;  // 50/50 split

// Enable peer-to-peer for fast GPU-to-GPU transfers
config.enable_peer_to_peer = true;
```

### 2. Dynamic Load Balancing Configuration

```cpp
// Enable automatic load balancing
config.enable_dynamic_load_balancing = true;
config.load_balance_threshold = 0.8f;      // Rebalance at 80% utilization
config.load_balance_interval_ms = 5000;    // Check every 5 seconds

// JIT Eviction for LoRA Adapters
config.enable_jit_eviction = true;
config.adapter_cache_size = 10;            // Max 10 adapters per GPU
config.eviction_threshold = 0.9f;          // Evict when VRAM > 90%
```

### 3. GPU Health Monitoring

```cpp
// Enable health checks
config.enable_health_checks = true;
config.health_check_interval_ms = 10000;         // Check every 10 seconds
config.max_gpu_temperature_celsius = 85.0f;      // Alert at 85°C
config.max_gpu_utilization = 0.95f;              // Alert at 95% utilization
config.auto_failover_on_error = true;            // Auto-failover to healthy GPUs
```

### 4. Persistent Pinning

```cpp
// Pin critical models and adapters
config.enable_persistent_pinning = true;
config.pinned_model_ids = {"mistral-7b-base", "llama-3-8b"};
config.pinned_adapter_ids = {"legal-qa-v1", "medical-diagnosis-v1"};
config.pinned_resource_priority = 10;  // High priority (0-10 scale)
```

## Usage Examples

### Example 1: Initialize Multi-GPU System

```cpp
#include "llm/llama_resource_manager.h"
#include "llm/gpu_memory_manager.h"
#include "llm/adapter_load_balancer.h"

// 1. Configure GPU memory manager
themis::llm::GPUMemoryManager::Config mem_config;
mem_config.enable_multi_gpu = true;
mem_config.gpu_devices = {0, 1, 2, 3};
mem_config.enable_peer_access = true;
mem_config.max_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24 GB per GPU

auto memory_manager = std::make_shared<themis::llm::GPUMemoryManager>(mem_config);

// 2. Configure load balancer
themis::llm::AdapterLoadBalancer::Config lb_config;
lb_config.enable_dynamic_balancing = true;
lb_config.enable_jit_eviction = true;
lb_config.max_adapters_per_gpu = 10;
lb_config.rebalance_threshold = 0.8f;

auto load_balancer = std::make_shared<themis::llm::AdapterLoadBalancer>(
    memory_manager, lb_config);

// 3. Configure backend
themis::llm::GPUBackendConfig gpu_config;
gpu_config.primary_gpu_id = 0;
gpu_config.secondary_gpus = {1, 2, 3};
gpu_config.tensor_parallel_mode = GPUBackendConfig::TensorParallelismMode::TENSOR;
gpu_config.enable_dynamic_load_balancing = true;
gpu_config.enable_health_checks = true;
gpu_config.enable_persistent_pinning = true;

// 4. Load model with multi-GPU support
llama_model_params model_params = llama_model_default_params();
model_params.n_gpu_layers = -1;  // All layers on GPU

themis::llm::BackendAwareLlamaModelHandle model(
    "/path/to/model.gguf",
    model_params,
    gpu_config
);

std::cout << "Model loaded on " << model.gpu_devices().size() << " GPUs" << std::endl;
```

### Example 2: Place LoRA Adapters with Load Balancing

```cpp
// Place adapters dynamically across GPUs
std::vector<std::string> adapter_ids = {
    "legal-qa-v1", "medical-v1", "code-gen-v1", 
    "chat-v1", "translation-v1"
};

for (const auto& adapter_id : adapter_ids) {
    size_t vram_bytes = 256 * 1024 * 1024;  // 256 MB per adapter
    int priority = 5;  // Medium priority
    
    // Select optimal GPU for adapter
    int gpu_id = load_balancer->selectGPUForAdapter(adapter_id, vram_bytes, priority);
    
    if (gpu_id >= 0) {
        // Place adapter on selected GPU
        bool success = load_balancer->placeAdapter(
            adapter_id, gpu_id, vram_bytes, priority);
        
        if (success) {
            std::cout << "Adapter " << adapter_id 
                      << " placed on GPU " << gpu_id << std::endl;
        }
    } else {
        std::cerr << "No suitable GPU found for adapter " << adapter_id << std::endl;
    }
}
```

### Example 3: Monitor GPU Health and Statistics

```cpp
// Get per-GPU statistics
auto all_gpu_stats = memory_manager->getAllGPUStats();

for (const auto& stats : all_gpu_stats) {
    std::cout << "GPU " << stats.device_id << ":" << std::endl;
    std::cout << "  VRAM Usage: " << (stats.used_vram_bytes / (1024.0 * 1024.0)) 
              << " / " << (stats.total_vram_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
    std::cout << "  Utilization: " << stats.utilization_percent << "%" << std::endl;
    std::cout << "  Temperature: " << stats.temperature_celsius << "°C" << std::endl;
    std::cout << "  Health: " << (stats.is_healthy ? "Healthy" : "Unhealthy") << std::endl;
    std::cout << "  Loaded Models: " << stats.loaded_models.size() << std::endl;
    std::cout << "  Loaded Adapters: " << stats.loaded_adapters.size() << std::endl;
}

// Check if load rebalancing is needed
if (memory_manager->needsLoadRebalancing(0.3f)) {
    std::cout << "Load imbalance detected, triggering rebalancing..." << std::endl;
    load_balancer->rebalance();
}
```

### Example 4: Pin Critical Adapters

```cpp
// Pin high-priority adapters to prevent eviction
std::vector<std::string> critical_adapters = {
    "legal-qa-v1", "medical-diagnosis-v1"
};

for (const auto& adapter_id : critical_adapters) {
    // Place with high priority
    int gpu_id = load_balancer->selectGPUForAdapter(adapter_id, 256 * 1024 * 1024, 10);
    load_balancer->placeAdapter(adapter_id, gpu_id, 256 * 1024 * 1024, 10, true);  // pinned=true
    
    std::cout << "Critical adapter " << adapter_id << " pinned on GPU " << gpu_id << std::endl;
}

// Verify pinning status
for (const auto& adapter_id : critical_adapters) {
    if (load_balancer->isAdapterPinned(adapter_id)) {
        std::cout << adapter_id << " is pinned and protected from eviction" << std::endl;
    }
}
```

### Example 5: Handle GPU Failures with Automatic Failover

```cpp
// Simulate GPU failure detection
int failing_gpu = 2;

// Health monitoring will detect the issue
auto health = memory_manager->getGPUHealth(failing_gpu);

if (!health.is_healthy) {
    std::cout << "GPU " << failing_gpu << " is unhealthy: " << health.last_error << std::endl;
    
    // Get adapters on failing GPU
    auto adapters_on_gpu = load_balancer->getGPUAdapters(failing_gpu);
    
    std::cout << "Migrating " << adapters_on_gpu.size() 
              << " adapters to healthy GPUs..." << std::endl;
    
    // Migrate to healthy GPUs
    for (const auto& adapter_id : adapters_on_gpu) {
        // Skip pinned adapters if they can't be migrated
        if (load_balancer->isAdapterPinned(adapter_id)) {
            std::cout << "Skipping pinned adapter: " << adapter_id << std::endl;
            continue;
        }
        
        // Find least loaded healthy GPU
        int target_gpu = memory_manager->getLeastLoadedGPU();
        
        if (target_gpu >= 0 && target_gpu != failing_gpu) {
            bool success = load_balancer->migrateAdapter(adapter_id, target_gpu);
            if (success) {
                std::cout << "  Migrated " << adapter_id 
                          << " to GPU " << target_gpu << std::endl;
            }
        }
    }
}
```

### Example 6: Periodic Load Balancing Loop

```cpp
#include <thread>
#include <chrono>

// Background thread for periodic load balancing
void loadBalancingLoop(
    std::shared_ptr<themis::llm::AdapterLoadBalancer> load_balancer,
    int interval_ms) {
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        
        // Get load balancing statistics
        auto stats = load_balancer->getStats();
        
        std::cout << "=== Load Balancing Stats ===" << std::endl;
        std::cout << "Adapters: " << stats.num_adapters << std::endl;
        std::cout << "GPUs: " << stats.num_gpus << std::endl;
        std::cout << "Avg Load: " << (stats.average_gpu_load * 100) << "%" << std::endl;
        std::cout << "Max Load: " << (stats.max_gpu_load * 100) << "%" << std::endl;
        std::cout << "Min Load: " << (stats.min_gpu_load * 100) << "%" << std::endl;
        std::cout << "Migrations: " << stats.num_migrations << std::endl;
        std::cout << "Evictions: " << stats.num_evictions << std::endl;
        
        // Trigger rebalancing if needed
        if (stats.max_gpu_load - stats.min_gpu_load > 0.3f) {
            std::cout << "Load imbalance detected, rebalancing..." << std::endl;
            load_balancer->rebalance();
        }
    }
}

// Start background thread
std::thread lb_thread(loadBalancingLoop, load_balancer, 5000);
lb_thread.detach();
```

## Tensor Parallelism Modes

### NONE (Single GPU)
- Default mode for single GPU systems
- All layers on primary GPU
- No distribution

### PIPELINE (Layer-wise Distribution)
```
GPU 0: Layers 0-9
GPU 1: Layers 10-19
GPU 2: Layers 20-29
GPU 3: Layers 30-39
```
- Sequential processing through GPUs
- Good for memory-constrained scenarios
- Moderate parallelism

### TENSOR (Split Tensors)
```
GPU 0: 25% of each layer
GPU 1: 25% of each layer
GPU 2: 25% of each layer
GPU 3: 25% of each layer
```
- Maximum parallelism
- Requires high GPU-to-GPU bandwidth
- Best for large models

### HYBRID (Pipeline + Tensor)
```
GPU 0-1: Layers 0-19 (tensor split)
GPU 2-3: Layers 20-39 (tensor split)
```
- Balanced approach
- Good for 4+ GPUs
- Optimal for most scenarios

## Performance Tuning

### 1. Optimize Tensor Split Ratio

```cpp
// For models with uneven layer sizes
config.tensor_split_ratio = 0.6f;  // 60/40 split instead of 50/50
```

### 2. Adjust Cache Size per GPU

```cpp
// More adapters on GPUs with more VRAM
config.adapter_cache_size = 15;  // Increase from default 10
```

### 3. Fine-tune Eviction Threshold

```cpp
// More aggressive eviction
config.eviction_threshold = 0.85f;  // Evict at 85% instead of 90%
```

### 4. Balance Check Interval

```cpp
// More frequent rebalancing for dynamic workloads
config.load_balance_interval_ms = 3000;  // Check every 3 seconds
```

## Monitoring and Metrics

### Key Metrics to Track

1. **Per-GPU VRAM Usage**: `memory_manager->getGPUStats(gpu_id).used_vram_bytes`
2. **GPU Utilization**: `memory_manager->getGPUStats(gpu_id).utilization_percent`
3. **GPU Temperature**: `memory_manager->getGPUStats(gpu_id).temperature_celsius`
4. **Adapter Count per GPU**: `load_balancer->getGPUAdapters(gpu_id).size()`
5. **Migration Count**: `load_balancer->getStats().num_migrations`
6. **Eviction Count**: `load_balancer->getStats().num_evictions`

### Integration with Prometheus/Grafana

```cpp
// Export metrics for Prometheus
void exportMetrics(
    std::shared_ptr<themis::llm::GPUMemoryManager> memory_manager,
    std::shared_ptr<themis::llm::AdapterLoadBalancer> load_balancer) {
    
    auto all_stats = memory_manager->getAllGPUStats();
    
    for (const auto& stats : all_stats) {
        // Prometheus metric format
        std::cout << "gpu_vram_used_bytes{gpu=\"" << stats.device_id << "\"} " 
                  << stats.used_vram_bytes << std::endl;
        std::cout << "gpu_utilization_percent{gpu=\"" << stats.device_id << "\"} " 
                  << stats.utilization_percent << std::endl;
        std::cout << "gpu_temperature_celsius{gpu=\"" << stats.device_id << "\"} " 
                  << stats.temperature_celsius << std::endl;
        std::cout << "gpu_adapter_count{gpu=\"" << stats.device_id << "\"} " 
                  << stats.loaded_adapters.size() << std::endl;
    }
    
    auto lb_stats = load_balancer->getStats();
    std::cout << "adapter_migrations_total " << lb_stats.num_migrations << std::endl;
    std::cout << "adapter_evictions_total " << lb_stats.num_evictions << std::endl;
    std::cout << "gpu_load_average " << lb_stats.average_gpu_load << std::endl;
}
```

## Best Practices

### 1. Start with Conservative Settings
- Begin with single GPU mode
- Gradually enable multi-GPU features
- Monitor performance at each step

### 2. Pin Critical Adapters Early
- Identify high-priority adapters
- Pin them immediately after loading
- Set appropriate priority levels (8-10 for critical)

### 3. Monitor GPU Health Continuously
- Check health metrics every 10 seconds
- Set reasonable temperature thresholds (< 85°C)
- Enable auto-failover for production systems

### 4. Balance Load Proactively
- Don't wait for severe imbalances
- Use threshold of 0.2-0.3 (20-30% difference)
- Rebalance every 5-10 seconds

### 5. Test Failover Scenarios
- Simulate GPU failures
- Verify adapter migration works
- Ensure pinned adapters are handled correctly

## Troubleshooting

### Issue: Adapters Not Migrating

**Symptoms**: Adapters remain on overloaded GPU despite rebalancing

**Solutions**:
1. Check if adapters are pinned: `load_balancer->isAdapterPinned(adapter_id)`
2. Verify target GPU has enough free VRAM
3. Check if migration is enabled: `config.enable_migration = true`

### Issue: Frequent Evictions

**Symptoms**: High eviction count, poor performance

**Solutions**:
1. Increase adapter cache size: `config.adapter_cache_size = 15`
2. Raise eviction threshold: `config.eviction_threshold = 0.95f`
3. Add more GPUs to distribute load
4. Pin frequently-used adapters

### Issue: GPU Marked Unhealthy Incorrectly

**Symptoms**: Healthy GPU marked as unhealthy

**Solutions**:
1. Adjust temperature threshold: `config.max_gpu_temperature_celsius = 90.0f`
2. Increase utilization threshold: `config.max_gpu_utilization = 0.98f`
3. Check for actual hardware issues
4. Manually reset health status: `memory_manager->markGPUHealthy(gpu_id)`

## API Reference

### GPUMemoryManager

```cpp
// Statistics
GPUStats getGPUStats(int gpu_device_id) const;
std::vector<GPUStats> getAllGPUStats() const;

// Health Monitoring
GPUHealth getGPUHealth(int gpu_device_id) const;
bool isGPUHealthy(int gpu_device_id) const;
void markGPUUnhealthy(int gpu_device_id, const std::string& reason);
void markGPUHealthy(int gpu_device_id);

// Load Balancing
int getLeastLoadedGPU() const;
std::vector<int> getHealthyGPUs() const;
float getAverageGPULoad() const;
bool needsLoadRebalancing(float threshold) const;
```

### AdapterLoadBalancer

```cpp
// Placement
int selectGPUForAdapter(const std::string& adapter_id, size_t vram_bytes, int priority);
bool placeAdapter(const std::string& adapter_id, int gpu_device_id, size_t vram_bytes, 
                  int priority, bool pinned = false);
bool removeAdapter(const std::string& adapter_id);

// Queries
int getAdapterGPU(const std::string& adapter_id) const;
std::vector<std::string> getGPUAdapters(int gpu_device_id) const;
bool isAdapterLoaded(const std::string& adapter_id) const;

// Pinning
bool pinAdapter(const std::string& adapter_id);
bool unpinAdapter(const std::string& adapter_id);
bool isAdapterPinned(const std::string& adapter_id) const;

// Load Balancing
bool rebalance();
bool migrateAdapter(const std::string& adapter_id, int target_gpu_id);
std::vector<std::string> evictLRUAdapters(int gpu_device_id, size_t required_bytes);

// Access Tracking
void recordAccess(const std::string& adapter_id);

// Statistics
LoadBalanceStats getStats() const;
float getGPULoad(int gpu_device_id) const;
```

## Related Documentation

- [LoRA Training Framework Integration](LORA_TRAINING_FRAMEWORK_INTEGRATION.md)
- [GPU Memory Manager Implementation](GPU_MEMORY_MANAGER.md)
- [llama.cpp Resource Manager](LLAMA_RESOURCE_MANAGER.md)
- [Multi-GPU Benchmarks](../benchmarks/MULTI_GPU_PERFORMANCE.md)

## Changelog

### Version 1.0 (January 19, 2026)
- Initial implementation
- Multi-GPU distribution with tensor parallelism
- Dynamic load balancing
- GPU health monitoring
- Persistent pinning
- JIT eviction with LRU policy

## License

Copyright © 2026 ThemisDB Project. All rights reserved.
