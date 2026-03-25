# BackendRegistry Thread-Safety CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/backend-registry-thread-safety-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **BackendRegistry Thread-Safety**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (6 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `backend-registry-thread-safety-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `backend-registry-thread-safety-tests`
**Anzeigename:** BackendRegistry thread-safety tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Configure and build (thread-safety test target)** — `./.github/actions/configure-themis`
- **Run BackendRegistry thread-safety unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🔒 BackendRegistry Thread-Safety – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/backend-registry-thread-safety-ci.yml)
- [Alle Workflows](README.md)
