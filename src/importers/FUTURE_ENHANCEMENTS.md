---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; src/importers/README.md; src/importers/ROADMAP.md; tests/importers/CMakeLists.txt; benchmarks/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7426"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# Importers Module - Future Enhancements

> This is a proposed, evidence-first plan for v2.5.0 and v3.0.0, not evidence
> that the capabilities below are implemented or approved as release gates.
> Current assessment, wave dependencies, estimates, risks, metrics, and research
> references are in [`ROADMAP.md`](./ROADMAP.md).

<!-- Historical planning metadata; no current delivery commitment is implied. -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- v2.5.0: verify and harden selected existing file, relational, CDC/stream, and
  object import paths, prioritizing live correctness, bounded failure behavior,
  and reproducible evidence over connector-count growth.
- v3.0.0: subject to an approved major-version ADR, evolve toward a
  capability-driven importer API, durable replay/recovery, scalable streaming,
  and governed plugin/connector operations.
- These are proposals; root release governance and human approval remain
  authoritative for dates, breaking changes, and release readiness.

## Design Constraints

- Keep v2.5.0 changes source-compatible unless a separate API review approves an
  additive extension; breaking public changes require v3.0.0 migration planning.
- Do not infer connector capability from shared option or interface presence.
  Unsupported behavior must fail deterministically and be visible.
- Use at-least-once plus idempotent application as the default replay design.
  Claim exactly-once only when tests establish a source/coordinator/sink atomic
  boundary for the named connector combination.
- Preserve declared ordering scope, enforce configured resource bounds, and
  distinguish cancellation, rollback, partial progress, and committed state.
- Keep checkpoints, quarantine, and diagnostic data under explicit access,
  redaction, and retention policies; do not make a compliance claim from
  documentation alone.

## Required Interfaces

| Interface | Requirement |
|---|---|
| `IImporter` and connector APIs | Document the actual per-connector capabilities for ordering, cancellation, timeout, quotas, checkpointing, and permission checks; add tests for advertised behavior |
| `importers_api_contract.h` and coordinator | v2.5.0: identify a live consumer and validate the supported visibility/idempotency guarantees; v3.0.0: version capability, retry, ordering, and commit semantics |
| schema and conflict APIs | Test additive/breaking evolution, deterministic conflict outcomes, error locations, and bad-row disposition on each selected connector |
| Kafka/CDC interfaces | Define source position, ordering scope, offset/checkpoint boundary, replay behavior, and late/out-of-order event handling |
| plugin APIs | Keep current ABI behavior in v2.5.0; design explicit compatibility/capability negotiation and migration path for v3.0.0 |
| diagnostics and audit callbacks | Define structured fields, secret/raw-row redaction, artifact ownership, retention, and safe retry information |

## Implementation Notes

- Follow `ROADMAP.md` Waves 1–4 for v2.5.0 and Waves 5–7 for v3.0.0.
- Start with a source-backed connector/capability matrix; do not pursue parity
  where the connectors have materially different commit or resume semantics.
- For v2.5.0, repair or replace the three test sources excluded by
  `tests/importers/CMakeLists.txt` and add opt-in live tests for selected
  flat-file, relational, and Kafka/CDC paths.
- For v3.0.0, prototype durable checkpoint/commit coordination and bounded
  backpressure/partition concurrency only after contract and architecture
  review.
- Preserve the current synthetic IMRG microbenchmarks as microbenchmarks. Add
  end-to-end connector workloads before publishing throughput or tail-latency
  claims.

## Test Strategy

- Run all maintained importer CTest targets in a test-enabled configuration;
  separately report the current skipped suites until repaired or replaced.
- Add local-service integration and restart/failure-injection tests for each
  selected connector; test before/after source acknowledgement, checkpoint, and
  destination commit.
- Cover schema changes, conflict behavior, malformed/oversized rows, limits,
  permissions, cancellation, timeout, redaction, and recovery by capability.
- Keep unit tests deterministic and external-service tests explicit; mocks are
  not evidence of live connector behavior or database atomicity.
- For v3.0.0, test replay and ordering across duplicate, late/out-of-order, and
  interrupted streams with the declared ordering and delivery scope.

## Performance Targets

- v2.5.0: record at least five independent Release-profile runs per end-to-end
  workload, including throughput, p50/p95/p99, CPU, peak RSS, errors, and
  recovery lag; retain machine-readable outputs with workload, hardware, and
  build metadata.
- Compare only with a same-environment baseline; investigate any p95 regression
  greater than 10% before accepting a release comparison.
- Establish absolute throughput and latency gates only after capturing a named,
  representative baseline. Existing IMRG labels are not measured p99 or
  enforced thresholds.
- v3.0.0: report throughput and tail latency at 1, 2, and 4 workers, with queue
  and memory bounds enabled; do not trade away declared ordering or correctness
  to improve the throughput result.

## Security / Reliability

- Test row/byte/batch/timeout bounds at and beyond configured limits; malformed
  input and schema growth must not bypass those bounds.
- Verify permission-denied behavior and ensure credentials and raw sensitive
  rows are absent from captured diagnostics.
- Define access and retention for checkpoints, quarantine, audit, and recovery
  artifacts; test that cancellation and retry do not imply rollback or
  exactly-once behavior.
- Exercise connector outages, restart/replay, and resource exhaustion. Keep
  private credentials and artifacts out of Community/Minimal workflows.

## Research Inputs

- Kafka and Debezium delivery-semantics documentation: at-least-once can produce
  duplicates; exactly-once depends on a supported transactional boundary.
- Apache Beam Programming Guide: watermarks and late data inform CDC/stream
  ordering and replay test cases.
- Wang, Boissier, and Rabl, *A Survey of Stream Processing System Benchmarks*
  (2026, DOI 10.1007/978-3-031-93858-0_2): informs multi-dimensional,
  workload-specific benchmark coverage.
- NIST SP 800-92, *Guide to Computer Security Log Management*: informs the
  operational lifecycle of importer audit and diagnostic records.
- Links and the source-to-gap mapping are maintained in
  [`ROADMAP.md`](./ROADMAP.md); these sources are design inputs, not evidence
  that any importer capability or compliance control is delivered.