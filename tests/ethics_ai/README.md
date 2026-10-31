---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# tests/ethics_ai

Focused test sources for `src/ethics_ai`. When `THEMIS_BUILD_TESTS=ON` and GTest is available, root `tests/CMakeLists.txt` auto-discovers module directories containing `CMakeLists.txt` and adds `tests/ethics_ai/`. Its module CMake file globs `test_*.cpp`, creates `module_ethics_ai_<source-stem>_focused` executables, and registers CTest names `<source-stem>_EthicsAiFocusedTests` through `themis_register_module_focused_test()` under the `ethics_ai` and `release_critical` labels.

These are in-tree reference/compatibility-path tests; they are not evidence that the external canonical plugin identified by root `ROADMAP.md` is covered.

The LDM contract tests exercise deterministic/mock inputs, including the default stub inference path; they are not real-LLM or end-to-end plugin-integration tests. Test names and assertions are in the individual `test_*.cpp` files.

For example, `test_ethics_ai_ldm_contract_focused.cpp` produces target `module_ethics_ai_test_ethics_ai_ldm_contract_focused_focused` and CTest name `test_ethics_ai_ldm_contract_focused_EthicsAiFocusedTests`. Each focused test compiles a selected source list directly, rather than depending on the full shared plugin target. The glob is evaluated at configure time; targets also depend on the required test libraries and `themis_core`.
