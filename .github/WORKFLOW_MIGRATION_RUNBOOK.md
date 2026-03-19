# Workflow Migration Runbook

## Overview

This runbook documents the migration of ThemisDB GitHub Actions workflows from
a flat structure to a hierarchical 9-category layout under `.github/workflows/`.

## Migration Summary

| Metric | Value |
|--------|-------|
| Workflows migrated | 138 |
| Old structure | Flat (all in `.github/workflows/`) |
| New structure | 9 categories + subcategories |
| Breaking changes | None (all `uses:` references updated atomically) |
| Git history | Preserved via `git mv` |

## Completed Steps

### Step 1: Directory structure creation

All category and subcategory directories created:

```
.github/workflows/
├── _templates/
├── 01-core/
├── 02-feature-modules/
│   ├── acceleration/
│   ├── adaptive-query/
│   ├── chimera/
│   ├── config/
│   ├── llm/
│   ├── replication/
│   ├── resilience/
│   ├── security/
│   ├── storage/
│   └── transactions/
├── 03-editions/
├── 04-release/
├── 05-quality/
│   ├── build/
│   ├── security/
│   └── validation/
├── 06-infrastructure/
│   ├── distributed/
│   ├── gpu/
│   ├── networking/
│   └── observability/
├── 07-data-pipelines/
├── 08-maintenance/
├── 09-pr-gates/
└── docs/
```

### Step 2: Workflow file migration

All 138 workflow files moved using `git mv` to preserve history.
See `WORKFLOW_REGISTRY.md` for the complete old→new path mapping.

### Step 3: Reference updates

Two reusable workflows are called by many others. Their `uses:` paths were
updated across all callers:

| Old path | New path | Callers updated |
|----------|----------|-----------------|
| `./.github/workflows/ci-scope-classifier.yml` | `./.github/workflows/01-core/ci-scope-classifier.yml` | 107 |
| `./.github/workflows/edition-build-ci.yml` | `./.github/workflows/03-editions/edition-build-ci.yml` | 5 |

## Rollback Procedure

If a rollback is required, revert the migration commit:

```bash
git revert <migration-commit-sha>
git push origin <branch>
```

This restores all files to their original flat-structure locations and reverts
all `uses:` reference updates in a single operation.

## Validation Checklist

- [x] All 138 workflow files present in new locations
- [x] No `.yml` files remain in `.github/workflows/` root
- [x] All `uses:` references updated to new paths
- [x] Git history preserved (`git log --follow` works on moved files)
- [ ] Workflow runs verified green post-migration
- [ ] Branch protection rules reviewed (if workflow names are referenced)

## Troubleshooting

### Workflow not triggering

GitHub Actions discovers workflows anywhere under `.github/workflows/` including
subdirectories. If a workflow is not triggering, verify:

1. The file is a valid YAML with a `name:` and `on:` key.
2. The trigger event matches the expected branch/path filters.
3. The `uses:` path (if a reusable workflow) matches the new location exactly.

### `uses:` path not found

If you see `Could not find reusable workflow`, check that the path uses the
new location:

```yaml
# Correct
uses: ./.github/workflows/01-core/ci-scope-classifier.yml

# Incorrect (old flat path — no longer valid)
uses: ./.github/workflows/ci-scope-classifier.yml
```

### Adding a new workflow

Place the new workflow in the appropriate category subdirectory and follow the
naming convention documented in `WORKFLOW_ORGANIZATION.md`.
