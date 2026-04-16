# [Manual] Sync Roadmap Issues

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_sync-roadmap-issues.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Sync Roadmap Issues**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `mode` | Execution mode | ✅ | `preview` |
| `priority` | Optional priority filter (select "all" for no filter) | — | `all` |
| `module` | Optional module filter | — | `` |
| `limit` | Optional max item count | — | `` |
| `backfill` | Backfill issue refs into src/ROADMAP.md after apply | — | `False` |
| `manifest` | Manifest path for backfill mode | — | `artifacts/roadmap-issues/roadmap-issues-apply.json` |

## Jobs

### `sync-roadmap-issues`
**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Build command line** — `FLAGS=(--mode "${{ github.event.inputs.mode }}")`
- **Run roadmap issue sync** — `python scripts/sync-issues-from-roadmap.py ${{ steps.flags.outputs.flags }}`
- **Upload roadmap issue artifacts** — `actions/upload-artifact@v4`
- **Commit roadmap backfill** — `git config user.name "github-actions[bot]"`
- **Write job summary** — `echo "## Roadmap Issue Sync" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `write`
- `issues`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_sync-roadmap-issues.yml)
- [Alle Workflows](../README.md)


