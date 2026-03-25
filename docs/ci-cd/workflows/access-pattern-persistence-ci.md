# IndexRecommender Access-Pattern Persistence CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/access-pattern-persistence-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **IndexRecommender Access-Pattern Persistence**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (7 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (7 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `access-pattern-persistence-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `access-pattern-persistence-unit-tests`
**Anzeigename:** IndexRecommender persistence tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 3 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run IndexRecommender unit tests** — `cd build`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 📊 IndexRecommender Access-Pattern Persistence – Unit Tests" >> "$GITHUB`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/access-pattern-persistence-ci.yml)
- [Alle Workflows](README.md)
