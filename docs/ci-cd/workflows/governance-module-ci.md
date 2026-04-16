# Governance Module CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/governance-module-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Governance Module**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (23 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (23 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `governance-unit-tests`
**Anzeigename:** Governance unit tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run CcpaRulesFocusedTests** — `set -o pipefail`
- **Run CrossTenantPolicyInheritanceFocusedTests** — `set -o pipefail`
- **Run DataLineageFocusedTests** — `set -o pipefail`
- **Run DataMaskerFocusedTests** — `set -o pipefail`
- **Run PciDssRulesFocusedTests** — `set -o pipefail`
- **Run PolicyReviewFocusedTests** — `set -o pipefail`
- **Run PolicyTemplateFocusedTests** — `set -o pipefail`
- **Run Soc2ControlsFocusedTests** — `set -o pipefail`
- **Run ComplianceReportingFocusedTests** — `set -o pipefail`
- **Run ModelGovernanceFocusedTests** — `set -o pipefail`
- **Run ComplianceSecurityGovernanceFocusedTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🏛️ Governance Module CI — Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

### `governance-policy-engine-tests`
**Anzeigename:** Governance policy engine tests (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up C++ build environment** — `./.github/actions/setup-cpp-build`
- **Configure and build** — `./.github/actions/configure-themis`
- **Run GovernancePolicyHotReloadTests** — `set -o pipefail`
- **Run GovernanceOpaAdapterFocusedTests** — `set -o pipefail`
- **Run GovernancePolicySimulationFocusedTests** — `set -o pipefail`
- **Run GovernanceComplianceTimeWindowFocusedTests** — `set -o pipefail`
- **Run GovernanceReviewSchedulerFocusedTests** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`
- **Write job summary** — `echo "## 🏛️ Governance Module CI — Policy Engine Tests" >> "$GITHUB_STEP_SUMMARY`

## Verwandte Ressourcen

- [Workflow-Datei](../.github/workflows/governance-module-ci.yml)
- [Alle Workflows](README.md)


