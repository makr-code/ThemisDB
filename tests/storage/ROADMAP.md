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
  - "../../src/storage/ROADMAP.md"
  - "../../ROADMAP.md"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage test roadmap

The test directory has no independent delivery plan. Storage implementation
priorities are tracked in [`../../src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md);
repository-wide maturity status is tracked in [`../../ROADMAP.md`](../../ROADMAP.md).

For test changes:

- add focused `test_*.cpp` sources and verify their target in
  [`CMakeLists.txt`](CMakeLists.txt);
- link each test to a specific implementation behavior or contract;
- record executed results separately from source registration.

TODO: add issue-linked test milestones here only if the canonical storage
roadmap assigns them and defines acceptance evidence.
