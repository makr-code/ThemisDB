---
Author: "ThemisDB Contributors"
Created: "2026-03-15"
Last Updated: "2026-11-28"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB Release Strategy

> Status: Active
> Purpose: governed release and automated nightly publication process
>
> Canonical root onboarding path: [README.md](README.md) → [QUICKSTART.md](QUICKSTART.md) → [SETUP.md](SETUP.md) → [SUPPORT.md](SUPPORT.md) → [BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md) → [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) → [VERSIONING.md](VERSIONING.md)
>
> Release flow diagram reference: [docs/release/RELEASE_TARGET_ARCHITECTURE.md](docs/release/RELEASE_TARGET_ARCHITECTURE.md) · [docs/RELEASE_ARTIFACT_LOCATIONS.md](docs/RELEASE_ARTIFACT_LOCATIONS.md)

## 1. Principles

- `develop` is the working branch.
- The canonical branch and edition model is defined in `BRANCHING_STRATEGY.md`.
- Release branches are optional and short-lived.
- Regular releases require release-manager review; nightly prereleases use the automated flow in §8.
- One source tag identifies one released source state.
- A release may contain multiple artefacts, but those artefacts are produced once by the canonical CPack-backed release build matrix.
- Package availability is evaluated per OS/architecture lane. A failed lane does not block publication of other lanes when at least one distributable package exists and all produced packages pass runtime validation; the aggregate workflow status remains failed to expose the incomplete platform set.
- Nightly Docker manifest promotion remains multi-architecture and requires successful runtime-validated `amd64` and `arm64` images, even when a partial GitHub package release is available.
- Do not create separate Git tags for ZIP, MSI, Docker image, or other package variants.
- Every release tag must have release notes.
- GitHub Issues for a release must be linked via the corresponding milestone.
- Pull requests and routine development use fast, targeted validation to minimize resource usage and keep velocity high.
- Scheduled nightly builds perform the deeper full validation pass for integration, packaging, and distribution readiness.
- Regular release publication remains human-gated and is not triggered by every merge or push; nightly GitHub prereleases and validated Docker manifests use the automatic flow in §8.

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

- Regular release tags are annotated or signed; the automated nightly workflow creates a lightweight tag at the built commit.
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
4. Run the canonical CPack-backed release build to generate the release artefacts.
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
4. Run the canonical CPack-backed release build to generate the release artefacts.
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
4. Run the canonical CPack-backed release build to generate the release artefacts.
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
4. Run the canonical CPack-backed release build to generate the release artefacts.
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
4. Run the canonical CPack-backed release build to generate the release artefacts.
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

Nightly prereleases provide early access to development builds. A scheduled build is an opportunity to publish, not a guarantee that every run produces a release.

### Triggers

1. **Schedule:** `.github/workflows/build-mainline.yml` is scheduled daily at 02:00 UTC. On completion of that scheduled `develop` run, `.github/workflows/release-nightly.yml` handles a successful or failed upstream conclusion; the nightly workflow does not have its own cron. Its own package and runtime gates still apply.
2. **Manual:** Dispatch `release-nightly.yml`; `force=true` bypasses the no-change check. `dry_run` defaults to `true`.
3. **Changes:** Normally proceed only when commits exist since the last reachable published nightly release, or no such release exists. Draft releases and orphan tags are not baselines. Without changes and without force, publication is skipped.

### Workflow

Workflow: `.github/workflows/release-nightly.yml`

