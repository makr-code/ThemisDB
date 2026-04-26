# Multi-LoRA Adapter Fusion Guide

## Overview

ThemisDB's Multi-LoRA Adapter Fusion feature enables dynamic composition of multiple LoRA adapters with advanced scheduling, caching, and performance optimization. This guide covers the complete API and usage patterns.

## Table of Contents

1. [Core Concepts](#core-concepts)
2. [Fusion Strategies](#fusion-strategies)
3. [API Reference](#api-reference)
4. [Usage Examples](#usage-examples)
5. [Performance Considerations](#performance-considerations)
6. [Best Practices](#best-practices)

## Core Concepts

### Adapter Fusion

Adapter fusion combines multiple LoRA adapters into a single effective adapter using weighted blending:

```
W_fused = α₁ * LoRA₁ + α₂ * LoRA₂ + ... + αₙ * LoRAₙ
```

Where:
- `W_fused` is the resulting fused adapter weights
- `LoRAᵢ` are the source adapter weights
- `αᵢ` are the blend weights (normalized to sum to 1.0)

### Fusion Cache

The fusion cache stores computed fused adapters to avoid repeated computation. Cache entries include:
- Source adapter IDs
- Blend weights
- Creation timestamp
- Last used timestamp
- Usage statistics

### Compatibility Checks

Before fusion, the system validates:
- **Base Model Match**: All adapters must be for the same base model
- **Quantization Match** (optional): Quantization modes must match
- **GPU Placement Match** (optional): GPU placement strategies must match
- **Rank Match** (optional): LoRA ranks must match

## Fusion Strategies

### 1. STATIC Fusion

Fixed weights, ideal for production deployments with consistent blend ratios.

**Use Cases:**
- Production serving with a fixed adapter combination
- Multi-domain models with static expertise distribution
- Long-running deployments with stable requirements

**Characteristics:**
- Weights cannot be changed after creation
- Highest cache efficiency
- Lowest runtime overhead

### 2. DYNAMIC Fusion

Runtime-adjustable weights for interactive experimentation and tuning.

**Use Cases:**
- Development and experimentation
- Interactive weight tuning
- Real-time adaptation based on feedback

**Characteristics:**
- Weights can be updated via `updateFusionWeights()`
- Cache invalidation on weight updates
- Moderate runtime overhead

### 3. SCHEDULED Fusion

Time-varying weights using alpha scheduling for A/B testing and smooth transitions.

**Use Cases:**
- A/B testing between adapter versions
- Gradual migration from one adapter to another
- Performance-based adaptive blending
- Circadian or time-based adapter selection

**Characteristics:**
- Weights computed from schedule function
- Supports custom scheduling logic
- Automatic weight interpolation

## API Reference

### Core Fusion Functions

#### `fuseLoRAsAdvanced`

```cpp
bool fuseLoRAsAdvanced(
    const std::string& fused_id,
    const FusionConfig& config
);
```

Create a fused adapter with advanced configuration.

**Parameters:**
- `fused_id`: Unique identifier for the fused adapter
- `config`: Fusion configuration (see FusionConfig)

**Returns:** `true` if fusion succeeded, `false` otherwise

#### `updateFusionWeights`

```cpp
bool updateFusionWeights(
    const std::string& fusion_id,
    const std::vector<float>& new_weights
);
```

Update blend weights for a DYNAMIC fusion.

**Parameters:**
- `fusion_id`: ID of the fusion to update
- `new_weights`: New blend weights (must match number of source adapters)

**Returns:** `true` if weights updated, `false` if fusion not found or not DYNAMIC

#### `setAlphaSchedule`

```cpp
bool setAlphaSchedule(
    const std::string& fusion_id,
    const AlphaSchedule& schedule
);
```

Configure alpha scheduling for SCHEDULED fusion.

**Parameters:**
- `fusion_id`: ID of the fusion to configure
- `schedule`: Alpha schedule configuration

**Returns:** `true` if schedule set successfully

### Cache Management

#### `invalidateFusionCache`

```cpp
bool invalidateFusionCache(const std::string& fusion_id);
```

Force recomputation of a fused adapter.

#### `clearFusionCache`

```cpp
size_t clearFusionCache();
```

Remove all cached fusion entries.

**Returns:** Number of entries cleared

#### `listFusionCache`

```cpp
std::vector<FusionCacheEntry> listFusionCache() const;
```

Get metadata for all cached fusions.

### Metrics and Monitoring

#### `getFusionMetrics`

```cpp
FusionMetrics getFusionMetrics() const;
```

Get comprehensive fusion performance metrics including:
- Total fusions performed
- Cache hit/miss rates
- Average fusion and inference times
- Per-strategy breakdown

#### `getCurrentFusionWeights`

```cpp
std::vector<float> getCurrentFusionWeights(const std::string& fusion_id) const;
```

Get current effective weights (accounts for scheduling).

### Compatibility Validation

#### `checkFusionCompatibility`

```cpp
bool checkFusionCompatibility(
    const std::vector<std::string>& lora_ids,
    const FusionConfig& config
) const;
```

Validate that adapters can be safely fused.

## Usage Examples

### Example 1: Static Fusion for Production

```cpp
#include "llm/multi_lora_manager.h"

// Initialize manager
MultiLoRAManager::Config config;
config.enable_adapter_fusion = true;
MultiLoRAManager manager(config);

// Load adapters
manager.loadLoRA("legal-qa", "/models/legal-qa.bin", "llama-7b", 1.0f);
manager.loadLoRA("medical-qa", "/models/medical-qa.bin", "llama-7b", 1.0f);

// Create static fusion
FusionConfig fusion_config;
fusion_config.strategy = FusionStrategy::STATIC;
fusion_config.source_lora_ids = {"legal-qa", "medical-qa"};
fusion_config.weights = {0.7f, 0.3f};  // 70% legal, 30% medical
fusion_config.enable_cache = true;
fusion_config.cache_ttl = std::chrono::hours(24);

bool fused = manager.fuseLoRAsAdvanced("legal-medical-hybrid", fusion_config);

if (fused) {
    // Use the fused adapter in inference
    manager.applyLoRA("legal-medical-hybrid", context_handle);
}
```

### Example 2: Dynamic Fusion with Interactive Tuning

```cpp
// Create dynamic fusion
FusionConfig config;
config.strategy = FusionStrategy::DYNAMIC;
config.source_lora_ids = {"adapter-v1", "adapter-v2"};
config.weights = {0.5f, 0.5f};

manager.fuseLoRAsAdvanced("tunable-fusion", config);

// In tuning loop
for (int iter = 0; iter < tuning_iterations; ++iter) {
    // Evaluate performance
    float performance = evaluate_on_validation_set();
    
    // Adjust weights based on performance
    float weight_v1 = compute_new_weight(performance);
    float weight_v2 = 1.0f - weight_v1;
    
    manager.updateFusionWeights("tunable-fusion", {weight_v1, weight_v2});
}
```

### Example 3: A/B Testing with Scheduled Fusion

```cpp
// Setup A/B test with gradual rollout
FusionConfig config;
config.strategy = FusionStrategy::SCHEDULED;
config.source_lora_ids = {"current-prod", "new-candidate"};
config.weights = {1.0f, 0.0f};  // Initial state: 100% current

// Configure schedule: gradual transition over 1 hour
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.start_time = std::chrono::system_clock::now();
schedule.transition_duration = std::chrono::seconds(3600);  // 1 hour
schedule.static_weights = {1.0f, 0.0f};  // Start
schedule.a_weight = 0.5f;  // End: 50/50
schedule.b_weight = 0.5f;

config.alpha_schedule = schedule;

manager.fuseLoRAsAdvanced("ab-test-fusion", config);
manager.setAlphaSchedule("ab-test-fusion", schedule);

// Weights automatically transition from 100/0 to 50/50 over 1 hour
```

### Example 4: Custom Scheduling Function

```cpp
// Circadian adapter scheduling
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.start_time = std::chrono::system_clock::now();

// Custom function: higher weight for "daytime" adapter during business hours
schedule.schedule_func = [](double time_offset) -> std::vector<float> {
    // time_offset in seconds since start
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto* tm = std::localtime(&time_t);
    
    int hour = tm->tm_hour;
    
    // Business hours (9 AM - 5 PM): favor daytime adapter
    float daytime_weight;
    if (hour >= 9 && hour < 17) {
        daytime_weight = 0.8f;
    } else {
        daytime_weight = 0.2f;
    }
    
    return {daytime_weight, 1.0f - daytime_weight};
};

manager.setAlphaSchedule("circadian-fusion", schedule);
```

### Example 5: Performance-Based Adaptive Fusion

```cpp
// Adaptive fusion based on real-time performance
class AdaptiveFusionController {
public:
    void updateBasedOnPerformance(
        MultiLoRAManager& manager,
        const std::string& fusion_id,
        double performance_metric
    ) {
        // Track performance history
        performance_history_.push_back(performance_metric);
        
        if (performance_history_.size() > 100) {
            performance_history_.erase(performance_history_.begin());
        }
        
        // Compute exponential moving average
        double ema = compute_ema(performance_history_);
        
        // Adjust weights: increase weight of better-performing adapter
        float weight_adjustment = (ema > target_performance_) ? 0.05f : -0.05f;
        
        auto current_weights = manager.getCurrentFusionWeights(fusion_id);
        if (!current_weights.empty()) {
            current_weights[0] = std::clamp(
                current_weights[0] + weight_adjustment, 
                0.1f, 0.9f
            );
            current_weights[1] = 1.0f - current_weights[0];
            
            manager.updateFusionWeights(fusion_id, current_weights);
        }
    }
    
private:
    std::vector<double> performance_history_;
    double target_performance_ = 0.85;
    
    double compute_ema(const std::vector<double>& values) {
        double alpha = 0.1;
        double ema = values[0];
        for (size_t i = 1; i < values.size(); ++i) {
            ema = alpha * values[i] + (1 - alpha) * ema;
        }
        return ema;
    }
};
```

## Performance Considerations

### Cache Efficiency

- **Cache Hit Rate**: Monitor via `getFusionMetrics().cache_hits`
- **TTL Configuration**: Longer TTL = higher hit rate but stale cache risk
- **Memory Usage**: Each cached fusion consumes VRAM proportional to largest source adapter

### Fusion Overhead

| Strategy | Fusion Time | Runtime Overhead | Cache Benefit |
|----------|-------------|------------------|---------------|
| STATIC   | ~10ms       | Minimal          | High          |
| DYNAMIC  | ~10ms       | Low              | Medium        |
| SCHEDULED| ~10ms       | Medium           | Low           |

### Compatibility Checks

- **Base Model**: ~0.1ms (string comparison)
- **Quantization**: ~0.2ms (mode + dimensions check)
- **GPU Placement**: ~0.1ms (placement + device check)
- **Rank**: ~0.1ms (integer comparison)

### Recommended Configurations

#### Development/Experimentation
```cpp
config.enable_cache = true;
config.cache_ttl = std::chrono::minutes(10);  // Short TTL
config.enforce_quantization_match = false;     // Allow mixing
```

#### Production Serving
```cpp
config.enable_cache = true;
config.cache_ttl = std::chrono::hours(24);     // Long TTL
config.enforce_quantization_match = true;      // Strict validation
config.enforce_rank_match = true;
```

## Best Practices

### 1. Compatibility Pre-Check

Always validate compatibility before attempting fusion:

```cpp
if (!manager.checkFusionCompatibility(lora_ids, fusion_config)) {
    // Handle incompatibility
    spdlog::error("Adapters are not compatible for fusion");
    return false;
}
```

### 2. Weight Normalization

Weights are automatically normalized to sum to 1.0, but use sensible initial values:

```cpp
// Good: Clear semantic meaning
fusion_config.weights = {0.6f, 0.4f};  // 60/40 split

// Avoid: Non-normalized (works but less clear)
fusion_config.weights = {3.0f, 2.0f};  // Will be normalized to 0.6/0.4
```

### 3. Monitor Metrics

Track fusion performance in production:

```cpp
auto metrics = manager.getFusionMetrics();

if (metrics.cache_hits + metrics.cache_misses > 0) {
    float hit_rate = static_cast<float>(metrics.cache_hits) / 
                     (metrics.cache_hits + metrics.cache_misses);
    
    if (hit_rate < 0.5f) {
        spdlog::warn("Fusion cache hit rate is low: {:.2f}", hit_rate);
        // Consider increasing cache TTL or using STATIC strategy
    }
}
```

### 4. Graceful Degradation

Handle fusion failures gracefully:

```cpp
bool fused = manager.fuseLoRAsAdvanced(fusion_id, config);

if (!fused) {
    // Fall back to single adapter
    spdlog::warn("Fusion failed, using primary adapter only");
    manager.applyLoRA(config.source_lora_ids[0], context_handle);
}
```

### 5. Cache Invalidation Strategy

Invalidate cache when source adapters are updated:

```cpp
// After reloading an adapter
manager.unloadLoRA("updated-adapter");
manager.loadLoRA("updated-adapter", new_path, base_model, 1.0f);

// Invalidate all fusions using this adapter
for (const auto& entry : manager.listFusionCache()) {
    if (std::find(entry.source_lora_ids.begin(), 
                  entry.source_lora_ids.end(), 
                  "updated-adapter") != entry.source_lora_ids.end()) {
        manager.invalidateFusionCache(entry.fusion_id);
    }
}
```

### 6. Scheduled Fusion Monitoring

For scheduled fusions, log weight changes for debugging:

```cpp
auto log_weights = [&]() {
    auto weights = manager.getCurrentFusionWeights("scheduled-fusion");
    spdlog::info("Current fusion weights: [{:.3f}, {:.3f}]", 
                 weights[0], weights[1]);
};

// Log periodically during transition
std::thread monitor_thread([&]() {
    while (in_transition) {
        log_weights();
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
});
```

## Troubleshooting

### High Cache Miss Rate

**Symptoms:** Low cache hit rate in metrics

**Causes:**
- TTL too short
- Using DYNAMIC/SCHEDULED strategies unnecessarily
- Frequent weight updates

**Solutions:**
- Increase `cache_ttl`
- Use STATIC strategy when weights don't change
- Batch weight updates

### Fusion Failures

**Symptoms:** `fuseLoRAsAdvanced()` returns false

**Causes:**
- Incompatible adapters (different base models)
- Quantization mismatch
- GPU placement conflict

**Solutions:**
- Check compatibility first: `checkFusionCompatibility()`
- Relax compatibility requirements in config
- Ensure all adapters loaded successfully

### High Fusion Latency

**Symptoms:** Slow fusion operations

**Causes:**
- Many source adapters (>5)
- No cache utilization
- Expensive compatibility checks

**Solutions:**
- Limit number of fused adapters
- Enable caching for STATIC fusions
- Disable unnecessary compatibility checks

### Memory Pressure

**Symptoms:** Frequent evictions, OOM errors

**Causes:**
- Too many cached fusions
- Large fused adapters
- Insufficient VRAM budget

**Solutions:**
- Reduce `cache_ttl`
- Increase `max_lora_vram_mb`
- Use `clearFusionCache()` periodically
- Enable quantization

## See Also

- [LoRA Adapter Guide](LORA_ADAPTER_GUIDE.md)
- [Multi-LoRA Management](MULTI_LORA_MANAGEMENT.md)
- [Performance Tuning](PERFORMANCE_TUNING.md)
- [API Reference](API_REFERENCE.md)
