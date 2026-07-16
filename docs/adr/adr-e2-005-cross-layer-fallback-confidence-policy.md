# ADR E2-005: Cross-Layer Fallback, Confidence, and Fail-Closed Policy

- Status: Accepted
- Date: 2026-06-17
- Deciders: ThemisDB Architecture / Retrieval Governance
- Supersedes: None
- Related:
  - GAP_ANALYSIS section 4.1
  - docs/implementation/CROSS_CUTTING_GAPS_ISSUE_TREE_2026-06-17.md (CCG-401, CCG-402)
  - include/rag/tensor_rag_pipeline.h
  - src/rag/tensor_rag_pipeline.cpp

## Context
The layered retrieval path (ANN -> Tensor -> Graph -> Final Layer) is implemented,
but fallback and confidence behavior is not yet uniformly governed across all
handoffs. This causes risk in production behavior consistency, explainability,
and incident handling.

## Decision
ThemisDB adopts one mandatory cross-layer policy contract with three parts:

1. Fallback contract
- Every layer must declare fallback mode explicitly per request:
  - `none`
  - `degraded_continue`
  - `fail_closed`
- Fallback must include a machine-readable reason code.
- Silent fallback is forbidden.

2. Confidence contract
- Confidence thresholds are versioned policy values, not ad-hoc constants.
- Threshold provenance (policy version + threshold key) must be attached to
  routing decisions.
- Confidence escalation from one layer to the next must be traceable.

3. Fail-closed contract
- Security, policy, and compatibility violations are fail-closed by default.
- Availability-only failures may use degraded-continue if and only if policy
  explicitly allows it for the target route.
- Final-layer compatibility rejection is always fail-closed.

## Consequences
Positive:
- Deterministic runtime behavior across environments.
- Better incident triage via explicit reason codes and policy provenance.
- Stronger compliance posture for package/model governance.

Trade-offs:
- Additional metadata in decision payloads.
- Slight implementation overhead in each handoff path.

## Implementation Notes
Required runtime fields in decision metadata:
- `fallback_mode`
- `fallback_reason_code`
- `confidence_policy_version`
- `confidence_threshold_key`
- `escalation_source_layer`

Target integration order:
1. TensorRAGPipeline decision envelope
2. ANN frontdoor routing diagnostics
3. Graph truth validation result metadata
4. Final-layer resolution diagnostics

## Validation
Minimum acceptance tests:
1. Fallback determinism
- Same input and policy produce the same fallback mode/reason.

2. Confidence provenance
- Decisions include policy version and threshold key for all confidence-based
  triggers.

3. Fail-closed enforcement
- Compatibility/policy violations terminate with fail-closed behavior and
  explicit reason code.

4. Cross-layer trace continuity
- Escalation metadata links ANN -> Tensor -> Graph -> Final Layer without gaps.

## Rollout
Phase 1:
- Add policy metadata fields and reason codes.
- Backward-compatible defaults preserving current behavior.

Phase 2:
- Enforce policy checks at all layer handoffs.
- Enable dashboards/alerts on fallback and fail-closed rates.
