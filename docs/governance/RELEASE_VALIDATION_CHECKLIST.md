# Release Manager Pre-Release Validation Checklist

> **Purpose:** Comprehensive checklist for release managers to validate readiness before releasing a version  
> **Audience:** Release managers, maintainers  
> **Frequency:** Run before each release (RC, stable, patch)  
> **Status:** Active (2026-08-18)

---

## Template Usage

For each release, create a new section below with the version number and date. Copy the checklist and fill it in.

---

## v2.4.0 Release Checklist (Template)

**Release Manager:** [Name]  
**Release Date (Planned):** [YYYY-MM-DD]  
**Release Date (Actual):** [YYYY-MM-DD] (fill after tag creation)  
**Status:** [PLANNED | IN PROGRESS | COMPLETE | ROLLED BACK]

### 1. Scope Validation (Start: 2 weeks before release)

#### 1.1 Milestone & ROADMAP Alignment

- [ ] Milestone `v2.4.0` exists on GitHub
- [ ] Milestone description matches ROADMAP scope
  - Reference: [ROADMAP.md — v2.4.0 Release](../../ROADMAP.md#v240)
  - Verify: all planned features are accounted for
- [ ] ROADMAP "Current Status" section is up-to-date for this version
- [ ] No scope creep: all PRs in milestone align with planned features

#### 1.2 Critical Issue Resolution

- [ ] All CRITICAL issues for this version are resolved or explicitly deferred
  - Deferred issues must have tracking ticket and documented rationale
- [ ] All HIGH issues targeted for this version are resolved or have mitigation plan
- [ ] No regressions introduced in the last 2 weeks of development
  - Verified by: Wave 7 test results, release_critical CI status

#### 1.3 Feature Completeness

- [ ] All "In Progress" items in ROADMAP for this version are DONE
- [ ] All "Planned" items for this version are either DONE or moved to next version
- [ ] No unfinished Phase 1-6 deliverables for committed modules
  - Example: if Sharding Phase 6 is in roadmap, verify PHASE_6_ACCEPTANCE_CHECKLIST is signed off

### 2. PR & Changelog Review (Start: 1 week before release)

#### 2.1 PR Milestone Audit

- [ ] All open PRs targeting this version have been reviewed
- [ ] All PRs merged to `develop` targeting this version are in the milestone
- [ ] No orphaned PRs in `[Unreleased]` that should be in this version
- [ ] PR count in milestone matches expected scope (document expected count: ___)

#### 2.2 Changelog Accuracy

- [ ] `CHANGELOG.md` entry for this version is COMPLETE with sections:
  - [ ] `### Added` (new features with #PR references)
  - [ ] `### Changed` (breaking/significant changes)
  - [ ] `### Deprecated` (if any)
  - [ ] `### Removed` (if any)
  - [ ] `### Fixed` (bug fixes)
  - [ ] `### Security` (security patches)
- [ ] All PR titles in CHANGELOG are descriptive and user-friendly
- [ ] Breaking changes section is complete with migration guidance links
- [ ] Release date placeholder is filled in: `## [2.4.0] - YYYY-MM-DD`
- [ ] Changelog passes linting (if applicable)

#### 2.3 PR Scope Alignment

- [ ] Each PR in milestone has a clear and documented scope
- [ ] No PRs have scope that conflicts with other PRs
- [ ] Documentation PRs cover all new features/changes

### 3. Testing & Quality Gates (Start: 48 hours before release)

#### 3.1 CI/CD Status

- [ ] `release_critical` workflow is GREEN on `develop` for latest commit
- [ ] Wave 7 tests PASS on current baseline (all 6 gate categories)
  - [ ] correctness
  - [ ] performance
  - [ ] distributed_consistency
  - [ ] concurrency
  - [ ] fault_tolerance
  - [ ] security
- [ ] No new CRITICAL or HIGH scanner findings in target modules:
  - [ ] server
  - [ ] llm
  - [ ] sharding

#### 3.2 Module Maturity Verification

- [ ] For each committed Phase 6 module, verify acceptance checklist is signed:
  - Module: _________ Acceptance: PASS / FAIL
  - Module: _________ Acceptance: PASS / FAIL
  - Module: _________ Acceptance: PASS / FAIL
  - (Example modules: Process, Updates, Sharding, LLM)
- [ ] All Phase 1-6 exit gates met for committed modules (see ROADMAP)

#### 3.3 Performance & Benchmarks

- [ ] Benchmark suite runs successfully on the release branch
- [ ] No performance regressions > 5% vs. previous release
  - Critical paths checked: query_execution, transaction_throughput, shard_rebalancing
- [ ] Benchmark results are recorded and available at: [path to results]

#### 3.4 Security Validation

- [ ] Security checklist passed (SECURITY.md review):
  - [ ] No new secrets/credentials in source
  - [ ] SBOM (Software Bill of Materials) generated and reviewed
  - [ ] Dependency vulnerabilities checked (no critical/high unmitigated)
  - [ ] Security scanners (CodeQL, etc.) pass with no new CRITICAL findings
- [ ] If security patches included, verify they do not introduce regressions

### 4. Documentation & Release Notes (Start: 48 hours before release)

#### 4.1 Release Notes (GitHub Releases)

- [ ] Release notes are drafted and ready to publish
- [ ] Format follows previous release notes (e.g., v2.3.0)
- [ ] Sections included:
  - [ ] Overview/Summary
  - [ ] Major Features (from CHANGELOG Added section)
  - [ ] Breaking Changes (with migration guide links)
  - [ ] Bug Fixes (summary of Fixed section)
  - [ ] Security Updates (from Security section)
  - [ ] Known Issues & Limitations
  - [ ] Upgrade Guide (link to docs/migration/ if major version)
  - [ ] Contributors (optional)
- [ ] Links are tested and working (ROADMAP, docs, migration guides)

#### 4.2 API & User-Facing Documentation

- [ ] Doxygen API docs generated and reviewed for new/changed APIs
- [ ] README.md is up-to-date for this version
- [ ] Migration guide exists in `docs/migration/` if MAJOR version bump
- [ ] Examples in `examples/` are updated for new features
- [ ] Known issues documented in release notes and/or issue tracker

#### 4.3 Operational Documentation

- [ ] Runbooks exist for new operational features
- [ ] SLA targets documented (if applicable)
- [ ] Deployment guide is current for this version
- [ ] Health check procedures documented

### 5. Version File Updates (Start: 24 hours before release)

#### 5.1 Version Identifiers

- [ ] `VERSION` file updated to release version (e.g., `2.4.0`)
- [ ] `CMakeLists.txt` project version matches `VERSION` file
- [ ] `RELEASE_TYPE` file (if present) updated to `stable` / `rc` / etc.
- [ ] Git commit created for version bumps: message format is descriptive

#### 5.2 Branch & Tag Preparation

- [ ] Release branch created (if applicable, e.g., `release/v2.4.0`)
- [ ] Git tag name ready: `v2.4.0` (follows pattern from VERSIONING.md)
- [ ] Annotation for git tag prepared (include release date, features summary)
- [ ] Tag command ready to execute: `git tag -a v2.4.0 -m "..."`

### 6. Edition-Specific Validation (if applicable)

- [ ] Community edition builds successfully
- [ ] Minimal edition builds successfully (if maintaining separate build)
- [ ] Enterprise/Private editions (if applicable) are coordinated
- [ ] Docker images build and publish successfully
- [ ] Package generation (DEB/RPM/MSI) succeeds

### 7. Stakeholder Sign-Off (if required)

- [ ] Security maintainer approval received (if security patches included)
- [ ] Module owners sign-off on Phase 6 acceptance (if applicable)
- [ ] Lead maintainer approval to proceed with release
  - Approval method: GitHub comment, explicit email, meeting decision
  - Approval timestamp: [record here]

### 8. Release Execution (24-48 hours before release)

#### 8.1 Pre-Release Validation

- [ ] All checklist items above are COMPLETE
- [ ] No last-minute PRs merged without review (avoid scope creep)
- [ ] Branch is clean: no uncommitted changes
- [ ] Latest `develop` branch is verified

#### 8.2 Release Candidate (if applicable)

For RC releases only:
- [ ] Create RC tag: `v2.4.0-rc1`
- [ ] Publish RC release on GitHub
- [ ] Announce RC to testers/stakeholders
- [ ] Set RC due date: [date for final RC window]
- [ ] Define RC exit criteria (e.g., "0 critical bugs, 2 weeks without regression")

#### 8.3 Stable Release

For stable releases only:
- [ ] Confirm RC testing period is complete (if coming from RC)
- [ ] No critical issues found during RC
- [ ] Create stable tag: `v2.4.0`
- [ ] Publish release on GitHub (copy from CHANGELOG.md)
- [ ] Announce release on GitHub Discussions
- [ ] Update website/documentation homepage if applicable
- [ ] Close milestone on GitHub

### 9. Post-Release (immediately after release)

#### 9.1 Release Documentation

- [ ] Release tag exists and is signed (if using signed tags)
- [ ] GitHub Releases page has full release notes
- [ ] CHANGELOG.md entry shows correct release date
- [ ] Release is listed in SUPPORT.md under "Supported Versions"

#### 9.2 Next Version Preparation

- [ ] Next version milestone(s) created (e.g., `v2.5.0-alpha1`, `v2.4.1`)
- [ ] `docs/governance/PR_VERSION_TARGETING.md` is updated with new active milestones
- [ ] Backlog triage scheduled (if needed for `[Unreleased]`)
- [ ] ROADMAP.md is updated with next phase planning (optional, can be async)

#### 9.3 Monitoring & Rollback Readiness

- [ ] Release is announced and monitoring begins
- [ ] Rollback plan is documented and communicated (link: ____)
- [ ] Issue tracker is monitored for regressions
- [ ] Hotfix branch readiness: if critical issue found, process is:
  1. Create branch from release tag: `git checkout -b hotfix/v2.4.1 v2.4.0`
  2. Cherry-pick critical fixes
  3. Tag as: `v2.4.1`
  4. Update CHANGELOG.md with patch notes

### 10. Rollback Checklist (if release needs to be rolled back)

- [ ] Root cause documented in issue/comment
- [ ] Decision to rollback approved by maintainers
- [ ] Previous stable release is restored (e.g., `v2.3.0`)
- [ ] Announcement published about rollback
- [ ] Post-mortem scheduled to analyze what went wrong
- [ ] Blocking issues fixed before reattempting release

---

## Sign-Off

**Release Manager:** ___________________________  
**Date:** ___________________________  
**Status:** ✓ APPROVED / ✗ BLOCKED (reason: _______________)  

**Lead Maintainer Approval:**  
Name: ___________________________  
Date: ___________________________  
Approval Method: ___________________________  

---

## Quick Reference Links

- [VERSIONING.md](../../VERSIONING.md) — Version format rules
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process gates
- [CHANGELOG.md](../../CHANGELOG.md) — Changelog format and examples
- [ROADMAP.md](../../ROADMAP.md) — Release planning and scope
- [docs/governance/PR_VERSION_TARGETING.md](PR_VERSION_TARGETING.md) — PR version targeting policy
- [docs/governance/GITHUB_MILESTONES_SETUP.md](GITHUB_MILESTONES_SETUP.md) — Milestone management
- [SUPPORT.md](../../SUPPORT.md) — Supported versions policy
- [SOP.md](../../SOP.md) — Release step-by-step procedures (if exists)

---

**This checklist was created:** 2026-08-18  
**Last reviewed:** [date]  
**Applicable to:** All releases (RC, stable, patch)
