# GitHub Workflow Framework Design (ThemisDB)

Status: Draft v1
Date: 2026-08-25
Scope: Repository workflows in `.github/workflows/`

## 1. Ziel

Dieses Dokument definiert ein sauberes, effizientes und eindeutig benennbares Workflow-Framework fuer ThemisDB.

Schwerpunkte:
- klare Zuordnung von Zweck und Verantwortlichkeit
- kontrollierter Rechenaufwand ueber eine eindeutige 0-9 Kennzahl
- standardisiertes Naming fuer Datei- und UI-Namen
- klare Trennung zwischen orchestrierenden Workflows und wiederverwendbaren Sub-Workflows

## 2. Ist-Analyse (Stand 2026-08-25)

### 2.1 Umfang
- Anzahl Workflow-Dateien: 36
- Gesamtzeilen in Workflows: 11,636
- Top-Level Jobs gesamt: 208

### 2.2 Trigger-Verteilung (Anzahl Workflows, die Trigger nutzen)
- `workflow_dispatch`: 32
- `push`: 18
- `schedule`: 17
- `pull_request`: 14
- `workflow_call`: 9
- `release`: 2
- `workflow_run`: 2
- `pull_request_target`: 1
- `pull_request_review`: 1
- `issue_comment`: 1
- `issues`: 1

### 2.3 Auffaellige Effizienz- und Governance-Punkte
- Viele Workflows sind gleichzeitig auf `push`, `pull_request` und `workflow_dispatch` aktiv.
- Mehrere Heavy-Lanes laufen auf `push` statt strikt auf `schedule` oder `workflow_dispatch`.
- Rechenaufwandssteuerung ist uneinheitlich (teilweise keine klare Trennung zwischen Gate, Build, Nightly, Maintenance).
- Benennung ist inkonsistent in Datei- und Anzeigenamen.

### 2.4 Naming-Inkonsistenzen (Beispiele)
- Mischung aus `CI - ...` und `CI — ...`
- Mischung aus `Edition - ...` und anderen Trennzeichen
- Uneinheitliche Schreibweisen wie `distributed_knowledge` im UI-Namen
- Teilweise uneinheitliche Prefix-Logik in Dateinamen

## 2.5 Migration-Status: Status-/Issue-Interface (Stand 2026-08-25)

Der Repository-Standard fuer Issue-/Status-Tracking ist jetzt die gemeinsame Composite Action
[.github/actions/status-flags-and-issues/action.yml](.github/actions/status-flags-and-issues/action.yml).

### 2.5.1 Zielzustand
- `upsert_issue`: vorhandene Tracker-Issue suchen, aktualisieren oder neu anlegen
- `set_status`, `clear_status`, `replace_status_group`: Labels als Status-Signal verwalten
- `comment_issue`: Issue-Kommentare mit Marker-basierter Idempotenz schreiben
- `close_issue`: abgeschlossene Tracker-Issues sauber schließen

### 2.5.2 Migrationsstatus der Workflow-Landschaft
- Migrated: Build, LLM, supply-chain, CI health, maintenance issues, security pentest, governance gates
- Aktiv und zentralisiert: [.github/workflows/reusable-status-flags-and-issues.yml](.github/workflows/reusable-status-flags-and-issues.yml)
- Teilweise bewusst ausserhalb der Status-Schnittstelle: PR-review, milestone assignment, semantic labeling, release flow and label sync remain workflow-specific because they are not issue lifecycle state tracking.

### 2.5.3 Standardregel
Alle GitHub-Issue-/Label-Aktionen, die einen Tracker-State modellieren, muessen via der shared interface laufen.
Workflow-spezifische Automationen, die keine Tracker-Status semantisch abbilden (z. B. Review-Request, Milestones, AI-Semantic-Labeling), duerfen weiterhin direkt im Workflow bleiben.

## 3. Zielbild Architektur

## 3.1 Workflow-Typen

1. Gate
- schnelle, deterministische PR-Schutzchecks
- Ziel: Signal in <10 Minuten
- Trigger: primar `pull_request`, optional `workflow_dispatch`

2. Build/Test
- kompilieren, testen, package sanity
- Trigger: `push` auf kanonische Branches + `workflow_dispatch`
- Heavy Varianten optional als nightly

3. Release
- Tag/schedule/dispatch gesteuerte Artefakt-Pipelines
- Trigger: `schedule`, `workflow_dispatch`, ggf. `release`/`workflow_call`

4. Security/Compliance
- scans, SBOM, policy checks
- Trigger: `schedule` + gezielte PR-Gates (nur lightweight)

