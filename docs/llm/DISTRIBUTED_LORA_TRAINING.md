# Distributed LoRA Training

## Overview

ThemisDB supports distributed LoRA (Low-Rank Adaptation) training across multiple shards, enabling efficient fine-tuning of large language models across a cluster of machines. The distributed training coordinator manages gradient synchronization, fault tolerance, and checkpoint management across all participating nodes.

## Architecture

### Components

1. **DistributedTrainingCoordinator**: Central orchestrator that manages the training process
2. **GradientAggregator**: Implements various gradient aggregation strategies (All-Reduce, Parameter Server, Ring All-Reduce)
3. **ShardRouter**: Handles inter-shard communication
4. **ShardTopology**: Manages shard discovery and health monitoring

### Gradient Synchronization Strategies

#### 1. All-Reduce
- **Description**: All shards exchange gradients and compute the average
- **Best for**: Small to medium clusters (2-8 nodes)
- **Communication**: O(N) per shard
- **Advantages**: Simple, balanced load
- **Disadvantages**: Communication overhead grows with cluster size

```yaml
distributed:
  sync_strategy: "all_reduce"
```

#### 2. Parameter Server
- **Description**: Central parameter server aggregates gradients from workers
- **Best for**: Heterogeneous clusters with different compute capacities
- **Communication**: O(1) per shard to server
- **Advantages**: Supports weighted averaging, good for heterogeneous hardware
- **Disadvantages**: Parameter server can be a bottleneck

```yaml
distributed:
  sync_strategy: "parameter_server"
  parameter_server:
    shard_weights:
      "shard-1": 1.0
      "shard-2": 2.0  # 2x weight if it has more compute
      "shard-3": 1.0
```

#### 3. Ring All-Reduce
- **Description**: Gradients flow through a ring of shards
- **Best for**: Large clusters (8+ nodes)
- **Communication**: O(1) per shard
- **Advantages**: Most efficient for large clusters, linear scaling
- **Disadvantages**: More complex implementation

```yaml
distributed:
  sync_strategy: "ring_all_reduce"
```

## Configuration

### Basic Setup

```yaml
lora_training:
  distributed:
    enabled: true
    coordinator_shard: "shard-coordinator"
    participant_shards:
      - "shard-1"
      - "shard-2"
      - "shard-3"
    sync_strategy: "all_reduce"
    gradient_accumulation_steps: 4
```

### Gradient Compression

Reduce bandwidth usage with gradient compression:

```yaml
distributed:
  compression: "quantization_8bit"  # 8-bit quantization (4x reduction)
  # Options:
  # - none: No compression
  # - quantization_8bit: 8-bit quantization (~4x compression)
  # - quantization_4bit: 4-bit quantization (~8x compression, more aggressive)
  # - sparse_topk: Send only top 10% of gradients by magnitude
```

### Fault Tolerance

Configure automatic recovery from shard failures:

```yaml
distributed:
  max_retry_attempts: 3
  timeout_seconds: 300
  enable_checkpointing: true
  checkpoint_frequency: 100
  checkpoint_path: "data/distributed_checkpoints"
```

## Usage

### API

#### Start Distributed Training

```bash
POST /lora/train/distributed
Content-Type: application/json

{
  "adapter_id": "my-distributed-adapter",
  "training_data": {
    "inputs": ["question 1", "question 2", "..."],
    "targets": ["answer 1", "answer 2", "..."]
  },
  "hyperparameters": {
    "rank": 16,
    "alpha": 32,
    "learning_rate": 0.0001,
    "num_epochs": 10,
    "batch_size": 32
  }
}
```

Response:
```json
{
  "success": true,
  "adapter_id": "my-distributed-adapter",
  "metrics": {
    "distributed_mode": true,
    "total_steps": 1000,
    "successful_steps": 998,
    "gradient_syncs": 250,
    "avg_sync_time_ms": 45.2,
    "active_shards": 3,
    "total_shards": 3,
    "effective_speedup": 2.8,
    "bandwidth_saved_gb": 1.2
  }
}
```

#### Monitor Training Progress

```bash
GET /lora/train/distributed/status?adapter_id=my-distributed-adapter
```

Response:
```json
{
  "adapter_id": "my-distributed-adapter",
  "status": "training",
  "current_step": 500,
  "total_steps": 1000,
  "active_shards": 3,
  "shard_states": {
    "shard-1": {
      "is_active": true,
      "current_step": 500,
      "current_loss": 0.42,
      "gpu_utilization": 0.95
    },
    "shard-2": {
      "is_active": true,
      "current_step": 500,
      "current_loss": 0.41,
      "gpu_utilization": 0.93
    },
    "shard-3": {
      "is_active": true,
      "current_step": 500,
      "current_loss": 0.43,
      "gpu_utilization": 0.94
    }
  }
}
```

### Programmatic Usage (C++)

