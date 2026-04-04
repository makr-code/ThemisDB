# PR Path Gate · hyperscaler

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/09-pr-gates_pr-path-gate-hyperscaler.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **PR Path Gate · hyperscaler**.

## Auslöser (Triggers)

- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (Branches: `hyperscaler`)

## Nebenläufigkeit

- **Gruppe:** `pr-path-gate-hyperscaler-${{ github.event.pull_request.number }}`
- **Cancel-in-progress:** Ja

## Jobs

### `path-gate`
**Anzeigename:** Hyperscaler-lane path policy

**Läuft auf:** `ubuntu-22.04`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Get changed files** — `set -euo pipefail`
- **Validate Helm charts (if changed)** — `set -euo pipefail`
- **Validate Kubernetes operator manifests (if changed)** — `set -euo pipefail`
- **Summarize path gate result** — `echo "✅  Hyperscaler-lane path policy passed."`
- **Write job summary** — `echo "## 🔒 PR Path Gate — Hyperscaler release lane" >> "$GITHUB_STEP_SUMMARY"`

## Berechtigungen

- `contents`: `read`
- `pull-requests`: `write`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/09-pr-gates_pr-path-gate-hyperscaler.yml)
- [Alle Workflows](../README.md)
