---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `tests/query/CMakeLists.txt`, `tests/CMakeLists.txt`, `include/query/`, `src/query/`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

# Query test architecture

The query test directory contains C++ tests for behavior implemented in
`src/query/` and exposed through `include/query/`. Tests are evidence about the
specific paths they exercise; they do not themselves implement query behavior
or establish a security boundary.

`tests/query/CMakeLists.txt` defines module-focused targets, with source-file
existence checks for conditional targets. Other query-related tests can be
registered by the broader `tests/CMakeLists.txt`. Therefore, source-file
presence, CMake registration, and a passing test result are separate facts.

When changing query behavior, follow the relevant API declaration and
implementation, then identify the focused test and its actual CMake/CTest
registration. The module-level contract and pipeline are documented in
[`../../src/query/ARCHITECTURE.md`](../../src/query/ARCHITECTURE.md); current
module status is in [`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md).
