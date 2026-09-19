# ThemisDB macOS Lane GA Checklist

> Status: Active (release hardening)
> Scope: promotion of macos-release from experimental opt-in lane to default lane
> Source of truth: [RELEASE_TARGET_ARCHITECTURE.md](RELEASE_TARGET_ARCHITECTURE.md), [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md), [.github/workflows/release-build-matrix.yml](../../.github/workflows/release-build-matrix.yml)

## 1. Goal

This checklist defines objective release evidence required to promote the macos-release lane from opt-in (macos-release-only) to default execution in the canonical build matrix.

## 2. Mandatory Evidence Gates

All gates are mandatory.

### Gate A: Runner Stability

- [ ] At least 3 consecutive successful runs on GitHub-hosted macos-latest for release-build-matrix.yml with build_matrix=macos-release-only
- [ ] No flaky runner-only failures in setup/bootstrap/package steps across those 3 runs

Evidence:

- workflow run URLs
- step logs for setup/configure/build/validation/package

### Gate B: Packaging Output

- [ ] At least one distributable artifact produced by CPack on macOS (.dmg or .tar.gz)
- [ ] Artifact is uploaded via workflow artifact step and is downloadable
- [ ] Package install smoke check completed (manual or scripted) on macOS host

Evidence:

- CPack logs (cpack-*-macos-release.log)
- artifact listing
- smoke-check output

### Gate C: Manifest and Integrity

- [ ] SHA256SUMS.txt contains macOS artifact entries
- [ ] RELEASE_MANIFEST.txt contains macOS artifact entries
- [ ] checksum verification passes for macOS artifacts

Evidence:

- manifest excerpt
- checksum verification output

### Gate D: Naming and Policy Alignment

- [ ] Artifact naming follows canonical release naming policy
- [ ] Architecture naming is consistent (arm64 / x64) and documented
- [ ] No conflict with Linux/Windows naming conventions

Evidence:

- produced filenames
- documentation references updated if naming changed

### Gate E: Consumer Publication Definition

- [ ] Dedicated macOS consumer publication route documented (for example Homebrew and/or direct DMG/PKG)
- [ ] Ownership and rollback process for this consumer route documented
- [ ] Release notes template updated with macOS section

Evidence:

- linked workflow/docs PRs
- release notes example

## 3. Non-Blocking vs Blocking Rules

Current lane mode:

- experimental
- opt-in only (macos-release-only)

Promotion to default in all is allowed only when Gates A-E are all checked.

If any gate fails:

- keep lane opt-in
- open/track follow-up issue
- do not silently change release expectations in docs

## 4. Local Validation Notes

- Local act dry-runs validate workflow schema/orchestration only.
- Local act dry-runs do not validate native macos-latest runtime, toolchain, codesign/notarization, or installer behavior.
- Native compatibility evidence must come from GitHub-hosted macOS runs.

## 5. Promotion Change Set

When all gates pass, apply all changes in one PR:

- Update .github/workflows/release-build-matrix.yml so macos-release is included in build_matrix=all.
- Keep/adjust macos-release-only as a focused debug selector.
- Update .github/workflows/release-mainline.yml comments/options if matrix semantics change.
- Update docs:
  - docs/release/RELEASE_TARGET_ARCHITECTURE.md
  - docs/RELEASE_ARTIFACT_LOCATIONS.md
  - docs/RELEASE_PROCESS_QUICKREF.md
  - .github/WORKFLOW_REGISTRY.md

## 6. Sign-Off

- [ ] Release maintainer sign-off
- [ ] CI/governance sign-off
- [ ] Documentation sign-off
