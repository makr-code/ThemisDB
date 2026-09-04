# ThemisDB Release Strategy

> Status: Active
> Purpose: compact, manual, CI-free release process
>
> Canonical root onboarding path: [README.md](README.md) → [QUICKSTART.md](QUICKSTART.md) → [SETUP.md](SETUP.md) → [SUPPORT.md](SUPPORT.md) → [BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md) → [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) → [VERSIONING.md](VERSIONING.md)

## 1. Principles

- `develop` is the working branch.
- The canonical branch and edition model is defined in `BRANCHING_STRATEGY.md`.
- Release branches are optional and short-lived.
- Releases are created manually by a release manager.
- One source tag identifies one released source state.
- A release may contain multiple artefacts.
- Do not create separate Git tags for ZIP, MSI, Docker image, or other package variants.
- Every release tag must have release notes.
- GitHub Issues for a release must be linked via the corresponding milestone.

## 2. Versioning

ThemisDB uses Semantic Versioning.

Allowed examples:

- `2.4.0`
- `2.4.1`
- `2.5.0`
- `2.5.0-alpha1`

| `RELEASE_TYPE` value | Tag suffix example | Milestone naming pattern | Changelog entry pattern |
|---|---|---|---|

## 2.1.1 PR Version Targeting Integration

Every pull request MUST declare a **target version** that maps to a GitHub milestone. This ensures:

- **Traceability**: each PR is linked to its planned release
- **Changelog automation**: PR metadata feeds into `CHANGELOG.md` entries
- **Release scope validation**: maintainers can verify PR scope against planned features

**Rules:**
- PR author selects from active milestones in `.github/pull_request_template.md` (Target Version field)
- Milestone must exist on GitHub before PR is merged
- PR is assigned the corresponding milestone by automation or manual review
- Release manager uses milestone scope to validate release readiness

See [docs/governance/PR_VERSION_TARGETING.md](docs/governance/PR_VERSION_TARGETING.md) for full policy, selection criteria, and release manager workflow.

## 2.2 AI-/Agent Governance Alignment

- `COPILOT_INSTRUCTIONS.md` and `.github/copilot-instructions.md` define how AI/agent documentation updates must keep `BRANCHING_STRATEGY.md`, `VERSIONING.md`, this file, `CHANGELOG.md`, `ROADMAP.md`, and `FUTURE_ENHANCEMENTS.md` synchronized.
- Release documentation updates are only complete when versioning model, release type mapping, branch model, and changelog traceability remain consistent across these root documents.

