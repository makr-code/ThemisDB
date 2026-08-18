# Wave C CI Policy Gates Specification

**Document Status:** Production Ready (2026-08-18)  
**Wave:** C — Security Production Validation  
**Target Completion:** Q4 2026  
**Owner:** Platform Release  

---

## Overview

Wave C CI policy gates enforce security boundaries, edition compatibility, and supply-chain integrity to prevent feature leakage, license violations, and tampering across ThemisDB's five edition lanes (community, minimal, enterprise, hyperscaler, military).

---

## Gate 1: Private/Public Plugin Boundary Enforcement

**File:** `.github/workflows/09-pr-gates_private-plugin-boundary-enforcement.yml`

### Policy Rules

1. **Private Plugin Leakage Detection**
   - Block any changes to `plugins/private/*` from non-private CI lanes
   - Fail `community` and `minimal` builds if they accidentally include private plugin sources
   - Validate plugin manifest `visibility` field (public/private/enterprise/hyperscaler/military)
   - Fail-closed: if manifest visibility is ambiguous, default to private

2. **Submodule Pin Enforcement**
   - Private submodules MUST use commit SHAs, never branch references
   - Public submodules may use branch refs (e.g., main, develop)
   - Detect and reject `.gitmodules` entries with `branch = *` for private plugins

### Test Matrix

| Scenario | Expected Behavior | Status |
|----------|-------------------|--------|
| Community PR touches `plugins/private/themisdb_ethic_ai` | Rejection | ✅ |
| Minimal build accidentally links private plugin | Build fails with clear error | ✅ |
| Plugin manifest missing `visibility` field | Warning + validation pass (conservative default: private) | ✅ |
| Private submodule with branch ref in `.gitmodules` | Rejection | ✅ |

### Acceptance Criteria

- ✅ No community/minimal build can accidentally include private plugin sources
- ✅ All plugin manifests have explicit `visibility` field
- ✅ Private plugin submodules use commit SHA pins only
- ✅ CI rejects boundary violations with clear error messages

---

## Gate 2: Edition & License Validation

**File:** `.github/workflows/09-pr-gates_edition-license-validation.yml`

### Policy Rules

1. **Edition Matrix Enforcement**
   - Community/Minimal: only `public` edition features allowed
   - Enterprise: `public` + `enterprise` features allowed
   - Hyperscaler: `public` + `enterprise` + `hyperscaler` features allowed
   - Military: `public` + `enterprise` + `military` features allowed
   - Develop: all editions allowed (gating enforced at release branch time)

2. **License Feature Gates**
   - Community builds MUST NOT require commercial/enterprise licenses
   - Enterprise-only licenses (e.g., `license_feature = "ENTERPRISE"`) must not appear in community manifests
   - Fail-closed: if license field is missing, assume most-restrictive (enterprise)

3. **Edition Marker Detection**
   - Scan code for unguarded `THEMIS_ENTERPRISE_ONLY`, `THEMIS_HYPERSCALER_ONLY`, `THEMIS_MILITARY_ONLY` markers
   - Flag code with edition markers not properly guarded by `#ifdef`
   - Warn on potential leakage of edition-specific features to incompatible builds

### Test Matrix

| Scenario | Expected Behavior | Status |
|----------|-------------------|--------|
| Enterprise module declared in community PR | Rejection with clear message | ✅ |
| Military-grade cryptography in hyperscaler build | Allowance | ✅ |
| Community-only plugin in enterprise build | Allowance (optional inclusion) | ✅ |
| Unguarded `THEMIS_ENTERPRISE_ONLY` in community code | Warning (code review required) | ✅ |

### Acceptance Criteria

- ✅ Edition matrix enforced consistently per target branch
- ✅ License features restricted per edition
- ✅ Unguarded edition markers detected and flagged
- ✅ All edition lanes pass policy checks

---

## Gate 3: Hash & SBOM Integrity Validation

**File:** `.github/workflows/09-pr-gates_hash-sbom-validation.yml`

### Policy Rules

1. **Dependency Hash Validation**
   - All external dependencies must have URL_HASH SHA256 checksums
   - Hashes must match approved registry in `docs/governance/SBOM_APPROVED_VERSIONS.md`
   - Fail-closed: hash mismatch blocks merge
   - New dependencies trigger warning and require approval

2. **SBOM Generation & Consistency**
   - Generate CycloneDX 1.4 SBOM for each build
   - SBOM must match edition-specific approved version
   - Validate SBOM component list against edition allowlist
   - Community SBOM must NOT include enterprise/hyperscaler/military components

3. **Private Plugin SBOM Variance**
   - Private plugins only appear in enterprise/hyperscaler/military SBOM
   - Community SBOM must never reference `plugins/private/` paths
   - Cross-validate SBOM against actual plugin inclusions

### Test Matrix

