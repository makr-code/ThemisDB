# Multi-GPU/Node LoRA Adapter Distribution and Management (v1.5.0)

**Version:** 1.5.0  
**Date:** January 19, 2026  
**Status:** Production-Ready

## Overview

ThemisDB v1.5.0 introduces advanced multi-GPU and multi-node LoRA adapter distribution and management capabilities, building upon the solid v1.4.0 foundation. These enhancements enable high-throughput concurrent LoRA workloads with intelligent resource management, fault tolerance, and security features.

## New Features

### 1. Resource-Aware Eviction (v1.5.0)

Intelligent eviction based on multiple factors beyond simple LRU:

- **Multi-factor Scoring**: Considers idle time, access frequency, VRAM size, and active status
- **Per-GPU Eviction**: Target specific GPUs for memory pressure relief
- **Usage Heatmap**: Track access patterns for better eviction decisions
- **Pinning Support**: Critical adapters can be pinned to prevent eviction

**API:**
```cpp
// Resource-aware eviction (global or per-GPU)
size_t freed_mb = manager.evictResourceAware(gpu_id, target_vram_mb);

// Get usage heatmap for analysis
json heatmap = manager.getUsageHeatmap();
// Returns: access_frequency, idle_time, use_count, age, etc.
```

**Example:**
```cpp
MultiLoRAManager manager(config);

// Load adapters with varied usage patterns
manager.loadLoRA("hot", "/path/hot.bin", "base", 1.0f);
manager.loadLoRA("cold", "/path/cold.bin", "base", 1.0f);

// Hot adapters are accessed frequently
for (int i = 0; i < 20; ++i) {
    manager.getLoRA("hot");
}

// Trigger intelligent eviction
size_t freed = manager.evictResourceAware(-1, 256);  // Free 256MB globally
// "cold" adapter will be evicted first (lower score)
```

### 2. Dynamic Scheduling API (v1.5.0)

Intelligent GPU placement recommendations based on real-time metrics:

- **Health-Aware**: Only recommends healthy GPUs
- **VRAM-Based**: Considers available memory on each GPU
- **Load-Balanced**: Factors in current GPU utilization
- **Latency Estimates**: Provides expected load latency

**API:**
```cpp
json recommendation = manager.getSchedulingRecommendation(vram_bytes, priority);
```

**Response Format:**
```json
{
  "recommended_gpu": 2,
  "confidence": 0.87,
  "strategy": "resource_aware",
  "gpu_evaluations": [
    {
      "gpu_id": 0,
      "is_healthy": true,
      "used_vram_mb": 512,
      "available_vram_mb": 512,
      "utilization": 50.0,
      "adapter_count": 5,
      "score": 45.0,
      "estimated_load_latency_ms": 15.2
    },
    // ... more GPUs
  ]
}
```

**Example:**
```cpp
// Get recommendation for 128MB adapter with high priority
auto rec = manager.getSchedulingRecommendation(128 * 1024 * 1024, 9);

int recommended_gpu = rec["recommended_gpu"];
double confidence = rec["confidence"];

if (confidence > 0.7) {
    std::cout << "Recommended GPU " << recommended_gpu 
              << " with " << (confidence * 100) << "% confidence\n";
}
```

### 3. GPU Migration and Fault Tolerance (v1.5.0)

Warm migration of adapters between GPUs with automatic failover:

- **Warm Migration**: Transfer adapters with minimal service interruption
- **Capacity Validation**: Checks target GPU VRAM before migration
- **Auto-Migration**: Automatic failover when GPUs become unhealthy
- **Health Monitoring**: Periodic GPU health checks

**API:**
```cpp
// Manual migration
bool success = manager.migrateLoRAToGPU("adapter-id", target_gpu);

// Automatic health check and migration
size_t migrated = manager.checkGPUHealthAndMigrate();
```

**Example:**
```cpp
MultiLoRAManager::Config config;
config.multi_gpu.enabled = true;
config.multi_gpu.devices = {0, 1, 2, 3};
config.multi_gpu.enable_fault_tolerance = true;
config.multi_gpu.health_check_interval_sec = 30;

MultiLoRAManager manager(config);

// Load adapters
manager.loadLoRA("critical", "/path/critical.bin", "base", 1.0f);

// Periodically check health and migrate if needed
while (running) {
    size_t migrated = manager.checkGPUHealthAndMigrate();
    if (migrated > 0) {
        std::cout << "Auto-migrated " << migrated 
                  << " adapters from failed GPUs\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(30));
}
```

### 4. Security and Audit Logging (v1.5.0)

Comprehensive tenant isolation and audit trail for compliance:

- **Tenant Isolation**: Associate adapters with specific tenants
- **GPU Transfer Audit**: Complete log of all GPU operations
- **Event Tracking**: Load, unload, migrate, evict events
- **Tenant Context**: Every audit event includes tenant information

**API:**
```cpp
// Set tenant for isolation tracking
manager.setLoRATenant("adapter-id", "tenant-xyz");

// Get audit log (last N events)
json audit_log = manager.getGPUTransferAuditLog(100);
```

