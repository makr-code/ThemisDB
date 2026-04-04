### Context

This issue implements the roadmap item 'API Gateway Enhancements' for the server domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: API Gateway Enhancements

### Goal

Deliver the scoped changes for API Gateway Enhancements in src/server/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### API Gateway Enhancements

#### Distributed API Gateway
**Priority:** High  
**Target Version:** v1.7.0

Deploy API Gateway in distributed mode with Raft consensus.

**Features:**
- Multi-node gateway cluster
- Raft-based configuration sync
- Automatic failover
- Session affinity for WebSocket/SSE
- Distributed rate limiting

**Architecture:**
```
Client → Load Balancer → [Gateway Node 1]
                       → [Gateway Node 2] ← Raft Cluster
                       → [Gateway Node 3]
```

---

#### Smart Routing
**Priority:** Medium  
**Target Version:** v1.8.0

ML-based routing decisions for optimal performance.

**Approach:**
- Learn query patterns and latencies
- Predict which shard has cached data
- Route to least-loaded backend
- Avoid backends with high tail latency

**Expected Improvement:** 20-40% latency reduction via smart routing

---

#### Request Coalescing
**Priority:** Medium  
**Target Version:** v1.7.0

Merge duplicate in-flight requests to same resource.

**Scenario:**
```
Time 0ms:  Client A requests GET /api/v1/entities/123
Time 2ms:  Client B requests GET /api/v1/entities/123
Time 5ms:  Backend returns response
Time 5ms:  Both clients receive same response
```

**Benefits:**
- Reduce backend load
- Lower latency for duplicate requests
- Especially effective for expensive queries

---

### Acceptance Criteria

- [ ] Multi-node gateway cluster
- [ ] Raft-based configuration sync
- [ ] Automatic failover
- [ ] Session affinity for WebSocket/SSE
- [ ] Distributed rate limiting
- [ ] Learn query patterns and latencies
- [ ] Predict which shard has cached data
- [ ] Route to least-loaded backend
- [ ] Avoid backends with high tail latency
- [ ] Reduce backend load
- [ ] Lower latency for duplicate requests
- [ ] Especially effective for expensive queries

### Relationships

- Roadmap row: #102 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/server/FUTURE_ENHANCEMENTS.md#api-gateway-enhancements
- Source key: roadmap:102:server:v1.7.0:api-gateway-enhancements

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:102:server:v1.7.0:api-gateway-enhancements -->
<!-- roadmap-ref: row=102;module=server;target=v1.7.0 -->
<!-- roadmap-detail: src/server/FUTURE_ENHANCEMENTS.md#api-gateway-enhancements -->
