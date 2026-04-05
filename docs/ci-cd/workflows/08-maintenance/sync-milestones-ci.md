# Sync-Milestones CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_sync-milestones-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Sync-Milestones**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `scripts/sync-milestones-from-roadmap.py`, `tests/test_sync_milestones.py`, `src/**/ROADMAP.md`, `.github/workflows/08-maintenance_sync-milestones-ci.yml`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `scripts/sync-milestones-from-roadmap.py`, `tests/test_sync_milestones.py`, `src/**/ROADMAP.md`, `.github/workflows/08-maintenance_sync-milestones-ci.yml`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `test`
**Anzeigename:** Unit tests

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Install test dependencies** — `pip install pytest --quiet`
- **Run unit tests** — `python -m pytest tests/test_sync_milestones.py -v`
- **Write job summary** — `echo "## 🔄 Sync-Milestones CI – Unit Tests" >> "$GITHUB_STEP_SUMMARY"`

### `dry-run`
**Anzeigename:** Dry-run smoke test

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Dry-run the sync script** — `python scripts/sync-milestones-from-roadmap.py --dry-run`
- **Verify audit report was generated** — `test -f docs/issue-milestone-audit.md && echo "✅ Audit report present"`
- **Write job summary** — `echo "## 🔄 Sync-Milestones CI – Dry-Run Smoke Test" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_sync-milestones-ci.yml)
- [Alle Workflows](../README.md)


