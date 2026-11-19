---
Author: "ThemisDB Team"
Created: "2026-10-09"
Last Updated: "2026-11-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

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

Select all that apply: bug fix (non-breaking), new feature (non-breaking), refactoring (non-breaking), documentation, breaking change (requires MAJOR version bump — see [VERSIONING.md](../VERSIONING.md)), security fix, other (specify).

## Breaking Change Checklist

<!-- Only applies to breaking changes. Copy applicable work items into the
     checklist below so the task gate does not treat irrelevant items as tasks. -->

- Plan the MAJOR version bump in `VERSION` and `CMakeLists.txt`.
- Add a migration guide in `docs/migration/`.
- Prepare the GitHub Discussions announcement (≥ 2 weeks before release).
- Update the CHANGELOG `### Removed` / `### Changed` section.

## Testing

- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing performed
- [ ] Benchmarks run (if performance-sensitive change)

## Security Tiering Impact (for Runtime Changes)

<!-- Reference model: ARCHITECTURE.md -> Security & Hardening Tiering Model -->

- Impacted tier(s): T0 Trusted Core / T1 Security & Platform Services / T2 Data Plane Engines / T3 Interface & Protocol Edge / T4 Managed Extension Runtime / T5 Plugin Boundary / N/A (docs-only / non-runtime)

For runtime changes, copy applicable trust-boundary, control-validation, test, and security-approval work into the checklist below.

## 📚 Research & Knowledge (wenn applicable)

Based on scientific papers or best practices? Yes / No.

**Relevante Quellen:**
- Paper: <!-- docs/research/papers/<file>.md -->
- Best Practice: <!-- docs/research/best_practices/<file>.md -->
- Architecture Decision: <!-- docs/research/architecture_decisions/adr_<NNN>.md -->
- If applicable, add research files, module README links, and implementation-influence entries as tasks in the checklist below.

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

For release-scoped changes, add the applicable review steps to the checklist below:

- Review release readiness with `.github/prompts/release-readiness-check.prompt.md` for branch transition scope.
- Validate branch governance against `BRANCHING_STRATEGY.md` and `RELEASE_STRATEGY.md`.
- Validate versioning/changelog impact against `VERSIONING.md` and `CHANGELOG.md`.

## Checklist

- [ ] Code follows project style guidelines (clang-format / clang-tidy)
- [ ] Self-review completed
- [ ] Documentation updated (if needed)
- [ ] Update CHANGELOG.md under `[Unreleased]`
- [ ] No new warnings introduced
- [ ] Security-sensitive paths reviewed by security maintainer (if applicable)

## Scanner and IntelliSense Gates

- [ ] IntelliSense/Compiler: no new errors in changed files
- [ ] clang-tidy/cppcheck: no new high-risk findings in changed files
- [ ] Gap Scanner: no new `critical` findings in categories `security`, `input_validation`, `query_correctness`, `distributed_consistency`, `concurrency`, `memory`
- [ ] Gap Scanner: no new `high` findings in the same categories (or explicitly approved)
- [ ] Gap Scanner delta report attached (baseline vs current), not only absolute totals
- [ ] New `unknown` scanner findings triaged (fixed, re-categorized, or justified)
