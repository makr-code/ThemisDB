---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "architecture-governance"
Canonical_References: "tests/exporters/CMakeLists.txt; tests/exporters/test_*.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter tests architecture

Exact model backend not verifiable from final document text alone.

`tests/exporters/` contains focused C++ tests for exporter APIs and behavior;
it does not contain production implementations. `CMakeLists.txt` globs the
top-level `test_*.cpp` files, creates one executable per file named
`module_exporters_<stem>_focused`, and registers a CTest entry named
`<stem>_ExportersFocusedTests` with the `exporters` label. The contract
hardening, phase-23 hardening, and encryption targets also receive Wave B labels.

Test executables include both repository `include/` and `src/` paths and link
`themis_core`, test libraries, spdlog, and Threads. The source tests should be
the evidence for behavior; this folder's docs do not replace the public
contracts in `include/exporters/`.
