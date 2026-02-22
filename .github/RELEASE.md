# Release Management

## Release Versioning

ThemisDB follows **Semantic Versioning** ([SemVer 2.0.0](https://semver.org/)):

```
X.Y.Z[-pre-release][+build]
│ │ │
│ │ └── Patch: backwards-compatible bug fixes
│ └──── Minor: backwards-compatible new features
└────── Major: incompatible API changes
```

### Pre-release Labels

| Label | Usage | Example |
|-------|-------|---------|
| `alpha` | Early development, unstable | `2.0.0-alpha.1` |
| `beta` | Feature-complete, integration testing | `2.0.0-beta.2` |
| `rc` | Release candidate, stabilization only | `2.0.0-rc.1` |

### Breaking Changes Policy

- Breaking changes are **only** allowed in Major version bumps.
- All breaking changes **must** be documented in `CHANGELOG.md` under a `### Breaking Changes` section.
- Deprecation warnings are added in the preceding Minor release (see Deprecation Policy).
- A migration guide is published before or alongside the Major release.

---

## Release Cycle

| Phase | Duration | Description |
|-------|----------|-------------|
| **Feature Development** | Until feature freeze | New features merged to `develop` |
| **Feature Freeze** | T-4 weeks | No new features; only bugfixes |
| **Code Freeze** | T-2 weeks | Only critical bugfixes and docs |
| **Testing Period** | T-2 weeks to T-1 week | Full regression + performance tests |
| **Release Candidate** | T-1 week | RC tag cut; final validation |
| **Release** | T-0 | Final tag, artifacts, announcement |
| **Post-Release Support** | T+4 weeks | Patch-level fixes for regressions |

### Branch Strategy

- `develop` → active development target
- `release/X.Y.Z` → stabilization branch cut from `develop` at feature freeze
- `main` → always reflects latest stable release (merged from `release/X.Y.Z` at release)
- Hotfix branches → `hotfix/X.Y.Z` cut from `main`

---

## Release Checklist

Before tagging a release, all items must be completed:

- [ ] All planned PRs merged and CI passing on `release/X.Y.Z`
- [ ] `CHANGELOG.md` updated with full list of changes
- [ ] Version numbers bumped in `VERSION`, `CMakeLists.txt`, and package manifests
- [ ] Security review completed (no open Critical/High CVEs)
- [ ] Performance baseline established and regressions resolved
- [ ] Release notes prepared (summary for end-users)
- [ ] Git tag created: `git tag -s vX.Y.Z -m "Release vX.Y.Z"`
- [ ] Release artifacts built and signed (checksums + GPG signature)
- [ ] Docker images built, scanned, and pushed to registry
- [ ] Documentation updated (`docs/`, `mkdocs.yml`)
- [ ] SBOM generated and attached to release
- [ ] `main` branch updated (merge `release/X.Y.Z` → `main`)
- [ ] Release announcement prepared (GitHub Release, mailing list)
- [ ] Post-release support window opened (milestone in GitHub)

---

## Deprecation Policy

- Deprecated features receive a **minimum 2 major versions** notice before removal.
- Deprecations are announced in `CHANGELOG.md` and in-code with compiler warnings or runtime log warnings.
- A **migration guide** is provided alongside the deprecation announcement.
- Timeline example: Feature deprecated in v1.5.0 → removed no earlier than v3.0.0.

### Deprecation Warning in Code

```cpp
// Example C++ deprecation annotation:
[[deprecated("Use newFunction() instead. Will be removed in v3.0.")]]
void oldFunction();
```

---

## Rollback Procedure

### When to Rollback

Initiate a rollback if any of the following occur within 72 hours of release:

- Critical regression affecting core database operations
- Security vulnerability introduced by the release
- Data corruption or data loss risk identified
- More than 10% error rate increase in production telemetry

### How to Rollback

1. **Announce** the rollback decision in the incident channel and GitHub issue.
2. **Tag** the previous stable version as the current stable: re-point `latest` Docker tag.
3. **Revert** the release branch merge on `main` if possible, or create `hotfix/X.Y.Z-rollback`.
4. **Notify** users via GitHub Release notes, flagging the release as pre-release.
5. **Publish** a patch release (`X.Y.Z+1`) with the regression fixed as soon as possible.

### Customer Communication Plan

| Severity | Communication Channel | Timing |
|----------|----------------------|--------|
| Critical regression | GitHub Security Advisory + email | Within 2 hours |
| High regression | GitHub Release notes + issue | Within 24 hours |
| Medium regression | GitHub Issue + CHANGELOG note | Within 72 hours |

### Post-Mortem Requirements

For any rollback event, a post-mortem must be completed within **5 business days**:

- Timeline of events
- Root cause analysis
- Impact assessment
- Corrective actions with owners and due dates
- Process improvements to prevent recurrence

Post-mortems are published in `docs/post-mortems/` and linked from the GitHub issue.

---

*This release policy is reviewed with each major release cycle. Last review: 2026-Q1.*
