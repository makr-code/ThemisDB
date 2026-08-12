# Config Audit Trail CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_config_config-audit-trail-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Config Audit Trail**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `community`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `config-audit-trail-unit-tests`
**Anzeigename:** Config audit trail tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure (config path resolver test target)** — `cmake -B build -G Ninja \`
- **Build ConfigPathResolver focused test binary** — `cmake --build build --target test_config_path_resolver_focused -- -j$(nproc)`
- **Run Config Audit Trail unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📋 Config Audit Trail – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei (historisch)](/.github/workflows/02-feature-modules_config_config-audit-trail-ci.yml)
- [Aktuelle Workflow-Governance](/.github/WORKFLOW_REGISTRY.md)

