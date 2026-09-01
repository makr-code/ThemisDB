# GitHub Action Pinning Policy

> **Status:** Active (2026-09-01)  
> **Scope:** All ThemisDB CI/CD workflows  
> **Objective:** Balance security, reliability, and cross-platform compatibility

## Overview

Version pinning for third-party GitHub Actions is governed by a **4-tier classification** that reduces cross-compilation friction (Docker/Linux/macOS/Windows) while maintaining security for critical workflows.

---

## Tier Classification

### Tier 1: MUST PIN (Security/Stability Critical)

**Requirement:** Full commit SHA pins (immutable reference)  
**Format:** `uses: owner/action@<40-char-sha> # v<semver>`

**Actions:**
- `github/codeql-action` — Static analysis gate; any unexpected version change risks security bypass
- `aquasecurity/fortify-action` — SAST compliance gate
- `github/security-consolidation` — Security correlation and reporting
- `softprops/action-gh-release` — Release signing and credential handling
- `./.github/actions/status-flags-and-issues` — Private plugin boundary enforcement
- `./.github/actions/manage-governance-issue` — Governance tracker mutations

**Rationale:**
- Prevents security bypass via silent action upgrades
- Protects sensitive workflows that manage credentials, signing, or access control
- Repository-internal actions require SHA pins to prevent accidental regressions

**Review Cadence:** Quarterly; any upgrade requires security sign-off

---

### Tier 2: SHOULD PIN (Reliability Critical)

**Requirement:** Semantic version tag (e.g., `@v4`) or broken SHA pin  
**Format:** `uses: owner/action@v<major> # <semver description>`

**Actions:**
- `actions/checkout` — Repo state is critical to all builds
- `actions/upload-artifact` — Test/build artifacts; may break if format changes
- `actions/download-artifact` — Artifact consumption
- `actions/setup-python` — Runtime selection for Python-based CI jobs
- `actions/setup-node` — Runtime selection for Node.js CI jobs
- `mozilla-actions/sccache-action` — Compiler cache setup

**Rationale:**
- Stability: Major version bumps are rare and signal real changes
- Compatibility: Patch and minor updates typically fix bugs and improve reliability
- Reduces maintenance burden: No need to chase every point release
- Allows cross-platform registry servers to resolve tags independently

**Review Cadence:** Semi-annually; upgrade on major version changes or security advisories

**Example:**
```yaml
- uses: actions/checkout@v4
  # Allows v4.0.0, v4.1.0, v4.2.2, etc.
  # Pin to v4 to auto-receive fixes within major version
```

---

### Tier 3: OPTIONAL PIN (Cross-Compilation Friendly)

**Requirement:** Semantic version tag (`@v0`, `@v1`) or `@latest`  
**Format:** `uses: owner/action@v<major> # <purpose>`

**Actions:**
- `anchore/sbom-action` — SBOM generation (non-critical tooling)
- `aquasecurity/trivy-action` — Dependency scanning (non-fatal advisory)
- `actions-rs/clippy-check` — Linting (fail-soft only)
- `github-super-linter/super-linter` — Format checking
- Custom linters/formatters (eslint, pylint, shellcheck via containers)

**Rationale:**
- Enables cross-platform builds to resolve versions independently
- Docker/Ubuntu/macOS registries may have divergent tag histories with full SHAs
- Improves workflow resilience when registries are under stress
- Simplifies local action validation in different environments

**Review Cadence:** Annually; subscribe to security advisories

**Example:**
```yaml
- uses: aquasecurity/trivy-action@v0
  # Allow v0.0.x, v0.1.x, v0.2.x
  # Platform registries resolve tag independently
```

---

### Tier 4: DYNAMIC VERSIONS (Maximum Compatibility)

**Requirement:** `@latest` or no version specifier  
**Format:** `uses: owner/action` or `uses: owner/action@latest`

**Actions:**
- Development/debugging tools (ad-hoc tracing, log capture)
- Temporary artifact exploration or diagnostics
- CI health dashboards and non-blocking observability jobs
- Preview features or experimental workflows (with clear exit date)

**Rationale:**
- Maximum cross-platform flexibility
- No risk to critical paths (observability-only or non-blocking)
- Simplest maintenance model for ephemeral jobs

**Review Cadence:** None required; mark with `# temporary`, `# debug`, or removal target date

**Example:**
```yaml
- uses: actions/upload-artifact@latest  # temporary: debugging issue #12345
- uses: some/tool  # will be removed after build stabilization (target: 2026-12-01)
```

