# Validate Config Mapping

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/05-quality_validation_validate-config-mapping.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Validate Config Mapping**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Pfade: `src/config/config_path_resolver.cpp`, `src/config/config_path_resolver.h`, `config/**`, `scripts/validate_config_mapping.py`)
- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (Pfade: `src/config/config_path_resolver.cpp`, `src/config/config_path_resolver.h`, `config/**`, `scripts/validate_config_mapping.py`)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `validate-mapping`
**Anzeigename:** Validate Config Path Mapping Table

**Läuft auf:** `ubuntu-latest`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_config_changes == 'true'`

**Schritte:**

- **Checkout code** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Validate mapping table** — `python3 scripts/validate_config_mapping.py`
- **Check for schema file** — `if [ ! -f "config/schema/path_mapping.schema.json" ]; then`
- **Write job summary** — `echo "## ⚙️ Validate Config Mapping" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/05-quality_validation_validate-config-mapping.yml)
- [Alle Workflows](../README.md)


