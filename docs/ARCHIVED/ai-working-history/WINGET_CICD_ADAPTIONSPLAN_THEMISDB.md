# ThemisDB CI/CD Adaptionsplan basierend auf Winget-Patterns

Stand: 2026-08-21
Zielbranch: develop
Kontext: Uebernahme bewaehrter Workflow-Prinzipien aus microsoft/winget-cli und microsoft/winget-pkgs in die ThemisDB-Workflow-Landschaft.

## Umsetzungsstatus

- PR-1 (Welle 1, Quick Wins): umgesetzt
  - Rechtehaertung (top-level reduziert, job-lokal erweitert)
  - Concurrency-Namenskonvention vereinheitlicht
  - Scope-Verlagerung fuer security-events in statischer Analyse
- PR-2 (Recommend-Only Diagnose): umgesetzt
  - Neuer Workflow fuer PR-Failure-Diagnose als Kommentar-Assistenz
  - Keine Auto-Close-/Auto-Label-/Code-Mutationspfade
- PR-2b (Ueberschneidungsbereinigung): umgesetzt
  - Doppelte PR-Kommentare mit maintenance-build-issues verhindert
- PR-3 (Observe-Mode Guardrails): umgesetzt
  - Neuer Workflow fuer workflow governance checks im Observe-Mode
  - Optionaler Enforce-Schalter fuer HIGH Findings

## 1. Zielbild

ThemisDB soll ein CI/CD-System erhalten, das:

- standardmaessig mit minimalen Berechtigungen laeuft,
- teure Jobs nur bei relevanten Aenderungen startet,
- PR/Issue-Automation als empfehlende Assistenz (nicht selbst-ausfuehrend) nutzt,
- reproduzierbar und auditierbar bleibt,
- Kosten fuer Runner-Zeit und AI-Assistenz kontrollierbar macht.

## 2. Kernmuster aus Winget und ThemisDB-Umsetzung

## 2.1 Least Privilege als Default

Winget-Muster:

- top-level permissions restriktiv,
- Write-Rechte nur in gezielten Folgejobs.

ThemisDB-Adaption:

- fuer alle Workflows ein Basisprofil erzwingen:
  - top-level nur read,
  - write nur job-lokal,
  - security-events nur dort, wo SARIF oder Security-Upload erfolgt.

Betroffene Startdateien:

- .github/workflows/ci-pr-gates.yml
- .github/workflows/quality-static-analysis.yml
- .github/workflows/maintenance-issues.yml
- .github/workflows/security-consolidated.yml
- .github/workflows/codeql.yml

## 2.2 Ereignis- und Scope-getriebene Ausfuehrung

Winget-Muster:

- path-filter und event-filter sehr strikt,
- Label-Trigger fuer Spezialautomationen.

ThemisDB-Adaption:

- teure Build-/Analysejobs nur bei relevanten Pfaden,
- triageartige Jobs nur bei spezifischen Labels,
- Workflow Dispatch mit klaren Inputs fuer manuelle Diagnosen.

## 2.3 Concurrency und Vermeidung doppelter Laeufe

Winget-Muster:

- concurrency group pro PR/Issue/Ref,
- cancel-in-progress dort, wo sinnvoll.

ThemisDB-Adaption:

- Namenskonvention vereinheitlichen:
  - ci-${workflow}-${pull_request_number_or_ref}
- fuer PR-Linien cancel-in-progress true,
- fuer scheduled Governance/Audit-Jobs cancel-in-progress false nur falls historisches Ergebnis benoetigt wird.

## 2.4 Analyse-Job getrennt von Kommentar/Write-Job

Winget-Muster:

- Analysejob read-only,
- Folgejob schreibt Kommentar nur bei echtem Befund.

ThemisDB-Adaption:

- bei Maintenance- und Triage-Workflows zweiteilige Struktur:
  - diagnose
  - report
- report-Job schreibt nur:
  - wenn Diagnose ein relevantes Signal liefert,
  - und Triggerbedingungen erfuellt sind.

## 2.5 Recommend-Only statt Auto-Aktion

Winget-Muster:

- Bot gibt Evidenz und Empfehlung,
- kein automatisches Schliessen/Mergen.

ThemisDB-Adaption:

- fuer Build-/Flake-/Policy-Triage nur:
  - Kommentar,
  - Label-Vorschlag,
  - Runbook-Link.
- keine automatische PR-Aenderung, kein Auto-Close von Issues ohne expliziten Maintainer-Trigger.

## 2.6 Reproduzierbarkeit und Supply-Chain-Haertung

Winget-Muster:

- lock-basierte Agent-Workflows,
- konsequentes Pinnen von Actions.

ThemisDB-Adaption:

