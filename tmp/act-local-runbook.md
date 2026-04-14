# Local act Runbook (ThemisDB)

## Verified baseline

- Use Docker Desktop with running daemon (`docker version` must show Server section).
- For non-dry runs in this repo, prefer:
  - `--bind`
  - `--artifact-server-path tmp/act-artifacts`

## Proven commands

### 1) Dry-run matrix: Release workflows

PowerShell script:

- `tmp/run-act-release-matrix.ps1`

Outputs:

- `tmp/act-04-release-dryrun-matrix.txt`
- `tmp/act-04-release-dryrun-matrix.json`

### 2) Dry-run matrix: Maintenance workflows

PowerShell script:

- `tmp/run-act-maintenance-matrix.ps1`

Outputs:

- `tmp/act-08-maintenance-dryrun-matrix.txt`
- `tmp/act-08-maintenance-dryrun-matrix.json`

### 3) Non-dry sanity: Root Docs Hygiene

Input event file:

- `tmp/act-maint-hygiene-nondry.json`

Command:

```powershell
act workflow_dispatch -W .github/workflows/08-maintenance_root-docs-hygiene.yml -e tmp/act-maint-hygiene-nondry.json -j check --bind --artifact-server-path tmp/act-artifacts
```

### 4) Non-dry sanity: Docs Orphan Check

Input event file:

- `tmp/act-maint-orphan-nondry.json`

Command:

```powershell
act workflow_dispatch -W .github/workflows/08-maintenance_docs-orphan-check.yml -e tmp/act-maint-orphan-nondry.json -j check --bind --artifact-server-path tmp/act-artifacts
```

## Known local pitfalls

- Without `--bind`, large repo copy into container can hang at checkout/copy stages.
- Without `--artifact-server-path`, artifact upload may fail in local act runs.
- Windows workflows in dry mode may need platform mapping:

```powershell
-P windows-latest=catthehacker/ubuntu:act-latest
```
