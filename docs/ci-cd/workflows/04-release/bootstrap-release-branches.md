# [Manual] Bootstrap Release Branches

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🖱️ **Manuell**

> **Workflow-Datei:** `.github/workflows/04-release_bootstrap-release-branches.yml`

## Aufgabe

Manuell ausgelöster Workflow für: **Bootstrap Release Branches**.

## Auslöser (Triggers)

- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Eingaben (Inputs)

| Name | Beschreibung | Pflicht | Standard |
|------|--------------|---------|----------|
| `source_branch` | Source branch to create release lanes from (default: develop) | — | `develop` |
| `dry_run` | Dry-run: check which branches would be created without creating them | — | `false` |

## Jobs

### `bootstrap`
**Anzeigename:** Create release-lane branches

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Configure git** — `git config --global user.name "github-actions[bot]"`
- **Determine source ref** — `SOURCE="${{ github.event.inputs.source_branch }}"`
- **Create enterprise branch** — `SOURCE="${{ steps.source.outputs.source }}"`
- **Create hyperscaler branch** — `SOURCE="${{ steps.source.outputs.source }}"`
- **Write job summary** — `SOURCE="${{ steps.source.outputs.source }}"`

## Berechtigungen

- `contents`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/04-release_bootstrap-release-branches.yml)
- [Alle Workflows](../README.md)

