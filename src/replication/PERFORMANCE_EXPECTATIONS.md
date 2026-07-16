# PERFORMANCE_EXPECTATIONS - src/replication

## Scope

- Module: src/replication
- This file defines measurable replication module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_replication_throughput.cpp
  - benchmarks/bench_changefeed_throughput.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| RLP-1 | WAL and replication-manager core paths remain bounded | WalBenchFixture/Append, WalBenchFixture/ReadFrom, BM_WALEntry_Serialize, BM_WALEntry_Deserialize, BM_ReplicationManager_Initialize, BM_ReplicationManager_PromoteToLeader |
| RLP-2 | conflict-resolution paths remain bounded | BM_HLCConflictDetection, BM_CRDTMerge |
| RLP-3 | changefeed recording/polling/subscriber and lag-sensitive paths remain bounded | ChangefeedBenchmarkFixture/EventRecordingThroughput, ChangefeedBenchmarkFixture/EventPolling, ChangefeedBenchmarkFixture/ConcurrentSubscribers, BM_EventTypeMix, BM_BurstWrites, BM_ReplicationLag, BM_ReplicationLagWAN, BM_RecordEventLatency, BM_ListEventsLatency |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| RLG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| RLG-2 | replication hot-path p99 <= release threshold | p99 from mapped replication benchmark cases |
| RLG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional replication benchmark scenarios are introduced.

## Sourcecode Verification (Module: replication/performance)

- Verified benchmark sources:
  - benchmarks/bench_replication_throughput.cpp
  - benchmarks/bench_changefeed_throughput.cpp
- Verified mapping surfaces:
  - WAL/manager/promotion, conflict resolution, and changefeed/lag behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.