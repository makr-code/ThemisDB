# Distributed Training with ShardRouter and ShardTopology Integration

## Overview

This document describes the integration of **ShardRouter** and **ShardTopology** into the distributed LoRA training system, enabling real inter-shard communication for production deployments.

## Architecture

### Components

1. **TrainingServiceRegistry**: Singleton service registry for dependency injection
2. **LoRATrainingService**: Training orchestrator with shard infrastructure support
3. **DistributedTrainingCoordinator**: Handles gradient synchronization via RPC
4. **ShardRouter**: Routes RPC requests to appropriate shards
5. **ShardTopology**: Manages cluster topology and shard discovery

### Component Relationships

```
┌─────────────────────────────────────────────────────────────┐
│                    Training Application                      │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              TrainingServiceRegistry (Singleton)             │
│  • registerShardRouter(router)                              │
│  • registerShardTopology(topology)                          │
│  • getShardRouter() / getShardTopology()                    │
└──────────────┬──────────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────────┐
│                  LoRATrainingService                         │
│  Config:                                                     │
│  • shard_router                                             │
│  • shard_topology                                           │
│  • enable_distributed_training                              │
│  • participant_shards                                       │
└──────────────┬──────────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────────┐
│            DistributedTrainingCoordinator                    │
│  • collectGradients() via ShardRouter RPC                   │
│  • broadcastGradients() via ShardRouter RPC                 │
│  • checkShardHealth() via ShardRouter RPC                   │
│  • validateShardParticipation() via ShardTopology           │
└──────────┬──────────────────────────────────┬───────────────┘
           │                                  │
           ▼                                  ▼
┌───────────────────────┐          ┌──────────────────────────┐
│    ShardRouter        │          │    ShardTopology         │
│  • executeQuery()     │          │  • getHealthyShards()    │
│  • routeRequest()     │          │  • getShard()            │
│  • get/put/del        │          │  • addShard()            │
└───────────────────────┘          └──────────────────────────┘
           │
           ▼
┌───────────────────────────────────────────────────────────┐
│                    Shard Cluster                          │
│  Shard-1 ◄──► Shard-2 ◄──► Shard-3 ◄──► ... ◄──► Shard-N │
└───────────────────────────────────────────────────────────┘
```

## Usage

### Option 1: Direct Dependency Injection (Recommended)

```cpp
#include "llm/lora_framework/lora_training_service.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"

// 1. Create shard infrastructure
auto shard_topology = std::make_shared<ShardTopology>(topology_config);
auto shard_router = std::make_shared<ShardRouter>(
    urn_resolver, remote_executor, router_config
);

// 2. Configure training service with shard infrastructure
LoRATrainingService::Config config;
config.enable_distributed_training = true;
config.coordinator_shard = "shard-1";
config.participant_shards = {"shard-1", "shard-2", "shard-3"};
config.shard_router = shard_router;           // Inject directly
config.shard_topology = shard_topology;       // Inject directly

// 3. Create service (automatically registers with registry)
LoRATrainingService service(config);

// 4. Execute distributed training
auto result = service.trainDistributed(adapter_id, training_data);
```

### Option 2: Via Service Registry

```cpp
#include "llm/lora_framework/training_service_registry.h"

// 1. Register globally
auto& registry = TrainingServiceRegistry::getInstance();
registry.registerShardRouter(shard_router);
registry.registerShardTopology(shard_topology);

// 2. Configure training (will use registry)
LoRATrainingService::Config config;
config.enable_distributed_training = true;
config.coordinator_shard = "shard-1";
config.participant_shards = {"shard-1", "shard-2", "shard-3"};
// No need to set shard_router/shard_topology - uses registry

// 3. Create service
LoRATrainingService service(config);

// 4. Execute distributed training
auto result = service.trainDistributed(adapter_id, training_data);
```

### Option 3: Standalone Mode (No Infrastructure)

```cpp
// Configure without shard infrastructure
LoRATrainingService::Config config;
config.enable_distributed_training = true;
config.participant_shards = {"shard-1", "shard-2", "shard-3"};
// No shard_router or shard_topology provided

// Training will run in standalone mode with simulated gradients
LoRATrainingService service(config);
auto result = service.trainDistributed(adapter_id, training_data);
```

## Deployment Modes

### 1. Standalone Mode
- **Use Case**: Development, testing, single-machine training
- **Configuration**: No ShardRouter/ShardTopology provided
- **Behavior**: Simulated gradients, no network communication
- **Performance**: Fast, no network overhead

