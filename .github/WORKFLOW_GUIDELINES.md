# Workflow Guidelines

## Scope
Diese Richtlinie gilt fuer den schlanken, release-zentrierten Workflow-Kern.
Die kanonische Liste aktiver Workflows steht in `.github/WORKFLOW_REGISTRY.md`.
Workflows unter `.github/no_workflows/` gelten als bewusst deaktivierte Quarantaene und
duerfen nicht stillschweigend reaktiviert werden.

## Aktive Workflows (21)
- `.github/workflows/automation-community.yml`
- `.github/workflows/ci-benchmarks.yml`
- `.github/workflows/ci-build.yml`
- `.github/workflows/ci-pr-gates.yml`
- `.github/workflows/ci-release.yml`
- `.github/workflows/codeql.yml`
- `.github/workflows/compliance-supply-chain.yml`
- `.github/workflows/copilot-ollama-router-ci.yml`
- `.github/workflows/copilot-regression-guard.yml`
- `.github/workflows/docker-image.yml`
- `.github/workflows/edition-hyperscaler-ci.yml`
- `.github/workflows/governance-gates.yml`
- `.github/workflows/maintenance-cache-warming.yml`
- `.github/workflows/maintenance-ci-health.yml`
- `.github/workflows/maintenance-security-alerts.yml`
- `.github/workflows/maintenance-gs3-gaps.yml`
- `.github/workflows/maintenance-docs.yml`
- `.github/workflows/quality-static-analysis.yml`
- `.github/workflows/release-changelog.yml`
- `.github/workflows/security-pentest-quarterly.yml`
- `.github/workflows/security-scanning.yml`
- `.github/workflows/security.yml`
- `.github/workflows/fortify.yml`

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
- Behalte den numerischen Prefix je Verantwortungsbereich (`01`, `03`, `04`, `09`) bei.
- Dateinamen muessen den Zweck klar beschreiben und lane-neutral bleiben.
- Keine neuen kategorischen Prefixe ohne Registry-Update.

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
  Enforcement: `actionlint` + SHA-Pin-Prüfung in `quality-static-analysis.yml`.
- Compliance-Gates fuer Dependencies muessen branch- und pfadbegrenzt sein und ein downloadbares Audit-Artefakt erzeugen.
- OIDC-basierte Authentifizierung (kein long-lived PAT) fuer ghcr.io und neue Registry-Ziele.

## Build Caching (sccache)
- `ci-build.yml` und `maintenance-cache-warming.yml` nutzen `mozilla-actions/sccache-action` mit `SCCACHE_GHA_ENABLED=true`.
- CMake muss mit `-DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache` konfiguriert werden.
- Cache-Warming erfolgt wöchentlich (montags 00:00 UTC) für Linux (GCC) und Windows (MSVC).
- Erwartete Wiedertreffer-Rate: 60–80% bei unveränderter Toolchain + vcpkg-Baseline.

## CI Health Dashboard
- `maintenance-ci-health.yml` aggregiert wöchentlich (sonntags 06:00 UTC) pass/fail-Raten je Workflow.
- Schwellwert für chronische Fehler: >30% Fehlerrate bei ≥3 Fehlern → öffnet `ci/chronic-failure`-Issue.
- Lookback-Fenster konfigurierbar via `workflow_dispatch` Input `lookback_days` (Standard: 7).

## Manually Triggering Workflows
Empfohlen via GitHub CLI:

```bash
gh workflow run "03-editions_ci.yml" --repo makr-code/ThemisDB --ref develop --field edition=COMMUNITY --field build_type=Release
gh workflow run "ci-release.yml" --repo makr-code/ThemisDB --ref develop --field edition=community --field build_matrix=community-only
```

## Troubleshooting
- Lokal zuerst linten: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint`
- Danach Dry-Run: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun`
- Registry, Guidelines und Reaktivierungsbegruendung bei Struktur-Aenderungen immer zusammen aktualisieren.

## Doxygen Coverage Threshold (Maintainer)
- Der Doxygen-Coverage-Gate liest den Schwellwert zentral aus `.github/ci-scope-config.yaml` unter `quality_gates.docs_coverage_threshold`.
- Standardwert ist `90`.
- Empfohlene stufenweise Anhebung: `90 -> 92 -> 95`.
- Nach jeder Anhebung zuerst mehrere PR-Laeufe beobachten und nur bei stabiler Signalqualitaet weiter erhoehen.
- Bei hoher False-Positive-Rate den Schwellwert voruebergehend zuruecksetzen und Doku-Luecken gezielt abbauen.

## PR Governance for AI-Generated Changes
- Pull Requests labeled `ai-generated` require maintainer review before merge.
- Copilot PR summary/review support should be enabled in repository settings to assist human review, but does not replace maintainer approval.
- `ai-generated` PRs should explicitly document validation scope and documentation synchronization status.
- Auto-merge for `ai-generated` PRs should remain disabled unless a maintainer explicitly authorizes it.