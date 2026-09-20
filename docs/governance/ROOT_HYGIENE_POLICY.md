# Root Hygiene Policy

**Author:** ThemisDB Contributors  
**Created:** 2026-09-20  
**Last Updated:** 2026-09-20  
**Status:** active

## Purpose
Define mandatory placement rules so the repository root stays focused on entry-point, governance, and release-critical documentation.

## Scope
Applies to files created or modified in the repository root (`/`).

## Allowed in Root (default)
- Repository entry docs: `README.md`, `QUICKSTART.md`, `SETUP.md`, `SUPPORT.md`
- Governance and release docs: `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `CHANGELOG.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, `BRANCHING_STRATEGY.md`, `DOCUMENTATION_GOVERNANCE.md`
- Security and contribution docs: `SECURITY.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `MAINTAINERS.md`
- Root navigation and architecture pointers: `INDEX.md`, `MODULE_INDEX.md`, `ARCHITECTURE.md`, `AUDIT.md`, `GOVERNANCE.md`, `SOP.md`, `LOG.md`, `CTEST.md`, `TARGET_ARCHITECTURE.md`

## Must Not Stay in Root
- Execution artifacts and local diagnostics (`*.log`, transient reports, temp outputs)
- One-off implementation reports and phase summaries
- Tool-specific prompts/agent payloads that can live under `.github/prompts` or `.github/agents`

## Placement Rules
- Build/test/runtime logs: `logs/local-snapshots/YYYY-MM-DD/root/` (local, non-versioned)
- Governance-related reports: `docs/reports/`
- Analysis documents: `docs/analysis/`
- Community/credits material: `docs/community/`
- Copilot and automation prompts: `.github/prompts/`
- Agent definitions: `.github/agents/`

## Enforcement
- New root `*.log` files must be moved out of root before commit.
- New root `*.md` files require explicit justification in the PR description.
- If a root move changes references, all affected links/config paths must be updated in the same change.

## Change Process
1. Move file(s) with history (`git mv`).
2. Update references via workspace search.
3. Verify root inventory (`*.md`, `*.log`) before merge.
4. Record governance-impacting decisions in `DOCUMENTATION_GOVERNANCE.md` when needed.
