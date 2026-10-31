---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Training Module Developer Guide

## Start here

The module is split between public contracts in
[`include/training/`](../../include/training/) and implementations in this
directory. Start with [`ARCHITECTURE.md`](./ARCHITECTURE.md) for boundaries,
[`API.md`](./API.md) for the public entry-point index, and
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) before changing behavior. Treat
header Doxygen and `include/training/TRAINING_CONTRACT.md` as the detailed
contract.

## Build and focused tests

Configure the repository using the supported CMake preset for the local
environment, then build the focused target for the changed component. The
training targets are registered by
[`tests/training/CMakeLists.txt`](../../tests/training/CMakeLists.txt) with the
`training` label and a 120-second per-test timeout.

From a configured build directory, for example:

```sh
cmake --build <build-dir> --target module_training_test_training_pipeline_e2e_focused
ctest --test-dir <build-dir> -L training --output-on-failure
```

Use the narrowest target/test first. The label command runs the registered
training tests in that build tree, not necessarily every training-related test
registered elsewhere in the repository. For code changes, also run the
repository's required validation for the affected dependency or public
contract.

## Change checklist

1. Identify whether the change belongs in a public header or internal source.
2. Preserve explicit error results, checkpoint integrity and resume behavior,
   callback sanitization, and documented ownership/thread-safety contracts.
3. Add or update a focused test under `tests/training/`; include edge cases
   such as empty input, invalid configuration, absent external services, and
   interrupted checkpoint recovery where relevant.
4. Update this module's affected developer docs and API comments in the same
   change. Do not report performance gains without benchmark evidence.
5. Review the current roadmap and future work before changing acceptance or
   readiness claims.

## Known implementation caveats

- Selection's configured embedding model name is not currently used by its
  hash-derived clustering implementation; see [`EMBEDDING.md`](./EMBEDDING.md).
- The optional OCR path currently emits a placeholder sample rather than
  transcribed image text; see [`MODELING.md`](./MODELING.md).
- A null query engine selects documented offline/test behavior in some
  components. Such results are not a production database integration test.

## Related module material

- [`README.md`](./README.md) — module overview.
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — boundaries and flows.
- [`ROADMAP.md`](./ROADMAP.md) — current status and acceptance work.
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — planned changes.
- [`SECURITY.md`](./SECURITY.md) and [`TRUST.md`](./TRUST.md) — controls and
  trust boundaries.
- [`RUNBOOK.md`](./RUNBOOK.md) and [`OPERATIONS.md`](./OPERATIONS.md) —
  diagnosis and operating checks.
