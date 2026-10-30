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

# Distributed Knowledge API Reference

## Purpose and boundary

For developers integrating public C++ interfaces from
`include/distributed_knowledge/`. The interfaces coordinate supplied data and
callbacks; they do not themselves establish an authenticated network
connection, query remote shards, or persist federation state. The normative
frozen entity/federation contract and error taxonomy are in
[distributed_knowledge_api_contract.h](../../include/distributed_knowledge/distributed_knowledge_api_contract.h).

## Public surfaces

| Header and primary type | Contract summary |
|---|---|
| `adapter_capability_announcement.h` — `GossipAdapterPublisher` | Serializes capability announcements; sets local shard ID/time; accepts inbound JSON and calls an optional consumer. |
| `lora_federation_coordinator.h` — `LoRAFederationCoordinator` | Accepts current-round `EncryptedGradient` records and produces a `GlobalAdapterDelta` after participant, policy, and privacy checks. `EncryptedGradient::data` is an opaque JSON value; the type does not encrypt it. |
| `federated_rag_merger.h` — `FederatedRAGMerger` | Merges already-fetched `ShardRetrievalResult` values with RRF, score-weighted, or round-robin strategy; optionally deduplicates and truncates. |
| `cross_shard_feedback_sync.h` — `CrossShardFeedbackSync` | Publishes anonymized-origin feedback summaries through a supplied gossip callback and handles inbound JSON with dedup and optional policy checks. |
| `federated_distillation_coordinator.h` — `FederatedDistillationCoordinator` | Accepts teacher soft labels, enforces configured round/privacy/policy bounds, and dispatches a prepared round to registered student callbacks. |
| `dk_diagnostic_emitter.h` — `DistributedKnowledgeDiagnosticEmitter` | Emits structured events to registered listeners for timeout, dedup, trust, partial merge, and rollback diagnostics. |

## Integration requirements

1. Validate configuration before constructing the component; constructors may
   throw `std::invalid_argument`.
2. Set explicit participant limits, timeouts, policy callbacks, and privacy
   bounds appropriate to the deployment. Do not treat defaults as production
   approval; see [PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md).
3. Handle exceptions at the service boundary. In particular, RAG merge can
   throw when all supplied results time out; aggregation can reject insufficient
   participants or an exhausted privacy budget; distillation can reject policy
   or budget violations.
4. Do not infer success from a publish counter alone. Feedback dispatch is
   callback-based and gossip exceptions are counted as skipped publishes.
5. Treat JSON serialization as schema transport, not authentication,
   authorization, or encryption.

## Stable shared contract

`DKErrorCode` includes `FEDERATION_TIMEOUT`, `TRUST_GATE_REJECTED`, and
`RETRIEVAL_LIMIT_EXCEEDED` among its explicit failure values.
`isRetryableCode()` currently returns true only for `FEDERATION_TIMEOUT` and
`TOMBSTONE_PROPAGATION_FAILED`. The API contract defines entity-size,
peer-count, timeout, path-depth, and expected propagation constants; consumers
should use those declarations instead of copying limits.

Review the individual headers for method signatures, ownership, serialized
fields, and exact exception behavior before integration. This summary does not
replace Doxygen declarations. For implementation and test guidance, see
[IMPLEMENTATION.md](./IMPLEMENTATION.md) and
[MODULE_GUIDE.md](./MODULE_GUIDE.md).
