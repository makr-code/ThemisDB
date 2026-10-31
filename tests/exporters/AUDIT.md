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
SOT_Domain: "module-behavior"
Canonical_References: "tests/exporters/CMakeLists.txt; tests/exporters/test_*.cpp; include/exporters/*.h"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter tests audit notes

Exact model backend not verifiable from final document text alone.

## Evidence boundary

The test directory contains focused suites for format serialization, filtering,
policy/error handling, encryption, template validation, streaming, incremental
watermarks, joins, and Hub client outcomes. The CMake file registers one
focused-test target per top-level `test_*.cpp` source.

The presence of a test is not evidence that it passed in a particular build.
No exporter test run is asserted by this audit note. For current run results,
record the configure/build/CTest commands and outcomes in the validating change.

## Review checklist

- [ ] Compare changed behavior with the relevant public header contract.
- [ ] Add a focused regression beside the owning exporter test.
- [ ] Build and run the matching CTest entry; record any unavailable dependency
      or environment blocker rather than marking it passed.
- [ ] Review security-related tests as regression evidence, not as a substitute
      for deployment security review.
