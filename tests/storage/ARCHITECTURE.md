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
  - "../../include/storage/"
  - "../../src/storage/"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage test architecture

`tests/storage/` is a flat collection of test translation units, not a runtime
module or public API boundary. `CMakeLists.txt` discovers `test_*.cpp`, builds
focused targets linked to `themis_core`, test libraries, spdlog, and threads,
and registers unit-tier CTest entries with a 120-second timeout.

Some tests add explicit storage implementation sources or labels. Review the
CMake file for exact per-target behavior; this page does not claim that every
test exercises production dependencies or that any test has passed.

The corresponding component declarations and implementations live in
[`../../include/storage/`](../../include/storage/) and
[`../../src/storage/`](../../src/storage/).