The root evidence set for this path is maintained in `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `CHANGELOG.md`, and the referenced test/benchmark/runbook artefacts.
## 2.4 Private Plugin Release And Packaging Rules
- Community release lanes (`develop` validation, `community` publication) must not require private credentials, private submodule checkout, or private artefact packaging.
- Private plugins are consumed only through commit-pinned submodules under `plugins/private/*`, and those submodule paths should mirror the current plugin names after the corresponding private repositories are provisioned.
- Private edition publication must use scoped credentials (GitHub App or deploy key) per family/edition; personal credentials and unscoped machine tokens are not allowed.
- Community publication requires both a source-leakage check (no private paths copied into public trees) and an artefact-leakage check (package contents, symbol/strings scans, and SBOM review).
- Manifest compatibility fields (`allowed_editions`, `license_feature`, `min_themisdb_version`, `max_themisdb_version`, `compatible_core_abi`) are part of the release acceptance contract for private plugins.

## 2.5 Release Hardening Execution Batches

For the current release-candidate hardening path, execution is tracked in four mandatory batches:

1. **Batch A** — status/evidence synchronization and gate-board alignment
2. **Batch B** — sharding Phase 6 consistency/recovery sign-off completion
3. **Batch C** — Wave 8 + chaos/fault-injection + sanitizer + penetration-test evidence
4. **Batch D** — operations/SLA/runbook/governance final readiness and controlled promotion

Batch boundaries are not advisory: each batch requires updated evidence references in `ROADMAP.md`, `NEXT_PHASE_IMPLEMENTATION_PLAN.md`, and `ai_working/NEXT_PHASE_STATUS.md` before the next batch starts.

Current tracked state: Batch A complete; Batch B complete (including sharding WAL/failover boundary evidence closure); Batch C closed — sanitizer evidence (`docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`: ASan/UBSan/TSan 0 new defects) and pentest evidence (`security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`: 0 new Critical/High, PTR-01/PTR-02 accepted) delivered; Batch D technical gates complete, with final human governance sign-off pending at `docs/governance/GA_PROMOTION_SIGN_OFF.md` Section 9 (D-11).

## 3. Tags

Minimal tags:

- Stable: `minimal-vX.Y.Z`
- Pre-release: `minimal-vX.Y.Z-rcN`

Community tags:

- Stable: `vX.Y.Z`
- Pre-release: `vX.Y.Z-alphaN`, `vX.Y.Z-betaN`, `vX.Y.Z-rcN`

Enterprise tags:

- Stable: `enterprise-vX.Y.Z`
- Pre-release: `enterprise-vX.Y.Z-rcN`

Hyperscaler tags:

- Stable: `hyperscaler-vX.Y.Z`
- Pre-release: `hyperscaler-vX.Y.Z-rcN`

Military tags:

- Stable: `military-vX.Y.Z`
- Pre-release: `military-vX.Y.Z-rcN`

Rules:

- Tags are annotated or signed.
- Tags are created only after artefacts and release notes are ready.
- One tag may point to multiple packaged outputs.
- ZIP and MSI belong to the same release tag if they come from the same commit.

### 3.1 Docker Image Tagging (Release Alignment)

For Docker publication, image tags must preserve the canonical release tag identity and must not be derived from fallback timestamps during release publication.

Rules:

- For release publication, Docker image tags are derived from the resolved canonical Git tag at the release commit (`vX.Y.Z`, `enterprise-vX.Y.Z`, `hyperscaler-vX.Y.Z`, `military-vX.Y.Z`, `minimal-vX.Y.Z`).
- In `workflow_run`-triggered Docker publication, version resolution must use tags that point to the triggering `head_sha`.
- If a release-triggered publication cannot resolve a canonical release tag at `head_sha`, publication must fail fast.
- Timestamp/dev fallback versions are allowed only for non-release/manual development publication paths.

## 4. Branches

Canonical permanent branches:

- `develop`: normal development and integration
- `minimal`: minimal release lane
- `community`: community release lane
- `enterprise`: enterprise release lane
- `hyperscaler`: hyperscaler release lane
- `military`: military release lane

Legacy names:

- `main`: historical community release lane, replaced by `community`
- `millitary`: historical misspelling, replaced by `military`

Rules:

- Do not develop directly on release lanes.
- Prepare a release on a temporary branch if needed.
- Merge the prepared state into the target release lane.
- Create the tag on the release lane after the final manual check.
- Do not use `main` or `millitary` for new release preparation.

## 5. Release Notes

Each release must have one note file.

Preferred paths:

- Minimal: `docs/de/releases/RELEASE_NOTES_minimal-vX.Y.Z.md`
- Community: `docs/de/releases/RELEASE_NOTES_vX.Y.Z.md`
- Community pre-release: `docs/de/releases/RELEASE_NOTES_vX.Y.Z-<suffix>.md`
- Enterprise: `docs/de/releases/RELEASE_NOTES_enterprise-vX.Y.Z.md`
- Hyperscaler: `docs/de/releases/RELEASE_NOTES_hyperscaler-vX.Y.Z.md`
- Military: `docs/de/releases/RELEASE_NOTES_military-vX.Y.Z.md`

Minimum content:

- version
- date
- release type
- milestone
- scope
- included artefacts
- important fixes
- breaking changes
- upgrade notes
- rollback note

## 6. Milestones And Issues

Every release uses one GitHub milestone as the planning and closure anchor.

Rules:

- before a release starts, create the corresponding GitHub milestone
- milestone name should match the release version or release lane
- use `minimal-vX.Y.Z` for minimal releases
- use `vX.Y.Z` for community releases
- use `enterprise-vX.Y.Z` for enterprise releases
- use `hyperscaler-vX.Y.Z` for hyperscaler releases
- use `military-vX.Y.Z` for military releases
- pre-releases may use milestone names such as `v1.9.0-alpha1` or `v1.9.0-rc1`
- every issue intended for the release must be assigned to that milestone
- issues without a matching milestone are not part of the release scope
- release notes should reference the milestone as the scope anchor
- before tagging, check that the milestone scope is in a releasable state

Minimum milestone hygiene:

- title is final
- release scope is clear
- open blocking issues are known
- closed issues actually belong to the shipped release
- postponed issues are moved to a later milestone

## 7. Manual Release Flow

### Minimal

1. Start from `develop`.
2. Create a temporary release branch if cleanup is needed.
3. Merge the final state into `minimal`.
4. Build the required artefacts manually.
5. Write the release notes.
6. Create the tag on `minimal`.
7. Publish the artefacts under the same release entry.

Example:

```bash
git checkout minimal
git pull --ff-only origin minimal
git tag -s minimal-v1.9.0 -m "ThemisDB Minimal v1.9.0"
git push origin minimal-v1.9.0
```

### Community

1. Start from `develop`.
2. Create a temporary release branch if cleanup is needed.
3. Merge the final state into `community`.
4. Build the required artefacts manually.
5. Write the release notes.
6. Create the tag on `community`.
7. Publish the artefacts under the same release entry.

Example:

```bash
git checkout community
git pull --ff-only origin community
git tag -s v1.9.0-alpha1 -m "ThemisDB v1.9.0-alpha1"
git push origin v1.9.0-alpha1
```

### Enterprise

1. Start from `develop`.
2. Prepare the release state.
3. Merge into `enterprise`.
4. Build artefacts manually.
5. Write the release notes.
6. Create the tag on `enterprise`.
7. Publish the artefacts privately.

Example:

```bash
git checkout enterprise
git pull --ff-only origin enterprise
git tag -s enterprise-v1.9.0 -m "ThemisDB Enterprise v1.9.0"
git push origin enterprise-v1.9.0
```

### Hyperscaler

1. Start from `develop`.
2. Prepare the release state.
3. Merge into `hyperscaler`.
4. Build artefacts manually.
5. Write the release notes.
6. Create the tag on `hyperscaler`.
7. Publish the artefacts privately.

Example:

```bash
git checkout hyperscaler
git pull --ff-only origin hyperscaler
git tag -s hyperscaler-v1.9.0 -m "ThemisDB Hyperscaler v1.9.0"
git push origin hyperscaler-v1.9.0
```

### Military

1. Start from `develop`.
2. Prepare the release state.
3. Merge into `military`.
4. Build artefacts manually.
5. Write the release notes.
6. Create the tag on `military`.
7. Publish the artefacts privately.

Example:

```bash
git checkout military
git pull --ff-only origin military
git tag -s military-v1.9.0 -m "ThemisDB Military v1.9.0"
git push origin military-v1.9.0
```

## 8. Nightly Release Flow (Automated)

Nightly releases are generated automatically every day at 03:30 UTC (or on manual trigger) to provide early access to development builds.

### Triggers

1. **Schedule:** Daily at 03:30 UTC
2. **Manual:** Workflow dispatch with optional force flag
3. **Condition:** Only if commits exist since last nightly release (skip if no changes)

### Workflow

Workflow: `.github/workflows/release-nightly.yml`

**Steps:**
1. Detect commits since last nightly via git tags
2. Set `RELEASE_TYPE=nightly`
3. Generate nightly version: `v2.4.0-nightly.YYYYMMDD.runnum` (e.g., `v2.4.0-nightly.20260904.1234`)
4. Build community edition (matrix: linux-release, windows-release)
5. Generate build metadata (timestamp, commit hash, build number, release type)
6. Create GitHub Pre-release with tag `v2.4.0-nightly.YYYYMMDD`
7. Push Docker images:
   - `themisdb:nightly`
   - `themisdb:nightly-YYYYMMDD`
   - `themisdb:2.4.0-nightly-YYYYMMDD`
8. Attach build-metadata.json, SBOM, and changelog to release
9. Post notification to GitHub Discussions (pinned nightly tracker issue)

### Artifacts

Nightly releases include:
- Linux: `.tar.gz`, `.zip`, `.deb`, `.rpm`
- Windows: `.zip`
- Build metadata: `build-metadata.json`
- Software Bill of Materials: `sbom-source.cyclonedx.json`
- Nightly changelog (generated from commits since last nightly)

### Versioning

- Nightly tag format: `v<major>.<minor>.<patch>-nightly.<YYYYMMDD>.<runnum>`
- Docker tags:
  - `themisdb:nightly` (always latest nightly)
  - `themisdb:nightly-YYYYMMDD` (specific date)
  - `themisdb:2.4.0-nightly-YYYYMMDD` (full version with date)
- OCI labels: `org.opencontainers.image.revision` (git commit), `io.themisdb.build.type=nightly`

### Quality Gates

- No additional CI gates beyond standard build verification
- Nightly releases are for early testing; use RC or stable for production

### Rules

- Nightly releases are marked as `prerelease=true` on GitHub
- Pre-release images do not receive the `latest` tag
- Nightly artifacts are not submitted to WinGet
- Nightly changelog is auto-generated from `git log` since last nightly
- Old nightly releases (>30 days) may be cleaned up to manage storage

---

## 9. Semi-Automatic Release via PR Labels (Alpha/Beta/RC/Stable)

Semi-automatic releases are triggered by applying release labels to pull requests. Upon PR merge, the workflow automatically bumps the version, updates changelog, and creates a release PR.

### Workflow

Workflow: `.github/workflows/release-promote.yml`

### Trigger Mechanisms

**Method 1: Label on PR**
1. Author or maintainer applies label to PR before merge:
   - `release/alpha` → promotes to alpha release
   - `release/beta` → promotes to beta release
   - `release/rc` → promotes to release candidate
   - `release/stable` → promotes to stable release
2. PR is merged into `develop`
3. Workflow triggers automatically (via `pull_request` closed event)

**Method 2: Manual Dispatch**
1. Call workflow via GitHub UI or CLI with inputs:
   - `release_type`: alpha|beta|rc|stable
   - `version_bump`: major|minor|patch (defaults to minor)
   - `target_branch`: develop (default) or alternative

### Workflow Steps

1. **Extract Release Context:** Determine release type from label or manual input
2. **Bump Version:** Read current VERSION, determine bump type, write new version
   - Default: MINOR bump for new releases
   - Override: Input `version_bump` parameter for MAJOR or PATCH
3. **Update RELEASE_TYPE:** Write `RELEASE_TYPE` file with new type (alpha|beta|rc|stable)
4. **Update CHANGELOG:** Add new section `## [<new-version>] - YYYY-MM-DD` with git log entries
5. **Create Release PR:** 
   - Branch: `release/v<new-version>`
   - Target: `develop`
   - Includes PR checklist for manual verification
6. **Notify Source PR:** Comment on the original PR with promotion status

### Release PR Checklist

The auto-generated release PR includes this checklist:

- [ ] CI gates passed (build, test, lint)
- [ ] Changelog entries are accurate
- [ ] VERSIONING.md examples updated if needed
- [ ] No breaking changes, or breaking changes documented in CHANGELOG
- [ ] Release notes drafted (if stable/rc)
- [ ] All documentation synchronized
- [ ] Two approvals required (maintainers)

### Approval & Merge

1. Release PR must receive **≥ 2 approvals** from maintainers
2. All CI gates must pass (`edition-community-ci`, PR gates)
3. Maintainer merges release PR into `develop`
4. After merge, maintainer manually creates the release tag:
   ```bash
   git checkout develop
   git tag -s v<new-version> -m "Release v<new-version>"
   git push origin v<new-version>
   ```
5. Tag push triggers `release-publish.yml` (unified orchestration)

### Versioning Rules

| Release Type | Version Bump | Example |
|---|---|---|
| Alpha | MINOR (default) | `2.4.0` → `2.5.0-alpha1` |
| Beta | MINOR (default) | `2.5.0-alpha2` → `2.5.0-beta1` |
| RC | PATCH (auto-increment) | `2.5.0-beta1` → `2.5.0-rc1` |
| Stable | PATCH (auto-increment) | `2.5.0-rc1` → `2.5.0` |
| Hotfix | PATCH (manual) | `2.5.0` → `2.5.1` |

### Examples

**Example 1: Alpha Release**
1. PR merged with label `release/alpha`
2. Workflow bumps: `2.4.0` → `2.5.0-alpha1`
3. Sets `RELEASE_TYPE=alpha`
4. Creates PR: `release/v2.5.0-alpha1` → `develop`
5. After approval & merge, maintainer tags: `v2.5.0-alpha1`
6. Tag triggers `release-publish.yml` → builds and publishes to all registries

**Example 2: Stable Release from RC**
1. PR merged with label `release/stable`
2. Workflow bumps: `2.5.0-rc1` → `2.5.0` (removes suffix)
3. Sets `RELEASE_TYPE=stable`
4. Creates PR with release checklist
5. After approval & merge, maintainer tags: `v2.5.0`
6. Tag triggers `release-publish.yml` → builds, publishes to GitHub + Docker + WinGet

---

## 8. Historical Release and Tag Reassignment Policy

Historical releases and tags must be reassigned logically to the canonical edition lanes, but published tags should normally remain immutable.

## 10. Historical Release and Tag Reassignment Policy

A Git tag points to a commit, not to a branch. Therefore, historical release correction should prefer branch and documentation alignment over tag rewriting.

### 10.1 Core rule

1. identify the intended edition and canonical target branch
2. ensure the tagged release commit is reachable from the correct canonical edition branch
3. correct release notes, changelog, and governance references
4. only retag if the tag is clearly internal/unpublished and explicit human approval exists

### 10.2 Preferred correction order

Historical Community releases cut from `main` should be treated as Community releases and migrated logically to `community`.

### 10.3 Community migration rule

Historical Military releases associated with `millitary` should be treated as Military releases and migrated logically to `military`.

### 10.4 Military migration rule

Published or externally consumed tags should not be force-moved as a normal migration step.

If a historical tag is wrong, preferred remedies are:

- preserve the old tag as historical fact
- create a corrected replacement release if necessary
- document the correction in release notes and governance docs

### 10.5 Published tag immutability

Each historical tag/release should be inventoried with:

- tag name
- commit SHA
- publication status
- intended edition
- current reachable branches
- canonical target branch
- required corrective action

### 10.6 Required historical inventory

Typical artefacts of one release:

- source archive
- ZIP package
- MSI package
- container image
- checksum file
- SBOM artefacts (`sbom-source.cyclonedx.json`, `sbom-source.spdx.json`, `sbom-source.SHA256SUMS`, `sbom-vcpkg-baseline-verification.json`)

Rules:

- Artefacts from the same commit share the same release tag.
- Artefact names may differ by platform or edition.
- Artefact checksums should be published together with the release.

## 11. Artefacts

ThemisDB Community releases are published to the [Windows Package Manager Community Repository](https://github.com/microsoft/winget-pkgs) under the identifier `ThemisDB.ThemisDB`.

### Installation for end users

Example commands below use `<version>` as a placeholder to avoid stale pinned examples.

```powershell
# Stable release
winget install ThemisDB.ThemisDB

# Specific version
winget install ThemisDB.ThemisDB --version <version>

# Upgrade to latest stable
winget upgrade ThemisDB.ThemisDB
```

### Manifest files

Manifests live under `packaging/winget/manifests/t/ThemisDB/ThemisDB/<version>/` and consist of three required files:

| File | Purpose |
|---|---|
| `ThemisDB.ThemisDB.yaml` | Version manifest |
| `ThemisDB.ThemisDB.installer.yaml` | Installer type, URL, SHA256 |
| `ThemisDB.ThemisDB.locale.en-US.yaml` | English metadata (required default locale) |
| `ThemisDB.ThemisDB.locale.de-DE.yaml` | German locale (optional, generated alongside) |

### Generating manifests

```powershell
# From a published GitHub Release asset (ZIP)
pwsh scripts/release/new-winget-manifest.ps1 `
    -Version <version> `
    -InstallerType zip `
    -InstallerUrl https://github.com/makr-code/ThemisDB/releases/download/v<version>/themisdb-<version>-community-binary-x64.zip `
    -InstallerSha256 <SHA256_FROM_RELEASE> `
    -PackageDependencies Microsoft.VCRedist.2015+.x64 `
    -IncludeGermanLocale

# From an MSI release
pwsh scripts/release/new-winget-manifest.ps1 `
    -Version <version> `
    -InstallerType msi `
    -InstallerUrl https://github.com/makr-code/ThemisDB/releases/download/v<version>/ThemisDB-COMMUNITY-<version>-windows-x64.msi `
    -InstallerSha256 <SHA256_FROM_RELEASE> `
    -IncludeGermanLocale
```

Versions with a pre-release suffix (`-alpha`, `-beta`, `-rc*`) are represented by the package version itself, for example `2.4.0-alpha`. The WinGet installer manifest does not accept an `IsPreRelease` field in the current schema.

### Submitting a release

```powershell
# Erstellt Fork, Branch, Commit und Draft-PR gegen microsoft/winget-pkgs
pwsh scripts/release/submit-winget-pkgs.ps1 `
    -Version <version> `
    -ForkOwner <github-username>
```

Rules:

- Submit only one version per PR. Wait for approval before submitting the next version.
- Submit stable releases first. RC/alpha may follow after the stable PR is merged.
- Never submit a version with a placeholder SHA256.
- Validate locally before submitting: `winget validate --manifest packaging/winget/manifests/t/ThemisDB/ThemisDB/<version>`
- ZIP manifests must include `Microsoft.VCRedist.2015+.x64` so the portable binary can start on clean Windows machines.
- After submission remove the Draft status on the PR to trigger Microsoft's automated validation pipeline.

### Pre-release policy

Pre-release versions (`-rc*`, `-alpha`, `-beta`) are published to winget-pkgs only after the corresponding stable release is accepted. This prevents `winget upgrade` from pushing pre-release software to users who installed a stable version.

## 11.1 Windows Package Manager (WinGet) Distribution

ThemisDB Community images are published to Docker Hub as `themisdb/themisdb`.

### Image tags

| Tag | Meaning |
|---|---|
| `themisdb/themisdb:<version>` | Pinned version |
| `themisdb/themisdb:2.4` | Minor-version floating tag |
| `themisdb/themisdb:latest` | Latest stable release only |

Pre-release versions (any version with a `-` suffix) do **not** receive the `latest` tag.

### CI pipeline

The `.github/workflows/docker-image.yml` workflow runs after a successful `.github/workflows/ci-release.yml` release run and only publishes when the configured release/publication gates allow it.

Required secrets: `DOCKERHUB_USERNAME`, `DOCKERHUB_TOKEN`.

Platforms: `linux/amd64`, `linux/arm64` (via Docker Buildx + GitHub Actions cache).

Dockerfile: `docker/Dockerfile.unified` with `--build-arg THEMIS_EDITION=COMMUNITY`.

### Local build and push

```bash
# Build + push stable release
TAG=<version> PUSH=true bash scripts/build-docker.sh

# Build only (no push, for local testing)
TAG=<version> bash scripts/build-docker.sh

# Multi-arch (requires buildx builder)
PLATFORMS=linux/amd64,linux/arm64 TAG=<version> PUSH=true bash scripts/build-docker.sh
```

```powershell
# Windows (PowerShell)
.\scripts\build-docker.ps1 -Tag <version> -Push
```

### Rules

- Never push a pre-release image as `latest`.
- Image name must be `themisdb/themisdb` (not `themisdb/themis`).
- One image build per released commit — do not rebuild the same tag from a different commit.

## 11.2 Docker Distribution

ThemisDB provides DEB, RPM, and TGZ packages for Linux server deployments.

### Formats

| Format | Targets | Generator |
|---|---|---|
| `.deb` | Debian 12+, Ubuntu 22.04+ | `cpack -G DEB` |
| `.rpm` | RHEL 9+, Fedora 39+ | `cpack -G RPM` |
| `.tar.gz` | Any Linux x86\_64 | `cpack -G TGZ` |

### CI pipeline

Linux native package generation is handled in `.github/workflows/ci-release.yml` by the `build-matrix` Linux `linux-release` lane and consolidated by the `package` job.

All generated Linux distributables (`.tar.gz`, `.zip`, `.deb`, `.rpm`) plus `SHA256SUMS.txt` and `RELEASE_MANIFEST.txt` are uploaded as the release package artifact set and attached to the GitHub Release.

### Local packaging

```bash
# Build + package (TGZ + DEB + checksums)
bash scripts/build-linux.sh --skip-tests
```

The CPack step is skipped in debug builds and can be suppressed with `SKIP_PACKAGE=1`.

### Checklist before publishing

- Verify checksums: `sha256sum -c *.sha256`
- Smoke-test the DEB on a clean Debian/Ubuntu container before attaching to the GitHub Release.
- RPM packaging fails gracefully when `rpmbuild` is unavailable; validate separately if RPM is required.

This project centralizes packaging decisions here to make release behavior deterministic. The following questions are open and should be decided by the release maintainer. Suggested defaults are provided.

1) Primary package formats to produce and test in CI
	- Options: `ZIP`/`TGZ`, `DEB`/`RPM`, `WIX`/`MSI`, `Docker` images
	- Suggested default: Produce `ZIP` and `TGZ` for all release lanes in CI as primary artifacts. Add `DEB`/`RPM` for server-targeted lanes (community/enterprise/hyperscaler) and `WIX`/`MSI` for Windows-focused releases when required.

2) PR automation policy for packaging-related changes
	- Options: `SuggestOnly` (agent produces patches and PR text) or `AutoPR` (agent creates branch + PR automatically)
	- Suggested default: `SuggestOnly` (manual branch/PR creation by maintainer). Enable `AutoPR` only after an explicit opt-in and with codeowner verification.

3) Artifact signing and repository
	- Questions: Where should artifacts be signed (CI post-processing vs integrated)? Which artifact repository will be used (GitHub Releases, Artifactory, Nexus)? Are there per-edition signing/licensing requirements?
	- Suggested default: Keep signing as a post-CPack CI step that publishes signed artifacts to GitHub Releases for community lanes and to the enterprise artifact store for private lanes. Keep license files in `LICENSE.*` at repo root and reference them from `CPackConfig.cmake` per edition.

4) Release gating rules for packaging
	- Questions: Should packaging generation be gated by additional manual checks (e.g., security/signature verification) before tag creation?
	- Suggested default: Require manual verification of signatures and checksums before creating the release tag. CI may produce artifacts and checksums but tagging remains a human-confirmed action (matches repository policy).

Action: Maintain these decisions in this file once agreed; CI and `.agent.md` should be updated to reflect chosen defaults.

## 11.3 Linux Native Package Distribution

Before tagging, verify manually:

- correct branch
- correct version in `VERSION`
- correct GitHub milestone exists
- release issues are assigned to the milestone
- blockers in the milestone are resolved or consciously deferred
- Wave 7 evidence shows all six PASS gates on the current baseline
- `release_critical` CI is green on `develop`
- top-risk module sign-off exists for `server`, `llm`, and `sharding`
- resilience/security/operations artefacts (Wave 5/6 retention, Wave 8 or equivalent, chaos/fault injection, sanitizer/recovery, penetration test, SLA, runbooks) are complete or explicitly deferred with approval
- release notes exist
- artefacts were built successfully
- package contents are plausible
- LLM bundle check: release packages from `windows-release`, `linux-release`, and `community-release` presets must include TinyLlama GGUF runtime payload under `models/` (source default: `models`)
- Licensing constraint: Gemma-family models are local-test assets only and must not be distributed in public/community release artifacts.
- checksums were generated if required
- SBOM + Supply-Chain evidence was generated and archived (policy: `docs/de/security/security_sbom.md`)
- WinGet manifests were regenerated and validated (`winget validate`) if a Windows ZIP or MSI artefact changed
- no unintended local changes are included

For historical reassignment work, also verify:

- canonical edition is identified correctly
- target release commit is reachable from the canonical branch
- legacy branch references are documented or removed as planned
- no published tag is rewritten without explicit approval

## 12. Manual Checklist

If a release must be reverted:

1. revert or fix on the release lane
2. create a new patch release
3. do not move or reuse an existing tag
4. update the release notes with the rollback context

Example:

```bash
git checkout community
git revert <commit>
git push origin community
git tag -s v1.9.1 -m "ThemisDB v1.9.1"
git push origin v1.9.1
```

## 13. Rollback

- keep one tag per released source state
- keep one milestone per release scope
- keep issues linked to the correct milestone
- keep release notes mandatory
- keep pre-release tags explicit
- keep package variants under one release
- keep WinGet manifests in sync with published GitHub Release assets
- keep the process manual, short, and auditable
- keep branch and edition naming aligned with `BRANCHING_STRATEGY.md`
- prefer canonical branch alignment over rewriting published tags

---
Zuletzt geprueft (Root-Sync): 2026-07-27
