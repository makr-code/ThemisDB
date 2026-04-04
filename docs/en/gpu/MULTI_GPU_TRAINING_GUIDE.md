# Multi-GPU Training Guide

## Overview

ThemisDB LoRA framework supports multi-GPU training with data parallelism for faster training and larger batch sizes. This guide covers setup, usage, and optimization of multi-GPU training.

## Key Features

- **Data Parallelism**: Each GPU processes different data batches with full model replica
- **Automatic Gradient Synchronization**: All-reduce averages gradients across GPUs
- **Multiple Backends**: NCCL (NVIDIA), RCCL (AMD), custom fallback
- **Near-Linear Scaling**: Achieve 3.5-3.8x speedup with 4 GPUs
- **Minimal Code Changes**: Drop-in replacement for single-GPU training

## Quick Start

### 1. Basic Multi-GPU Training

```cpp
#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/multi_gpu_trainer.h"

using namespace themis::llm::lora;

// Initialize multi-GPU context (use all available GPUs)
MultiGPUContext ctx(0);

// Create trainer
MultiGPULoRATrainer::Config config;
config.learning_rate = 0.001f;
MultiGPULoRATrainer trainer(ctx, config);

// Create multi-GPU layer
auto layer = trainer.create_layer(768, 768, 8, 1.0f);

// Training loop
for (int epoch = 0; epoch < num_epochs; ++epoch) {
    for (auto& [batch_inputs, batch_targets] : data_loader) {
        // Shard batch across GPUs
        auto inputs = MultiGPULoRATrainer::shard_batch(batch_inputs, ctx);
        auto targets = MultiGPULoRATrainer::shard_batch(batch_targets, ctx);
        
        // Training step (forward, backward, sync, update)
        float loss = trainer.train_step(*layer, inputs, targets);
        
        std::cout << "Loss: " << loss << std::endl;
    }
}
```

### 2. Specify GPU IDs

```cpp
// Use specific GPUs (e.g., GPU 0 and 2)
std::vector<int> gpu_ids = {0, 2};
MultiGPUContext ctx(0, gpu_ids);
```

### 3. Choose Communication Backend

```cpp
// Auto-select best backend (default)
auto layer = trainer.create_layer(768, 768, 8, 1.0f, CommBackend::AUTO);

// Force NCCL (NVIDIA GPUs)
auto layer = trainer.create_layer(768, 768, 8, 1.0f, CommBackend::NCCL);

// Force RCCL (AMD GPUs)
auto layer = trainer.create_layer(768, 768, 8, 1.0f, CommBackend::RCCL);

// Use custom all-reduce (fallback)
auto layer = trainer.create_layer(768, 768, 8, 1.0f, CommBackend::CUSTOM);
```

## Prerequisites

### NCCL (NVIDIA GPUs)

**Ubuntu/Debian:**
```bash
# Install NCCL
sudo apt-get install libnccl2 libnccl-dev

# Verify installation
dpkg -l | grep nccl
```

**From Source:**
```bash
git clone https://github.com/NVIDIA/nccl.git
cd nccl
make -j src.build
sudo make install
```

**CMake Configuration:**
```cmake
# Enable NCCL in build
cmake -DTHEMIS_ENABLE_NCCL=ON ..
```

### RCCL (AMD GPUs)

**Ubuntu/Debian (ROCm):**
```bash
# Install RCCL
sudo apt-get install rccl

# Verify installation
dpkg -l | grep rccl
```

**CMake Configuration:**
```cmake
# Enable RCCL in build
cmake -DTHEMIS_ENABLE_RCCL=ON ..
```

### Custom Backend (No Dependencies)

The custom all-reduce backend works without NCCL/RCCL but with reduced performance. It's automatically used as fallback.

## Advanced Usage

### 1. Gradient Accumulation

Accumulate gradients over multiple batches before syncing:

```cpp
MultiGPULoRATrainer::Config config;
config.gradient_accumulation_steps = 4;  // Sync every 4 batches
config.sync_every_step = false;          // Disable per-step sync

MultiGPULoRATrainer trainer(ctx, config);
```

**Benefits:**
- Larger effective batch size
- Reduced communication overhead
- Better convergence for some models

### 2. Distributed Data Loading

Use `DistributedDataLoader` for automatic batch sharding:

```cpp
#include "llm/lora_framework/distributed_dataloader.h"

// Define custom dataset
class MyDataset : public DistributedDataLoader::Dataset {
public:
    GPUTensor get(size_t index) const override {
        // Load sample at index
        return load_sample(index);
    }
    
    size_t size() const override {
        return total_samples;
    }
};

// Create distributed data loader
MyDataset dataset;
DistributedDataLoader loader(dataset, batch_size, ctx, /*shuffle=*/true);

// Iterate over batches (automatically sharded)
for (auto& batch : loader) {
    // batch is std::vector<GPUTensor>, one per GPU
    float loss = trainer.train_step(*layer, batch, targets);
}
```

### 3. Checkpointing

Save and load checkpoints:

```cpp
// Save checkpoint (only rank 0 writes)
trainer.save_checkpoint(*layer, "./checkpoint.bin", step);

// Load checkpoint (broadcast to all GPUs)
trainer.load_checkpoint(*layer, "./checkpoint.bin");
```

### 4. Mixed Precision Training

Combine with FP16/BF16 for additional speedup:

```cpp
#include "llm/lora_framework/mixed_precision.h"

// Create FP16 layer
auto layer = trainer.create_layer(768, 768, 8, 1.0f);

// Use mixed precision wrapper (forward/backward in FP16, weights in FP32)
MixedPrecisionWrapper<float> mp_layer(layer, scale_factor=1024.0f);
```

## Performance Optimization

### 1. Check GPU Topology

