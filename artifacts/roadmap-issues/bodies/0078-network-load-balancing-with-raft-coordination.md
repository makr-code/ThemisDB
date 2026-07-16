### Context

This issue implements the roadmap item 'Load Balancing with Raft Coordination' for the network domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Load Balancing with Raft Coordination

### Goal

Deliver the scoped changes for Load Balancing with Raft Coordination in src/network/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Load Balancing with Raft Coordination
**Priority:** High  
**Target Version:** v1.8.0

Add Raft-based load balancing for distributed query routing.

**Features:**
- Raft consensus for load balancer state
- Automatic failover on node failures
- Health-based routing decisions
- Dynamic weight adjustment based on load
- Cross-datacenter routing

**Architecture:**
```
┌─────────────────────────────────────────────────────┐
│          Raft Load Balancer Cluster                  │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐             │
│  │Leader   │  │Follower │  │Follower │             │
│  │(active) │  │(standby)│  │(standby)│             │
│  └─────────┘  └─────────┘  └─────────┘             │
│       │              │             │                 │
│       │   (consensus on routing decisions)          │
│       │              │             │                 │
│       └──────────────┴─────────────┘                │
│                      │                               │
└──────────────────────┼───────────────────────────────┘
                       │
         ┌─────────────┼─────────────┐
         │             │             │
    ┌────▼────┐  ┌────▼────┐  ┌────▼────┐
    │ Shard 1 │  │ Shard 2 │  │ Shard 3 │
    │ (node1) │  │ (node2) │  │ (node3) │
    └─────────┘  └─────────┘  └─────────┘
```

**API:**
```cpp
RaftLoadBalancer::Config config;
config.raft_port = 8774;
config.health_check_interval_ms = 5000;
config.rebalance_threshold = 0.2;  // 20% load imbalance

RaftLoadBalancer lb(config);
lb.add_backend("node1:8766", /*weight=*/1.0);
lb.add_backend("node2:8766", /*weight=*/1.0);
lb.add_backend("node3:8766", /*weight=*/1.0);
lb.start();

// Client-side
auto conn = lb.get_connection();
// Automatically routed to least loaded backend
```

**Load Balancing Strategies:**
- **Round Robin:** Simple, predictable
- **Least Connections:** Route to backend with fewest active connections
- **Weighted Round Robin:** Distribute based on capacity weights
- **Health-Based:** Exclude unhealthy backends
- **Consistent Hashing:** Sticky routing for caching

**Failover:**
- Raft leader monitors backend health
- Automatic removal of failed backends
- Automatic re-addition when backend recovers
- Leader election on LB leader failure

---

### Acceptance Criteria

- [ ] Raft consensus for load balancer state
- [ ] Automatic failover on node failures
- [ ] Health-based routing decisions
- [ ] Dynamic weight adjustment based on load
- [ ] Cross-datacenter routing
- [ ] **Round Robin:** Simple, predictable
- [ ] **Least Connections:** Route to backend with fewest active connections
- [ ] **Weighted Round Robin:** Distribute based on capacity weights
- [ ] **Health-Based:** Exclude unhealthy backends
- [ ] **Consistent Hashing:** Sticky routing for caching
- [ ] Raft leader monitors backend health
- [ ] Automatic removal of failed backends
- [ ] Automatic re-addition when backend recovers
- [ ] Leader election on LB leader failure

### Relationships

- Roadmap row: #78 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/network/FUTURE_ENHANCEMENTS.md#load-balancing-with-raft-coordination
- Source key: roadmap:78:network:v1.8.0:load-balancing-with-raft-coordination

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:78:network:v1.8.0:load-balancing-with-raft-coordination -->
<!-- roadmap-ref: row=78;module=network;target=v1.8.0 -->
<!-- roadmap-detail: src/network/FUTURE_ENHANCEMENTS.md#load-balancing-with-raft-coordination -->
