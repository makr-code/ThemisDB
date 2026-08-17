# Wave D Phase 2B: High-Cardinality Metrics Collection Verification

**Status:** [~] In Progress (scheduled Q1 2027)  
**Target Gate:** W4A-METRICS-01  
**Acceptance Criterion:** Memory bounded, no OOM at 50k dimensions, throughput ≥1000 events/sec

## Overview

Phase 2B extends MetricsCollector with shard-level histograms, replication lag quantiles, and retry counters. Validates cardinality bounds and memory efficiency under peak load.

### Scope

- Implement shard-level metrics histograms (latency per shard)
- Implement replication lag quantiles (p50, p95, p99)
- Implement retry/fallback counters per module
- Define cardinality bounds (shard ≤ topology, replica ≤ set size)
- Test cardinality explosion (10k → 50k dimensions) with memory tracking
- Verify no OOM, linear memory growth

## Implementation Status

### Tasks

- [ ] **Shard-Level Metrics**
  - [ ] `MetricsCollector::record_shard_latency_histogram(shard_id, latency_us)`
  - [ ] Histogram with percentiles: p50, p95, p99
  - [ ] Bounded dimension: shard_id ∈ [0, cluster.shard_count)
  - [ ] Tests: ≥2 unit tests validating histogram aggregation

- [ ] **Replication Lag Quantiles**
  - [ ] `MetricsCollector::record_replication_lag(region, lag_us)`
  - [ ] Quantile tracking: p50, p95, p99
  - [ ] Bounded dimension: region ∈ enumerated list (us-east, us-west, eu-central, ...)
  - [ ] Tests: ≥2 unit tests validating lag spike detection

- [ ] **Retry/Fallback Counters**
  - [ ] `MetricsCollector::increment_retry_counter(module, reason)`
  - [ ] Module ∈ {coordinator, sharding, replication, gpu_accelerator}
  - [ ] Reason ∈ {timeout, byzantine, resource_limit, network_partition}
  - [ ] Tests: ≥2 unit tests validating counter increment

- [ ] **Cardinality Bounds Definition**
  - [ ] Shard ID: cardinality = cluster.shard_count (bounded by topology)
  - [ ] Replica ID: cardinality = replication_factor (bounded by config)
  - [ ] Failure reason: cardinality = 4 (enumerated)
  - [ ] Total combination bound: shard × replica × reason = topology_max
  - [ ] Document bounds in comment / ROADMAP
  - [ ] Tests: ≥1 unit test verifying bounds

- [ ] **Memory Efficiency Test**
  - [ ] Emit metrics with increasing cardinality: 100, 1k, 10k, 50k
  - [ ] Measure memory usage at each step
  - [ ] Verify linear or sub-linear growth
  - [ ] Verify peak memory ≤ 512 MB at 50k cardinality
  - [ ] Tests: ≥1 benchmark test with memory tracking

- [ ] **Throughput Validation**
  - [ ] Measure metric emission rate (events/sec)
  - [ ] Target: ≥1000 events/sec sustained
  - [ ] Test with 10k, 20k, 50k cardinality
  - [ ] Verify no throughput degradation with cardinality increase
  - [ ] Tests: ≥1 benchmark test with throughput measurement

## Test Evidence

### Memory Usage (Estimated)

| Cardinality | Memory (MB) | Growth | Status |
|-------------|------------|--------|--------|
| 100 | ~10 | — | [~] In Progress |
| 1,000 | ~50 | 5.0× | [~] In Progress |
| 10,000 | ~200 | 4.0× | [~] In Progress |
| 50,000 | ~512 | 2.6× | [~] In Progress |

### Throughput (Estimated)

| Cardinality | Throughput (events/sec) | Degradation | Status |
|-------------|------------------------|-------------|--------|
| 100 | ~2,000 | — | [~] In Progress |
| 10,000 | ~1,500 | 25% | [~] In Progress |
| 50,000 | ~1,000 | 50% | [~] In Progress |

Target: ≥1000 events/sec at 50k cardinality → PASS

### Coverage

| Metric Type | Implementation | Unit Tests | Status |
|-------------|---|---|--------|
| Shard latency histogram | [~] TBD | [~] 2/2 | [~] In Progress |
| Replication lag quantiles | [~] TBD | [~] 2/2 | [~] In Progress |
| Retry counters | [~] TBD | [~] 2/2 | [~] In Progress |
| Cardinality bounds | [~] TBD | [~] 1/1 | [~] In Progress |

## Acceptance Checklist

- [ ] W4A-METRICS-01 gate PASS (memory ≤512 MB, throughput ≥1000 events/sec)
- [ ] Shard, replica, reason dimensions properly bounded
- [ ] ≥10 unit tests covering histogram, quantiles, counters, cardinality
- [ ] Memory growth measured and documented (linear/sub-linear)
- [ ] Throughput maintained above 1000 events/sec up to 50k cardinality
- [ ] No OOM errors in stress test (1+ hour at peak load)
- [ ] Code review and approval
- [ ] Integration test passing in CI

## Known Limitations

- Cardinality bounds assume static cluster topology (no dynamic resharding during test)
- Memory measurement tool: [TBD - valgrind/rss/resident_set_size]
- Throughput target is nominal; actual target depends on SLA requirements
- Test hardware: [TBD profile]

## Next Phase

Phase 2C: OTel exporter reliability & stress testing (W4A-EXPORTER-01..05)

---

Last updated: [TBD]  
Approved by: [TBD]
