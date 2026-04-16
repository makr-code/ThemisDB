# [Manual] Sync Milestones from Roadmap

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_sync-milestones.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

Manuell ausgelöster Workflow für: **Sync Milestones from Roadmap**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `dry_run` | Dry-run (show what would change, no writes) | — | `true` |
| `audit_only` | Only generate docs/issue-milestone-audit.md (no milestone writes) | — | `false` |
| `verbose` | Verbose output (one line per issue) | — | `false` |

## Jobs

### `sync`
**Anzeigename:** Sync milestones from roadmap

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Build flags** — `FLAGS=""`
- **Run sync script** — `python scripts/sync-milestones-from-roadmap.py ${{ steps.flags.outputs.flags }}`
- **Commit refreshed audit report** — `git config user.name  "github-actions[bot]"`
- **Write job summary** — `echo "## 🔄 Sync Milestones from Roadmap" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `issues`: `write`
- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_sync-milestones.yml)
- [Alle Workflows](../README.md)


