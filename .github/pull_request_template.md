# Pull Request

> For EPIC-branch workflow PRs, use `.github/PULL_REQUEST_TEMPLATE/epic-branch-flow.md`.
> This applies to both `feature/* -> epic/*` and `epic/* -> develop` integration PRs.

## Target Version (Required)

<!-- Select the version this PR targets. Must be an existing GitHub milestone.
     See docs/governance/PR_VERSION_TARGETING.md for guidance.
     Valid formats: v2.4.0, v2.4.1, v2.5.0-alpha1, v2.5.0-beta1, v2.5.0-rc1, [Unreleased]
     
     Selection criteria:
     - Features: assign to next planned MINOR (e.g., v2.5.0-alpha1)
     - Bug fixes (stable): assign to v2.4.0 or v2.4.1
     - Docs: same version as the documented feature
     - Security: current stable version first, then backports
     - Infrastructure: next MINOR or [Unreleased]
-->

**Target Version:** (choose one from active milestones)

<!-- Briefly explain why this version was chosen -->

---

## Description

<!-- Describe the changes in this PR -->
<!-- For AI-assisted PRs, use `.github/prompts/compose-ai-pr-report.prompt.md` and paste into `.github/copilot/PR_AI_REPORT_TEMPLATE.md` structure. -->

## Linked Issues

<!-- Reference related issues: "Closes #123", "Fixes #456", "Related to #789" -->

## Type of Change

- [ ] Bug fix (non-breaking)
- [ ] New feature (non-breaking)
- [ ] Refactoring (non-breaking)
- [ ] Documentation
- [ ] Breaking change (requires MAJOR version bump — see [VERSIONING.md](../VERSIONING.md))
- [ ] Security fix
- [ ] Other:

## Breaking Change Checklist

<!-- Only fill out if "Breaking change" is checked above -->

- [ ] MAJOR version bump planned in `VERSION` and `CMakeLists.txt`
- [ ] Migration guide added in `docs/migration/`
- [ ] Announcement prepared for GitHub Discussions (≥ 2 weeks before release)
- [ ] CHANGELOG `### Removed` / `### Changed` section updated

## Testing

- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing performed
- [ ] Benchmarks run (if performance-sensitive change)

## Security Tiering Impact (Required for Runtime Changes)

<!-- Reference model: ARCHITECTURE.md -> Security & Hardening Tiering Model -->

- Impacted tier(s):
  - [ ] T0 Trusted Core
  - [ ] T1 Security & Platform Services
  - [ ] T2 Data Plane Engines
  - [ ] T3 Interface & Protocol Edge
  - [ ] T4 Managed Extension Runtime
  - [ ] T5 Plugin Boundary
  - [ ] N/A (docs-only / non-runtime)

- [ ] Trust-boundary crossings documented in PR description (example: T3 -> T2, T5 -> T4 brokered call)
- [ ] Boundary controls validated for affected T3/T4/T5 paths (AuthN/AuthZ, validation, rate limits, audit)
- [ ] Boundary-focused tests added/updated or explicit N/A rationale provided
- [ ] If trust level/privilege increased, security maintainer approval is attached

## 📚 Research & Knowledge (wenn applicable)

- [ ] Diese PR basiert auf wissenschaftlichen Paper(s) oder Best Practices?
  - Falls JA: Research-Dateien in `/docs/research/` angelegt?
  - Falls JA: Im Modul-README unter "Wissenschaftliche Grundlagen" verlinkt?
  - Falls JA: In `/docs/research/implementation_influence/` eingetragen?

**Relevante Quellen:**
- [ ] Paper: <!-- docs/research/papers/<file>.md -->
- [ ] Best Practice: <!-- docs/research/best_practices/<file>.md -->
- [ ] Architecture Decision: <!-- docs/research/architecture_decisions/adr_<NNN>.md -->

## AI-Generated Code (KI-generierter Code)

<!-- Tool-Referenz: siehe `.github/instructions/cpp-language-service-tools.instructions.md` -->

- [ ] Symbol-Referenzen mit `GetSymbolReferences_CppTools` geprüft (siehe `.github/instructions/cpp-language-service-tools.instructions.md`)
- [ ] Keine rohen Pointer und kein `new`/`delete` ohne explizites Review eingeführt
- [ ] RAII und Exception-Safety für neue/angepasste Pfade geprüft
- [ ] Keine unnötig komplexen KI-Abstraktionen eingeführt
- [ ] Performance-Metriken geprüft, falls Hotpath betroffen

## AI Review Workflow (Required for AI-assisted PRs)

<!-- Use prompts under `.github/prompts/` with the `themisdb-reviewer` agent -->

- [ ] Findings-first review performed with `.github/prompts/pr-diff-findings-review.prompt.md`
- [ ] Security hardening review performed for security-sensitive/runtime changes with `.github/prompts/security-hardening-review.prompt.md` (or N/A documented)
- [ ] API impact review performed for API/contract changes with `.github/prompts/api-change-impact-review.prompt.md` (or N/A documented)
- [ ] All Critical/High findings are resolved or explicitly accepted with rationale in PR description
- [ ] Residual risks and follow-up actions documented in PR description
- [ ] Severity policy applied according to `.github/copilot/REVIEW_SEVERITY_POLICY.md`

## High-Finding Exception Record (only if High is accepted)

<!-- Validate this section with `.github/prompts/verify-high-exception-record.prompt.md` before merge. -->

- [ ] High-finding exception claimed in this PR

- Finding reference:
- Maintainer approver:
- Mitigation in current release:
- Target fix milestone:
- Tracking issue:
- Validation evidence:

## Release Readiness Gate (Required for release-scoped changes)

- [ ] Release readiness reviewed with `.github/prompts/release-readiness-check.prompt.md` for branch transition scope
- [ ] Branch governance validated against `BRANCHING_STRATEGY.md` and `RELEASE_STRATEGY.md`
- [ ] Versioning/changelog impact validated against `VERSIONING.md` and `CHANGELOG.md`

## Checklist

- [ ] Code follows project style guidelines (clang-format / clang-tidy)
- [ ] Self-review completed
- [ ] Documentation updated (if needed)
- [ ] CHANGELOG.md updated under `[Unreleased]`
- [ ] No new warnings introduced
- [ ] Security-sensitive paths reviewed by security maintainer (if applicable)

## Scanner and IntelliSense Gates

- [ ] IntelliSense/Compiler: no new errors in changed files
- [ ] clang-tidy/cppcheck: no new high-risk findings in changed files
- [ ] Gap Scanner: no new `critical` findings in categories `security`, `input_validation`, `query_correctness`, `distributed_consistency`, `concurrency`, `memory`
- [ ] Gap Scanner: no new `high` findings in the same categories (or explicitly approved)
- [ ] Gap Scanner delta report attached (baseline vs current), not only absolute totals
- [ ] New `unknown` scanner findings triaged (fixed, re-categorized, or justified)

