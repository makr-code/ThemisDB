# ThemisDB Review Severity Policy

This policy defines merge decisions for findings-first reviews.

## Severity Definitions

- Critical: probable crash, data loss/corruption, privilege bypass, severe security break.
- High: user-visible functional break, incorrect output, major reliability regression.
- Medium: edge-case correctness risk, flaky behavior, missing validation likely to regress.
- Low: maintainability concern with plausible near-term operational impact.

## Merge Rules

1. Critical findings
- Must be fixed before merge.
- No waiver allowed for release branches.

2. High findings
- Must be fixed before merge.
- Exception only with maintainer approval and explicit mitigation plan.

3. Medium findings
- Should be fixed before merge when low-cost.
- May be deferred only with owner, target milestone, and tracking issue.

4. Low findings
- May be deferred with documented rationale.

## Exception Process (High Only)

- Required approver: maintainer.
- Required documentation in PR:
  - finding reference
  - reason for exception
  - mitigation in current release
  - removal/fix milestone
  - tracking issue

## Required PR Evidence

- Findings summary by severity.
- Disposition for each finding: resolved, accepted exception, deferred.
- Validation evidence for fixes or mitigations.

## Release Branch Rule

For release-scoped changes, open Critical or High findings block release readiness and merge.

## Ownership

- Author: prepares findings disposition and evidence.
- Reviewer: verifies severity and evidence.
- Maintainer: final decision and exception approval.
