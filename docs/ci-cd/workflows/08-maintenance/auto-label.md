# Auto Label

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/08-maintenance_auto-label.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Auto Label**.

## Auslöser (Triggers)

- **`pull_request_target`** — Automatisch bei Pull Requests (Ziel-Kontext)

## Jobs

### `auto-label`
**Läuft auf:** `ubuntu-latest`

**Schritte:**

- **Label based on changed files** — `actions/labeler@v5`
- **Write job summary** — `echo "## 🏷️ Auto Label" >> "$GITHUB_STEP_SUMMARY"`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/08-maintenance_auto-label.yml)
- [Alle Workflows](../README.md)
