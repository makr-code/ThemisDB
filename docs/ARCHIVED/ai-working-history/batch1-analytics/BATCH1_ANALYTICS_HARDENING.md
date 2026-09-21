# BATCH 1: Analytics Module Hardening - Implementation Plan

## Task: Distributed Analytics Coordinator Safety Controls

### Acceptance Criteria (from ROADMAP)
- [ ] Fail-closed behavior for unsupported dependency/capability states
- [ ] Circuit breaker pattern for failed shards
- [ ] Timeout + recovery mechanism
- [ ] Bounded queue for managing concurrent requests
- [ ] Comprehensive error-path handling for degradation scenarios
- [ ] All tests PASS
- [ ] Benchmarks show gates PASS

### Implementation Steps

#### Phase 1: Add Circuit Breaker
- Track per-shard state: CLOSED (healthy), OPEN (failed), HALF_OPEN (recovering)
- Config: failure_threshold, recovery_delay_ms, recovery_attempts
- Mark shards OPEN after N consecutive failures
- Attempt recovery after delay with HALF_OPEN state
- Return to CLOSED on success or OPEN on continued failure

#### Phase 2: Add Bounded Queue
- Limit concurrent in-flight requests per shard
- Queue requests when at capacity
- Process queued requests as earlier ones complete
- Block with timeout when queue is full

#### Phase 3: Add Recovery & Backoff
- Exponential backoff for recovery attempts
- Max recovery delay cap
- Track recovery success/failure counts
- Automatic health monitor updates based on circuit state

#### Phase 4: Enhanced Diagnostics
- Track circuit breaker state transitions
- Log state changes with reasons
- Include circuit breaker state in ShardExecutionInfo
- Add circuit_state_changes counter to Statistics

#### Phase 5: Testing
- test_analytics_distributed_coordinator_safety.cpp
  - Circuit breaker state transitions
  - Bounded queue behavior
  - Recovery and backoff
  - Degradation scenarios
  - Edge cases and error paths

#### Phase 6: Benchmarks & Gates
- Verify existing gates still pass
- Add safety control scenarios to benchmarks
- No performance regression

### Files to Modify
1. include/analytics/distributed_analytics.h - Add safety control structs
2. src/analytics/distributed_analytics.cpp - Implement circuit breaker, queue, recovery
3. tests/analytics/test_analytics_distributed_coordinator_safety.cpp - NEW
4. src/analytics/ROADMAP.md - Update status

### Key Design Decisions
- Circuit breaker state is per-shard and atomic
- Recovery uses bounded exponential backoff
- Bounded queue prevents resource exhaustion
- Health monitor drives circuit breaker state recovery
- Fail-closed: OPEN shards are skipped until recovery succeeds
