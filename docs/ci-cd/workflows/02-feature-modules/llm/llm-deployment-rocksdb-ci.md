# LLM Deployment Plugin RocksDB CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/llm/llm-deployment-rocksdb-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **LLM Deployment Plugin RocksDB**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `llm-deployment-rocksdb`
**Anzeigename:** LLM Deployment Plugin (RocksDB Storage)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_llm_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Install RocksDB** — `sudo apt-get update && sudo apt-get install -y librocksdb-dev`
- **Configure CMake (CPU only, LLM enabled)** — `cmake -B build -G Ninja \`
- **Build LLM deployment plugin tests** — `cmake --build build --target themisdb_tests -j$(nproc)`
- **Run LLM deployment plugin tests** — `cd build`
- **Write job summary** — `echo "## 🤖 LLM Deployment Plugin RocksDB CI" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/llm/llm-deployment-rocksdb-ci.yml)
- [Alle Workflows](../README.md)
