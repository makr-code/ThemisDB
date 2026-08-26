# Workflow Guidelines

## Scope
Diese Richtlinie gilt fuer den schlanken, release-zentrierten Workflow-Kern.
Die kanonische Liste aktiver Workflows steht in `.github/WORKFLOW_REGISTRY.md`.
Workflows unter `.github/no_workflows/` gelten als bewusst deaktivierte Quarantaene und
duerfen nicht stillschweigend reaktiviert werden.

## Aktive Workflows (40)
Die aktuelle kanonische Liste steht in `.github/WORKFLOW_REGISTRY.md`; der alte 21er-Stand war veraltet und wird hier durch den aktuellen, im Repository geltenden Zustand ersetzt.

Kernliste der aktiven Workflows:
- `.github/workflows/gate-pr-community-failclosed.yml`
- `.github/workflows/gate-pr-edition-license.yml`
- `.github/workflows/gate-pr-hash-sbom.yml`
- `.github/workflows/gate-pr-plugin-boundary.yml`
- `.github/workflows/automation-community.yml`
- `.github/workflows/build-benchmarks.yml`
- `.github/workflows/build-mainline.yml`
- `.github/workflows/build-content-regression.yml`
- `.github/workflows/build-llm-inference.yml`
- `.github/workflows/gate-pr-core.yml`
- `.github/workflows/gate-pr-doxygen-governance.yml`
- `.github/workflows/release-build-matrix.yml`
- `.github/workflows/release-mainline.yml`
- `.github/workflows/build-widget.yml`
- `.github/workflows/reusable-cmake-build.yml`
- `.github/workflows/security-codeql.yml`
- `.github/workflows/compliance-supply-chain.yml`
- `.github/workflows/build-ollama-router.yml`
- `.github/workflows/gate-copilot-regression.yml`
- `.github/workflows/release-docker-image.yml`
- `.github/workflows/edition-hyperscaler-ci.yml`
- `.github/workflows/security-fortify.yml`
- `.github/workflows/security-fuzzing.yml`
- `.github/workflows/compliance-governance-gates.yml`
- `.github/workflows/maintenance-ai-working.yml`
- `.github/workflows/maintenance-build-issues.yml`
- `.github/workflows/maintenance-ci-health.yml`
- `.github/workflows/maintenance-docs.yml`
- `.github/workflows/maintenance-issues.yml`
- `.github/workflows/maintenance-issue-recommendations.yml`
  — Recommend-only Issue Triage: kommentiert offene Issues mit merged-PR-Evidenz und schliesst nie automatisch
- `.github/workflows/maintenance-labels.yml`
- `.github/workflows/maintenance-milestones.yml`
- `.github/workflows/maintenance-pr-failure-diagnosis.yml`
- `.github/workflows/maintenance-workflow-guardrails-observe.yml`
- `.github/workflows/release-changelog.yml`
- `.github/workflows/reusable-status-flags-and-issues.yml`
- `.github/workflows/security-consolidated.yml`
- `.github/workflows/security-pentest-quarterly.yml`
- `.github/workflows/gate-distributed-knowledge.yml`
- `.github/workflows/gate-pr-version-targeting.yml`

Archiviert in `.github/no_workflows/` (im Zuge Workflow Framework Refactoring):
  `security.yml`, `security-scanning.yml`, `security-scan.yml`,
  `maintenance-gs3-gaps.yml`, `maintenance-security-alerts.yml`

## Harte Grenzen fuer neue oder reaktivierte CI
- Default ist `kein neuer Workflow`. Bevorzuge einen neuen Job in einem bestehenden Workflow.
- Alles unter `.github/no_workflows/` bleibt deaktiviert, bis eine explizite Reaktivierungsentscheidung dokumentiert ist.
- Jeder reaktivierte Workflow braucht einen klar benannten Owner, ein Ablaufdatum fuer die naechste Review und einen Abschaltplan.
- Pull-Request-Trigger sind nur zulaessig, wenn `branches:` und `paths:` beide eng begrenzt sind.
- `paths:` duerfen nur datei- oder modulspezifische Bereiche enthalten. Globale Trigger wie `src/**`, `include/**`, `**/*.md` oder Repo-weit wirksame Sammelmuster sind fuer neue PR-Workflows nicht zulaessig.
- `push:` auf `develop` oder `community` ist nur fuer Release-, Packaging- oder explizit nicht-blockierende Nachtlaeufe zulaessig.
- Schwere Jobs muessen `workflow_dispatch` oder `schedule` bevorzugen. Sie duerfen nicht bei jedem PR-Sync anlaufen.
- Jeder PR-Workflow braucht `concurrency` mit workflow/ref-Gruppierung und `cancel-in-progress: true`.
- Jeder Workflow muss minimale `permissions` setzen und darf keine impliziten Default-Rechte nutzen.
- Wenn Triggergrenzen nicht knapp und messbar formulierbar sind, bleibt der Workflow in `.github/no_workflows/`.
- Reaktivierungen oder neue Workflow-Dateien muessen den `Workflow Boundary Guard` passieren, bevor sie aktiv bleiben duerfen.