5. Maintenance/Observability
- labels, dashboards, issue automation, health
- Trigger: `schedule`, `workflow_dispatch`, `workflow_run`

6. Reusable Sub-Workflows
- reine `workflow_call` Bausteine ohne eigene Produkttrigger
- von Gate/Build/Release Workflows aufgerufen

## 3.2 Empfohlene Layer
- L0 Policy/Gate: schnelle PR-Checks
- L1 Build/Test: branchbezogene Build-Qualitaet
- L2 Release: artefaktorientierte Pipelines
- L3 Security/Compliance: planbare Scans
- L4 Maintenance: Betriebs- und Governance-Hygiene

## 4. Naming-Standard (verbindlich)

## 4.1 Dateiname (Workflow-Datei)

Schema:
`<domain>-<purpose>[-<scope>].yml`

Regeln:
- nur lowercase
- separator nur `-`
- keine numerischen Prefixe im Dateinamen (nur falls historisch zwingend)
- keine Abkuerzungen ohne Glossar
- max. 4 Segmente nach Domain

Domain-Werte:
- `gate`
- `build`
- `release`
- `security`
- `compliance`
- `maintenance`
- `automation`
- `edition`
- `reusable`

Beispiele:
- `gate-pr-core.yml`
- `build-mainline.yml`
- `release-nightly.yml`
- `security-consolidated.yml`
- `reusable-cmake-build.yml`

## 4.2 Anzeigename (`name:` im Workflow)

Schema:
`<Domain>: <Purpose> [<Scope>]`

Regeln:
- genau ein Trennzeichenstil: `:`
- keine Mischformen aus `-`, `—`, `:` fuer die Primarstruktur
- Domain immer vorne, Title Case
- Scope in eckigen Klammern

Beispiele:
- `Gate: PR Core`
- `Build: Mainline [develop]`
- `Release: Nightly [03:00 UTC]`
- `Maintenance: CI Health Dashboard`

## 4.3 Job-Namen

Schema:
`<verb>-<object>[-<scope>]`

Regeln:
- lowercase + `-`
- kurz, action-orientiert
- stable IDs fuer Monitoring/Required Checks

Beispiele:
- `check-policy`
- `build-linux-release`
- `test-content-regression`
- `publish-artifacts`

## 5. Trigger- und Kostenmodell

## 5.1 Eindeutige Aufwandskennzahl (0-9)

Jeder Workflow erhaelt einen Rechenaufwand-Score `R` von 0 bis 9:
- `0` = geringster Aufwand
- `9` = sehr hoher Aufwand

Der Score wird aus drei Faktoren berechnet:
- `K` = CI-Kosten pro Run (relative Kostenklasse 0-9)
- `L` = Laufzeit (0-9)
- `N` = Runner-Nutzung (Parallelitaet, CPU/GPU/Memory-Footprint) (0-9)

Formel (gerundet):

`R = round(0.4 * K + 0.3 * L + 0.3 * N)`

Bewertungsraster:
- `R 0-2` niedrig
- `R 3-5` mittel
- `R 6-7` hoch
- `R 8-9` sehr hoch

## 5.2 Kalibrierung der drei Faktoren

`K` (CI-Kostenklasse):
- 0-2: sehr geringe Runner-Minuten, keine spezialisierten Runner
- 3-5: moderate Runner-Minuten, normale Build/Test-Lanes
- 6-7: hohe Runner-Minuten oder aufwaendige Matrix
- 8-9: sehr hohe Kosten, GPU/mehrere Plattformen/umfangreiche Artefaktkette

`L` (Laufzeitklasse):
- 0-2: bis 5 Minuten
- 3-5: >5 bis 20 Minuten
- 6-7: >20 bis 45 Minuten
- 8-9: >45 Minuten

`N` (Runner-Nutzung):
- 0-2: ein Runner, geringe Last
- 3-5: begrenzte Parallelitaet oder mehrere Jobs
- 6-7: hohe Parallelitaet, grosse Matrix oder hohe Memory-Last
- 8-9: sehr hohe Parallelitaet und/oder GPU-Lanes mit hoher Last

## 5.3 Trigger-Richtlinien nach Score

- `R 0-2`: `pull_request`, `push`, `workflow_dispatch` erlaubt
- `R 3-5`: `pull_request` nur mit engen `paths`, bevorzugt `push` + `workflow_dispatch`
- `R 6-7`: bevorzugt `push` (kanonische Branches), `schedule`, `workflow_dispatch`; kein breiter PR-Trigger
- `R 8-9`: standardmaessig nur `schedule` + `workflow_dispatch`; `push` nur mit expliziter Governance-Begruendung

