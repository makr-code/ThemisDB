# Acceleration Benchmark CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_acceleration_acceleration-benchmark-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Acceleration Benchmark**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `acceleration-benchmarks`
**Anzeigename:** Acceleration Benchmarks (CPU, ${{ matrix.os }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_acceleration_changes == 'true'`
**Matrix:** 1 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install system dependencies** — `sudo apt-get update -qq`
- **Install Google Benchmark** — `sudo apt-get install -y libbenchmark-dev 2>/dev/null || true`
- **Configure (acceleration benchmarks, CPU-only)** — `cmake -B build_bench -G Ninja \`
- **Build bench_cuda_vs_cpu** — `cmake --build build_bench --target bench_cuda_vs_cpu -- -j$(nproc)`
- **Run regression detector unit tests** — `mkdir -p benchmark_results`
- **Run acceleration benchmarks (JSON output)** — `mkdir -p benchmark_results`
- **Detect performance regressions** — `python3 benchmarks/performance_regression_detector.py \`
- **Upload benchmark results and reports** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## ⚡ Acceleration Benchmark CI – Benchmarks" >> "$GITHUB_STEP_SUMMARY"`

### `update-baseline`
**Anzeigename:** Update Acceleration Baseline

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `acceleration-benchmarks`
**Bedingung:** `github.event_name == 'push' && github.ref == 'refs/heads/main' && needs.acceleration-benchmarks.resu`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Download benchmark results artifact** — `actions/download-artifact@v4`
- **Rebuild baseline from latest results** — `python3 - <<'PYEOF'`
- **Commit and push updated baseline** — `git config user.name  'ThemisDB CI Bot'`
- **Write job summary** — `echo "## ⚡ Acceleration Benchmark CI – Update Baseline" >> "$GITHUB_STEP_SUMMARY`

### `benchmark-gate`
**Anzeigename:** Acceleration Benchmark Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `acceleration-benchmarks`
**Bedingung:** `always()`

**Schritte:**

- **Check benchmark status** — `result="${{ needs.acceleration-benchmarks.result }}"`
- **Write job summary** — `result="${{ needs.acceleration-benchmarks.result }}"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_acceleration_acceleration-benchmark-ci.yml)
- [Alle Workflows](../README.md)


