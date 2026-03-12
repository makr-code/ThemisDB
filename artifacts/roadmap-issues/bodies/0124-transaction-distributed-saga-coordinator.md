### Context

This issue implements the roadmap item 'Distributed SAGA Coordinator' for the transaction domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.9.0.

Primary detail section: Distributed SAGA Coordinator

### Goal

Deliver the scoped changes for Distributed SAGA Coordinator in src/transaction/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### Distributed SAGA Coordinator
**Priority:** High  
**Target Version:** v1.9.0

Cross-cluster SAGA coordination with failure recovery.

**Features:**
- Multi-cluster orchestration
- Persistent SAGA state
- Automatic recovery after coordinator crash
- Compensation retry policies
- SAGA visualization and debugging

**Architecture:**
```cpp
class DistributedSAGACoordinator {
public:
    struct RemoteStep {
        std::string service_endpoint;
        std::string operation;
        nlohmann::json params;
        std::string compensate_operation;
        nlohmann::json compensate_params;
    };
    
    struct DistributedSAGA {
        std::string saga_id;
        std::vector<RemoteStep> steps;
        std::map<std::string, std::string> context;  // Shared data
    };
    
    // Execute distributed SAGA
    Status executeDistributed(const DistributedSAGA& saga);
    
    // Recovery from crash
    void recoverInProgressSAGAs();
    
    // Query SAGA status across cluster
    SAGAStatus getDistributedStatus(const std::string& saga_id);
};

// Example: Multi-service SAGA
DistributedSAGACoordinator::DistributedSAGA saga;
saga.saga_id = "order-123";

saga.steps.push_back({
    .service_endpoint = "http://inventory:8080",
    .operation = "/reserve",
    .params = {{"sku", "ABC123"}, {"quantity", 5}},
    .compensate_operation = "/release",
    .compensate_params = {{"sku", "ABC123"}, {"quantity", 5}}
});

saga.steps.push_back({
    .service_endpoint = "http://payment:8080",
    .operation = "/charge",
    .params = {{"amount", 99.99}, {"currency", "USD"}},
    .compensate_operation = "/refund",
    .compensate_params = {{"amount", 99.99}}
});

saga_coordinator.executeDistributed(saga);
```

**Failure Recovery:**
- Persistent SAGA log in RocksDB
- Coordinator election for HA
- Automatic step retry with exponential backoff
- Manual intervention API for stuck SAGAs

---

### Acceptance Criteria

- [ ] Multi-cluster orchestration
- [ ] Persistent SAGA state
- [ ] Automatic recovery after coordinator crash
- [ ] Compensation retry policies
- [ ] SAGA visualization and debugging
- [ ] Persistent SAGA log in RocksDB
- [ ] Coordinator election for HA
- [ ] Automatic step retry with exponential backoff
- [ ] Manual intervention API for stuck SAGAs

### Relationships

- Roadmap row: #124 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/transaction/FUTURE_ENHANCEMENTS.md#distributed-saga-coordinator
- Source key: roadmap:124:transaction:v1.9.0:distributed-saga-coordinator

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:124:transaction:v1.9.0:distributed-saga-coordinator -->
<!-- roadmap-ref: row=124;module=transaction;target=v1.9.0 -->
<!-- roadmap-detail: src/transaction/FUTURE_ENHANCEMENTS.md#distributed-saga-coordinator -->