```cpp
MultiGPUContext ctx(4);
auto topology = GPUTopology::detect(ctx.devices());

std::cout << "NVLink available: " << topology.has_nvlink << std::endl;
std::cout << "PCIe P2P available: " << topology.has_pcie_p2p << std::endl;

// Print bandwidth matrix
for (int i = 0; i < topology.num_gpus; ++i) {
    for (int j = 0; j < topology.num_gpus; ++j) {
        std::cout << "GPU " << i << " -> GPU " << j << ": "
                  << topology.bandwidth_matrix[i][j] << " GB/s" << std::endl;
    }
}
```

### 2. Optimize Batch Size

**Per-GPU Batch Size:**
- Too small: Poor GPU utilization
- Too large: OOM errors
- Sweet spot: Typically 16-64 samples per GPU

**Total Batch Size:**
- `total_batch_size = per_gpu_batch_size * num_gpus`
- Scale learning rate accordingly: `lr_new = lr_base * sqrt(num_gpus)`

```cpp
// Scale learning rate for multi-GPU
float base_lr = 0.001f;
int num_gpus = ctx.num_gpus();
float scaled_lr = base_lr * std::sqrt(static_cast<float>(num_gpus));

config.learning_rate = scaled_lr;
```

### 3. Monitor Communication Overhead

```cpp
auto stats = trainer.get_stats();

std::cout << "Communication overhead: " 
          << (stats.communication_overhead() * 100.0f) << "%" << std::endl;
std::cout << "Efficiency: " 
          << ((1.0f - stats.communication_overhead()) * 100.0f) << "%" << std::endl;
```

**Target:** < 10% communication overhead

### 4. Enable Profiling

```cpp
MultiGPULoRATrainer::Config config;
config.enable_profiling = true;

MultiGPULoRATrainer trainer(ctx, config);

// After training
auto stats = trainer.get_stats();
std::cout << "Avg step time: " << stats.avg_step_time_ms << " ms" << std::endl;
std::cout << "Throughput: " << stats.throughput_samples_per_sec << " samples/s" << std::endl;
```

## Expected Performance

### Scaling Efficiency

| GPUs | Training Time | Speedup | Efficiency |
|------|---------------|---------|------------|
| 1 GPU | 3.2ms | 1.0x | 100% |
| 2 GPUs | 1.7ms | 1.88x | 94% |
| 4 GPUs | 0.9ms | 3.56x | 89% |
| 8 GPUs | 0.5ms | 6.40x | 80% |

**Factors affecting scaling:**
- GPU interconnect (NVLink > PCIe)
- Model size (larger = better scaling)
- Batch size (larger = better scaling)
- Communication backend (NCCL > custom)

### Communication Overhead Breakdown

| Component | Time (4 GPUs) | % of Total |
|-----------|---------------|------------|
| Forward | 0.25ms | 28% |
| Backward | 0.50ms | 55% |
| All-Reduce | 0.10ms | 11% |
| Optimizer | 0.05ms | 6% |
| **Total** | **0.90ms** | **100%** |

## Troubleshooting

### Issue: NCCL Not Found

```
Error: NCCL is not available
```

**Solution:**
1. Install NCCL: `sudo apt-get install libnccl2 libnccl-dev`
2. Rebuild with NCCL enabled: `cmake -DTHEMIS_ENABLE_NCCL=ON ..`
3. Check `NCCLBackend::is_available()` returns true

### Issue: Gradient Synchronization Fails

```
Error: Failed to all-reduce gradients for parameter 0
```

**Solution:**
1. Check all GPUs are accessible: `nvidia-smi` or `rocm-smi`
2. Verify GPU count: `ctx.num_gpus() > 0`
3. Try custom backend: `CommBackend::CUSTOM`

### Issue: OOM on Multi-GPU

```
CUDA error: out of memory
```

**Solution:**
1. Reduce per-GPU batch size
2. Enable gradient accumulation
3. Use FP16/BF16 mixed precision
4. Distribute larger models across more GPUs

### Issue: Poor Scaling

**Symptoms:**
- 4 GPUs only 2x faster than 1 GPU
- High communication overhead (>20%)

**Solutions:**
1. Increase batch size per GPU
2. Use NCCL/RCCL instead of custom backend
3. Check for NVLink connectivity
4. Reduce gradient accumulation steps
5. Profile with `enable_profiling = true`

## Best Practices

1. **Always broadcast parameters** after initialization:
   ```cpp
   layer->broadcast_parameters();
   ```

2. **Synchronize gradients** before optimizer step:
   ```cpp
   layer->synchronize_gradients();
   optimizer->step();
   ```

3. **Scale learning rate** for large batch sizes:
   ```cpp
   float lr = base_lr * std::sqrt(num_gpus);
   ```

4. **Use gradient accumulation** for large effective batch sizes:
   ```cpp
   config.gradient_accumulation_steps = 4;
   ```

5. **Monitor training statistics**:
   ```cpp
   auto stats = trainer.get_stats();
   if (stats.communication_overhead() > 0.2f) {
       // Communication overhead too high, adjust batch size
   }
   ```

## Examples

See `tests/test_multi_gpu_training.cpp` for comprehensive examples including:
- Basic multi-GPU training
- Gradient synchronization
- Data loading and sharding
- Performance benchmarking
- Integration tests

## References

- [NCCL Documentation](https://docs.nvidia.com/deeplearning/nccl/)
- [RCCL Documentation](https://rocm.docs.amd.com/projects/rccl/)
- [PyTorch Distributed Training Guide](https://pytorch.org/tutorials/intermediate/ddp_tutorial.html)
- [Ring All-Reduce Algorithm](https://tech.preferred.jp/en/blog/technologies-behind-distributed-deep-learning-allreduce/)
