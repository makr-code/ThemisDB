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

