# Wave 3 Benchmark Suite (W3)

## Scope

W3 erweitert die Benchmark-Abdeckung für produktionsnahe, priorisierte Critical-Flows:

- **B3-A** Full-Function-Workloads über mehrere Komponenten
- **B3-B** Scale/Stress-Dimensionen (Datenvolumen, Parallelität, Request-Mix)
- **B3-C** Vergleichbare Reports + Guardrails für klare Regressionen

Implementierung:

- Runner/Reporter: `benchmarks/wave3_benchmark_suite.py`
- Workload-Profile: `benchmarks/wave3_workload_profiles.json`

## Workload-Profile

Aktive Profile:

- `read-heavy` → `critical_read_path` (`bench_ycsb`)
- `write-heavy` → `critical_write_path` (`bench_batch_insert`)
- `mixed` → `critical_mixed_path` (`bench_cross_functional_end_to_end`)

Jedes Profil definiert pro Dimension:

- `dataset_scale`: `small|medium|large`
- `benchmark_filter`: Google-Benchmark-Filter
- `parallelism`: Vergleichslabel für Lastdimension
- `request_mix`: deklarativer Mix (z. B. `read=70,write=30`)

## Metriken

Pro Workload-Dimension werden erfasst:

- `throughput_ops_per_sec`
- `p50_latency_ms`
- `p95_latency_ms`
- `p99_latency_ms`
- `resource_indicators` (u. a. Laufzeit pro Run, CPU-Anzahl, Load-Average falls vorhanden)

Latenzen werden aus Google-Benchmark-`real_time` inkl. `time_unit` in Millisekunden normalisiert.

## Ausführung

```bash
python3 benchmarks/wave3_benchmark_suite.py run \
  --benchmark-bin-dir <build>/bin \
  --profiles-file benchmarks/wave3_workload_profiles.json \
  --output benchmarks/benchmark_results/wave3_current.json
```

Optional für Runner ohne Binaries (Plan-/Schema-Validierung):

```bash
python3 benchmarks/wave3_benchmark_suite.py run \
  --benchmark-bin-dir /tmp/nonexistent \
  --profiles-file benchmarks/wave3_workload_profiles.json \
  --output /tmp/wave3_dry.json \
  --dry-run
```

## Baseline-/Vergleichslogik

Vergleich zwischen Baseline und Current:

```bash
python3 benchmarks/wave3_benchmark_suite.py compare \
  --baseline benchmarks/benchmark_results/wave3_baseline.json \
  --current benchmarks/benchmark_results/wave3_current.json \
  --output benchmarks/benchmark_results/wave3_compare.json
```

Verglichen wird pro `workload_key`:

- Throughput-Regression `%` = `(baseline - current) / baseline * 100`
- p95/p99-Regression `%` = `(current - baseline) / baseline * 100`

## Guardrails

Pro Profil sind Grenzwerte konfiguriert:

- `throughput_drop_percent`
- `p95_latency_increase_percent`
- `p99_latency_increase_percent`

Ein Vergleichseintrag ist **blocking**, wenn mindestens ein Guardrail verletzt ist.
Die Compare-CLI liefert Exit-Code `1` bei blocking Regressionen, sonst `0`.
