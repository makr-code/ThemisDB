# Validate Roadmap

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_validate-roadmap.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Validate Roadmap**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `roadmap.md`, `ROADMAP.md`, `future_enhancement.md`)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `roadmap.md`, `ROADMAP.md`, `future_enhancement.md`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `validate-roadmap`
**Anzeigename:** Validate Roadmap Structure

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Validate roadmap structure** — `python .github/scripts/validate_roadmap.py`
- **Write job summary** — `echo "## 🗺️ Validate Roadmap Structure" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_validate-roadmap.yml)
- [Alle Workflows](../README.md)