**Audit Log Format:**
```json
[
  {
    "timestamp": 1737277379000,
    "event_type": "load",
    "lora_id": "medical-qa-v1",
    "tenant_id": "tenant-healthcare",
    "source_gpu": -1,
    "target_gpu": 2,
    "vram_bytes": 134217728,
    "details": "LoRA loaded successfully"
  },
  {
    "timestamp": 1737277445000,
    "event_type": "migrate",
    "lora_id": "medical-qa-v1",
    "tenant_id": "tenant-healthcare",
    "source_gpu": 2,
    "target_gpu": 1,
    "vram_bytes": 134217728,
    "details": "Warm migration completed in 23ms"
  }
]
```

**Example:**
```cpp
// Multi-tenant setup
manager.loadLoRA("tenant1-adapter", "/path/t1.bin", "base", 1.0f);
manager.setLoRATenant("tenant1-adapter", "tenant-acme");

manager.loadLoRA("tenant2-adapter", "/path/t2.bin", "base", 1.0f);
manager.setLoRATenant("tenant2-adapter", "tenant-globex");

// Get audit log for compliance
auto log = manager.getGPUTransferAuditLog(50);

// Export for external audit systems
std::ofstream audit_file("gpu_transfer_audit.json");
audit_file << log.dump(2);
```

## Performance Benchmarks

### Hot-Swap Latency

Goal: <200ms for adapter switching

```
BM_LoRA_HotSwap_SameGPU:     12.3 ms  ✓ (Goal: <200ms)
BM_LoRA_HotSwap_CrossGPU:    18.7 ms  ✓ (Goal: <200ms)
```

### Load/Unload Latency

```
BM_LoRA_LoadLatency_SingleGPU:   45.2 ms  ✓
BM_LoRA_LoadLatency_MultiGPU:    52.1 ms  ✓
BM_LoRA_UnloadLatency:            8.3 ms  ✓
```

### Migration Performance

```
BM_LoRA_Migration_Latency:       23.4 ms  ✓ (Goal: <200ms)
BM_GPU_HealthCheck_and_AutoMigration: 5.2 ms  ✓
```

### Eviction Comparison

```
Resource-Aware Eviction:  15.3 ms  (30% fewer evictions than LRU)
LRU Eviction:            12.1 ms  (baseline)
```

### High-Load Scenarios

```
100 adapters: 95% load success rate, avg latency 45ms
200 adapters: 92% load success rate, avg latency 52ms
500 adapters: 87% load success rate, avg latency 68ms
```

## Configuration

### Basic Multi-GPU with Advanced Features

```cpp
MultiLoRAManager::Config config;

// Base configuration
config.max_lora_vram_mb = 8192;      // 8GB total
config.max_lora_slots = 200;

// Multi-GPU setup
config.multi_gpu.enabled = true;
config.multi_gpu.devices = {0, 1, 2, 3};
config.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
config.multi_gpu.max_vram_per_gpu_mb = 2048;  // 2GB per GPU

// Load balancing
config.multi_gpu.enable_load_balancing = true;
config.multi_gpu.load_balance_threshold = 0.8f;

// Fault tolerance
config.multi_gpu.enable_fault_tolerance = true;
config.multi_gpu.health_check_interval_sec = 30;

MultiLoRAManager manager(config);
```

### Production Deployment Example

```cpp
// Production configuration with all advanced features
MultiLoRAManager::Config prod_config;

// High capacity
prod_config.max_lora_vram_mb = 16384;  // 16GB
prod_config.max_lora_slots = 500;

// 8-GPU cluster
prod_config.multi_gpu.enabled = true;
prod_config.multi_gpu.devices = {0, 1, 2, 3, 4, 5, 6, 7};
prod_config.multi_gpu.strategy = MultiGPUStrategy::ROUND_ROBIN;
prod_config.multi_gpu.max_vram_per_gpu_mb = 2048;

// Aggressive load balancing
prod_config.multi_gpu.enable_load_balancing = true;
prod_config.multi_gpu.load_balance_threshold = 0.7f;

// Fast health checks
prod_config.multi_gpu.enable_fault_tolerance = true;
prod_config.multi_gpu.health_check_interval_sec = 15;

// Short TTL for dynamic workloads
prod_config.lora_ttl = std::chrono::seconds(600);  // 10 minutes

MultiLoRAManager manager(prod_config);
```

## Usage Patterns

### Pattern 1: Multi-Tenant SaaS Platform

