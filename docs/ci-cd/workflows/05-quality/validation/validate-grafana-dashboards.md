# Validate Grafana Dashboards

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_validate-grafana-dashboards.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Validate Grafana Dashboards**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `grafana/**/*.json`, `scripts/validate_grafana_dashboards.py`, `.github/workflows/05-quality_validation_validate-grafana-dashboards.yml`)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `grafana/**/*.json`, `scripts/validate_grafana_dashboards.py`, `.github/workflows/05-quality_validation_validate-grafana-dashboards.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `validate-dashboards`
**Anzeigename:** Validate Grafana Dashboard JSON

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_grafana_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python 3.11** — `actions/setup-python@v5`
- **Run Grafana dashboard validation** — `python3 scripts/validate_grafana_dashboards.py grafana/`
- **Report validation result** — `echo "✅ All Grafana dashboard JSON files passed static validation."`
- **Write job summary** — `echo "## 📊 Validate Grafana Dashboards" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_validate-grafana-dashboards.yml)
- [Alle Workflows](../README.md)


