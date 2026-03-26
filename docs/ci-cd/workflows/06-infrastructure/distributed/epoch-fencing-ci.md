# Epoch Fencing CI (Phase 4.1)

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/06-infrastructure_distributed_epoch-fencing-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Epoch Fencing (Phase 4.1)**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `epoch-fencing-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core_ci-scope-classifier.yml`

### `epoch-fencing-tests`
**Anzeigename:** Epoch Fencing Tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build epoch fencing test target** — `./.github/actions/configure-themis`
- **Run Epoch Fencing tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔐 Epoch Fencing Tests (Phase 4.1)" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/06-infrastructure_distributed_epoch-fencing-ci.yml)
- [Alle Workflows](../README.md)
