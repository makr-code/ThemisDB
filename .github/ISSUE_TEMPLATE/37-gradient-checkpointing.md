---
name: "⚡ Implement Gradient Checkpointing for Memory-Efficient Training"
about: Reduce activation memory footprint with gradient checkpointing (Medium Priority - P2)
title: "[GPU Training] Implement Gradient Checkpointing for Large Model Training"
labels: priority:P2, type:optimization, area:llm, area:gpu, effort:medium, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Aktivierungs-Speicher wächst linear mit Sequenzlänge und Batch-Größe. Gradient Checkpointing kann Speicherverbrauch um 50-80% reduzieren mit nur 20-30% Performance-Overhead, ermöglicht größere Batches und längere Sequenzen.

**EN**: Activation memory grows linearly with sequence length and batch size. Gradient checkpointing can reduce memory footprint by 50-80% with only 20-30% performance overhead, enabling larger batches and longer sequences.

**Research Background**: 
- **Paper**: "Training Deep Nets with Sublinear Memory Cost" (Chen et al., 2016)
- **Paper**: "Checkmate: Breaking the Memory Wall with Optimal Tensor Rematerialization" (Jain et al., 2020)
- **Technique**: Trade computation for memory by selectively recomputing activations during backward pass

**Current Status**: Not implemented  
**Impact**: ⚡ **50-80% Memory Reduction** - Enables 2-4x larger batch sizes

## 🎯 Ziele / Goals

- [ ] Selective checkpoint placement in LoRA layers
- [ ] Recomputation strategy during backward pass
- [ ] Configurable checkpoint frequency
- [ ] Memory savings measurement
- [ ] Performance profiling (compute vs memory trade-off)

## 📝 Aufgaben / Tasks

### 1. Gradient Checkpointing Strategy
**Priorität**: P2 - Medium

**Research Insights**:
From "Training Deep Nets with Sublinear Memory Cost":
- Checkpoint every √n layers reduces memory from O(n) to O(√n)
- Optimal checkpointing: balance memory vs recomputation
- For transformers: checkpoint attention + FFN boundaries

**Implementation Strategy**:
```cpp
// File: include/llm/lora_framework/gradient_checkpointing.h

enum class CheckpointStrategy {
    NONE,           // No checkpointing
    UNIFORM,        // Checkpoint every N layers
    SQRT_N,         // Checkpoint every √n layers (optimal for memory)
    ATTENTION_ONLY, // Checkpoint attention layers only
    CUSTOM          // User-defined checkpoint points
};

struct CheckpointConfig {
    CheckpointStrategy strategy = CheckpointStrategy::SQRT_N;
    int checkpoint_frequency = 0;  // For UNIFORM strategy
    bool checkpoint_attention = true;
    bool checkpoint_ffn = false;
    bool checkpoint_lora = true;  // Checkpoint LoRA layers
};

class GradientCheckpointer {
public:
    explicit GradientCheckpointer(const CheckpointConfig& config);
    
    /**
     * @brief Mark layer for checkpointing
     * @param layer_id Unique layer identifier
     * @return true if this layer should be checkpointed
     */
    bool shouldCheckpoint(int layer_id) const;
    
    /**
     * @brief Save activation for later recomputation
     */
    void saveActivation(int layer_id, const GPUTensor& activation);
    
    /**
     * @brief Recompute activation during backward pass
     */
    GPUTensor recomputeActivation(int layer_id);
    
    /**
     * @brief Get memory statistics
     */
    struct Stats {
        size_t memory_saved_bytes;
        size_t recomputation_time_ms;
        float memory_reduction_pct;
    };
    
    Stats getStats() const;
    
private:
    CheckpointConfig config_;
    std::unordered_map<int, GPUTensor> checkpointed_activations_;
    Stats stats_;
};
```

**Tasks**:
- [ ] Implement checkpoint strategy selection
- [ ] Add checkpoint decision logic
- [ ] Integrate with GPULoRALayer
- [ ] Add memory tracking

**File**: `include/llm/lora_framework/gradient_checkpointing.h`

---

### 2. Selective Activation Caching
**Priorität**: P2 - Medium

**Optimization Technique**:
```cpp
// File: src/llm/lora_framework/gpu_lora_layers.cpp

class CheckpointedGPULoRALayer : public GPULoRALayer {
public:
    GPUTensor forward(const GPUTensor& input) override {
        // Save input for recomputation if checkpointed
        if (checkpointer_->shouldCheckpoint(layer_id_)) {
            // Don't save intermediate activations
            // Only save input + weights (small)
            checkpointer_->saveCheckpoint(layer_id_, input, {B_, A_});
        } else {
            // Normal forward with activation cache
            cached_input_ = input;  // Save for backward
            cached_intermediate_ = computeIntermediate(input);
        }
        
        return computeOutput(input);
    }
    
    GPUTensor backward(const GPUTensor& grad_output) override {
        GPUTensor input;
        
        if (checkpointer_->shouldCheckpoint(layer_id_)) {
            // ✅ Recompute forward pass (trade compute for memory)
            input = checkpointer_->recomputeForward(layer_id_);
            spdlog::debug("Recomputed activations for layer {}", layer_id_);
        } else {
            // Use cached activations
            input = cached_input_;
        }
        
        // Standard backward pass
        return computeGradient(grad_output, input);
    }
    
private:
    GradientCheckpointer* checkpointer_;
    int layer_id_;
    GPUTensor cached_input_;       // Only if not checkpointed
    GPUTensor cached_intermediate_; // Only if not checkpointed
};
```

