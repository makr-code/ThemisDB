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
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "ROADMAP.md; .gitmodules; include/ethics_ai/ethics_ai_plugin_interface.h; include/ethics_ai/ethics_selection_router.h; src/ethics_ai/ethics_ai_plugin.cpp; src/ethics_ai/discourse_engine.cpp; src/ethics_ai/discourse_orchestrator.cpp; src/ethics_ai/meta_verdict_builder.cpp; src/ethics_ai/CMakeLists.txt; tests/ethics_ai/CMakeLists.txt; benchmarks/CMakeLists.txt; benchmarks/ethics_ai/CMakeLists.txt"
source: "include/ethics_ai/ethics_ai_plugin_interface.h; include/ethics_ai/ethics_selection_router.h; src/ethics_ai/ethics_ai_plugin.cpp; src/ethics_ai/discourse_engine.cpp; src/ethics_ai/discourse_orchestrator.cpp; src/ethics_ai/meta_verdict_builder.cpp; src/ethics_ai/CMakeLists.txt; tests/ethics_ai/CMakeLists.txt; benchmarks/CMakeLists.txt; benchmarks/ethics_ai/CMakeLists.txt"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Ethics AI — Developer LLM Wiki

Curated orientation for coding and review work. This page is secondary context,
not an API contract, release-status source, or legal compliance assessment.
Root governance and current source/header/test evidence take precedence.

## Authority and source boundary

Root [`ROADMAP.md`](../../ROADMAP.md) designates the external
`makr-code/themisdb_ethic_ai` repository at
`plugins/themisdb_ethic_ai/` as the canonical plugin implementation. The
in-tree [`src/ethics_ai/`](../../src/ethics_ai/) and
[`include/ethics_ai/`](../../include/ethics_ai/) files are reference and
compatibility surfaces; their behavior does not establish canonical external
plugin behavior.

The `.gitmodules` entry pins that submodule to
`ce401ad9d604012a2c02655e79f5c17f57a9f82d` on `develop`, but it is not
initialized in this checkout. GitHub MCP reads for the repository returned
404, so the pinned external implementation and its tests could not be
inspected. Resolve the access/source gap before making claims or implementing
production LDM wiring against that canonical plugin.

## In-tree runtime and API boundaries

- The plugin interface is declared in
  [`ethics_ai_plugin_interface.h`](../../include/ethics_ai/ethics_ai_plugin_interface.h)
  under `themis::plugins::ethics`.
- In the in-tree plugin, `IEthicsAIPlugin::makeDecision()` delegates to
  `EthicalDiscourseEngine::makeDecision()`; it does not call
  `EthicsSelectionRouter`, `DiscourseOrchestrator`, `MetaVerdictBuilder`, or
  `MirrorSchoolHandler`.
- The router exposes `SELECTION_ONLY`, `LAYERED_FULL`, and `LAYERED_FAST`
  planning surfaces. The helper implementations are not proof that the plugin
  decision path executes an LDM pipeline.
- Without an injected callback, the in-tree orchestrator returns deterministic
  synthetic school output based on the school ID and ignores the dilemma text.
  `runEbene2()` builds cluster-majority summaries and tension-axis records,
  rather than the specified multi-round inter-cluster dialogue.
- `MetaVerdictBuilder` assembles from caller-provided results and legal
  grounding; it does not retrieve legal material and currently does not use
  the supplied cluster-position input.
- In the in-tree implementation, `initialize()` attempts profile loading
  before setting the initialized flag; that load returns an error which is
  ignored. After successful initialization, callers must explicitly invoke
  `loadPhilosophyProfiles(path)` and handle its result.

These are observations about the checked-in parent-repository sources only.
They are not recommendations to enable deterministic fallback inference in
production or evidence of behavior in the unavailable canonical plugin.

## Tests and benchmarks

Focused in-tree tests are globbed from `test_*.cpp` by
[`tests/ethics_ai/CMakeLists.txt`](../../tests/ethics_ai/CMakeLists.txt) and
added through the root test-module discovery loop. Registration depends on the
test-enabled build and required test libraries. Target names are
`module_ethics_ai_<source-stem>_focused`; CTest names are
`<source-stem>_EthicsAiFocusedTests`. These tests cover the in-tree reference
path and do not establish canonical external plugin coverage.

The LDM benchmark target is conditionally added by
[`benchmarks/ethics_ai/CMakeLists.txt`](../../benchmarks/ethics_ai/CMakeLists.txt)
when the root benchmark build is enabled and its benchmark/core targets exist.
Its fixtures use a mock registry and deterministic inference, not a live LLM or
legal database. The plugin benchmark is sourced from
[`bench_ethics_ai_plugin.cpp`](../../benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp);
its root CMake target guard must match that nested path. A source-declared
threshold or registered benchmark is not a measured result or a release gate
without a recorded run and threshold-enforcing job.

## Source priority

For code changes, inspect the canonical external plugin if available, then the
public headers, implementation, focused tests, and CMake registration for the
exact path being changed. For current parent-repository status, use
[`src/ethics_ai/ROADMAP.md`](../../src/ethics_ai/ROADMAP.md) together with the
root roadmap; this wiki page must not redefine project maturity or release
readiness.
