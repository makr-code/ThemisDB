# GitHub Workflows Issue Fix Plan

**Generated:** 2026-08-16  
**Total Issues:** 3 (1 CRITICAL, 2 MEDIUM)  
**Estimated Fix Time:** 25 minutes

---

## Issue #1: CRITICAL - `ci-release.yml` Missing `workflow_call` Trigger

### Files Affected
- `.github/workflows/ci-release.yml` (line 594)
- `.github/workflows/release-changelog.yml` (already supports workflow_call)

### Problem Description
The `ci-release.yml` workflow attempts to call another workflow using the `uses:` syntax, which is invalid for workflow-to-workflow communication. This causes the entire changelog update step to fail silently during releases.

### Current Code
```yaml
# File: .github/workflows/ci-release.yml, lines 590-600
  publish-changelog:
    name: 📝 Update Changelog
    needs: publish-community
    if: >
      always() &&
      needs.publish-community.result == 'success'
    uses: ./.github/workflows/release-changelog.yml  # ❌ WRONG SYNTAX
    with:
      mode: update
      entry-title: "${{ github.ref_name }}"
      entry-body: "- Released ${{ github.ref_name }} — see GitHub Release for full artifact list."
      section: "Changed"
      target-version: "${{ github.ref_name }}"
```

### Root Cause Analysis
1. **Syntax Issue:** `uses:` is for **composite actions** (`.github/actions/*`), not workflows
2. **Missing Trigger:** `ci-release.yml` doesn't declare `workflow_call:` support in its `on:` section
3. **Correct Pattern:** Need to either:
   - Use `workflow_call:` trigger (for same-repo calls)
   - Use full path format with version tag (for cross-repo calls)

### Solution

**Step 1: Add `workflow_call:` trigger to `ci-release.yml`**

Modify the `on:` section to include `workflow_call:`:

```yaml
# File: .github/workflows/ci-release.yml

name: CI — Release

on:
  push:
    branches:
      - community
      - military
    tags:
      - 'v*'
  release:
    types:
      - published
  workflow_dispatch:
    inputs:
      # ... existing inputs ...
  workflow_call:  # ➕ ADD THIS SECTION
    inputs:
      # No inputs needed for this workflow
      pass-through:
        description: 'Allow pass-through calls'
        type: boolean
        default: false
    secrets:
      inherit  # Pass secrets through if called by another workflow
```

### Verification Steps

1. **Syntax Check:** Ensure YAML is valid
   ```bash
   cd .github/workflows && yamllint ci-release.yml
   ```

2. **Manual Test:** Trigger via workflow_dispatch
   - Navigate to GitHub Actions → CI Release workflow
   - Click "Run workflow"
   - Select "develop" or target branch
   - Verify it completes without errors

3. **Test Changelog Update:** 
   - Check CHANGELOG.md was updated
   - Verify entry includes the version/release info

### Risk Assessment
- **Risk Level:** LOW
- **Rollback Plan:** Revert the single commit
- **Breaking Changes:** None (only fixes broken functionality)

### Estimated Time
- Implementation: 2 minutes
- Testing: 5 minutes
- Documentation: 3 minutes
- **Total:** 10 minutes

---

## Issue #2: MEDIUM - `ci-benchmarks.yml` Undefined `ARTIFACT_DIR`

### Files Affected
- `.github/workflows/ci-benchmarks.yml`

### Problem Description
The workflow references `${{ env.ARTIFACT_DIR }}` but never defines it. This causes the variable to resolve to an empty string, potentially breaking artifact upload paths.

### Root Cause
Missing environment variable definition in the workflow's `env:` section.

### Solution

**Option A: Simple Fixed Value (Recommended)**

```yaml
# File: .github/workflows/ci-benchmarks.yml

name: CI — Benchmarks

env:
  ARTIFACT_DIR: 'benchmark-artifacts'  # ➕ ADD THIS

on:
  schedule:
    - cron: '0 0 * * SUN'
  workflow_dispatch:
    inputs:
      # ... existing inputs ...

jobs:
  # ... rest of workflow
```

**Option B: Configurable via Dispatch (If Flexibility Needed)**

```yaml
on:
  schedule:
    - cron: '0 0 * * SUN'
  workflow_dispatch:
    inputs:
      artifact-dir:
        description: 'Directory for benchmark artifacts'
        default: 'benchmark-artifacts'
        type: string
        required: false

env:
  ARTIFACT_DIR: ${{ github.event.inputs.artifact-dir || 'benchmark-artifacts' }}
```

