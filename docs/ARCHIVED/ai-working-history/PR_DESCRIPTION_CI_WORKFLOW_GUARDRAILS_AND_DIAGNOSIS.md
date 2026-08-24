# PR: CI Workflow Guardrails + Recommend-Only Diagnose-Assist

## Ziel

Diese Aenderung uebernimmt bewaehrte CI/CD-Prinzipien aus dem Winget-Umfeld in ThemisDB, ohne bestehende Build-/Release-Logik funktional zu brechen.

Schwerpunkte:

- Rechtehaertung nach Least-Privilege
- Konsistente Concurrency-Gruppen
- Recommend-only Diagnose bei PR-CI-Fehlern
- Observe-Mode Guardrails fuer Workflow-Governance

## Umfang

## Geaenderte Dateien

- .github/workflows/ci-pr-gates.yml
- .github/workflows/quality-static-analysis.yml
- .github/workflows/maintenance-issues.yml
- .github/workflows/maintenance-ci-health.yml
- .github/workflows/maintenance-build-issues.yml
- .github/workflows/maintenance-pr-failure-diagnosis.yml (neu)
- .github/workflows/maintenance-workflow-guardrails-observe.yml (neu)
- ai_working/WINGET_CICD_ADAPTIONSPLAN_THEMISDB.md

## Was wurde umgesetzt

1. Rechtehaertung (PR-1)
- Top-level write-Rechte in mehreren Workflows entfernt.
- Write-Rechte nur auf Job-Ebene dort gesetzt, wo sie wirklich benoetigt werden.
- security-events write in der statischen Analyse auf den SARIF-relevanten Job begrenzt.

2. Concurrency-Namensschema vereinheitlicht (PR-1)
- Einheitlichere Gruppen mit ci- Prefix.
- PR- oder Ref-basierte Gruppierung fuer bessere Kollisionsvermeidung.

3. Recommend-only PR-Fehlerdiagnose (PR-2)
- Neuer Workflow maintenance-pr-failure-diagnosis.yml.
- Trigger auf fehlgeschlagene PR-Runs von:
  - CI — Build
  - CI — PR Gates
  - Quality — Static Analysis
- Ausgabe: evidenzbasierter PR-Kommentar (create/update).
- Keine Labels, kein Auto-Close, keine Code-Mutationen.

4. Ueberschneidungsbereinigung (PR-2b)
- In maintenance-build-issues.yml wurde der PR-Kommentarpfad auf Docker Image CI/CD Failures begrenzt.
- Verhindert Doppelkommentare mit dem neuen Recommend-only Diagnose-Workflow.

5. Observe-Mode Governance Guardrails (PR-3)
- Neuer Workflow maintenance-workflow-guardrails-observe.yml.
- Prueft Workflow-Policies auf:
  - permissions Breite,
  - pull_request_target + write Kombination,
  - unpinned Third-Party Actions,
  - continue-on-error true Hinweise.
- Standard: non-blocking Report + Artifact.
- Optional: enforce_mode fuer Fail bei HIGH Findings.

## Verhalten/Impact

- Keine Aenderung an Produktionscodepfaden.
- Keine Aenderung an Build-Artefakten.
- Verbesserte Governance, geringeres Privileg-Risiko, klarere Diagnose-UX im PR-Fluss.

## Validierung

- Workflow-Dateien wurden auf Diagnose-/Syntax-Fehler geprueft.
- Neue Workflows sind technisch valide.
- Bestehende Hinweise zu optionalen Secret-Kontexten in maintenance-build-issues.yml waren bereits vorhanden und wurden nicht durch diese Aenderung eingefuehrt.

## Risikoanalyse

Niedrig bis mittel:

- Niedrig: Rechtehaertung und Concurrency-Konventionen in bestehenden Workflows.
- Mittel: Neuer Diagnose-Workflow (Kommentarverhalten), mitigiert durch recommend-only Design und Marker-basiertes Update statt Kommentar-Spam.

## Rollback

Schneller Rueckbau moeglich durch:

1. Entfernen der neuen Workflows:
- maintenance-pr-failure-diagnosis.yml
- maintenance-workflow-guardrails-observe.yml

2. Revert der vier bestehenden Workflow-Dateien auf Vorzustand:
- ci-pr-gates.yml
- quality-static-analysis.yml
- maintenance-issues.yml
- maintenance-ci-health.yml
- maintenance-build-issues.yml

## Follow-up (optional)

- Observe-Mode 1-2 Wochen laufen lassen und Findings auswerten.
- Danach ggf. enforce_mode in definierten Branches aktivieren.
- Optionalen Global-Audit ueber alle Workflow-Dateien mit priorisiertem Remediation-Backlog erstellen.
