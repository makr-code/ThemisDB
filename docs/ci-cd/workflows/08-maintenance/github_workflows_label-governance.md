# [Manual] Label Governance - Setup & Audit

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_github_workflows_label-governance.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Label Governance - Setup & Audit**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `action` | Action to perform | ✅ | `audit` |

## Jobs

### `label-governance`
**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout code** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run Audit Report** — `python3 .github/scripts/audit-labels.py`
- **Create Missing Labels** — `python3 .github/scripts/create-labels.py`
- **Migrate Existing Labels** — `python3 .github/scripts/migrate-labels.py`
- **Generate Report** — `echo "## 📊 Label Governance Workflow Report" >> $GITHUB_STEP_SUMMARY`

### `validate-labels`
**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `label-governance`
**Bedingung:** `success()`

**Schritte:**

- **Checkout code** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Validate Label Configuration** — `python3 .github/scripts/validate-labels.py`
- **Write job summary** — `echo "## ✅ Label Governance – Validation" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_github_workflows_label-governance.yml)
- [Alle Workflows](../README.md)


