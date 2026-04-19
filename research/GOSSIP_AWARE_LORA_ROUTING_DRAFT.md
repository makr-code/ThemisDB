# Gossip-Aware LoRA Routing: Federated Fine-Tuning at Scale

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  

---

## I. Executive Summary

This draft explores **Gossip-Aware LoRA Routing**: a decentralized approach to federated model fine-tuning where LoRA adapters are replicated across nodes using epidemic gossip protocols, and query routing is aware of adapter availability and freshness.

**Key Innovation**: Instead of a central LoRA repository, adapters propagate via gossip; queries are routed to nodes with fresh, relevant adapters; conflict resolution via last-write-wins with vector clocks.

## II. Problem Statement

Current LoRA deployment assumes:
- Centralized LoRA repository (single point of contention)
- Query dispatcher must consult central adapter registry
- Latency increases with cluster size
- Failures in central node block all LoRA serving

**Proposed Solution**: Gossip-based LoRA replication + topology-aware query routing

## III. Architecture Overview

### 1. Gossip Protocol Layer
- Each node maintains a local LoRA cache (adapter_id → weights)
- Periodic gossip exchanges: "Hello, I have adapters {A1, A2, A3} at version {v1, v2, v3}"
- Lazy replication: Adapters transferred on-demand (only if query routed to node)
- Conflict resolution: Vector clock + last-write-wins

### 2. Query Routing
- Query arrives with hint: "Prefer LoRA adapter A5"
- Router checks local cache: `A5 in cache? → serve locally`
- Not in cache: `Gossip neighbor map → identify node with A5 → route query`
- If multiple nodes have A5: choose by `(freshness, latency)`

### 3. Consistency Model
- **Eventual Consistency**: New LoRA adapters propagate via gossip (seconds to minutes)
- **Read Your Writes**: Same-session queries see consistent adapter versions
- **Causal Consistency**: Vector clocks order adapter updates

## IV. Implementation Considerations

### Gossip Protocol Details
- **Fanout**: 3–5 neighbors per round (typical epidemic parameters)
- **Round Duration**: 100ms (tunable)
- **Adapter Size**: Typical LoRA (rank-16–64) ~10–50MB; manageable in gossip payloads
- **Cache Eviction**: LRU with adaptive size based on node memory

### Query Routing Algorithm
```
route_query(query, adapter_id):
  if adapter_id in local_cache:
    return execute_locally()
  
  candidates = gossip_neighborhood[adapter_id]
  if candidates.empty():
    return NOT_FOUND
  
  best_node = candidates.min_by(freshness, latency)
  return forward_to(best_node, query)
```

### Consistency Guarantees
- **Adapters propagate in <5 gossip rounds** (~500ms worst-case)
- **Cache coherence via vector clocks**: If node A updates A5 to v2, all nodes will eventually receive v2 marked with vector clock
- **Conflict resolution**: During merge, keep adapter with higher vector clock; if tied, keep by lexicographic adapter_id

## V. Implementation Phases

### Phase 1: Gossip Foundation (Pending)
- [ ] Implement epidemic gossip protocol (select/fanout/merge logic)
- [ ] LoRA cache per node + eviction policy
- [ ] Vector clock per adapter

### Phase 2: Query Routing Integration (Pending)
- [ ] Extend query executor with adapter availability check
- [ ] Implement topology-aware routing algorithm
- [ ] Add metrics: adapter hit rate, routing latency, cache efficiency

### Phase 3: Convergence & Recovery (Pending)
- [ ] Simulate node failures; validate adapter recovery
- [ ] Measure propagation latency under various topologies
- [ ] Test cache incoherence scenarios

### Phase 4: Production Hardening (Pending)
- [ ] Rate limiting on gossip messages (prevent amplification)
- [ ] Adapter signature verification (prevent poisoning)
- [ ] Monitoring + alerting for consistency violations

## VI. Expected Benefits

| Metric | Centralized | Gossip-Aware | Improvement |
|--------|------------|--------------|-------------|
| P99 routing latency | 50ms | 10ms (local) / 30ms (remote) | 5–50x |
| Central node utilization | High | Low | Reduced contention |
| Availability | ~99% (single node SLA) | ~99.99% (Byzantine consensus) | 100x reduction in P(outage) |
| Adapter propagation time | Immediate | <1s (gossip rounds) | Acceptable latency |

## VII. Challenges & Mitigations

| Challenge | Mitigation |
|-----------|-----------|
| Adapter inconsistency across queries | Vector clocks + causal consistency |
| Cache thrashing (adapter spam) | TTL + popularity-based eviction |
| Gossip amplification (DDoS) | Fanout limit (3–5) + rate limiting |
| Adapter poisoning | Cryptographic signatures on adapters |
| Partition tolerance | Quorum-based consensus for critical adapters |

## VIII. Related Work

- **Gossip Protocols**: Epidemic algorithms (Demers et al., 1988)
- **Federated Learning**: FedAvg (McMahan et al., 2016)
- **LoRA**: Low-Rank Adaptation (Hu et al., 2021)
- **Vector Clocks**: Lamport clocks + causal consistency (Lamport, 1978)

## IX. Next Steps

1. **Literature Review**: Study existing gossip-based LoRA systems (e.g., FedLoRA, GossipFL)
2. **Prototype**: Implement minimal gossip protocol + routing in ThemisDB test harness
3. **Simulation**: Model convergence time, cache efficiency on synthetic topologies
4. **Integration**: Extend `src/rag/lora_manager.h` with gossip layer
5. **Evaluation**: Benchmark multi-node deployment with varying adapter popularity

---

*Placeholder for research paper draft. Pending concrete implementation evidence and simulation results.*
