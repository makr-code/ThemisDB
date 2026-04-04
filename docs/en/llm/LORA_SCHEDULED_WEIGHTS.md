# LoRA Scheduled Weight Mode

**Version:** See repository `VERSION` file (currently 1.4.1-dev, targeting 1.5.0)  
**Date:** 2026-01-22  
**Status:** Complete

---

## Overview

The LoRA Scheduled Weight Mode enables **time-based dynamic blending** of multiple LoRA adapters with smooth transitions. This feature is essential for advanced use cases like A/B testing, gradual model transitions, and adaptive adapter scheduling.

## Features

### Supported Scheduling Strategies

| Strategy | Description | Use Case |
|----------|-------------|----------|
| **LINEAR** | Smooth linear transition between weights | Gradual model transitions |
| **EXPONENTIAL** | Exponential decay or growth | Fast initial changes, slow convergence |
| **STEP_WISE** | Discrete transitions at specified times | Phased rollouts, time-based A/B tests |
| **CUSTOM** | User-defined schedule functions | Complex adaptive strategies |

---

## Usage Examples

### 1. Linear Scheduling

Linear scheduling provides smooth, predictable transitions between adapter weights.

```cpp
#include "llm/multi_lora_manager.h"

MultiLoRAManager manager(config);

// Load adapters
manager.loadLoRA("adapter-v1", "/path/to/v1.bin", "base-model", 1.0f);
manager.loadLoRA("adapter-v2", "/path/to/v2.bin", "base-model", 1.0f);

// Configure fusion with linear scheduling
FusionConfig fusion_config;
fusion_config.strategy = FusionStrategy::SCHEDULED;
fusion_config.source_lora_ids = {"adapter-v1", "adapter-v2"};
fusion_config.weights = {0.9f, 0.1f};  // Initial fallback weights

// Create schedule
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::LINEAR;
schedule.static_weights = {0.9f, 0.1f};   // Start: 90% v1, 10% v2
schedule.target_weights = {0.1f, 0.9f};   // End: 10% v1, 90% v2
schedule.start_time = std::chrono::system_clock::now();
schedule.transition_duration = std::chrono::hours(24);  // 24-hour transition

fusion_config.alpha_schedule = schedule;

// Create fusion
manager.fuseLoRAsAdvanced("gradual-transition", fusion_config);
manager.setAlphaSchedule("gradual-transition", schedule);

// Weights will automatically transition over 24 hours
// At t=0h:  [0.9, 0.1]
// At t=12h: [0.5, 0.5]
// At t=24h: [0.1, 0.9]
```

### 2. Exponential Decay

Exponential decay provides fast transitions initially, then slows down as it approaches the target.

```cpp
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::EXPONENTIAL;
schedule.static_weights = {0.95f, 0.05f};   // Start: 95% old, 5% new
schedule.target_weights = {0.05f, 0.95f};   // End: 5% old, 95% new
schedule.exponential_base = 2.0f;           // Controls decay rate
schedule.exponential_decay = true;          // Decay mode (vs growth)
schedule.start_time = std::chrono::system_clock::now();
schedule.transition_duration = std::chrono::hours(12);

// Fast transition initially (e.g., 80% → 50% in first 2 hours)
// Slower later (e.g., 20% → 5% in last 8 hours)
```

### 3. Exponential Growth

Exponential growth provides slow transitions initially, then speeds up as it approaches the target.

```cpp
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::EXPONENTIAL;
schedule.static_weights = {0.95f, 0.05f};
schedule.target_weights = {0.05f, 0.95f};
schedule.exponential_base = 2.0f;
schedule.exponential_decay = false;         // Growth mode
schedule.start_time = std::chrono::system_clock::now();
schedule.transition_duration = std::chrono::hours(12);

// Slow transition initially (e.g., 95% → 80% in first 8 hours)
// Fast later (e.g., 50% → 5% in last 2 hours)
```

### 4. Step-Wise Scheduling

Step-wise scheduling provides discrete transitions at specific time points.

```cpp
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;
schedule.static_weights = {0.9f, 0.1f};  // Initial weights

// Define time points (in seconds from start_time)
schedule.step_times = {
    3600.0,    // 1 hour
    7200.0,    // 2 hours
    10800.0    // 3 hours
};

// Define weights at each step
schedule.step_weights = {
    {0.9f, 0.1f},  // Before 1h: 90/10
    {0.7f, 0.3f},  // 1h-2h: 70/30
    {0.4f, 0.6f},  // 2h-3h: 40/60
    {0.1f, 0.9f}   // After 3h: 10/90
};

schedule.start_time = std::chrono::system_clock::now();

// Weights change instantly at each time point
```