### 2. Development Mode
- **Use Case**: Local multi-shard testing
- **Configuration**: Local ShardRouter with mock shards
- **Behavior**: Real RPC calls to local processes
- **Performance**: Network overhead of localhost

### 3. Production Mode
- **Use Case**: Multi-datacenter deployment
- **Configuration**: Full ShardRouter/ShardTopology with gRPC/NCCL
- **Behavior**: Real inter-shard communication
- **Performance**: Optimized for distributed training

## RPC Protocol Details

### Gradient Collection

**Request Format:**
```json
{
  "adapter_id": "adapter-123",
  "step_number": 42,
  "timeout_ms": 30000
}
```

**RPC Query:**
```
collect_gradients:{request_json}
```

**Response Format:**
```json
{
  "gradients": [
    {
      "layer_name": "lora_q_proj",
      "source_shard": "shard-1",
      "step_number": 42,
      "timestamp_ms": 1234567890,
      "shape": [64, 64],
      "data": [0.1, 0.2, ...],
      "compression_type": 0
    }
  ]
}
```

### Gradient Broadcasting

**Request Format:**
```json
{
  "step_number": 42,
  "gradients": [
    {
      "layer_name": "lora_q_proj",
      "data": [0.1, 0.2, ...],
      "shape": [64, 64],
      "compression_type": 1
    }
  ]
}
```

**RPC Query:**
```
apply_gradients:{request_json}
```

**Response Format:**
```json
{
  "success": true
}
```

### Health Check

**Request Format:**
```json
{
  "type": "health_check",
  "timestamp": 1234567890
}
```

**RPC Query:**
```
health_check:{request_json}
```

**Response Format:**
```json
{
  "is_active": true,
  "gpu_utilization": 0.85,
  "memory_usage_gb": 12.5,
  "last_heartbeat_ms": 1234567890
}
```

### Ping

**Request Format:**
```json
{
  "type": "ping",
  "timestamp": 1234567890
}
```

**RPC Query:**
```
ping:{request_json}
```

**Response Format:**
```json
{
  "success": true
}
```

## Configuration Options

### LoRATrainingService::Config

```cpp
struct Config {
    // Distributed training
    bool enable_distributed_training = false;
    std::string coordinator_shard;
    std::vector<std::string> participant_shards;
    
    // Shard infrastructure (optional)
    std::shared_ptr<ShardRouter> shard_router;
    std::shared_ptr<ShardTopology> shard_topology;
    bool auto_discover_shards = true;
    
    // Training parameters
    LoRAHyperparameters default_hyperparameters;
    std::string base_model_path;
    
    // Production features
    MixedPrecisionConfig mixed_precision;
    LRSchedulerConfig lr_scheduler;
    GradientClippingConfig gradient_clipping;
    GradientAccumulationConfig gradient_accumulation;
};
```

### DistributedTrainingConfig

```cpp
struct DistributedTrainingConfig {
    SyncStrategy sync_strategy = SyncStrategy::ALL_REDUCE;
    GradientCompressionType compression = GradientCompressionType::NONE;
    
    std::string coordinator_shard;
    std::vector<std::string> participant_shards;
    
    int gradient_accumulation_steps = 1;
    int sync_frequency = 1;
    float gradient_clip_norm = 1.0f;
    
    bool use_mixed_precision = false;
    bool sparse_gradients = false;
    float sparse_threshold = 1e-6f;
    
    int max_retry_attempts = 3;
    int timeout_seconds = 300;
    
    bool enable_checkpointing = true;
    int checkpoint_frequency = 100;
    std::string checkpoint_path;
};
```

## Error Handling

### Shard Failures

The coordinator automatically handles shard failures:

1. **Detection**: Health checks detect unresponsive shards
2. **Marking**: Shard marked as inactive after consecutive failures
3. **Removal**: Inactive shards removed from active shard list
4. **Continuation**: Training continues with remaining shards
5. **Logging**: All failures logged with spdlog

```cpp
bool handleShardFailure(const std::string& failed_shard) {
    // Remove from active shards
    active_shards_.erase(...);
    
    // Update state
    shard_states_[failed_shard].is_active = false;
    stats_.shard_failures++;
    
    // Check if can continue
    if (active_shards_.empty()) {
        return false;  // Cannot continue
    }
    
    return true;  // Continue with remaining shards
}
```

### Network Timeouts

Configurable timeout handling:

```cpp
dist_config.timeout_seconds = 300;  // 5 minutes
dist_config.max_retry_attempts = 3;
```

### Gradient Collection Failures

Partial failures handled gracefully:

