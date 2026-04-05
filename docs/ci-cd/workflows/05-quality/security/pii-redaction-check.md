# PII Redaction Policy Check

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_security_pii-redaction-check.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **PII Redaction Policy Check**.

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
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `pii-leakage-lint`
**Anzeigename:** PII Leakage Static Lint

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run PII leakage check** — `python .github/scripts/check_pii_leakage.py --root .`
- **Write job summary** — `echo "## 🔒 PII Redaction – Leakage Static Lint" >> "$GITHUB_STEP_SUMMARY"`

### `pii-pattern-config-validation`
**Anzeigename:** PII Pattern Config Validation

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install PyYAML** — `pip install pyyaml`
- **Validate pii_patterns.yaml schema** — `python - <<'EOF'`
- **Write job summary** — `echo "## 🔒 PII Redaction – Pattern Config Validation" >> "$GITHUB_STEP_SUMMARY"`

### `pii-docs-check`
**Anzeigename:** PII Documentation Presence Check

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Check that PII Redaction Policy docs exist** — `DOC="docs/en/security/PII_REDACTION_POLICY.md"`
- **Check docs contain required sections** — `DOC="docs/en/security/PII_REDACTION_POLICY.md"`
- **Write job summary** — `echo "## 🔒 PII Redaction – Documentation Check" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_security_pii-redaction-check.yml)
- [Alle Workflows](../README.md)


