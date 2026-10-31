---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Test Layer — Candidate Work

## Open evidence tasks

- [ ] Map every `config_contract.h` declaration to an enforcing test or identify
  the missing test/code evidence.
- [ ] Specify the supported schema keyword set and add explicit tests for
  ignored or unsupported schema keywords.
- [ ] Add/confirm regression coverage that distinguishes `get()` errors from
  `tryGet()`'s `std::nullopt` behavior for missing and corrupted values.
- [ ] Establish platform-specific watcher behavior from CI execution evidence;
  source presence alone does not demonstrate all backends.
- [ ] Register or explicitly document the status of
  `test_lru_boundary_focused.cpp` in the root CMake test configuration.

## Change rule

Tests should assert behavior visible in source contracts and public interfaces.
Do not create a policy fixture or imply that production configuration is
consumed by this test directory unless a corresponding file and code path are
added.

Root registrations and example commands are in [`README.md`](README.md);
runtime gaps are tracked in
[`../../src/config/MODULE_GAPS.md`](../../src/config/MODULE_GAPS.md).