```cpp
try {
    auto response = shard_router_->executeQuery(rpc_query);
    // Parse gradients
} catch (const std::exception& e) {
    spdlog::error("Failed to collect from {}: {}", shard_id, e.what());
    handleShardFailure(shard_id);
    // Continue with other shards
}
```

## Performance Optimization

### Gradient Compression

Enable compression to reduce network bandwidth:

```cpp
dist_config.compression = GradientCompressionType::QUANTIZATION_8BIT;
```

**Compression Types:**
- `NONE`: No compression (default)
- `QUANTIZATION_8BIT`: 8-bit quantization (~4x reduction)
- `QUANTIZATION_4BIT`: 4-bit quantization (~8x reduction)
- `SPARSE_TOPK`: Send only top-K gradients (~10x reduction)
- `ERROR_FEEDBACK`: Compression with error feedback

### Parallel Broadcasting

Gradients broadcast to all shards in parallel:

```cpp
std::vector<std::future<bool>> futures;
for (const auto& shard_id : active_shards_) {
    futures.push_back(std::async(std::launch::async, [&, shard_id]() {
        // Send to shard
        return shard_router_->executeQuery(rpc_query);
    }));
}
```

### Mixed Precision

Reduce gradient size with FP16/BF16:

```cpp
training_config.mixed_precision.enabled = true;
training_config.mixed_precision.dtype = PrecisionType::FP16;
```

## Monitoring and Metrics

### Training Metrics

```cpp
struct TrainingResult {
    bool success;
    float final_loss;
    int epochs_completed;
    std::chrono::seconds training_time;
    
    json metrics = {
        {"distributed_mode", true},
        {"total_steps", 1000},
        {"successful_steps", 995},
        {"gradient_syncs", 250},
        {"avg_sync_time_ms", 45.2},
        {"active_shards", 3},
        {"total_shards", 3},
        {"effective_speedup", 2.8},
        {"communication_overhead_pct", 15.3}
    };
};
```

### Shard Health Metrics

```cpp
struct ShardTrainingState {
    std::string shard_id;
    int current_step;
    float current_loss;
    bool is_active;
    float gpu_utilization;
    float memory_usage_gb;
    int64_t last_heartbeat_ms;
    int consecutive_failures;
};
```

## Testing

### Unit Tests

Run registry tests:
```bash
./tests/test_training_service_registry
```

### Integration Tests

Run distributed training tests:
```bash
./tests/test_distributed_training_coordinator
```

### Example

Run complete example:
```bash
./examples/example_distributed_lora_training
```

## Security Considerations

### PKI/mTLS Support

ShardRouter integrates with PKI infrastructure:

```cpp
ShardInfo shard;
shard.certificate_serial = "X509-SERIAL-NUMBER";
shard.capabilities = {"read", "write", "admin"};
```

### Authentication

All RPC requests authenticated via mTLS certificates.

### Authorization

Capability-based access control:

```cpp
if (!shard_info.hasCapability("write")) {
    throw std::runtime_error("Shard lacks write capability");
}
```

## Troubleshooting

### Issue: "No ShardRouter available, using simulated gradients"

**Cause**: ShardRouter not registered
**Solution**: 
```cpp
registry.registerShardRouter(shard_router);
```

### Issue: "Shard X not available in topology"

**Cause**: Shard not registered in ShardTopology
**Solution**:
```cpp
ShardInfo shard_info;
shard_info.shard_id = "shard-X";
shard_topology->addShard(shard_info);
```

### Issue: "Failed to collect gradients from shard"

**Cause**: Network issues, shard down, or timeout
**Solution**: Check shard health, increase timeout, verify network

### Issue: "Health check failed for shard"

**Cause**: Shard unresponsive or overloaded
**Solution**: Check shard logs, verify resources, restart if needed

## Future Enhancements

1. **NCCL Backend**: Direct GPU-to-GPU communication
2. **RDMA Support**: InfiniBand for low-latency
3. **Adaptive Compression**: Dynamic compression based on bandwidth
4. **Gradient Caching**: Cache recent gradients for fault tolerance
5. **Hierarchical All-Reduce**: Multi-level aggregation
6. **Dynamic Shard Discovery**: Automatic topology updates

## References

- [ShardRouter API](../../include/sharding/shard_router.h)
- [ShardTopology API](../../include/sharding/shard_topology.h)
- [DistributedTrainingCoordinator API](../../include/llm/distributed_training_coordinator.h)
- [TrainingServiceRegistry API](../../include/llm/lora_framework/training_service_registry.h)
- [Example Code](../../examples/example_distributed_lora_training.cpp)
- [Unit Tests](../../tests/test_training_service_registry.cpp)
