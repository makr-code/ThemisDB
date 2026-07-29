# Chimera Integration CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules_chimera_chimera-integration-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Chimera Integration**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (11 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (11 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `simulation-unit-tests`
**Anzeigename:** Unit tests (simulation mode)

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout** — `actions/checkout@v4`
- **Install build tools** — `sudo apt-get update -qq`
- **Configure (simulation mode – no real drivers)** — `cmake -S . -B build/sim \`
- **Build chimera unit tests** — `# Build all chimera test targets; fail the job if the build fails.`
- **Write job summary** — `echo "## 🧪 Chimera Integration CI – Simulation Unit Tests" >> "$GITHUB_STEP_SUMM`

### `integration-tests`
**Anzeigename:** Integration tests (real drivers)

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout** — `actions/checkout@v4`
- **Start backend containers** — `docker compose \`
- **Install build tools and driver dependencies** — `sudo apt-get update -qq`
- **Wait for backends to be healthy** — `echo "Waiting for MongoDB..."`
- **Configure with real drivers (THEMIS_ENABLE_*)** — `cmake -S . -B build/integration \`
- **Build integration targets** — `cmake --build build/integration --parallel`
- **Run chimera integration smoke tests** — `set -euo pipefail`
- **Stop containers** — `docker compose \`
- **Upload integration test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🧪 Chimera Integration CI – Real Driver Tests" >> "$GITHUB_STEP_SUMMARY"`

### `integration-gate`
**Anzeigename:** Chimera Integration Gate

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `simulation-unit-tests`, `integration-tests`
**Bedingung:** `always()`

**Schritte:**

- **Evaluate results** — `sim="${{ needs.simulation-unit-tests.result }}"`
- **Write job summary** — `echo "## 🧪 Chimera Integration CI – Gate" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_chimera_chimera-integration-ci.yml)
- [Alle Workflows](../README.md)
