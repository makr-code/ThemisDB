# Edition · HYPERSCALER · CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/03-editions_edition-hyperscaler-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Edition · HYPERSCALER ·**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `hyperscaler`, `develop`) (8 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `build`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/03-editions_edition-build-ci.yml`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/03-editions_edition-hyperscaler-ci.yml)
- [Alle Workflows](../README.md)
