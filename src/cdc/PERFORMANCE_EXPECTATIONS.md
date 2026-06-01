# PERFORMANCE_EXPECTATIONS - src/cdc

## Scope

- Module: src/cdc
- This file defines measurable CDC module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_changefeed_throughput.cpp
  - benchmarks/bench_replication_throughput.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CDC-1 | mixed event-type and burst write paths remain within release baseline budget | BM_EventTypeMix, BM_BurstWrites |
| CDC-2 | replication lag paths remain bounded for local and WAN profiles | BM_ReplicationLag, BM_ReplicationLagWAN |
| CDC-3 | event record and list latency paths remain bounded | BM_RecordEventLatency, BM_ListEventsLatency |
| CDC-4 | WAL entry serialization/deserialization paths remain bounded | BM_WALEntry_Serialize, BM_WALEntry_Deserialize |
| CDC-5 | replication manager initialization and leadership promotion paths remain bounded | BM_ReplicationManager_Initialize, BM_ReplicationManager_PromoteToLeader |
| CDC-6 | conflict detection and merge paths remain bounded in release profile | BM_HLCConflictDetection, BM_CRDTMerge |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CG-2 | CDC hot-path p99 <= release threshold | p99 from mapped changefeed/replication benchmark cases |
| CG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: cdc/performance)

- Verified benchmark sources:
  - benchmarks/bench_changefeed_throughput.cpp
  - benchmarks/bench_replication_throughput.cpp
- Verified mapping surfaces:
  - event mix/burst, record/list latency, and replication lag paths
  - WAL serialize/deserialize and replication manager control paths
  - conflict detection and merge benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.