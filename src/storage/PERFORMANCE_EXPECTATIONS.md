# PERFORMANCE_EXPECTATIONS - src/storage

## Scope

- Module: src/storage
- This file defines measurable storage module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_storage_performance.cpp
  - benchmarks/bench_user_storage_mount_latency.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| STGP-1 | allocator and memory path behavior remain bounded under representative allocation patterns | BM_Allocator_System_Small, BM_Allocator_Themis_Small, BM_Allocator_System_Large, BM_Allocator_Themis_Large, BM_Allocator_Mixed, BM_Memory_RegularPages_Sequential, BM_Memory_HugePages_Sequential, BM_Memory_RegularPages_Random, BM_Memory_HugePages_Random, BM_Memory_Overhead |
| STGP-2 | storage sustained-write and WAL group-commit paths remain bounded | BM_Storage_SustainedWrite_NoSync, BM_Storage_SustainedWrite_Batched, BM_Storage_SustainedWrite_FullSync, BM_WAL_GroupCommit_Batch |
| STGP-3 | RCU read/write helper behavior remains bounded | BM_RCU_Read_SingleThread, BM_RCU_Write_WithSync, BM_RCU_Read_MultiThread |
| STGP-4 | user storage mount and mount-dispatch helper paths remain bounded | GocryptfsBenchFixture/Initialize, GocryptfsBenchFixture/CheckAvailability, GocryptfsBenchFixture/IsMounted, GocryptfsBenchFixture/MountContainer_FastFail, GocryptfsBenchFixture/UnmountContainer_FastFail, GocryptfsBenchFixture/MountUnmountCycle, BM_MountDispatch_NoKDF, GocryptfsBenchFixture/BackendMeta |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| STGG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| STGG-2 | storage hot-path p99 <= release threshold | p99 from mapped storage benchmark cases |
| STGG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional storage benchmark scenarios are introduced.

## Sourcecode Verification (Module: storage/performance)

- Verified benchmark sources:
  - benchmarks/bench_storage_performance.cpp
  - benchmarks/bench_user_storage_mount_latency.cpp
- Verified mapping surfaces:
  - allocator/memory, RCU, sustained-write/WAL, and mount-latency behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.