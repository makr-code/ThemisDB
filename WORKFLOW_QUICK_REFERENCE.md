# Workflow Issues Quick Reference

**Generated:** 2026-08-16  
**Quick Answer Guide for All Workflow Issues**

---

## 🔴 CRITICAL - Must Fix Today

### Issue: ci-release.yml Line 594

**The Bug:**
```yaml
❌ WRONG:
uses: ./.github/workflows/release-changelog.yml

✅ CORRECT:
Add workflow_call: to ci-release.yml's on: section
```

**Why It Matters:**
- Every release fails to update changelog
- Production releases incomplete

**The Fix (2 lines):**
```yaml
# Add to .github/workflows/ci-release.yml "on:" section:

  workflow_call:
    secrets: inherit
```

**How to Verify:**
1. Trigger ci-release.yml via workflow_dispatch
2. Check CHANGELOG.md was updated
3. Should see no errors in GitHub Actions logs

---

## 🟡 MEDIUM - Fix This Sprint

### Issue 1: ci-benchmarks.yml

**The Bug:**
```yaml
❌ WRONG:
${{ env.ARTIFACT_DIR }}  # Variable undefined

✅ CORRECT:
env:
  ARTIFACT_DIR: 'benchmark-artifacts'
```

**The Fix (1 line):**
```yaml
env:
  ARTIFACT_DIR: 'benchmark-artifacts'
```

**How to Verify:**
1. Trigger ci-benchmarks.yml
2. Check logs for ARTIFACT_DIR value
3. Should NOT be empty

---

### Issue 2: maintenance-issues.yml

**The Bug:**
```yaml
❌ WRONG:
${{ env.TARGET_BRANCH }}  # Variable undefined

✅ CORRECT:
env:
  TARGET_BRANCH: ${{ github.event.repository.default_branch }}
```

**The Fix (1 line):**
```yaml
env:
  TARGET_BRANCH: ${{ github.event.repository.default_branch }}
```

**How to Verify:**
1. Trigger maintenance-issues.yml
2. Check logs for TARGET_BRANCH value
3. Should match your default branch (develop/main)

---

## All Issues at a Glance

| File | Line | Issue | Fix | Time |
|------|------|-------|-----|------|
| ci-release.yml | 594 | Missing `workflow_call:` | Add to `on:` | 2 min |
| ci-benchmarks.yml | env | Undefined `ARTIFACT_DIR` | Define env var | 1 min |
| maintenance-issues.yml | env | Undefined `TARGET_BRANCH` | Define env var | 1 min |

---

## Testing Each Fix

### Test ci-release.yml
```bash
# Navigate to GitHub Actions > CI Release
# Click "Run workflow" > Select branch > Run
# Should complete without errors
# Verify CHANGELOG.md changes
```

### Test ci-benchmarks.yml
```bash
# Navigate to GitHub Actions > CI Benchmarks
# Click "Run workflow" > Select branch > Run
# Should complete without errors
# Verify artifacts uploaded
```

### Test maintenance-issues.yml
```bash
# Navigate to GitHub Actions > Maintenance Issues
# Click "Run workflow" > Select branch > Run
# Should complete without errors
# Verify issue operations on correct branch
```

---

## Documentation

**Comprehensive Details:**
- `WORKFLOW_ANALYSIS_REPORT.md` - Full analysis of all 23 workflows
- `WORKFLOW_FIX_PLAN.md` - Step-by-step implementation guide
- `WORKFLOW_ISSUES_SUMMARY.txt` - Executive summary

---

## Key Files to Modify

Only 3 files need changes:
1. `.github/workflows/ci-release.yml` (add 2 lines)
2. `.github/workflows/ci-benchmarks.yml` (add 1 line)
3. `.github/workflows/maintenance-issues.yml` (add 1 line)

**Total Lines to Add:** 4 lines
**Total Time:** ~5 minutes
**Complexity:** Low - Simple additions to `on:` and `env:` sections

---

## When You're Done

✅ All tests pass
✅ No undefined variable warnings
✅ Workflows execute as expected
✅ Commit to develop
✅ Deploy with next release

---

## Need Help?

- **Detailed Analysis:** See `WORKFLOW_ANALYSIS_REPORT.md`
- **Implementation Steps:** See `WORKFLOW_FIX_PLAN.md`
- **Questions about workflow syntax:** Check GitHub Actions documentation
- **Questions about specific workflow logic:** Review workflow comments

---

## Summary

- **Total Issues:** 3
- **Severity:** 1 CRITICAL, 2 MEDIUM
- **Fix Time:** 5-10 minutes
- **Complexity:** Low
- **Risk:** Low
- **Impact if Fixed:** High (ensures release process works, improves reliability)

Ready to fix? Start with `WORKFLOW_FIX_PLAN.md`
