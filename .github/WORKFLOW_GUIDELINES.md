# Workflow Guidelines

## Adding a New Workflow

1. **Choose the correct category** — use the table in `WORKFLOW_ORGANIZATION.md`.
2. **Place in the right subdirectory** — e.g. a new storage feature CI goes in
   `02-feature-modules/storage/`.
3. **Follow the naming convention**:
   - Feature CI: `<feature-name>-ci.yml`
   - Event-driven / maintenance: `<purpose>.yml`
4. **Gate on changed files** — call `01-core/ci-scope-classifier.yml` at the
   top of your workflow to avoid unnecessary runs:

```yaml
jobs:
  ci-scope:
    uses: ./.github/workflows/01-core/ci-scope-classifier.yml

  my-job:
    needs: ci-scope
    if: needs.ci-scope.outputs.has_code_changes == 'true'
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      # ...
```

## Reusable Workflows

Two workflows are designed to be called by others (`workflow_call`):

| Workflow | Path | Purpose |
|----------|------|---------|
| CI Scope Classifier | `01-core/ci-scope-classifier.yml` | Gates jobs on changed file paths |
| Edition Build CI | `03-editions/edition-build-ci.yml` | Shared edition build logic |

When calling them, always use the full path from the repository root:

```yaml
uses: ./.github/workflows/01-core/ci-scope-classifier.yml
uses: ./.github/workflows/03-editions/edition-build-ci.yml
```

## Security Best Practices

- Never print secrets in workflow steps.
- Use `secrets: inherit` only when calling internal reusable workflows.
- Prefer `permissions: read-all` and grant only what is needed.
- Pin third-party actions to a full commit SHA, not a mutable tag.

## Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| CI workflow | `<feature>-ci.yml` | `transaction-ssi-ci.yml` |
| Maintenance workflow | `<purpose>.yml` | `auto-label.yml` |
| Reusable workflow | `<name>-ci.yml` or `<name>.yml` with `workflow_call` trigger | `edition-build-ci.yml` |
| Workflow `name:` field | Title case, concise | `Transaction SSI CI` |
| Job IDs | kebab-case | `run-ssi-tests` |

## Category Quick Reference

| Category | Use when… |
|----------|-----------|
| `01-core/` | Shared infrastructure used by many other workflows |
| `02-feature-modules/` | Testing a specific ThemisDB feature or module |
| `03-editions/` | Building or validating a product edition |
| `04-release/` | Publishing, archiving, or deploying a release |
| `05-quality/` | Security scanning, build checks, or config validation |
| `06-infrastructure/` | GPU, distributed systems, networking, observability |
| `07-data-pipelines/` | Data ingestion, streaming, import/export |
| `08-maintenance/` | Labels, docs sync, audits, SDK generation |
| `09-pr-gates/` | Blocking or labelling PRs based on changed paths |
| `docs/` | Generating or publishing documentation |
