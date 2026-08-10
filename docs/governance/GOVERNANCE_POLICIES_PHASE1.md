# ThemisDB Governance Policies — Phase 1 Foundation

**Document Type:** Governance Policy Framework  
**Status:** Phase 1 Foundation (2026-08-10)  
**Version:** 1.0  
**Owner:** @makr-code / Platform Release  
**Enforcement Phase:** Phase 2 (CI automation) + Phase 3 (mandatory gates)

---

## Purpose

This document codifies governance policies that were previously implicit or documented in scattered files. It serves as the foundation for Phase 2 automation (CI checks, lint rules) and Phase 3 enforcement (policy violation gates).

---

## 1. Documentation Parity Policy

### 1.1 Principle

Every code change that affects public API, module behavior, or architecture must be reflected in documentation within the same pull request.

### 1.2 Scope

- **In Scope:** API signature changes, behavioral changes, feature additions, breaking changes
- **Out of Scope:** Internal refactoring without behavioral impact, bug fixes that don't change contract

### 1.3 Documentation Artifacts (Priority Order)

1. **L1 Source Docs:**
   - Doxygen `@file`, `@brief`, `@param`, `@return`, `@throws` comments on public APIs
   - Module-adjacent `src/<module>/ROADMAP.md` for status updates
   - Module-adjacent `src/<module>/FUTURE_ENHANCEMENTS.md` for planned work

2. **L2 Summaries:**
   - Module `README.md` files in `src/<module>/`
   - Cross-module technical specs in `docs/<category>/`

3. **L3 Root Governance:**
   - `ROADMAP.md` (milestones, phase completion)
   - `CHANGELOG.md` (release notes)
   - `SECURITY.md` (module security status)
   - `GOVERNANCE.md` (policy changes)

### 1.4 Enforcement Mechanism (Phase 2-3)

**Phase 2:**
- PR template includes "Documentation Updates" checkbox
- Code review checklist flags missing L1 updates
- Wiki/discussion links point to documentation parity policy

**Phase 3:**
- CI lint rule: detect code changes without corresponding doc updates (fail non-blocking)
- Mandatory review by Documentation Lead for any `.md` file touched
- CI gate: release-critical branch requires clean documentation lint

### 1.5 Violations & Remediation

| Violation | Severity | Remediation | Approval |
|-----------|----------|------------|----------|
| API change, no Doxygen | MEDIUM | Add Doxygen comments; PR review required | Maintainer + Doc Lead |
| Module ROADMAP not updated for phase completion | MEDIUM | Update phase markers; roadmap review | Module Owner + Project Lead |
| CHANGELOG missing for release-critical PR | HIGH | Add entry before merge; release manager review | Release Manager |
| Behavioral change, no architecture doc update | MEDIUM | Update `docs/architecture/` or FUTURE_ENHANCEMENTS.md | Project Lead |

---

## 2. Branch Governance Enforcement Policy

### 2.1 Canonical Branch Names

**Permanent canonical branches (only):**
- `develop` (working branch, all phases of work)
- `community` (release lane for open-source builds)
- `minimal` (release lane for minimal feature set)
- `enterprise` (release lane for enterprise features)
- `hyperscaler` (release lane for hyperscaler-grade performance)
- `military` (release lane for government/military features)

**Legacy names (DEPRECATED - must not be used in new workflows):**
- ❌ `main` → replaced by `community`
- ❌ `millitary` (misspelling) → replaced by `military`

### 2.2 Enforcement Rules

1. **CI Rejection:** Any PR targeting legacy branch names is rejected with clear message pointing to BRANCHING_STRATEGY.md
2. **Workflow Validation:** GitHub Actions workflows must validate branch name against canonical list
3. **Documentation:** All automation, CI, release docs must use canonical names only
4. **Submodule Pins:** Private plugin submodule references must use canonical branch names in `.gitmodules`

### 2.3 Enforcement Mechanism (Phase 2-3)

**Phase 1 (Audit):**
- Scan all `.github/workflows/` for legacy branch references
- Scan all `CMakeLists.txt` for hardcoded branch assumptions
- Document all found violations

**Phase 2:**
- Add GitHub Action: `branch-name-validator.yml` (reusable workflow)
- Reject PRs with legacy names in branch target or workflow references
- Notify contributors of correct canonical name to use

**Phase 3:**
- Mandatory branch governance gate on all release-critical PRs
- Audit trail: log all branch name violations monthly

---

## 3. Private Plugin Sourcing Policy

### 3.1 Principle

Community and Minimal edition builds must succeed without private credentials, private source code, or private artefacts. Private plugins are consumed only through commit-pinned submodules that degrade gracefully when absent.

### 3.2 Scope

