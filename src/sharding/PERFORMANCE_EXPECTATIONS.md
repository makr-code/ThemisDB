# PERFORMANCE_EXPECTATIONS - src/sharding

## Scope

- Module: src/sharding
- This file defines measurable sharding module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_sharding_performance.cpp
  - benchmarks/bench_shard_routing.cpp
  - benchmarks/bench_shard_resource_manager.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| SHDP-1 | shard routing and connection distribution paths remain bounded | ShardRoutingFixture/SingleShardLookup, ShardRoutingFixture/ConsistentHashPerformance, ShardRoutingFixture/BatchRouting, BM_ConnectionPoolHitRate, BM_DistributionQuality, ShardRoutingFixture/HotShardPattern |
| SHDP-2 | scatter/gather, cross-shard transaction, and zero-downtime split paths remain bounded | ScatterGatherFixture/ScatterGatherLatency, CrossShardJoinFixture/BroadcastHashJoin, CrossShardJoinFixture/CoLocatedJoinSimulation, BM_PercolatorCommitLatency, ShardSplitDowntimeFixture/ZeroDowntimeReadAvailability |
| SHDP-3 | rebalancing, anti-entropy, replication catchup, snapshot and gossip paths remain bounded | RebalancingFixture/WriteLatencyDuringMigration, RebalancingFixture/RebalancerDecisionCycle, RebalancingFixture/AntiEntropyScanThroughput, RebalancingFixture/GpuReedSolomonThroughput, RebalancingFixture/SnapshotTransfer1GB, RebalancingFixture/SnapshotCompressionRatioZstdL3, RebalancingFixture/ReplicaCatchupThroughput, GossipOverheadFixture/TopologyPropagation100Nodes |
| SHDP-4 | shard resource manager lookup and health-score helper paths remain bounded | BM_ResourceManager_GetSnapshot, BM_ResourceManager_CanAcceptQuery, BM_ResourceManager_PeerLookup, BM_ResourceManager_GetHealthyPeers, BM_ResourceManager_CalculateHealthScore, BM_ResourceManager_SnapshotSerialization |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| SHDG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| SHDG-2 | sharding hot-path p99 <= release threshold | p99 from mapped sharding benchmark cases |
| SHDG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional sharding benchmark scenarios are introduced.

## Sourcecode Verification (Module: sharding/performance)

- Verified benchmark sources:
  - benchmarks/bench_sharding_performance.cpp
  - benchmarks/bench_shard_routing.cpp
  - benchmarks/bench_shard_resource_manager.cpp
- Verified mapping surfaces:
  - routing, cross-shard, rebalancing/repair, gossip, and resource-manager behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.