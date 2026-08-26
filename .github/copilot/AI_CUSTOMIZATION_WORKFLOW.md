# ThemisDB AI Customization Workflow

This playbook defines how to use the workspace prompts, agents, and instructions for daily development.

## Objectives

- Keep AI-assisted changes small, verifiable, and production-oriented.
- Enforce roadmap-first implementation and branch governance.
- Standardize findings-first review before merge and release.

## Active Building Blocks

### Instructions

- `.github/instructions/themisdb-ai-delivery.instructions.md`
- Existing C++ instructions under `.github/instructions/`

### Agents

- `.github/agents/themisdb-implementer.agent.md`
- `.github/agents/themisdb-reviewer.agent.md`

### Prompts

- `.github/prompts/roadmap-to-production.prompt.md`
- `.github/prompts/build-triage-windows-release.prompt.md`
- `.github/prompts/security-hardening-review.prompt.md`
- `.github/prompts/api-change-impact-review.prompt.md`
- `.github/prompts/pr-diff-findings-review.prompt.md`
- `.github/prompts/release-readiness-check.prompt.md`
- `.github/prompts/compose-ai-pr-report.prompt.md`
- `.github/prompts/verify-high-exception-record.prompt.md`

## Recommended Team Flow

1. Plan and implement
- Use `Roadmap To Production` for roadmap or feature work.
- Use `Build Triage Windows Release` for failing build or test targets.

2. Review before merge
- Run `PR Diff Findings Review` to get findings-first risk analysis.
- Run `Security Hardening Review` for security-sensitive changes.
- Run `API Change Impact Review` when interfaces or behavior contracts change.

3. Release gate
- Run `Release Readiness Check` for branch transition readiness.
- Resolve all blocking findings before release handoff.

4. PR reporting
- Run `Compose AI PR Report` to produce a paste-ready report block.
- Use `.github/copilot/PR_AI_REPORT_TEMPLATE.md` for consistent PR documentation.

5. Exception compliance check
- If any High finding is accepted, run `Verify High Exception Record` against PR text before merge.
- CI guard: `.github/workflows/gate-pr-core.yml` validates record completeness for the governance scope.

## Usage Notes

- Default implementation branch is `develop` unless explicitly release-scoped.
- Public C++ API changes require synchronized Doxygen and docs updates.
- Prefer focused build and test verification before broad test sweeps.

## Suggested PR Checklist

- [ ] Implementation done via roadmap-first or explicit issue scope
- [ ] Focused build and test evidence attached
- [ ] Findings-first review performed and acted upon
- [ ] API/documentation updates completed where needed
- [ ] Release-readiness check performed for release-scoped changes

## Enforcement Entry Point

- Required PR gates are captured in [.github/pull_request_template.md](../pull_request_template.md)
- Severity-based merge decisions are defined in [REVIEW_SEVERITY_POLICY.md](REVIEW_SEVERITY_POLICY.md)
