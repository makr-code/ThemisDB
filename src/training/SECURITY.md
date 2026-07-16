# Security - Training Module

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the training module focuses on deterministic training-data handling, explicit checkpoint and adapter lifecycle behavior, provenance-preserving orchestration, and bounded serving-handoff semantics.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| hidden corruption in checkpoints or adapters | explicit checkpoint lifecycle and adapter error behavior; manifest integrity validation rejects path-traversal and malformed SHA-256 fields |
| opaque low-confidence or mislabeled sample flow | bounded labeling and provenance tracking behavior |
| unsafe deploy/rollback handoff | explicit adapter serving handoff outcomes; `llm_router_->setAdapterWeight` calls protected by mutex to prevent data races |
| silent enrichment or selection degradation | diagnosable enrichment and data-selection behavior |
| unbounded provenance write blocking | configurable `write_timeout_ms` enforces a per-call deadline in `ProvenanceTracker::write` |

## Implemented Security Controls

- training, checkpoint, and adapter operations expose explicit outcomes.
- provenance and selection behavior remains observable and auditable.
- low-confidence and enrichment edge cases remain diagnosable.
- serving-handoff paths remain explicit and bounded.

## Security Follow-ups

- broaden fault-injection coverage for checkpoint corruption and rollback edge cases.
- deepen stress coverage for concurrent training and adapter lifecycle scenarios.
- tighten diagnostics taxonomy across labeling, checkpoint, and serving incidents.

## Scanner Finding Resolution (issue #5414, batches 1–6, 13–14)

All 295 Critical/High findings from the gap scanner have been resolved:
- **Fixed (7 genuine defects):** data_race in `incremental_lora_trainer` (router
  mutex), `adalora_tt_bridge` (fingerprint_graph mutex), and `adalora_tt_bridge`
  export-cache access (`cache_mutex_`); model_integrity_gap in
  `lora_checkpoint_manager` (path-traversal + SHA-256 field validation);
  no_timeout in `provenance_tracker` (write_timeout_ms enforcement).
- **Confirmed false positives (288 findings):** full justification per category in
  `MODULE_GAPS.md` batches 3–6 and post-fix notes in batch 14; root causes include
  scanner triggering on float-tensor variable names, auto-generated comment
  headers, pimpl-pointer calls, and sentinel equality comparisons. No exploitable
  defects remain open.

## Sourcecode Verification (Module: training/security)

- Verified files:
  - src/training/auto_labeler.cpp
  - src/training/incremental_lora_trainer.cpp
  - src/training/lora_checkpoint_manager.cpp
  - src/training/lora_adapter.cpp
  - src/training/adapter_serving.cpp
  - src/training/provenance_tracker.cpp
- Verified controls:
  - explicit checkpoint and adapter fault signaling
  - observable labeling and provenance behavior
  - deterministic serving-handoff outcomes

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
