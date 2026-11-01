---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: architecture-governance
Canonical_Sources: include/process/*.h; src/process/ARCHITECTURE.md; src/process/API.md; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Public-Header Architecture

**AI provenance:** Exact model backend not verifiable from final document text alone.

`include/process/` exposes declarations for process-model management, format handling, linking, retrieval, analytics, diagnostics, and integration surfaces. The header set changes over time; do not treat lists or counts in older snapshots as exhaustive. For the current declarations, inspect this directory and the linked module [API guide](../../src/process/API.md).

## Contract boundary

- Public signatures, enum values, defaults, and comments in the relevant header are authoritative for that interface.
- Runtime behavior must be confirmed in the matching implementation; a header comment or design-contract file alone does not establish that behavior is implemented or validated.
- API compatibility/stability must be determined by the repository's current versioning policy; this page does not designate every header as a stable ABI.
- Process model deployment to `ProcessGraphManager` is an integration surface, not evidence that the process module owns a general workflow orchestration service.
- Federation, audit, diagnostics, and telemetry contracts are present. Their existence does not establish production maturity; current implementations include explicit alpha/simplified behavior.

## Public components and data flow

The public headers cover model lifecycle and format handlers, linking, graph/agentic/light retrieval, diagnostics, analytics, and integration surfaces. At runtime, callers provide records or import supported formats through the model manager; retrieval combines available model/instance data and optional link/community evidence. The detailed implementation flow and limits are documented in the [module architecture](../../src/process/ARCHITECTURE.md).

## Failure paths

- Invalid or missing input can produce failure results, empty retrieval material, or explicit diagnostics depending on the operation; inspect the specific API contract.
- GLOBAL retrieval may fall back to LOCAL when community reports are unavailable.
- Profile activation and rollback are multiple writes without a transaction guarantee.
- The light-retriever resource limits are declared but not enforced by the current implementation.

## Non-goals

- This header directory does not define ownership of a general workflow execution service.
- Listing an API or design contract here does not certify its implementation, deployment controls, or release readiness.

## Source and validation layout

- Implementation: [`../../src/process/`](../../src/process/)
- Tests: [`../../tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt)
- Benchmarks: [`../../benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt)
- Detailed architecture: [`../../src/process/ARCHITECTURE.md`](../../src/process/ARCHITECTURE.md)

The test and benchmark CMake files define registration, not proof of successful execution. There is no `src/process/CMakeLists.txt`.
