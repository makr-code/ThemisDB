# LLM Module - Future Enhancements

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

## Scope

This document covers forward-looking hardening and capability expansion for the LLM module beyond currently shipped behavior in core runtime paths.

## Design Constraints

- Preserve stable inference and streaming interfaces for existing callers.
- Avoid implicit policy bypass paths when introducing new routing/execution shortcuts.
- Keep model/adapter lifecycle ownership explicit across manager and plugin layers.
- Keep optional backend features degradable with clear fallback behavior.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| Engine submit and streaming APIs | server handlers, adapters, orchestration layers | Must stay backward-compatible across minor releases |
| Router and shard-load update interfaces | scheduler, route selection, distributed execution | Must provide deterministic fallback when telemetry is missing |
| Plugin and adapter lifecycle APIs | plugin manager, deployment/orchestration paths | Must expose explicit load/unload/error states |
| Policy and prompt guard interfaces | API handlers and inference submit paths | Must execute before backend inference calls |

## Implementation Notes

- Complete remaining distributed speculative-routing and fan-out hardening.
- Consolidate runtime metrics and error-surface semantics across async/enhanced engines.
- Expand adapter and cache isolation behavior for high-concurrency tenant scenarios.
- Align operator observability surfaces with real runtime controls and failure states.

### Wave B B3: Multi-Task LoRA Fine-Tuning
**Priority:** High
**Target:** Q1–Q2 2027

- design shared LoRA base with task-specific projection heads
- add domain-gating to route requests to task-specialized adapters
- add joint multi-task loss with configurable weighting
- add ablation workflow to compare shared vs separate adapters
- run three-task benchmark evaluation for transfer and robustness

## Test Strategy

- Add focused tests for distributed execution fallback and partial-failure merge behavior.
- Add stress tests for queue saturation, cancellation storms, and adapter churn.
- Add regression suites for policy enforcement and prompt safety checks under load.
- Keep benchmark-to-target mappings source-verifiable and release-repeatable.

## Performance Targets

- Maintain stable token throughput and prompt latency against current release baselines.
- Keep routing and orchestration overhead bounded under concurrent multi-shard requests.
- Keep cache and adapter operations within bounded latency envelopes under contention.

## Security / Reliability

- Preserve deny-by-default behavior for invalid or unauthorized inference requests.
- Ensure adapter/model path handling remains trust-bound and canonicalized.
- Ensure audit and diagnostics contain enough context for incident triage.
- Require explicit fallback signaling on partial distributed failures.

## Wave B Acceptance Gates (B3)

- average task performance ≥ +8% vs single-task baseline
- training-time increase ≤ 15%
- robust behavior across task configurations

## Related Documents

- AI wave tracker: `../ai/ROADMAP.md`
- bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Risk Backlog

- Distributed optimization regressions under heterogeneous backend availability.
- Cache consistency edge cases during rapid model/adapter lifecycle operations.
- Operational drift between documented and deployed backend feature combinations.
