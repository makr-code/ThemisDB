---
Author: "ThemisDB Team"
Created: "2026-08-05"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Tests for `prompt_engineering`

The module test CMake file discovers every `test_*.cpp` in this directory and registers a focused unit-test target with the `prompt_engineering` CTest label. The current source files are:

- `test_prompt_engineering_focused.cpp` — manager, version-control, feedback, optimizer, evaluator, and metrics smoke checks
- `test_prompt_engineering_adversarial_focused.cpp` — adversarial prompt/detector cases
- `test_prompt_engineering_validation_hardening.cpp` — template validation edge cases
- `test_prompt_engineering_highcardinality_stress.cpp` — high-cardinality stress scenarios
- `test_llm_reflection_adapter_focused.cpp` — reflection adapter checks
- `test_rewrite_engine_focused.cpp` — three interface/compile-contract checks; it does not exercise rewrite execution
- `test_structured_output_focused.cpp` — structured-output checks

The PE-FT identifiers in older roadmap and issue-closure snapshots are not GoogleTest names for the current focused test source; do not use those snapshots as current test counts or pass results.

## Build and test

From the repository root, configure the existing community preset with tests enabled:

```bash
cmake --preset community-release-allow-missing-rocksdb \
  -DTHEMIS_BUILD_TESTS=ON
```

Build the manager/version/evaluator focused target:

```bash
cmake --build build-community-release-allow-missing-rocksdb \
  --target module_prompt_engineering_test_prompt_engineering_focused_focused
```

Run all registered prompt-engineering tests:

```bash
ctest --preset community-release-allow-missing-rocksdb \
  -L prompt_engineering --output-on-failure
```

The CMake registration is in [`CMakeLists.txt`](./CMakeLists.txt). The label selects all module test targets, not only the target shown above. A test result must be recorded from an actual run; this document does not assert that the current suite passed.
