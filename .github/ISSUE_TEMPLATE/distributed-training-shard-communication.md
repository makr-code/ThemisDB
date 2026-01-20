---
name: "Distributed Training: ShardRouter/ShardTopology Integration"
about: Implement full inter-shard RPC communication for distributed training
title: "[Distributed Training] Integrate ShardRouter and ShardTopology for Inter-Shard Communication"
labels: priority:P1, type:enhancement, area:sharding, effort:large, component:distributed-training
assignees: ''
---

## 🎯 Objective

Replace null ShardRouter/ShardTopology placeholders with proper dependency injection to enable full inter-shard RPC communication for distributed training.

**Status:** 📋 Planned  
**Priority:** P1 (High)  
**Effort:** 4-5 weeks  
**Dependencies:** 
- Distributed training coordinator integration (✅ COMPLETE)
- ShardRouter RPC infrastructure (✅ EXISTS)
- ShardTopology service discovery (✅ EXISTS)

## 📋 Background

The current distributed training implementation creates the coordinator with null ShardRouter/ShardTopology for standalone operation. This was intentional for the initial implementation, but production deployments require actual inter-shard communication.

**Current Implementation:**
```cpp
// NOTE: Standalone mode - no inter-shard communication
std::shared_ptr<ShardRouter> shard_router = nullptr;
std::shared_ptr<ShardTopology> shard_topology = nullptr;

auto coordinator = DistributedTrainingCoordinatorFactory::create(
    shard_router, 
    shard_topology, 
    dist_config
);
```

**Target Implementation:**
```cpp
// Full inter-shard communication
auto shard_router = getShardRouterFromRegistry();
auto shard_topology = getShardTopologyFromRegistry();

auto coordinator = DistributedTrainingCoordinatorFactory::create(
    shard_router, 
    shard_topology, 
    dist_config
);
```

## 🔧 Implementation Tasks

### 1. Service Registry for Dependency Injection (Week 1)

**Files to Create:**
- [ ] `include/llm/lora_framework/training_service_registry.h` - Service registry interface
- [ ] `src/llm/lora_framework/training_service_registry.cpp` - Implementation

**Design:**
```cpp
class TrainingServiceRegistry {
public:
    static TrainingServiceRegistry& getInstance();
    
    // Register shard infrastructure
    void registerShardRouter(std::shared_ptr<ShardRouter> router);
    void registerShardTopology(std::shared_ptr<ShardTopology> topology);
    
    // Retrieve for dependency injection
    std::shared_ptr<ShardRouter> getShardRouter();
    std::shared_ptr<ShardTopology> getShardTopology();
    
    // Check availability
    bool hasShardInfrastructure() const;
};
```

**Testing:**
- [ ] Unit tests for registry operations
- [ ] Thread-safety tests for concurrent access
- [ ] Integration tests with mock shards

---

### 2. Update LoRATrainingService Constructor (Week 1-2)

**Files to Modify:**
- [ ] `include/llm/lora_framework/lora_training_service.h` - Add registry injection
- [ ] `src/llm/lora_framework/lora_training_service.cpp` - Use registry

**Changes Required:**
```cpp
class LoRATrainingService {
public:
    struct Config {
        // ... existing fields ...
        
        // Distributed training infrastructure (optional)
        std::shared_ptr<ShardRouter> shard_router;
        std::shared_ptr<ShardTopology> shard_topology;
        bool auto_discover_shards = true;
    };
    
    explicit LoRATrainingService(const Config& config);
};
```

**Implementation:**
```cpp
LoRATrainingService::Impl::Impl(const Config& config) 
    : config_(config) {
    
    // Register shard infrastructure if provided
    if (config.shard_router && config.shard_topology) {
        TrainingServiceRegistry::getInstance()
            .registerShardRouter(config.shard_router);
        TrainingServiceRegistry::getInstance()
            .registerShardTopology(config.shard_topology);
        
        spdlog::info("Shard infrastructure registered for distributed training");
    }
}
```

**Testing:**
- [ ] Test with and without shard infrastructure
- [ ] Verify backward compatibility
- [ ] Test auto-discovery mode

---

### 3. Implement RPC for Gradient Collection (Week 2-3)

**Files to Modify:**
- [ ] `src/llm/distributed_training_coordinator.cpp` - Use ShardRouter for RPC