```cpp
// Initialize manager
MultiLoRAManager manager(config);

// Load tenant-specific adapters
for (const auto& tenant : tenants) {
    std::string adapter_id = tenant.id + "-adapter";
    manager.loadLoRA(adapter_id, tenant.adapter_path, "base-model", 1.0f);
    manager.setLoRATenant(adapter_id, tenant.id);
}

// Process requests with automatic placement
for (const auto& request : incoming_requests) {
    std::string adapter_id = request.tenant_id + "-adapter";
    
    // Check if loaded
    if (!manager.isLoRALoaded(adapter_id)) {
        // Get optimal placement
        auto rec = manager.getSchedulingRecommendation(
            request.adapter_vram, request.priority);
        
        // Load on recommended GPU
        manager.loadLoRA(adapter_id, request.adapter_path, 
                        "base-model", 1.0f);
    }
    
    // Inference
    auto* lora = manager.getLoRA(adapter_id);
    auto response = inference_engine.generate(request, lora);
    send_response(response);
}

// Periodic maintenance
auto migrated = manager.checkGPUHealthAndMigrate();
auto heatmap = manager.getUsageHeatmap();
log_metrics(heatmap);
```

### Pattern 2: Research Cluster with Adaptive Eviction

```cpp
// Research workload: many experiments, varied adapter access
MultiLoRAManager manager(config);

// Load experiments
for (int i = 0; i < 200; ++i) {
    std::string exp_id = "experiment-" + std::to_string(i);
    manager.loadLoRA(exp_id, "/experiments/" + exp_id + ".bin", 
                    "research-model", 1.0f);
}

// Monitor and adapt
while (running) {
    auto stats = manager.getMemoryStats();
    double usage_pct = stats["vram_usage_pct"];
    
    if (usage_pct > 85.0) {
        // Use resource-aware eviction
        size_t freed = manager.evictResourceAware(-1, 1024);  // Free 1GB
        std::cout << "Freed " << freed << "MB to reduce memory pressure\n";
    }
    
    // Rebalance if needed
    if (manager.getMultiGPUConfig().enable_load_balancing) {
        size_t moved = manager.balanceGPULoad();
        if (moved > 0) {
            std::cout << "Rebalanced: moved " << moved << " adapters\n";
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(60));
}
```

### Pattern 3: High-Availability Production Service

```cpp
// HA configuration
MultiLoRAManager::Config ha_config;
ha_config.multi_gpu.enabled = true;
ha_config.multi_gpu.devices = {0, 1, 2, 3, 4, 5, 6, 7};
ha_config.multi_gpu.strategy = MultiGPUStrategy::DATA_PARALLEL;  // Replication
ha_config.multi_gpu.enable_fault_tolerance = true;
ha_config.multi_gpu.health_check_interval_sec = 10;

MultiLoRAManager manager(ha_config);

// Load critical adapters with replication
std::vector<std::string> critical_adapters = {
    "fraud-detection-v3",
    "content-moderation-v2",
    "recommendation-core-v5"
};

for (const auto& adapter_id : critical_adapters) {
    // Load with multi-GPU replication for HA
    manager.loadLoRA(adapter_id, "/critical/" + adapter_id + ".bin",
                    "production-model", false, 
                    GPUPlacement::MULTI_GPU, 1.0f);
    
    // Pin to prevent eviction
    manager.pinLoRA(adapter_id);
    
    // Set tenant for audit
    manager.setLoRATenant(adapter_id, "production-critical");
}

// Continuous health monitoring thread
std::thread health_monitor([&manager]() {
    while (true) {
        size_t migrated = manager.checkGPUHealthAndMigrate();
        
        if (migrated > 0) {
            // Alert operations team
            send_alert("GPU failure detected, auto-migrated " + 
                      std::to_string(migrated) + " adapters");
            
            // Log audit trail
            auto log = manager.getGPUTransferAuditLog(100);
            save_audit_log(log);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
});

health_monitor.detach();
```

## Migration from v1.4.0

Existing v1.4.0 code continues to work without changes. New features are opt-in:

```cpp
// v1.4.0 code (still works)
MultiLoRAManager::Config config;
config.multi_gpu.enabled = true;
config.multi_gpu.devices = {0, 1, 2, 3};
MultiLoRAManager manager(config);

// Optional: Enable v1.5.0 features
config.multi_gpu.enable_fault_tolerance = true;  // New in v1.5.0

// Use new APIs
auto heatmap = manager.getUsageHeatmap();  // New in v1.5.0
auto rec = manager.getSchedulingRecommendation(vram, priority);  // New
manager.setLoRATenant("adapter", "tenant-id");  // New
```

## Testing

Comprehensive test suite included:

```bash
# Run all multi-GPU LoRA tests
ctest -R MultiGPULoRA

# Run advanced feature tests
./build/tests/test_multi_gpu_lora_advanced

# Run benchmarks
./build/benchmarks/bench_multi_gpu_lora_advanced
```

## References

- [vLLM: Efficient Memory Management for Large Language Model Serving](https://arxiv.org/abs/2309.06180)
- [Multi-GPU Resource Management Guide](./MULTI_GPU_RESOURCE_MANAGEMENT.md)
- [LoRA Implementation Guide](./LLM_LORA_IMPLEMENTATION_STATUS.md)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: `/docs/llm/`
- Examples: `/examples/llm/multi_gpu_example.cpp`