**Steps:**
1. Check for changes against the last reachable published nightly release and build the community package matrix at the selected source commit with the `nightly` release-type override.
2. Generate build metadata and the GitHub tag `v<version>-nightly.<YYYYMMDD>.<runnum>` from `VERSION`, the UTC date and workflow run number. The tag identifies the built source commit; an existing tag is not moved.
3. Create a draft GitHub prerelease, upload the produced package assets and `build-metadata.json`, then verify asset count, names and sizes and recheck the tag's source commit before publishing. Release notes contain build information and up to 20 recent commits since that published baseline (or from HEAD if none exists). A retry of the same run can resume only a matching tag and draft for the same source commit; it overwrites only expected asset names and verifies the complete asset set. A tag/ref mismatch, non-draft release, unexpected draft asset, or changed published-nightly baseline fails closed and leaves existing state untouched. Tags and published releases are never moved or deleted automatically; inspect and reconcile inconsistent state manually.
4. Build and validate Docker runtime candidates independently for `amd64` and `arm64`. Promote multi-architecture manifests only after GitHub prerelease publication succeeds and both validated platform digests are available.
5. After GitHub prerelease publication, reconcile per-version WinGet and Docker tracking issues. Nightly Linux and Windows distribution issue handoff is unavailable: their approval workflows currently target stable channels. Do not create those nightly issues until the downstream paths safely handle prereleases. A tracking issue is not a submission or proof of publication; the WinGet path requires separate opt-in and approval (§11.4).
6. Report individual job outcomes in the run summary and update the nightly status tracker for eligible publishing attempts and change-gate failures. No-change runs do not update the tracker. Inspect the jobs and assets rather than treating a notification as proof of success.

### Artifacts

Assets depend on which platform lanes and package generators succeed. The release includes the packages actually produced and `build-metadata.json`; do not assume every platform or format is present. Release notes contain the generated recent-change list. Inspect the release assets and run evidence for any additional artefacts.

### Versioning

- Nightly tag format: `v<major>.<minor>.<patch>-nightly.<YYYYMMDD>.<runnum>`
- Package builds use the `nightly` channel; package filenames need not contain the date or run number and are not a unique per-run CPack version. WiX/MSI uses the numeric base version, also without a date or run number. Consequently, package managers cannot reliably distinguish or upgrade successive nightly builds with the same base version; the unique GitHub tag does not change the installed package version.
- Docker Hub manifests: `themisdb/themisdb:nightly`, `themisdb/themisdb:nightly-YYYYMMDD`, and `themisdb/themisdb:<version>-nightly-YYYYMMDD`. GHCR receives `ghcr.io/<owner>/<repo>:nightly` and `:nightly-YYYYMMDD`. Date tags can be replaced by another run on the same day.

### Quality Gates

- Package publication requires at least one distributable package and successful runtime validation of every produced package. Missing failed lanes may yield a partial release, but a failed lane remains visible in the aggregate matrix and final status; a runtime validation failure or zero packages blocks package publication.
- Docker manifest promotion requires successful GitHub prerelease publication, successful runtime validation and recorded digests for **both** `amd64` and `arm64`, even if a partial GitHub package release is allowed. A failed Docker lane blocks promotion, not necessarily GitHub prerelease publication; a failed release publication blocks promotion.
- The separate approved Docker publication for a nightly tag fails closed unless the selected source itself satisfies the release-channel contract: the Docker context includes `RELEASE_TYPE`, the build stage copies it into `/src`, and CMake reads it. This semantic check tolerates unrelated changes in older source files, but tags that predate the contract cannot be approved without publishing a new tag. Do not move an existing tag; the source-preflight override does not bypass this contract.
- Nightlies are for early testing; use RC or stable for production.

### Rules

- Nightly releases are marked as `prerelease=true` on GitHub
- Nightly images do not receive the `latest` tag.
- WinGet is not submitted automatically. An explicit nightly version, prerelease opt-in, maintainer command and protected environment approval are required on the issue-driven path; a partial release without a suitable Windows ZIP cannot be submitted. Upstream acceptance and availability are not verified.
- A manual dry run builds and validates where the gates permit, but does not create a release, promote Docker manifests, or update tracking issues. Inspect its actual job results for skips and failures. Publishing runs can fail or be partial after some side effects have already occurred; they do not automatically roll back an already published release.

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
6. Publish the GitHub prerelease and use the controlled downstream publication workflows; WinGet preview requires the explicit opt-in below

