# ThemisDB Release Process Quick Reference

> **Status:** Active  
> **Purpose:** Decision tree and quick reference for ThemisDB release processes  
> **See also:** [RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md) · [VERSIONING.md](../VERSIONING.md) · [SOP.md](../SOP.md)

---

## Decision Tree: Which Release Type?

```
Start: You have code changes ready to publish

  ┌─────────────────────────────────────────────────────────────────┐
  │ Are these changes from continuous development on `develop`?     │
  │                                                                 │
  │ ├─ YES, daily → NIGHTLY RELEASE (automatic, 03:30 UTC)         │
  │ └─ NO, planned release → continue below                         │
  └─────────────────────────────────────────────────────────────────┘
           ↓

  ┌─────────────────────────────────────────────────────────────────┐
  │ Is this a critical security or P0 bug fix?                      │
  │                                                                 │
  │ ├─ YES → HOTFIX RELEASE (SOP-02)                               │
  │ │        Patch version bump, deploy within 48 hours            │
  │ │        (Branch from stable release tag)                       │
  │ └─ NO → continue below                                          │
  └─────────────────────────────────────────────────────────────────┘
           ↓

  ┌─────────────────────────────────────────────────────────────────┐
  │ Is this a release with new features/breaking changes?           │
  │                                                                 │
  │ ├─ YES, experimental → ALPHA RELEASE                           │
  │ │  (Label PR: release/alpha; version: 2.5.0-alpha1)           │
  │ │                                                               │
  │ ├─ YES, feature-complete → BETA RELEASE                        │
  │ │  (Label PR: release/beta; version: 2.5.0-beta1)             │
  │ │                                                               │
  │ ├─ YES, ready for testing → RC RELEASE                         │
  │ │  (Label PR: release/rc; version: 2.5.0-rc1)                 │
  │ │  (Feature-frozen, bug fixes only)                            │
  │ │                                                               │
  │ └─ NO, production-ready → STABLE RELEASE                       │
  │    (Label PR: release/stable; version: 2.5.0)                 │
  │    (Requires Wave 7 gates, security review, runbooks)         │
  └─────────────────────────────────────────────────────────────────┘
```

---

## Quick Commands

### Nightly Release (Automatic)

No action required; runs automatically every day at 03:30 UTC.

To trigger manually:
```bash
gh workflow run release-nightly.yml --ref develop
```

### Semi-Automatic Release (Alpha/Beta/RC/Stable)

1. **Set up:** Make sure your code is on `develop` and merged to a PR
2. **Label:** Apply one of these labels to the PR before or after merge:
   - `release/alpha`
   - `release/beta`
   - `release/rc`
   - `release/stable`
3. **Workflow runs:** After merge, the release workflow auto-creates a release PR
4. **Review:** Review and merge the release PR after ≥ 2 approvals
5. **Tag:** After merge, manually create and push the tag:
   ```bash
   git checkout develop
   git pull
   git tag -s v<new-version> -m "Release v<new-version>"
   git push origin v<new-version>
   ```
6. **Publish:** Tag push triggers the unified release orchestration

### Hotfix Release (P0/P1 Bug or Security Fix)

