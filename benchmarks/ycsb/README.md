---
Author: ThemisDB Contributors
Created: 2026-01-03
Last Updated: 2026-10-29
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# YCSB Benchmarks (`benchmarks/ycsb/`)

Bereichsdokumentation für Throughput-/Latenz-Benchmarks im YCSB-ähnlichen KV-Workloadprofil.

## Scope

- CRUD-lastige Lastprofile mit Fokus auf Storage- und Zugriffspfadverhalten
- Vergleich von Latenzverteilungen und Throughput unter variierenden Workload-Mixen
- Baseline für skalierungsnahe Änderungen im Kernsystem

## Layer Mapping (Zielarchitektur)

| Layer | Relevanz | Begründung |
|---|---|---|
| ANN Frontdoor | Niedrig | Vektorsuche ist nicht primärer Messgegenstand |
| Tensor Mid-Layer | Niedrig | Tensorverarbeitung steht nicht im Zentrum |
| Graph Truth Layer | Mittel | konsistente Datenzugriffspfade bleiben Grundlage |
| LLM/LoRA Final Layer | Niedrig | YCSB adressiert primär Kern-Datenpfade |

## Installation

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
```

## Usage

- YCSB-nahe Lastprofile als Storage-/KV-Baselines verwenden
- Ergebnisse zusammen mit den zentralen Benchmark-Reports unter `benchmarks/` auswerten

### HTTP Entity measurements

The Python adapter in `themisdb_adapter.py` performs actual Entity API operations
through `httpx.Client`, using the server's canonical `collection:pk` key format.
It does not use the SDK's `model.namespace.collection` names: a live test showed
that the server rejects dots in path segments before reaching the Entity handler.
No global server validation or SDK namespace rules were weakened to bypass this.
The server must already be running
on a dedicated benchmark instance. Dataset load, readback validation and cleanup
are outside the measurement loop. A unique collection isolates each invocation;
cleanup deletes only keys for which the adapter attempted writes.

Install the HTTP client and optional local process observer. The local SDK is
not required for this CLI.

```powershell
.\.venv\Scripts\python.exe -m pip install "httpx>=0.26" "psutil>=5.9.0"
.\.venv\Scripts\python.exe benchmarks\standard_benchmarks_integration.py `
	--endpoint http://127.0.0.1:8765 --workload C `
	--record-count 1000 --operations 10000 --seed 42 `
	--output logs\ycsb_http_c.json
```

Choose a new output path for each run. The CLI rejects existing output files,
invalid arguments, endpoint URLs containing credentials and unsupported workloads.
It publishes JSON only after measurement and cleanup succeed. HTTP failures,
missing/corrupted reads and rejected writes abort the run, without synthetic
sleep-based replacements. Cleanup can also fail; inspect exceptions and the unique
collection name before retrying against an unavailable server. `--timeout` bounds
each HTTP request, not the entire experiment. Client close belongs to the caller
when using the adapter programmatically. Only use serial calls; concurrency is
not implemented by this adapter.

For optional server resources, pass `--server-pid PID` with the actual local
server PID. The CLI requires a loopback endpoint and verifies that the process
listens on the requested port before any dataset writes. It captures four
snapshots (before load, after load, after measurement, after cleanup) and reports
CPU/process-I/O deltas separately for these phases. Phase CPU includes server
background work and orchestration, not just engine execution. RSS/private memory
are endpoint samples, not phase maxima. Windows peak working set and commit are
server-lifetime high waters, cumulative across workloads sharing a server.
Missing platform counters are `null`; no children, physical disk I/O, GPU memory,
energy or disk footprint are included. Without a PID, `server_resources` is `null`.

Server-side rate limits are part of the experimental configuration. The default
100 requests/minute can reject dataset loading before measurement. HTTP 429 aborts
rather than triggering hidden retries. A dedicated loopback-only benchmark
instance can explicitly set `THEMIS_RATE_LIMIT_PER_MINUTE` before launch; record
the value in its run manifest. Do not disable or whitelist limits on a production
instance. The verified live smoke used 100000 requests/minute, not a bypass.

Supported workloads: A (50% read/50% update), B (95% read/5% update), C (read only),
F (50% read/50% read-modify-write). The seeded selection is probabilistic; the
reported operation counts are the actual observed mix, not guaranteed percentages.
D/E are rejected before writes because latest-key and ordered-scan semantics are
not implemented.

These are **YCSB-inspired measurements, not official YCSB results**:

- Uniform keys rather than the upstream Zipfian workload distribution.
- Updates replace the entire record through the Entity API; RMW reads and then
	replaces a record in two non-atomic HTTP calls.
- Latencies include HTTP client/thread dispatch, record generation and validation costs.
	The report contains request-level p50/p95/p99, throughput and operation counts.
- The dataset is warm after load/readback. No separate warmup, fixed-duration
	steady state, concurrent load, independent repeated trials or confidence
	intervals are supplied by this CLI.
- No upstream-driver compliance is inferred; JSON declares
  `external_comparison_ready: false`. Server observation is optional and separate
  from the MSVC runner's process metrics.
- The CLI does not configure authentication or server durability. Use a dedicated
	benchmark environment and do not weaken a production server's security settings.

`YCSBBenchmark.run_workload()` now requires an explicit asynchronous operation
adapter. `StandardBenchmarkRunner.run_ycsb_suite()` requires an adapter per database;
older demo/full-suite calls without adapters fail instead of producing invented
database comparisons. Unverified expected-throughput/latency and ratio fields are
`null`; YCSB reports have no industry grades. Other historical TPC/Sysbench methods
in these modules have not been hardened or certified by this change.

Focused contract tests (HTTP test transport plus real process/connection checks,
not performance evidence):

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s benchmarks/tests -p test_standard_benchmarks.py -v
```

The CLI has 20 focused tests. A separate live functional smoke on 2026-10-29
completed A/B/C/F with 100 records and 200 operations each on an isolated
Community Debug server, with successful read validation and cleanup. JSON/CSV,
CLI logs, server output and binary/source hashes are stored under
`benchmarks/benchmark_results/live_ycsb/20261029_183902/` (`live_summary.json`).
These short Debug measurements establish functionality and resource capture,
not production performance or cross-database ranking. The test server was stopped.
The external comparison requirements remain in
[`../BENCHMARK_STANDARDS.md`](../BENCHMARK_STANDARDS.md).

### Release Optimization Control

The local `vscode-windows-release` preset disables the two-handler MSVC
low-memory optimization override through
`THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY=OFF`. Default CI behavior is unchanged.
The exact cache behavior and build requirements are documented in
[`../../cmake/README.md`](../../cmake/README.md).

The earlier Release run at `release_ycsb/20261029_200312/` contains twelve
successful runs (1000 records/2000 operations, three sequential repetitions per
workload), but used the old `/Od` overrides. Its results retain their original
binary hash. After the local `/O2` rebuild, four separate 100-record/200-operation
functional smokes passed under
`optimized_release_validation/20261029_204406/` with their own hash and compiler
commands. The different protocols do not establish an optimization speedup.

The optimized Index-Stats probe returned 200 using `GET /index/stats` with a
JSON body. The query-string form returned 404 before handler dispatch; this
routing observation is retained separately and was not changed by the build fix.

## Bezug

- Benchmark-Root: [`../README.md`](../README.md)
- Bereichsplanung: [`../ROADMAP.md`](../ROADMAP.md)
- Erweiterungen: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Strategischer Layer-Kontext: [`../../FUTURE_PLAN.md`](../../FUTURE_PLAN.md)
