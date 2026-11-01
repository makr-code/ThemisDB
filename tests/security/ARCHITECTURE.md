---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - tests/security/CMakeLists.txt
  - tests/CMakeLists.txt
  - tests/security/README.md
  - tests/security/test_security_evidence_collector.cpp
  - tests/security/test_security_phase2_crypto_hardening_focused.cpp
  - tests/security/test_security_phase3_policy_hardening_focused.cpp
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security test-suite structure

This directory is test code, not a runtime security component. It has 88
top-level `test_*.cpp` files. Separately, three nested attack-vector sources
live under `attack-vectors/{authentication,crypto,injection}/`, and one
additional source lives under `ai_safety/`.

[`tests/security/CMakeLists.txt`](CMakeLists.txt) uses nonrecursive globs for
top-level `test_*.cpp` and `test_security_*.cpp` files. The first glob creates
focused targets; the second considers auto-focused targets and checks for an
existing target before adding one. These globs do not include the nested
attack-vector or `ai_safety` sources. `tests/CMakeLists.txt` discovers this
module CMake file and also has explicit focused registrations for selected
security tests. See [`README.md`](README.md) for the detailed source-count and
build notes.

The test directory does not own production interfaces, application
configuration, or an end-to-end security pipeline. Interface contracts belong
to `include/security/`; implementation ownership and readiness are described
in [`src/security/ARCHITECTURE.md`](../../src/security/ARCHITECTURE.md) and the
module roadmap.
