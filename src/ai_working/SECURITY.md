# Security — ai_working Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via the project-level SECURITY.md.

## Security Scope

The `ai_working` module contains no runtime C++ code. Security considerations apply exclusively to the content of committed planning and working artifacts.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| Secrets accidentally committed in working artifacts | secret scanning applied to all committed files |
| Stale artifacts referencing sensitive deployment details | archival process removes superseded docs |
| Unauthorized modification of governance docs | standard branch protection and PR review gates |
| Sensitive planning information exposed in public artifacts | maintainer review required before merge |

## Implemented Security Controls

- Secret scanning (`gitleaks`) is applied to all committed artifacts in `ai_working/` and `src/ai_working/`.
- Stale wave-specific artifacts containing deployment or configuration details are archived and reviewed before archival.
- All commits to the module pass through standard PR review and CI gate checks.

## Security Follow-ups

- Verify that no working artifacts contain API keys, tokens, or deployment credentials before each merge.
- Continue enforcing branch protection on the `develop` branch.
- Ensure archival automation (planned Q4 2026) applies the same secret scan before archiving.
