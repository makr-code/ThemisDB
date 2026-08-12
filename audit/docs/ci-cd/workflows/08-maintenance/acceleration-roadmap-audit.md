# Acceleration ROADMAP Audit

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_acceleration-roadmap-audit.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Acceleration ROADMAP Audit**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `community`, `develop`) (Pfade: `src/acceleration/ROADMAP.md`, `src/acceleration/**`, `scripts/acceleration_roadmap_audit.py`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `src/acceleration/ROADMAP.md`, `src/acceleration/**`, `scripts/acceleration_roadmap_audit.py`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `roadmap-audit`
**Anzeigename:** Acceleration ROADMAP Audit

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run ROADMAP audit** — `python3 scripts/acceleration_roadmap_audit.py \`
- **Store audit exit code** — `echo "AUDIT_EXIT_CODE=${{ steps.audit.outcome == 'failure' && '1' || '0' }}" >> `
- **Upload audit reports** — `actions/upload-artifact@v4`
- **Summarize audit results** — `echo "## Acceleration ROADMAP Audit" >> "$GITHUB_STEP_SUMMARY"`
- **Fail if discrepancies found** — `echo "::error::ROADMAP audit found discrepancies. Check the uploaded artifact fo`

## Verwandte Ressourcen

- [Workflow-Datei (historisch)](/.github/workflows/08-maintenance_acceleration-roadmap-audit.yml)
- [Aktuelle Workflow-Governance](/.github/WORKFLOW_REGISTRY.md)

