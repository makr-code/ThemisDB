# Security — ai_working Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

Report vulnerabilities via the project-level SECURITY.md.

## Security Scope

The `ai_working` module contains no runtime C++ code. Security considerations apply exclusively to the content of committed planning and working artifacts.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| Secrets accidentally committed in working artifacts | Repository-wide secret-scanning tooling exists; the ai_working maintenance workflow does not itself scan these files |
| Sensitive material retained in obsolete artifacts | Review archive candidates and destination content; archival moves files and does not remove sensitive content |
| Unauthorized modification of governance docs | Use repository branch protection and PR review; verify the applicable checks on the target branch |
| Sensitive planning information exposed in public artifacts | Review public-facing artifacts before merge; confirm applicable review controls on the target branch |

## Implemented Security Controls

- Repository tooling includes Gitleaks configuration and a secret-scanning script (`scripts/secret_scan.py`); the exact CI coverage for these paths is not established by `.github/workflows/maintenance-ai-working.yml`.
- The module helper can move non-protected Markdown files to `docs/ARCHIVED/ai-working-history/`; it does not redact or classify secrets.
- The maintenance workflow runs a required-document check, but that check does not scan secrets or establish PR-review coverage.

## Security Follow-ups

- Run the repository's configured secret-scanning check on changed working artifacts and verify its result; do not infer coverage from the maintenance workflow.
- Review for secrets before and after archive moves; archived material remains in the repository.
- Continue enforcing branch protection and PR review for the target branch.