1. Create `hotfix/X.Y.Z-description` branch from the stable release tag
2. Apply the minimal fix
3. Follow [SOP-02](../SOP.md#sop-02--hotfix--patch-release)
4. Tag as `vX.Y.Z` and push

### Rollback a Release

If a release is broken post-publish:

```bash
gh workflow run release-rollback.yml \
  --ref develop \
  --field version=<broken-version> \
  --field delete-github-release=true \
  --field reason="<explanation>"
```

---

## Release Type Summary

| Type | Trigger | Version | Cadence | Workflows | Approval |
|---|---|---|---|---|---|
| **Nightly** | Automatic daily schedule | `v2.4.0-nightly.YYYYMMDD.N` | Daily 03:30 UTC | Auto `release-nightly.yml` | None (CI only) |
| **Alpha** | Label PR `release/alpha` | `v2.5.0-alpha1` | As needed | `release-promote.yml` → manual tag → `release-publish.yml` | ≥ 2 maintainers |
| **Beta** | Label PR `release/beta` | `v2.5.0-beta1` | As needed | `release-promote.yml` → manual tag → `release-publish.yml` | ≥ 2 maintainers |
| **RC** | Label PR `release/rc` | `v2.5.0-rc1` | 1–2 weeks before stable | `release-promote.yml` → manual tag → `release-publish.yml` | ≥ 2 maintainers |
| **Stable** | Label PR `release/stable` | `v2.5.0` | Every 6–8 weeks (MINOR) | `release-promote.yml` → manual tag → `release-publish.yml` | Wave 7 gates + ≥ 2 maintainers |
| **Hotfix** | P0/P1 bug / security fix | `vX.Y.Z` (PATCH bump) | As needed (48 h SLA) | Manual via [SOP-02](../SOP.md#sop-02--hotfix--patch-release) | ≥ 2 maintainers (P0: expedited) |

---

## Version Numbering Cheat Sheet

### Semantic Versioning Format

```
<major>.<minor>.<patch>[-suffix]
   2  .  5    .  0      [-rc1]
```

| Segment | Increments when |
|---|---|
| **major** | Incompatible API / wire protocol changes |
| **minor** | New backward-compatible features |
| **patch** | Backward-compatible bug fixes |
| **suffix** | Pre-release qualifiers (alpha, beta, rc) |

### Version Bump Rules for Releases

| Release Type | Current → Next | Example |
|---|---|---|
| **Alpha** (first) | Bump MINOR, add `-alpha1` | `2.4.0` → `2.5.0-alpha1` |
| **Beta** (from alpha) | Keep MINOR, increment suffix | `2.5.0-alpha2` → `2.5.0-beta1` |
| **RC** (from beta) | Keep MINOR, increment suffix | `2.5.0-beta1` → `2.5.0-rc1` |
| **Stable** (from rc) | Remove suffix | `2.5.0-rc1` → `2.5.0` |
| **Nightly** (daily) | Add `-nightly.YYYYMMDD.N` | `2.4.0-nightly.20260904.1234` |
| **Hotfix** (on stable) | Bump PATCH | `2.5.0` → `2.5.1` |

---

## Where Are Releases Published?

| Destination | Community | Enterprise | Hyperscaler | Military | Minimal |
|---|---|---|---|---|---|
| **GitHub Releases** | ✅ All types | ✅ Stable/RC | ✅ Stable | ✅ Stable | ✅ Stable |
| **Docker Hub** | ✅ Stable + pre-release | — | — | — | — |
| **GHCR** | ✅ Stable + pre-release | — | — | — | — |
| **WinGet** | ✅ Stable only | — | — | — | — |
| **DEB/RPM** | ✅ Stable + pre-release | — | — | — | — |
| **Private Registry** | — | ✅ (enterprise-reg) | ✅ (oem-reg) | ✅ (mil-reg) | — |

**See also:** [RELEASE_ARTIFACT_LOCATIONS.md](RELEASE_ARTIFACT_LOCATIONS.md)

---

## Verification Checklist Before Tagging

Before creating a release tag, verify manually:

**Version & Files:**
- [ ] Correct branch (`community`, `enterprise`, `military`, `hyperscaler`, `minimal`)
- [ ] Correct version in `VERSION` file
- [ ] Correct `RELEASE_TYPE` file
- [ ] CHANGELOG.md entries match the version

**CI & Quality:**
- [ ] All CI workflows green on the commit to be tagged
- [ ] For stable/rc: Wave 7 all-PASS on current baseline
- [ ] For stable/rc: `release_critical` CI green on `develop`
- [ ] For stable: top-risk module sign-offs (server, llm, sharding)

**Documentation & Governance:**
- [ ] Release notes prepared (if stable/rc)
- [ ] ROADMAP.md and FUTURE_ENHANCEMENTS.md synchronized
- [ ] No unintended local changes included

**Artifacts:**
- [ ] Build completed successfully
- [ ] Package contents plausible (no missing binaries)
- [ ] Checksums generated
- [ ] SBOM + supply-chain evidence ready
- [ ] WinGet manifests validated (if Windows package changed)

---

## Troubleshooting

**Q: When should I use nightly vs. alpha?**  
A: Nightly = automatic daily developer builds. Alpha = first intentional pre-release with API that may still change significantly.

**Q: Can I bump MAJOR or PATCH instead of MINOR?**  
A: Yes, use the manual dispatch option:
```bash
gh workflow run release-promote.yml \
  --field release_type=rc \
  --field version_bump=patch
```

**Q: What if CI fails after the release PR is created?**  
A: Push fixes to the release branch; CI will re-run automatically. Once green, request approval.

**Q: How do I verify the release published correctly?**  
A: Check the three registries:
1. GitHub Release: `gh release view v<version>`
2. Docker: `docker pull themisdb:v<version>`
3. WinGet: `winget show ThemisDB.ThemisDB --version <version>` (stable only)

---

## Related Documentation

- [RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md) — Comprehensive strategy, branches, phases
- [VERSIONING.md](../VERSIONING.md) — Semantic versioning rules, edition versioning
- [SOP.md](../SOP.md) — Detailed step-by-step procedures (SOP-01 through SOP-08)
- [RELEASE_ARTIFACT_LOCATIONS.md](RELEASE_ARTIFACT_LOCATIONS.md) — Where to find each artifact type
- [BUILD_METADATA_SPEC.md](BUILD_METADATA_SPEC.md) — Build metadata format and distribution

---
Zuletzt aktualisiert: 2026-09-04
