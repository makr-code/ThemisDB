# Chimera Benchmark CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_chimera_chimera-benchmark-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Chimera Benchmark**.

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
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `chimera-benchmarks`
**Anzeigename:** Chimera Benchmarks (${{ matrix.os }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_chimera_changes == 'true'`
**Matrix:** 1 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install Python dependencies** — `pip install --upgrade pip`
- **Run chimera regression detector unit tests** — `mkdir -p benchmark_results`
- **Run CHIMERA benchmark harness** — `mkdir -p benchmark_results`
- **Detect performance regressions** — `python3 benchmarks/chimera/chimera_regression_detector.py \`
- **Upload benchmark results and reports** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🧪 Chimera Benchmark CI – Benchmarks" >> "$GITHUB_STEP_SUMMARY"`

### `update-baseline`
**Anzeigename:** Update Chimera Baseline

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `chimera-benchmarks`
**Bedingung:** `github.event_name == 'push' && github.ref == 'refs/heads/main' && needs.chimera-benchmarks.result ==`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Download benchmark results artifact** — `actions/download-artifact@v4`
- **Rebuild baseline from latest results** — `python3 - <<'PYEOF'`
- **Commit and push updated baseline** — `git config user.name  'ThemisDB CI Bot'`
- **Write job summary** — `echo "## 🧪 Chimera Benchmark CI – Update Baseline" >> "$GITHUB_STEP_SUMMARY"`

### `benchmark-gate`
**Anzeigename:** Chimera Benchmark Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `chimera-benchmarks`
**Bedingung:** `always()`

**Schritte:**

- **Check benchmark status** — `result="${{ needs.chimera-benchmarks.result }}"`
- **Write job summary** — `result="${{ needs.chimera-benchmarks.result }}"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_chimera_chimera-benchmark-ci.yml)
- [Alle Workflows](../README.md)
