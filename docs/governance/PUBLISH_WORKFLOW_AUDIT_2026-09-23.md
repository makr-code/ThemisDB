# Publish Workflow Audit — 2026-09-23

## Executive Summary

Audit of all publish/release workflows identified **5 workflows with governance issues**:

- **3 Critical:** Automatic publishing to external registries without approval (WinGet, Docker, GitHub Release)
- **2 Partial:** Manual publishing without tracking/approval mechanism (Linux/Windows distributions)
- **1 Already Fixed:** Wiki publishing (wiki-publish-from-issue.yml implemented)
- **1 Good:** Changelog (manual workflow_dispatch only)

## Workflows Audited

| Workflow | Trigger | Risk | Status |
|----------|---------|------|--------|
| publish-wiki.yml | workflow_run | ❌ → ✅ | **FIXED** - Added approval gate |
| release-mainline.yml | Tag push | 🔴 CRITICAL | No approval before GitHub Release publish |
| release-docker-image.yml | workflow_dispatch | 🔴 CRITICAL | Defaults to push=true, no tracking |
| release-winget.yml | release event | 🔴 CRITICAL | Automatic external registry push, no approval |
| release-linux-distribution.yml | workflow_call | 🟡 MEDIUM | Manual but no tracking/approval |
| release-windows-distribution.yml | workflow_call | 🟡 MEDIUM | Manual but no tracking/approval |
| release-changelog.yml | workflow_dispatch | 🟢 LOW | Good - manual only |

## Critical Issues

### 1. GitHub Release Publishing (release-mainline.yml)
**Risk:** Automatic publish on tag push with no approval
**Flow:**
1. Tag push → release-mainline.yml triggered
2. Automatic build, validate, package, publish
3. Anyone with push access can create release

**Fix:** Implement approval gate between validation and publish

### 2. WinGet Distribution (release-winget.yml)
**Risk:** Automatic external registry submission on release event
**Flow:**
1. GitHub Release published (from release-mainline.yml)
2. WinGet workflow triggered automatically
3. Creates PR in external winget-pkgs fork
4. No validation/approval tracking

**Fix:** Create tracking issue + approval gate before WinGet PR

### 3. Docker Distribution (release-docker-image.yml)
**Risk:** Defaults to pushing to docker.io without approval
**Flow:**
1. workflow_dispatch with push_to_registry=true (default!)
2. No explicit approval mechanism
3. Can push to public docker.io registry

**Fix:** Change default to false + create approval gate

## Recommended Implementation

### Phase 1: Critical Fixes
1. Create `release-winget-approval.yml` (similar to wiki-publish-from-issue.yml)
2. Change Docker default: `push_to_registry: false`
3. Create `release-mainline-approval.yml` for GitHub Release publish gate

### Phase 2: Governance Enhancements
4. Add tracking issues for Linux/Windows distribution publishes
5. Create approval workflows for distro publishing

### Phase 3: Documentation
6. Document publish approval keywords and workflows
7. Update RELEASE_STRATEGY.md with new approval gates

## Pattern for All Approval Workflows

Based on successful implementation of `wiki-publish-from-issue.yml`:

1. **Trigger:** `issue_comment` on tracking issue
2. **Keywords:** `/publish`, `/approve`, `/publish-<registry>`
3. **Permission Check:** Maintainer-only (admin/maintain roles)
4. **Action:** Rebuild/validate with fresh inputs
5. **Execution:** Atomic push to registry
6. **Tracking:** Comment results on issue, close on success

## Files to be Created/Modified

### New Files (Approval Workflows)
- `.github/workflows/release-winget-approval.yml` (~250 lines)
- `.github/workflows/release-docker-approval.yml` (~250 lines)
- `.github/workflows/release-mainline-approval.yml` (~300 lines)

### Files to Modify
- `.github/workflows/release-docker-image.yml` - Change default
- `.github/workflows/release-mainline.yml` - Add tracking issue job
- `RELEASE_STRATEGY.md` - Document new approval gates
- `docs/governance/RELEASE_GOVERNANCE.md` (new) - Approval keywords

## Next Steps

**User Action Required:**
1. Review this audit findings
2. Decide on remediation priority (Phase 1/2/3)
3. Approve implementation plan

**Estimated Effort:** 13-18 hours for full Phase 1+2 remediation

---
**Audit Date:** 2026-09-23  
**Auditor:** Copilot SWE Agent  
**Status:** Ready for implementation
