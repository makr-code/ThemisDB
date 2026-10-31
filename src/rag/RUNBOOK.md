---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# RAG Incident Runbook

AI provenance note: Exact model backend not verifiable from final document text alone.

## Scope and safety

This runbook covers diagnosis of the RAG module's C++ components. It does not
replace server, storage, LLM-provider, or shard runbooks. The module's monitor,
refresh, and telemetry classes are not evidence that a production service has
wired those components into its request path. Confirm the deployment's
integration before taking an action.

During an incident, preserve request IDs, timestamps, model/index versions, and
sanitized error details. Avoid copying raw queries, retrieved documents, or
credentials into logs or issue comments. Do not bypass an authorization check,
quality gate, or injection control to restore throughput.

## Triage sequence

1. Identify the affected request path and scope: collection, shard/replica,
   model version, and whether the issue is retrieval, context, generation, or
   evaluation.
2. Check application-side request/error rates and retrieval/generation
   latencies. Compare against the same deployment's baseline, not a target
   written in a design specification.
3. Inspect the RAG component result and logs. Preserve `IndexResult`,
   `HybridFusionResult`, `AssembledContext`, quality-gate status, freshness
   state, and span stats when the caller exposes them.
4. Reproduce with the smallest relevant focused test before changing model,
   index, or thresholds.
5. Apply only a reversible, caller-supported mitigation. This module does not
   provide a general atomic index/model rollback API.

## Symptoms and response

### Missing or unexpectedly ranked results

- Verify that the upstream sparse and dense searches actually returned
  candidates and that their IDs refer to the same documents.
- `HybridRetriever::fuse()` combines only the lists passed to it.
  `retrieveWithVectorizer()` dense-scores only its BM25 input pool, so it cannot
  recover a result missing from that pool.
- For RRF, verify the input lists are in intended rank order. For linear
  fusion, verify score scale, normalization, weights, and `top_k`.
- Compare candidate IDs and scores on a fixed fixture, then run
  `tests/rag/test_rag_hybrid_retriever.cpp`. Do not adjust relevance thresholds
  to conceal an index or candidate-generation fault.

### Empty, truncated, or over-budget context

- Inspect the assembler's candidate count, system prompt/query sizes, model
  context limit, response reservation, `tokens_used`, and
  `tokens_remaining_for_response`.
- Empty input or no available context returns a valid empty context. With
  partial chunks enabled, the final chunk can be truncated and marked; callers
  must propagate `was_truncated` as appropriate.
- Confirm that the generation request uses the returned budget via
  `RAGContextAssembler::computeMaxTokens()`. Test the affected boundary in
  `tests/rag/test_rag_context_assembler.cpp` and
  `tests/rag/test_rag_budget_consistency_focused.cpp`.

### Ingestion reports success but documents are absent

- Inspect the returned `IndexResult`: `ok`, `doc_id`, `vector_count`, and
  `entity_count`. `ok == true` with `vector_count == 0` can mean a vector sink
  failed; that failure is logged but is not reflected as `ok == false`.
- Check the `RAGIngestionBridge::indexDocument` start/completion/warning logs
  for `fallback=true` and writer failures. Workflow fallback produces one
  canonical retrieval chunk, but that does not prove that the configured sink
  persisted it.
- Confirm the vector/graph sink is connected to the expected collection and
  that any query-time authorization filter admits the source.
- Re-index only after verifying the destination and write semantics.
  Identical text in a collection receives the same deterministic document ID,
  so a sink may replace prior records.
- Validate with `tests/rag/test_rag_ingestion_bridge_hardening_focused.cpp`
  and the sink's own integration/health checks.

### Staleness or refresh breach

- Read `IngestionLatencyMonitor::GetPercentiles()` and
  `GetShardStatuses()`; distinguish stale input timestamps from a stalled
  ingestion or failed index write.
- `FreshnessSLAEnforcer::TriggerEmergencyRefresh()` reports whether a refresh
  was scheduled, not whether it completed. The implementation limits emergency
  scheduling attempts; inspect the scheduler and backend for completion.
- Check registered fallback shard IDs and the caller's routing decision.
  `fallback_active` is state exposed by the enforcer, not proof that queries
  were routed to a replica.
- After recovery, verify new ingestion timestamps and retrieval results before
  clearing the incident. Do not treat the current enforcer's reported
  compliance percentage as a measured historical SLA; see
  [operations limitations](./OPERATIONS.md#implementation-limits-relevant-to-operations).

### Prompt-injection finding or suspicious generated answer

- Inspect the detector's severity, finding category, and matched offset in
  `InjectionScanResult`. The detector uses rules and heuristics; no finding is
  not proof that content is safe.
- Follow the application's configured response policy: block or escalate
  high-severity content and sanitize only through the intended
  `PromptInjectionSanitizer` path. Do not manually edit a finding away to
  release the answer.
- Quarantine or review the upstream source when a retrieved document contains
  malicious instructions. Check if the same source entered other collections
  or tenants.
- Run `tests/rag/test_rag_prompt_injection.cpp` for a detector/sanitizer
  regression. See [trust boundaries](./TRUST.md).

### Quality gate fails or escalates

- Preserve the `QualityCheckResult` status, failed stage, dimension scores,
  confidence, and failure reasons.
- `FAILED`, `RETRY_NEEDED`, and `ESCALATE` are distinct outcomes. Do not turn
  them into an unconditional pass. Verify the judge/verifier backend is
  available and distinguish infrastructure errors from actual low scores.
- Re-run the same query and evidence in the corresponding judge test before
  tuning thresholds. A unit-test score is not a production-quality baseline.

### Telemetry appears absent

- Inspect `OTELSpanEmitter::GetStats()` and whether the caller ended spans and
  flushed them. `RagQualityMonitor` emits Prometheus-formatted gauge lines to
  the application log, not to an HTTP scrape endpoint.
- The current `OTELSpanEmitter` implementation serializes and clears its local
  span buffer; it does not send an OTLP network request. `emitted_spans` is not
  evidence of collector receipt. Escalate collector delivery to the service
  integration owner; do not rely on this class alone for alerting.

## Rollback and escalation

Rollback is deployment-specific. Before reverting a model or index, capture
its version/hash and the active mapping, and confirm the application has a
known-good artifact and a safe switch procedure. If no verified rollback path
exists, stop promotion or writes through the owning deployment controls and
escalate to the RAG, search/storage, or LLM owner rather than inventing a
module-level rollback.

## References

- [Operations guide](./OPERATIONS.md)
- [Trust model](./TRUST.md)
- [API reference](./API.md)
- `include/rag/hybrid_retriever.h`, `include/rag/rag_context_assembler.h`,
  `include/rag/rag_ingestion_bridge.h`, `include/rag/freshness_sla_enforcer.h`
- `tests/rag/test_rag_hybrid_retriever.cpp`
- `tests/rag/test_rag_context_assembler.cpp`
- `tests/rag/test_rag_ingestion_bridge_hardening_focused.cpp`
- `tests/rag/test_rag_prompt_injection.cpp`
