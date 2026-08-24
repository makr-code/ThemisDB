# Commit Plan: CI Guardrails Series

Stand: 2026-08-21
Ziel: Die umgesetzten Workflow-Aenderungen in nachvollziehbare, risikoarme Commits aufteilen.

## Commit 1: Least-Privilege + Concurrency Baseline

Scope:
- .github/workflows/ci-pr-gates.yml
- .github/workflows/quality-static-analysis.yml
- .github/workflows/maintenance-issues.yml
- .github/workflows/maintenance-ci-health.yml

Inhalt:
- Top-level Berechtigungen reduziert
- Job-lokale Berechtigungen explizit gesetzt
- Concurrency-Gruppen vereinheitlicht
- security-events auf relevanten Job begrenzt

Suggested message:
chore(ci): harden workflow permissions and standardize concurrency groups

Suggested commands:
- git add .github/workflows/ci-pr-gates.yml
- git add .github/workflows/quality-static-analysis.yml
- git add .github/workflows/maintenance-issues.yml
- git add .github/workflows/maintenance-ci-health.yml
- git commit -m "chore(ci): harden workflow permissions and standardize concurrency groups"

## Commit 2: Recommend-Only PR Failure Diagnosis

Scope:
- .github/workflows/maintenance-pr-failure-diagnosis.yml

Inhalt:
- Neuer Diagnose-Workflow fuer fehlgeschlagene PR-CI-Runs
- Marker-basiertes Update bestehender Kommentare
- Rein recommend-only, ohne Auto-Close und ohne Label-Mutation

Suggested message:
feat(ci): add recommend-only PR failure diagnosis workflow

Suggested commands:
- git add .github/workflows/maintenance-pr-failure-diagnosis.yml
- git commit -m "feat(ci): add recommend-only PR failure diagnosis workflow"

## Commit 3: Overlap Cleanup in Build-Issue Maintenance

Scope:
- .github/workflows/maintenance-build-issues.yml

Inhalt:
- PR-Kommentarpfad auf Docker Image CI/CD Failures begrenzt
- Verhindert Doppelkommentare mit Diagnose-Workflow
- Rechtehaertung auf Job-Ebene und Concurrency-Namensschema angepasst

Suggested message:
chore(ci): prevent duplicate PR comments in build issue maintenance

Suggested commands:
- git add .github/workflows/maintenance-build-issues.yml
- git commit -m "chore(ci): prevent duplicate PR comments in build issue maintenance"

## Commit 4: Observe-Mode Workflow Guardrails

Scope:
- .github/workflows/maintenance-workflow-guardrails-observe.yml

Inhalt:
- Neuer Observe-Mode Governance-Workflow fuer Workflow-Dateien
- Findings als Summary und Artifact
- Optionaler Enforce-Schalter bei HIGH Findings

Suggested message:
feat(ci): add observe-mode workflow guardrails maintenance job

Suggested commands:
- git add .github/workflows/maintenance-workflow-guardrails-observe.yml
- git commit -m "feat(ci): add observe-mode workflow guardrails maintenance job"

## Commit 5: Dokumentation

Scope:
- ai_working/WINGET_CICD_ADAPTIONSPLAN_THEMISDB.md
- ai_working/PR_DESCRIPTION_CI_WORKFLOW_GUARDRAILS_AND_DIAGNOSIS.md
- ai_working/COMMIT_PLAN_CI_GUARDRAILS_SERIES.md

Inhalt:
- Umsetzungsstatus im Adaptionsplan aktualisiert
- PR-Beschreibung bereitgestellt
- Commit-Plan dokumentiert

Suggested message:
chore(docs): update CI guardrails implementation plan and PR notes

Suggested commands:
- git add ai_working/WINGET_CICD_ADAPTIONSPLAN_THEMISDB.md
- git add ai_working/PR_DESCRIPTION_CI_WORKFLOW_GUARDRAILS_AND_DIAGNOSIS.md
- git add ai_working/COMMIT_PLAN_CI_GUARDRAILS_SERIES.md
- git commit -m "chore(docs): update CI guardrails implementation plan and PR notes"

## Push Hinweis

Nach den Commits:
- git push origin develop

Alternativ auf Feature-Branch:
- git checkout -b chore/ci-guardrails-series
- git push -u origin chore/ci-guardrails-series
