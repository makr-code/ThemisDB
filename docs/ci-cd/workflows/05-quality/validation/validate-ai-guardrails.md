# Validate AI-Guardrails

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_validate-ai-guardrails.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Validate AI-Guardrails**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `.github/COPILOT_INSTRUCTIONS.md`, `.github/copilot/**`, `.github/scripts/validate_copilot_refs.py`, `.github/workflows/05-quality_validation_validate-ai-guardrails.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `.github/COPILOT_INSTRUCTIONS.md`, `.github/copilot/**`, `.github/scripts/validate_copilot_refs.py`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `validate-copilot-instructions`
**Anzeigename:** Validate Copilot Instructions

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_doc_only_changes == 'true' || needs.ci-scope-classifier.output`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Validate file references** — `python .github/scripts/validate_copilot_refs.py`
- **Check file size** — `MAIN_FILE=".github/COPILOT_INSTRUCTIONS.md"`
- **Check for broken links** — `echo "Checking for broken internal links..."`
- **Write job summary** — `echo "## 🤖 AI Guardrails – Copilot Instructions Validation" >> "$GITHUB_STEP_SUM`

### `lint-markdown`
**Anzeigename:** Lint Markdown Files

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_doc_only_changes == 'true' || needs.ci-scope-classifier.output`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Run markdownlint** — `nosborn/github-action-markdown-cli@v3.3.0`
- **Write job summary** — `echo "## 📝 AI Guardrails – Markdown Lint" >> "$GITHUB_STEP_SUMMARY"`

### `check-structure`
**Anzeigename:** Check Module Structure

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_doc_only_changes == 'true' || needs.ci-scope-classifier.output`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify required modules exist** — `echo "Checking for required Copilot modules..."`
- **Write job summary** — `echo "## 🏗️ AI Guardrails – Module Structure Check" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_validate-ai-guardrails.yml)
- [Alle Workflows](../README.md)


