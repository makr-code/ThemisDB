# Research Documentation Validation

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

⏰ **Geplant**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_research-validation.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Zeitgesteuert ausgeführter Workflow: **Research Documentation Validation**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (6 überwachte Pfade)
- **`schedule`** — Zeitgesteuert (Cron-Schedule) (`0 9 * * MON`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `check-research-links`
**Anzeigename:** Validate Research Links

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `github.event_name == 'schedule' || github.event_name == 'workflow_dispatch' || github.event_name == `

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Check for undocumented research references** — `python3 scripts/validate_research_links.py`
- **Write job summary** — `echo "## 🔗 Research Validation – Link Check" >> "$GITHUB_STEP_SUMMARY"`

### `validate-research-metadata`
**Anzeigename:** Validate Research Metadata

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `github.event_name == 'schedule' || github.event_name == 'workflow_dispatch' || github.event_name == `

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Validate research file metadata** — `python3 scripts/validate_research_metadata.py`
- **Write job summary** — `echo "## 🏷️ Research Validation – Metadata Check" >> "$GITHUB_STEP_SUMMARY"`

### `generate-research-report`
**Anzeigename:** Generate Research Index (dry-run)

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `github.event_name == 'schedule' || github.event_name == 'workflow_dispatch' || github.event_name == `

**Schritte:**

- ***(ohne Name)*** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Dry-run index generation** — `python3 scripts/generate_research_index.py --dry-run`
- **Write job summary** — `echo "## 📑 Research Validation – Index Generation (dry-run)" >> "$GITHUB_STEP_SU`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_research-validation.yml)
- [Alle Workflows](../README.md)


