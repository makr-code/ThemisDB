# Retrieval Module - Future Enhancements

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- transition EPIC 1 retrieval contracts from scaffold to production behavior
- deepen reliability, observability, and policy controls across retrieval layers
- complete benchmark-backed readiness gates for release integration

## Design Constraints

- keep contract compatibility within the active major release line
- preserve explicit dependency sequencing with EPIC 2 and EPIC 3
- fail safely when optional backend capabilities are unavailable

## Planned Enhancement Themes

1. runtime hardening for error handling and degraded-mode behavior
2. deeper contract and scenario test coverage for multi-stage retrieval flows
3. benchmark-backed performance envelopes and release gates
4. clearer operational diagnostics for policy/governance decisions

## Validation Strategy

- expand `tests/epic1_retrieval/` for contract, edge-case, and integration scenarios
- implement `benchmarks/epic1_retrieval/` for stage-level and end-to-end profiling
- promote only after roadmap phase-gate criteria are satisfied

## RAG-Readiness Audit Backlog (2026-09-23)

- [ ] Retrieval-Scopes zwischen README/ROADMAP/Future Enhancements konsolidieren (Target: Q4 2026)
  - Rationale: Aktuelle Modultexte enthalten widerspruechliche Reifeaussagen (Scaffold vs. fortgeschrittener Rollout).
  - Umsetzungsschritte: (1) deklarativen Ist-Scope fuer `src/retrieval` festschreiben, (2) nicht gelieferte EPIC-Surfaces explizit als geplant markieren, (3) Verweise auf `src/rag`/`src/index` fuer aktive Retrieval-Pfade klaeren.
  - Abhaengigkeiten: `src/retrieval/README.md`, `src/retrieval/src/README.md`, `src/retrieval/ROADMAP.md`.
  - Messbares DoD: Keine widerspruechlichen Reifestatus mehr in retrieval-nahen Moduldokumenten; alle Claims source-verifizierbar.

- [ ] Verbindlicher Delivery-Entscheid fuer EPIC1 Retrieval-Sources (Target: Q1 2027)
  - Rationale: Fuer RAG-Planbarkeit muss klar sein, welche Retrieval-Surfaces real geliefert werden oder dauerhaft out-of-scope bleiben.
  - Umsetzungsschritte: (1) pro geplanter Source (`ann_frontdoor`, `tensor_midlayer`, `graph_validator`, ...) Entscheidung `implementieren` vs `retiren`, (2) Gate-Kriterien je Entscheidung, (3) CMake/Test-Mapping.
  - Abhaengigkeiten: `src/index`, `src/query`, `src/rag`, `tests/epic1_retrieval`.
  - Messbares DoD: 100% der EPIC1-Sources haben Status + Termin + Test-Gate; kein "unassigned"-Eintrag.