```cpp
#include "llm/lora_framework/lora_training_service.h"

// Configure service
LoRATrainingService::Config config;
config.enable_distributed_training = true;
config.coordinator_shard = "shard-coordinator";
config.participant_shards = {"shard-1", "shard-2", "shard-3"};

// Create service
LoRATrainingService service(config);

// Prepare training data
TrainingData data;
data.inputs = {"input1", "input2", "..."};
data.targets = {"target1", "target2", "..."};

// Configure hyperparameters
LoRAHyperparameters hyper;
hyper.rank = 16;
hyper.alpha = 32;
hyper.learning_rate = 0.0001;
hyper.num_epochs = 10;
hyper.batch_size = 32;

// Train
auto result = service.trainDistributed("my-adapter", data, hyper);

if (result.success) {
    std::cout << "Training completed!" << std::endl;
    std::cout << "Effective speedup: " << result.metrics["effective_speedup"] << "x" << std::endl;
    std::cout << "Active shards: " << result.metrics["active_shards"] << std::endl;
}
```

## Monitoring & Metrics

### Key Metrics

| Metric | Description | Good Value |
|--------|-------------|------------|
| `effective_speedup` | Speedup vs single shard | Close to N (number of shards) |
| `avg_sync_time_ms` | Average gradient sync time | < 100ms for local network |
| `communication_overhead_pct` | % time spent on communication | < 20% |
| `bandwidth_saved_gb` | Bandwidth saved by compression | Higher with compression enabled |
| `shard_failures` | Number of shard failures | 0 in stable setup |
| `successful_recoveries` | Recovered from failures | Equal to shard_failures |

### Troubleshooting

#### High Sync Time
- **Symptom**: `avg_sync_time_ms > 1000ms`
- **Causes**: Network latency, large gradients
- **Solutions**: 
  - Enable gradient compression
  - Reduce batch size
  - Check network configuration
  - Use ring all-reduce for large clusters

#### Frequent Shard Failures
- **Symptom**: `shard_failures > 10`
- **Causes**: Network issues, GPU OOM, hardware failures
- **Solutions**:
  - Check shard logs
  - Verify hardware health
  - Reduce memory usage
  - Increase timeout settings

#### Low Speedup
- **Symptom**: `effective_speedup < N/2`
- **Causes**: Communication overhead, load imbalance
- **Solutions**:
  - Enable gradient accumulation
  - Balance data distribution
  - Use faster network (InfiniBand)
  - Enable compression

## Performance Tuning

### Gradient Accumulation

Accumulate gradients locally before synchronizing:

```yaml
distributed:
  gradient_accumulation_steps: 4  # Sync every 4 steps
  sync_frequency: 1
```

Benefits:
- Reduces communication frequency by 4x
- Better GPU utilization
- Lower communication overhead

### Compression Comparison

| Compression | Bandwidth Reduction | Accuracy Impact | CPU Overhead |
|-------------|---------------------|-----------------|--------------|
| None | 1x | None | None |
| 8-bit Quantization | 4x | Minimal (<0.1%) | Low |
| 4-bit Quantization | 8x | Small (<0.5%) | Low |
| Sparse Top-K | 10x | Moderate (1-2%) | Medium |

### Network Optimization

For production deployments:
- Use RDMA/InfiniBand for low-latency communication
- Enable NCCL for NVIDIA GPUs
- Configure network topology for optimal routing

## Best Practices

1. **Start Simple**: Begin with all-reduce and no compression
2. **Monitor Metrics**: Watch sync times and speedup
3. **Enable Checkpointing**: Protect against failures
4. **Use Compression**: For bandwidth-constrained environments
5. **Scale Gradually**: Test with 2-3 shards before scaling to larger clusters
6. **Balance Data**: Ensure equal data distribution across shards
7. **Health Checks**: Monitor shard health and GPU utilization

## Example: Training Llama 7B

```yaml
lora_training:
  base_model_path: "models/llama-2-7b.gguf"
  
  distributed:
    enabled: true
    coordinator_shard: "gpu-node-1"
    participant_shards:
      - "gpu-node-1"
      - "gpu-node-2"
      - "gpu-node-3"
      - "gpu-node-4"
    
    sync_strategy: "all_reduce"
    compression: "quantization_8bit"
    gradient_accumulation_steps: 8
    use_mixed_precision: true
    
    checkpoint_frequency: 500
    checkpoint_path: "/shared/checkpoints"

default_hyperparameters:
  rank: 32
  alpha: 64
  learning_rate: 0.0001
  num_epochs: 3
  batch_size: 16  # Per shard
```

Expected performance:
- Total batch size: 64 (16 per shard × 4 shards)
- Effective speedup: ~3.5x
- Avg sync time: ~50ms (with 10GbE network)
- Bandwidth usage: ~400MB/step (without compression)
- Bandwidth usage: ~100MB/step (with 8-bit compression)

## Security Considerations

### Byzantine Fault Detection

Protect against malicious or corrupted gradients:

```yaml
distributed:
  fault_tolerance:
    byzantine_detection: true
    detection_threshold: 0.05  # Flag if gradients differ >5% from median
```

### Secure Communication

Use TLS for inter-shard communication:

```yaml
network:
  tls:
    enabled: true
    min_version: "1.3"
    cert_file: "/etc/themisdb/certs/node.crt"
    key_file: "/etc/themisdb/certs/node.key"
    ca_file: "/etc/themisdb/certs/ca.crt"
```

## References

- [DistributedTrainingCoordinator API Documentation](../api/distributed_training_coordinator.md)
- [Gradient Aggregation Strategies](../algorithms/gradient_aggregation.md)
- [Fault Tolerance Guide](../operations/fault_tolerance.md)
- [Performance Tuning Guide](../performance/tuning.md)
