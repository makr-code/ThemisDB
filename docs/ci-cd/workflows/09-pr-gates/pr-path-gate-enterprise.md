# PR Path Gate · enterprise

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/09-pr-gates_pr-path-gate-enterprise.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **PR Path Gate · enterprise**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Branches: `enterprise`)

## Nebenläufigkeit

- **Gruppe:** `pr-path-gate-enterprise-${{ github.event.pull_request.number }}`
- **Cancel-in-progress:** Ja

## Jobs

### `path-gate`
**Anzeigename:** Enterprise-lane path policy

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Get changed files** — `set -euo pipefail`
- **Enforce Enterprise-lane path policy** — `set -euo pipefail`
- **Write job summary** — `echo "## 🔒 PR Path Gate — Enterprise release lane" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`
- `pull-requests`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/09-pr-gates_pr-path-gate-enterprise.yml)
- [Alle Workflows](../README.md)


