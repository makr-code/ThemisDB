---
Author: "ThemisDB Team"
Created: "2026-04-17"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "tests/performance/CMakeLists.txt"
  - "tests/performance/phase3/*.cpp"
  - "tests/performance/phase4/*.cpp"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Test Improvements

The concrete coverage gap visible in the current CMake configuration is
registration of nested test sources. Twelve C++ test files are present under
`tests/performance/phase3/` and `phase4/`, but the module's two source globs
match only files directly under `tests/performance/`.

## Candidate work

- Add explicit phase3/phase4 CMake registration or establish and document an
  alternate supported execution mechanism.
- Avoid duplicate executable/test registration for root-level
  `test_performance_*.cpp` files unless both focused and auto-focused suites
  are intentional.
- Tie future test coverage to the corresponding public header/implementation
  contracts, including capability absence and error/fallback behavior.
- Validate the configured target list before using test-source presence as
  evidence of coverage.

No behavior change or test pass is claimed by this planning note. See
[`ROADMAP.md`](ROADMAP.md) for the active registration follow-up.

Exact model backend not verifiable from final document text alone.
