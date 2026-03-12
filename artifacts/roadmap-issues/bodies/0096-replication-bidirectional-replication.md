### Context

This issue implements the roadmap item 'Bidirectional Replication' for the replication domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Bidirectional Replication

### Goal

Deliver the scoped changes for Bidirectional Replication in src/replication/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Bidirectional Replication
**Priority:** High  
**Target Version:** v1.7.0

Enable true bidirectional replication between two nodes with automatic conflict resolution, useful for active-active deployments.

**Features:**
- Symmetric replication (both nodes are primary)
- Conflict detection using timestamps and sequence numbers
- Configurable conflict resolution per table
- Origin tracking to prevent replication loops
- DDL replication with conflict detection

**Architecture:**
```cpp
class BidirectionalReplicationManager {
public:
    struct BidiConfig {
        std::string local_node_id;
        std::string remote_node_id;
        std::string remote_endpoint;
        
        // Conflict resolution
        ConflictResolution default_strategy = ConflictResolution::LAST_WRITE_WINS;
        std::map<std::string, ConflictResolution> collection_strategies;
        
        // Origin tracking
        bool track_origin = true;
        bool replicate_foreign_changes = false;  // Prevent loops
        
        // Sync
        uint32_t sync_interval_ms = 1000;
        bool bidirectional_sync = true;
    };
    
    explicit BidirectionalReplicationManager(const BidiConfig& config);
    
    // Start bidirectional replication
    bool start();
    void stop();
    
    // Check synchronization status
    struct SyncStatus {
        uint64_t local_sequence;
        uint64_t remote_sequence;
        int64_t lag_ms;
        uint64_t conflicts_last_hour;
        bool is_synchronized;
    };
    SyncStatus getSyncStatus() const;
    
    // Manual conflict resolution
    void resolveConflict(
        const std::string& document_id,
        const std::string& winner_node
    );
    
private:
    // Origin tracking
    struct OriginInfo {
        std::string origin_node;
        uint64_t origin_sequence;
        std::chrono::system_clock::time_point origin_timestamp;
    };
    
    OriginInfo getOrigin(const std::string& document_id);
    bool isLocalOrigin(const OriginInfo& origin);
};

// Example: Active-active setup
BidiConfig config;
config.local_node_id = "us-west-1";
config.remote_node_id = "us-east-1";
config.remote_endpoint = "us-east-1.example.com:7000";
config.default_strategy = ConflictResolution::LAST_WRITE_WINS;
config.collection_strategies["critical_data"] = ConflictResolution::CUSTOM;

BidirectionalReplicationManager bidi(config);
bidi.start();

// Monitor synchronization
auto status = bidi.getSyncStatus();
if (!status.is_synchronized) {
    std::cerr << "WARNING: Nodes not synchronized, lag=" 
              << status.lag_ms << "ms" << std::endl;
}
```

**Use Cases:**
- Active-active database pairs for high availability
- Cross-datacenter writes with low latency
- Disaster recovery without data loss

---

### Acceptance Criteria

- [ ] Symmetric replication (both nodes are primary)
- [ ] Conflict detection using timestamps and sequence numbers
- [ ] Configurable conflict resolution per table
- [ ] Origin tracking to prevent replication loops
- [ ] DDL replication with conflict detection
- [ ] Active-active database pairs for high availability
- [ ] Cross-datacenter writes with low latency
- [ ] Disaster recovery without data loss

### Relationships

- Roadmap row: #96 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/replication/FUTURE_ENHANCEMENTS.md#bidirectional-replication
- Source key: roadmap:96:replication:v1.7.0:bidirectional-replication

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:96:replication:v1.7.0:bidirectional-replication -->
<!-- roadmap-ref: row=96;module=replication;target=v1.7.0 -->
<!-- roadmap-detail: src/replication/FUTURE_ENHANCEMENTS.md#bidirectional-replication -->
