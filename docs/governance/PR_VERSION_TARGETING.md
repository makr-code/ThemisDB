# PR Version Targeting Governance

> **Status:** Active  
> **Applies to:** All pull requests to ThemisDB  
> **Last Updated:** 2026-08-18

---

## Overview

Every pull request (PR) to ThemisDB must declare a **target version** — the planned release version in which the PR's changes will be shipped. This governance policy ensures that:

1. **Release planning is precise**: each PR is mapped to a specific milestone and release
2. **Changelog accuracy**: `CHANGELOG.md` entries can be generated from PR metadata
3. **Milestone tracking**: GitHub milestones reflect the actual scope of work per release
4. **ROADMAP alignment**: feature/bugfix work is tied to the roadmap or backlog

---

## 1. PR Version Targeting Requirements

### 1.1 Mandatory Field

Every PR **MUST** fill in the **"Target Version"** field in the PR template (`.github/pull_request_template.md`).

- The field is required for PR merge
- The PR author selects from a list of active milestones (see §1.3)
- The version must follow the format defined in [VERSIONING.md](../../VERSIONING.md)

### 1.2 Valid Version Formats

The target version must match one of these patterns:

| Pattern | Example | Use case |
|---------|---------|----------|
| Stable | `v2.4.0` | Merged into `community` or released from stable branch |
| Pre-release | `v2.5.0-alpha1`, `v2.5.0-beta1`, `v2.4.0-rc1` | Pre-release milestones on `develop` |
| Backlog | `[Unreleased]` | Work not yet assigned to a specific release |
| Hotfix | `v2.4.1` | Critical patches to a stable release |

