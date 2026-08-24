# Adaptive Thinking for ThemisDB (llama.cpp)

Status: Draft for implementation issue breakdown
Target Milestone: v1.22.0
Date: 2026-05-29
Owner Area: LLM / Server / RAG / Observability

## 1) Problem Statement

ThemisDB currently applies mostly static inference settings per request path. This leads to one-size-fits-all behavior:
- simple requests can be over-provisioned (unnecessary latency/cost),
- complex requests can be under-provisioned (quality/regression risk),
- no explicit policy exists to scale test-time compute by request complexity and SLA class.

Goal: introduce Adaptive Thinking orchestration around llama.cpp inference that dynamically adjusts compute budget, retrieval depth, and verification strategy based on request difficulty and service constraints.

## 2) Scope (v1.22.0)

In scope:
- Request complexity scoring (heuristic, deterministic first version).
- Policy engine mapping score + endpoint + SLA to inference profile.
- Two-stage execution path (fast pass, deep pass on low confidence / high complexity).
- Optional selective self-consistency for high-criticality flows.
- Adaptive RAG retrieval depth (top_k / rerank budget) for rag endpoints.
- Telemetry and dashboards for latency/quality/cost impact.
- Safety gates and rollout controls (feature flags, kill switch, shadow mode).

Out of scope for v1.22.0:
- Model-level architecture changes in upstream llama.cpp.
- New training pipelines or fine-tuning workflows.
- Full speculative decoding framework across multiple draft models.

## 3) Architecture Design

### 3.1 High-level flow

1. Request arrives at inference or rag endpoint.
2. Difficulty scorer computes complexity class (S0/S1/S2/S3).
3. Policy engine selects profile (token budget, temperature, top_k, verification mode, timeout).
4. Execute fast pass.
5. If escalation criteria match, execute deep pass and return improved answer.
6. Emit telemetry for scorer, policy, latency, token use, escalation, and outcome class.

### 3.2 Core components

A) Difficulty Scorer
- Input signals:
  - prompt length and structural markers (multi-step constraints, code/math/table asks),
  - endpoint type (inference vs rag vs docs-help),
  - optional retrieval metadata (candidate quality, entropy proxy).
- Output:
  - complexity class S0..S3,
  - confidence estimate for class assignment,
  - reason codes for observability.

B) Policy Engine
- Deterministic mapping from {endpoint, complexity, SLA tier} -> {profile}.
- Profile fields:
  - max_tokens,
  - temperature / top_p,
  - context budget,
  - rag top_k and rerank budget,
  - retry/escalation threshold,
  - hard/soft timeout,
  - self-consistency sample count (default 1).

C) Execution Orchestrator
- Fast pass first for all requests.
- Deep pass only when one of these is true:
  - low confidence from answer validator,
  - complexity class >= S2 and endpoint criticality high,
  - policy requires verification mode.
- If deep pass disabled by policy/flag, return fast pass with marker.

D) Answer Validator (lightweight)
- Non-semantic checks for v1:
  - empty/near-empty output,
  - hard truncation/hit token ceiling with low information density,
  - malformed JSON where JSON contract expected.
- Exposes escalation reason codes.

E) Adaptive RAG Controller
- S0/S1: lower top_k, strict timeout.
- S2/S3: expanded top_k, optional reranking path, higher generation budget.

F) Observability Pack
- Metrics:
  - adaptive_requests_total{endpoint,complexity,profile},
  - adaptive_escalations_total{reason},
  - adaptive_latency_ms_bucket{stage=fast|deep},
  - adaptive_tokens_generated_total,
  - adaptive_quality_guard_failures_total.
- Structured logs and trace attributes for request-level diagnosis.

## 4) API / Config Contract

Introduce config block (server config):

- adaptive_thinking.enabled: bool
- adaptive_thinking.mode: off | shadow | active
- adaptive_thinking.default_sla: standard | low-latency | high-quality
- adaptive_thinking.max_deep_pass_ratio: float [0..1]
- adaptive_thinking.self_consistency.max_samples: int
- adaptive_thinking.policy_profile_file: path (optional override)

