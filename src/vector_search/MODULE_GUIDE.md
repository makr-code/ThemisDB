---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Vector Search Module Guide

## Purpose

This page is the contributor-facing guide for the vector-search documentation and governance layer in `src/vector_search/`. It explains the ownership boundary, the source-of-truth implementation, and the validation paths that matter when making changes.

## Ownership model

This directory does not contain a second runtime implementation. It is the documentation, evidence, and governance layer for the live vector-search subsystem.

Source of truth:

- `include/index/vector_index.h`
- `include/index/multi_vector_search.h`
- `src/index/vector_index.cpp`
- `src/index/advanced_vector_index.cpp`
- `src/index/multi_vector_search.cpp`

Related validation artifacts:

- `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
- `tests/integration/test_vector_search_soak.cpp`
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`

## How to read the module

Read the docs in this order:

1. `README.md` — module scope and overview.
2. `ARCHITECTURE.md` — runtime boundaries and failure domains.
3. `RESEARCH.md` — assumptions and trade-offs.
4. `MODELING.md` — retrieval model and invariants.
5. `IMPLEMENTATION.md` — runtime responsibilities and validation model.
6. `API.md` — public contract, configuration, and result semantics.
7. `RUNBOOK.md` and `OPERATIONS.md` — diagnosis and recovery flow.
8. `TRUST.md` — risk and security posture.

## When to update documentation

Update these docs when any of the following changes:

- metric semantics or supported enums change,
- index lifecycle or persistence behavior changes,
- advanced backend support is added or removed,
- search fusion or ranking heuristics change,
- operational SLOs or benchmark gates change,
- security or trust controls change.

## Contributor workflow

1. Identify the implementation in `src/index/` or `include/index/`.
2. Check the relevant module doc before changing logic.
3. Validate the minimal test or benchmark covering the behavior.
4. Keep the documentation aligned with the actual behavior, especially for failure modes and operational limits.
5. If the module contract changes, update both source and docs in the same patch.

## Validation commands

The module's focused validation path is:

```bash
python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
```

For feature-level validation in the repo, use the targeted vector-search tests and benchmark gates that already exist in the project tree.

## Guardrails

- Do not duplicate the runtime under `src/vector_search/`.
- Do not claim future capability as current capability.
- Do not soften invalid inputs into silent fallback behavior.
- Keep the governance docs aligned with benchmark, test, and code evidence.

## Conclusion

The vector_search module is a governance and evidence layer around the real index runtime. It exists to keep contract boundaries, validation expectations, and operational risk visible without creating a second source of truth.