## Reaktivierungs-Checkliste
- Der fachliche Nutzen ist branch-gate-relevant, release-relevant oder compliance-pflichtig.
- Die Logik passt nicht sinnvoll als Job in einen bestehenden aktiven Workflow.
- Trigger sind auf konkrete Dateien, ein einzelnes Modul oder einen klaren Release-Pfad begrenzt.
- Der Workflow enthaelt eine Kostenbremse: `paths`, `branches`, `concurrency`, kurze `timeout-minutes` und moeglichst fruehe Exit-Bedingungen.
- Der Workflow wurde lokal mit `scripts/test-github-actions-local.ps1` geprueft.
- Die Reaktivierung ist in `.github/WORKFLOW_REGISTRY.md` dokumentiert.
- Fuer den ersten Rollout ist der Workflow entweder `workflow_dispatch`-only oder nicht-blockierend (`continue-on-error` bzw. kein Required Check), bis die Triggerqualitaet verifiziert ist.

## Verbotene Muster
- Ein Spezialworkflow, der denselben Dateibaum wie ein bestehender Workflow ueberwacht.
- Ein Benchmark-, Audit- oder Nightly-Workflow als Required PR Check.
- Trigger auf Dokumentationsaenderungen fuer Build-, Security- oder Performance-Jobs.
- Neue Schatten-CI in Form von fast identischen Kopien bestehender Build- oder Testlogik.
- Reaktivierung aus `.github/no_workflows/`, ohne die Ursache fuer die frueheren Uebertrigger zu dokumentieren.

## Naming Conventions
- Behalte das kanonische, prefixfreie `<domain>-<purpose>[-<scope>].yml`-Schema aus `WORKFLOW_FRAMEWORK_DESIGN.md` bei.
- Dateinamen muessen den Zweck klar beschreiben, lane-neutral bleiben und zu den registrierten Domain-Werten passen.
- Neue oder reaktivierte Workflows bekommen nur nach Registry- und Guidelines-Update einen Dateinamen.

## Best Practices
- Trigger nur fuer reale Gates/Release-Lanes definieren (keine Schatten-CI).
- `paths:` und `branches:` eng schneiden; im Zweifel enger statt "vorsichtshalber breit".
- `concurrency` mit `cancel-in-progress` auf Push/PR-Workflows setzen.
- Berechtigungen minimal halten (`permissions` least privilege).
- Schwere Benchmark-, GPU- und Sweep-Jobs standardmaessig ueber `schedule` oder `workflow_dispatch` isolieren.

## Security Guidelines
- Keine Secrets im YAML oder in Shell-Skripten hardcoden.
- Publish-Workflows nur ueber Tag- oder Environment-Gates freigeben.
- Third-party Actions auf immutable Commit-SHAs pinnen (SHA-only, kein `@vX.Y.Z` Tag als einzige Referenz).
  Beispiel: `uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683  # v4.2.2`
  Enforcement: `gate-pr-core.yml` Preflight-Checks + lokales `actionlint` via `scripts/test-github-actions-local.ps1`.
- Compliance-Gates fuer Dependencies muessen branch- und pfadbegrenzt sein und ein downloadbares Audit-Artefakt erzeugen.
- OIDC-basierte Authentifizierung (kein long-lived PAT) fuer ghcr.io und neue Registry-Ziele.

