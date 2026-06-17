# Lifecycle Promotion and Rollback Runbook (2026-06-17)

## Purpose
Operational runbook for package/model/adapter lifecycle transitions in the final
layer, including promotion and rollback with governance and compatibility gates.

It operationalizes:
- CCG-441 (promotion workflow)
- CCG-442 (rollback workflow)
- CCG-443 (operational runbook)

## Scope
Applies to:
- package status transitions
- compatibility-gated activation
- emergency rollback to known-good package state

## Roles and Ownership
- Release owner: approves promotion and rollback decisions
- Retrieval owner: validates compatibility and runtime behavior
- SRE/on-call: executes runbook in production windows
- Security/governance owner: validates policy compliance

## Promotion Workflow

### Allowed state machine
- `draft` -> `staging` -> `canary` -> `production`
- rollback transition allowed from `staging|canary|production` -> `previous_known_good`

Forbidden transitions:
- `draft` -> `production` (must pass staging and canary)
- any transition without compatibility approval

### Pre-checks (must pass)
1. Package integrity
- package metadata complete (`package_id`, `target_model_id`, `primary_adapter_id`).
- signatures/checksums valid.

2. Compatibility
- base model and version compatibility check returns accepted.
- no unresolved compatibility warnings.

3. Observability readiness
- correlation and routing telemetry present in staging.
- dashboards/alerts available for handoff SLOs.

4. Operational readiness
- rollback candidate identified (`previous_known_good`).
- on-call and release owner acknowledged rollout window.

### Execution steps
1. Transition `draft` -> `staging`
2. Run staging validation suite and compare against baseline
3. Transition `staging` -> `canary` with limited traffic
4. Monitor canary SLOs for defined soak window
5. If healthy, transition `canary` -> `production`
6. Record release evidence and close rollout

### Promotion success criteria
- no fail-closed compatibility violations
- fallback rate and latency within SLO limits
- no critical error-code spikes in final-layer resolution path

## Rollback Workflow

### Rollback triggers
- compatibility rejection spike beyond threshold
- handoff latency breach sustained beyond burn-rate policy
- elevated fail-closed rate with production impact
- correctness regression confirmed by integration probes

### Rollback pre-checks
1. Confirm target rollback package exists and is verified
2. Confirm rollback reason code and incident reference
3. Confirm observability stream is healthy for post-rollback validation

### Execution steps
1. Freeze new promotions
2. Transition active package to rollback target (`previous_known_good`)
3. Re-route traffic to rollback package
4. Invalidate stale caches if required by model/adapter switch semantics
5. Run post-rollback smoke and correctness probes
6. Declare rollback complete and open remediation work item

### Post-rollback validation
- final-layer compatibility rejection rate returns to baseline
- handoff latency and fallback rates recover to SLO
- key customer flows pass canary probes

## Abort Criteria
Abort promotion immediately if one of the following occurs:
- compatibility gate fails
- fail-closed reason codes indicate policy/security violation
- canary SLO breach persists beyond rollback threshold window

## Required Evidence Artifacts
For every promotion/rollback event store:
- policy version and gate evaluation result
- package ids (from/to)
- timestamps and operator identity
- SLO snapshots before and after
- incident reference (for rollback)

## Command/Procedure Placeholders
Exact CLI/API commands are environment-specific and must be standardized in the
release automation layer. Until then, use controlled manual transitions with
mandatory dual approval.

## Dry-Run Checklist (staging)
1. Execute full promotion path to canary equivalent
2. Trigger synthetic rollback scenario
3. Validate all evidence artifacts are generated
4. Validate ownership handoff and incident communication flow

## Review Cadence
- Monthly: runbook readiness drill
- Quarterly: policy threshold review
- Per release: promotion and rollback retrospective updates