See [VERSIONING.md §1](../../VERSIONING.md#1-version-format) for full SemVer rules.

### 1.3 Active Milestones

The repository maintains the following milestone structure on GitHub:

**Stable versions** (released or in release candidate phase):
- `v2.4.0` — Current stable or pending GA promotion
- `v2.4.1` — Hotfix/patch milestone for v2.4.x line
- `v2.3.x` — Legacy maintenance (if applicable)

**Pre-release versions** (under active development on `develop`):
- `v2.5.0-alpha1` — Early preview of next minor
- `v2.5.0-beta1` — Feature-complete pre-release
- `v2.5.0-rc1` — Release candidate (pre-GA)

**Backlog**:
- `[Unreleased]` — Work queued but not yet assigned to a release

New milestones are created by the release manager when a new version is planned. See §3 for the process.

### 1.4 Selection Criteria

**Choose the target version based on:**

1. **Feature/Enhancement work:**
   - Assign to the next planned `MINOR` version (e.g., `v2.5.0-alpha1`)
   - If unsure, use `[Unreleased]` and the release manager will triage

2. **Bug fixes:**
   - If fixing a regression in the current RC/stable line → assign to that version (e.g., `v2.4.0-rc1` or `v2.4.1`)
   - If fixing a pre-existing bug in a stable release → use `v2.4.1` (patch milestone)
   - If it's a general quality fix for the next release → assign to next planned version (e.g., `v2.5.0-alpha1`)

3. **Documentation:**
   - Assign to the same version the documented feature targets
   - If documentation applies to multiple versions, choose the earliest version where the feature is available

4. **Security fixes:**
   - Assign to the current stable version first (e.g., `v2.4.0` or `v2.4.1`)
   - Backport to prior maintenance versions if applicable

5. **Refactoring / Infrastructure:**
   - Assign to the next planned `MINOR` (e.g., `v2.5.0-alpha1`)
   - Use `[Unreleased]` if it's a pure quality improvement not tied to a feature

---

## 2. PR Template Integration

The PR template (`.github/pull_request_template.md`) includes a dedicated section:

```markdown
## Target Version (Required)

<!-- Select the version this PR targets. Must be an existing GitHub milestone.
     See docs/governance/PR_VERSION_TARGETING.md for guidance.
     Valid formats: v2.4.0, v2.5.0-alpha1, v2.5.0-beta1, v2.4.0-rc1, [Unreleased] -->

**Target Version:** (choose one: v2.4.0 | v2.4.1 | v2.5.0-alpha1 | v2.5.0-beta1 | v2.5.0-rc1 | [Unreleased])

<!-- Briefly explain why this version was chosen -->
```

---

## 3. GitHub Milestone Workflow

### 3.1 Creating Milestones

**Who:** Release manager (or authorized developer)  
**When:** Before feature branch development begins for a new version  
**Process:**

1. Open GitHub → Repository → Issues → Milestones → New Milestone
2. Title: Use the version string exactly as it appears in `VERSION`/`VERSIONING.md` (e.g., `v2.5.0-alpha1`)
3. Description: Include:
   - Version and release type (stable / pre-release / patch)
   - Planned release date (if known)
   - Link to the corresponding ROADMAP section
   - Any special notes (e.g., breaking changes, feature highlights)
4. Due date: Set to approximate release date (optional but recommended)

**Example milestone description:**

```
# v2.5.0-alpha1

**Release Type:** Alpha pre-release  
**Planned Date:** 2026-09-30  
**Focus:** New Query Optimization APIs, improved sharding performance  

See [ROADMAP.md - v2.5.0 Alpha Phase](../../ROADMAP.md#v250-alpha-phase)

Breaking changes: none planned for alpha  
Known limitations: GPU kernel batching not yet implemented
```

### 3.2 Deprecating/Closing Milestones

When a version is released or superseded:

1. On GitHub, close the milestone
2. Update `docs/governance/PR_VERSION_TARGETING.md` to remove it from the "Active Milestones" list
3. If the version is released, add a note in [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) referencing the release date and tag

---

## 4. Synchronization Rules

### 4.1 PR → Milestone Binding

- The PR author sets the "Target Version" field
- GitHub automation (or manual PR review) assigns the corresponding milestone to the PR
- If the milestone does not exist, the PR is blocked until:
  - The milestone is created, OR
  - The author changes the target version to an existing milestone

### 4.2 PR ↔ CHANGELOG Sync

- When a PR is merged with a target version, the PR number and title should be added to the corresponding section in `CHANGELOG.md`
- Example entry:
  ```markdown
  ### Added
  - Query optimization API for custom index selection (#1234)
  - New `QueryOptimizer::selectIndex()` method with performance hints
  ```
- See [VERSIONING.md §7](../../VERSIONING.md#7-changelog-requirements) for full changelog format

### 4.3 Version ↔ ROADMAP Sync

- Each released version should have corresponding entries in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`
- If a PR targets a version that's documented in ROADMAP, verify that the PR scope aligns with the ROADMAP scope
- If the PR scope diverges significantly, update the ROADMAP or discuss with the maintainer

### 4.4 Backlog Triage

- PRs assigned to `[Unreleased]` are reviewed by the release manager quarterly
- Release manager assigns them to specific upcoming versions based on:
  - Feature priority and roadmap alignment
  - Risk/complexity assessment
  - Release timeline

---

## 5. Release Manager Workflow

### 5.1 Pre-Release Preparation

1. Review all open PRs targeting the release version
2. Verify that PR scope matches the planned milestone scope
3. Ensure CHANGELOG entries are complete and accurate
4. Check that no PRs are orphaned in `[Unreleased]` that should be in this release

### 5.2 Release Readiness Checklist

Before cutting a release tag:

- [ ] All PRs for this version are merged and closed
- [ ] All PR numbers and scopes are reflected in `CHANGELOG.md`
- [ ] `CHANGELOG.md` version entry is complete (Added, Changed, Fixed, Security sections)
- [ ] The milestone is closed on GitHub
- [ ] Version files are updated (`VERSION`, `CMakeLists.txt`)
- [ ] Git tag is created with proper format (e.g., `v2.4.0`)
- [ ] Release notes are published on GitHub Releases

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for full release procedures.

---

## 6. Branch Governance Integration

This policy works in conjunction with [BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md):

| Branch | PR Target Version | Milestone | Use case |
|--------|-------------------|-----------|----------|
| `develop` | `v2.5.0-alpha1`, `v2.5.0-beta1`, `[Unreleased]` | Next planned MINOR | Active feature development |
| `feature/*` | `v2.5.0-*` | Same as parent epic | Feature branch work |
| `epic/*` | `v2.5.0-*` | Next MINOR integration | Feature integration onto develop |
| `release/*` | `v2.4.0`, `v2.4.0-rc1` | Current release version | Release candidate stabilization |
| `hotfix/*` | `v2.4.1` | Patch version | Critical fixes to stable |
| `community` | `v2.4.0` | Stable version | Community edition release |
| `minimal` | `v2.4.0` | Stable version | Minimal edition release |

---

## 7. FAQ

**Q: My PR fixes a bug but I don't know which version to target.**  
A: Use `[Unreleased]` and add a comment explaining the bug. The release manager will triage it during pre-release review.

**Q: I'm working on a feature for v2.5.0 but the alpha milestone doesn't exist yet.**  
A: Create the milestone or contact the release manager. Do not proceed without an active target milestone.

**Q: Can I change a PR's target version after opening it?**  
A: Yes. Update the "Target Version" field in the PR description, and the reviewer/automation will update the milestone assignment.

**Q: What if my PR spans multiple versions (e.g., backport)?**  
A: Create separate PRs for each version. Each PR must have a single target version. (You can reference related PRs with "Related to #NNN".)

**Q: Who decides if a PR targets the right version?**  
A: The PR author proposes a version; the reviewer/maintainer validates it against ROADMAP and release scope. Disagreements are resolved via GitHub discussion.

---

## 8. Related Documents

- [VERSIONING.md](../../VERSIONING.md) — SemVer policy and version format rules
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process and branch model
- [BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md) — Git workflow and branch naming
- [ROADMAP.md](../../ROADMAP.md) — Planned features and milestones
- [FUTURE_ENHANCEMENTS.md](../../FUTURE_ENHANCEMENTS.md) — Open enhancement backlog
- [CHANGELOG.md](../../CHANGELOG.md) — Full release history
- `.github/pull_request_template.md` — PR template with version targeting field

---

**Governance Status:** Active  
**Last Reviewed:** 2026-08-18  
**Next Review:** 2026-09-18 (or after first release using this policy)