---

## Tier Transition & Review Process

### Upgrading Within Tier
- **Tier 1 → Tier 1:** Requires security team approval
- **Tier 2 → Tier 2:** Auto-update to latest within major version; document in PR
- **Tier 3 → Tier 3:** Auto-update; no approval needed
- **Tier 4 → Tier 4:** Auto-update; implicit (always latest)

### Moving Between Tiers
- **Tier 1 ↔ Tier 2:** MUST include governance document update + security sign-off
- **Tier 2 ↔ Tier 3:** Reassess workflow safety; document rationale in PR
- **Any → Tier 4:** Only for non-critical/temporary jobs; include removal plan

### Deprecation Path
1. Mark action as deprecated with GitHub issue + PR comment
2. Notify maintainers of dependent workflows
3. Provide 2-week notice before enforcement
4. Migrate to replacement or remove non-critical workflow

---

## Tier 1 Allowlist (Security-Critical Actions)

These actions MUST always use full SHA pins:

| Action | Reason | Last Reviewed |
|--------|--------|---|
| `github/codeql-action` | SAST gate; prevents silent security bypass | 2026-09-01 |
| `aquasecurity/fortify-action` | SAST/DAST compliance gate | 2026-09-01 |
| `softprops/action-gh-release` | Release signing & credential handling | 2026-09-01 |
| `./.github/actions/status-flags-and-issues` | Private plugin boundary enforcement | 2026-09-01 |
| `./.github/actions/manage-governance-issue` | Governance tracker mutations | 2026-09-01 |
| `actions/create-release` (if used) | Release artifact signing | 2026-09-01 |

---

## Docker Base Image Versioning

### Policy
- **ubuntu:latest** — Preferred for CI (Ubuntu LTS patches automatically)
- **ubuntu:24.04** — Allowed for LTS stability; prefer ubuntu:latest
- **ubuntu:22.04** — Legacy; mark as deprecated; migrate to ubuntu:24.04 or latest
- **python:3.11-slim** — Preferred; allows Python 3.11.x patch updates
- **python:3.11-slim-bookworm** — Lock to Debian Bookworm; use only if required

**Rationale:**
- Base image minor version pins prevent cross-platform Docker pulls from diverging
- `ubuntu:latest` and `python:3.x-slim` allow patch updates (critical for security)
- Explicit `-bookworm` locks break compatibility with non-Debian registries

---

## sccache Version Pinning

**Current Policy:** Remove explicit version pin in `setup-cpp-build/action.yml`

**Rationale:**
- `mozilla-actions/sccache-action@v0.0.6` already pins to a stable action version
- Double-pinning (`version: "v0.8.1"`) creates cache invalidation across platforms
- Allow sccache-action to manage its own version compatibility

**Change:**
```yaml
# OLD (remove explicit version pin)
- uses: mozilla-actions/sccache-action@<sha>
  with:
    version: "v0.8.1"  # ← REMOVE THIS

# NEW (let sccache-action manage version)
- uses: mozilla-actions/sccache-action@v0.0.6
  # Action will use its default sccache version
```

---

## Compliance & Enforcement

### CI Gates
- **gate-pr-core.yml** validates that Tier 1 actions use full SHA pins
- **No enforcement** for Tier 2-4 (permissive; prefer documentation over CI gates)
- Actions violating their tier assignment will be caught in manual review

### Local Validation
```bash
# Lint workflows locally for pinning compliance
pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode lint
```

### Quarterly Audit
- Repository maintainers review action pinning against this policy
- Any tier misclassification is logged + corrected in next batch PR
- Security-critical actions (Tier 1) are audited before each major release

---

## FAQ

**Q: Why not pin everything to SHA?**  
A: Cross-compilation complexity (Docker/Linux/macOS/Windows) + maintenance burden (256+ SHAs to track). Tier 1 security-critical actions remain SHA-pinned.

**Q: What if an action fails with a newer version?**  
A: Report issue; revert to previous major version if needed; escalate to action owner if it's a regression.

**Q: Can I pin a Tier 3 action to SHA?**  
A: Yes; it's encouraged where possible, but not required. Semantic version tags are sufficient.

**Q: What about internal (`./.github/actions/`) actions?**  
A: Always use SHA pins (Tier 1). They're part of the codebase and subject to git history tracking.

---

## References

- `.github/WORKFLOW_GUIDELINES.md` — General workflow policy
- `.github/actions/` — Composite actions (all require SHA pins)
- `VERSIONING.md` — ThemisDB semantic versioning
