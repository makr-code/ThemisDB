---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Models and Algorithms

## Purpose and scope

For developers implementing or evaluating the module's aggregation, retrieval,
and distillation logic. This describes the algorithms visible in the
[public interfaces](../../include/distributed_knowledge/) and their
implementations; it does not prescribe a local-training objective or a
cluster-wide consistency protocol.

## Retrieval fusion

`FederatedRAGMerger` accepts an ordered vector of per-shard responses. It
doesn't contact shards or check their authorization. Failed responses
(`ok == false`) and timed-out responses are omitted from strategy processing.
The configured strategies are:

- **Reciprocal Rank Fusion (default):** add `boost / (rrf_constant + rank)`
  per document ID, where rank comes from `rank_in_shard` or the 1-based list
  position. The boost applies only when specialization boosting is enabled
  and the shard's accuracy delta is positive.
- **Score-weighted:** add each relevance score multiplied by a weight derived
  from the shard accuracy delta; the implementation clamps the weight to at
  least `0.01`.
- **Round-robin:** interleave each responding shard's list by rank position,
  with shard input order determining ties.

When enabled, deduplication happens after fusion and before top-K truncation.
It preserves the first occurrence and does not combine duplicate metadata.
Empty input produces an empty context; if configured timeout semantics deem
all supplied shards timed out, merge throws `std::runtime_error`. These
semantics do not impose freshness ordering or a version-vector check.

## LoRA update aggregation

`LoRAFederationCoordinator` accepts one gradient per shard for the current
round; stale/future round submissions and duplicate shard submissions are
ignored. It requires the configured minimum participant count. Numeric JSON
fields are aggregated independently by key:

- default/FedAvg path: arithmetic mean weighted by `sample_count` when
  `weight_by_sample_count` is true, otherwise an unweighted mean;
- `median`: median of values for each key;
- the source treats FedProx as the same aggregation operation as FedAvg;
  FedProx's local objective is outside this coordinator.

Configured differential-privacy noise is applied before the resulting delta
is produced, and the round/budget accounting advances on successful
aggregation. An optional outlier filter runs before aggregation and participant
count is checked again after filtering. Configuration names do not prove that
the submitted opaque JSON payload has been encrypted.

## Federated distillation

`FederatedDistillationCoordinator` holds a pending teacher ID and non-empty
soft-label batch. A broadcast checks round/privacy limits, bounded policy, and
the optional policy callback before dispatching. With `require_dp` enabled,
Gaussian noise is applied to probabilities, values are clamped to `[0, 1]`,
and each probability vector is normalized. The coordinator records epsilon
spent and broadcasts the same `DistillationRound` to registered student
callbacks. Callback failures and partial student delivery need to be handled
by the integrating service; this API does not provide a durable transaction.

The built-in Gaussian implementation uses the configured epsilon, delta, and
sensitivity. A custom noise generator can be injected. Privacy guarantees
depend on those values, the generator, data sensitivity assumptions, and
composition outside this process; they are not established solely by setting
`require_dp`.

## Model selection and evaluation

Selection should be based on workload evidence, not strategy names. Compare
ranking quality and duplicate behavior for retrieval; aggregation error and
participant skew for gradients; and downstream student utility against
privacy-budget consumption for distillation. Capture the benchmark and
reproducibility fields in [RESEARCH.md](./RESEARCH.md), and use
[PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md) for the module's
release-gate requirements.