## 5.4 Trigger-Richtlinien
- PR-Trigger nur mit engen `paths` und branch filter.
- Heavy Lanes standardmaessig nicht auf PR-Sync.
- Nightly/Soak/Benchmark immer `schedule` + manuell.
- Reusable Workflows nur `workflow_call`.
- `concurrency` fuer alle PR-/Push-Lanes mit `cancel-in-progress: true`.

## 5.5 Pflichtfeld im Workflow-Header (neu)

Jeder Workflow dokumentiert kuenftig im Kopfkommentar:
- `Rechenaufwand-Score: R=<0-9>`
- `K=<0-9>, L=<0-9>, N=<0-9>`
- Datum der letzten Kalibrierung

Beispiel:

`# Rechenaufwand-Score: R=7 (K=8, L=6, N=7) | last-calibrated: 2026-08-25`

## 6. Reusable Sub-Workflow Design

Ziel:
- Build- und Packaging-Logik zentralisieren
- Duplikate reduzieren
- einheitliche Inputs/Outputs

Mindestanforderungen fuer `workflow_call` Workflows:
- klar definierte Inputs mit Typen und Defaults
- dokumentierte Outputs
- feste Laufzeitgrenzen (`timeout-minutes`)
- Artefaktvertrag (Name, Pfad, retention)

Empfohlene Sub-Workflows (kanonisch):
- `reusable-cmake-build.yml`
- `reusable-release-matrix.yml`
- `reusable-docker-image.yml`
- `reusable-release-changelog.yml`

## 6.1 Einheitliche Status-Schnittstelle fuer alle Workflows (verbindlich)

Ziel:
- Alle Workflows setzen/entfernen Status-Labels ueber denselben Pfad.
- Alle Workflows erstellen/aktualisieren/schliessen Issues ueber denselben Pfad.
- Keine ad-hoc GitHub API Skripte mehr pro Workflow fuer diese Aufgaben.

Kanonische Schnittstelle:
- Reusable Workflow: `reusable-status-flags-and-issues.yml` (neu)
- Darunter Composite Action(s) in `.github/actions/` als technische Implementierung.

Pflichtregel:
- Workflows duerfen Status-Labels und Governance-/Health-Issues nur noch ueber diese Schnittstelle verwalten.

## 6.2 Contract der Status-Schnittstelle

Operationen (Input `operation`):
- `set_status`: Label(s) auf Ziel-Issue setzen
- `clear_status`: Label(s) von Ziel-Issue entfernen
- `replace_status_group`: atomarer Austausch innerhalb einer Label-Gruppe
- `upsert_issue`: bestehendes Tracker-Issue finden oder neu erstellen
- `close_issue`: bestehendes Issue mit `state_reason` schliessen
- `comment_issue`: standardisierten Kommentar anhaengen

Pflicht-Inputs:
- `operation`
- `scope` (z. B. build, test, security, governance, release)
- `status_key` (z. B. build-ok, build-failed, chronic-failure)
- `source_workflow`
- `source_run_id`
- `source_sha`

Optionale Inputs:
- `issue_number`
- `issue_title`
- `issue_body`
- `labels_to_add`
- `labels_to_remove`
- `status_group`
- `state_reason`

Standard-Outputs:
- `resolved_issue_number`
- `applied_labels`
- `removed_labels`
- `action_result` (created, updated, unchanged, closed)

## 6.3 Status-Flag Taxonomie

Label-Keys werden auf zentrale Labelnamen gemappt (Single Source of Truth: `.github/labels.yml`):
- build: `ci/build-ok`, `ci/build-failed`
- test: `ci/test-ok`, `ci/test-failed`
- pipeline health: `ci/failure`, `ci/chronic-failure`, `ci/build-error`
- governance/security: `governance/drift`, `governance/wave-gate-fail`, `security`, `security/critical`
- flow control: `status/build-pending`, `status/blocked`, `status/needs-approval`

Regel:
- Direkte Labelstrings in Workflows vermeiden; stattdessen nur `status_key` + Mapping.

## 6.4 Sicherheits- und Berechtigungsmodell

Minimale Rechte fuer die Schnittstelle:
- `issues: write`
- `contents: read`
- optional `pull-requests: write` nur wenn zwingend notwendig

Regeln:
- Keine impliziten Default-Permissions.
- Jede aufrufende Pipeline vererbt nur Minimalrechte.
- Alle Mutationen werden in Job Summary protokolliert (wer, was, warum, run id).

## 6.5 Migrationsregeln fuer bestehende Workflows

Pfad zur Umstellung:
1. Bestehende `actions/github-script` Label/Issue-Logik identifizieren.
2. Aufruf auf `reusable-status-flags-and-issues.yml` umstellen.
3. Lokale Sonderlogik durch standardisierte Inputs ersetzen.
4. Alte ad-hoc API Calls entfernen.

