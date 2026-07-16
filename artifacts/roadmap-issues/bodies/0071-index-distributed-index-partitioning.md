### Context

This issue implements the roadmap item 'Distributed Index Partitioning' for the index domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Distributed Index Partitioning

### Goal

Deliver the scoped changes for Distributed Index Partitioning in src/index/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Distributed Index Partitioning
**Priority:** High  
**Target Version:** v1.7.0

Shard indexes across multiple nodes for horizontal scalability.

**Features:**
- **Hash Partitioning**: Distribute keys by hash(pk) % num_shards
- **Range Partitioning**: Split by key ranges (e.g., A-M, N-Z)
- **Consistent Hashing**: Minimize data movement on resharding
- **Shard Rebalancing**: Automatic redistribution on node add/remove
- **Distributed Queries**: Scatter-gather for cross-shard queries

**Architecture:**
```
┌────────────────┐
│ Query Planner  │
└────────┬───────┘
         │ Scatter
         ├─────────────┬─────────────┬─────────────┐
         ▼             ▼             ▼             ▼
    ┌────────┐    ┌────────┐    ┌────────┐    ┌────────┐
    │Shard 0 │    │Shard 1 │    │Shard 2 │    │Shard 3 │
    │ (A-F)  │    │ (G-M)  │    │ (N-S)  │    │ (T-Z)  │
    └────────┘    └────────┘    └────────┘    └────────┘
         │             │             │             │
         └─────────────┴─────────────┴─────────────┘
                       │ Gather
                       ▼
              ┌────────────────┐
              │ Merge Results  │
              └────────────────┘
```

**API:**
```cpp
// Configure partitioning
IndexManager::ShardingConfig config;
config.num_shards = 4;
config.strategy = ShardingStrategy::HASH;
config.replication_factor = 2;  // Replicate each shard 2x

index_manager->enableSharding(config);

// Queries automatically routed to correct shards
auto results = vim.search("embeddings", query_vector, 10);
// Internally: scatter to shards, merge top-k results
```

**Challenges:**
- Cross-shard joins and traversals
- Maintaining global statistics (IDF for full-text)
- Atomic multi-shard transactions
- Network latency for distributed queries

---

### Acceptance Criteria

- [ ] **Hash Partitioning**: Distribute keys by hash(pk) % num_shards
- [ ] **Range Partitioning**: Split by key ranges (e.g., A-M, N-Z)
- [ ] **Consistent Hashing**: Minimize data movement on resharding
- [ ] **Shard Rebalancing**: Automatic redistribution on node add/remove
- [ ] **Distributed Queries**: Scatter-gather for cross-shard queries
- [ ] Cross-shard joins and traversals
- [ ] Maintaining global statistics (IDF for full-text)
- [ ] Atomic multi-shard transactions
- [ ] Network latency for distributed queries

### Relationships

- Roadmap row: #71 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/index/FUTURE_ENHANCEMENTS.md#distributed-index-partitioning
- Source key: roadmap:71:index:v1.6.0:distributed-index-partitioning

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:71:index:v1.6.0:distributed-index-partitioning -->
<!-- roadmap-ref: row=71;module=index;target=v1.6.0 -->
<!-- roadmap-detail: src/index/FUTURE_ENHANCEMENTS.md#distributed-index-partitioning -->
