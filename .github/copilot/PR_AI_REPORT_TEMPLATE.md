# PR AI Report Template

Use this block in PR descriptions for AI-assisted changes.

## AI Findings Summary

- Critical:
- High:
- Medium:
- Low:
- Decision notes (resolved/accepted/deferred):

## Changes Implemented

- Scope:
- Key files:
- Behavior/API changes:

## Validation Evidence

- Build commands and outcomes:
- Focused tests and outcomes:
- Additional checks:

## Residual Risks

- Risk 1:
- Risk 2:

## Follow-up Actions

- Action 1:
- Action 2:

## Release Readiness (if applicable)

- Target transition (example: develop -> community):
- Verdict (Ready / Conditionally Ready / Not Ready):
- Blocking items:

## Example (High Exception Accepted)

- Finding reference: High-2 (API timeout fallback path)
- Maintainer approver: @maintainer-handle (PR comment link)
- Mitigation in current release: request timeout capped to 2s and fallback path rate-limited
- Target fix milestone: 1.10.0
- Tracking issue: #6123
- Validation evidence: `ctest --preset windows-release -R "ApiTimeout|GrpcApiServerTests" -j 1` passed
- Severity policy reference: `.github/copilot/REVIEW_SEVERITY_POLICY.md`
