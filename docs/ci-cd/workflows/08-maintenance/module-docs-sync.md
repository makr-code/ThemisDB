# Module Docs Sync

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/08-maintenance_module-docs-sync.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Module Docs Sync**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `develop`) (6 überwachte Pfade)
- **`schedule`** — Zeitgesteuert (Cron-Schedule) (`0 3 * * *`, täglich um 03:00 UTC)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `dry_run` | Dry-run: scan and report without writing files or creating issues | — | `False` |

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `sync`
**Anzeigename:** Build · Changelog · Issues

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Run module_docs_builder** — `python3 tools/module_docs_builder.py \`
- **Update CHANGELOG.md** — `TODAY=$(date -u +%Y-%m-%d)`
- **Commit and push changes** — `git config user.name  "github-actions[bot]"`
- **Create issues for underdocumented modules** — `python3 tools/ci/module_docs_issue_reporter.py \`
  - Titel-Format: `[MODULE] <modul>` (inkl. `include_<modul>` für include-only Module)
  - Standard-Milestone: `v1.8.0` (konfigurierbar via `--module-milestone`)
  - Label-Set: `documentation`, `type:documentation`, `area:docs`, `lang:german`, `lang:english`, `priority:medium`, `priority:P2`, `milestone:current`
- **Write job summary** — `echo "## 📚 Module Docs Sync" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `write`
- `issues`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_module-docs-sync.yml)
- [Alle Workflows](../README.md)


