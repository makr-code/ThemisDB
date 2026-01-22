---
name: "Distributed Training: Loss Aggregation from Shards"
about: Implement actual loss aggregation from distributed shard trainers
title: "[Distributed Training] Implement Real Loss Aggregation from Shard Trainers"
labels: priority:P2, type:enhancement, area:llm, effort:medium, component:distributed-training
assignees: ''
---

## 🎯 Objective

Replace simulated loss values in distributed training with actual loss aggregation from shard trainers.

**Status:** 📋 Planned  
**Priority:** P2 (Medium)  
**Effort:** 2-3 weeks  
**Dependencies:** 
- Distributed training coordinator integration (✅ COMPLETE)
- Local shard trainer loss computation

## 📋 Background

The current distributed training implementation (PR #XXX) uses simulated loss values for demonstration purposes. In production, loss values should be:
1. Computed by each shard during local training
2. Collected during gradient exchange
3. Aggregated across shards (e.g., weighted average)
4. Returned in the step result

**Current Implementation:**
```cpp
// NOTE: Simulated loss for demonstration
constexpr float loss_decay_rate = 0.1f;
float simulated_loss = 1.0f / (1.0f + step * loss_decay_rate);
loss_history.push_back(simulated_loss);
```

**Target Implementation:**
```cpp
// Actual loss from distributed training
auto step_result = coordinator->executeStep();
if (step_result.success && step_result.aggregated_loss.has_value()) {
    loss_history.push_back(step_result.aggregated_loss.value());
}
```

## 🔧 Implementation Tasks

### 1. Extend GradientExchangeMessage to Include Loss (Week 1)

**Files to Modify:**
- [ ] `include/llm/distributed_training_coordinator.h` - Add loss field to GradientExchangeMessage
- [ ] `src/llm/distributed_training_coordinator.cpp` - Serialize/deserialize loss values

**Changes Required:**
```cpp
struct GradientExchangeMessage {
    // ... existing fields ...
    
    // Loss metrics from this shard
    std::optional<float> local_loss;
    std::optional<float> local_accuracy;
    int samples_in_batch;
    
    json toJSON() const;
    static GradientExchangeMessage fromJSON(const json& j);
};
```

**Testing:**
- [ ] Unit tests for loss serialization/deserialization
- [ ] Verify backward compatibility with existing messages

---

### 2. Implement Loss Aggregation in Coordinator (Week 1-2)

**Files to Modify:**
- [ ] `include/llm/distributed_training_coordinator.h` - Add aggregated_loss to StepResult
- [ ] `src/llm/distributed_training_coordinator.cpp` - Aggregate loss from all shards

**Methods to Add:**
```cpp
class DistributedTrainingCoordinator {
    // ...
    
    // Aggregate loss values from all shards
    std::optional<float> aggregateLoss(
        const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
    );
    
    // Weighted average based on samples processed
    float computeWeightedLoss(
        const std::vector<std::pair<float, int>>& shard_losses_and_counts
    );
};

struct StepResult {
    // ... existing fields ...
    std::optional<float> aggregated_loss;
    std::optional<float> aggregated_accuracy;
    std::map<std::string, float> per_shard_loss;  // For monitoring
};
```

**Aggregation Strategy:**
- Weighted average by number of samples in each shard's batch
- Handle missing loss values gracefully
- Support different aggregation methods (mean, median, min, max)

**Testing:**
- [ ] Unit tests for loss aggregation
- [ ] Test with unequal batch sizes across shards
- [ ] Test with missing loss values from failed shards

---

### 3. Integrate with Local Shard Trainers (Week 2-3)

**Files to Modify:**
- [ ] `src/llm/lora_framework/lora_training_service.cpp` - Pass actual loss to coordinator
- [ ] Integration with local training loops

**Changes Required:**
1. Compute loss during local training on each shard
2. Include loss in gradient messages sent to coordinator
3. Update collectGradients() to extract loss values

**Implementation Notes:**
- Loss should be computed after forward pass, before gradients
- Use same loss function across all shards for consistency
- Consider different loss types: MSE, CrossEntropy, Custom

**Testing:**
- [ ] Integration tests with real training data
- [ ] Verify loss convergence across shards
- [ ] Compare distributed loss with single-shard baseline

---

### 4. Update Training Service Integration (Week 3)

**Files to Modify:**
- [ ] `src/llm/lora_framework/lora_training_service.cpp` - Remove simulated loss

**Changes:**
```cpp
// Remove simulated loss calculation
// constexpr float loss_decay_rate = 0.1f;
// float simulated_loss = 1.0f / (1.0f + step * loss_decay_rate);

// Use actual aggregated loss
if (step_result.success && step_result.aggregated_loss.has_value()) {
    loss_history.push_back(step_result.aggregated_loss.value());
    
    // Update result metrics
    result.metrics["per_shard_loss"] = step_result.per_shard_loss;
    result.metrics["loss_variance"] = computeLossVariance(step_result.per_shard_loss);
}
```

**Testing:**
- [ ] End-to-end distributed training test
- [ ] Verify loss values are realistic
- [ ] Compare with single-shard training

---

## 📊 Acceptance Criteria

- [ ] Loss values are computed by each shard during local training
- [ ] Loss values are included in gradient exchange messages
- [ ] Coordinator aggregates loss using weighted average
- [ ] Training service uses actual aggregated loss (no simulation)
- [ ] All existing distributed training tests pass
- [ ] New tests cover loss aggregation scenarios
- [ ] Documentation updated with loss aggregation details
- [ ] Performance impact is negligible (< 1% overhead)

## 📈 Expected Benefits

- **Accurate monitoring** of distributed training convergence
- **Early detection** of training issues (divergence, overfitting)
- **Better debugging** with per-shard loss tracking
- **Production ready** loss metrics for MLOps pipelines

## 🔗 Related Issues

- Distributed Training Integration (PR #XXX) - Initial implementation
- Shard Trainer Enhancement - Local loss computation
- Monitoring Dashboard - Display distributed loss metrics

## 📝 Additional Notes

**Loss Aggregation Strategies:**
- **Weighted Average** (default): `Σ(loss_i × samples_i) / Σ(samples_i)`
- **Simple Mean**: `Σ(loss_i) / N`
- **Median**: Robust to outliers from failed shards
- **Min/Max**: Useful for monitoring shard health

**Future Enhancements:**
- Support for multiple loss metrics (training, validation)
- Loss history tracking and visualization
- Anomaly detection for divergent shards
- Automatic learning rate adjustment based on loss trends