- bestehendes SHA-Pinning beibehalten und auf Luecken pruefen,
- in CI-Preflight explizit sicherstellen:
  - nur zugelassene unpinned actions/* oder github/* falls policy-konform,
  - Drittanbieter nur Full-SHA,
  - kein self-mutating git push in Gate-Workflows.

## 3. Umsetzung in 3 Wellen

## Welle 1 (Quick Wins, 3-5 Tage)

Ziel: Sofortige Stabilitaet, Sicherheit, geringere Runner-Kosten.

Massnahmen:

1. Permissions-Audit + Härtung
2. Concurrency-Konvention vereinheitlichen
3. Path-Filter fuer teure Jobs nachziehen
4. Analyse/Write-Job-Trennung in Maintenance-Workflows

Dateien (mindestens):

- .github/workflows/ci-pr-gates.yml
- .github/workflows/quality-static-analysis.yml
- .github/workflows/maintenance-issues.yml
- .github/workflows/maintenance-ci-health.yml

Definition of Done:

- alle geaenderten Workflows bestehen Preflight,
- keine eskalierten Fehlberechtigungen im Security-Review,
- mittlere PR-Laufzeit fuer nicht-relevante Aenderungen sinkt messbar.

## Welle 2 (Maintainer Assist, 1-2 Wochen)

Ziel: Schnellere Bearbeitung von wiederkehrenden CI-Fehlern.

Massnahmen:

1. Recommend-Only Triage fuer typische PR-Fehler
2. Label-gebundene Diagnose-Workflows
3. Standardisiertes Kommentarformat mit Evidenzblöcken

Vorgeschlagene neue Workflows:

- .github/workflows/maintenance-pr-failure-diagnosis.yml
- .github/workflows/maintenance-flake-triage.yml

Definition of Done:

- Triage-Kommentare enthalten reproduzierbare Evidenz,
- keine automatischen state-mutations ohne Maintainer-Freigabe,
- Mean Time to First Action auf CI-Fehler sinkt.

## Welle 3 (Optionale AI-Orchestrierung, 2-4 Wochen)

Ziel: Kontrollierte AI-Assists mit Budget-, Policy- und Audit-Leitplanken.

Massnahmen:

1. Budget-Guardrails (Token/Tag)
2. Domain-Allowlist fuer ausgehende AI-Hilfsjobs
3. lock-file-basierte Governance fuer generierte Agent-Workflows

Hinweis:

- Nur umsetzen, wenn operativer Nutzen > Betriebsaufwand.
- Sonst bei klassischer GitHub-Actions-Automation bleiben.

Definition of Done:

- dokumentierte Kostenobergrenzen,
- nachvollziehbare Aktivierungsbedingungen,
- Rollback-Plan fuer Deaktivierung in < 1 Tag.

## 4. Technische Akzeptanzkriterien

- Sicherheitskriterium:
  - Kein Workflow erhaelt write, wenn read ausreichend ist.
- Effizienzkriterium:
  - Teure Jobs laufen nur bei relevanten Dateiaenderungen.
- Governancekriterium:
  - Keine Auto-Close/Auto-Merge-Mechanik in neuen Assistenz-Workflows.
- Reproduzierbarkeitskriterium:
  - Dritte Actions sind mit Full-SHA gepinnt.
- Betriebskriterium:
  - Jede neue Automation besitzt klaren Eigentuemer und Runbook-Verweis.

## 5. KPI-Set zur Erfolgsmessung

- PR-Durchlaufzeit (Median)
- Runner-Minuten pro PR
- Anteil irrelevanter Trigger (Job lief, obwohl Scope nicht betroffen)
- Mean Time to First Action bei CI-Fehlern
- Anzahl manueller Re-Runs pro Woche
- Anzahl Security/Permission Findings pro Monat

## 6. Risiko- und Rollback-Plan

Top-Risiken:

1. Zu strenge path-filter ueberspringen notwendige Pruefungen.
2. Label-Trigger koennen durch falsche Labelpraxis ins Leere laufen.
3. Zusatzlogik erhoeht Komplexitaet in Maintenance-Workflows.

Gegenmassnahmen:

- Start mit observe-mode bei neuen Triage-Jobs,
- 2 Wochen Shadow-Metriken vor hartem Gate,
- klare Abschalt-Flags via workflow_dispatch input oder env flag.

Rollback:

- Neue Assistenz-Workflows deaktivieren,
- bestehende Gate-Workflows unveraendert weiter nutzen,
- Rueckbau per kleinem Revert-PR je Welle.

## 7. PR-Schnitt fuer die Umsetzung

PR 1: Workflow-Härtung Baseline

- Permissions-Audit
- Concurrency-Konvention
- Path-Filter-Optimierung

PR 2: Recommend-Only Maintenance Assist

- Neuer Diagnoseworkflow fuer PR-Fehler
- Standardisierte Evidenz-Kommentare

PR 3: Optional AI Guardrails

- Budget-/Policy-Leitplanken
- Dokumentierte Aktivierungs- und Abschaltstrategie

## 8. Operative Checkliste fuer den Start

1. Baseline-Metriken der letzten 14 Tage exportieren.
2. PR 1 als kleinste risikoarme Aenderung umsetzen.
3. Nach Merge 1 Woche vergleichen: Laufzeit, Kosten, Fehltrigger.
4. Erst danach PR 2 aktivieren.
5. PR 3 nur bei klarem ROI entscheiden.

## 9. Entscheidungsvorlage fuer Maintainer

Empfehlung:

- Welle 1 sofort,
- Welle 2 zeitnah,
- Welle 3 optional nach KPI-Review.

Go/No-Go Kriterien:

- Go fuer Welle 2, wenn Welle 1 keine Gate-Regressions erzeugt.
- Go fuer Welle 3, wenn erwartete Zeitersparnis und Kostenkontrolle nachweisbar sind.
