# Primary-Docs Index Generator

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/docs_primary-docs-index.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Primary-Docs Index Generator**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`, `release/**`) (5 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (5 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `include_root` | Also scan the repository root (non-recursive) | — | `False` |
| `output_format` | Output format | — | `json` |

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `generate-index`
**Anzeigename:** Generate Primary-Docs Index

**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Set up Python** — `actions/setup-python@v5`
- **Build extra flags** — `FLAGS=""`
- **Run primary_docs_indexer** — `python3 tools/primary_docs_indexer.py \`
- **Show index summary** — `echo "### Primary-Docs Index Summary" >> "$GITHUB_STEP_SUMMARY"`
- **Upload index as artifact** — `actions/upload-artifact@v4`
- **Commit updated index (main branch only)** — `git config user.name  "github-actions[bot]"`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/docs_primary-docs-index.yml)
- [Alle Workflows](../README.md)


