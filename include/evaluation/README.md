# Evaluation include directory

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

This directory currently contains only `evaluation.h`. That file is a placeholder
umbrella header; it does not include or declare the evaluation component APIs.
Do not use it as evidence that the metric, planner, or lifecycle interfaces are
available through this header.

## Contract headers and build ownership

The component declarations are in `src/evaluation/include/`. The module CMake
file installs those headers under `include/evaluation` as part of the
`themis_evaluation` target when that target is included in the configured build.
That target compiles the benchmark matrix, hardware profile, retrieval metrics,
ablation, and artifact lifecycle sources. Query planning and approximation rules
are separate conditional targets; header presence alone does not mean that their
implementations are linked.

The comment in `evaluation.h` mentions `THEMIS_BUILD_EVALUATION=ON`, but
`src/evaluation/CMakeLists.txt` does not define that option. It declares
`themis_evaluation` and the conditional `epic2_*` targets described above.

For the actual contracts, source-to-target mapping, and focused tests, see
[`../src/evaluation/API.md`](../../src/evaluation/API.md),
[`../src/evaluation/IMPLEMENTATION.md`](../../src/evaluation/IMPLEMENTATION.md),
and [`../tests/evaluation/README.md`](../../tests/evaluation/README.md).

There is no verified standalone usage snippet for this umbrella header because
it currently contributes no component declarations. Check the target and
installed include layout in `src/evaluation/CMakeLists.txt` before integrating a
component API.
