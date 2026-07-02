# Phase 2.4 Release Candidate (RC) Branch Strategy

**Release Version**: v2.4.0  
**Strategy Owner**: Release Manager  
**Target Timeline**: July 2026  
**Status**: Planning Phase

---

## Table of Contents

1. [Overview](#overview)
2. [Branch Model](#branch-model)
3. [RC Branch Lifecycle](#rc-branch-lifecycle)
4. [Tagging Strategy](#tagging-strategy)
5. [Merge Procedures](#merge-procedures)
6. [Hotfix & Patch Procedures](#hotfix--patch-procedures)
7. [Release Artifacts](#release-artifacts)
8. [Rollback Plan](#rollback-plan)
9. [Communication Plan](#communication-plan)

---

## Overview

The RC (Release Candidate) branching strategy enables controlled testing and stabilization of v2.4.0 before production release. The strategy supports:

- **Multiple RC iterations** (v2.4.0-rc1, v2.4.0-rc2, etc.) if needed
- **Bug fix backports** from main/develop to RC branch
- **Clear separation** between development and release
- **Automated testing** on each RC tag
- **Production release** from final RC tag

### Key Principles

1. **Feature Freeze** — No new features on RC branch (bug fixes only)
2. **Stability First** — Release only when all tests pass + stability verified
3. **Minimal Changes** — Only critical fixes merged to RC
4. **Traceability** — Every change logged with PR + commit hash
5. **Automation** — CI/CD pipeline verifies every RC tag

---

## Branch Model

### Repository Branch Structure

```
main (v1.9.x — Maintenance)
  ↑ (merge on stable release)
  │
develop (next feature development)
  ↑ (create release branch from here)
  │
release/v2.4-rc1 (Release Candidate branch)
  ├── v2.4.0-rc1 (tag)
  ├── v2.4.0-rc1-patch1 (tag — critical fix)
  ├── v2.4.0-rc1-patch2 (tag — additional fix)
  ├── cherry-pick: v1.9.0-rc1 → v2.4.0-rc1 (if needed)
  └── v2.4.0 (release tag — final merge to main)
```

### Branch Naming Convention

| Branch Type | Naming Convention | Purpose | Lifecycle |
|-------------|-------------------|---------|-----------|
| **Feature** | `feature/*` | Development | Merged to `develop` |
| **Develop** | `develop` | Integration | Staging for RC |
| **RC** | `release/v2.4-rc1` | Release Candidate | 2–4 weeks |
| **Hotfix RC** | `hotfix/v2.4.0-rc1-*` | Critical fixes | Merged to RC, then release |
| **Main** | `main` | Production | Latest stable |

---

## RC Branch Lifecycle

### Phase 1: RC1 Creation (Week -1)

#### Step 1.1: Pre-RC Validation

```bash
# Ensure develop branch is stable
git checkout develop
git pull origin develop

# Run comprehensive tests
ctest --preset linux-release -R "graph" --output-on-failure
# Expected: 326/326 PASS

# Code review all pending PRs
# Expected: 0 unreviewed PRs, 0 open feedback
```

**Checklist**:
- [ ] All Phase 2.4 PRs merged and tested
- [ ] `develop` branch tests: 326/326 PASS
- [ ] No critical issues in backlog
- [ ] Feature freeze announced
- [ ] Release notes drafted

**Sign-Off**: [ ] Release Manager

#### Step 1.2: Create RC Branch

```bash
# Create and push RC branch from develop
git checkout develop
git pull origin develop
git checkout -b release/v2.4-rc1
git push origin release/v2.4-rc1

# Protect RC branch (admin settings)
# - Require 2 code review approvals
# - Dismiss stale PR approvals on commit
# - Allow only documented CI to push
```

**Verification**:
- [ ] Branch `release/v2.4-rc1` exists on GitHub
- [ ] Branch protection rules applied
- [ ] CI pipeline triggered on branch

#### Step 1.3: Create RC1 Tag

```bash
# Create signed tag on RC branch
git tag -s v2.4.0-rc1 -m "Release Candidate 1: v2.4.0-rc1" <GPG_KEY_ID>
git push origin v2.4.0-rc1

# Verify tag created
git tag -v v2.4.0-rc1
# Expected output: gpg: Signature made [date] using RSA key...
```

**Tag Format**: `v2.4.0-rc<N>` where N = 1, 2, 3, ...

**Verification**:
- [ ] Tag `v2.4.0-rc1` exists on GitHub
- [ ] Tag is GPG-signed
- [ ] CI pipeline triggered (build, tests, artifacts)

---

### Phase 2: RC Testing & Stabilization (Week 0)

#### Step 2.1: Continuous Integration Pipeline

**Automated on Every Tag/Push to RC Branch**:

```bash
# 1. Build
cmake --preset community-release
cmake --build --preset community-release --parallel 16

# 2. Run full test suite
ctest --preset linux-release -R "graph" --output-on-failure

# 3. Memory safety checks
cmake --preset asan-release
ctest --preset asan-release -R "graph"

# 4. Performance baseline
./build/community-release/bin/themis-perf-benchmark \
  --suite graph_module \
  --iterations 10000 \
  --output v2.4.0-rc1-baseline.json

# 5. Code quality gates
clang-tidy src/graph/*.cpp -- -I. | tee clang-tidy-results.txt
# Expected: 0 errors

# 6. Security scan
codeql database analyze <db> security-and-quality.qls
# Expected: 0 high-severity alerts

# 7. Generate artifacts
cmake --install build/community-release --prefix /tmp/themisdb-2.4.0-rc1
```

**Success Criteria**:
- ✅ Build succeeds without warnings
- ✅ All 326 tests PASS
- ✅ Zero sanitizer errors
- ✅ No performance regressions > 10%
- ✅ Zero code quality issues
- ✅ Zero security alerts
- ✅ Artifacts generated

#### Step 2.2: Stability Testing (100x Iterations)

**Performed During RC Stabilization**:

```bash
# Run 100 consecutive iterations
#!/bin/bash
PASSED=0
FAILED=0
for i in {1..100}; do
  ctest --preset linux-release -R "graph" --output-on-failure || {
    FAILED=$((FAILED+1))
    echo "Run $i FAILED"
    break
  }
  PASSED=$((PASSED+1))
  echo "Run $i PASSED at $(date)"
done

echo "Stability Report: $PASSED PASSED, $FAILED FAILED"
```

**Success Criteria**:
- ✅ 100/100 runs PASS
- ✅ Zero flaky tests
- ✅ Execution time variance < 5%
- ✅ No memory leaks

**Outcome**: If PASS → Proceed to release. If FAIL → Create hotfix PR.

---

### Phase 3: Bug Fix Backports (If Needed)

#### Step 3.1: Identify Issues

During RC stabilization testing, if bugs are found:

```bash
# Create GitHub issue
# Title: [RC] Bug in [component]
# Labels: rc-blocker, priority-high
# Assign to developer

# Example issue:
# Title: [RC] Graph validator infinite loop in multi-hop traversal
# Priority: CRITICAL (blocks release)
```

#### Step 3.2: Create Hotfix PR

```bash
# Developer creates hotfix branch from release/v2.4-rc1
git checkout release/v2.4-rc1
git pull origin release/v2.4-rc1
git checkout -b hotfix/v2.4.0-rc1-graph-traversal-fix

# Make fix
vi src/graph/graph_validator.cpp

# Commit with clear message
git commit -m "Fix: Prevent infinite loop in multi-hop traversal (issue #XXXXX)"
git push origin hotfix/v2.4.0-rc1-graph-traversal-fix

# Create PR to release/v2.4-rc1
# Description:
#   Fixes [issue #XXXXX]: Graph validator infinite loop
#   - Added loop counter + max iterations guard
#   - Test added: test_graph_validator_max_depth_enforced
#   - Performance impact: none
```

**Review & Approval**:
- [ ] 2+ code reviews (require both)
- [ ] All tests pass on PR
- [ ] Security review (if security-related)

#### Step 3.3: Merge Hotfix

```bash
# Merge hotfix to RC branch (fast-forward if possible)
git checkout release/v2.4-rc1
git pull origin release/v2.4-rc1
git merge hotfix/v2.4.0-rc1-graph-traversal-fix
git push origin release/v2.4-rc1

# Trigger CI pipeline
# Wait for: build, tests, performance baseline ✅

# If all pass, create RCN-patch1 tag
# If hotfix fails tests, revert and refine
```

---

## Tagging Strategy

### RC Tag Naming Convention

```
v2.4.0-rc<N>[-patch<M>]

Examples:
  v2.4.0-rc1          (Initial RC)
  v2.4.0-rc1-patch1   (Bug fix during RC1 testing)
  v2.4.0-rc1-patch2   (Additional bug fix during RC1)
  v2.4.0-rc2          (New RC iteration after major redesign)
  v2.4.0-rc2-patch1   (Bug fix during RC2)
  v2.4.0              (Final release from RC)
```

### RC Tag Progression

| Phase | Tag | Purpose | Testing Duration | Success Criteria |
|-------|-----|---------|------------------|------------------|
| **Stabilization Start** | `v2.4.0-rc1` | Initial RC release | 1 week | Basic tests pass |
| **Critical Fix #1** | `v2.4.0-rc1-patch1` | Address blocker | 1–2 days | Full suite passes |
| **Critical Fix #2** | `v2.4.0-rc1-patch2` | Address blocker | 1–2 days | Full suite passes |
| **New RC** | `v2.4.0-rc2` | Major redesign/scope change | 3–5 days | All tests + 100x stability ✅ |
| **Final Release** | `v2.4.0` | Production release | _N/A_ | All RC criteria met |

### Tag Management

```bash
# List all RC tags
git tag | grep "v2.4.0-rc"
# Output:
#   v2.4.0-rc1
#   v2.4.0-rc1-patch1
#   v2.4.0-rc2

# Delete tag (if mistake)
git tag -d v2.4.0-rc1-patch1  # Local
git push origin --delete v2.4.0-rc1-patch1  # Remote

# Verify tag GPG signature
git tag -v v2.4.0-rc1
# Expected: "Good signature from ThemisDB Release Team"
```

---

## Merge Procedures

### Scenario 1: No Major Issues Found (Ideal Path)

```
Timeline: 7–10 days

v2.4.0-rc1 Tag
  ↓ (1 week testing)
  All 326 tests pass ✅
  100x stability pass ✅
  Performance baseline OK ✅
  No critical issues ✅
  ↓
v2.4.0 Release Tag
  ↓ (merge to main)
git checkout main
git pull origin main
git merge --no-ff release/v2.4-rc1 -m "Release v2.4.0"
git push origin main
git tag v2.4.0 && git push origin v2.4.0
```

**Duration**: 1 week

---

### Scenario 2: Minor Bugs Found (Typical Path)

```
Timeline: 10–14 days

v2.4.0-rc1 Tag
  ↓ (3 days testing)
  Bug #1 found: Graph traversal
  ↓
v2.4.0-rc1-patch1 Tag
  ↓ (2 days testing)
  Bug #2 found: Cache invalidation
  ↓
v2.4.0-rc1-patch2 Tag
  ↓ (3 days testing)
  All tests pass ✅
  100x stability pass ✅
  No blockers ✅
  ↓
v2.4.0 Release Tag
  ↓ (merge to main)
```

**Duration**: 2 weeks

---

### Scenario 3: Major Issues Found (Redesign Path)

```
Timeline: 21+ days

v2.4.0-rc1 Tag
  ↓ (3 days testing)
  Critical architecture issue found
  ↓ (requires design change)
Rollback RC1, redesign component
  ↓
v2.4.0-rc2 Tag (new branch iteration)
  ↓ (7 days testing)
  All tests pass ✅
  100x stability pass ✅
  ↓
v2.4.0 Release Tag
```

**Duration**: 3 weeks

---

## Hotfix & Patch Procedures

### During RC Phase: Bug Found → Hotfix PR Flow

```
1. Developer identifies bug during RC testing
   ↓
2. Create hotfix branch: hotfix/v2.4.0-rc1-<fix-name>
   ↓
3. Create PR to release/v2.4-rc1
   ├─ Title: [RC-Hotfix] <Issue>
   ├─ Reviewers: 2+ approvals required
   ├─ Tests: All pass
   └─ Artifacts: Regenerated
   ↓
4. If tests pass:
   ├─ Merge to release/v2.4-rc1
   ├─ Create tag: v2.4.0-rc1-patch<N>
   ├─ Trigger full CI pipeline
   └─ Continue testing
   ↓
5. If tests fail:
   ├─ Revert merge
   ├─ Request fixes on hotfix PR
   └─ Repeat testing
```

### Post-Release: Critical Bug Found → v2.4.1 Hotfix

```
Released: v2.4.0 on main
  ↓ (production use)
  Critical bug discovered
  ↓
1. Create hotfix branch: hotfix/v2.4.1-<fix-name>
   Branch from: main (not develop)
   ↓
2. Create PR to main
   ├─ Title: [Hotfix] <Issue>
   ├─ Reviewers: 2+ approvals (strict)
   ├─ Tests: Full suite passes
   └─ Security review: Required if security-related
   ↓
3. Merge to main, create tag v2.4.1
   ↓
4. Backport to develop (if still on Phase 2 development)
   Create PR: develop ← hotfix/v2.4.1-*
```

---

## Release Artifacts

### Artifact Generation on Each RC Tag

**Automated via CI/CD Pipeline**:

```
v2.4.0-rc1 tag pushed
  ↓
GitHub Actions triggered
  ├─ Build
  │  ├─ Community edition: themisdb-2.4.0-rc1-community-binary-x64
  │  ├─ Minimal edition: themisdb-2.4.0-rc1-minimal-binary-x64
  │  └─ Enterprise (if branch): themisdb-2.4.0-rc1-enterprise-binary-x64
  │
  ├─ Docker images
  │  ├─ themisdb/themisdb:2.4.0-rc1-community
  │  └─ themisdb/themisdb-minimal:2.4.0-rc1-minimal
  │
  ├─ Documentation
  │  ├─ Doxygen HTML: docs/html/2.4.0-rc1/
  │  ├─ Release notes: PHASE_2_4_RELEASE_NOTES.md
  │  └─ Migration guide: docs/migration/v1.9-to-v2.4.md
  │
  ├─ Test artifacts
  │  ├─ Test results: test-results-2.4.0-rc1.xml
  │  ├─ Coverage report: coverage-2.4.0-rc1.html
  │  └─ Performance baseline: benchmark-2.4.0-rc1.json
  │
  ├─ Security artifacts
  │  ├─ SBOM (CycloneDX): sbom-2.4.0-rc1.xml
  │  ├─ CodeQL results: codeql-2.4.0-rc1.sarif
  │  └─ Dependency report: dependencies-2.4.0-rc1.json
  │
  └─ Checksums
     ├─ SHA-256: checksums-2.4.0-rc1.txt
     └─ GPG signature: checksums-2.4.0-rc1.txt.asc
```

### Artifact Storage

| Artifact | Storage Location | Access |
|----------|------------------|--------|
| **Binaries** | GitHub Releases → v2.4.0-rc1 | Public |
| **Docker images** | Docker Hub (community), Private Registry (enterprise) | Public/Private |
| **Documentation** | docs.themisdb.io/v2.4.0-rc1 | Public |
| **Test results** | GitHub Actions artifacts | Public (workflow URL) |
| **Coverage reports** | Codecov.io / GitHub Pages | Public |
| **Security artifacts** | GitHub Security tab + private S3 | Internal |

---

## Rollback Plan

### Scenario: RC1 Critical Issue → Rollback to v1.9.x

```
v2.4.0-rc1 in production (deployed to canary)
  ↓ (30 min later)
  Critical bug causes 50% error rate
  ↓ (decision: ROLLBACK)

Immediate Actions:
  1. Trigger incident response
  2. Stop v2.4.0-rc1 deployment (stop canary traffic)
  3. Redirect traffic back to v1.9.x baseline
  4. Create incident ticket
  5. Notify stakeholders

Rollback Command:
  kubectl set image deployment/themisdb \
    themisdb=themisdb/themisdb:1.9.0-community-binary-x64

Verification:
  kubectl rollout status deployment/themisdb
  curl https://api.themisdb.io/health
  # Expected: 200 OK, error rate < 0.1%

Post-Rollback:
  1. Investigate root cause
  2. Create fix PR to release/v2.4-rc1
  3. Create v2.4.0-rc1-patch1 tag
  4. Re-test (full cycle: 48 hours)
  5. If safe, redeploy v2.4.0-rc1-patch1
```

### Rollback Criteria

| Trigger | Action | Timeline |
|---------|--------|----------|
| Error rate > 1% for 5 min | Automatic canary halt | Immediate |
| p99 latency spike > 50% | Alert + manual review | 5 min |
| Data corruption detected | Immediate rollback | Immediate |
| Security vulnerability found | Immediate rollback + investigation | Immediate |
| OOM/resource exhaustion | Rollback + capacity investigation | 5 min |

---

## Communication Plan

### Week -1: Pre-RC Announcement

**Audience**: Internal team, partners, stakeholders

```
Subject: Phase 2.4 Release Candidate — v2.4.0-rc1 Coming Week of [date]

Overview:
  - Graph Truth Validation Layer now complete
  - All Phase 2.1–2.3 gap fixes integrated
  - 326 tests verified, 100x stability validated
  - Feature freeze in effect

Timeline:
  - RC1 tag: [date]
  - RC testing window: [date] – [date] (1–2 weeks)
  - Production release: [date] (target)

Expectations:
  - Canary deployment to 5% traffic (48 hours)
  - Monitor for errors, performance regressions
  - Rapid rollback if critical issues found

Call to Action:
  - Run v2.4.0-rc1 in dev/test environments
  - Report issues to [support email]
```

### Week 0: RC Testing Communication

**Daily standup during RC stabilization**:

```
Status: RC1 Testing Day 3/7
  ✅ Build: Success
  ✅ Tests: 326/326 PASS (100x runs: 45/100 completed)
  ✅ Performance: Within 5% of baseline
  ⚠️ Found: Graph cache invalidation edge case
     PR #5601 in review, targeting fix by EOD tomorrow
```

### Week 1: Release Announcement

**Upon v2.4.0 final release**:

```
Subject: ThemisDB v2.4.0 — Now Available!

Release Highlights:
  ✅ Graph Truth Validation Layer (secure ACL + multi-hop)
  ✅ Performance +15–25% improvement
  ✅ 326 comprehensive tests
  ✅ Production-ready stability

What's New:
  - GraphTruthValidator fail-closed by default
  - Multi-hop BFS traversal with confidence scoring
  - Exception safety hardening (RAII, smart pointers)
  - Performance optimization (move semantics, caching)

Upgrade Instructions:
  - Backward compatible with v1.9.x
  - No data migration required
  - See PHASE_2_4_RELEASE_NOTES_DRAFT.md for details

Download:
  - Binaries: https://github.com/themisdb/themisdb/releases/tag/v2.4.0
  - Docker: docker pull themisdb/themisdb:2.4.0-community-binary-x64
  - Homebrew: brew install themisdb@2.4 (if applicable)

Support:
  - Docs: https://docs.themisdb.io/v2.4.0
  - Issues: https://github.com/themisdb/themisdb/issues
  - Forum: https://forum.themisdb.io/
```

---

## RC Decision Matrix

### When to Proceed to Release

| Criterion | Pass | Fail | Decision |
|-----------|------|------|----------|
| **All 326 tests pass** | ✅ | ❌ | Blocker |
| **100x stability** | ✅ | ❌ | Blocker |
| **Performance baseline** | ✅ | ❌ | Blocker |
| **Security scan** | ✅ (0 critical) | ❌ (1+ critical) | Blocker |
| **Code quality gate** | ✅ (0 errors) | ❌ (1+ errors) | Blocker |
| **Documentation complete** | ✅ | ❌ | Blocker |
| **Upgrade path verified** | ✅ | ❌ | Blocker |
| **Rollback tested** | ✅ | ❌ | Blocker |

### RC Progression Decision Tree

```
v2.4.0-rc1 tests pass? ❌
  └─ Create hotfix PR → v2.4.0-rc1-patch1

v2.4.0-rc1-patch1 tests pass? ❌
  └─ Create hotfix PR → v2.4.0-rc1-patch2

v2.4.0-rc1-patch2 tests pass? ✅
  ├─ 100x stability pass? ✅
  │   ├─ Performance baseline pass? ✅
  │   │   ├─ Security scan pass? ✅
  │   │   │   ├─ Documentation complete? ✅
  │   │   │   │   ├─ Upgrade path verified? ✅
  │   │   │   │   │   └─ ✅ PROCEED TO v2.4.0 RELEASE
  │   │   │   │   └─ ❌ Complete documentation
  │   │   │   └─ ❌ Fix security alerts
  │   │   └─ ❌ Investigate performance regression
  │   └─ ❌ Re-run 100x stability
  └─ ❌ Create v2.4.0-rc2 (major redesign)
```

---

## Template: RC Status Report

**Fill out daily during RC testing**:

```markdown
## RC Status Report — v2.4.0-rc1

**Date**: [YYYY-MM-DD]  
**Reporting**: [Name]  
**Status**: 🟢 ON TRACK | 🟡 MINOR ISSUES | 🔴 BLOCKED

### Build Status
- [ ] Community edition: ✅ PASS
- [ ] Minimal edition: ✅ PASS
- [ ] Enterprise edition: ✅ PASS (if applicable)

### Test Results
- Unit tests: 156/156 PASS
- Integration tests: 87/87 PASS
- Performance tests: 51/51 PASS
- Determinism tests: 32/32 PASS
- **Total: 326/326 PASS** ✅

### Stability Verification
- Iteration: 45/100 complete
- Pass rate: 100%
- Flaky tests: 0
- ETA: [date] (remaining 55 runs × 5 min each)

### Performance Metrics
- Single-hop p99: 0.8ms (baseline: 1.2ms) — ✅ +33% improvement
- 5-hop p99: 42ms (baseline: 60ms) — ✅ +30% improvement
- Memory overhead: +2% (baseline: +3%) — ✅ Within threshold

### Known Issues
- [Optional] Graph cache invalidation edge case
  - PR #5601: In review, targeting fix EOD [date]
  - Impact: LOW (affects rarely-used code path)
  - Blocker: NO

### Sign-Off
- [ ] QA Lead: Approve for continued testing
- [ ] Release Manager: Approve for next milestone

**Next Update**: [next date/time]
```

---

## Summary

### RC Branch Strategy at a Glance

| Phase | Duration | Key Activities | Success Criteria |
|-------|----------|-----------------|------------------|
| **RC1 Creation** | 1 day | Create branch, tag, CI pipeline | v2.4.0-rc1 tag exists |
| **Stabilization** | 7–14 days | Testing, bug fixes, backports | 326/326 tests PASS, 100x stable |
| **Release Prep** | 1–2 days | Artifacts, documentation, final checks | All artifacts ready |
| **Release** | 1 day | Final tag v2.4.0, merge to main | Production deployment complete |

### Key Files

- **Release Notes**: `PHASE_2_4_RELEASE_NOTES_DRAFT.md`
- **Release Checklist**: `PHASE_2_4_RELEASE_CHECKLIST.md`
- **Stability Test Plan**: `PHASE_2_4_STABILITY_TEST_PLAN.md`

---

*Last Updated: 2026-07-01*  
*Approved by: [Release Manager]*  
*Next Review: v2.5.0 Release Cycle*
