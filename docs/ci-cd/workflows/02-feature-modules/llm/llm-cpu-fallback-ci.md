# LLM CPU Fallback CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **LLM CPU Fallback**.

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
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `llm-cpu-fallback`
**Anzeigename:** LLM Kernel Fusion (CPU Fallback)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_llm_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Install RocksDB** — `sudo apt-get update && sudo apt-get install -y librocksdb-dev`
- **Configure CMake (CPU only, LLM enabled)** — `cmake -B build -G Ninja \`
- **Build kernel fusion CPU fallback test** — `cmake --build build --target themisdb_tests -j$(nproc)`
- **Run kernel fusion CPU fallback tests** — `cd build`
- **Write job summary** — `echo "## 🤖 LLM CPU Fallback CI – Kernel Fusion Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml)
- [Alle Workflows](../README.md)


