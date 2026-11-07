# Ingestion Module - Future Enhancements

<!-- Status: planning/backlog; implementation not implied | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · RESEARCH.md · PERFORMANCE_EXPECTATIONS.md · EMBEDDING.md -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Scope

- v2.5.0: close source-backed output, error-contract, untrusted-input, recovery,
  and evidence gaps without an intentional breaking API change.
- v3.0.0: version the ingestion job/record contract and qualify distributed
  recovery, tenant controls, embedding/index lifecycle, and scaling.
- Both release labels are planning targets only; release scheduling remains
  governed by the root roadmap and release strategy.

> The items below are proposals/backlog, not claims that the behavior is
> implemented. The RAG lifecycle items in the final section are not covered by
> `ReindexDecisionEngine` alone; see [`EMBEDDING.md`](./EMBEDDING.md) for current
> source-backed behavior and limits.

## Design Constraints

- ingestion contracts remain backward compatible within major release line.
- connector guarantees must be capability-specific; do not infer common
  ordering, replay, idempotency, or transaction semantics.
- finite limits, retry policy, and degraded behavior must be explicit for
  untrusted or unbounded sources. Preserve zero/unlimited compatibility unless a
  migration is approved.
- acknowledgement means the documented sink commit point, not retrieval,
  extraction, or queue admission.
- unavailable validators/inference backends and partial results must remain
  explicit and observable.
- v3.0.0 may version or break contracts only with compatibility and migration
  tests plus release-note guidance.

## Required Interfaces

| Interface | Requirement |
|---|---|
| connector interfaces | deterministic source intake and error semantics |
| control interfaces | bounded retry/rate/checkpoint/quarantine behavior |
| quality interfaces | explicit validation/judge outcome semantics |
| workflow interfaces | stable step orchestration and adapter integration |
| sink and checkpoint interfaces | explicit commit visibility, durable resume, and replay/idempotency capabilities |
| job/record metadata | versioned source identity, position, schema, and transformation provenance |

## Implementation Notes

- deliver workflow-produced entities to the selected sink, or return an
  explicit failure; reconcile duplicate error-code declarations and the
  CMake-skipped focused error-contract suite.
- document connector-specific capabilities and define finite production
  profiles, hostile URL/redirect policy, file-type/size limits, and secret
  redaction.
- prove restart/replay with a durable checkpoint and selected sink; advertise
  only delivery semantics supported by each source/sink pair.
- implement the approved versioned job/record contract and shared checkpoint
  coordination only for connectors that declare the required capabilities.
- publish a connector capability and setup matrix covering required
  configuration, limits, authentication, failure/retry behavior, and sink
  guarantees; verify the examples with the focused connector tests.
- treat embedding/index migration as a lifecycle: persist manifests and test
  dual-read/rollback if approved, or retain an explicit unsupported boundary.
- attach matching workload, hardware, software/build, offered-load, and
  result artifacts to performance claims.

## Test Strategy

- unit and integration suites for connector and orchestration behaviors.
- regressions for unsupported/degraded connector and workflow scenarios.
- deterministic stress runs for high-throughput ingestion operations.
- crash/restart tests before and after checkpoint and sink commit; compare the
  final visible record set for missing and duplicate records.
- injected DNS/HTTP tests for denied destinations and redirects, plus parser
  tests for oversized and malformed inputs; do not make real network calls in
  unit tests.
- contract conformance and schema-evolution tests per connector/sink
  capability.
- release-profile benchmarks and production-shaped connector-to-sink tests;
  include slow-sink/backpressure and load-generator saturation cases.

## Performance Targets

- establish a reproducible baseline per supported connector/sink and hardware
  class before setting a production capacity SLO.
- report offered and completed records/sec, p50/p95/p99 latency, error rate,
  queue/backpressure time, checkpoint lag, and peak/steady-state memory.
- proposed v2.5.0 regression gate: no more than 10% p99 regression for the
  same benchmark case, build, and hardware class; ratify only after baseline
  review.
- proposed v3.0.0 scaling gate: four-worker throughput at least 2.0x the
  one-worker result on the same representative workload/hardware, with the
  agreed p95/p99 SLO met and bounded memory. Ratify against v2.5.0 evidence.
- source-declared mock/in-memory thresholds remain microbenchmark regressions;
  they do not substitute for connector-to-sink capacity or durability results.

## Security / Reliability

- require a selected outbound-network policy for URL connectors; validate
  scheme, destination addresses, and redirect destinations, and document
  private-network exceptions.
