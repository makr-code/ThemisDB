# Edition · ENTERPRISE · CI

🔄 **CI/CD**

> **Workflow-Datei:** `.github/workflows/03-editions/edition-enterprise-ci.yml`

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Edition · ENTERPRISE ·**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `enterprise`, `develop`) (8 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `build`
**Typ:** Reusable Workflow Call
**Verwendet:** `./.github/workflows/03-editions/edition-build-ci.yml`

## Berechtigungen

- `contents`: `read`

## Verwandte Ressourcen

- [Workflow-Datei](../../.github/workflows/03-editions/edition-enterprise-ci.yml)
- [Alle Workflows](../README.md)
