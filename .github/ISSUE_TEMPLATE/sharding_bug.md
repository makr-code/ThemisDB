---
name: Sharding/Distributed Systems Bug
about: Report a bug in ThemisDB's distributed sharding, consensus, or transaction coordination
title: '[SHARDING BUG] '
labels: ['type:bug', 'area:sharding', 'priority:P1']
assignees: ''
---

## Bug Description
<!-- A clear and concise description of the sharding/distributed systems bug -->

## Component Affected
<!-- Check all that apply -->
- [ ] Consensus Module (Raft/Gossip/Paxos)
- [ ] Cross-Shard Transaction Coordinator
- [ ] Shard Router
- [ ] Metadata Sharding
- [ ] Deadlock Detection
- [ ] Replication/WAL
- [ ] Other: _______

## Steps to Reproduce
1. Configure cluster with: <!-- e.g., 3 nodes, Paxos consensus -->
2. Execute operation: <!-- e.g., cross-shard transaction -->
3. Observe behavior: <!-- What goes wrong -->

## Expected Behavior
<!-- What should happen in a correctly functioning distributed system -->

## Actual Behavior
<!-- What actually happened -->

## Configuration
```yaml
# Paste relevant sharding configuration
sharding:
  consensus:
    type: # raft, paxos, or gossip
    node_id: 
    cluster_nodes: []
  transactions:
    protocol: # two_phase_commit, three_phase_commit, saga, percolator
    enable_deadlock_detection: 
```

## Environment
- **ThemisDB Version**: <!-- e.g., 1.4.0, 1.4.1-dev -->
- **Consensus Algorithm**: <!-- Raft, Paxos, or Gossip -->
- **Transaction Protocol**: <!-- 2PC, 3PC, SAGA, Percolator -->
- **Cluster Size**: <!-- Number of nodes -->
- **Operating System**: <!-- e.g., Ubuntu 22.04 -->
- **Network Topology**: <!-- Single-DC, Multi-DC, etc. -->

## Logs/Error Messages
<!-- Include relevant consensus logs, transaction logs, or error messages -->
```
# Paste logs here
# Look for: consensus proposals, transaction state, shard communication
```

## Impact Assessment
<!-- Check severity -->
- [ ] Data loss or corruption
- [ ] Split-brain scenario
- [ ] Transaction inconsistency
- [ ] Performance degradation
- [ ] Node failure/crash
- [ ] Other: _______

## Additional Context
<!-- Metrics, network latency, load patterns, timing diagrams -->

## Possible Root Cause
<!-- Optional: Your analysis of what might be causing the issue -->

---

**Checklist:**
- [ ] I have searched existing issues to ensure this is not a duplicate
- [ ] I have included cluster configuration details
- [ ] I have provided steps to reproduce across multiple nodes
- [ ] I have included relevant consensus/transaction logs
- [ ] I have documented the impact on data consistency
