# LLM OpenAI-Compat Adapter CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/llm/llm-openai-compat-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **LLM OpenAI-Compat Adapter**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `llm-openai-compat`
**Anzeigename:** LLM OpenAI-Compat Adapter Tests

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_llm_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (CPU only, LLM enabled)** — `cmake -B build -G Ninja \`
- **Build OpenAI-compat adapter test** — `cmake --build build --target themisdb_tests -j$(nproc)`
- **Run OpenAI-compat adapter tests** — `cd build`
- **Write job summary** — `echo "## 🤖 LLM OpenAI-Compat Adapter CI" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/llm/llm-openai-compat-ci.yml)
- [Alle Workflows](../README.md)