**Current (Simulated):**
```cpp
std::map<std::string, std::vector<GradientTensor>> 
DistributedTrainingCoordinator::collectGradients(int step_number) {
    // Creates dummy gradients for testing
    for (const auto& shard_id : active_shards_) {
        GradientTensor dummy;
        dummy.data.resize(64 * 64, 0.1f);
        // ...
    }
}
```

**Target (Real RPC):**
```cpp
std::map<std::string, std::vector<GradientTensor>> 
DistributedTrainingCoordinator::collectGradients(int step_number) {
    if (!shard_router_) {
        // Fallback to simulated mode
        spdlog::warn("No ShardRouter, using simulated gradients");
        return simulatedGradients();
    }
    
    // Real RPC to shards
    std::map<std::string, std::vector<GradientTensor>> collected;
    
    for (const auto& shard_id : active_shards_) {
        try {
            // Send RPC request to shard
            auto request = GradientCollectionRequest{
                .adapter_id = adapter_id_,
                .step_number = step_number,
                .timeout_ms = config_.timeout_seconds * 1000
            };
            
            auto response = shard_router_->sendRequest(
                shard_id,
                "collect_gradients",
                request.toJSON()
            );
            
            // Parse response
            auto gradients = parseGradientResponse(response);
            collected[shard_id] = gradients;
            
        } catch (const std::exception& e) {
            spdlog::error("Failed to collect gradients from {}: {}", 
                        shard_id, e.what());
            handleShardFailure(shard_id);
        }
    }
    
    return collected;
}
```

**Testing:**
- [ ] Integration tests with mock ShardRouter
- [ ] Test with network failures/timeouts
- [ ] Test with partial shard responses
- [ ] Performance tests (latency, throughput)

---

### 4. Implement RPC for Gradient Broadcasting (Week 3-4)

**Files to Modify:**
- [ ] `src/llm/distributed_training_coordinator.cpp` - Broadcast via ShardRouter

**Implementation:**
```cpp
bool DistributedTrainingCoordinator::broadcastGradients(
    const std::vector<GradientTensor>& gradients,
    int step_number
) {
    if (!shard_router_) {
        spdlog::warn("No ShardRouter, skipping broadcast");
        return true;  // Simulate success
    }
    
    // Compress gradients if configured
    auto compressed = config_.compression != GradientCompressionType::NONE ?
        compressGradients(gradients) : gradients;
    
    // Broadcast to all shards in parallel
    std::vector<std::future<bool>> futures;
    
    for (const auto& shard_id : active_shards_) {
        futures.push_back(std::async(std::launch::async, [&, shard_id]() {
            try {
                auto request = GradientBroadcastRequest{
                    .gradients = compressed,
                    .step_number = step_number
                };
                
                shard_router_->sendRequest(
                    shard_id,
                    "apply_gradients",
                    request.toJSON()
                );
                
                return true;
            } catch (const std::exception& e) {
                spdlog::error("Failed to broadcast to {}: {}", 
                            shard_id, e.what());
                return false;
            }
        }));
    }
    
    // Wait for all broadcasts
    bool all_success = true;
    for (auto& future : futures) {
        all_success &= future.get();
    }
    
    return all_success;
}
```

**Testing:**
- [ ] Test parallel broadcast to multiple shards
- [ ] Test with network failures
- [ ] Verify gradient compression/decompression
- [ ] Performance benchmarks

---

### 5. Shard Discovery and Health Monitoring (Week 4-5)

**Files to Modify:**
- [ ] `src/llm/distributed_training_coordinator.cpp` - Use ShardTopology

**Implementation:**
```cpp
bool DistributedTrainingCoordinator::validateShardParticipation() {
    if (!shard_topology_) {
        spdlog::warn("No ShardTopology, skipping validation");
        return true;
    }
    
    // Query topology for available shards
    auto available_shards = shard_topology_->getActiveShards();
    
    // Verify all participant shards are available
    for (const auto& shard_id : config_.participant_shards) {
        if (std::find(available_shards.begin(), available_shards.end(), 
                     shard_id) == available_shards.end()) {
            spdlog::error("Shard {} not available in topology", shard_id);
            return false;
        }
        
        // Send ping to verify reachability
        if (!pingShard(shard_id)) {
            spdlog::error("Shard {} not reachable", shard_id);
            return false;
        }
    }
    
    return true;
}

std::map<std::string, ShardTrainingState> 
DistributedTrainingCoordinator::checkShardHealth() {
    if (!shard_router_) {
        // Simulate healthy shards
        return simulateHealthyShards();
    }
    
    // Real health checks via RPC
    for (auto& [shard_id, state] : shard_states_) {
        try {
            auto health = shard_router_->getShardHealth(shard_id);
            
            state.is_active = health.is_active;
            state.gpu_utilization = health.gpu_utilization;
            state.memory_usage_gb = health.memory_usage_gb;
            state.last_heartbeat_ms = health.last_heartbeat_ms;
            
        } catch (const std::exception& e) {
            spdlog::warn("Health check failed for {}: {}", shard_id, e.what());
            state.consecutive_failures++;
        }
    }
    
    return shard_states_;
}
```

