# Plugin Security CRL/OCSP CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/plugin-security-crl-ocsp-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Plugin Security CRL/OCSP**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/ci-scope-classifier.yml`

### `plugin-security-crl-ocsp-tests`
**Anzeigename:** Plugin Security CRL/OCSP (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run PluginSecurityCRLOCSPTests via ctest** — `set -o pipefail`
- **Run PluginSecurityCRLOCSP focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/plugin-security-crl-ocsp-ci.yml)
- [Alle Workflows](README.md)
