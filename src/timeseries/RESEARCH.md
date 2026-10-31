---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

# Timeseries Research and Evaluation

This page is for maintainers evaluating changes to time-series algorithms. It
records the questions the implementation and its tests can answer; it is not a
claim that every workload or performance target has been independently
measured. The code and executable tests take precedence over historical
measurements in [PERFORMANCE_BASELINE.md](PERFORMANCE_BASELINE.md).

Exact model backend not verifiable from final document text alone.

## Evaluation questions

The module is built around numeric samples grouped by metric and entity. Its
method choices have separate correctness and cost questions:

| Method | Evaluation question | Evidence and limitation |
|---|---|---|
| Gorilla encoding | Does decoding reproduce timestamps and the exact binary64 value bits, including NaN and infinities? | `tests/timeseries/test_timeseries_contract_hardening_focused.cpp` names these cases. `benchmarks/timeseries/bench_timeseries_release_gates.cpp` measures codec cost; a benchmark result is not proof of storage or network throughput. |
| Buffered writes | How do buffer size, flush interval, and backpressure settings affect memory, write latency, and flush behavior? | `TSAutoBufferConfig` and `TSAutoBufferStats` expose the controls and counters in `include/timeseries/ts_auto_buffer.h`. The adaptive flush benchmark is `benchmarks/timeseries/bench_timeseries_adaptive_flush.cpp`. |
| Range reads | Are the requested interval, entity, exact tag filters, and result limit honored? | `TSStore::QueryOptions` is in `include/timeseries/tsstore.h`; test the persisted store path, not only the contract test's local mock helpers. |
| Rollups | Are tier outputs deterministic, and are refresh watermarks advanced only after successful work? | `include/timeseries/downsampling.h` defines ordered tiers and refresh; `tests/timeseries/test_continuous_agg_materialization.cpp` exercises materialization and incremental refresh. |
| Retention | Does cleanup use the configured per-metric age and make deletions observable? | `src/timeseries/retention.cpp` computes the cutoff and updates stats/audit entries. The API does not provide an undelete operation. |

## Assumptions and boundaries

- `TSStore::DataPoint` and Gorilla encode timestamps in milliseconds, as declared
  by `include/timeseries/tsstore.h` and `include/timeseries/gorilla.h`.
- Workloads with smooth values may compress differently from noisy or highly
  irregular samples. Compression ratios in the existing performance document
  are workload-specific, not correctness guarantees.
- Continuous refresh and downsampling are explicit operations. A registered
  policy alone does not establish that its refresh worker is running.
- There is no native learned model, feature-vector embedding, or vector search
  implementation in `src/timeseries`; anomaly detection is a separate
  statistical API in `include/timeseries/anomaly_detection.h`.

## Reproducible validation

Use the focused module tests for contract and implementation behavior. The
contract-hardening test file contains named cases for write ordering, inclusive
range edges, Gorilla bit-pattern round trips, and downsampling edge cases. Its
local contract helpers are not a substitute for tests against `TSStore` and
RocksDB.

The benchmark sources are registered under `benchmarks/timeseries/`. Build and
run them using the repository's configured CMake preset and target definitions;
record the compiler, preset, CPU, seed, warmup, and repetitions with any
measurement before comparing it to a baseline. Do not interpret the
2026-08-07 values in `PERFORMANCE_BASELINE.md` as a current-hardware result.

## Contract discrepancy to resolve

`include/timeseries/timeseries_api_contract.h` describes positive Unix
nanosecond timestamps and strictly increasing writes. The concrete
`TSStore::DataPoint` API uses `timestamp_ms`; `TSStore::putDataPoint()` only
checks the configured late-arrival window when it is positive, and the
watermark comparison accepts a timestamp equal to the watermark. The contract
hardening tests use local mock helpers. Reconcile this difference in the API
contract and implementation before using the frozen contract as a description
of concrete `TSStore` behavior.

## References

- [API contract](../../include/timeseries/timeseries_api_contract.h)
- [TSStore interface](../../include/timeseries/tsstore.h)
- [Gorilla interface](../../include/timeseries/gorilla.h)
- [Downsampling interface](../../include/timeseries/downsampling.h)
- [Focused tests](../../tests/timeseries/)
- [Benchmark sources](../../benchmarks/timeseries/)