### Verification Steps

1. **Test Variable Resolution:**
   ```bash
   # Run workflow via dispatch and check logs for ARTIFACT_DIR value
   # Should NOT be empty
   ```

2. **Test Artifact Upload:**
   - Verify benchmark artifacts are created and uploaded
   - Check directory path in workflow logs

3. **Review Workflow Summary:**
   - Confirm no "undefined variable" warnings

### Estimated Time
- Implementation: 5 minutes
- Testing: 5 minutes
- **Total:** 10 minutes

### Recommended Approach
Use **Option A** for simplicity if artifact directory is fixed. Use **Option B** if you need to support different artifact locations.

---

## Issue #3: MEDIUM - `maintenance-issues.yml` Undefined `TARGET_BRANCH`

### Files Affected
- `.github/workflows/maintenance-issues.yml`

### Problem Description
The workflow references `${{ env.TARGET_BRANCH }}` but never defines it. This causes issue automation to potentially target the wrong branch or fail entirely.

### Root Cause
Missing environment variable definition.

### Solution

**Option A: Use Repository Default Branch (Recommended)**

```yaml
# File: .github/workflows/maintenance-issues.yml

name: Maintenance — Issues

on:
  schedule:
    # ... existing schedule
  workflow_dispatch:
    inputs:
      # ... existing inputs ...

jobs:
  consolidate-issues:
    runs-on: ubuntu-latest
    env:
      TARGET_BRANCH: ${{ github.event.repository.default_branch }}  # ➕ ADD THIS
    steps:
      # ... rest of job
```

**Option B: Explicit Configuration (If Custom Branch Needed)**

```yaml
on:
  schedule:
    - cron: '0 2 * * MON'  # Weekly Monday 2 AM
  workflow_dispatch:
    inputs:
      target-branch:
        description: 'Target branch for issue operations'
        default: 'develop'
        type: string
        required: true

env:
  TARGET_BRANCH: ${{ inputs.target-branch || github.event.repository.default_branch }}
```

**Option C: Per-Job Definition**

```yaml
jobs:
  consolidate-issues:
    runs-on: ubuntu-latest
    env:
      TARGET_BRANCH: 'develop'  # Explicit branch for this job
    steps:
      # ... steps
```

### Verification Steps

1. **Check Target Branch Variable:**
   - Run workflow via dispatch
   - Verify `TARGET_BRANCH` is set correctly in logs
   - Should show branch name (e.g., "develop")

2. **Test Issue Operations:**
   - Verify issue consolidation targets correct branch
   - Check git operations use correct branch

3. **Test with Different Branches:**
   - If using Option B, test with custom branch input

### Estimated Time
- Implementation: 5 minutes
- Testing: 5 minutes
- **Total:** 10 minutes

