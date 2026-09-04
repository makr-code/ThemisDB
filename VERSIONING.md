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

The canonical version is stored in two places that must always be kept in sync:

| File | Format | Example |
|---|---|---|
| [`VERSION`](VERSION) | Plain text, one line | `<major>.<minor>.<patch>[-pre]` |
| [`RELEASE_TYPE`](RELEASE_TYPE) | Plain text, one line | `nightly`, `alpha`, `beta`, `rc`, or `stable` |
| [`CHANGELOG.md`](CHANGELOG.md) | Keep a Changelog header | `## [<major>.<minor>.<patch>] - YYYY-MM-DD` |

Additionally, the CMake build system reads the version at configure time via the `VERSION` file and from `CMakeLists.txt` `project()` call. Keep these consistent.

The `RELEASE_TYPE` file holds the current release type as a single line of text. This file allows workflows and helper scripts to determine the release classification without parsing version strings. Valid values are: `nightly`, `alpha`, `beta`, `rc`, `stable`.

### Version Update Procedures

When manually creating a release:

1. Update `VERSION` with the new version string (e.g., `2.5.0` or `2.5.0-rc1`)
2. Update `RELEASE_TYPE` with the release classification (e.g., `stable` or `rc`)
3. Update `CHANGELOG.md` with the new version section
4. Verify `CMakeLists.txt` version matches `VERSION`
5. Commit all three files together
6. Create release tag: `git tag -s v<version> -m "Release v<version>"`

When using semi-automatic release workflows:
- `.github/workflows/release-promote.yml` automatically updates `VERSION`, `RELEASE_TYPE`, and `CHANGELOG.md`
- Manual tag creation is required after PR approval and merge

### 2.1 Pull Request Version Targeting

Every PR must declare a **target version** at merge time. This maps the PR to a GitHub milestone and enables:

- **Release scope tracking**: milestone aggregates all PRs targeting a release
- **Changelog generation**: PR titles/numbers are recorded in `CHANGELOG.md`
- **Roadmap alignment**: PR scope is validated against planned features in `ROADMAP.md`

**PR Version Selection:**

| PR Type | Target Version | Example |
|---------|----------------|---------|
| New feature | Next planned MINOR | `v2.5.0-alpha1` for feature work on develop |
| Bug fix (current RC/stable) | Current release or patch | `v<current-rc>` or `v<current-stable-patch>` |
| Bug fix (general) | Next MINOR | `v2.5.0-alpha1` |
| Documentation | Feature version | Same as documented feature |
| Security patch | Current stable first | `v<current-stable>` then backport to `v<current-stable-patch>` |
| Infrastructure / Refactoring | Next MINOR or backlog | `v2.5.0-alpha1` or `[Unreleased]` |

See [docs/governance/PR_VERSION_TARGETING.md](docs/governance/PR_VERSION_TARGETING.md) for detailed selection criteria and release manager workflow.

---

## 3. Release Types

| Type | Description | Example tag |
|---|---|---|
| **Alpha** | Early preview; API may change significantly | `v2.5.0-alpha1` |
| **Beta** | Feature-complete; API stabilising | `v2.5.0-beta1` |
| **Release Candidate (RC)** | Feature-frozen; only bug fixes | `vX.Y.Z-rcN` |
| **Stable** | General availability (GA) | `vX.Y.Z` |
| **Patch / Hotfix** | Critical fixes on a stable release | `vX.Y.(Z+1)` |

Releases progress through the type sequence: alpha → beta → rc → stable.  
Critical security fixes may bypass the pre-release sequence and be released directly as a patch.

`RELEASE_TYPE` values are normalized to: `alpha`, `beta`, `rc`, `stable`.

Canonical suffixes:

| `RELEASE_TYPE` | Canonical suffix | Legacy suffixes (historical entries only) |
|---|---|---|
| `alpha` | `-alphaN` | `-alpha` |
| `beta` | `-betaN` | `-beta.N` |
| `rc` | `-rcN` | `-rc.N`, `-rc` |
| `stable` | _(none)_ | n/a |

---

## 3. Release Types