## Composite Actions
- `.github/actions/setup-cpp-build/`  — C++ Build-Setup (sccache, toolchain, deps)
- `.github/actions/setup-python-script/`  — Reusable checkout→python→script→upload pattern (ersetzt 21× Duplizierung)
- `.github/actions/status-flags-and-issues/`  — Kanonische Schnittstelle für Issue-/PR-Kommentare, Labels und Status-Tracker; ersetzt direkte `github.rest.issues.*`/`createComment`-Blöcke in Workflows.
- `.github/actions/manage-governance-issue/`  — Kanonische create/update/close Issue-Action (aktuell noch nicht von Workflows verwendet; kanon. Referenz für künftige Migrations)
- `build-mainline.yml` und `maintenance-cache-warming.yml` nutzen `mozilla-actions/sccache-action` mit `SCCACHE_GHA_ENABLED=true`.
- CMake muss mit `-DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache` konfiguriert werden.
- Cache-Warming erfolgt wöchentlich (montags 00:00 UTC) für Linux (GCC) und Windows (MSVC).
- Erwartete Wiedertreffer-Rate: 60–80% bei unveränderter Toolchain + vcpkg-Baseline.

## Issue-/PR-Kommentar-Standard
- GitHub-Workflows dürfen Fehler, Statusänderungen und Recovery-Hinweise nicht direkt per ad-hoc `github.rest.issues.createComment` oder PR-Kommentar-Blocks schreiben, wenn dies als trackerbasierte Status- oder Fehlerkommunikation modelliert ist.
- Die gemeinsame Action `.github/actions/status-flags-and-issues` ist die Standard-Implementierung für:
  - `upsert_issue`
  - `set_status` / `clear_status` / `replace_status_group`
  - `comment_issue`
  - `close_issue`
- `comment_issue` darf auf Issues und PRs kommentieren; in GitHub ist ein PR als Issue mit `issue_number` adressierbar. Für klaren Zweck sollte der Workflow den `target-type` explizit auf `issue`, `pr` oder `auto` setzen.
- Empfehlungskommentare mit dynamischer Evidenz (z. B. `merged PR`/`timeline`-Analyse) sind eine erlaubte Sonderform, sofern sie comment-only bleiben und keine Tracker-Status-Mutation, kein `close_issue`, kein Label-Override und keine Branch-Mutation ausführen. In solchen Workflows muss der Marker-Mechanismus strikt idempotent bleiben.
- Kommentare müssen idempotent sein: Marker wie `<!-- ci-build:type:build-status -->` oder `<!-- ci-error:... -->` verhindern Spam und doppelte Fehlerposts.
- Fehler-Kommentare sollten immer mindestens enthalten: Workflow-Name, Run-Link, Commit/SHA, betroffenen Job und zentrale Fehlermeldung oder Testname.
- Für `pull_request`-Workflows ist der Kommentar auf den PR zu senden; für `push`/`schedule`-Workflows wird typischerweise der Tracker-Issue-Kommentar verwendet.

## CI Health Dashboard
- `maintenance-ci-health.yml` aggregiert wöchentlich (sonntags 06:00 UTC) pass/fail-Raten je Workflow.
- Schwellwert für chronische Fehler: >30% Fehlerrate bei ≥3 Fehlern → öffnet `ci/chronic-failure`-Issue.
- Lookback-Fenster konfigurierbar via `workflow_dispatch` Input `lookback_days` (Standard: 7).

## Manually Triggering Workflows
Empfohlen via GitHub CLI:

```bash
gh workflow run .github/workflows/edition-hyperscaler-ci.yml --repo makr-code/ThemisDB --ref hyperscaler
gh workflow run "release-mainline.yml" --repo makr-code/ThemisDB --ref develop --field edition=community --field build_matrix=community-only
```

## Lokales Testsystem fuer GitHub Actions

Der Repository-Standard fuer lokale Validierung ist das Script
`scripts/test-github-actions-local.ps1`. Es kapselt die drei wesentlichen
Pruefschritte fuer GitHub Actions im lokalen Repo-Kontext:

1. `actionlint`-Linting fuer YAML-/Workflow-Syntax
2. `act`-Dry-Run fuer ausgewählte Events wie `push`, `pull_request`, `workflow_dispatch`, `schedule`
3. Protokollierung in einem dedizierten Log-Ordner mit nachvollziehbaren Artefakten

### Verwendete Tools

- `docker` fuer den `rhysd/actionlint:latest` Container
- `act` fuer lokale Dry-Run-Ausfuehrung ohne echten GitHub Runner
- PowerShell (`pwsh`) als Wrapper-Shell fuer das Repo-Skript

### Skript-Interface

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

