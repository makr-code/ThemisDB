# Distributed Retrieval Execution Design (2026-06-17)

## Purpose
This design specifies how distributed retrieval executes across shards with
predictable latency, bounded cost, and deterministic merge behavior.

It operationalizes:
- CCG-421 (federated execution policy)
- CCG-422 (cost-aware shard pruning)

## Scope
Applies to federated retrieval in:
- ANN frontdoor shard routing
- Tensor mid-layer candidate refinement
- Cross-shard candidate merge and escalation decisions

## Design Goals
1. Deterministic execution under partial failures
2. Bounded fan-out and timeout behavior
3. Cost-aware shard participation with quality guardrails
4. Stable, explainable merge ordering

## Request Lifecycle
1. Build shard plan
- Compute candidate shard set from routing hints and shard summaries.
- Apply policy budgets (`max_shards`, `max_cost_budget`).

2. Execute fan-out
- Dispatch retrieval request to selected shards concurrently.
- Track per-shard timeout and retry budget.

3. Collect partial responses
- Accept successful shard responses until global timeout.
- Record failed/timeout shards with reason codes.

4. Merge and rank
- Merge candidates with deterministic tie-breakers.
- Apply quality floor and prune low-utility tails.

5. Escalate if needed
- If quality/confidence below threshold, escalate to next layer.

## Policy Contract

### Execution policy fields
- `max_shards` (hard cap)
- `fanout_timeout_ms`
- `per_shard_timeout_ms`
- `retry_budget_per_shard`
- `failover_enabled`
- `global_cost_budget`
- `min_quality_floor`

### Deterministic merge contract
Primary sort keys:
1. `candidate_score` (descending)
2. `shard_confidence` (descending)
3. `freshness_epoch` (descending)
4. `candidate_id` (ascending, deterministic tie-break)

### Failure-handling contract
- Shard timeout: mark as `SHARD_TIMEOUT`, continue if policy allows.
- Shard failure: retry up to budget; then mark `SHARD_FAILED`.
- Excessive shard loss beyond policy threshold: `fail_closed`.

## Cost-Aware Shard Pruning

### Shard utility score
For each shard, compute:
- `utility = alpha * expected_relevance + beta * freshness + gamma * locality`
- `cost = latency_estimate + io_cost + transfer_cost`
- `priority = utility / max(cost, epsilon)`

Selection strategy:
- Rank shards by `priority`.
- Select top shards until either:
  - `max_shards` reached, or
  - `global_cost_budget` exhausted.

Guardrails:
- Always include mandatory shards from routing constraints.
- Enforce minimum diversity across shard groups when configured.

## Telemetry Requirements
Each federated request MUST emit:
- `federated_plan_shard_count`
- `federated_selected_shard_count`
- `federated_cost_budget_used`
- `federated_timeout_shard_count`
- `federated_failed_shard_count`
- `federated_merge_deterministic=true|false`

Reason fields:
- `shard_prune_reason_code`
- `merge_tiebreak_reason_code`
- `escalation_reason_code`

## Validation Strategy

### Unit tests
1. Plan budgeting
- Verify `max_shards` and `global_cost_budget` caps are never exceeded.

2. Deterministic merge
- Same inputs produce identical ordered output across repeated runs.

3. Retry/failure policy
- Verify timeout/failure reasons and retry budget exhaustion behavior.

4. Cost-aware pruning
- Verify low-priority shards are pruned before high-priority shards.

### Integration tests
1. Partial shard failure scenario
- Ensure graceful degradation and deterministic merge output.

2. Full escalation scenario
- Ensure low-quality federated results escalate correctly.

3. High-load scenario
- Verify merge stability and budget compliance under concurrency.

## Rollout Plan
Phase 1:
- Introduce policy fields and deterministic merge implementation.
- Emit telemetry for planning and failures.

Phase 2:
- Enable cost-aware pruning with guardrails.
- Add benchmark-based quality/cost regression checks.

Phase 3:
- Enforce budget/SLO checks in release readiness gates.

## Ownership
- Retrieval architecture: execution policy and merge contract.
- Distributed runtime owners: fan-out, retry/failover, shard planners.
- Observability team: telemetry and alerting.