**Community Build Rules:**
- [ ] No private credentials in environment variables, `.gitmodules`, or CMake
- [ ] No private repository paths copied into public source tree
- [ ] Missing private submodules must not cause configure/build failures
- [ ] Private-gated features must use runtime flags (not compile-time)

**Private Edition Build Rules:**
- [ ] Private submodules checked out via scoped GitHub App credentials (not personal tokens)
- [ ] All private submodule paths mirror canonical plugin names
- [ ] Edition gating enforced at runtime via manifest metadata

### 3.3 Private Plugin Registry

| Plugin | Private Repo | Submodule Path | Status | Allowed Editions |
|--------|-------------|----------------|--------|------------------|
| ethics_ai | makr-code/themisdb_ethic_ai | `plugins/private/themisdb_ethic_ai/` | Wave 1 | enterprise/hyperscaler/military |
| storage | makr-code/themisdb_storage | `plugins/private/themisdb_storage/` | Wave 1 | enterprise/hyperscaler/military |
| importer | makr-code/themisdb_importer | `plugins/private/themisdb_importer/` | Wave 1 | enterprise/hyperscaler/military |
| llm_wiki | makr-code/themisdb_llm_wiki | `plugins/private/themisdb_llm_wiki/` | Wave 1 | enterprise/hyperscaler/military |

### 3.4 Enforcement Mechanism (Phase 2-3)

**Phase 1 (Audit):**
- Audit `.gitmodules` consistency (all Wave-1 plugins registered with correct paths)
- Verify `plugins/CMakeLists.txt` has no hard-fail dependencies on private submodules
- Check Community build paths for stray private credential references

**Phase 2:**
- Add CI gate: `private-plugin-boundary.yml` (already exists, enhance it)
- CI check: Community builds succeed with all private submodules removed
- Artifact-leakage scan: verify package contents don't contain private code

**Phase 3:**
- Mandatory private-plugin boundary check on all PRs touching `plugins/private/**`, `.gitmodules`, or private CMake
- Monthly audit: scan all releases for source/artifact leakage

---

## 4. Edition Compliance Policy

### 4.1 Principle

Features, code paths, and optimizations designated for military/hyperscaler editions must never activate or leak into Community/Minimal builds, even if the source is present.

### 4.2 Edition-Gating Mechanism

**Compile-time gating** (allowed):
- CMake feature flags: `WITH_MILITARY_FEATURES`, `WITH_HYPERSCALER_OPTIMIZATIONS`
- C++ preprocessor: `#ifdef THEMIS_MILITARY_BUILD`
- Conditional source registration in `CMakeLists.txt`

**Runtime gating** (preferred):
- Plugin manifest `allowed_editions` metadata
- License feature checks at runtime
- RBAC/tenant configuration checks

### 4.3 Edition Compliance Rules

| Edition | Exclusive Features | Gating Method | Verification |
|---------|-------------------|---------------|--------------|
| **Community** | None (open baseline) | N/A | Community CI passes without private plugins |
| **Minimal** | Reduced tensor/distributed support | CMake flags | Minimal CI passes with reduced feature set |
| **Enterprise** | User storage, multi-tenancy, RBAC | Runtime manifest checks | Enterprise CI gates on license presence |
| **Hyperscaler** | GPU acceleration, distributed sharding, NUMA tuning | CMake + runtime flags | Hyperscaler CI gates on CUDA availability |
| **Military** | HSM integration, FIPS compliance, provenance audit | Compile-time + runtime | Military CI gates on HSM credentials |

### 4.4 Enforcement Mechanism (Phase 2-3)

**Phase 1 (Audit):**
- Scan source for military/hyperscaler feature flags; verify they're guarded
- Check CMakeLists.txt for unguarded exclusive feature compilation
- Verify plugin manifests express edition gating consistently

**Phase 2:**
- Add CI job: build each edition separately, verify no cross-edition leakage
- Runtime check: log edition gating decisions on startup
- Manifest schema validation: enforce `allowed_editions` field presence

**Phase 3:**
- Edition compliance gate: fail release if exclusive features leak to wrong edition
- Security audit: quarterly scan for unintended edition feature activation

---

## 5. Breaking Change Policy

### 5.1 Principle

Breaking changes require explicit approval, clear communication, and migration guidance before release.

### 5.2 Breaking Change Definition

**Qualifies as breaking change:**
- Public API signature change (function prototype, struct layout)
- Serialization format change (protocol incompatibility)
- Behavioral change that existing code depends on
- Configuration schema change requiring manual updates
- Data model schema change (database migration required)

**Does NOT qualify (safe changes):**
- Internal implementation refactoring (private APIs)
- Performance optimizations with same interface
- Bug fixes that restore intended behavior
- New optional features that don't affect existing code

