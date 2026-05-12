# Benchmark Run Preparation (Windows, v1.9.0-alpha)

Date: 2026-05-12
Scope: Prepare reproducible benchmark execution and verify SLO benchmark coverage against PERFORMANCE_EXPECTATIONS.md.

## 1. Preparation Status

- Dedicated benchmark preset added in CMakeUserPresets.json:
  - configure preset: vscode-windows-bench-release
  - build preset: windows-bench-release
- Mapping audit (Check 7a) now passes after adding missing IDs:
  - Added IDs: TFG-1, TFG-2, TFG-3, TFG-4, TDM-1, TDM-2
  - Tool status: PASS

## 2. Run Commands (Windows)

From repository root:

```powershell
cmake --preset vscode-windows-bench-release
cmake --build --preset windows-bench-release --parallel 4
```

After build, benchmark binaries are expected under:

- build/windows-bench-release/bin

Run core SLO benchmarks (example sequence):

```powershell
cd build/windows-bench-release/bin
./bench_query.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_query_targeted.json --benchmark_out_format=json
./bench_vector_search.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_vector_search_targeted.json --benchmark_out_format=json
./bench_olap_performance.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_olap_targeted.json --benchmark_out_format=json
./bench_graph_traversal.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_graph_targeted.json --benchmark_out_format=json
./bench_timeseries_ingestion.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_timeseries_targeted.json --benchmark_out_format=json
./bench_timeseries_adaptive_flush.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_timeseries_adaptive_flush_targeted.json --benchmark_out_format=json
./bench_tpcc.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_tpcc_targeted.json --benchmark_out_format=json
./bench_ycsb.exe --benchmark_min_time=0.05s --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_ycsb_targeted.json --benchmark_out_format=json
```

Optional mapping gate:

```powershell
c:/Projects/ThemisDB/.venv/Scripts/python.exe tools/verify_benchmark_mapping.py
```

## 3. Coverage Check Summary (from benchmark_target_mapping.json)

Per current mapping state:

- mapped: 191
- proxy: 20
- not_measurable: 9
- gap: 0
- total: 220 entries in mapping
- coverage for PERFORMANCE_EXPECTATIONS target IDs: 197/197 mapped in file (Check 7a PASS)

Module-level risk areas (relevance quality):

- sharding: 10 proxy, 1 not_measurable (only 1 direct mapped)
- replication: 5 proxy
- transaction: 4 proxy
- voice: 3 not_measurable
- geo: 3 not_measurable
- cache: 1 proxy

Interpretation:

- All documented target IDs are now represented in the mapping.
- The largest quality risk is not missing IDs, but reliance on proxy benchmarks for some distributed modules.
- not_measurable entries are mostly hardware or environment constrained cases.

## 4. VaultHSM / PKCS#11 Benchmark Run (Windows)

If real HSM hardware is available (VaultHSM or compatible PKCS#11 endpoint), run the dedicated HSM benchmark binary with explicit provider path and PIN.

Build target:

```powershell
cmake --build --preset windows-bench-release --target bench_hsm_provider --parallel 4
```

Set runtime variables (example values):

```powershell
$env:THEMIS_TEST_HSM_LIBRARY = "C:\\path\\to\\pkcs11-provider.dll"
$env:THEMIS_TEST_HSM_PIN = "1234"
```

Run stub + real HSM suite:

```powershell
cd build/windows-bench-release/bin
./bench_hsm_provider.exe --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_hsm_provider_vaulthsm.json --benchmark_out_format=json
```

Optional focused real-HSM-only run:

```powershell
./bench_hsm_provider.exe --benchmark_filter="BM_HSM_(Sign|Verify)_Real|BM_HSM_Sign_Parallel" --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_out=bench_hsm_provider_vaulthsm_real_only.json --benchmark_out_format=json
```

Expected behavior:

- Stub benchmarks always run and provide baseline overhead.
- Real HSM benchmarks run only if THEMIS_TEST_HSM_LIBRARY resolves to a valid PKCS#11 provider.
- If provider is missing, benchmark output reports "HSM lib not found" and real cases are skipped.

## 5. Immediate Follow-up Recommendations

1. Prioritize direct benchmark implementations for sharding proxy IDs.
2. Convert replication/transaction proxy IDs to direct benchmark cases where feasible.
3. Add dedicated TensorDeduplication benchmark cases for TDM-1/TDM-2 (currently correctness-only, not performance-measured).
4. Keep benchmark runs separate from full CTest runs to avoid resource contention on Windows.
