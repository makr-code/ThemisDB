# Gradient Checkpointing Implementation Guide

## Overview

Gradient checkpointing (also known as activation checkpointing) is a memory optimization technique that trades computation for memory during neural network training. Instead of storing all intermediate activations for the backward pass, we selectively store only some checkpoints and recompute the others on-the-fly during backpropagation.

## Research Background

This implementation is based on:

- **Chen et al. (2016)**: "Training Deep Nets with Sublinear Memory Cost" - arXiv:1604.06174
- **Jain et al. (2020)**: "Checkmate: Breaking the Memory Wall with Optimal Tensor Rematerialization" - MLSys 2020

### Key Benefits

- **Memory Reduction**: 50-80% reduction in activation memory
- **Larger Batch Sizes**: Enables 2-4x larger batches
- **Longer Sequences**: Support for longer sequence lengths
- **Minimal Overhead**: Only 20-30% compute overhead with optimal strategies

### Trade-offs

| Metric | Without Checkpointing | With Checkpointing (SQRT_N) |
|--------|----------------------|----------------------------|
| Activation Memory | 100% | 20-30% |
| Compute Time | 100% | 120-130% |
| Batch Size | 1x | 2-4x |
| Training Throughput | 1x | ~0.8x (but larger batches compensate) |

## Architecture

### Components

1. **GradientCheckpointer**: Core checkpointing logic and strategy implementation
2. **GPULoRALayer**: Extended to support selective activation caching
3. **GPUTrainingLoop**: Orchestrates checkpointing across layers

### Checkpoint Strategies

#### 1. NONE
- **Description**: No checkpointing (baseline)
- **Use Case**: When memory is not a constraint
- **Memory**: 100% activations stored
- **Compute**: No overhead

#### 2. SQRT_N (Recommended)
- **Description**: Checkpoint every √n layers
- **Use Case**: Optimal balance between memory and compute
- **Memory**: O(√n) instead of O(n)
- **Compute**: ~25% overhead
- **Example**: 36 layers → checkpoint every 6th layer (layers 0, 6, 12, 18, 24, 30)

```cpp
CheckpointConfig config;
config.strategy = CheckpointStrategy::SQRT_N;
config.total_layers = 36;
```

#### 3. UNIFORM
- **Description**: Checkpoint every N layers
- **Use Case**: Custom memory/compute trade-off
- **Memory**: Depends on frequency
- **Compute**: ~30% overhead per checkpoint
- **Example**: Checkpoint every 4 layers

```cpp
CheckpointConfig config;
config.strategy = CheckpointStrategy::UNIFORM;
config.checkpoint_frequency = 4;
config.total_layers = 32;
```

#### 4. ATTENTION_ONLY
- **Description**: Only checkpoint attention layers (most memory-intensive)
- **Use Case**: Transformer models with distinct layer types
- **Memory**: Depends on attention layer count
- **Compute**: Variable

```cpp
CheckpointConfig config;
config.strategy = CheckpointStrategy::ATTENTION_ONLY;
config.total_layers = 32;

GradientCheckpointer checkpointer(config);
checkpointer.setLayerType(0, LayerType::ATTENTION);
checkpointer.setLayerType(1, LayerType::FFN);
```

#### 5. CUSTOM
- **Description**: User-defined checkpoint points
- **Use Case**: Fine-grained control over checkpointing
- **Memory**: Depends on custom points
- **Compute**: Variable

```cpp
CheckpointConfig config;
config.strategy = CheckpointStrategy::CUSTOM;
config.total_layers = 32;

GradientCheckpointer checkpointer(config);
checkpointer.addCustomCheckpoint(5);
checkpointer.addCustomCheckpoint(10);
checkpointer.addCustomCheckpoint(20);
```

## Usage Examples

### Basic Usage with Training Loop

```cpp
#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/gradient_checkpointing.h"

// Configure training with gradient checkpointing
GPUTrainingConfig config;
config.num_epochs = 10;
config.learning_rate = 1e-4f;
config.device = Device::cuda();

// Enable gradient checkpointing
config.enable_gradient_checkpointing = true;
config.checkpoint_strategy = CheckpointStrategy::SQRT_N;

// Create training loop
GPUTrainingLoop trainer(config);

// Set up data loader
auto data_loader = std::make_unique<GPUDataLoader>(tokenizer, loader_config);
trainer.setDataLoader(std::move(data_loader));

// Add layers
for (auto* layer : lora_layers) {
    trainer.addLayer(layer);
}

// Train (checkpointing is automatic)
trainer.train();
```

### Advanced: Custom Checkpointing Strategy

```cpp
// Create custom checkpoint configuration
CheckpointConfig checkpoint_config;
checkpoint_config.strategy = CheckpointStrategy::CUSTOM;
checkpoint_config.total_layers = 32;

GradientCheckpointer checkpointer(checkpoint_config);

// Checkpoint specific layers (e.g., every attention layer)
for (int i = 0; i < 32; i += 2) {  // Every other layer
    checkpointer.addCustomCheckpoint(i);
}

// Estimate savings before training
size_t avg_activation_size = 8 * 1024 * 1024;  // 8MB per layer
size_t memory_savings = checkpointer.estimateMemorySavings(avg_activation_size);
float compute_overhead = checkpointer.estimateComputeOverhead();

spdlog::info("Estimated memory savings: {:.2f} GB", 
             memory_savings / (1024.0 * 1024.0 * 1024.0));
spdlog::info("Estimated compute overhead: {:.1f}%", compute_overhead);
```

### Layer-Level Integration

