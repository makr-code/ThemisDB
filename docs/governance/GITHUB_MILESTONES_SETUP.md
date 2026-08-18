# GitHub Milestones Setup & Release Manager Checklist

> **Purpose:** Reference guide for creating and managing GitHub milestones aligned with version targeting policy  
> **Audience:** Release managers, maintainers  
> **Status:** Active (2026-08-18)

---

## 1. Recommended Milestone Structure

Based on the version targeting policy (`docs/governance/PR_VERSION_TARGETING.md`), create the following milestones on GitHub:

### 1.1 Active Development Milestones

These milestones are for PRs currently under development on `develop`:

| Milestone | Type | Description |
|-----------|------|-------------|
| `v2.5.0-alpha1` | Pre-release | Early preview of next minor release |
| `v2.5.0-beta1` | Pre-release | Feature-complete, stabilizing |
| `v2.5.0-rc1` | Pre-release | Release candidate, feature frozen |

### 1.2 Stable/Release Milestones

These milestones are for released versions or in-progress release candidates:

| Milestone | Type | Description |
|-----------|------|-------------|
| `v2.4.0` | Stable | Current stable or pending GA promotion |
| `v2.4.1` | Patch | Hotfix/patch milestone for v2.4.x line |
| `v2.3.x` | Legacy | Previous stable (if applicable) |

### 1.3 Backlog Milestone

| Milestone | Type | Description |
|-----------|------|-------------|
| `[Unreleased]` | Backlog | Work not yet assigned to a specific release; triaged quarterly by RM |

---

## 2. Step-by-Step Milestone Creation

### 2.1 Prerequisites

- GitHub repository access with permissions to manage milestones
- Planned release dates (optional but recommended)
- Linked ROADMAP section for each version

### 2.2 Creating a Milestone

**Via GitHub UI:**

1. Go to **Issues → Milestones → New Milestone**
2. Fill in the form:

   | Field | Example Value |
   |-------|----------------|
   | **Title** | `v2.5.0-alpha1` |
   | **Description** | (see 2.3 below) |
   | **Due Date** | 2026-09-30 (optional) |

3. Click **Create Milestone**

**Via GitHub CLI (if available):**

```bash
gh milestone create v2.5.0-alpha1 \
  --description "Alpha pre-release of v2.5.0. See ROADMAP.md#v250" \
  --due-date "2026-09-30"
```

### 2.3 Recommended Milestone Descriptions

Use this template when creating each milestone:

#### For Pre-Release Versions (alpha/beta/rc)

```markdown
# v2.5.0-alpha1

**Release Type:** Alpha pre-release  
**Branch:** develop  
**Planned Date:** 2026-09-30  

## Scope
See [ROADMAP.md — v2.5.0 Alpha Phase](../../ROADMAP.md#v250-alpha)

Focus areas:
- Query optimization APIs
- Improved sharding performance
- Enhanced GPU kernel batching

## Known Limitations
- GPU kernel batching: planned for beta
- Distributed cache consistency: Phase 5 work

## Breaking Changes
None planned for alpha.

## More Info
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md)
- [VERSIONING.md](../../VERSIONING.md)
- [PR Version Targeting Policy](../../docs/governance/PR_VERSION_TARGETING.md)
```

#### For Stable Versions

```markdown
# v2.4.0

**Release Type:** Stable / General Availability  
**Branch:** community  
**Release Date:** 2026-08-25  

## Scope
See [ROADMAP.md — v2.4.0 Release](../../ROADMAP.md#v240)

Major features:
- User storage encryption
- LLM plugin improvements
- Sharding Phase 6 completion

## Release Notes
[GitHub Releases — v2.4.0](https://github.com/makr-code/ThemisDB/releases/tag/v2.4.0)

## Support
- [SUPPORT.md](../../SUPPORT.md)
- [Security Patching Policy](../../SECURITY.md)
```

#### For Patch/Hotfix Versions

```markdown
# v2.4.1

**Release Type:** Patch / Hotfix  
**Branch:** hotfix/v2.4.1  
**Purpose:** Critical bug fixes and security patches for v2.4.0

## Scope
High-priority fixes to v2.4.0 only. No new features.

## Example Issues
- Critical: Transaction timeout regression
- Security: Input validation bypass

See linked PRs and issues for details.
```

#### For Backlog

