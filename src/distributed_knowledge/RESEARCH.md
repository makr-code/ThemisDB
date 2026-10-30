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

# Distributed Knowledge Research Notes

## Purpose and evidence boundary

This page records the engineering questions and evaluation evidence for
cross-shard knowledge coordination. It is for module developers and reviewers,
not a claim that a particular federation algorithm has won an empirical
comparison. The authoritative behavior is in the public headers and
implementations listed below; release posture and incomplete work remain in
[ROADMAP.md](./ROADMAP.md).

Primary evidence:

- [API contract](../../include/distributed_knowledge/distributed_knowledge_api_contract.h)
- [RAG merge API](../../include/distributed_knowledge/federated_rag_merger.h)
- [LoRA coordinator](../../include/distributed_knowledge/lora_federation_coordinator.h)
- [Feedback sync API](../../include/distributed_knowledge/cross_shard_feedback_sync.h)
- [Distillation coordinator](../../include/distributed_knowledge/federated_distillation_coordinator.h)
- [Focused test inventory](../../tests/distributed_knowledge/TEST_INVENTORY.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)

## Problem and assumptions

The module coordinates data already produced by shards: adapter capability
announcements, model-update contributions, ranked retrieval results, feedback
summaries, and soft labels. It does not own the shard query fan-out, model
training, transport, or durable storage. The merge path treats shard results
as potentially stale and eventually consistent; feedback gossip can arrive
late or out of order. Aggregation and distillation use explicit round
bookkeeping and privacy-budget checks.

Results depend on correct caller-supplied shard identity, ranking, sample
counts, policy callbacks, and timeouts. The source does not establish a
production dataset, a real-cluster evaluation, or comparative retrieval-quality
results. The focused suites primarily verify component contracts and edge
cases, not end-to-end model quality.

## Methods under evaluation

- **Retrieval fusion:** RRF combines ranks using `1 / (k + rank)`;
  score-weighted merge accumulates relevance scores with shard accuracy
  weighting; round-robin interleaves shard lists. Optional deduplication keeps
  the first ranked document ID before top-K truncation.
- **Adapter aggregation:** numeric fields are combined per key using
  sample-count-weighted averaging by default; the `median` configuration uses
  a per-key median. The coordinator applies its configured differential
  privacy noise before returning a delta.
- **Feedback synchronization:** outbound summaries anonymize `shard_origin`;
  inbound processing uses summary-ID deduplication and optional policy and
  zero-trust callbacks. Delivery through the gossip callback is best-effort.
- **Distillation:** a round submits teacher soft labels and, when configured,
  applies Gaussian noise before callbacks receive the round. Round and
  epsilon limits, bounded policy, and an optional policy callback can block
  broadcast.

These are component algorithms and contracts, not evidence that shard data is
cryptographically protected or that outputs meet a particular quality target.
See [MODELING.md](./MODELING.md) for formulas and [TRUST.md](./TRUST.md) for
security assumptions.

## Evaluation and reproducibility

Use the focused tests in `tests/distributed_knowledge/` to verify serialization,
policy rejection, duplicate handling, timeout behavior, and round state.
The [test inventory](../../tests/distributed_knowledge/TEST_INVENTORY.md)
identifies cases by surface. For performance, run the mapped cases in
`benchmarks/distributed_knowledge/` and compare against the release baseline;
the module performance contract specifies a regression ceiling of 10 percent,
p99 threshold comparison, and complete mapped-case manifests.

Record at least:

1. shard count, response order, failure/timeout pattern, and document/gradient
   cardinality;
2. selected strategy and all relevant policy, privacy, and timeout settings;
3. p50/p95/p99 latency, throughput, output cardinality, and partial-response
   rate;
4. test/benchmark revision, build profile, hardware, and baseline manifest.

No measured benchmark values or retrieval-quality dataset are asserted by this
page. Remaining timeout, policy, deduplication, and benchmark-depth work is
tracked in the [module roadmap](./ROADMAP.md) and
[future enhancements](./FUTURE_ENHANCEMENTS.md).
