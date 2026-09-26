# Workflow Governance Automation — Phase 3 Implementation

> Author: ThemisDB Contributors
> Created: 2026-09-26
> Last Updated: 2026-09-26
> Implemented: 2026-09-26
> Status: ACTIVE
> Scope: Automated enforcement of WORKFLOW_GUIDELINES.md and WORKFLOW_REGISTRY.md

## Overview

Phase 3 implements two key automation layers to prevent workflow governance drift:

1. **Pre-Commit Hook** (`workflow-registry-sync.sh`) — Local validation before push
2. **CI-Gate Workflow** (`gate-workflow-guidelines.yml`) — Automated PR validation in GitHub

## Component 1: Pre-Commit Hook

### Installation

```bash
# Automatic setup (one-time per clone)
./setup-git-hooks.sh

# Manual setup
git config core.hooksPath .githooks
```

### Hook Location
- Script: `.github/pre-commit-hooks/workflow-registry-sync.sh`
- Dispatcher: `.githooks/pre-commit`
- Setup: `setup-git-hooks.sh`

### Validation Rules

**Rule 1: Workflow Registry Sync**
- Enforces: All `.github/workflows/*.yml` files are documented in `WORKFLOW_REGISTRY.md`
- Trigger: On any commit touching `.github/workflows/`
- Behavior: Blocks commit if any workflow is undocumented
- Failure message: Lists undocumented workflows and instructs user to update REGISTRY

### Example: Blocked Commit

```
❌ WORKFLOW REGISTRY SYNC CHECK FAILED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  Total workflows: 83
  Documented: 82
  Missing from REGISTRY:
    - my-new-workflow.yml

  Action: Update .github/WORKFLOW_REGISTRY.md with the missing workflows
          and commit again.
```

## Component 2: CI-Gate Workflow (gate-workflow-guidelines.yml)

### Trigger Points

| Trigger | When | Scope |
|---------|------|-------|
| **pull_request** | PR modifies `.github/workflows/*.yml` | Changed files only |
| **schedule** | Nightly 02:00 UTC | All workflows (drift detection) |
| **workflow_dispatch** | Manual (ad-hoc) | Selectable scope (all/changed/registry-sync) |

### Validation Checks

| Check | Severity | Fix |
|-------|----------|-----|
| Naming Convention (Build:/Gate:/Release:/etc.) | VIOLATION | Add proper prefix to workflow `name:` field |
| Concurrency block present | WARNING | Add `concurrency:` section with `group` and `cancel-in-progress` |
| Matrix workflows with qualified groups | WARNING | Ensure `group:` includes `${{ matrix.* }}` variables |
| cancel-in-progress: true without matrix | WARNING | Use `false` or qualify with matrix dimensions |
| GitHub Actions floating versions | WARNING | Pin to SHA (e.g., `actions/checkout@692973e3d...`) |
| push.tags without branches guard | VIOLATION | Add `branches: [develop, main, community, release/**]` |
| Registry sync (all workflows documented) | VIOLATION | Update `WORKFLOW_REGISTRY.md` |

### Outputs

**On PR (with comment):**
```
## Workflow Guidelines Compliance ✅ PASS

- **Violations**: 0
- **Warnings**: 2

See the job logs for details.

Reference: WORKFLOW_GUIDELINES.md
```

**On Failure:**
- Blocks PR merge until violations are fixed
- Warnings do not block merge but should be addressed

## Governance Rules Enforced

### 1. Naming Convention
All workflows must follow naming prefixes:
- `Build:` — Compile and build jobs
- `Gate:` — PR validation gates
- `Release:` — Release/publish workflows
- `Maintenance:` — Housekeeping and monitoring
- `Security:` — Security scanning
- `Compliance:` — Compliance checks
- `Edition:` — Edition-specific workflows
- `Automation:` — Automated coordination
- `Wiki:` — Documentation automation
- `Benchmark:` — Performance measurement
- `Governance:` — Governance and sign-off
- `Reusable:` — Reusable workflow templates
- `Publish:` — Content publication

### 2. Concurrency Configuration
- All workflows MUST define `concurrency:` block
- `group:` must uniquely identify workflow + trigger context
- For matrix workflows: `group` MUST include matrix dimensions (e.g., `${{ matrix.phase }}`)
- `cancel-in-progress:` should be `false` unless explicitly justified

### 3. Trigger Policy
- `push:` with `tags:` MUST include `branches:` guard
  - Allowed: `develop`, `community`, `release/**`
  - Prevents accidental release triggers from feature branches
