---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/query/aql_parser.h`, `include/query/query_engine.h`, `src/query/aql_parser.cpp`, `src/query/query_engine.cpp`, `tests/query/CMakeLists.txt`
Related issues: #7093, #7167

# Query module guide

## Audience and ownership

For developers changing query parsing, planning/execution, or query-specific behavior. Public declarations live under `include/query/`; implementations and module documentation live under `src/query/`. Storage owns persistence; query callers should use its interfaces rather than add storage behavior here.

## Start with the affected surface

- Parser/AST: `include/query/aql_parser.h`, `src/query/aql_parser.cpp`.
- Execution: `include/query/query_engine.h`, `src/query/query_engine.cpp`.
- Then inspect the adjacent implementation and tests for the particular query feature; the module has more than one query form and executor.

## Maintainer workflow

1. Confirm the public signature and result type before changing an implementation.
2. Trace the specific parse or execution call path; do not assume all AQL features share identical validation.
3. Keep typed errors visible and preserve the declared result contract.
4. Add a focused regression in `tests/query/`; run its registered target from `tests/query/CMakeLists.txt`.
5. Update `API.md` for caller-visible contract changes and `IMPLEMENTATION.md` for implementation/invariant changes.

## Failure cases to cover

Malformed input, scope-validation failure, and executor errors are separate boundaries. Test the one affected by the change and verify that failures are not consumed as successful results.

See `README.md`, `ARCHITECTURE.md`, and `ROADMAP.md` for wider module context.
