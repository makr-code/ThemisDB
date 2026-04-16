# [Manual] Add Documentation Metadata

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_add-doc-metadata.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Add Documentation Metadata**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `dry_run` | Run in dry-run mode (no changes) | — | `false` |

## Jobs

### `add-metadata`
**Anzeigename:** Add Metadata to Markdown Files

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Configure Git** — `git config --local user.email "github-actions[bot]@users.noreply.github.com"`
- **Run metadata script (dry-run)** — `python3 scripts/add_doc_metadata.py --dry-run`
- **Run metadata script** — `python3 scripts/add_doc_metadata.py`
- **Check for changes** — `if git diff --quiet; then`
- **Commit and push changes** — `git add "*.md"`
- **Summary** — `if [ "${{ steps.check_changes.outputs.changes }}" = "true" ]; then`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_add-doc-metadata.yml)
- [Alle Workflows](../README.md)