```cpp
// Enable checkpointing for specific layers
GPULoRALayer layer1(in_dim, out_dim, rank, 1.0f, device);
layer1.set_checkpointing(true);
layer1.set_layer_id(0);

GPULoRALayer layer2(in_dim, out_dim, rank, 1.0f, device);
layer2.set_checkpointing(false);  // Don't checkpoint this layer
layer2.set_layer_id(1);

// Forward pass (automatic checkpointing)
GPUTensor output1 = layer1.forward(input);
GPUTensor output2 = layer2.forward(output1);

// Backward pass (automatic recomputation for checkpointed layers)
GPUTensor grad1 = layer2.backward(grad_output);
GPUTensor grad0 = layer1.backward(grad1);  // Recomputes activations here
```

## Performance Tuning

### Memory vs. Compute Trade-off

Choose strategy based on your constraints:

| Priority | Strategy | Memory Savings | Compute Overhead |
|----------|----------|---------------|------------------|
| Maximum Memory Savings | SQRT_N | 70-80% | 25% |
| Balanced | UNIFORM (freq=4) | 50-60% | 20% |
| Minimal Overhead | UNIFORM (freq=8) | 30-40% | 15% |
| Custom | CUSTOM | Variable | Variable |

### Optimal Batch Size Scaling

With checkpointing enabled, you can increase batch size:

```cpp
// Without checkpointing: max batch_size = 4
GPUTrainingConfig config_baseline;
config_baseline.enable_gradient_checkpointing = false;

// With checkpointing: can use batch_size = 8-16
GPUTrainingConfig config_checkpoint;
config_checkpoint.enable_gradient_checkpointing = true;
config_checkpoint.checkpoint_strategy = CheckpointStrategy::SQRT_N;

// Larger batches improve GPU utilization and convergence
```

### Monitoring

Track checkpointing statistics during training:

```cpp
// Statistics are logged every 100 steps automatically
// Example output:
// [info] Checkpoint stats: 75.2% memory reduction, 24.8% compute overhead, 1250ms recompute time
```

## Expected Performance

### Llama-7B Example

| Configuration | Memory Usage | Training Time | Batch Size |
|---------------|-------------|---------------|------------|
| No Checkpointing | 14 GB | 100% | 4 |
| SQRT_N Checkpointing | 3 GB | 125% | 16 |
| **Net Throughput** | **-78% memory** | **+260% throughput** | **4x batch** |

### Memory Calculation

For a model with:
- 32 layers
- Batch size: 4
- Sequence length: 512
- Hidden dimension: 768
- Data type: float32 (4 bytes)

**Without Checkpointing:**
- Per layer: 4 × 512 × 768 × 4 = 6.3 MB
- Total: 32 × 6.3 MB = 201.6 MB

**With SQRT_N (6 checkpoints):**
- Checkpointed: 6 × 6.3 MB = 37.8 MB
- Savings: 163.8 MB (81.3%)

## Integration with Other Features

### Compatible with Mixed Precision

```cpp
GPUTrainingConfig config;
config.enable_gradient_checkpointing = true;
config.use_mixed_precision = true;  // FP16/BF16 training

// Both features work together for maximum memory efficiency
```

### Compatible with Multi-GPU

```cpp
GPUTrainingConfig config;
config.enable_gradient_checkpointing = true;
config.use_multi_gpu = true;
config.gpu_ids = {0, 1, 2, 3};

// Checkpointing is applied per GPU independently
```

## Best Practices

1. **Start with SQRT_N**: It provides optimal memory-compute trade-off
2. **Monitor Statistics**: Check logs for memory reduction and overhead
3. **Adjust Batch Size**: Increase batch size to utilize saved memory
4. **Profile First**: Measure baseline before enabling checkpointing
5. **Test Incrementally**: Validate accuracy is maintained

## Troubleshooting

### Issue: "Checkpointing enabled but no input saved"

**Cause**: Layer forward pass didn't save input for checkpointed layer.

**Solution**: Ensure layer is properly initialized:
```cpp
layer.set_checkpointing(true);
layer.set_layer_id(layer_id);
```

### Issue: High compute overhead (>40%)

**Cause**: Too many layers checkpointed.

**Solution**: Reduce checkpoint frequency:
```cpp
config.checkpoint_strategy = CheckpointStrategy::UNIFORM;
config.checkpoint_frequency = 8;  // Checkpoint less frequently
```

### Issue: Out of memory even with checkpointing

**Cause**: Batch size still too large or insufficient GPU memory.

**Solution**: Further reduce batch size or use gradient accumulation:
```cpp
config.gradient_accumulation_steps = 4;  // Accumulate over 4 steps
```

## References

1. Chen, T., Xu, B., Zhang, C., & Guestrin, C. (2016). Training Deep Nets with Sublinear Memory Cost. arXiv:1604.06174
2. Jain, P., Jain, A., Nrusimha, A., et al. (2020). Checkmate: Breaking the Memory Wall with Optimal Tensor Rematerialization. MLSys 2020
3. Rajbhandari, S., Rasley, J., Ruwase, O., & He, Y. (2021). ZeRO-Offload: Democratizing Billion-Scale Model Training. USENIX ATC 2021

## Implementation Status

- ✅ Core checkpointing infrastructure
- ✅ Strategy implementations (NONE, UNIFORM, SQRT_N, ATTENTION_ONLY, CUSTOM)
- ✅ GPU LoRA layer integration
- ✅ Training loop integration
- ✅ Statistics tracking
- ✅ Comprehensive tests
- ✅ Documentation

## Future Enhancements

- [ ] Automatic strategy selection based on available memory
- [ ] Per-layer memory profiling for optimal checkpoint placement
- [ ] Integration with distributed training frameworks
- [ ] Checkpoint caching for repeated forward passes
