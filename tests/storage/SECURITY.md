---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "CMakeLists.txt"
  - "../../src/storage/SECURITY.md"
  - "../../SECURITY.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Security scope of storage tests

This directory contains tests for storage behavior; it is not itself a security
boundary or deployment policy. Test presence does not certify the security of
production configuration, stored data, backups, logs, or external providers.

Security-sensitive changes should add tests for the changed input, failure,
and configuration paths and should be reviewed against the implementation and
the module's [`../../src/storage/SECURITY.md`](../../src/storage/SECURITY.md).
Report vulnerabilities through the repository's
[`../../SECURITY.md`](../../SECURITY.md) process.

TODO: link explicit security-test names only after verifying the current test
source and registration in `CMakeLists.txt`.