### 5.3 Breaking Change Announcement

**Mandatory Artifacts:**
1. Version bump: MAJOR version increment (e.g., `2.x.x` → `3.0.0`)
2. CHANGELOG entry: explicit "BREAKING CHANGES" section
   ```markdown
   ## [3.0.0] - 2026-12-01
   ### Breaking Changes
   - `process_manager.h`: `commitModel(model)` → `commitModel(model, timeout_ms)` 
     [Migration: all callers must specify timeout]
   ```
3. Migration guide: `docs/migration/MIGRATION_2.x_TO_3.0.md`
   - Before/after code examples
   - Rollback procedures
   - Timeline for support (e.g., "2.x receives patch support until 2027-01")

**Approval Gate:**
- Project Lead must explicitly approve breaking change
- Security Lead must review if change affects security model
- Release manager must coordinate communication timing

### 5.4 Enforcement Mechanism (Phase 2-3)

**Phase 1:**
- Document all known breaking changes required for future releases

**Phase 2:**
- CI check: MAJOR version bump requires MIGRATION guide + CHANGELOG section
- PR template: "Does this introduce breaking changes?" checkbox with guidance

**Phase 3:**
- Release gate: fail promotion if breaking change without migration guide
- Deprecation tracking: maintain deprecation budget per release

---

## 6. Release Promotion & Gate Enforcement

### 6.1 Release Promotion Workflow

```
Code changes on develop
    ↓ (must pass release_critical CI)
CI Green on develop (all Wave 7-9 gates PASS)
    ↓ (Batch D checklist + human sign-off required)
GA_PROMOTION_SIGN_OFF.md §9 human sign-off
    ↓
Tag v2.4.0
    ↓
Merge tag commit to community branch
    ↓
Package + publish (Community/Enterprise/Military variants)
    ↓
Release published
```

### 6.2 Human Sign-Off Gates (Phase 1)

**D-1 through D-10:** Automated verification (CI + evidence registry)  
**D-11:** HUMAN SIGN-OFF (Blocker — only remaining gate before v2.4.0 GA)

**Who can sign off:**
- Project Lead (@makr-code)
- Release Manager (designated role)
- Security Lead (for security gates)

**Sign-off evidence:**
- GitHub commit signature
- Timestamp in GA_PROMOTION_SIGN_OFF.md §9
- Approval comment on release PR

---

## 7. Policy Violation Escalation

### 7.1 Severity Levels

| Level | Impact | Approval Required | Timeline |
|-------|--------|------------------|----------|
| **Critical** | Blocks release or exposes security risk | Project Lead + Security Lead | Immediate (within 24h) |
| **High** | Violates mandatory gate; non-compliant code ship risk | Project Lead + Maintainer | Within 3 business days |
| **Medium** | Best practice violation; documentation drift | Maintainer | Within 1 sprint |
| **Low** | Style/process guidance; non-blocking | Contributor feedback loop | Next sprint |

### 7.2 Violation Tracking

**Phase 1:** Manual tracking via GitHub issues (tagged `policy-violation`)  
**Phase 2:** Automated tracking via CI reporting (dashboard)  
**Phase 3:** Monthly compliance report to platform lead

---

## 8. Policy Review & Update Cadence

| Policy | Review Frequency | Owner | Last Updated |
|--------|------------------|-------|--------------|
| Documentation Parity | Quarterly | Documentation Lead | 2026-08-10 |
| Branch Governance | Per release | Platform Release | 2026-08-10 |
| Private Plugin Sourcing | Per wave release | Platform Release | 2026-08-10 |
| Edition Compliance | Per release | Security Lead | 2026-08-10 |
| Breaking Changes | Per release | Project Lead | 2026-08-10 |
| Release Promotion | Quarterly | Release Manager | 2026-08-10 |

**Next Full Policy Review:** Q4 2026 (after Phase 2 automation complete)

---

## 9. Policy Appendix: Cross-References

- **Release Strategy:** `RELEASE_STRATEGY.md` §2.3 (RC-to-Stable gate model)
- **Branching Strategy:** `BRANCHING_STRATEGY.md` (canonical branch definitions)
- **Documentation Governance:** `DOCUMENTATION_GOVERNANCE.md` (L0-L4 hierarchy)
- **GA Promotion Gate:** `docs/governance/GA_PROMOTION_SIGN_OFF.md` §9 (human sign-off)
- **Evidence Registry:** `docs/governance/MATURITY_EVIDENCE_REGISTRY.md` (gate tracking)

---

**Approval Status:** Awaiting platform release review (Phase 1 foundation, enforcement begins Phase 2)  
**Last Updated:** 2026-08-10  
**Next Review:** 2026-08-17 (weekly Phase 1 checkpoint)
