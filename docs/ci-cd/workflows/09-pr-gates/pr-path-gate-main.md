# PR Path Gate · main (Community)

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/09-pr-gates_pr-path-gate-main.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **PR Path Gate · main (Community)**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Branches: `main`)

## Nebenläufigkeit

- **Gruppe:** `pr-path-gate-main-${{ github.event.pull_request.number }}`
- **Cancel-in-progress:** Ja

## Jobs

### `path-gate`
**Anzeigename:** Community-only path policy

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Get changed files** — `set -euo pipefail`
- **Enforce Community-only path policy** — `set -euo pipefail`
- **Write job summary** — `echo "## 🔒 PR Path Gate — Community release lane (main)" >> "$GITHUB_STEP_SUMMAR`

## Berechtigungen

- `contents`: `read`
- `pull-requests`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/09-pr-gates_pr-path-gate-main.yml)
- [Alle Workflows](../README.md)


