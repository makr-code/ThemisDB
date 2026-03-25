# Content Deduplication via Perceptual Hashing CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/content-dedup-perceptual-hashing-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Content Deduplication via Perceptual Hashing**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (15 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (15 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `content-dedup-perceptual-hashing-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `content-dedup-tests`
**Anzeigename:** Content Deduplication (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run ContentDeduplicationFocusedTests via ctest** — `set -o pipefail`
- **Run tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔍 Content Deduplication via Perceptual Hashing – Unit Tests (v1.8.0)" >`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/content-dedup-perceptual-hashing-ci.yml)
- [Alle Workflows](README.md)
