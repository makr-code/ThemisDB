---
name: Sharding Performance Issue
about: Report performance issues in distributed sharding, consensus, or transactions
title: '[SHARDING PERF] '
labels: ['type:performance', 'area:sharding', 'priority:P2']
assignees: ''
---

## Performance Issue Description
<!-- Clear description of the performance problem in the distributed system -->

## Component Affected
<!-- Check all that apply -->
- [ ] Consensus Module (Raft/Gossip/Paxos)
- [ ] Cross-Shard Transactions (2PC/3PC/SAGA/Percolator)
- [ ] Shard Router
- [ ] Metadata Operations
- [ ] Network Communication/RPC
- [ ] Replication/WAL
- [ ] Other: _______

## Performance Metrics
<!-- Provide specific metrics that demonstrate the issue -->

### Current Performance
- **Throughput**: <!-- e.g., 1,000 ops/sec (expected 10,000) -->
- **Latency**: <!-- e.g., p50: 100ms, p99: 500ms -->
- **Network Overhead**: <!-- e.g., 10 RTTs per transaction -->
- **CPU Usage**: <!-- e.g., 80% on coordinator -->
- **Memory Usage**: <!-- e.g., 4GB per node -->

### Expected Performance
- **Throughput**: <!-- Target ops/sec -->
- **Latency**: <!-- Target latency metrics -->
- **Network**: <!-- Expected network characteristics -->

## Configuration
```yaml
# Cluster configuration
sharding:
  consensus:
    type: # raft, paxos, gossip
    heartbeat_interval_ms: 
    batch_size:
  transactions:
    protocol: # two_phase_commit, etc.
    prepare_timeout_ms:
    commit_timeout_ms:
    enable_deadlock_detection:
```

## Environment
- **ThemisDB Version**: <!-- e.g., 1.4.0 -->
- **Cluster Size**: <!-- Number of nodes -->
- **Consensus Algorithm**: <!-- Raft, Paxos, Gossip -->
- **Transaction Protocol**: <!-- 2PC, 3PC, SAGA, Percolator -->
- **Network Topology**: <!-- Single-DC, Multi-DC, latency between nodes -->
- **Hardware**: <!-- CPU, RAM, Network bandwidth -->
- **Operating System**: <!-- e.g., Ubuntu 22.04 -->

## Workload Characteristics
<!-- Describe the workload causing the performance issue -->
- **Read/Write Ratio**: <!-- e.g., 70% reads, 30% writes -->
- **Cross-Shard Operations**: <!-- Percentage of ops crossing shards -->
- **Transaction Size**: <!-- Number of operations per transaction -->
- **Concurrency**: <!-- Number of concurrent clients/transactions -->
- **Data Distribution**: <!-- How data is distributed across shards -->

## Reproduction Steps
1. Configure cluster with above settings
2. Run workload: <!-- describe or provide benchmark script -->
3. Measure: <!-- what to measure and how -->

## Profiling Data
<!-- If available, include profiling data -->
```
# CPU profile hotspots
# Memory allocations
# Network traces
```

## Bottleneck Analysis
<!-- Your analysis of where the bottleneck might be -->
- [ ] Consensus proposal latency
- [ ] Two-phase commit coordination overhead
- [ ] Deadlock detection algorithm
- [ ] Metadata lookups
- [ ] Network serialization/deserialization
- [ ] Lock contention
- [ ] WAL/disk I/O
- [ ] Other: _______

## Proposed Optimizations
<!-- Optional: Suggestions for improvement -->

### Short-term Improvements
<!-- Quick wins, configuration tuning -->

### Long-term Optimizations
<!-- Architectural changes, algorithmic improvements -->

## Performance Impact
<!-- What is the business/user impact? -->
- [ ] Critical - Blocking production deployment
- [ ] High - Significantly affects user experience
- [ ] Medium - Noticeable but workable
- [ ] Low - Minor improvement opportunity

## Additional Context
<!-- Comparison with other distributed databases, benchmarks, graphs, metrics -->

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have provided specific performance metrics
- [ ] I have included cluster configuration
- [ ] I have described the workload characteristics
- [ ] I have provided steps to reproduce the performance issue
- [ ] I have attempted basic performance tuning