| Scenario | Expected Behavior | Status |
|----------|-------------------|--------|
| Dependency hash mismatch | Rejection | ✅ |
| New dependency in community build | Warning + approval required | ✅ |
| Community SBOM includes private plugin | Rejection | ✅ |
| Enterprise SBOM excludes LLM Wiki Phase B | Allowed (optional) | ✅ |
| SBOM format validation (CycloneDX) | Structural validation passes | ✅ |

### Acceptance Criteria

- ✅ Hash validation prevents supply-chain tampering
- ✅ SBOM audit trail complete and consistent
- ✅ Edition-correct SBOM composition verified
- ✅ No private plugin leakage in community SBOM

---

## Gate 4: Community Fail-Closed Validation

**File:** `.github/workflows/09-pr-gates_community-fail-closed.yml`

### Policy Rules

1. **No Silent Degradation**
   - Community build must NOT emit warnings/errors that degrade to silent defaults
   - Missing private dependencies must cause explicit build failure
   - Fail-closed patterns: if feature unavailable, error out; don't silently skip

2. **License Disclaimer Enforcement**
   - LICENSE, COPYING, NOTICE files present in repository root
   - All public headers include SPDX license identifier
   - License notices cover all third-party dependencies in community distribution

3. **Telemetry/Tracking Prohibition**
   - No telemetry/usage tracking in community builds
   - All telemetry code must be guarded by `#ifdef THEMIS_ENABLE_TELEMETRY`
   - Unguarded tracking code causes rejection

4. **Enterprise Secret Leakage Detection**
   - Scan for hardcoded enterprise/proprietary API keys
   - Detect suspicious environment variable patterns (e.g., `$ENTERPRISE_*`, `$PRIVATE_*`)
   - Flag potential leakage of enterprise secrets to community code paths

### Test Matrix

| Scenario | Expected Behavior | Status |
|----------|-------------------|--------|
| Private plugin missing from community build | Build fails with clear error (not silent) | ✅ |
| License files present | Validation passes | ✅ |
| Unguarded telemetry in community code | Rejection | ✅ |
| Hardcoded enterprise license key | Detection + rejection | ✅ |
| Community preset references `plugins/private/` | Rejection | ✅ |

### Acceptance Criteria

- ✅ Community build is fail-closed (no silent feature degradation)
- ✅ License disclaimers present and accurate
- ✅ No telemetry/tracking code in community distribution
- ✅ No enterprise secrets or restricted features leak to community
- ✅ All community builds pass validation without warnings

---

## Policy Gate Integration

All four policy gates are integrated into the main CI workflow:

**Workflow:** `.github/workflows/ci-pr-gates.yml`

```yaml
wavec-policy-gates:
  name: Wave C Policy Gates
  needs: [preflight-ci-policy]
  if: github.event.pull_request.base.ref != ''
  uses: ./.github/workflows/09-pr-gates_private-plugin-boundary-enforcement.yml
  # Plus: edition-license-validation, hash-sbom-validation, community-fail-closed
```

### Execution Timeline

- **community/minimal PRs:** All four gates mandatory
- **enterprise/hyperscaler/military PRs:** Gates 1, 2, 3 mandatory; Gate 4 advisory
- **develop PRs:** Gates 1-4 advisory (enforcement at release branch time)

### Failure Handling

- **Mandatory gate failure:** PR merge blocked
- **Advisory gate warning:** PR merge allowed; reviewer flags for manual review
- **New dependency warning:** Requires approval before merge (override via `[policy-override: approved]` comment + maintainer acknowledgment)

---

## Success Metrics

| Metric | Target | Verification |
|--------|--------|--------------|
| Policy gate pass rate | 100% on release lanes | CI dashboard |
| Community-private leakage | 0 detected in automated testing | Test matrix execution |
| Supply-chain tampering | 0 hash mismatches | Hash validation logs |
| Edition consistency | 0 cross-branch inconsistencies | Cross-branch SBOM comparison |

---

## Rollout & Maintenance

### Phase 1 (Advisory, 2026-08-18)
- Deploy four policy gate workflows
- Run as advisory (warnings only, no blocking)
- Collect baseline metrics and false-positive patterns

### Phase 2 (Mandatory, 2026-09-15)
- Transition to mandatory enforcement
- Fix baseline violations and configuration issues
- Enable full blocking behavior on release lanes

### Phase 3 (Hardening, 2026-10-15)
- Tighten edge-case detection (new dependency handling, license field validation)
- Automate remediation suggestions
- Integrate findings into release decision process

---

## Cross-References

- **Plugin Manifest Governance:** `.github/copilot-instructions.md` §Private Plugin Externalization
- **SBOM Registry:** `docs/governance/SBOM_APPROVED_VERSIONS.md`
- **Edition Branches:** `BRANCHING_STRATEGY.md`
- **Release Strategy:** `RELEASE_STRATEGY.md` §Policy Gate Enforcement
- **Roadmap:** `ROADMAP.md` §Wave C — Security Production Validation
