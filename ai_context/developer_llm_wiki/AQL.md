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
Canonical_References: "include/aql/llm_aql_handler.h; include/aql/aql_query_validator.h; src/aql/ARCHITECTURE.md; src/aql/API.md; src/aql/AQL_ASSISTANCE_CONTRACT.md; tests/aql/CMakeLists.txt; benchmarks/aql/CMakeLists.txt"
source: "include/aql/llm_aql_handler.h; src/aql/llm_aql_handler.cpp; src/aql/aql_query_validator.cpp; tests/aql/CMakeLists.txt; benchmarks/aql/CMakeLists.txt"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# AQL — Developer LLM Wiki

Curated orientation for coding and review work in the AQL assistance module.
This is secondary context, not a public API contract or release-status source.
Prefer the public headers for signatures, implementation and focused tests for
runtime behavior, and module documentation for evidence and navigation.

## Module shape and boundaries

The `aql` module contains query-authoring helpers and LLM-assisted workflows:
translation, validation, conversation context, few-shot examples, confidence
scoring, syntax highlighting, agent/tool composition, and selected adapters.
Public declarations are under [`include/aql/`](../../include/aql/);
implementation is under [`src/aql/`](../../src/aql/). The core AQL parser and
query execution are owned by `query`, not by this assistance module. A generated
query is not executed or authorized by `LLMAQLHandler`.

Start with the module
[`README.md`](../../src/aql/README.md), implementation
[`ARCHITECTURE.md`](../../src/aql/ARCHITECTURE.md), public header map
[`include/aql/ARCHITECTURE.md`](../../include/aql/ARCHITECTURE.md), and the
[`API.md`](../../src/aql/API.md) integration notes.

## Translation behavior to preserve

- `LLMAQLHandler` sanitizes and bounds prompt inputs, obtains a candidate from
  the configured generation path, validates it, and applies collection access
  and scope checks before returning it.
- Handler construction creates a parser service if one is not injected. An
  explicit `setParserService(nullptr)` disables parser-backed checking and
  selects the weaker string-level `AQLQueryValidator` fallback.
- Translation validation defaults to `WARN_ONLY`. On validation failure, this
  mode logs a warning and can return the candidate; use `REJECT_ON_ERROR` or
  `RETRY_ON_ERROR` when the caller requires rejection/retry behavior.
- Empty natural-language input has no special-case return contract. Empty
  generated AQL fails the internal validator, but the result still depends on
  validation mode.
- Parsing is not execution, semantic correctness against live data, or
  authorization. Apply host policy at every query execution or tool boundary.
- `validateAQLWithParser()` is a private source helper, not a public handler
  method. The former frozen prose and unsupported-provider trigger claims are
  reconciled in [`AQL_ASSISTANCE_CONTRACT.md`](../../src/aql/AQL_ASSISTANCE_CONTRACT.md).

## Tests and benchmarks

Focused AQL tests are discovered by
[`tests/aql/CMakeLists.txt`](../../tests/aql/CMakeLists.txt) from
`test_aql_*.cpp`; registration, optional local implementation sources, and
target names are conditional on the configured build. Configure a test-enabled
build before using CTest. The root
[`CMAKE.md`](../../CMAKE.md) documents the repository's test-build policy.

Benchmarks are registered only when `THEMIS_BUILD_BENCHMARKS` is enabled; see
[`benchmarks/aql/CMakeLists.txt`](../../benchmarks/aql/CMakeLists.txt).
Benchmark source thresholds and case listings are not evidence of a current
run or representative-hardware performance.

The generated [AQL Doxygen snapshot](../../src/aql/DOXYGEN.md) is derived
reference material. Use it only with its generation date and current headers;
it does not replace source contracts.

## Source priority and freshness

For behavior, prefer the implementation and focused test for the exact path,
then public header contracts and the module API/architecture documents, then
this summary. Root governance overrides module synthesis. The inventory timestamp and hash in `WIKI_STATUS.json` describe the last full
sync and can predate manual curated-page changes; check freshness before relying
on the inventory as current.