- apply explicit file size/type and parser resource limits before expensive
  extraction; treat source headers and filenames as untrusted.
- keep credentials and sensitive payloads out of diagnostics, and specify
  access/retention for checkpoints, quarantine, and lineage.
- enforce authorization at the owning service; the in-process admin API is not
  an authentication boundary.
- preserve explicit failure signaling and bounded retry under connector
  outages, slow sinks, malformed data, and unavailable checkpoint stores.

## v2.5.0 — Priority backlog

- [ ] **Wave 1 — workflow result and error-contract correctness:** persist every
  successful filesystem-workflow fixture through its selected sink; report
  sink rejection as failure; reconcile `IngestionErrorCode` headers and
  re-register or replace the skipped focused suite. (Dependencies: sink/API
  owner; Target: v2.5.0.) **DoD:** all deterministic success fixtures are
  visible at the sink, all failures are counted, and canonical error-contract
  tests run in CTest.
- [ ] **Wave 2 — bounded intake and SSRF/file controls:** define finite
  production profiles and test address, redirect, payload-size, and malformed
  parser boundaries for relevant connectors. (Dependencies: connector
  transports and deployment policy; Target: v2.5.0.) **DoD:** all cases in the
  reviewed hostile-input corpus are rejected before unauthorized fetch or
  unbounded parsing; examples state their finite limits.
- [ ] **Wave 3 — durable restart/replay:** configure a durable checkpoint
  implementation and verify replay/idempotency at sink commit boundaries.
  (Dependencies: selected sink and source-position APIs; Target: v2.5.0.)
  **DoD:** a 10,000-record restart fixture has zero missing and zero duplicate
  visible records across injected restart points.
- [ ] **Wave 4 — production-shaped baseline:** publish three matching runs and
  a 12-hour mixed-source soak with full workload/hardware/build metadata.
  (Dependencies: Waves 1–3 and a representative environment; Target: v2.5.0.)
  **DoD:** baseline artifacts report p50/p95/p99, offered/completed rate,
  errors, queue/backpressure, checkpoint lag, and memory; no unexplained
  failures or unbounded queue/memory growth.

## v3.0.0 — Strategic backlog

- [ ] **Wave 5 — versioned job/record contract:** specify identity, ordering,
  schema evolution, retryability, acknowledgement, and capability negotiation.
  (Dependencies: v2.5.0 sink/replay evidence; Target: v3.0.0.) **DoD:** each
  supported connector/sink pair passes contract and compatibility tests.
- [ ] **Wave 6 — shared durable coordination:** add checkpoint/lease storage,
  fencing, and restart coordination only for capable source/sink pairs.
  (Dependencies: durable-store and connector capabilities; Target: v3.0.0.)
  **DoD:** a 3-worker fault-injection run recovers 10,000 records without
  missing/duplicate visible records and meets a published recovery/lag SLO.
- [ ] **Wave 7 — tenant and audit controls:** implement service-boundary
  authorization, tenant quotas, redaction, and retention controls for
  diagnostics, quarantine, lineage, and checkpoints. (Dependencies: server,
  auth, observability, and compliance owners; Target: v3.0.0.) **DoD:**
  unauthorized actions are denied, tenant quota tests remain isolated, and
  secret/PII fixtures do not appear in telemetry.
- [ ] **Wave 8 — embedding/index lifecycle:** persist version manifests and
  implement dual-read, routing, and rollback only if confirmed by the owning
  RAG/index design; otherwise document the unsupported boundary. (Dependencies:
  versioned metadata and index/RAG owners; Target: v3.0.0.) **DoD:** every
  supported model/dimension/chunk/schema change has a durable, tested migration
  state and no silent profile mixing.
- [ ] **Wave 9 — release scale qualification:** run representative 1/2/4-worker
  source-to-sink workloads and a 24-hour soak. (Dependencies: Waves 5–8 and
  agreed service objectives; Target: v3.0.0.) **DoD:** four-worker throughput
  is at least 2.0x single-worker throughput on the same workload/hardware,
  p95/p99 objectives are met, records are not lost/duplicated, and memory is
  bounded. Ratify the proposed threshold against v2.5.0 measurements.

Estimates, candidate performance gates, compliance scope, and release timing
are proposals; see [`ROADMAP.md`](./ROADMAP.md) and
[`RESEARCH.md`](./RESEARCH.md) for dependencies, evidence limits, and the
research basis.

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
