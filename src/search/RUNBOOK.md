---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Search Module Runbook

This runbook captures how to diagnose and recover from the most common search-module production faults. It is grounded in the existing hybrid, distributed, reranking, and layered retrieval logic.

## Scenario 1 — BM25 or vector backend is empty or degraded

### Symptom

- search queries return near-empty result sets
- log output shows backend warnings or partial-result diagnostics
- `SearchStats::partial_result` or `primary_error_code` indicates degradation

### Diagnosis

1. Examine the query path in `HybridSearch::search()`.
2. Verify whether the lexical source, vector source, or both were unavailable.
3. Check whether the configured candidate limits are too small for the workload.

### Recovery

- increase `k_bm25` and/or `k_vector` within the configured maxima
- confirm the storage or ANN backend is healthy
- validate that the configured table and column still exist and are populated
- confirm there is no metric mismatch or empty vector-query path disturbing the dense path

## Scenario 2 — Distributed hybrid search times out or loses shards

### Symptom

- only a subset of shards return results
- many remote shard entries show `success == false`
- high latency or reduced recall across the distributed query path

### Diagnosis

1. Review `DistributedHybridSearch::search()` in `src/search/distributed_hybrid_search.cpp`.
2. Check the shard timeout and batch execution logic.
3. Inspect whether `resolver_` and `executor_` are healthy and returning usable shard metadata.

### Recovery

- increase `shard_timeout_ms` if the network or remote service is slow but healthy
- if `skip_failed_shards` is enabled, confirm that fallback behavior is acceptable for the caller
- verify that downstream shard services are not under load or experiencing transient failure
- compare partial results against the expected count and decide whether to degrade or fail the query at the API boundary

## Scenario 3 — LLM reranker is unavailable or malformed

### Symptom

- reranking is missing or unstable
- logs show backend errors or invalid score output
- the final ranking resembles the original fused order without semantic adjustment

### Diagnosis

1. Inspect the `LlmReranker::rerank()` logic.
2. Verify the backend callable is installed and non-null.
3. Validate that the prompt and parsed score output match the module's expected format.

### Recovery

- reattach the backend to the reranker
- disable reranking for a failing backend if the data path is not safe for LLM dependency
- check that `fallback_to_original` is set appropriately for the operational scenario
- treat malformed score output as a degraded but still valid result, not as a fatal error

## Scenario 4 — Layered retrieval is timing out or pruned

### Symptom

- `LayeredRetrievalResult::timed_out` or `guardrail_pruned` is set
- lower layers return fallback answers or empty candidate sets
- per-layer decisions show `TIMEOUT_SKIP` or `GUARDRAIL_SKIP`

### Diagnosis

1. Inspect `LayeredRetrievalOrchestrator` execution decisions.
2. Confirm that configured timeouts and guardrails are aligned with the actual workload.
3. Check whether the ANN, tensor, graph, or LLM layer is the bottleneck.

### Recovery

- lower `max_ann_candidates`, `max_prompt_chars`, or `max_layers` when the workload is too expensive
- increase the timeout budget only when the pipeline is known to be legitimate but slow
- prefer fallback output over complete failure if the query pipeline can still produce a useful answer

## Escalation guidance

Escalate to module owners or release owners when:

- the same query path shows repeated shard degradation across multiple shards
- the LLM reranker introduces persistent ranking drift or invalid outputs
- the layered retrieval pipeline consistently misses the latency budget
- result quality drops below the acceptable threshold for production traffic

## Related docs

- `src/search/OPERATIONS.md`
- `src/search/ARCHITECTURE.md`
- `src/search/API.md`
- `src/search/IMPLEMENTATION.md`
- `src/search/SECURITY.md`
