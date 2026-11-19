# Content Module - Future Enhancements

<!-- Status: current | validated: 2026-11-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · MODULE_GAPS_BATCH5.md · CONTENT_DEFERRED_FEATURES.md -->

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone.
-->

## Scope

- hardening and refinement of ingestion/extraction/enrichment runtime behavior
- expansion of deterministic reliability under mixed-format and async-pressure scenarios
- stricter benchmark-backed guardrails for content processing hot paths
- Batch 5 finalization: documentation synchronization and production readiness gates for v2.4.0 GA

## Deferred Features from Batch 5 (CMT-7502 TODO Scan)

The following features are deferred past v2.4.0 GA; see `CONTENT_DEFERRED_FEATURES.md` for complete inventory:

- **Optimization:** Replace linear search with hash table for 1000+ patterns in abuse_detector (Performance, Q4 2026)
- **Feature Flag:** Add geospatial distance filtering when CUDA support available (Enhancement, Q4 2026)
- **Vendor Integration:** Integrate with Cloudflare Abuse Database API (pending contract, Q4 2026)
- **Archive:** Expand compression format support beyond ZIP/TAR (Enhancement, Q1 2027)
- **OCR:** Add handwriting recognition for scanned documents (Enhancement, Q1 2027)

## Design Constraints

- content contracts remain backward compatible within major release line.
- validation and policy gates remain explicit and fail-closed.
- optional processor degradation paths remain bounded and observable.
- ingestion state transitions remain auditable and diagnosable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| ingestion interfaces | deterministic intake and routing semantics |
| validation/security interfaces | explicit pre-processing rejection behavior |
| extraction/enrichment interfaces | stable format-aware processing contracts |
| async/ops interfaces | bounded queue behavior and operational observability |

## Implementation Notes

- **Versioning correctness:** replace process-local-only state with an explicitly
  selected durable/concurrent contract before using the helper for audit or recovery;
  distinguish empty snapshots from metadata-only entries, validate metadata-only hashes
  and sizes, and prevent version-number reuse after deletion. Do not advertise rollback
  until an implemented API and recovery test exist.
- **Validation capabilities:** reject or explicitly report configuration that requests
  unavailable malware, PII, abuse, schema, or timeout enforcement. A check is enabled
  only when its implementation runs and its result is observable.
- **Retry and fallback:** validate stage JSON fail-closed, set and enforce per-stage
  attempt and total-delay ceilings, and either implement `AsyncIngestionConfig` retries
  or remove/deprecate the unused setting through the approved compatibility process.
  Keep optional format fallbacks distinct from successful extraction in the result.
- **Evidence:** add manager-level tests for required format routes, invalid/degraded
  capability states, stage recovery, and versioning edge cases before expanding support.

## Test Strategy

- Require 100% pass on focused versioning, validation, security, pipeline, and streaming
  tests for the release-candidate SHA, including empty snapshots, deleted latest
  versions, metadata-only versions, malformed config, unavailable capability, and
  transient/permanent failure outcomes.
- Add end-to-end coverage only for the selected manager/caller routes; standalone
  processor tests do not establish end-to-end support.
- Assert retries do not exceed an explicitly approved attempt and elapsed-delay ceiling;
  test the async worker separately because its current `retry_attempts` setting is unused.
- Run registered production-path benchmarks with a complete workload/build/hardware
  manifest and compare p99 with the reviewed baseline using the 10% ceiling in
  [performance expectations](PERFORMANCE_EXPECTATIONS.md).

## Performance Targets

- For each selected production benchmark, p99 is no more than 110% of its reviewed
  baseline on matched hardware and workload.
- 100% of release-mapped benchmark cases are present in the candidate run manifest;
  synthetic and mock cases do not count.
- No p95/p99 or throughput claim is accepted without reproducible raw results and
  environment metadata. Until then, the gate remains pending.

## Security / Reliability

- 0 invalid stage configurations silently fall back to defaults; unavailable required
  validation capabilities fail before ingestion, while optional skips remain observable.
- All retry/recovery paths have enforced finite attempt and elapsed-time ceilings; an
  unavailable required processor never becomes a success-only metadata fallback.
- Archive and queue limits remain tested at just-below, exact, and just-above boundaries.
- Evidence artifacts are from the exact release-candidate SHA and no older than 14 days
  at sign-off; missing or stale evidence is a blocker, not a pass.
