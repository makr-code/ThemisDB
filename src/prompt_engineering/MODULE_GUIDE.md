---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering Module Guide

## Audience and scope

Use this guide to navigate and validate the prompt-engineering subsystem. It covers module-owned prompt templates, version history, evaluation, feedback, metrics, injection heuristics, and rewrite rules. LLM transport and model lifecycle are owned by their integration modules.

## Source map

- Public contracts: [`include/prompt_engineering/`](../../include/prompt_engineering/)
- Implementations: this directory (`src/prompt_engineering/`)
- Focused unit tests: [`tests/prompt_engineering/`](../../tests/prompt_engineering/)
- Performance benchmark: [`benchmarks/prompt_engineering/bench_prompt_engineering.cpp`](../../benchmarks/prompt_engineering/bench_prompt_engineering.cpp)
- Architecture and module status: [`ARCHITECTURE.md`](./ARCHITECTURE.md), [`ROADMAP.md`](./ROADMAP.md)
- Engineering contracts: [`IMPLEMENTATION.md`](./IMPLEMENTATION.md), [`API.md`](./API.md), [`TRUST.md`](./TRUST.md)

Start with [`API.md`](./API.md) to choose an interface, then read the corresponding public header and implementation before changing behavior. Treat `ROADMAP.md` as delivery/status context, not as a substitute for current code.

## Local test workflow

From the repository root, configure the existing community release preset with tests enabled:

```bash
cmake --preset community-release-allow-missing-rocksdb \
  -DTHEMIS_BUILD_TESTS=ON
```

Build the focused manager/version/evaluator test:

```bash
cmake --build build-community-release-allow-missing-rocksdb \
  --target module_prompt_engineering_test_prompt_engineering_focused_focused
```

Run the prompt-engineering labeled module tests:

```bash
ctest --preset community-release-allow-missing-rocksdb \
  -L prompt_engineering --output-on-failure
```

The label includes all tests registered by [`tests/prompt_engineering/CMakeLists.txt`](../../tests/prompt_engineering/CMakeLists.txt), not only the focused manager test. Test executables link against `themis_core` and `themis_llm`; a configure or build failure should be triaged as a repository dependency/build issue before attributing it to a prompt-engineering assertion.

## Change checklist

1. Preserve the distinction between manager struct validation and JSON validator injection heuristics.
2. Add regression coverage for empty/missing state, malformed input, and error/blocked outcomes.
3. For rewrite-rule changes, cover phase ordering, priority ties, loader rejection, trace limits, and max-step termination as applicable.
4. For evaluator or optimizer changes, use fixed cases and record configuration/provider details.
5. Run the focused tests, then the relevant benchmark cases; compare against the release baseline described in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
6. Update the relevant module docs in the same change; do not claim a production-safe default unless source and deployment configuration establish it.

## Known boundaries

- Test success does not establish that the evaluation fixtures represent production prompts.
- The default evaluator is lexical unless a provider is attached.
- Prompt substitution and injection detection do not replace downstream authorization or output validation.
- Operational symptoms and recovery paths are in [`RUNBOOK.md`](./RUNBOOK.md) and [`OPERATIONS.md`](./OPERATIONS.md).

Exact model backend not verifiable from final document text alone.
