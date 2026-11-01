---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM-L-Wiki architecture

## Purpose and evidence boundary

This document describes the module path `src/llm_l_wiki/`, not the separately
implemented `src/llm_wiki/` module or the `themisdb_llm_wiki` plugin. It is for
developers assessing whether this path can be built, integrated, or enabled.
Statements below are limited to the checked-out source inventory, build
references, and the repository's test-ownership record.

## Components and ownership

| Area | Verified state in this checkout |
|---|---|
| `src/llm_l_wiki/FEATURES.md` | Module documentation |
| `src/llm_l_wiki/wikipedia/wiki_workspace_orchestrator.cpp` | Empty translation unit (zero bytes) |
| `include/llm_l_wiki/` | No module public header found |
| Build registration | No CMake target/reference to `src/llm_l_wiki` found |
| Direct tests and benchmarks | None listed for this module in `tests/TEST_DENSITY_MATRIX.md` |

No runtime component, API boundary, persistence boundary, or owning service can
be established from these artifacts. In particular, a filename containing
“orchestrator” does not establish that orchestration exists.

## Runtime flow and failure domains

There is no source-backed request or data flow for this module. The only
verified flow is the documentation/evidence flow: module inventory identifies
the path, while build registration and direct tests would be needed before
runtime behavior could be asserted.

Current practical failure domains are therefore integration and documentation
errors: a consumer may mistake this path for `llm_wiki`, or a release record may
claim a feature for which no target or tests exist. Do not route production
traffic to `llm_l_wiki` or treat adjacent-module behavior as a fallback.

## Constraints and non-goals

- Do not assign APIs, storage formats, model providers, embeddings, or
  guardrails to this module without source and tests.
- Keep `src/llm_wiki/` and `plugins/themisdb_llm_wiki/` as separate ownership
  boundaries; their contracts are not inherited here.
- Before introducing runtime behavior, establish ownership, build wiring,
  failure semantics, and direct test coverage in the same implementation work.

## References

- [Implementation inventory](IMPLEMENTATION.md)
- [API status](API.md)
- [Repository test-density evidence](../../tests/TEST_DENSITY_MATRIX.md)
- [Separate `llm_wiki` target](../llm_wiki/CMakeLists.txt)

> AI-assisted documentation. Exact model backend not verifiable from final document text alone.
