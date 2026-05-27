# ThemisDB Release Strategy

> Status: Active
> Purpose: compact, manual, CI-free release process
>
> Canonical root onboarding path: [README.md](README.md) → [QUICKSTART.md](QUICKSTART.md) → [SETUP.md](SETUP.md) → [SUPPORT.md](SUPPORT.md) → [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md) → [INDEX.md](INDEX.md)

## 1. Principles

- `develop` is the working branch.
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

- `1.9.0`
- `1.9.1`
- `1.10.0`
- `1.9.0-alpha1`
- `1.9.0-beta1`
- `1.9.0-rc1`

Rules:

- `MAJOR`: incompatible change
- `MINOR`: new backward-compatible functionality
- `PATCH`: backward-compatible fix
- `alpha`, `beta`, `rc`: pre-release stages

Canonical pre-release suffixes are `-alphaN`, `-betaN`, and `-rcN`. Legacy forms (`-alpha`, `-beta.N`, `-rc.N`, `-rc`) are accepted only for historical tags/changelog entries.

## 2.1 Milestone Model Alignment (ROADMAP / FUTURE / CHANGELOG)

- Release scope is planned in `ROADMAP.md` milestone sections.
- Open enhancement backlog is tracked in `FUTURE_ENHANCEMENTS.md`.
- `CHANGELOG.md` entries must map to the milestone scope and reference the related enhancement/backlog item when applicable.

| `RELEASE_TYPE` value | Tag suffix example | Milestone naming pattern | Changelog entry pattern |
|---|---|---|---|
| `alpha` | `v1.9.0-alpha1` | `v1.9.0-alpha1` | `## [Unreleased]` until cut, then `## [1.9.0-alpha1] - YYYY-MM-DD` |
| `beta` | `v1.9.0-beta1` | `v1.9.0-beta1` | `## [Unreleased]` until cut, then `## [1.9.0-beta1] - YYYY-MM-DD` |
| `rc` | `v1.9.0-rc1` | `v1.9.0-rc1` | `## [Unreleased]` until cut, then `## [1.9.0-rc1] - YYYY-MM-DD` |
| `stable` | `v1.9.0` | `v1.9.0` | `## [Unreleased]` until cut, then `## [1.9.0] - YYYY-MM-DD` |

## 2.2 AI-/Agent Governance Alignment

- `COPILOT_INSTRUCTIONS.md` defines how AI/agent documentation updates must keep `VERSIONING.md`, this file, `CHANGELOG.md`, `ROADMAP.md`, and `FUTURE_ENHANCEMENTS.md` synchronized.
- Release documentation updates are only complete when versioning model, release type mapping, and changelog traceability remain consistent across these root documents.

## 3. Tags

Community tags:

- Stable: `vX.Y.Z`
- Pre-release: `vX.Y.Z-alphaN`, `vX.Y.Z-betaN`, `vX.Y.Z-rcN`

Enterprise tags:

- Stable: `enterprise-vX.Y.Z`
- Pre-release: `enterprise-vX.Y.Z-rcN`

Hyperscaler tags:

- Stable: `hyperscaler-vX.Y.Z`
- Pre-release: `hyperscaler-vX.Y.Z-rcN`

Rules:

- Tags are annotated or signed.
- Tags are created only after artefacts and release notes are ready.
- One tag may point to multiple packaged outputs.
- ZIP and MSI belong to the same release tag if they come from the same commit.

## 4. Branches

- `develop`: normal development
- `main`: community release lane
- `enterprise`: enterprise release lane
- `hyperscaler`: hyperscaler release lane

Rules:

- Do not develop directly on release lanes.
- Prepare a release on a temporary branch if needed.
- Merge the prepared state into the target release lane.
- Create the tag on the release lane after the final manual check.

## 5. Release Notes

Each release must have one note file.

Preferred paths:

- Community: `docs/de/releases/RELEASE_NOTES_vX.Y.Z.md`
- Community pre-release: `docs/de/releases/RELEASE_NOTES_vX.Y.Z-<suffix>.md`
- Enterprise: `docs/de/releases/RELEASE_NOTES_enterprise-vX.Y.Z.md`
- Hyperscaler: `docs/de/releases/RELEASE_NOTES_hyperscaler-vX.Y.Z.md`

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
- use `vX.Y.Z` for community releases
- use `enterprise-vX.Y.Z` for enterprise releases
- use `hyperscaler-vX.Y.Z` for hyperscaler releases
- pre-releases may use milestone names such as `v1.9.0-alpha` or `v1.9.0-rc.1`
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

### Community

1. Start from `develop`.
2. Create a temporary release branch if cleanup is needed.
3. Merge the final state into `main`.
4. Build the required artefacts manually.
5. Write the release notes.
6. Create the tag on `main`.
7. Publish the artefacts under the same release entry.

Example:

```bash
git checkout main
git pull --ff-only origin main
git tag -s v1.9.0-alpha -m "ThemisDB v1.9.0-alpha"
git push origin v1.9.0-alpha
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

## 8. Artefacts

Typical artefacts of one release:

- source archive
- ZIP package
- MSI package
- container image
- checksum file

Rules:

- Artefacts from the same commit share the same release tag.
- Artefact names may differ by platform or edition.
- Artefact checksums should be published together with the release.

## 9. Manual Checklist

Before tagging, verify manually:

- correct branch
- correct version in `VERSION`
- correct GitHub milestone exists
- release issues are assigned to the milestone
- blockers in the milestone are resolved or consciously deferred
- release notes exist
- artefacts were built successfully
- package contents are plausible
- checksums were generated if required
- no unintended local changes are included

## 10. Rollback

If a release must be reverted:

1. revert or fix on the release lane
2. create a new patch release
3. do not move or reuse an existing tag
4. update the release notes with the rollback context

Example:

```bash
git checkout main
git revert <commit>
git push origin main
git tag -s v1.9.1 -m "ThemisDB v1.9.1"
git push origin v1.9.1
```

## 11. Best-Practice Summary

- keep one tag per released source state
- keep one milestone per release scope
- keep issues linked to the correct milestone
- keep release notes mandatory
- keep pre-release tags explicit
- keep package variants under one release
- keep the process manual, short, and auditable

---
Zuletzt geprueft (Root-Sync): 2026-05-26