```markdown
# [Unreleased]

**Status:** Backlog (not yet assigned to a release)  

Work in this milestone is triaged quarterly by the release manager:
- Assessed for priority and roadmap fit
- Assigned to upcoming releases (v2.5.0-alpha1, v2.6.0, etc.)
- Or deprioritized/closed if no longer relevant

See [PR Version Targeting Policy](../../docs/governance/PR_VERSION_TARGETING.md#54-backlog-triage) for triage process.
```

---

## 3. Pre-Release Checklist

Use this checklist when preparing to release a version:

### 3.1 Scope Validation (2 weeks before release)

- [ ] All planned features for this version are in the milestone
- [ ] Milestone description matches ROADMAP scope
- [ ] No critical issues remain unresolved
- [ ] CHANGELOG.md section is started or drafted

### 3.2 PR Review (1 week before release)

- [ ] All open PRs targeting this version are reviewed
- [ ] PR scopes align with planned features
- [ ] No PRs are orphaned in `[Unreleased]` that should be in this release
- [ ] CHANGELOG entries are drafted for each PR

### 3.3 Final Release (24-48 hours before release)

- [ ] All PRs for this version are merged and closed
- [ ] Milestone is marked "closed" on GitHub
- [ ] CHANGELOG.md is complete and accurate:
  - [ ] Added
  - [ ] Changed
  - [ ] Fixed
  - [ ] Security
  - [ ] Deprecated (if any)
  - [ ] Removed (if any)
- [ ] VERSION file is updated to match release version
- [ ] CMakeLists.txt version is updated
- [ ] Git tag is created: `git tag v2.4.0`
- [ ] Release notes are published on GitHub Releases

### 3.4 Post-Release

- [ ] Close the milestone on GitHub
- [ ] Update `docs/governance/PR_VERSION_TARGETING.md` to remove the closed version from "Active Milestones"
- [ ] Create next version milestone(s) if not already done
- [ ] Announce release on GitHub Discussions

---

## 4. Quarterly Backlog Triage

Run this process every quarter to clear the `[Unreleased]` backlog:

### Process

1. **List all PRs in `[Unreleased]`**
   - Filter by milestone on GitHub: Issues → Milestones → [Unreleased]

2. **For each PR, decide:**
   - Assign to an upcoming release (v2.5.0-alpha1, v2.6.0, etc.)
   - Keep in backlog if priority is low
   - Close if no longer relevant

3. **Validation:**
   - New assignments must align with ROADMAP
   - High-priority items must have issues/tracking
   - Ensure no stale PRs remain

4. **Update metrics:**
   - Record the triage date in a comment on this document
   - Log decisions in `ai_working/BACKLOG_TRIAGE_LOG_QUARTERLY.md`

---

## 5. Active Milestone Status (2026-08-18)

**Last Updated:** 2026-08-18  
**Current Version:** 0.0.47 (production version; roadmap uses v2.4.0+)

### Suggested Current Milestones to Create

Based on VERSIONING.md and RELEASE_STRATEGY.md references, create these milestones:

- [ ] `v2.4.0` — Current stable or pending GA promotion
- [ ] `v2.4.1` — Hotfix/patch milestone
- [ ] `v2.5.0-alpha1` — Next alpha development
- [ ] `v2.5.0-beta1` — Next beta phase (if applicable)
- [ ] `[Unreleased]` — Backlog (if not already present)

### Milestone Refresh Schedule

- **Monthly:** Review open PRs against milestones
- **Quarterly:** Triage backlog
- **Before each release:** Validate scope alignment with ROADMAP
- **After each release:** Close milestone and create next version

---

## 6. Integration with CI/CD

Currently, version targeting is **manual**. Future enhancements (Block 2):

- [ ] GitHub Actions workflow to validate PR milestone assignment
- [ ] Automated CHANGELOG.md entry creation from PR metadata
- [ ] Pre-merge check: ensure PR has a valid target version milestone

See [docs/governance/PR_VERSION_TARGETING.md §3](PR_VERSION_TARGETING.md#3-github-milestone-workflow) for planned automation.

---

## 7. Related Documents

- [docs/governance/PR_VERSION_TARGETING.md](PR_VERSION_TARGETING.md) — Full PR version targeting policy
- [VERSIONING.md](../../VERSIONING.md) — Version format and SemVer rules
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process and gates
- [ROADMAP.md](../../ROADMAP.md) — Planned features and timeline
- [CHANGELOG.md](../../CHANGELOG.md) — Full release history

---

**Governance Status:** Active  
**Next Review:** 2026-09-18 (post-first-release)