**Example 2: Stable Release from RC**
1. PR merged with label `release/stable`
2. Workflow bumps: `2.5.0-rc1` → `2.5.0` (removes suffix)
3. Sets `RELEASE_TYPE=stable`
4. Creates PR with release checklist
5. After approval & merge, maintainer tags: `v2.5.0`
6. Publish the GitHub release and use the controlled downstream Docker and stable WinGet publication workflows

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

The stable ThemisDB Community package uses `ThemisDB.ThemisDB` in the [Windows Package Manager Community Repository](https://github.com/microsoft/winget-pkgs). Opt-in prerelease submissions use the separate `ThemisDB.ThemisDB.Preview` identifier; upstream acceptance of that package and its prerelease versions has not yet been proven.

### Installation for end users

Example commands below use `<version>` as a placeholder to avoid stale pinned examples.

```powershell
# Stable release
winget install ThemisDB.ThemisDB

# Specific version
winget install ThemisDB.ThemisDB --version <version>

# Upgrade to latest stable
winget upgrade ThemisDB.ThemisDB

# Preview, only after the package has been accepted upstream
winget install ThemisDB.ThemisDB.Preview
```

### Manifest files

Stable manifests live under `packaging/winget/manifests/t/ThemisDB/ThemisDB/<version>/` and consist of three required files; preview manifests use `packaging/winget/manifests/t/ThemisDB/ThemisDB/Preview/<version>/` and the `ThemisDB.ThemisDB.Preview` filename prefix:

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

These examples use stable defaults and must not be used unchanged for a
prerelease. For a prerelease, explicitly set the Preview identity, name,
moniker and portable command alias to match the workflow, and use the actual
published prerelease ZIP and checksum:

```powershell
pwsh scripts/release/new-winget-manifest.ps1 `
    -Version <prerelease-version> `
    -PackageIdentifier ThemisDB.ThemisDB.Preview `
    -PackageName "ThemisDB Preview" `
    -Moniker themisdb-preview `
    -PortableCommandAlias themis_server_preview `
    -InstallerType zip `
    -InstallerUrl <PUBLISHED_PRERELEASE_ZIP_URL> `
    -InstallerSha256 <SHA256_FROM_RELEASE> `
    -InstallerArchivePath <LOCAL_RELEASE_ZIP> `
    -PackageDependencies Microsoft.VCRedist.2015+.x64 `
    -IncludeGermanLocale
```

Versions with a pre-release suffix (`-alpha`, `-beta`, `-rc*`) are represented by the package version itself, for example `2.4.0-alpha`. The WinGet installer manifest does not accept an `IsPreRelease` field in the current schema.

### Submitting a stable release manually

```powershell
# Erstellt Fork, Branch, Commit und Draft-PR gegen microsoft/winget-pkgs
pwsh scripts/release/submit-winget-pkgs.ps1 `
    -Version <version> `
    -ForkOwner <github-username>
```

For a manually generated prerelease manifest, also pass
`-PackageIdentifier ThemisDB.ThemisDB.Preview` to the submission script;
its default is the stable identifier. Before using the direct scripts, verify
that the corresponding GitHub Release is published and non-draft. The submission
script independently checks the canonical `makr-code/ThemisDB` release and
rejects the stable identifier for a GitHub prerelease; manifest generation
alone does not check release state. The workflow's `allow_prerelease` opt-in
does not apply to direct script invocation. Submission requires a fork-write
`GH_TOKEN` or `-GitHubToken`; `RELEASE_GH_TOKEN`, when set, is used separately
to read release metadata, otherwise the fork token is used.

Rules:

- Submit only one version per PR. Wait for approval before submitting the next version.
- Stable is the default; prereleases require the separate opt-in policy below.
- Never submit a version with a placeholder SHA256.
- Validate locally before submitting: `winget validate --manifest packaging/winget/manifests/t/ThemisDB/ThemisDB/<version>`
- ZIP manifests must include `Microsoft.VCRedist.2015+.x64` so the portable binary can start on clean Windows machines.
- After submission remove the Draft status on the PR to trigger Microsoft's automated validation pipeline.

### Pre-release policy

The `release-winget.yml` manual dispatch and reusable call default to `allow_prerelease=false` and `dry_run=true`. Set `allow_prerelease=true` to opt in to a prerelease (a version with a suffix or a GitHub Release marked prerelease); set `dry_run=false` to request creation of a draft PR. Both stable and opted-in prerelease submissions require an existing, published, non-draft GitHub Release and its release assets. A dry run generates and validates manifests but does not submit a PR.

For `release-mainline.yml` manual dispatches, preflight checks out the selected workflow ref first, then switches to `tag_name` if that tag already exists. A not-yet-created tag is resolved against the selected workflow ref, allowing a dry-run build and payload validation without creating a tag, release, or publication PR. Actual publication remains separately approval-gated.

### Release metadata resolution

All downstream build and deployment paths resolve release and package metadata through `scripts/release/resolve_release_metadata.py` instead of hard-coded values:

| Workflow | Resolution | Metadata consumers |
|---|---|---|
| `release-winget.yml` | `version` input optional; blank resolves the newest published canonical `vX.Y.Z[-suffix]` release (prereleases only with `allow_prerelease=true`, nightly tags excluded). Requires exactly one non-empty `windows-x64/amd64 .zip` asset. | Installer URL from the GitHub asset, SHA-256 cross-checked against the GitHub asset digest, `-MetadataPath` for publisher, URLs, license, short description, release notes, tags |
| `release-windows-distribution.yml` | Explicit release tag; release must be published | Scoop `homepage`/`license`/`description`, Chocolatey `authors`/`projectUrl`/`licenseUrl`/`description`/`releaseNotes`/`tags`; JSON in `meta/release-metadata.json` |
| `release-linux-distribution.yml` | Explicit release tag; release must be published | DEB `Release` description; JSON validated and shipped in `meta/release-metadata.json` |
| `reusable-docker-image-platform.yml` | Repository metadata only (`--repository-only`) | OCI labels `source`, `url`, `documentation`, `description`, `licenses`, `vendor`, `version`, `revision` |

Field sources: GitHub repository `description`, `homepage` (invalid values such as `https:\\host` fall back to the repository URL with a warning), `topics` (tags), the license SPDX id (or the first line of `LICENSE` when GitHub reports `NOASSERTION`), the `LICENSE` copyright holder (publisher), and the GitHub Release body/URL. Missing description, missing license, unpublished/draft releases, non-ISO `published_at`, missing/ambiguous/empty assets, and invalid version or repository values stop the workflow with an `::error::` message. Payload validation (`validate_runtime_payload.py`), the separate Preview identifier, prerelease opt-in, `dry_run=true` defaults and human approval are unchanged.

### Correcting a defective published asset

The resolver does not repair release payloads. A published Windows ZIP that fails `validate_runtime_payload.py` (as `ThemisDB-COMMUNITY-2.4.0-alpha-windows-x64.zip` of `v2.4.0-alpha` does: `data/docs.db` and the bundled GGUF model are missing) must be replaced by a maintainer:

1. Preferred: cut a corrected prerelease (for example `v2.4.0-alpha.1`) through `release-mainline.yml` (`tag_name=2.4.0-alpha.1`, `edition=community`, `distribution=github`, first with `dry_run=true`, then `dry_run=false`) and approve it with `/publish-release`. Published tags are immutable and `stage-github-release.js` refuses to overwrite published releases.
2. Alternatively, a maintainer converts `v2.4.0-alpha` back to draft and re-runs `release-mainline.yml` for that tag; this temporarily withdraws an already downloaded release and needs explicit human approval.
3. Afterwards run `release-winget.yml` with `allow_prerelease=true` and `dry_run=true`; submit only after the dry run passes payload and checksum validation.

The separate `ThemisDB.ThemisDB.Preview` identifier is intended to isolate preview versions from `ThemisDB.ThemisDB` stable upgrades; that upgrade behavior remains to be verified. No prior stable PR merge is required by this policy. The issue-based approval path also requires explicit prerelease opt-in as described in §11.4. Upstream validation and acceptance of the Preview identifier and prerelease versions remain unproven: a draft PR is not publication, and availability must be verified after upstream review and merge.

## 11.1 Docker Distribution

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

Action: Maintain these decisions in this file once agreed; CI and `.github/agents/CMake-Agent.agent.md` should be updated to reflect chosen defaults.

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

## 11.4 Release Publishing Approval Gates (2026 Security Hardening)

Regular GitHub release publication and the issue-driven registry paths are human-gated to prevent accidental publication of incomplete or problematic releases. Nightly GitHub prereleases and validated nightly Docker manifests follow the automated exception in §8. The mainline and registry gates use commands on their respective tracking issues. The registry wrappers additionally require their protected GitHub Environment approval. Manual or reusable WinGet dispatch remains separately controlled and defaults to prerelease opt-out and dry-run.

### Flow Overview

```
release-mainline.yml (builds + signs artifacts)
        ↓
Stages draft GitHub Release + release-review issue
        ↓
Maintainer reviews metrics, checksums, signatures
        ↓
Posts standalone /publish-release comment
        ↓
Publishes GitHub Release; creates or reuses downstream tracking issues
        ↓
Each downstream issue requires its own command and environment approval;
monitor the dispatched workflow and verify its result separately
```

### GitHub Release Publication (`release-mainline-approval.yml`)

**Trigger:** A newly created whole comment containing exactly one command, with no additional text: `/publish-release`, `/approve-release`, or `@publish-release`. Use the lowercase commands shown, alone and not alongside review notes. Edited comments do not trigger commands; post a new comment for a retry. `/hold-release` or `/pause-release`, `/reject-release` or `/cancel-release`, and `/rollback-release` record the corresponding decision without publishing.

**Requirements:**
- Comment author must have `admin` or `maintain` role in repository
- Issue must be open and have the `release-review` label
- Release must exist on GitHub

**Action:**
1. Validates commenter permissions
2. Extracts version from issue body
3. Fetches the release by the issue version, downloads assets, verifies the checksum file and required manifest and archive, then publishes the draft
4. Creates or reuses a tracking issue per version and downstream registry, and comments with next steps
5. Leaves the review issue open; the command does not close it

**Example approval:**
```
/publish-release
```

Put review notes in a separate comment. Restaging the same draft reuses its open review issue without overwriting the human checklist; its restaging comment requires a fresh review because prior approvals may no longer apply. Duplicate review issues or a closed review issue block staging. Published releases cannot be overwritten by staging.

### GitHub Release → Downstream Approval Paths

After the GitHub Release exists, maintainers can independently approve these downstream publication paths:

1. **WinGet submission** (`release-winget-approval.yml`)
2. **Docker build + push** (`release-docker-approval.yml`)
3. **Linux distro bundle** (`release-linux-distro-approval.yml`)
4. **Windows distro bundle** (`release-windows-distro-approval.yml`)

### WinGet Community Publication (`release-winget-approval.yml`)

**Trigger:** A standalone whole-comment `/publish-winget`, `/approve-winget`, or `@publish-winget`, case-insensitive.

**Requirements:**
- Comment author must have `admin` or `maintain` role
- Issue must be open and have the `winget-release` label
- Corresponding GitHub Release must be published and non-draft

**Action:**
1. Validates commenter permissions
2. After protected environment approval, dispatches `release-winget.yml` with the version extracted from that issue
3. The downstream workflow resolves that published, non-draft GitHub Release and its Windows ZIP, validates the payload and generates manifests
4. It attempts to create a draft PR to `microsoft/winget-pkgs`; monitor the downstream run and PR separately
5. Community review and merge are required before verifying package availability

**Policy:**
- A version with a suffix or a GitHub Release marked prerelease requires explicit opt-in on the WinGet issue, as well as a published, non-draft release. Change the issue field from `**WinGet prerelease opt-in:** false` to `**WinGet prerelease opt-in:** true` before approving. Missing means false; invalid or duplicate fields fail the gate. The wrapper does not infer opt-in from the release.
- Community reviewers control merge timing (not automatic)

### Docker Registry Publication (`release-docker-approval.yml`)

**Trigger:** A standalone whole-comment `/publish-docker`, `/approve-docker`, or `@publish-docker`, case-insensitive.

**Requirements:**
- Comment author must have `admin` or `maintain` role
- Issue must be open and have the `docker-release` label
- Corresponding GitHub Release must exist

**Action:**
1. Validates commenter permissions
2. After protected environment approval, dispatches `release-docker-image.yml` for the issue version with `push_to_registry=true`
3. Comments that dispatch was requested; inspect the downstream run for build and publication results

**Security Note:**
- `release-docker-image.yml` defaults to `push_to_registry=false` in `workflow_dispatch` (manual safety)
- Must be explicitly set to `true` via approval gate
- OIDC-based auth to GHCR (no long-lived secrets)

### Linux and Windows Distribution Bundles

On the corresponding open tracking issue, use standalone `/publish-linux`, `/approve-linux`, or `@publish-linux` for `release-linux-distro-approval.yml`, and `/publish-windows`, `/approve-windows`, or `@publish-windows` for `release-windows-distro-approval.yml`. These wrappers dispatch the respective distribution workflows after environment approval; a dispatch is not confirmation that a bundle was published or that a community submission was merged.

All four registry gates also accept the standalone `/hold-<registry>` or `/pause-<registry>`, `/reject-<registry>` or `/cancel-<registry>`, and `/rollback-<registry>` commands, where `<registry>` is `winget`, `docker`, `linux`, or `windows`. These decisions do not dispatch the downstream workflow. Commands must be newly created comments; editing an existing comment does not trigger a command, so post a new comment to retry. Extra text or multiple commands are not valid approval comments.

After environment approval and before dispatch, the reusable registry gate rechecks that the issue is open and its body and required label have not changed. If that check fails, inspect the issue and post a fresh command after resolving the change.

### Approval Workflow Permission Model

All approval gates enforce the same permission model:

| User Role | Can Approve? | Reason |
|-----------|--------------|--------|
| `admin` | ✅ Yes | Repository owner |
| `maintain` | ✅ Yes | Release maintainer |
| `push` | ❌ No | Contributor, not maintainer |
| `triage` | ❌ No | Issue triager, not maintainer |
| `pull` | ❌ No | Read-only contributor |
| Not collaborator | ❌ No | External user |

### Tracking Issues

Downstream tracking issues are keyed by release version and registry. A retry reuses a matching open issue rather than creating another; a closed match or duplicate matches fail visibly and require human reconciliation. Registry wrappers skip comments on closed issues before requesting environment approval, and their summaries report the skip; the reusable gate fails if called directly with a closed issue. Neither reopens the issue. Legacy open issues receive updated instructions in an appended comment rather than having their human-edited bodies overwritten. A missing WinGet opt-in field remains false and is not automatically added. Idempotent issue creation does **not** make dispatch idempotent: repeated new approval comments can request multiple runs. Check existing runs before retrying.

The approval comment identifies the requested workflow and its run listing, not a confirmed downstream run or publication. Failures are visible in workflow results and the human-loop escalation issue. If GitHub publication succeeds but downstream issue creation fails, the release remains public and the review issue reports that reconciliation is needed. A gate summary reports job outcomes (`FAIL`, `CANCELLED`, `PASS WITH SKIPS`, or `PASS`); a successful wrapper summary is not evidence that the downstream workflow, upstream PR merge, or package publication succeeded.

**Labels:**
- `release-review` — GitHub Release approval tracking
- `winget-release` — WinGet submission approval tracking
- `docker-release` — Docker publication approval tracking
- `linux-distro-review`, `windows-distro-review` — respective distribution approval tracking

### Emergency Override Procedure

If a critical security issue requires immediate publication without normal review:

1. Contact release maintainer or repository owner (async escalation)
2. Approve normally via issue comment
3. Document the emergency context in the issue
4. Post-release incident review (within 48 hours)

**Note:** This procedure is for genuine security emergencies only. Habitual use indicates process problems that need fixing.

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