Runtime controls:
- Admin endpoint to inspect active profile decisions (safe redacted view).
- Per-request override header/option for experiments (guarded; privileged only).

## 5) Reliability and Safety

- Feature flag with runtime kill switch.
- Shadow mode to compute decisions without changing answer path.
- Backpressure guard: disable deep pass when queue depth exceeds threshold.
- Deterministic fallback to current static behavior on any scorer/policy failure.

## 6) Test Strategy

Unit tests:
- scorer deterministic classification and reason codes,
- policy mapping table coverage,
- escalation trigger matrix,
- validator checks (empty/truncated/malformed contract).

Integration tests:
- inference endpoint profile selection smoke,
- rag adaptive top_k behavior across complexity classes,
- shadow mode no-output-change guarantee,
- kill-switch fallback behavior.

Performance tests:
- baseline vs adaptive on representative prompt suite,
- p50/p95 latency deltas per endpoint,
- token spend and escalation ratio,
- quality proxy pass rate (non-empty, contract-valid, truncation reduction).

## 7) Acceptance Criteria (v1.22.0)

Functional:
- Adaptive mode selectable via config and runtime status visible.
- At least 3 complexity classes used in production path.
- Fast/deep orchestration enabled with deterministic fallback.

Quality/Performance:
- No p95 latency regression > 10% on low-complexity class S0/S1.
- Measurable quality proxy improvement on S2/S3 (target >= +8%).
- Deep-pass utilization stays within configured max_deep_pass_ratio.

Operational:
- Metrics and logs integrated into existing observability pipeline.
- Runbook and rollback instructions documented.

## 8) Implementation Phases

Phase 1: Design and Contracts
- finalize scorer features, policy schema, config contract,
- define telemetry names and cardinality constraints,
- add docs for rollout and failure behavior.

Phase 2: Core Orchestration
- implement scorer + policy engine,
- implement fast/deep orchestrator and fallback behavior,
- wire into inference and rag endpoint flows.

Phase 3: Validation + Guardrails
- add answer validator and escalation reasoning,
- add backpressure/queue guards,
- implement shadow mode path.

Phase 4: Tests
- add unit and integration tests,
- add benchmark scenario set and golden prompts,
- verify deterministic behavior in off/shadow/active modes.

Phase 5: Observability + Hardening
- expose metrics, traces, structured logs,
- add dashboards and alert thresholds,
- perform staged rollout with canary and kill-switch rehearsal.

Phase 6: Documentation + Go-Live
- operator guide, config examples, troubleshooting,
- release notes for v1.22.0,
- milestone closeout validation.

## 9) Issue Breakdown Plan (Wrapper + Sub-Issues)

Wrapper issue:
- Epic: Adaptive Thinking for ThemisDB llama.cpp inference (v1.22.0)

Planned sub-issues:
1. Design Spec + Config Contract
2. Difficulty Scorer + Policy Engine
3. Fast/Deep Orchestrator + Fallback
4. Adaptive RAG Depth Controller
5. Validator + Escalation Guardrails
6. Telemetry, Dashboards, and Alerts
7. Test Matrix (unit/integration/perf)
8. Rollout Runbook + Documentation

Dependency chain:
- 1 blocks 2..8
- 2 and 4 feed 3
- 3 and 5 feed 7
- 6 and 7 feed 8

## 10) Risks and Mitigations

Risk: latency spikes from deep pass overuse.
Mitigation: max_deep_pass_ratio, queue-aware suppression, SLA-aware gating.

Risk: unstable complexity heuristics.
Mitigation: deterministic v1 scorer, shadow mode calibration, reason-code telemetry.

Risk: noisy metric cardinality.
Mitigation: fixed enums for complexity/profile/reason, avoid free-form tags.

Risk: behavior regressions in existing demos.
Mitigation: fallback path preserved, dedicated demo smoke checks (DEMO_QUERIES flow).

## 11) Deliverables

- production code for adaptive orchestration in inference/rag flows,
- config and admin visibility,
- tests and benchmark report,
- observability package,
- release/runbook documentation,
- linked wrapper + sub-issues in milestone v1.22.0.