### 5. Custom Schedule Function

For complex scenarios, define your own scheduling logic.

```cpp
AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::CUSTOM;
schedule.start_time = std::chrono::system_clock::now();

// Custom schedule: Sine wave oscillation
schedule.schedule_func = [](double time_offset) -> std::vector<float> {
    // Oscillate between adapters with a 1-hour period
    // Using portable constant for pi (C++ standard compatible)
    constexpr double pi = 3.14159265358979323846;
    float phase = std::sin(2.0 * pi * time_offset / 3600.0);
    float weight_a = 0.5f + 0.4f * phase;  // Range: 0.1 to 0.9
    float weight_b = 1.0f - weight_a;
    return {weight_a, weight_b};
};
```

### 6. Multi-Adapter Scheduling

Schedule transitions across three or more adapters.

```cpp
manager.loadLoRA("adapter-a", "/path/to/a.bin", "base", 1.0f);
manager.loadLoRA("adapter-b", "/path/to/b.bin", "base", 1.0f);
manager.loadLoRA("adapter-c", "/path/to/c.bin", "base", 1.0f);

AlphaSchedule schedule;
schedule.strategy = FusionStrategy::SCHEDULED;
schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;
schedule.static_weights = {0.7f, 0.2f, 0.1f};

// Rotate focus among three adapters every hour
schedule.step_times = {3600.0, 7200.0};
schedule.step_weights = {
    {0.7f, 0.2f, 0.1f},  // Hour 0-1: Focus on A
    {0.2f, 0.7f, 0.1f},  // Hour 1-2: Focus on B
    {0.1f, 0.2f, 0.7f}   // Hour 2+: Focus on C
};

schedule.start_time = std::chrono::system_clock::now();
```

---

## Use Cases

### 1. Gradual Model Rollout

Roll out a new model version gradually to minimize risk:

```cpp
// Transition from old to new model over 7 days
schedule.scheduling_strategy = SchedulingStrategy::LINEAR;
schedule.static_weights = {1.0f, 0.0f};      // Day 0: 100% old
schedule.target_weights = {0.0f, 1.0f};      // Day 7: 100% new
schedule.transition_duration = std::chrono::hours(24 * 7);
```

### 2. A/B Testing with Time-Based Splits

Test different models with changing traffic percentages:

```cpp
// Week 1: 90/10, Week 2: 50/50, Week 3: 10/90
schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;
schedule.step_times = {
    7 * 24 * 3600.0,   // 1 week
    14 * 24 * 3600.0   // 2 weeks
};
schedule.step_weights = {
    {0.9f, 0.1f},  // Week 1
    {0.5f, 0.5f},  // Week 2
    {0.1f, 0.9f}   // Week 3+
};
```

### 3. Performance-Based Adaptive Scheduling

Combine with custom function for performance-based adaptation:

```cpp
schedule.scheduling_strategy = SchedulingStrategy::CUSTOM;
schedule.schedule_func = [&metrics](double time_offset) {
    // Favor better-performing model based on recent metrics
    float score_a = metrics.getRecentScore("adapter-a");
    float score_b = metrics.getRecentScore("adapter-b");
    float total = score_a + score_b;
    return {score_a / total, score_b / total};
};
```

### 4. Load-Based Model Selection

Switch models based on system load:

```cpp
schedule.scheduling_strategy = SchedulingStrategy::CUSTOM;
schedule.schedule_func = [&system](double time_offset) {
    float load = system.getCurrentLoad();
    if (load > 0.8f) {
        // High load: Use faster but less accurate model
        return {0.1f, 0.9f};
    } else {
        // Normal load: Use slower but more accurate model
        return {0.9f, 0.1f};
    }
};
```

---

## API Reference

### AlphaSchedule Structure

```cpp
struct AlphaSchedule {
    std::string schedule_id;
    FusionStrategy strategy;
    SchedulingStrategy scheduling_strategy;
    
    // Weight configuration
    std::vector<float> static_weights;      // Initial/fallback weights
    std::vector<float> target_weights;      // Target weights for transition
    
    // Time parameters
    std::chrono::system_clock::time_point start_time;
    std::chrono::seconds transition_duration;
    
    // Exponential parameters
    float exponential_base;                 // Base for exponential (default: 2.0)
    bool exponential_decay;                 // true=decay, false=growth
    
    // Step-wise parameters
    std::vector<double> step_times;         // Time points (seconds)
    std::vector<std::vector<float>> step_weights;  // Weights at each step
    
    // Custom schedule
    using ScheduleFunc = std::function<std::vector<float>(double)>;
    ScheduleFunc schedule_func;
    
    // Backward compatibility (legacy A/B testing)
    float a_weight;
    float b_weight;
};
```

