# Disabled-Stub-Policy für Benchmarks

## Zweck

Disabled-Stubs in Benchmark-Dateien dürfen nur **temporär** existieren.
Jeder Disabled-Stub muss eine Ticket-Referenz und ein Sunset-Datum (Deadline) tragen,
damit er nachverfolgbar bleibt und nicht unbemerkt verrottet.

## Regel

Jedes `BENCHMARK(BM_*_Disabled)` in einer `.cpp`-Datei unterhalb von `benchmarks/`
**muss** im Kommentar in derselben Datei folgendes enthalten:

1. **Issue-Referenz** – z. B. `Issue: #1234`
2. **Deadline** – z. B. `Deadline: v2.1.0` oder `Deadline: 2026-Q3`

Fehlt eines der beiden Felder, schlägt der CI-Check `check_disabled_stubs.py` fehl.

### Maximale Lebensdauer

Ein Disabled-Stub darf **maximal eine Release-Generation** bestehen bleiben.
Danach muss er entweder:
- durch einen echten Benchmark ersetzt werden, oder
- durch ein neues Ticket mit aktualisierter Deadline verlängert werden.

## Template

Verwende das folgende Template beim Anlegen eines neuen Disabled-Stubs:

```cpp
// Disabled: <Grund, warum der Benchmark deaktiviert wurde>
// TODO(#<issue-number>): Re-enable once <precondition> is met.
static void BM_<Name>_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}
// Disabled: <Kurzbeschreibung> | Deadline: <vX.Y.Z oder YYYY-QN> | Issue: #<number>
BENCHMARK(BM_<Name>_Disabled);
```

### Beispiel

```cpp
// Disabled: GPU vector index requires CUDA runner not available in standard CI.
// TODO(#1234): Re-enable once CUDA runner is provisioned in CI matrix.
static void BM_GpuVectorIndex_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
}
// Disabled: GPU vector index requires CUDA runner | Deadline: v2.1.0 | Issue: #1234
BENCHMARK(BM_GpuVectorIndex_Disabled);
```

## Enforcement

Der Check `tools/check_disabled_stubs.py` wird in CI ausgeführt und:

- **gibt WARN aus** für jeden Disabled-Stub ohne Deadline oder Issue-Referenz,
- **endet mit Exit-Code 1 (FAIL)**, wenn mindestens ein policy-widriger Stub gefunden wird.

Lokale Ausführung:

```bash
python3 tools/check_disabled_stubs.py
```

Mit explizitem Repo-Root:

```bash
python3 tools/check_disabled_stubs.py --repo-root /path/to/ThemisDB
```

## Verwandte Dokumente

- `PERFORMANCE_EXPECTATIONS.md` §1.4 Maßnahme 9
- `docs/ci-cd/perf_coverage_top10_audit.md` M09
- `tools/perf_coverage_top10_audit.py` `_m09_disabled_stub_policy()`
- `tools/perf_expectations_audit.py` `check_measure_9()`
