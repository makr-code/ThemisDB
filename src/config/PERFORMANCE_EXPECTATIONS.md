# PERFORMANCE_EXPECTATIONS - src/config

## Scope

- Module: src/config
- This file defines measurable config module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_config_path_resolver.cpp
  - benchmarks/bench_gossip_config.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CFG-1 | legacy-to-new mapping hot path remains within release baseline budget | BM_MapLegacyToNew |
| CFG-2 | unresolved/miss mapping path remains bounded | BM_MapLegacyToNew_Miss |
| CFG-3 | config update serialization compatibility path remains bounded | BM_ConfigUpdateSerialization |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CFGG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CFGG-2 | resolver path p99 <= release threshold | p99 from mapped resolver benchmark cases |
| CFGG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Current mapping is partially proxy-oriented and should be expanded with dedicated module-native benchmark coverage.

## Sourcecode Verification (Module: config/performance)

- Verified benchmark sources:
  - benchmarks/bench_config_path_resolver.cpp
  - benchmarks/bench_gossip_config.cpp
- Verified mapping surfaces:
  - resolver hit and miss mapping benchmark paths
  - config update serialization compatibility path
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.