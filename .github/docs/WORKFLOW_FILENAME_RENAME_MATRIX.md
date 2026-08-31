# Workflow Filename Rename Matrix

Status: Proposal
Date: 2026-08-25
Scope: `.github/workflows/*.yml`

## Ziel

Diese Matrix harmonisiert die aktuellen Workflow-Dateinamen mit dem in
`.github/docs/WORKFLOW_FRAMEWORK_DESIGN.md` definierten Schema:

- `<domain>-<purpose>[-<scope>].yml`
- lowercase
- separator nur `-`
- keine numerischen Prefixe
- maximal 4 Segmente nach Domain

## Konflikthinweis

Aktuell gibt es einen dokumentierten Zielkonflikt:

- `WORKFLOW_FRAMEWORK_DESIGN.md`: keine numerischen Prefixe
- `WORKFLOW_GUIDELINES.md`: historisch Prefixe (`01`, `03`, `04`, `09`) beibehalten

Diese Matrix folgt dem Zielbild aus `WORKFLOW_FRAMEWORK_DESIGN.md`.

## Rename Matrix (alt -> neu)

| Alt | Neu | Grund |
|---|---|---|
| `09-pr-gates_community-fail-closed.yml` | `gate-pr-community-failclosed.yml` | entfernt Prefix + `_`, Domain auf `gate` normalisiert |
| `09-pr-gates_edition-license-validation.yml` | `gate-pr-edition-license.yml` | entfernt Prefix + `_`, kompakter Scope |
| `09-pr-gates_hash-sbom-validation.yml` | `gate-pr-hash-sbom.yml` | entfernt Prefix + `_`, Domain konsistent |
| `09-pr-gates_private-plugin-boundary-enforcement.yml` | `gate-pr-plugin-boundary.yml` | entfernt Prefix + `_`, Segmentanzahl reduziert |
| `ci-benchmarks.yml` | `build-benchmarks.yml` | `ci`-Domain ersetzt durch erlaubtes `build` |
| `ci-content-regression.yml` | `build-content-regression.yml` | `ci` -> `build` |
| `ci-llm-inference.yml` | `build-llm-inference.yml` | `ci` -> `build` |
| `ci-pr-gates.yml` | `gate-pr-core.yml` | `ci` -> `gate` |
| `ci-release-build-matrix.yml` | `release-build-matrix.yml` | `ci` -> `release` |
| `ci-release.yml` | `release-mainline.yml` | `ci` -> `release` |
| `ci-widget.yml` | `build-widget.yml` | `ci` -> `build` |
| `codeql.yml` | `security-codeql.yml` | Domain auf `security` normalisiert |
| `copilot-ollama-router-ci.yml` | `build-ollama-router.yml` | `copilot`-Domain ersetzt durch `build` |
| `copilot-regression-guard.yml` | `gate-copilot-regression.yml` | `copilot`-Domain ersetzt durch `gate` |
| `docker-image.yml` | `release-docker-image.yml` | `docker`-Domain ersetzt durch `release` |
| `fortify.yml` | `security-fortify.yml` | Domain auf `security` normalisiert |
| `fuzzing.yml` | `security-fuzzing.yml` | Domain auf `security` normalisiert |
| `governance-gates.yml` | `compliance-governance-gates.yml` | `governance`-Domain ersetzt durch `compliance` |
| `validate-distributed-knowledge.yml` | `gate-distributed-knowledge.yml` | `validate`-Domain ersetzt durch `gate` |
| `validate-pr-version-targeting.yml` | `gate-pr-version-targeting.yml` | `validate`-Domain ersetzt durch `gate` |

## Migration-Checkliste (real verdrahtbar)

1. Datei umbenennen.
2. Referenzen in Doku aktualisieren:
   - `.github/WORKFLOW_REGISTRY.md`
   - `.github/WORKFLOW_GUIDELINES.md`
   - `.github/docs/WORKFLOW_OPTIMIZATION.md`
   - `.github/docs/WORKFLOW_FRAMEWORK_DESIGN.md`
3. `act`/`actionlint` lokal validieren.
4. Falls externe Trigger den Dateinamen verwenden (z. B. `gh workflow run`), Aufrufnamen aktualisieren.
5. PR in 2 Wellen ausrollen:
   - Welle 1: nur Rename + Referenzen
   - Welle 2: optionale inhaltliche Bereinigung

## Hinweis zur Betriebsstabilitaet

GitHub Actions nutzt intern primär Workflow-ID/Name, dennoch sollten alle
Dateiname-abhängigen Automationen (Scripts, Docs, CLI-Aufrufe, Monitoring)
mit migriert werden, um Drift zu vermeiden.
