# Ingestion Module - Future Enhancements

<!-- Status: current | validated: 2026-09-24 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of multi-source ingestion runtime behavior
- expansion of deterministic reliability under sustained connector and workflow load
- stronger benchmark-backed guardrails for ingestion hot paths

## Design Constraints

- ingestion contracts remain backward compatible within major release line.
- connector and control paths remain explicit and deterministic.
- validation/quality and workflow behavior remains bounded and observable.
- degraded connector modes remain explicit and non-silent.

## Required Interfaces

| Interface | Requirement |
|---|---|
| connector interfaces | deterministic source intake and error semantics |
| control interfaces | bounded retry/rate/checkpoint/quarantine behavior |
| quality interfaces | explicit validation/judge outcome semantics |
| workflow interfaces | stable step orchestration and adapter integration |

## Implementation Notes

- tighten parity across filesystem/API/stream/object/database/crawler connectors.
- standardize diagnostics for validation, connector, and workflow incidents.
- expand resilience tests for prolonged mixed-source ingestion workloads.
- broaden benchmark depth for extraction and quality-judge intensive scenarios.

## Test Strategy

- unit and integration suites for connector and orchestration behaviors.
- regressions for unsupported/degraded connector and workflow scenarios.
- deterministic stress runs for high-throughput ingestion operations.
- release-profile benchmark runs for mapped ingestion targets.

## Performance Targets

- control-plane and connector operations remain inside regression budgets.
- ingestion hot paths remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict validation and bounded retries before commit paths.
- preserve explicit failure signaling for connector capability issues.
- enforce bounded workflow behavior under malformed/partial inputs.
- keep diagnostics actionable for production ingestion incidents.
## RAG-Readiness Audit Backlog (2026-09-23)

- [ ] Chunking-/Embedding-Lifecycle als verbindlichen Vertrag abbilden (Target: Q4 2026)
  - Rationale: Retrieval-Qualitaet steht/faellt mit konsistentem Ingestion-Profil fuer Chunking und Embeddings.
  - Umsetzungsschritte: (1) Canonical chunking_profile IDs definieren, (2) Embedding-Modellversion in ingest metadata schreiben, (3) Re-ingest/reindex Trigger bei Profilwechsel.
  - Abhaengigkeiten: `src/rag/rag_ingestion_bridge.cpp`, `src/llm/wiki_index_store.cpp`, `src/index`.
  - Messbares DoD: 100% ingestierter RAG-Dokumente enthalten `chunking_profile` + `embedding_model_id`; kein silent-mix unterschiedlicher Profile im gleichen Index.

- [ ] Freshness-SLA fuer ingestion-to-index Pipeline etablieren (Target: Q1 2027)
  - Rationale: Produktions-RAG benoetigt messbare Aktualitaet bei CDC/API/Object-Storage Ingestion.
  - Umsetzungsschritte: (1) End-to-end Delay-Metrik erfassen, (2) Priorisierungsstrategie fuer hot documents, (3) Alarmierung bei SLA-Verletzung.
  - Abhaengigkeiten: `src/observability`, `src/index`, `src/server`.
  - Messbares DoD: p95 delay ingest->searchable < 5 Minuten (tier-konfigurierbar), Alert bei Ueberschreitung innerhalb 1 Minute.
