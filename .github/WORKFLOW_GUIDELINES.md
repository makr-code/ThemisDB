# Workflow Guidelines

## Scope
Diese Richtlinie gilt fuer den schlanken, release-zentrierten Workflow-Kern.
Die kanonische Liste aktiver Workflows steht in `.github/WORKFLOW_REGISTRY.md`.

## Aktive Workflows (8)
- `.github/workflows/01-core_ci.yml`
- `.github/workflows/03-editions_ci.yml`
- `.github/workflows/04-release_bootstrap-release-branches.yml`
- `.github/workflows/04-release_build-binaries.yml`
- `.github/workflows/04-release_publish-community.yml`
- `.github/workflows/04-release_publish-private.yml`
- `.github/workflows/09-pr-gates_quick-checks.yml`
- `.github/workflows/09-pr-gates_path-policy.yml`

## Naming Conventions
- Behalte den numerischen Prefix je Verantwortungsbereich (`01`, `03`, `04`, `09`) bei.
- Dateinamen muessen den Zweck klar beschreiben und lane-neutral bleiben.
- Keine neuen kategorischen Prefixe ohne Registry-Update.

## Best Practices
- Trigger nur fuer reale Gates/Release-Lanes definieren (keine Schatten-CI).
- `paths:` und `branches:` eng schneiden, damit unnötige Runs ausbleiben.
- `concurrency` mit `cancel-in-progress` auf Push/PR-Workflows setzen.
- Berechtigungen minimal halten (`permissions` least privilege).

## Security Guidelines
- Keine Secrets im YAML oder in Shell-Skripten hardcoden.
- Publish-Workflows nur ueber Tag- oder Environment-Gates freigeben.
- Third-party Actions auf stabile Major-Versionen pinnen.

## Manually Triggering Workflows
Empfohlen via GitHub CLI:

```bash
gh workflow run "03-editions_ci.yml" --repo makr-code/ThemisDB --ref develop --field edition=COMMUNITY --field build_type=Release
gh workflow run "04-release_bootstrap-release-branches.yml" --repo makr-code/ThemisDB --ref main
```

## Troubleshooting
- Lokal zuerst linten: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint`
- Danach Dry-Run: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode dryrun`
- Registry und Strategie bei Struktur-Aenderungen immer zusammen aktualisieren.