## Summary

Introduce Adaptive Thinking orchestration for ThemisDB llama.cpp inference to dynamically scale test-time compute by request complexity, SLA, and endpoint criticality.

## Goal

Deliver measurable quality gains on complex requests while preserving low latency on simple requests, with full rollback safety and observability.

## Milestone

v1.22.0

## Scope

- Complexity scoring and policy profiles
- Fast/deep pass orchestration with deterministic fallback
- Adaptive RAG depth (top_k/rerank budget)
- Validator-driven escalation
- Metrics, logs, traces, dashboards
- Tests, runbook, release docs

## Sub-Issues

- [ ] #TBD - Design Spec + Config Contract
- [ ] #TBD - Difficulty Scorer + Policy Engine
- [ ] #TBD - Fast/Deep Orchestrator + Fallback
- [ ] #TBD - Adaptive RAG Depth Controller
- [ ] #TBD - Validator + Escalation Guardrails
- [ ] #TBD - Telemetry, Dashboards, and Alerts
- [ ] #TBD - Test Matrix (unit/integration/perf)
- [ ] #TBD - Rollout Runbook + Documentation

## Dependency Plan

- Design contract blocks all implementation tickets.
- Orchestrator depends on scorer/policy and rag controller.
- Test and rollout tickets depend on core implementation and telemetry.

## Acceptance Criteria

- Adaptive mode works in off/shadow/active.
- Deterministic fallback to static behavior on errors.
- p95 latency for simple classes does not regress > 10%.
- Quality proxy for complex classes improves >= 8%.
- Deep pass ratio remains within configured limits.

## Labels

- type:feature
- area:llm
- area:rag
- area:server
- area:observability
- priority:P1