| Type | Description | Example tag | Cadence |
|---|---|---|---|
| **Nightly** | Automated daily builds from develop (for early testing) | `v2.4.0-nightly.20260904.1234` | Daily 03:30 UTC |
| **Alpha** | Early preview; API may change significantly | `v2.5.0-alpha1` | As needed |
| **Beta** | Feature-complete; API stabilising | `v2.5.0-beta1` | As needed |
| **Release Candidate (RC)** | Feature-frozen; only bug fixes | `v2.5.0-rc1` | 1–2 weeks before stable |
| **Stable** | General availability (GA) | `v2.5.0` | Every 6–8 weeks (MINOR) |
| **Patch / Hotfix** | Critical fixes on a stable release | `v2.5.1` | As needed (P0: 48h, P1: 1 week) |

### 3.1 Nightly Release Format

Nightly versions use a specialized format to support multiple nightly builds per day:

```
v<major>.<minor>.<patch>-nightly.<YYYYMMDD>.<runnum>
```

Where:
- `<major>.<minor>.<patch>` — base version from `VERSION` file
- `<YYYYMMDD>` — release date (e.g., `20260904` for September 4, 2026)
- `<runnum>` — GitHub Actions run number (ensures unique tags even on same day)

**Examples:**
- `v2.4.0-nightly.20260904.1234`
- `v2.4.0-nightly.20260904.1235` (second nightly on same day)

**Sorting:** Nightly tags sort correctly chronologically and with semantic version comparison tools.

**Docker tags:** Nightly images receive multiple tags:
- `themisdb:nightly` (points to latest nightly, always updated)
- `themisdb:nightly-20260904` (date-specific, never changes)
- `themisdb:2.4.0-nightly-20260904` (full version with date)
- **Never** receives `latest` or `<version>` tags

Nightly releases are released automatically via `.github/workflows/release-nightly.yml`. They are always marked as pre-releases on GitHub and are not submitted to WinGet.

### 3.2 Release Type Progression

Releases progress through the type sequence:

```
nightly → alpha → beta → rc → stable
```

- **Nightly → Alpha:** Manual decision to stabilize a development build
- **Alpha → Beta:** Features complete, API stabilizing
- **Beta → RC:** Beta testing complete, enters feature-freeze
- **RC → Stable:** Release candidate approved after final soak period
- **Stable → Patch:** Critical fix on current stable line (may skip other pre-release types)

Releases progress through the type sequence: alpha → beta → rc → stable.  
Critical security fixes may bypass the pre-release sequence and be released directly as a patch.

`RELEASE_TYPE` values are normalized to: `nightly`, `alpha`, `beta`, `rc`, `stable`.

Canonical suffixes:

| `RELEASE_TYPE` | Canonical suffix | Legacy suffixes (historical entries only) |
|---|---|---|
| `nightly` | `-nightly.<YYYYMMDD>.<runnum>` | n/a |
| `alpha` | `-alphaN` | `-alpha` |
| `beta` | `-betaN` | `-beta.N` |
| `rc` | `-rcN` | `-rc.N`, `-rc` |
| `stable` | _(none)_ | n/a |

---

## 4. Release Types (Previous)

A stable / GA tag may only be cut after the release-policy gates in `RELEASE_STRATEGY.md` are satisfied on `develop`.

Required evidence bundle:
- Wave 7 PASS on the current baseline
- green `release_critical` CI on `develop`
- no new CRITICAL findings in `server`, `llm`, and `sharding`
- required sanitizer, recovery, chaos/fault-injection, penetration-test, SLA, and runbook artefacts
- synchronized release/governance documentation (`ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `CHANGELOG.md`, branch/release/versioning docs)
- completed GA hardening execution batches (A-D) with boundary evidence updates in planning/status documents

Current batch tracking is maintained in `ROADMAP.md`, `NEXT_PHASE_IMPLEMENTATION_PLAN.md`, and `ai_working/NEXT_PHASE_STATUS.md`. Technical gates for Batch D (D-1..D-10) have passed. The final human governance sign-off (Section 9 of `docs/governance/GA_PROMOTION_SIGN_OFF.md`, gate D-11) is still pending and is the only remaining GA promotion blocker.

### 4.1 Stable / GA Promotion Evidence

| Release type | Approximate cadence |
|---|---|
| Stable MINOR | Every 6–8 weeks |
| Stable PATCH / Hotfix | As needed (P0 within 48 h, P1 within 1 week) |
| Release Candidate | 1–2 weeks before a stable release |

Release dates are tracked in [`CHANGELOG.md`](CHANGELOG.md) and announced via GitHub Releases.

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
