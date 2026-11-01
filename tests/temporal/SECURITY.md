---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Security boundary for temporal tests

This directory contains test code; it is not a security control or deployment
configuration surface. Passing temporal tests cannot establish authentication,
authorization, encryption, audit policy, or release security approval.

Review tests that exercise filesystem-backed CDC/cold-store paths for safe
temporary-directory handling and cleanup. Treat test fixtures, logs, and
benchmark outputs as potentially containing record payloads. Do not add
credentials or production data to fixtures.

Temporal API authorization and persistent-data protections belong to the
consuming service and its operations. Follow the project-level
[`SECURITY.md`](../../SECURITY.md) for vulnerability reporting and the module
[`../../src/temporal/SECURITY.md`](../../src/temporal/SECURITY.md) for API data
boundaries.

**TODO:** perform a separate test-harness security review before claiming that
the test suite has passed a security audit. No such review is represented here.
