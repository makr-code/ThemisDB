# ManifestDatabase File Deletion CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/02-feature-modules/storage/manifest-database-file-deletion-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **ManifestDatabase File Deletion**.

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
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `manifest-database-file-deletion-unit-tests`
**Anzeigename:** ManifestDatabase file deletion tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure** — `cmake -B build -G Ninja \`
- **Build focused test binary** — `cmake --build build --target test_manifest_database_file_deletion_focused -- -j$`
- **Run ManifestDatabase file deletion unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🗑️ ManifestDatabase File Deletion – Unit Tests" >> "$GITHUB_STEP_SUMMAR`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules/storage/manifest-database-file-deletion-ci.yml)
- [Alle Workflows](../README.md)
