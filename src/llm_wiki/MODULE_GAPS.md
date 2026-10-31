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

# LLM Wiki Module Gaps

## Purpose and evidence

This page tracks material gaps verified against current module source. It does not assert that every item is a release blocker; release decisions belong to the root [`ROADMAP.md`](../../ROADMAP.md) and maintainer sign-off.

## Implementation gaps

| ID | Gap | Current evidence | Required closure evidence |
|---|---|---|---|
| LW-GAP-01 | File ingestion does not parse source content into chunks | `LLMWikiPluginImpl::ingest()` creates generic placeholder text; workspace code copies the source file and writes supplied chunks | Content-based ingest tests covering valid files, filters, missing files, and partial failures |
| LW-GAP-02 | Plugin query is not indexed retrieval and ignores query options | `LLMWikiPluginImpl::query()` returns filtered in-memory chunks; `WikiQueryOptions` is unused | Tests proving top-k, min-score, workspace selection, save-page, and retrieval behavior |
| LW-GAP-03 | Wikipedia dump operation does not process a dump | `ingestWikipediaDump()` ignores both arguments and returns fixed counts | Pipeline integration and edition/feature-gate tests against real fixture data |
| LW-GAP-04 | Edition selection is not runtime licensing | `edition_gate.cpp` maps the compile-time macro to Enterprise or Community; feature checks do not perform independent license validation | Shipped-build and license-manager integration evidence |
| LW-GAP-05 | Process-policy configuration does not implement its full declared control plane | Policy loading validates schema and selected invariants; the YAML's full stage scheduling, ML control, telemetry, and rollout are not executed by plugin request paths | Runtime tests for every supported policy behavior or remove unsupported policy claims |
| LW-GAP-06 | Workspace checksum is not cryptographic | `workspace_state_manager.cpp` computes 64-bit FNV-1a and prefixes the value with `sha256:` | Replace or rename the integrity mechanism and add tampering tests before claiming cryptographic integrity |
| LW-GAP-07 | Workspace components have distinct state representations and synchronization contracts | The orchestrator and `WorkspaceStateManager` have separate data structures; the latter requires caller-side synchronization | Documented format ownership, migration compatibility, and concurrency tests |

## Testing and performance evidence gaps

- Tests are distributed across `tests/llm_wiki/`, `tests/llm/`, and `tests/integration/`. A discovered file is not evidence that the test was configured or passed.
- `bench_llm_wiki_dedicated_gates.cpp` measures local stub structures; `bench_wikipedia_throughput.cpp` generates synthetic article strings. Neither measures production plugin ingestion or query retrieval.
- Representative hardware results, production-path latency/throughput baselines, and resource limits remain to be established.

## Priorities

- [ ] Close LW-GAP-01 through LW-GAP-03 before describing source ingestion, retrieval, or Wikipedia dump ingestion as operational (Target: Q1 2027).
- [ ] Validate runtime licensing, policy behavior, and integrity guarantees before using them as production security controls (Target: Q1 2027).
- [ ] Collect configured test and representative benchmark evidence before readiness sign-off (Target: Q1 2027).

> Exact model backend not verifiable from final document text alone.
