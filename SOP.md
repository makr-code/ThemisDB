# ThemisDB — Standard Operating Procedures (SOP)

> **Version:** 1.0  
> **Status:** Active  
> **Owner:** Project Lead + delegated maintainers (see MAINTAINERS.md)  
> **Last Updated:** 2026-05-13

This document defines the standard operating procedures for recurring operational tasks in the ThemisDB project. Each SOP is self-contained and links to related documents where applicable.

---

## Table of Contents

1. [SOP-01 — Stable Release](#sop-01--stable-release)
2. [SOP-02 — Hotfix / Patch Release](#sop-02--hotfix--patch-release)
3. [SOP-03 — Release Candidate Promotion](#sop-03--release-candidate-promotion)
4. [SOP-04 — Rollback a Release](#sop-04--rollback-a-release)
5. [SOP-05 — Security Vulnerability Response](#sop-05--security-vulnerability-response)
6. [SOP-06 — Dependency Update](#sop-06--dependency-update)
7. [SOP-07 — New Contributor Onboarding](#sop-07--new-contributor-onboarding)
8. [SOP-08 — Incident Response](#sop-08--incident-response)
9. [Governance Alignment and Escalation Paths](#governance-alignment-and-escalation-paths)

---

## Governance Alignment and Escalation Paths

This SOP document is operational guidance and uses the same role model and escalation paths as:

- [GOVERNANCE.md](GOVERNANCE.md) for decision authority
- [MAINTAINERS.md](MAINTAINERS.md) for role ownership
- [CONTRIBUTING.md](CONTRIBUTING.md) for contributor workflow
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for community behavior enforcement
- [SECURITY.md](SECURITY.md) for security disclosure process

Canonical escalation channels:

- **General/project process:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Bug/features/process issues:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Security vulnerabilities:** [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)

---

## SOP-01 — Stable Release

**Trigger:** MINOR or MAJOR release scheduled on the roadmap.  
**Owner:** Release Manager  
**Prerequisite:** The final RC has passed CI and been soaked for ≥ 1 week in staging.

### Steps

| # | Action | Who | Checklist |
|---|---|---|---|
| 1 | Verify all items in the `[Unreleased]` CHANGELOG section are accurate and complete | RM | `[ ]` |
| 2 | Create a `release/vX.Y.0` branch from `develop` | RM | `[ ]` |
| 3 | Bump version in `VERSION` to `X.Y.0` and in `CMakeLists.txt` `project()` call | RM | `[ ]` |
| 4 | Rename `[Unreleased]` in `CHANGELOG.md` to `[X.Y.0] - YYYY-MM-DD` | RM | `[ ]` |
| 5 | Add new empty `[Unreleased]` section at the top of `CHANGELOG.md` | RM | `[ ]` |
| 6 | Open PR: `release/vX.Y.0` → `main` (Community) | RM | `[ ]` |
| 7 | Verify CI passes: edition-community-ci, pr-path-gate-main | CI Bot | `[ ]` |
| 8 | Merge PR into `main` after approval from ≥ 2 reviewers | RM | `[ ]` |
| 9 | Tag the merge commit: `git tag -s vX.Y.0 -m "Release vX.Y.0"` | RM | `[ ]` |
| 10 | Push tag: triggers Docker publish and GitHub Release workflow | RM | `[ ]` |
| 11 | Verify GitHub Release artefacts (archive, checksum, Docker image) | RM | `[ ]` |
| 12 | Merge `release/vX.Y.0` back into `develop` to sync version bump | RM | `[ ]` |
| 13 | Close the release milestone in GitHub | RM | `[ ]` |
| 14 | Post release announcement in GitHub Discussions | RM | `[ ]` |
| 15 | (Enterprise/Hyperscaler) Mirror release to respective lanes | RM | `[ ]` |

**Related:** [RELEASE_STRATEGY.md §4](RELEASE_STRATEGY.md#4-release-process) · [VERSIONING.md](VERSIONING.md) · [.github/RELEASE.md](.github/RELEASE.md)

---

## SOP-02 — Hotfix / Patch Release

**Trigger:** P0 (critical) or P1 (high-severity) bug or security vulnerability in a stable release.  
**Owner:** Release Manager + On-call engineer  
**SLA:** P0 fix within 48 h; P1 fix within 1 week.

### Steps

| # | Action | Who |
|---|---|---|
| 1 | Create `hotfix/X.Y.Z-description` branch from the affected release tag on `main` | On-call |
| 2 | Apply the minimal fix; add a regression test | On-call |
| 3 | Bump `VERSION` to `X.Y.Z` (PATCH increment) | On-call |
| 4 | Add CHANGELOG entry under a new `## [X.Y.Z] - YYYY-MM-DD` section | On-call |
| 5 | Open PR: `hotfix/…` → `main`; request expedited review | On-call |
| 6 | Verify CI passes (at minimum: community build + test) | CI Bot |
| 7 | Merge with ≥ 1 reviewer approval (P0) or ≥ 2 (P1) | RM |
| 8 | Tag: `git tag -s vX.Y.Z -m "Hotfix vX.Y.Z"` and push | RM |
| 9 | Verify GitHub Release and Docker image | RM |
| 10 | Back-port fix to `develop` via a separate PR | On-call |
| 11 | If a security fix: update [SECURITY.md](SECURITY.md) supported-versions table | RM |

> **Security hotfixes:** Follow [SOP-05](#sop-05--security-vulnerability-response) in parallel.

---

## SOP-03 — Release Candidate Promotion

**Trigger:** Feature freeze reached; development complete for the upcoming MINOR release.  
**Owner:** Release Manager

### Steps

| # | Action |
|---|---|
| 1 | Branch `release/vX.Y.0-rc1` from `develop` |
| 2 | Bump `VERSION` to `X.Y.0rc1`; update CHANGELOG header to `[X.Y.0-rc1] - YYYY-MM-DD` |
| 3 | Open PR: `release/vX.Y.0-rc1` → `main` |
| 4 | CI must pass: all edition builds green |
| 5 | Merge and tag `vX.Y.0-rc1`; publish pre-release Docker image |
| 6 | Announce RC in GitHub Discussions with a call for testing |
| 7 | Monitor issues labelled `regression` or `rc-blocker` for 1 week |
| 8 | If blockers found: fix on `release/vX.Y.0-rc1`, increment to `rc2`, repeat from step 3 |
| 9 | If no blockers: proceed with [SOP-01](#sop-01--stable-release) |

---

## SOP-04 — Rollback a Release

**Trigger:** Stable or RC release found to be severely broken post-publish.  
**Owner:** Release Manager

### Community (main)

```bash
git checkout main
git revert <merge-commit-sha> --no-commit
git commit -m "revert: roll back vX.Y.Z"
git push origin main
# Re-tag the previous good commit as vX.Y.(Z-1) if needed
```

### Docker image rollback

1. In DockerHub (or GHCR), retag the previous stable image as `latest`.
2. Optionally retract the broken tag via the registry API.
3. Re-trigger `dockerhub-publish-on-release.yml` via `workflow_dispatch` with the previous stable tag.

### Communication

- Post an announcement in GitHub Discussions: explain the rollback, affected versions, and the workaround.
- Update the GitHub Release description with a "⚠️ RETRACTED" notice.

**Related:** [RELEASE_STRATEGY.md §7](RELEASE_STRATEGY.md#7-rollback-plan)

---

---

## SOP-04b — Nightly Release (Automated)

**Trigger:** Automatic daily schedule at 03:30 UTC, or manual workflow dispatch.  
**Owner:** CI/automation (no human approval required)  
**Prerequisite:** Commits exist since last nightly release.

### Automation Flow

The `.github/workflows/release-nightly.yml` workflow runs automatically every day and:

1. Checks for commits since the last nightly tag
2. Sets `RELEASE_TYPE=nightly` 
3. Generates nightly version: `v<major>.<minor>.<patch>-nightly.<YYYYMMDD>.<runnum>`
4. Builds community edition (all platforms)
5. Generates build metadata (timestamp, commit hash, build number)
6. Creates GitHub pre-release with auto-generated release notes
7. Pushes Docker images:
   - `themisdb:nightly`
   - `themisdb:nightly-YYYYMMDD`
   - `themisdb:2.4.0-nightly-YYYYMMDD`
8. Attaches build metadata, SBOM, and changelog to the release
9. Posts notification to the pinned nightly tracker issue

### Manual Trigger

To trigger a nightly release manually:

```bash
gh workflow run release-nightly.yml --ref develop --field force=true
```

### When to Use Nightly Releases

- **For early testing:** Users and developers get the latest development build daily
- **For CI verification:** Automated verification that the pipeline works end-to-end
- **For rollback testing:** Nightly artifacts can be used to verify rollback procedures

### Nightly Release Artifacts

| Platform | Format | Located at |
|---|---|---|
| Linux x86_64 | `.tar.gz`, `.deb`, `.rpm`, `.zip` | GitHub Release assets |
| Windows x86_64 | `.zip` | GitHub Release assets |
| Docker | Multi-arch image | `themisdb:nightly`, `ghcr.io/makr-code/themisdb:nightly` |
| Build info | `build-metadata.json` | GitHub Release asset |
| Software BOM | `sbom-source.cyclonedx.json` | GitHub Release asset |

### Quality Assurance

- Nightly builds do NOT require additional sign-off beyond CI gates
- Nightly images are not pushed to `latest` tag
- Nightly versions are not submitted to WinGet
- Users are advised that nightly builds are for testing only

### Rollback

If a nightly build is broken:
1. The next nightly will automatically supersede it
2. Or manually delete the problematic release from GitHub UI
3. No code changes required

---

## SOP-04c — Semi-Automatic Release Promotion (via PR Labels)

**Trigger:** Pull request labeled with `release/alpha`, `release/beta`, `release/rc`, or `release/stable` and merged.  
**Owner:** Release Manager (approval), automation (promotion)  
**SLA:** Release PR created within 5 minutes of source PR merge.

### Workflow Overview

The `.github/workflows/release-promote.yml` workflow is triggered when a PR is merged with a release label:

1. Extract release type from the PR label
2. Read current VERSION file
3. Bump version according to release type (or manual input)
4. Update RELEASE_TYPE file
5. Update CHANGELOG.md with new version section and git log entries
6. Create release PR: `release/v<new-version>` → `develop`
7. Add PR checklist for manual verification
8. Comment on source PR with promotion status

### Step-by-Step: Promoting an Alpha Release

**Setup:**
1. Ensure code changes are merged to `develop`
2. Apply label `release/alpha` to the PR before merging

**Workflow Execution:**
1. PR is merged; label triggers `.github/workflows/release-promote.yml`
2. Workflow reads current version (e.g., `2.4.0`) and RELEASE_TYPE (e.g., `alpha`)
3. Bumps to next MINOR: `2.5.0-alpha1`
4. Sets `RELEASE_TYPE=alpha`
5. Updates CHANGELOG: adds `## [2.5.0-alpha1] - YYYY-MM-DD` section
6. Creates PR: `release/v2.5.0-alpha1` → `develop`
7. PR includes checklist:
   - [ ] CI gates passed
   - [ ] CHANGELOG entries accurate
   - [ ] VERSIONING.md examples updated
   - [ ] No breaking changes
   - [ ] Approved by ≥ 2 maintainers

**Manual Approval & Release:**
1. Maintainer reviews the generated release PR
2. Verifies: version bump is correct, changelog entries match reality
3. Requests changes if needed (e.g., if PATCH should be bumped instead)
4. Approves and merges release PR into `develop` after ≥ 2 approvals
5. **Manually creates the release tag:**
   ```bash
   git checkout develop
   git pull
   git tag -s v2.5.0-alpha1 -m "Release v2.5.0-alpha1"
   git push origin v2.5.0-alpha1
   ```
6. Tag push automatically triggers `.github/workflows/release-publish.yml`
7. Unified orchestrator builds, publishes to GitHub + Docker, and notifies

### Step-by-Step: Promoting a Stable Release from RC

**Setup:**
1. Current version is `2.5.0-rc1` (from a previous RC promotion)
2. RC has been tested and approved for promotion to stable
3. Apply label `release/stable` to the PR that merges final RC fixes

**Workflow Execution:**
1. Workflow reads version `2.5.0-rc1` and RELEASE_TYPE `rc`
2. Detects stable promotion; bumps to: `2.5.0` (removes -rc suffix)
3. Sets `RELEASE_TYPE=stable`
4. Updates CHANGELOG: `## [2.5.0] - YYYY-MM-DD`
5. Creates release PR with full checklist

**Manual Approval:**
1. Release manager reviews and verifies:
   - All Wave 7 gates passed
   - `release_critical` CI is green
   - Top-risk module sign-offs received (server, llm, sharding)
   - Resilience/security artifacts complete
   - Release notes drafted
2. Approves PR and merges into `develop`
3. Creates tag: `git tag -s v2.5.0 -m "Release v2.5.0"`
4. Pushes tag: `git push origin v2.5.0`

### Approval Checklist Template

The auto-generated release PR includes this checklist (reviewer must verify):

```markdown
## Release Promotion Checklist

- [ ] **CI Gates:** All CI workflows passed (build, test, lint)
- [ ] **Changelog:** Release notes in CHANGELOG.md are complete and accurate
- [ ] **Version:** VERSION and RELEASE_TYPE files contain correct values
- [ ] **Documentation:** VERSIONING.md examples are current (if applicable)
- [ ] **Breaking Changes:** No breaking changes introduced OR documented in CHANGELOG
- [ ] **Release Notes:** For stable/rc, release notes drafted and ready for publication
- [ ] **Governance:** All release governance documents synchronized (ROADMAP.md, FUTURE_ENHANCEMENTS.md)
- [ ] **Approvals:** This PR has been approved by ≥ 2 maintainers

## Manual Actions After Merge

After this PR is merged into `develop`, the release manager must:

1. Create the release tag:
   ```bash
   git checkout develop
   git pull
   git tag -s v<version> -m "Release v<version>"
   git push origin v<version>
   ```

2. Verify the tag triggers `release-publish.yml` workflow

3. Monitor the unified release orchestration workflow for completion
```

### Version Bump Rules

| Release Type | Bump Strategy | Example |
|---|---|---|
| **Alpha** | MINOR (default) | `2.4.0` → `2.5.0-alpha1` |
| **Beta** | PATCH (increment suffix) | `2.5.0-alpha2` → `2.5.0-beta1` |
| **RC** | PATCH (increment suffix) | `2.5.0-beta1` → `2.5.0-rc1` |
| **Stable** | Remove suffix | `2.5.0-rc1` → `2.5.0` |
| **Hotfix** | PATCH (manual via SOP-02) | `2.5.0` → `2.5.1` |

Override via manual dispatch:
```bash
gh workflow run release-promote.yml \
  --ref develop \
  --field release_type=rc \
  --field version_bump=patch
```

### Troubleshooting

**Q: The version bump is wrong (should be PATCH, got MINOR)**  
A: Approve the PR with a request for changes. The maintainer can:
1. Edit VERSION and RELEASE_TYPE files directly in the PR
2. Or dismiss the PR and trigger a manual dispatch with correct `version_bump=patch`

**Q: The release PR was created but CI is failing**  
A: Wait for CI to complete. If it fails:
1. Fix the issue in a new commit on the release branch
2. Push the fix; CI will re-run
3. Once green, request approval

**Q: The release PR was not created**  
A: Check the workflow logs:
```bash
gh run list --workflow release-promote.yml --limit 1
gh run view <run-id> --log
```

---

## SOP-05 — Security Vulnerability Response

**Trigger:** Security vulnerability reported (internally discovered or via responsible disclosure).  
**Owner:** Security Lead + Release Manager  
**SLA:** Acknowledge within 24 h; patch within 48 h (P0 / critical) or 7 days (P1 / high).

### Steps

| # | Action |
|---|---|
| 1 | Acknowledge the report via GitHub Security Advisories (private) |
| 2 | Assess severity: CVSS score, affected versions, exploitability |
| 3 | Create a private fork / security advisory draft in GitHub |
| 4 | Develop and test the fix in the private fork |
| 5 | Prepare CHANGELOG entry (security section) |
| 6 | Coordinate disclosure timeline with the reporter |
| 7 | Follow [SOP-02](#sop-02--hotfix--patch-release) to release the patch |
| 8 | Publish the GitHub Security Advisory after the patch is released |
| 9 | Request a CVE via GitHub if warranted |
| 10 | Update [SECURITY.md](SECURITY.md) with the new supported-versions table if needed |
| 11 | Post-mortem: document root cause and preventive measures in `docs/security/` |

> **Do NOT open a public issue for security vulnerabilities.** Use [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new).

**Related:** [SECURITY.md](SECURITY.md) · [.github/SECURITY.md](.github/SECURITY.md)

---

## SOP-06 — Dependency Update

**Trigger:** New version of a critical dependency (vcpkg, llama.cpp, RocksDB, etc.) is available, or a dependency has a known CVE.  
**Owner:** Maintainer

### Steps

| # | Action |
|---|---|
| 1 | Check the dependency's changelog for breaking changes |
| 2 | Update the version pin in `vcpkg.json` / `CMakeLists.txt` / submodule SHA |
| 3 | Run the full build on all relevant edition presets |
| 4 | Run the full test suite (`ctest --preset community-release`) |
| 5 | Run benchmarks if the dependency is performance-critical |
| 6 | Open a PR targeting `develop`; describe the update and test results |
| 7 | If a security fix: label the PR `security` and request expedited review |
| 8 | Add a CHANGELOG entry under `[Unreleased] / Changed` or `/ Security` |

---

## SOP-07 — New Contributor Onboarding

**Trigger:** A new contributor opens their first PR or asks for guidance.  
**Owner:** Any maintainer

### Steps

1. Welcome the contributor and point them to [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
2. Apply the `good first issue` label to suitable open issues.
3. Ensure the contributor's first PR is reviewed within **5 business days**.
4. Offer constructive, specific feedback; avoid vague rejections.
5. Once merged, thank the contributor and add them to the contributors list if maintained.

---

## SOP-08 — Incident Response

**Trigger:** Production outage, data loss event, or severe degradation reported by a user or monitoring.  
**Owner:** On-call engineer

### Severity levels

| Level | Definition | Response time |
|---|---|---|
| **P0** | Complete outage / data loss | Immediate (< 15 min) |
| **P1** | Major feature unavailable | < 2 h |
| **P2** | Degraded performance | < 8 h |
| **P3** | Minor issue / cosmetic | Next business day |

### Response playbook

| # | Action |
|---|---|
| 1 | **Detect** — alert from Prometheus/Grafana or user report |
| 2 | **Acknowledge** — assign an on-call engineer; open an incident issue on GitHub |
| 3 | **Communicate** — notify affected users via GitHub Discussions or status page |
| 4 | **Triage** — reproduce, determine scope and impact |
| 5 | **Mitigate** — apply a workaround if available (e.g., rollback — see [SOP-04](#sop-04--rollback-a-release)) |
| 6 | **Fix** — develop and test a permanent fix; follow [SOP-02](#sop-02--hotfix--patch-release) to release |
| 7 | **Verify** — confirm the fix resolves the issue in production / staging |
| 8 | **Post-mortem** — within 5 business days: write a blameless post-mortem in `docs/incidents/YYYY-MM-DD-title.md` covering: timeline, root cause, impact, action items |
| 9 | **Close** — close the incident issue; update runbooks if needed |

---

## Document History

| Version | Date | Author | Change |
|---|---|---|---|
| 1.0 | 2026-04-13 | Release Team | Initial version |

---
Zuletzt geprueft (Root-Sync): 2026-05-26

