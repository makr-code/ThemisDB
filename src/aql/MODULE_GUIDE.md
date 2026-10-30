---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Module Guide

## Purpose

Use this guide to navigate and safely change ThemisDB's AQL assistance
module. It is for contributors integrating or maintaining the assistance
surfaces, not for implementing the core parser or query execution engine.

## Read in this order

1. [`README.md`](README.md) for module scope and responsibilities.
2. [`ARCHITECTURE.md`](ARCHITECTURE.md) for module boundaries and flow.
3. [`API.md`](API.md) and [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md)
   before changing caller-visible semantics.
4. [`IMPLEMENTATION.md`](IMPLEMENTATION.md) for source ownership and
   regression coverage.
5. [`SECURITY.md`](SECURITY.md) and [`TRUST.md`](TRUST.md) before changing
   prompt, generated-query, bridge, or tool execution paths.
6. [`ROADMAP.md`](ROADMAP.md), [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md),
   and [`CHANGELOG.md`](CHANGELOG.md) for current planned work and history.

## Source ownership

- Public assistance interfaces: [`include/aql/`](../../include/aql/).
- Internal assistance implementation: this directory, `src/aql/`.
- Parser and execution integration: [`include/query/`](../../include/query/)
  and [`src/query/`](../query/); keep the module boundary explicit.
- Focused unit tests: [`tests/aql/`](../../tests/aql/).
- Operator procedures: [`docs/operability/RUNBOOK_AQL_ASSISTANCE.md`](../../docs/operability/RUNBOOK_AQL_ASSISTANCE.md).

## Change workflow

1. Identify whether the change affects a public interface, internal behavior,
   provider integration, or operational contract.
2. Read the corresponding source and tests; do not treat planning or audit
   summaries as stronger evidence than source and frozen contracts.
3. Preserve bounded inputs/context, explicit provider failure behavior, and
   the distinction between parser validity and authorization.
4. Add or update a focused test under `tests/aql/` for the changed behavior.
   The test CMake file auto-discovers `test_aql_*.cpp` and applies the
   module-focused target/link conventions.
5. Run the relevant focused target(s) and the repository documentation checks.
   Consult the repository's CMake presets rather than inventing a build
   configuration.
6. Update API/implementation docs and the changelog when behavior or a frozen
   caller contract changes.

## Typical review questions

- Can the request or provider response exceed a configured bound?
- Is a retry safe and bounded, or is the failure explicitly non-retryable?
- Does a fallback preserve the safety requirement, not merely availability?
- Does the caller still enforce authorization before execution?
- Are handler, bridge, provider, and callback lifetimes explicit?
- Do tests cover empty inputs, unavailable dependencies, and exhausted limits?

## Further reading

- [`RESEARCH.md`](RESEARCH.md), [`MODELING.md`](MODELING.md), and
  [`EMBEDDING.md`](EMBEDDING.md) cover evaluation and optional semantic ranking.
- [`RUNBOOK.md`](RUNBOOK.md) and [`OPERATIONS.md`](OPERATIONS.md) cover incident
  triage and day-to-day service operation.