**Memory Calculation**:
```
Without checkpointing (all activations cached):
- Input: batch_size × seq_len × hidden_dim × 4 bytes
- Intermediate: batch_size × seq_len × rank × 4 bytes  
- Output: batch_size × seq_len × hidden_dim × 4 bytes
Total per layer: ~3 × batch_size × seq_len × hidden_dim × 4 bytes

With checkpointing (recompute on backward):
- Checkpoint: input pointer + weight pointers (negligible)
- Recompute cost: 1 extra forward pass per backward
Total per layer: ~0 bytes activation memory
Trade-off: +33% compute time, -100% activation memory
```

**Tasks**:
- [ ] Implement checkpointed layer variant
- [ ] Add recomputation logic
- [ ] Measure memory savings
- [ ] Benchmark performance impact

---

### 3. Optimal Checkpoint Placement
**Priorität**: P2 - Medium

**Research-Based Strategy**:
From "Checkmate" paper, optimal checkpointing for transformers:

```cpp
// File: src/llm/lora_framework/gradient_checkpointing.cpp

bool GradientCheckpointer::shouldCheckpoint(int layer_id) const {
    switch (config_.strategy) {
        case CheckpointStrategy::SQRT_N:
            // Checkpoint every √n layers (optimal for deep networks)
            // For 32 layers: checkpoint at 0, 6, 12, 18, 24, 30
            int checkpoint_interval = static_cast<int>(std::sqrt(total_layers_));
            return (layer_id % checkpoint_interval) == 0;
        
        case CheckpointStrategy::UNIFORM:
            // Checkpoint every N layers
            return (layer_id % config_.checkpoint_frequency) == 0;
        
        case CheckpointStrategy::ATTENTION_ONLY:
            // Only checkpoint attention layers (most memory-intensive)
            return layer_types_[layer_id] == LayerType::ATTENTION;
        
        case CheckpointStrategy::CUSTOM:
            // User-defined checkpoints
            return custom_checkpoints_.count(layer_id) > 0;
        
        default:
            return false;
    }
}

size_t GradientCheckpointer::estimateMemorySavings() const {
    // Memory saved = (total layers - checkpointed layers) × activation size
    size_t total_activation_memory = total_layers_ * avg_activation_size_;
    size_t checkpointed_memory = num_checkpoints_ * avg_activation_size_;
    
    return total_activation_memory - checkpointed_memory;
}

float GradientCheckpointer::estimateComputeOverhead() const {
    // Compute overhead = (recomputation time) / (forward time)
    // Typically 20-30% for √n strategy
    float recompute_fraction = static_cast<float>(num_checkpoints_) / total_layers_;
    return recompute_fraction * 0.3f;  // 30% overhead per recomputation
}
```

**Tasks**:
- [ ] Implement √n checkpoint strategy
- [ ] Add memory estimation
- [ ] Add compute overhead estimation
- [ ] Validate optimal placement

---

### 4. Integration with GPU Training Loop
**Priorität**: P2 - Medium

**Update GPUTrainingConfig**:
```cpp
// File: include/llm/lora_framework/gpu_training_loop.h

struct GPUTrainingConfig {
    // ... existing fields ...
    
    // Gradient checkpointing
    bool enable_gradient_checkpointing = false;
    CheckpointStrategy checkpoint_strategy = CheckpointStrategy::SQRT_N;
    int checkpoint_frequency = 4;  // For UNIFORM strategy
};
```

**Update GPUTrainingLoop**:
```cpp
// File: src/llm/lora_framework/gpu_training_loop.cpp

void GPUTrainingLoop::initializeCheckpointing() {
    if (!config_.enable_gradient_checkpointing) {
        return;
    }
    
    CheckpointConfig checkpoint_config;
    checkpoint_config.strategy = config_.checkpoint_strategy;
    checkpoint_config.checkpoint_frequency = config_.checkpoint_frequency;
    
    checkpointer_ = std::make_unique<GradientCheckpointer>(checkpoint_config);
    
    // Apply checkpointing to layers
    for (size_t i = 0; i < layers_.size(); ++i) {
        if (checkpointer_->shouldCheckpoint(i)) {
            layers_[i]->enableCheckpointing(checkpointer_.get());
            spdlog::info("Enabled checkpointing for layer {}", i);
        }
    }
    
    auto stats = checkpointer_->estimateMemorySavings();
    spdlog::info("Estimated memory savings: {:.2f} GB", 
                 stats / (1024.0 * 1024.0 * 1024.0));
}

float GPUTrainingLoop::trainStep(const GPUBatch& batch) {
    // ... forward pass ...
    
    // Checkpointing happens automatically in layer forward/backward
    
    // ... backward pass ...
    
    // Log checkpoint statistics periodically
    if (current_metrics_.current_step % 100 == 0 && checkpointer_) {
        auto stats = checkpointer_->getStats();
        spdlog::info("Checkpoint stats: {:.1f}% memory reduction, {:.1f}ms recompute time",
                    stats.memory_reduction_pct, stats.recomputation_time_ms);
    }
    
    return loss;
}
```

