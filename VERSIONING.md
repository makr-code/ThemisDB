# ThemisDB — Versioning Policy

> **Status:** Active  
> **Applies to:** All ThemisDB editions (MINIMAL, COMMUNITY, ENTERPRISE, MILITARY, HYPERSCALER)

This document defines the versioning scheme, release cadence, and support lifecycle for ThemisDB.

---

## Table of Contents

1. [Version Format](#1-version-format)
2. [Version Identifiers in the Repository](#2-version-identifiers-in-the-repository)
3. [Release Types](#3-release-types)
4. [Release Cadence](#4-release-cadence)
5. [Supported Versions & End-of-Life](#5-supported-versions--end-of-life)
6. [Edition Versioning](#6-edition-versioning)
7. [Changelog Requirements](#7-changelog-requirements)
8. [Deprecation Policy](#8-deprecation-policy)
9. [Breaking Changes](#9-breaking-changes)
10. [Pre-release Identifiers](#10-pre-release-identifiers)

---

## 1. Version Format

ThemisDB follows [Semantic Versioning 2.0.0](https://semver.org/):

```
MAJOR.MINOR.PATCH[-PRE_RELEASE]

Examples:
  2.6.0
  2.6.0-rc1
  2.7.0-alpha1
  2.7.0-beta1
```

| Segment | Incremented when |
|---|---|
| **MAJOR** | Incompatible API or wire-protocol changes |
| **MINOR** | New backward-compatible functionality |
| **PATCH** | Backward-compatible bug fixes and security patches |
| **PRE_RELEASE** | Pre-release qualifier (see §10) |

> **Rule:** PATCH resets to `0` on a MINOR bump; MINOR resets to `0` on a MAJOR bump.

---

## 2. Version Identifiers in the Repository

The canonical contract is simple and deliberate:

- `VERSION` is the canonical product version for ThemisDB.
- `RELEASE_TYPE` is the canonical release channel (`nightly`, `alpha`, `beta`, `rc`, `stable`).
- Docker tags, OCI labels, GitHub Release tags, and package metadata are derived outputs from that pair.
- No independent version string should be introduced in Dockerfiles, build scripts, or release automation when the canonical root files already define the product version.

| File | Purpose | Example |
|---|---|---|
| [`VERSION`](VERSION) | Canonical product version | `2.4.0` or `2.4.0-rc1` |
| [`RELEASE_TYPE`](RELEASE_TYPE) | Canonical release channel | `nightly`, `alpha`, `beta`, `rc`, `stable` |
| [`CHANGELOG.md`](CHANGELOG.md) | Release history for each version | `## [2.4.0] - 2026-10-10` |
| `CMakeLists.txt` / `cmake/Versions.cmake` | Build-time consumption of the canonical version | Reads `VERSION` and exposes `THEMIS_VERSION` |

The build system must consume the root-level version files as the source of truth. `Dockerfile`, GHCR/GHCR tags, and release metadata are derived from that product version, not the other way around.

### Version Update Procedures

When creating or promoting a release:

1. Update `VERSION` with the target product version.
2. Update `RELEASE_TYPE` with the release channel.
3. Update `CHANGELOG.md` with the release notes section.
4. Ensure the CMake version metadata still matches the root `VERSION` file.
5. Create the release tag after approval: `git tag -s v<version> -m "Release v<version>"`.

When using the automation workflows:
- `.github/workflows/release-promote.yml` updates `VERSION`, `RELEASE_TYPE`, and `CHANGELOG.md`.
- `.github/workflows/release-docker-image.yml` emits Docker tags and OCI metadata from the canonical version data.
- Manual tag creation remains the final approval step, not the source of truth.

### 2.1 Pull Request Version Targeting

Every PR must declare a target version at merge time. This maps work to a release lane and keeps the changelog and roadmap aligned.

| PR Type | Target Version | Example |
|---------|----------------|---------|
| New feature | Next planned MINOR | `v2.5.0-alpha1` |
| Bug fix (current RC/stable) | Current release or patch | `v<current-rc>` or `v<current-stable-patch>` |
| Documentation | Feature version | Same as documented feature |
| Security patch | Current stable first | `v<current-stable>` then backport |
| Infrastructure / refactoring | Next MINOR or backlog | `v2.5.0-alpha1` or `[Unreleased]` |

See [docs/governance/PR_VERSION_TARGETING.md](docs/governance/PR_VERSION_TARGETING.md) for detailed selection criteria and release-manager workflow.

---

## 3. Release Types

| Type | Description | Example tag | Typical cadence |
|---|---|---|---|
| **Nightly** | Automated development build from the active branch | `v2.4.0-nightly.20260904.1234` | Daily / on demand |
| **Alpha** | Early preview; API may change | `v2.5.0-alpha1` | As needed |
| **Beta** | Feature-complete; API stabilising | `v2.5.0-beta1` | As needed |
| **Release Candidate (RC)** | Feature-frozen; bug-fix only | `v2.5.0-rc1` | 1–2 weeks before stable |
| **Stable** | General availability (GA) | `v2.5.0` | Every 6–8 weeks (MINOR) |
| **Patch / Hotfix** | Critical fix on a stable release | `v2.5.1` | As needed |

Release progression is:

```
nightly → alpha → beta → rc → stable
```

The canonical `RELEASE_TYPE` values are: `nightly`, `alpha`, `beta`, `rc`, `stable`.

### 3.1 Docker and package tags are derived, not authoritative

The product version and release type are the authoritative inputs. Docker version tags are generated from them by CI and release automation.

Examples:

| Canonical input | Derived Docker tag |
|---|---|
| `VERSION=2.4.0`, `RELEASE_TYPE=stable` | `themisdb:2.4.0`, `themisdb:2.4`, `themisdb:latest` |
| `VERSION=2.5.0`, `RELEASE_TYPE=rc` | `themisdb:2.5.0-rc1`, `themisdb:2.5-rc1` |
| `VERSION=2.4.0`, `RELEASE_TYPE=nightly` | `themisdb:nightly`, `themisdb:nightly-YYYYMMDD`, `themisdb:2.4.0-nightly-YYYYMMDD` |

OCI image labels should carry metadata such as build timestamp, git revision, and release type without creating a second independent release version system. The generated metadata action is the expected mechanism for this.

### 3.2 Nightly format

Nightly versions use a specialized format to support multiple nightly builds per day:

```
v<major>.<minor>.<patch>-nightly.<YYYYMMDD>.<runnum>
```

Example:
- `v2.4.0-nightly.20260904.1234`

This format is used for derived release metadata and tag generation only; the canonical root version remains the plain `VERSION` file value.

---

## 4. Release Quality Gates

A stable / GA tag may only be cut after the release-policy gates in `RELEASE_STRATEGY.md` are satisfied on `develop`.

Required evidence bundle:
- Wave 7 PASS on the current baseline
- green `release_critical` CI on `develop`
- no new CRITICAL findings in `server`, `llm`, and `sharding`
- required sanitizer, recovery, chaos/fault-injection, penetration-test, SLA, and runbook artefacts
- synchronized release/governance documentation (`ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `CHANGELOG.md`, branch/release/versioning docs)
- completed GA hardening execution batches (A-D) with boundary evidence updates in planning/status documents

Current batch tracking is maintained in `ROADMAP.md`, `NEXT_PHASE_IMPLEMENTATION_PLAN.md`, and `ai_working/NEXT_PHASE_STATUS.md`. Technical gates for Batch D (D-1..D-10) have passed. The final human governance sign-off is still pending and is the remaining GA promotion blocker.

### 4.1 Stable / GA Promotion Evidence

| Release type | Approximate cadence |
|---|---|
| Stable MINOR | Every 6–8 weeks |
| Stable PATCH / Hotfix | As needed (P0 within 48 h, P1 within 1 week) |
| Release Candidate | 1–2 weeks before a stable release |

Release dates are tracked in [`CHANGELOG.md`](CHANGELOG.md) and announced via GitHub Releases.

---

## 5. Supported Versions & End-of-Life

ThemisDB maintains support windows for the current stable release and the most recent prior major/minor line, with support obligations documented in the release notes and branch strategy. Backports for critical fixes must use the same semantics as the product version rule above and must not introduce independent version strings outside the canonical repo files.

### 5.1 Compatibility and migration

- Version bumps are semver-based and require a matching changelog entry.
- Breaking changes must be called out explicitly in the release notes and migration guidance.
- Backports must preserve the `VERSION` + `RELEASE_TYPE` contract and only derive tag metadata from it.

---

## 6. Edition Versioning

Edition-specific builds follow the same semantic version rules but may carry different branch or distribution contexts (for example: `community`, `enterprise`, `hyperscaler`, `military`, `minimal`). The canonical version data remains the same product version source, while edition routing is orthogonal to tag generation.

---

## 7. Changelog Requirements

Each published release must include a changelog section demonstrating:

- the released version number
- the release channel
- the validated functional scope
- migration or compatibility notes where relevant
- fixed issues and security updates

---

## 8. Deprecation Policy

Deprecated public behavior must be clearly labeled in the changelog and release notes with the planned removal target and migration path. The product version contract stays unchanged until the deprecation is formally removed in a major or minor release.

---

## 9. Breaking Changes

Breaking changes require explicit change notes and a version bump that matches the semantic-version policy. They must not be silently hidden behind a Docker tag or undocumented build artifact drift.

---

## 10. Pre-release Identifiers

The canonical pre-release suffixes are:

| `RELEASE_TYPE` | Canonical suffix |
|---|---|
| `nightly` | `-nightly.<YYYYMMDD>.<runnum>` |
| `alpha` | `-alphaN` |
| `beta` | `-betaN` |
| `rc` | `-rcN` |
| `stable` | none |

These are formatting conventions for release metadata and tags; they are not separate product version sources.

---

## 5. Release Cadence

| Version line | Status | Security updates | End-of-Life |
|---|---|---|---|
| **2.4.x** | ✅ Active / Current prerelease line | ✅ Yes | TBD |
| **2.3.x and earlier** | ⚠️ Historical lines — verify per release notes before promising support | Case-by-case | See `CHANGELOG.md` |

**Maintenance** means security patches and critical bug fixes only; no new features.  
**Unsupported** means no patches of any kind are provided.

---

## 6. Supported Versions & End-of-Life

## 7. Edition Versioning

Private plugins use their own SemVer in addition to the core repository version.

Rules:
- plugin `MAJOR`: plugin ABI/API break or incompatible core-compatibility contract change
- plugin `MINOR`: new backward-compatible capability
- plugin `PATCH`: backward-compatible fix or hardening
- the superproject release contract is the combination of a plugin-named private submodule pin + manifest compatibility fields, not a floating branch name
- private plugin manifests should declare `min_themisdb_version`, optional `max_themisdb_version`, and optional `compatible_core_abi`
- edition-restricted plugins must also declare `allowed_editions` and `license_feature` so runtime and packaging gates can stay fail-closed


All five editions share the same `MAJOR.MINOR.PATCH` base version. Edition-specific builds are distinguished by branch and release naming convention:

| Edition | Git branch | Docker tag pattern | Git tag pattern |
|---|---|---|---|
| COMMUNITY | `community` | `themisdb/themisdb:<version>-community-binary-x64` and `...-arm` | `v<version>` |
| ENTERPRISE | `enterprise` | `<private-registry>/themisdb-enterprise:<version>-enterprise-binary-x64` and `...-arm` | `enterprise-v<version>` |
| MILITARY | `military` | (private registry) | `military-v<version>` |
| HYPERSCALER | `hyperscaler` | `<oem-registry>/themisdb-hyperscaler:<version>-hyperscaler-binary-x64` and `...-arm` | `hyperscaler-v<version>` |
| MINIMAL | `minimal` | `themisdb/themisdb-minimal:<version>-minimal-binary-x64` and `...-arm` | `minimal-v<version>` |

Release assets on GitHub follow the same canonical basename:

`themisdb-{version}-{edition}-{sourcecode|binary}-{arm|x86|x64}`

See [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) for branch rules, CI gates, and the edition feature matrix.

---

## 7.1 Private Plugin Version Compatibility

Every release **must** include a corresponding entry in [`CHANGELOG.md`](CHANGELOG.md) following the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) convention:

```markdown
## [MAJOR.MINOR.PATCH] - YYYY-MM-DD

### Added
- ...

### Changed
- ...

### Deprecated
- ...

### Removed
- ...

### Fixed
- ...

### Security
- ...
```

The `[Unreleased]` section accumulates changes in progress and is renamed to the version number at release time.

---

## 8. Changelog Requirements

1. A feature is marked **deprecated** in the CHANGELOG under `### Deprecated`.
2. A deprecation notice is added to the API documentation and (where applicable) a compiler/runtime warning is emitted.
3. The deprecated feature is removed no earlier than the **next MAJOR release** (minimum one MINOR release notice period).
4. Deprecations are never introduced in PATCH releases.

---

## 9. Deprecation Policy

Breaking changes (API, ABI, wire-protocol, configuration schema) require a **MAJOR version bump**.

Before introducing a breaking change:
- Open a GitHub issue labelled `breaking-change` and link it from the CHANGELOG.
- Provide a migration guide in `docs/migration/` and reference it from the CHANGELOG.
- Where feasible, provide an automated migration tool or script.

> **Wire Protocol:** The ThemisDB Wire Protocol version is independently versioned (`V1`, `V2`, …). New protocol versions are introduced with MINOR version bumps and old versions remain supported for at least one full MAJOR cycle.

---

## 10. Breaking Changes

| Identifier | Meaning |
|---|---|
| `-alphaN` | Unstable preview (N = 1, 2, …) |
| `-betaN` | Feature-complete, stabilising |
| `-rcN` | Release candidate, feature-frozen |

Legacy forms `-alpha`, `-beta.N`, `-rc.N`, and `-rc` may still appear in historical release tags/changelog entries, but new releases should use the canonical `-alphaN` / `-betaN` / `-rcN` format.

Pre-release versions are never considered "stable" for production use. Docker tags for pre-releases carry the full qualifier (e.g., `themisdb/themisdb:1.9.0-rc1-community-binary-x64`) and the `latest` tag is only updated on stable releases.

---

## Related Documents

- [CHANGELOG.md](CHANGELOG.md) — Full release history
- [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) — Branch model, CI/CD, rollback
- [SOP.md](SOP.md) — Step-by-step release and hotfix procedures
- [SECURITY.md](SECURITY.md) — Security patch SLA
- [ai_context/COPILOT_INSTRUCTIONS.md](ai_context/COPILOT_INSTRUCTIONS.md) — AI/agent governance and documentation alignment rules
- [ROADMAP.md](ROADMAP.md) — Canonical feature/milestone scope
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) — Canonical open enhancement backlog

---
Zuletzt geprueft (Root-Sync): 2026-07-28 (Phase 6 in progress)