Optionale Parameter:

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun -Events push,pull_request,workflow_dispatch
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint -Workflow .github/workflows/build-mainline.yml
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all -LogDir tmp
```

### `lint`-Modus

Der `lint`-Modus startet `actionlint` als Docker-Container:

```powershell
docker run --rm -v "${PWD}:/repo" -w /repo rhysd/actionlint:latest -color
```

Ziel:
- syntaktische Workflow-Pruefung
- Validierung von `on:`, Jobs, `uses`, `runs`, Outputs, Expressions und generischer GitHub-Action-Syntax
- fruehzeitige Erkennung von YAML-/Semantikfehlern vor dem Push oder PR

Typische Fehlerquellen:
- ungueltinge YAML-Struktur
- falsche Ausdruckssyntax in `${{ ... }}`
- ungueltige `uses:`-Referenzen
- falsche Job-/Step-Definitionen
- falsche Trigger oder Secrets-Referenzen

### `dryrun`-Modus

Der `dryrun`-Modus verwendet `act` mit `-n`, also ohne echte Ausfuehrung im GitHub-Runner:

```powershell
act -n push
act -n pull_request
act -n workflow_dispatch
act -n schedule
```

Das Script ruft `act` fuer die Standard-Events auf:

```powershell
@('push', 'pull_request', 'workflow_dispatch', 'schedule')
```

Ziel:
- Job-/Step-Struktur lokal plausibilisieren
- pruefen, ob die Workflows als Runner-Graph und Job-Sequenz interpretierbar sind
- typische YAML-/Workflow-Fehler frueh erkennen, bevor ein PR gemerged wird

Wichtig:
- `act` ist ein Dry-Run, kein echter GitHub-Runner-Lauf.
- Wenn `act` bei einem Event keine passenden Stages findet, behandelt das Skript das als Hinweis-/Skip statt als fatale Fehlermeldung.
- Reale Runtime-Problems (Secrets, hosted runner, `workflow_run`, `release`-Integration) lassen sich lokal nicht vollstaendig simulieren.

### Log- und Artefakt-Handling

Das Skript legt Protokolle im angegebenen Log-Ordner an, standardmaessig unter `tmp/`:

- `actionlint_<timestamp>.log`
- `act_dryrun_<event>_<timestamp>.log`

Damit bleiben Ergebnisse reproduzierbar und lassen sich nach dem Lauf mit
`Get-Content` oder `Tee-Object` weiter analysieren.

### Erfolgs-/Fehlerkriterium

- `lint`/`dryrun`/`all` geben `ExitCode 0` aus, wenn alle durchgefuehrten Checks okay waren.
- Ein Fehler in einem der Tests setzt den Gesamtstatus auf `1`.
- Die Zusammenfassung meldet eindeutig, welche Phase fehlerhaft war und welcher Logpfad relevant ist.

### Zusätzliche Hinweise zur Umgebung

- In einer WSL-Umgebung ohne `pwsh` oder ohne laufenden Docker-Daemon kann das lokale Testsystem nicht ausgefuehrt werden.
- In solchen Faellen ist der richtige Nachweis eine echte GitHub-Actions-Ausfuehrung auf GitHub selbst, nicht nur die lokale Syntaxverifikation.
- Das Repo-Skript ist daher ein notwendiges lokales Gate, aber kein Ersatz fuer einen echten Remote-Lauf.

## Troubleshooting
- Lokal zuerst linten: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint`
- Danach Dry-Run: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun`
- Registry, Guidelines und Reaktivierungsbegruendung bei Struktur-Aenderungen immer zusammen aktualisieren.
- Geplante Dateinamen-Migrationen sind in `.github/docs/WORKFLOW_FILENAME_RENAME_MATRIX.md` dokumentiert.

## Doxygen Coverage Threshold (Maintainer)
- Der Doxygen-Coverage-Gate liest den Schwellwert zentral aus `.github/ci-scope-config.yaml` unter `quality_gates.docs_coverage_threshold`.
- Kanonische CI-Konfiguration ist `Doxyfile.audit`; der PR-Gate-Workflow verwendet eine daraus abgeleitete, modul-scoped Laufkonfiguration.
- Aktueller Standardwert ist `95`.
- Empfohlene stufenweise Anhebung ab diesem Stand: `95 -> 97 -> 99`.
- Strukturfehler (`@brief`, `@param`, `@return`, fehlender Doxygen-Block, Doxygen-Warnungen, fehlendes XML`) sind im PR-Gate blocking; Coverage < Threshold ist auf `develop` beobachtbar und auf Release-/Phase-6-Scope eskalationspflichtig.
- Ein genehmigter Tier-1-Override wird über das Label `governance/doxygen-waiver` sichtbar gemacht; ohne diese Freigabe bleibt der Coverage-Verstoss im eskalationspflichtigen Scope blockierend.
- Nach jeder Anhebung zuerst mehrere PR-Laeufe beobachten und nur bei stabiler Signalqualitaet weiter erhoehen.
- Bei hoher False-Positive-Rate den Schwellwert voruebergehend zuruecksetzen und Doku-Luecken gezielt abbauen.

