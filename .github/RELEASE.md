# Release Management Guidelines

This document describes the versioning scheme, release process, release schedule, and changelog format for ThemisDB.

---

## Version Numbering

ThemisDB follows [Semantic Versioning 2.0.0](https://semver.org/):

```
MAJOR.MINOR.PATCH[-prerelease]
```

| Segment       | When to increment                                            |
|---------------|--------------------------------------------------------------|
| **MAJOR**     | Incompatible API changes or major architectural shifts        |
| **MINOR**     | New backward-compatible features                             |
| **PATCH**     | Backward-compatible bug fixes and security patches           |
| **Pre-release** | `-alpha`, `-beta`, `-rc.N` suffixes for pre-GA releases   |

Examples: `2.0.0`, `2.1.0-beta`, `2.1.0-rc.1`, `2.1.0`

---

## Release Types

| Type              | Trigger                          | Branch           |
|-------------------|----------------------------------|------------------|
| **Patch release** | Security fixes, critical bugs    | `release/2.x.y`  |
| **Minor release** | Quarterly feature milestone      | `release/2.x.0`  |
| **Major release** | As planned in ROADMAP.md         | `release/3.0.0`  |
| **Pre-release**   | Feature preview / RC validation  | `release/X.Y.Z-rc.N` |

---

## Release Schedule

- **Patch releases** — as needed; security patches within 7 days of confirmed CVSS ≥ 9.0 vulnerability
- **Minor releases** — quarterly (end of Q1, Q2, Q3, Q4)
- **Major releases** — per ROADMAP.md milestones; announced ≥ 4 weeks in advance

---

## Release Checklist

Every release **must** satisfy the following checklist before the release tag is created:

### Pre-Release

- [ ] All ROADMAP.md features scoped for this release are marked `[x]` (complete)
- [ ] All CI status checks passing on the release branch
- [ ] Security audit complete (run `./scripts/comprehensive-code-audit.sh`)
- [ ] No open CVSS ≥ 7.0 advisories without an accepted mitigation
- [ ] Commit signing verified — all commits on release branch must be GPG-signed
- [ ] SBOM generated (`syft . -o cyclonedx-json > releases/<version>/sbom.json`)
- [ ] Dependency vulnerability scan clean (`trivy fs --scanners vuln,secret,misconfig .`)
- [ ] Documentation updated (API reference, CHANGELOG, README badges)
- [ ] `VERSION` file bumped to the new version string
- [ ] Migration guide written if there are breaking changes

### Release Execution

- [ ] Merge release branch into `main` via PR (2 approvals + code owner required)
- [ ] Git tag created: `git tag -s v<version> -m "Release v<version>"`
- [ ] Tag pushed: `git push origin v<version>`
- [ ] GitHub Release created with release notes
- [ ] SBOM and checksums attached to the GitHub Release
- [ ] Docker images built, scanned, and pushed to registry

### Post-Release

- [ ] Announce in project channels / mailing list
- [ ] Update `develop` branch with post-release commits (version bump, CHANGELOG header)
- [ ] Close the corresponding GitHub Milestone
- [ ] Update ROADMAP.md to mark completed items and add next-cycle items

---

## Changelog Format

ThemisDB follows the [Keep a Changelog](https://keepachangelog.com/) format:

```markdown
## [2.1.0] - 2026-06-30

### Added
- CUDA geospatial distance kernels (acceleration module)
- Vector quantization support (Target: Q2 2026)

### Fixed
- Race condition in MVCC snapshot isolation (#1234)

### Changed
- Breaking: Modified `QueryResult` API — see migration guide

### Deprecated
- Legacy Index format (migration path provided in MIGRATION.md)

### Removed
- Support for TLS 1.0/1.1 (deprecated since v1.x)

### Security
- Patched AQL injection vulnerability in parser (CVE-2026-XXXX, CVSS 8.1)
```

### Rules

1. Each release has its own `## [version] - YYYY-MM-DD` heading.
2. Entries are grouped under `Added`, `Fixed`, `Changed`, `Deprecated`, `Removed`, `Security`.
3. Security entries must include CVE number and CVSS score if applicable.
4. Breaking changes are highlighted with **Breaking:** prefix.
5. Issue/PR references are encouraged (`(#1234)` or `(PR #5678)`).
6. The `## [Unreleased]` heading at the top collects changes for the next release.

---

## Branching Model

```
main          ◄──── production releases (tags only, GPG-signed)
  │
  └── release/2.1.0   ◄── release preparation (RC testing)
        │
develop       ◄──── integration of completed features
  │
  ├── feature/…
  ├── bugfix/…
  └── hotfix/…  ──► main (cherry-picked for urgent security patches)
```

Hotfixes for critical security vulnerabilities are branched from `main`, patched, merged back, and immediately tagged as a PATCH release.

---

## Tagging Convention

```bash
# Annotated, GPG-signed tag (required for all releases)
git tag -s v2.1.0 -m "Release v2.1.0 — Quarterly Q2 2026"

# Push tag
git push origin v2.1.0
```

Lightweight tags are **not** accepted for production releases.

---

## Related Documents

- [SECURITY.md](SECURITY.md) — Vulnerability disclosure and response SLAs
- [CHANGELOG.md](../CHANGELOG.md) — Full changelog history
- [roadmap.md](../roadmap.md) — Feature roadmap and implementation phases
- [CONTRIBUTING.md](../CONTRIBUTING.md) — Contribution guidelines
