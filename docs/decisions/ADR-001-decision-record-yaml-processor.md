---
Datum: 2026-04-17
Status: Accepted
Primary (Quelle der Wahrheit): docs/decisions/ADR-001-decision-record-yaml-processor.md
Bezug / Reference: include/llm/decision_record_yaml_processor.h · src/llm/decision_record_yaml_processor.cpp · docs/issues/llm/DR-001-decision-record-yaml-integration.md
---

<!-- Breadcrumb: [docs](../README.md) › [decisions](.) › ADR-001 -->

# ADR-001 — Async YAML Decision Record Processor for LLM/LoRA

## Status

**Accepted** — 2026-04-17

## Context

ThemisDB's LLM/LoRA stack makes thousands of autonomous decisions per hour:
federated aggregation rounds, LoRA adapter selections, threshold updates,
circuit-breaker transitions, GDPR erasure triggers.  For compliance,
auditability, and reproducibility these decisions must be persisted with
sufficient context to reconstruct *what* was decided, *why*, and *what
happened*.

Two existing mechanisms were evaluated:

| Mechanism | Pros | Cons |
|-----------|------|------|
| `AIDecisionAuditor` (RocksDB) | Rich, queryable, signed | Heavy dependency; blocks inference hot-path; requires PKI setup |
| Structured logging (spdlog) | Lightweight | Non-structured; hard to parse; mixed with operational logs |

Neither was suitable as a *standalone*, decoupled traceability layer for the
LLM/LoRA subsystem.

## Decision

Introduce `DecisionRecordYamlProcessor` — a lightweight, **asynchronous, single
background-thread YAML writer** independent of RocksDB, PKI, and the core
ThemisDB inference path.

Key design choices:

1. **YAML over JSON** — Human-readable without tooling; can be `diff`-ed,
   `grep`-ed, and reviewed in a pull request.  YAML also renders natively in
   GitHub and most documentation portals.

2. **Async background thread** — `submit()` enqueues the record into a
   lock-free bounded queue.  The writer thread drains the queue and flushes
   to disk.  The LLM/LoRA caller never blocks on I/O.

3. **File-per-record, date-partitioned directory** — Path:
   `logs/decisions/YYYY-MM-DD/<timestamp>_<type>_<id>.yaml`
   This allows per-day archival, rotation by external tooling (logrotate,
   object-store sync), and avoids any single large file growing unboundedly.

4. **Optional injection via `setDecisionRecordProcessor()`** — Components
   that produce decisions accept a `shared_ptr<DecisionRecordYamlProcessor>`.
   When `nullptr`, no overhead is incurred.  This follows the existing
   injection pattern used for `AIDecisionAuditor`, `ZeroTrustPolicyEnforcer`,
   etc.

5. **Backpressure via `max_queue_depth`** — If the writer thread falls behind
   (e.g. disk full), `submit()` returns `false` (dropped) rather than blocking
   or crashing.

6. **Not a replacement for `AIDecisionAuditor`** — The YAML records are for
   operational traceability and developer review.  Compliance-grade PKI-signed
   audit records continue to be written by `AIDecisionAuditor`.

## Consequences

### Positive

- Zero impact on inference latency (async thread, bounded queue).
- Human-readable output; trivially searchable with `grep`/`jq`.
- No new system dependencies (yaml-cpp is already in the build graph).
- Easy to disable per deployment via `nullptr` injection.
- Compatible with CI log-artifact collection.

### Negative / Risks

- Disk write latency not measured per-record; YAML is larger than binary.
- `yaml-cpp` serialisation is single-threaded within the processor thread.
- Date-partitioned directories accumulate over time; external rotation required.

### Mitigations

- `max_queue_depth` (default: 4096) caps memory usage under disk-failure
  scenarios.
- Integration tests assert file creation and content within 200 ms.
- `.gitignore` excludes `logs/decisions/**/*.yaml`; only `.gitkeep` committed.

## Alternatives Considered

| Alternative | Reason Rejected |
|-------------|-----------------|
| OpenTelemetry spans | Too heavy; requires collector sidecar; not self-contained |
| SQLite per-component DB | Single-writer bottleneck; requires schema migrations |
| Direct `AIDecisionAuditor` integration | RocksDB + PKI dependency; blocks hot-path |
| Binary protobuf files | Not human-readable; harder to review in PRs |

## Related

- `include/llm/decision_record_yaml_processor.h`
- `src/llm/decision_record_yaml_processor.cpp`
- `tests/test_decision_record_yaml_processor.cpp`
- `docs/issues/llm/DR-001-decision-record-yaml-integration.md`
- `docs/issues/MASTER_IMPLEMENTATION_PLAN.md` — §S-16
