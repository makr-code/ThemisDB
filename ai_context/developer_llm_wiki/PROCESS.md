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
Canonical_References: "include/process/*.h; src/process/README.md; src/process/ARCHITECTURE.md; src/process/API.md; src/process/IMPLEMENTATION.md; src/process/PERFORMANCE_EXPECTATIONS.md; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; benchmarks/compare_expectations.py"
source: "include/process/; src/process/README.md; src/process/ARCHITECTURE.md; tests/process/CMakeLists.txt; benchmarks/process/CMakeLists.txt; benchmarks/compare_expectations.py"
source_class: "secondary"
source_weight: 60
---

> Exact model backend not verifiable from final document text alone.

# Process — Developer LLM Wiki

This is a secondary orientation for coding and review work. Public headers and
implementation establish behavior; module docs and focused tests provide
navigation and evidence. Root governance takes precedence over this summary.

## Module shape

The Process module manages process-model records and format handlers, process
graph/context retrieval, process-object linking, analytics helpers, and
integration surfaces for external orchestration profiles. The module does not
provide a general workflow execution service. Start with the [module README](../../src/process/README.md),
[architecture](../../src/process/ARCHITECTURE.md), [API guide](../../src/process/API.md),
and [implementation notes](../../src/process/IMPLEMENTATION.md).

## Runtime contracts and limits

- `ProcessModelManager` owns process-model lifecycle and exposes optional,
  caller-wired embedding and vector-index integrations. An available vector in a
  descriptor can be passed through; its presence does not mean the module
  generates an embedding.
- `ProcessGraphRag` assembles context from process instances, active nodes,
  model structure, attachments, compliance information, and similar cases.
  Returned context depends on the available stored data and configured
  integrations.
- `ProcessLightRetriever` supports LOCAL, GLOBAL, and keyword-routed AUTO.
  GLOBAL can fall back to LOCAL if community reports are unavailable. The
  header-declared resource budgets are not enforced by the current `retrieve()`
  path.
- `ProcessAgenticRag` adapts process graph retrieval to generic iterative RAG.
  Inspect its result and iteration history; adapter output does not establish
  answer correctness.
- `ProcessOrchestrationRegistry` validates profile ID/domain and the
  `internal_rag_stage` invariant. Profile storage and the active-domain pointer
  are separate writes, so activation and rollback are not transactional.
  Prediction scores are deterministic heuristics, not calibrated model output.
- Audit, telemetry, and federation interfaces must be checked against their
  implementation before relying on persistence, transport, cryptographic
  integrity, quorum, or production-readiness claims.

## Tests and benchmarks

[`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt) discovers
`test_*.cpp`. Standard focused targets are named
`module_process_<stem>_focused`, with CTest names
`<stem>_ProcessFocusedTests`; `test_process_*.cpp` files also participate in
the guarded autofocused registration. Registration is not evidence that a
target built or passed.

[`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt)
registers 12 executable targets containing 88 source-level Google Benchmark
macro declarations. These counts do not define performance gates. The
[expectation comparator](../../benchmarks/compare_expectations.py) reads rows
from a `## Specific Expectations` section; the Process performance page has no
such section, so the current comparator discovers zero Process expectation
targets.

`bench_process_dedicated_gates.cpp` comments list four numeric PM-BM targets.
They are not wired into the comparator or asserted by the benchmark code; the
benchmarks use in-process stub manager and signal-bus types, not OS process
operations. The tracked
[mining JSON](../../artifacts/nightly/bench_process_mining.json) is a
2026-04-15 sample with 22 results, not a current baseline or threshold pass.
The separate release coverage audit reports 14/14 Process source-to-CMake-to-
CTest mappings. Its [2026-10-30 run](https://github.com/makr-code/ThemisDB/actions/runs/41001268780)
passed overall; its latest visible [2026-11-01 run](https://github.com/makr-code/ThemisDB/actions/runs/41271872461)
also reports Process 14/14 but fails overall on an unregistered Training source.
These are coverage outcomes, not performance runs. The latest visible
[weekly performance run](https://github.com/makr-code/ThemisDB/actions/runs/40257720669)
failed at CMake configure before building or executing benchmarks. No current
Process performance pass or reproducible baseline is established.

For focused commands and the complete registration file list, use the
[benchmark catalog](../../benchmarks/process/README.md).

## Review path

1. Follow the [API guide](../../src/process/API.md) to the relevant public
   declaration and matching implementation.
2. Select the nearest focused test from
   [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt), then
   verify actual build and CTest output.
3. Check operational, security, and performance caveats in the module docs;
   never infer guarantees from a declaration, CMake registration, or historical
   report alone.

The [module roadmap](../../src/process/ROADMAP.md) is a planning document, not
release approval. If this page conflicts with current headers, implementation,
or canonical module documentation, correct this summary from those sources.
