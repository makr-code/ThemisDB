# Workflow Optimization Best Practices

Dieses Dokument beschreibt das Optimierungsmodell fuer den aktuellen,
release-zentrierten Workflow-Kern von ThemisDB.

Der Repository-Stand ist nicht mehr auf einen 8-Workflow-Kern reduziert; der
aktive Kern umfasst derzeit 38 Workflow-Dateien im Verzeichnis
`.github/workflows/`, mit enger Trigger-Disziplin und Quarantaene fuer
uebertriggernde Legacy-Lanes in `.github/no_workflows/`.

Die Governance fuer Labels und Milestones ist dabei getrennt, aber parallel
automatisiert:

- Label-Sync: `.github/workflows/maintenance-labels.yml` aus `.github/labels.yml`
- Milestone-Sync + Assignment: `.github/workflows/maintenance-milestones.yml` aus `.github/milestones.yml`

## 1. Trigger minimieren

- `pull_request` immer mit `types: [opened, synchronize, reopened]`.
- `push` nur auf produktive Lanes (`develop`, `main`, `enterprise`, `hyperscaler`).
- `paths` einsetzen, wenn ein Workflow nicht global relevant ist.

Beispiel:

```yaml
on:
  pull_request:
    types: [opened, synchronize, reopened]
    paths:
      - 'src/**'
      - 'include/**'
      - 'CMakeLists.txt'
```

## 2. Matrix statt Workflow-Sprawl

Edition-spezifische Builds werden ueber die kanonische Lane
`.github/workflows/edition-hyperscaler-ci.yml` umgesetzt, nicht ueber
Legacy-Dateien pro Edition.

## 3. Lane-basierte Release-Gates

- Community-Publish getrennt von Private-Publish.
- Private Publish nur ueber Tag-Praefixe und Environments freigeben.
- Branch-Bootstrap als expliziter manueller Schritt.

## 4. Concurrency und Ressourcen

Fuer Push/PR-Workflows gilt:

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

Damit werden veraltete Laeufe auf derselben Ref sauber abgebrochen.

## 5. Lokale Validierung (Pflicht vor Merge)

Das Repository verwendet ein zweistufiges lokales Testsystem fuer GitHub Actions:

### 5.1 `actionlint` (YAML-/Workflow-Syntax-Lint)

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint
```

Dies startet `actionlint` als Docker-Container (`rhysd/actionlint:latest`). Der
Lint dient der statischen Validierung der Workflow-Dateien und erkennt im
Regelfall Probleme wie:

- invalides YAML
- falsche `on:`-Trigger-Definitionen
- invalides `uses:`-Pattern
- fehlerhafte `if:`/`${{ }}`-Ausdruecke
- ungeeignete Job-/Step-Struktur

### 5.2 `act` Dry-Run (Jobs/Run-Graph plausibilisieren)

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun
```

Das Skript ruft `act` fuer Standard-Events wie `push`, `pull_request`,
`workflow_dispatch` und `schedule` auf, ohne echten GitHub-Runner auszufuehren.
Es prueft damit, ob die Workflows lokal als Runner-Graph und Job-Sequenz
interpretierbar sind.

Beispiel:

```powershell
act -n push
act -n pull_request
act -n workflow_dispatch
```

Hinweis: Event `schedule` kann lokal ohne passende Stages als Skip enden; das ist
kein Codefehler, sondern ein lokales Dry-Run-Limit.

### 5.3 Voller lokale Check

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all
```

Der komplette Lauf erzeugt Logs im Ordner `tmp/` und beendet mit Exit-Code 0,
wenn alle ausgefuehrten Pruefschritte erfolgreich waren.

### 5.4 Was lokal nicht beweisen kann

- echte GitHub-Runner-Umgebung
- Hosted-Runtime-Umgebung mit echten Secrets, Environments und Registry-Rechten
- Events, die nur im remote GitHub-Kontext relevant sind (`release`, `workflow_run`, `issue_comment`, `pull_request_target`)

Local validation is therefore a gate, not a substitute for remote runner integration testing.

## 6. Haeufige Anti-Patterns

| Anti-pattern | Zielmuster |
|---|---|
| Neue Mini-Workflows pro Modul | Bestehende Kern-Workflows erweitern |
| Ungefilterte `pull_request` Trigger | `types` + gezielte `paths` |
| Fehlende `permissions` | Least-Privilege pro Job |
| Release-Logik in CI-Workflows mischen | Trennung: Core/Edition/Release/PR-Gates |

## 7. Siehe auch

- `.github/WORKFLOW_REGISTRY.md`
- `RELEASE_STRATEGY.md`
- `scripts/test-github-actions-local.ps1`
