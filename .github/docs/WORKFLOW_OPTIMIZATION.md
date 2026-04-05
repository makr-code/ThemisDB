# Workflow Optimization Best Practices

Dieses Dokument beschreibt das Optimierungsmodell fuer den schlanken
8-Workflow-Kern.

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

Edition-spezifische Builds werden ueber eine Matrix in
`.github/workflows/03-editions_ci.yml` umgesetzt, nicht ueber eigene Dateien
pro Edition.

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

```powershell
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun
```

Hinweis: Event `schedule` kann lokal ohne passende Stages als Skip enden.

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