**Tasks**:
- [ ] Add checkpointing initialization
- [ ] Integrate with training loop
- [ ] Add statistics logging
- [ ] Test with different strategies

---

### 5. Performance Benchmarking
**Priorität**: P2 - Medium

**Benchmark Strategy**:
```cpp
// Test file: tests/test_gradient_checkpointing.cpp

TEST(GradientCheckpointingTest, MemoryReduction) {
    // Baseline: no checkpointing
    GPUTrainingConfig config_baseline;
    config_baseline.enable_gradient_checkpointing = false;
    auto baseline_memory = measurePeakMemory(config_baseline);
    
    // With checkpointing
    GPUTrainingConfig config_checkpoint;
    config_checkpoint.enable_gradient_checkpointing = true;
    config_checkpoint.checkpoint_strategy = CheckpointStrategy::SQRT_N;
    auto checkpoint_memory = measurePeakMemory(config_checkpoint);
    
    float reduction = 100.0f * (baseline_memory - checkpoint_memory) / baseline_memory;
    
    // Expect 50-80% memory reduction
    EXPECT_GT(reduction, 50.0f);
    EXPECT_LT(reduction, 85.0f);
}

TEST(GradientCheckpointingTest, ComputeOverhead) {
    // Measure training time with and without checkpointing
    auto baseline_time = measureTrainingTime(/*checkpointing=*/false);
    auto checkpoint_time = measureTrainingTime(/*checkpointing=*/true);
    
    float overhead = 100.0f * (checkpoint_time - baseline_time) / baseline_time;
    
    // Expect 20-30% compute overhead for √n strategy
    EXPECT_LT(overhead, 35.0f);
}

TEST(GradientCheckpointingTest, LargerBatchSizes) {
    // Without checkpointing: max batch_size = 4 (OOM at 8)
    // With checkpointing: should support batch_size = 8-16
    
    GPUTrainingConfig config;
    config.enable_gradient_checkpointing = true;
    
    // Should not OOM
    EXPECT_TRUE(trainWithBatchSize(config, 8));
    EXPECT_TRUE(trainWithBatchSize(config, 12));
}
```

**Tasks**:
- [ ] Create benchmarking tests
- [ ] Measure memory reduction
- [ ] Measure compute overhead
- [ ] Validate larger batch sizes
- [ ] Profile with nsys/rocprof

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] Gradient checkpointing reduces memory by 50-80%
- [ ] Compute overhead < 30% with √n strategy
- [ ] Enables 2-4x larger batch sizes
- [ ] Works with all GPU backends (CUDA, HIP, Vulkan, DirectX)
- [ ] Configurable checkpoint strategies (SQRT_N, UNIFORM, ATTENTION_ONLY)
- [ ] Memory statistics logging
- [ ] Performance profiling confirms trade-offs
- [ ] Comprehensive tests pass (>90% coverage)
- [ ] Integration with existing GPUTrainingLoop
- [ ] Documentation with memory/compute trade-off analysis

## 📊 Effort Estimation

- **Aufwand / Effort**: 1-2 weeks (Medium)
- **Komplexität / Complexity**: Medium-High
- **Risiko / Risk**: Low (well-researched technique)

## 🔗 Related Issues

- Issue #35: GPU Loss/Gradient Kernels
- Issue #36: GPU Mixed Precision Unscaling

## 📚 References

**Research Papers**:
- Chen et al. (2016): "Training Deep Nets with Sublinear Memory Cost" - arXiv:1604.06174
- Jain et al. (2020): "Checkmate: Breaking the Memory Wall with Optimal Tensor Rematerialization" - MLSys 2020
- Rajbhandari et al. (2021): "ZeRO-Offload" - USENIX ATC 2021

**Implementation References**:
- PyTorch torch.utils.checkpoint
- DeepSpeed activation checkpointing
- Megatron-LM selective recomputation
- NVIDIA Apex checkpoint utilities

**Memory Analysis**:
- Llama-7B without checkpointing: ~14 GB activation memory
- Llama-7B with √n checkpointing: ~3 GB activation memory
- Trade-off: 78% memory reduction, 25% slower training

---

**Priority**: P2 - Medium priority (significant optimization)  
**Impact**: Memory efficiency, larger batch sizes, longer sequences  
**Status**: Ready to implement
