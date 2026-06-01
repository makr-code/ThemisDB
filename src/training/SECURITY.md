# Security - Training Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the training module focuses on deterministic training-data handling, explicit checkpoint and adapter lifecycle behavior, provenance-preserving orchestration, and bounded serving-handoff semantics.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| hidden corruption in checkpoints or adapters | explicit checkpoint lifecycle and adapter error behavior |
| opaque low-confidence or mislabeled sample flow | bounded labeling and provenance tracking behavior |
| unsafe deploy/rollback handoff | explicit adapter serving handoff outcomes |
| silent enrichment or selection degradation | diagnosable enrichment and data-selection behavior |

## Implemented Security Controls

- training, checkpoint, and adapter operations expose explicit outcomes.
- provenance and selection behavior remains observable and auditable.
- low-confidence and enrichment edge cases remain diagnosable.
- serving-handoff paths remain explicit and bounded.

## Security Follow-ups

- broaden fault-injection coverage for checkpoint corruption and rollback edge cases.
- deepen stress coverage for concurrent training and adapter lifecycle scenarios.
- tighten diagnostics taxonomy across labeling, checkpoint, and serving incidents.

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
