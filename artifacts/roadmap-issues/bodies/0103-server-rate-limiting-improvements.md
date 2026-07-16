### Context

This issue implements the roadmap item 'Rate Limiting Improvements' for the server domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.6.0.

Primary detail section: Rate Limiting Improvements

### Goal

Deliver the scoped changes for Rate Limiting Improvements in src/server/ and complete the linked detail section in a release-ready state for v1.6.0.

### Detailed Scope

### Rate Limiting Improvements

#### Distributed Rate Limiting
**Priority:** High  
**Target Version:** v1.6.0

Cluster-wide rate limiting with Redis backend.

**Current:** Per-node rate limiting (can be bypassed with multiple nodes)  
**Target:** Shared rate limit state across all gateway nodes

**Implementation:**
```cpp
RateLimiterV2::Config config;
config.backend = RateLimiterV2::Backend::REDIS;
config.redis_url = "redis://cluster:6379";
config.bucket_capacity = 1000;
config.refill_rate = 100.0;

RateLimiterV2 limiter(config);
// All nodes share same token bucket in Redis
```

---

#### Adaptive Rate Limiting
**Priority:** Medium  
**Target Version:** v1.7.0

Automatically adjust rate limits based on backend health.

**Logic:**
- Monitor backend latency and error rates
- Reduce rate limits when backends struggle
- Increase rate limits during low load
- Per-tenant adaptive limits

**Example:**
```
Normal operation: 1000 req/min
Backend p99 > 500ms: Reduce to 500 req/min
Backend errors > 5%: Reduce to 200 req/min
Backend recovered: Gradually increase back to 1000 req/min
```

---

#### Cost-Based Rate Limiting
**Priority:** Medium  
**Target Version:** v1.7.0

Rate limit by resource cost rather than request count.

**Concept:**
- Simple GET = 1 unit
- Complex query = 10 units
- Vector search = 20 units
- LLM completion = 100 units

**Benefits:**
- Fairer resource allocation
- Prevent expensive operations from monopolizing resources
- Better alignment with usage-based pricing

---

### Acceptance Criteria

- [ ] Monitor backend latency and error rates
- [ ] Reduce rate limits when backends struggle
- [ ] Increase rate limits during low load
- [ ] Per-tenant adaptive limits
- [ ] Simple GET = 1 unit
- [ ] Complex query = 10 units
- [ ] Vector search = 20 units
- [ ] LLM completion = 100 units
- [ ] Fairer resource allocation
- [ ] Prevent expensive operations from monopolizing resources
- [ ] Better alignment with usage-based pricing

### Relationships

- Roadmap row: #103 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/server/FUTURE_ENHANCEMENTS.md#rate-limiting-improvements
- Source key: roadmap:103:server:v1.6.0:rate-limiting-improvements

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:103:server:v1.6.0:rate-limiting-improvements -->
<!-- roadmap-ref: row=103;module=server;target=v1.6.0 -->
<!-- roadmap-detail: src/server/FUTURE_ENHANCEMENTS.md#rate-limiting-improvements -->