### Key Methods

```cpp
// Create scheduled fusion
bool fuseLoRAsAdvanced(
    const std::string& fusion_id,
    const FusionConfig& config
);

// Set/update schedule
bool setAlphaSchedule(
    const std::string& fusion_id,
    const AlphaSchedule& schedule
);

// Get current weights (computed based on current time)
std::vector<float> getCurrentFusionWeights(
    const std::string& fusion_id
);

// Update schedule on the fly
bool updateFusionWeights(
    const std::string& fusion_id,
    const std::vector<float>& new_weights
);
```

---

## Best Practices

### 1. Choose the Right Strategy

- **LINEAR**: Default choice for most transitions
- **EXPONENTIAL DECAY**: When you want fast initial adoption
- **EXPONENTIAL GROWTH**: When you want cautious initial rollout
- **STEP_WISE**: For phased rollouts with clear milestones
- **CUSTOM**: For complex, dynamic scenarios

### 2. Set Appropriate Transition Durations

- **Too fast**: May cause instability, user confusion
- **Too slow**: Delays benefits of new model
- **Recommended**: 12-48 hours for production rollouts

### 3. Monitor Performance

```cpp
// Get fusion metrics
auto metrics = manager.getFusionMetrics();
std::cout << "Cache hits: " << metrics.cache_hits << std::endl;
std::cout << "Avg fusion time: " << metrics.avg_fusion_time_ms << "ms" << std::endl;
```

### 4. Handle Edge Cases

- Always provide `static_weights` as fallback
- Ensure weights sum to approximately 1.0
- Test schedule behavior before production deployment
- Consider time zone differences for scheduled transitions

### 5. Backward Compatibility

The old A/B testing syntax is still supported:

```cpp
schedule.a_weight = 0.7f;  // Legacy syntax
schedule.b_weight = 0.3f;

// Modern equivalent:
schedule.target_weights = {0.7f, 0.3f};
```

---

## Performance Considerations

### Cache Behavior

- **STATIC fusion**: Fully cached, never recomputed
- **DYNAMIC fusion**: Cache invalidated on manual updates
- **SCHEDULED fusion**: Weights recomputed on each access

### Optimization Tips

1. **Reduce Schedule Complexity**: Simpler schedules compute faster
2. **Batch Requests**: Group inference requests when possible
3. **Pre-compute Steps**: Use STEP_WISE for predictable patterns
4. **Monitor Overhead**: Check fusion metrics regularly

---

## Troubleshooting

### Weights Not Changing

**Problem:** Weights stay static despite schedule configuration.

**Solution:**
```cpp
// Ensure strategy is set to SCHEDULED
config.strategy = FusionStrategy::SCHEDULED;

// Verify schedule is set
manager.setAlphaSchedule(fusion_id, schedule);

// Check that transition_duration > 0 for LINEAR/EXPONENTIAL
schedule.transition_duration = std::chrono::seconds(3600);
```

### Unexpected Weight Values

**Problem:** Weights don't match expected values.

**Solution:**
```cpp
// Verify current time vs start_time
auto current_weights = manager.getCurrentFusionWeights(fusion_id);
for (auto w : current_weights) {
    std::cout << "Weight: " << w << std::endl;
}

// Check if transition has completed
auto elapsed = std::chrono::system_clock::now() - schedule.start_time;
if (elapsed >= schedule.transition_duration) {
    // Transition complete, should be at target_weights
}
```

### Performance Issues

**Problem:** High latency on weight computation.

**Solution:**
```cpp
// Use STEP_WISE instead of CUSTOM for predictable patterns
schedule.scheduling_strategy = SchedulingStrategy::STEP_WISE;

// Or cache frequently-accessed fusions
config.enable_cache = true;
config.cache_ttl = std::chrono::seconds(60);
```

---

## Related Documentation

- [LoRA Framework Developer Guide](LORA_FRAMEWORK_DEVELOPER_GUIDE.md)
- [Multi-LoRA Integration Examples](LORA_INTEGRATION_EXAMPLES.md)
- [LoRA Training Guide](LORA_TRAINING_GUIDE.md)

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.5.0 | 2026-01-22 | Initial release with LINEAR, EXPONENTIAL, STEP_WISE, CUSTOM strategies |

---

## Support

For questions or issues:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
