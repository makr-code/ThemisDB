# LoRA Certificate Store CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/05-quality/security/lora-certificate-store-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **LoRA Certificate Store**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (11 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (11 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `lora-certificate-store-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/01-core/ci-scope-classifier.yml`

### `lora-certificate-store-tests`
**Anzeigename:** LoRA Certificate Store (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run LoRACertificateStoreFocusedTests** — `set -o pipefail`
- **Run LoRA security tests via unified binary** — `set -o pipefail`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality/security/lora-certificate-store-ci.yml)
- [Alle Workflows](../README.md)
