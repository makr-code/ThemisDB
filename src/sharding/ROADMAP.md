# Sharding Production Readiness Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Introduction
Sharding is a database architecture pattern that involves breaking a database into smaller, more manageable pieces called shards. A comprehensive sharding strategy is essential for scaling applications effectively. This roadmap outlines the implementation phases for preparing the ThemisDB sharding architecture for production.

## Implementation Phases

### Phase 1: Consensus & Transaction Coordination (Status: Completed ✅)
- [x] Pluggable consensus framework supporting Raft, Gossip, and Paxos strategies
- [x] ConsensusFactory – runtime strategy selection and configuration
- [x] Cross-shard transaction coordinator with 2PC, 3PC, SAGA, and Percolator protocols
- [x] Distributed deadlock detection (wait-for-graph cycle detection)
- [x] Metadata sharding design and data-model documentation
- [x] Architecture documentation and ADRs

### Phase 2: Repair Engine & Observability (Status: Completed ✅)
- [x] ShardRepairEngine – automated shard repair orchestration
- [x] Reed-Solomon erasure decoder via Vandermonde matrix construction
- [x] Prometheus metrics for shard health, repair ops, and consensus latency
- [x] Admin API endpoints (shard status, force-repair, rebalance trigger)

### Phase 3: RPC Integration & Persistent State (Status: In Progress 🚧)
- [~] Full RPC integration for cross-shard read/write operations (`sharding/rpc/`) (Target: Q2 2026)
- [~] Persistent Paxos acceptor state (survives process restart) (Target: Q2 2026)
- [ ] Complete metadata shard implementation with consistent hashing (Target: Q3 2026)
- [ ] End-to-end cross-shard query routing layer (Target: Q3 2026)

### Phase 4: Hardening & Adaptive Rebalancing (Status: Planned 📋)
- [ ] gRPC transport with mTLS for all inter-shard RPC channels
- [ ] Adaptive rebalancer driven by per-shard access-pattern telemetry
- [ ] Reed-Solomon repair parallelisation across repair workers
- [ ] Raft snapshot compaction to bound log growth
- [ ] Chaos-engineering test suite (shard partition, node failure injection)

## Conclusion
Implementing sharding requires careful planning and execution. Following this roadmap will help ensure that the ThemisDB sharding architecture is robust, scalable, and ready for production deployment.