**Testing:**
- [ ] Test shard discovery with ShardTopology
- [ ] Test health monitoring with various shard states
- [ ] Test failover when shards become unhealthy
- [ ] Load testing with many shards

---

### 6. Update trainDistributed() Method (Week 5)

**Files to Modify:**
- [ ] `src/llm/lora_framework/lora_training_service.cpp` - Use registry

**Changes:**
```cpp
TrainingResult LoRATrainingService::trainDistributed(...) {
    // ... validation ...
    
    // Get shard infrastructure from registry
    auto registry = TrainingServiceRegistry::getInstance();
    auto shard_router = registry.hasShardInfrastructure() ?
        registry.getShardRouter() : nullptr;
    auto shard_topology = registry.hasShardInfrastructure() ?
        registry.getShardTopology() : nullptr;
    
    if (!shard_router || !shard_topology) {
        spdlog::warn("ShardRouter/ShardTopology not available");
        spdlog::info("Running in standalone mode (simulated gradients)");
    } else {
        spdlog::info("Using ShardRouter for inter-shard communication");
    }
    
    // Create coordinator with real or null dependencies
    auto coordinator = DistributedTrainingCoordinatorFactory::create(
        shard_router, 
        shard_topology, 
        dist_config
    );
    
    // ... rest of training logic ...
}
```

**Testing:**
- [ ] Test with full shard infrastructure
- [ ] Test fallback to standalone mode
- [ ] End-to-end distributed training test

---

## 📊 Acceptance Criteria

- [ ] ShardRouter and ShardTopology can be injected via service registry
- [ ] LoRATrainingService constructor accepts shard infrastructure
- [ ] Coordinator uses real RPC for gradient collection
- [ ] Coordinator uses real RPC for gradient broadcasting
- [ ] Shard discovery works via ShardTopology
- [ ] Health monitoring works via ShardRouter
- [ ] Graceful fallback to standalone mode when infrastructure unavailable
- [ ] All existing tests pass
- [ ] New integration tests with real shards
- [ ] Documentation updated with deployment guide
- [ ] Performance benchmarks show acceptable overhead (< 10%)

## 📈 Expected Benefits

- **Production ready** distributed training with real inter-shard communication
- **Scalability** to hundreds of shards across data centers
- **Fault tolerance** with automatic shard discovery and failover
- **Observability** with real-time shard health monitoring
- **Flexibility** with different network topologies and protocols

## 🔗 Related Issues

- Distributed Training Integration (PR #XXX) - Initial standalone implementation
- ShardRouter Enhancement - Add training-specific RPC methods
- ShardTopology Enhancement - Shard affinity and locality awareness

## 📝 Additional Notes

**Deployment Modes:**
1. **Standalone** (current): No ShardRouter/Topology, simulated gradients
2. **Development**: Local shards with mock RPC
3. **Production**: Real shards with NCCL/gRPC/RDMA

**RPC Protocol Options:**
- **gRPC**: Standard, good for most deployments
- **NCCL**: Optimized for NVIDIA GPUs, lowest latency
- **RDMA**: InfiniBand, best for HPC clusters
- **HTTP/2**: Fallback for cloud deployments

**Configuration Example:**
```yaml
lora_training:
  distributed:
    enabled: true
    coordinator_shard: "shard-1"
    participant_shards: ["shard-1", "shard-2", "shard-3"]
    
    # Infrastructure (auto-discovered if not specified)
    shard_router:
      protocol: grpc  # grpc, nccl, rdma
      address: "coordinator:50051"
    
    shard_topology:
      discovery: consul  # consul, etcd, static
      address: "consul:8500"
```