Akzeptanzkriterien:
- 100% der Workflows mit Label-/Issue-Mutation nutzen nur die zentrale Schnittstelle.
- Keine direkten REST-Mutationen fuer Label/Issue mehr in einzelnen Workflow-Dateien.
- Einheitliches Audit-Logging in allen aufrufenden Workflows.

## 6.6 Migrationsinventar (direkte Label/Issue-Mutationen)

Workflows mit direkter Label/Issue-REST-Logik (Ist-Stand, zu migrieren):
- `automation-community.yml`
- `ci-llm-inference.yml`
- `compliance-supply-chain.yml`
- `governance-gates.yml`
- `maintenance-build-issues.yml`
- `maintenance-issues.yml`
- `maintenance-labels.yml`
- `maintenance-pr-failure-diagnosis.yml`
- `security-pentest-quarterly.yml`

Bereits migriert auf zentrale Schnittstelle:
- `ci-build.yml`
- `maintenance-ci-health.yml`

Priorisierte Reihenfolge:
1. Build/Gate-nahe Kernworkflows
2. Maintenance-Workflows mit hoher Mutationsdichte
3. Governance/Compliance
4. Spezial- und Quartalsworkflows

## 7. Vorschlag zur Normalisierung vorhandener Namen

Diese Liste ist ein Entwurf fuer die erste Welle der Umbenennung (UI-Name, optional Dateiname):

- `CI — Build` -> `Build: Mainline [develop]`
- `CI — PR Gates` -> `Gate: PR Core`
- `CI — Benchmarks` -> `Build: Benchmarks [scheduled]`
- `CI — Release` -> `Release: Mainline`
- `CI - Release Build Matrix` -> `Reusable: Release Matrix`
- `Reusable CMake Build Pipeline` -> `Reusable: CMake Build`
- `Edition - Hyperscaler CI` -> `Edition: Hyperscaler CI`
- `Validate PR Version Targeting` -> `Gate: PR Version Targeting`
- `Validate distributed_knowledge Module Build & Tests` -> `Build: Distributed Knowledge Module`
- `Security: Quarterly Pentest Cadence` -> `Security: Quarterly Pentest`

## 8. Migrationsplan (inkrementell)

Phase 1 (niedriges Risiko)
- nur `name:` normalisieren
- keine Trigger-Aenderung
- keine Job-ID-Aenderung in Required Checks

Phase 2 (Kostenreduktion)
- Trigger-Haertung (`paths`, `branches`, `schedule`)
- Heavy Jobs von `push` auf `schedule/dispatch` verlagern

Phase 3 (Struktur)
- wiederverwendbare Sub-Workflows konsolidieren
- doppelte Logik entfernen

Phase 3a (Status-Standardisierung)
- zentrale Status-Schnittstelle (`reusable-status-flags-and-issues.yml`) einfuehren
- alle Label/Issue-Mutationen auf die Schnittstelle migrieren
- ad-hoc `github-script` Mutationen in Workflows entfernen

Phase 4 (Governance)
- Registry + Guidelines + Required Checks synchronisieren
- CI-Health Dashboard auf neue Namen umstellen

## 9. Messbare Erfolgsmetriken

- 20-35% weniger Runner-Minuten pro Woche
- PR-Feedback fuer Gate-Workflows weiterhin <10 min
- <5% false-positive Failures in Maintenance/Governance Lanes
- 100% Naming-Konformitaet fuer neue und migrierte Workflows
- 0 uneindeutige Workflow-Namen im Actions UI

## 10. Konkrete naechste Schritte

1. Baseline einfrieren
- aktuelle Workflowliste und Required Checks exportieren

2. Naming-Welle 1
- `name:`-Felder fuer Build/Gate/Release/Reusable normalisieren

3. Trigger-Welle 1
- Heavy PR-Trigger auf schedule/dispatch umstellen

4. Reusable-Haertung
- Input/Output-Vertraege in `workflow_call` Workflows standardisieren

4a. Status-Interface Rollout
- zentrale Label/Issue-Schnittstelle implementieren
- Workflows schrittweise migrieren (Build -> Release -> Maintenance -> Governance)
- nach Migration: direkte Label/Issue-REST Calls verbieten

5. Governance Sync
- `.github/WORKFLOW_GUIDELINES.md` und `.github/WORKFLOW_REGISTRY.md` nachziehen

---

Dieses Dokument ist als lebendes Design vorgesehen und soll bei jeder Workflow-Aenderung mit gepflegt werden.