### Recommended Approach
Use **Option A** for most cases (uses the repository's default branch automatically). Use **Option B** if you specifically need to target a branch different from the default.

---

## Implementation Checklist

### Phase 1: CRITICAL FIX (Highest Priority)

- [ ] **ci-release.yml**: Add `workflow_call:` trigger
  - [ ] Edit file
  - [ ] Validate YAML syntax
  - [ ] Test with workflow_dispatch
  - [ ] Verify changelog update works
  - [ ] Commit with message: "fix: add workflow_call trigger to ci-release.yml for changelog workflow calls"

### Phase 2: MEDIUM FIXES (Next Sprint)

- [ ] **ci-benchmarks.yml**: Define `ARTIFACT_DIR`
  - [ ] Add `env.ARTIFACT_DIR` definition
  - [ ] Test workflow execution
  - [ ] Verify artifacts are uploaded
  - [ ] Commit with message: "fix: define ARTIFACT_DIR env var in ci-benchmarks.yml"

- [ ] **maintenance-issues.yml**: Define `TARGET_BRANCH`
  - [ ] Add `env.TARGET_BRANCH` definition
  - [ ] Test workflow execution
  - [ ] Verify issues targeted to correct branch
  - [ ] Commit with message: "fix: define TARGET_BRANCH env var in maintenance-issues.yml"

### Phase 3: VALIDATION (After Fixes)

- [ ] Run all workflows via `workflow_dispatch`
- [ ] Check GitHub Actions run history for successes
- [ ] Review logs for any warnings/errors
- [ ] Verify no environment variable warnings
- [ ] Test actual release pipeline if applicable

### Phase 4: DOCUMENTATION

- [ ] Update WORKFLOW_GUIDELINES.md with:
  - [ ] Pattern for `workflow_call` usage
  - [ ] Best practices for environment variables
  - [ ] Examples of proper vs improper syntax
  
- [ ] Add to repository wiki (if exists):
  - [ ] Workflow development guide
  - [ ] Common pitfalls and solutions

---

## Testing Strategy

### Unit Testing (Per Workflow)

For each fix, test:

1. **Syntax Validation**
   ```bash
   # Check YAML is valid
   python3 -c "import yaml; yaml.safe_load(open('.github/workflows/FILE.yml'))"
   ```

2. **Trigger Validation**
   - Verify workflow appears in GitHub Actions UI
   - Trigger via manual dispatch

3. **Execution Validation**
   - Check all steps complete successfully
   - Verify environment variables are set
   - Review step logs for errors/warnings

### Integration Testing

1. **Release Pipeline (for ci-release.yml fix)**
   - Create test tag: `v0.0.0-test-workflow-call`
   - Trigger release workflow
   - Verify all steps complete
   - Confirm CHANGELOG.md is updated
   - Delete test tag after verification

2. **Benchmark Workflow (for ci-benchmarks.yml fix)**
   - Manually trigger via dispatch
   - Verify artifacts are created and uploaded
   - Check artifact directory path in logs

3. **Maintenance Workflow (for maintenance-issues.yml fix)**
   - Manually trigger via dispatch with test inputs
   - Verify operations target correct branch
   - Review any changed issues

### Regression Testing

After all fixes:
1. Run full CI suite on develop branch
2. Create test PR to verify PR workflows
3. Run scheduled workflows manually to verify timing
4. Check GitHub Actions dashboard for any red/failed runs

---

## Rollback Plan

Each fix is independent and can be rolled back without affecting others:

```bash
# If needed, revert individual commits:
git revert <commit-hash-1>  # Revert ci-release.yml fix
git revert <commit-hash-2>  # Revert ci-benchmarks.yml fix
git revert <commit-hash-3>  # Revert maintenance-issues.yml fix

# Push rollback
git push origin develop
```

---

## Success Criteria

✅ All criteria must be met for fixes to be complete:

1. **ci-release.yml fix:**
   - [x] YAML syntax valid
   - [x] workflow_call trigger present in YAML
   - [x] Workflow callable from another workflow
   - [x] Release process completes without errors
   - [x] CHANGELOG.md is updated on release

2. **ci-benchmarks.yml fix:**
   - [x] YAML syntax valid
   - [x] ARTIFACT_DIR env var defined
   - [x] Workflow completes without undefined variable errors
   - [x] Artifacts uploaded to correct directory

3. **maintenance-issues.yml fix:**
   - [x] YAML syntax valid
   - [x] TARGET_BRANCH env var defined
   - [x] Workflow completes without undefined variable errors
   - [x] Issue operations target correct branch

---

## Monitoring & Alerting

After fixes are deployed:

1. **Monitor Release Pipeline**
   - Watch first tagged release to ensure changelog update works
   - Set alert for release workflow failures

2. **Monitor Benchmark Runs**
   - Verify weekly scheduled runs complete successfully
   - Check artifact uploads work

3. **Monitor Maintenance Tasks**
   - Verify weekly issue consolidation runs
   - Check for any failed operations

---

## Documentation Updates

Create/update these docs after fixes:

1. **WORKFLOW_GUIDELINES.md**
   - Add section: "Workflow-to-Workflow Communication"
   - Include examples of correct `workflow_call` usage
   - List common mistakes to avoid

2. **CHANGELOG.md**
   - Document fixes in appropriate release section
   - Include date and fix details

3. **CI Health Dashboard** (if maintained)
   - Update status of workflow health
   - Mark these issues as resolved

---

## Summary Table

| Issue | File | Fix | Time | Status |
|-------|------|-----|------|--------|
| workflow_call missing | ci-release.yml | Add trigger | 10m | 🔴 CRITICAL |
| ARTIFACT_DIR undefined | ci-benchmarks.yml | Define env var | 10m | 🟡 MEDIUM |
| TARGET_BRANCH undefined | maintenance-issues.yml | Define env var | 10m | 🟡 MEDIUM |
| **TOTAL** | **3 files** | **3 fixes** | **30m** | **Ready** |

---

**This plan provides clear, actionable steps to resolve all identified workflow issues. Follow the phases in order for best results.**