- All triggers must be explicitly scoped (no repo-wide wildcards)

### 4. GitHub Action Pinning
- All `uses:` statements must reference SHA or stable version
- PROHIBITED: `@latest`, `@main`, `@master` (floating versions)
- REQUIRED: `@v1.2.3` (or SHA) for supply chain security

### 5. Registry Sync
- Every workflow in `.github/workflows/*.yml` MUST be documented in `WORKFLOW_REGISTRY.md`
- Documentation includes: filename, purpose, category, trigger info
- Failure to document = Git pre-commit failure + CI-gate violation

## Workflow: Adding a New Workflow

### Step 1: Create Workflow
```bash
cat > .github/workflows/build-my-feature.yml << 'EOF'
name: "Build: My Feature"
on:
  push:
    branches: [develop]
    paths: ['src/my-feature/**']
  pull_request:
    paths: ['src/my-feature/**']

concurrency:
  group: build-my-feature-${{ github.ref }}
  cancel-in-progress: false

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@692973e3d937dc1785ba5b1e112e8e93a61d6f60
      # ... rest of workflow
EOF
```

### Step 2: Document in WORKFLOW_REGISTRY.md
```markdown
| `build-my-feature.yml` | Build: My Feature (Feature-specific build) | active |
```

### Step 3: Commit
```bash
git add .github/workflows/build-my-feature.yml .github/WORKFLOW_REGISTRY.md
git commit -m "build: Add my-feature workflow"
# ✅ Pre-commit hook validates Registry sync
# ✅ Push to GitHub
# ✅ CI-gate validates compliance (auto-comment on PR)
```

## Troubleshooting

### Pre-Commit Hook: "Workflow Registry Sync Check Failed"

**Problem**: Added new workflow but forgot to document it

**Solution**:
1. Open `.github/WORKFLOW_REGISTRY.md`
2. Add a new row in the appropriate category table
3. Format: `| \`workflow-name.yml\` | Workflow: Description | status |`
4. Re-run commit after updating the registry: `git commit`

### CI-Gate: "push.tags without branches guard"

**Problem**: Workflow has `push: tags:` but no `branches:` constraint

**Solution**:
```yaml
push:
  tags: ['v*', 'release-*']
  branches: [develop, community, release/**]  # ← Add this
```

### CI-Gate: "Matrix workflow without matrix-qualified concurrency"

**Problem**: Workflow has `matrix:` but concurrency group doesn't include matrix vars

**Solution**:
```yaml
concurrency:
  group: my-workflow-${{ github.ref }}-${{ matrix.os }}-${{ matrix.compiler }}  # ← Add matrix vars
  cancel-in-progress: false
```

## Metrics & Observability

### Registry Sync Health
- Pre-commit hook output logs to `git commit` stdout
- Nightly CI-gate run on schedule (02:00 UTC) detects drift
- Failed gate creates GitHub issue (via `upsert_issue` action)

### Compliance Coverage
- All 83 workflows monitored
- Violations block PR merge
- Warnings tracked for compliance reporting

## Future Enhancements

1. **Actionlint Integration**: Validate YAML syntax and logic errors
2. **Concurrency Matrix Analysis**: Automated detection of concurrency group conflicts
3. **Action Update Bot**: Automated PR to pin new GitHub Action versions
4. **Compliance Dashboard**: Real-time metrics on workflow policy adherence
5. **Historical Audit**: Track governance violations over time

## References

- [WORKFLOW_GUIDELINES.md](.github/WORKFLOW_GUIDELINES.md) — Normative specification
- [WORKFLOW_REGISTRY.md](.github/WORKFLOW_REGISTRY.md) — Workflow inventory (83 workflows)
- [.github/workflows/gate-workflow-guidelines.yml](.github/workflows/gate-workflow-guidelines.yml) — CI-gate implementation
- [.github/pre-commit-hooks/workflow-registry-sync.sh](.github/pre-commit-hooks/workflow-registry-sync.sh) — Pre-commit hook
- [setup-git-hooks.sh](setup-git-hooks.sh) — Installation script

## Sign-Off

| Role | Name | Date | Status |
|------|------|------|--------|
| Implementation | CI/CD Governance | 2026-09-26 | ✅ COMPLETE |
| Verification | Workflow Gates | 2026-09-26 | ⏳ Pending first run |
| Approval | DevOps Team | TBD | ⏳ Pending |
