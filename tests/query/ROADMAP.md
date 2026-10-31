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
Canonical references: `tests/query/CMakeLists.txt`, `tests/CMakeLists.txt`, `src/query/ROADMAP.md`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

# Query test roadmap

## Current inventory

There are 61 C++ test source files in `tests/query/`. The directory CMake file
conditionally registers module-focused targets when their source files exist;
the root test CMake configuration also contains query-related targets. Neither
source-file count nor CMake registration is evidence that tests passed.

## Maintenance priorities

- Keep this inventory and the test-target documentation aligned with the
  current sources and CMake registration.
- For query behavior changes, add or update focused regressions and confirm
  the target is registered in the active build.
- Record build and test-run evidence separately from source/registration
  inventory.
- Use [`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md) for feature
  delivery and implementation status; this file does not create a separate
  query implementation roadmap.

## Open verification

This documentation pass inspected CMake registration but did not configure a
build or run the query test targets. Test execution status is therefore not
asserted here.