## Label Policy

### Kanonische Label-Definitionen
Alle Repository-Labels sind in `.github/labels.yml` definiert (Name, Farbe, Beschreibung, Typ).
Der Workflow `maintenance-labels.yml` synchronisiert diese Labels wöchentlich und bei Änderungen
an `.github/labels.yml`. Labels dürfen nur in `.github/labels.yml` hinzugefügt oder geändert werden.

### Milestone-Automation (kanonisch)
Milestones werden analog zentral verwaltet:

- Kanonische Definitionen in `.github/milestones.yml`
- Automatische Synchronisierung durch `.github/workflows/maintenance-milestones.yml`
- Automatische Zuordnung für Issues/PRs über Label-Regeln und optionales `Target Version`-Override

Damit werden fehlende Milestones nicht mehr nur protokolliert, sondern optional automatisch angelegt
(`auto_create_missing_milestones`) und direkt zugewiesen. Enthalten sind u. a. eigene Lanes für
`HOTPATCH` und `LONG-TERM`.

### Label-Typen
- **blocker** — blockiert Merge oder erfordert sofortige Maßnahme
- **warning** — Aufmerksamkeit erforderlich, kein harter Merge-Blocker
- **info** — informativ, keine Aktion erforderlich

### Blocker-Labels (blockieren Merge via Branch Protection Rules)
| Label | Gesetzt von | Gelöscht von |
|---|---|---|
| `ci/build-failed` | `build-mainline.yml` (push → develop, Failure) | `build-mainline.yml` bei nächstem Erfolg |
| `ci/test-failed` | `build-mainline.yml` (push → develop, Test-Failure) | `build-mainline.yml` bei nächstem Erfolg |
| `ci/failure` | `maintenance-build-issues.yml`, `maintenance-ci-health.yml` | manuell / nach Behebung |
| `ci/chronic-failure` | `maintenance-ci-health.yml` (>30% Fehlerrate) | `maintenance-ci-health.yml` bei Erholung |
| `ci/build-error` | `maintenance-build-issues.yml` | manuell |
| `governance/drift` | `compliance-governance-gates.yml` | `compliance-governance-gates.yml` nach Korrektur |
| `governance/wave-gate-fail` | `compliance-governance-gates.yml` | `compliance-governance-gates.yml` nach Gate-Erfüllung |
| `security` | `maintenance-issues.yml` | nach Schließung des Issues |
| `security/critical` | `maintenance-issues.yml` (critical findings) | nach Schließung des Issues |
| `status/needs-approval` | `maintenance-ci-health.yml`, `maintenance-issues.yml` | manuell durch Maintainer |
| `status/blocked` | manuell, `maintenance-build-issues.yml` | manuell |
| `breaking-change` | `automation-community.yml` (AI-Analyse) | manuell |
| `release_critical` | `actions/labeler` (path-basiert) | manuell |

### Trigger-Policy für Workflows
- **Schwere Build-Workflows** (C++ compile, test, scan, > 15 min) triggern **nur auf `push` zu kanonischen Branches**, nicht auf `pull_request`.
  - Betrifft: `build-mainline.yml`, `compliance-supply-chain.yml`
- **Leichtgewichtige PR-Gate-Workflows** (Policy-Checks, Lint, < 5 min) triggern auf `pull_request`.
  - Betrifft: `gate-pr-core.yml`, `gate-pr-*.yml`, `compliance-governance-gates.yml`, `gate-*.yml`
- Jeder PR-Workflow muss `concurrency` mit `cancel-in-progress: true` setzen.
- `paths:` muss für PR-Workflows eng begrenzt sein — kein `src/**` oder `include/**` als einziges Muster.

## PR Governance for AI-Generated Changes
- Pull Requests labeled `ai-generated` require maintainer review before merge.
- Copilot PR summary/review support should be enabled in repository settings to assist human review, but does not replace maintainer approval.
- `ai-generated` PRs should explicitly document validation scope and documentation synchronization status.
- Auto-merge for `ai-generated` PRs should remain disabled unless a maintainer explicitly authorizes it.