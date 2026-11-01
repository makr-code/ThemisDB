---
Author: "ThemisDB Team"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/prompt_engineering/README.md; src/prompt_engineering/README.md; src/prompt_engineering/ARCHITECTURE.md; src/prompt_engineering/API.md; src/prompt_engineering/IMPLEMENTATION.md; src/prompt_engineering/MODULE_GUIDE.md; tests/prompt_engineering/CMakeLists.txt; benchmarks/prompt_engineering/README.md"
---

> Exact model backend not verifiable from final document text alone.

# Prompt Engineering — Developer LLM Wiki

Curated orientation for coding and review work in the prompt-engineering module.
This is secondary context, not a public API contract or release-status source.
Prefer public headers for exact declarations, implementation and focused tests
for runtime behavior, and module documentation for integration and test guidance.

## Module shape and boundaries

The module provides C++ APIs for prompt templates and rendering, version
history, evaluation and optimization, feedback, metrics, safety heuristics,
and rewrite rules. Public interfaces are declared under
[`include/prompt_engineering/`](../../include/prompt_engineering/); implementation
is under [`src/prompt_engineering/`](../../src/prompt_engineering/). Production
sources are part of `themis_core`. The module does not own model serving,
inference transport, or the host's authorization and output policies.

Start with the module
[`README.md`](../../src/prompt_engineering/README.md),
[`ARCHITECTURE.md`](../../src/prompt_engineering/ARCHITECTURE.md), and
[`API.md`](../../src/prompt_engineering/API.md). Use the
[public-header inventory](../../include/prompt_engineering/README.md) to find
interfaces and then consult the corresponding header for its exact contract.

## Runtime paths to preserve

- `PromptEngineeringIntegration::beforeExecution()` prepares an
  `ExecutionContext`; the host sends its enhanced prompt through its own LLM
  adapter and supplies the result to `afterExecution()`. The façade does not
  make that model call or enforce that the host invokes both hooks.
- `PromptManager` validates, stores, reads, and renders templates. Context
  injection is literal placeholder substitution: it does not escape inserted
  values, authorize them, recursively expand them, or report missing keys.
  Persistence failures can be logged while an in-memory operation still
  reports success; verify durability separately when it is a requirement.
- `PromptEvaluator` uses lexical scoring by default. Semantic scoring requires
  a caller-supplied embedding provider with the expected output contract.
- `PromptInjectionDetector` is heuristic detection, not a complete security
  boundary or an authorization decision.
- `RewriteEngine` is a separate implemented API. The integration hooks do not
  invoke it automatically; check the actual caller before assuming a request
  has been rewritten.

Read [`IMPLEMENTATION.md`](../../src/prompt_engineering/IMPLEMENTATION.md) and
[`TRUST.md`](../../src/prompt_engineering/TRUST.md) before changing persistence,
rewrite, or prompt-data handling. Do not infer a module-wide thread-safety
guarantee; follow the synchronization and lifetime contract of each interface.

## Tests and benchmarks

[`tests/prompt_engineering/CMakeLists.txt`](../../tests/prompt_engineering/CMakeLists.txt)
discovers `test_*.cpp` files, creates one focused target per file, and registers
them with the `prompt_engineering` CTest label. Configure with tests enabled,
then use the commands in [`MODULE_GUIDE.md`](../../src/prompt_engineering/MODULE_GUIDE.md)
to build a focused target or run the label. Registration is not evidence that
the tests have passed in the current build.

Benchmark targets are enabled through the repository benchmark build when
`THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark is available. The module
[benchmark guide](../../benchmarks/prompt_engineering/README.md) lists its
targets and limitations. In particular, `bench_rewrite_engine.cpp` uses
synthetic helpers and does not measure the production `RewriteEngine` or
`RewriteRuleLoader`; source thresholds are not measured release results.

The generated [Doxygen inventory](../../src/prompt_engineering/DOXYGEN.md) and
[module-gap report](../../src/prompt_engineering/MODULE_GAPS.md) are derived
snapshots. Check their generation evidence and scope before treating their
inventories or findings as current.

## Source priority and freshness

For behavior, prefer the implementation and focused test for the exact path,
then public header contracts and module API/architecture documents, then this
summary. Root governance overrides module synthesis. The generated developer
wiki inventory and `WIKI_STATUS.json` describe the last completed sync, not a
continuous source scan.
