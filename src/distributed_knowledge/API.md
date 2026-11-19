---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-19"
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
| `adapter_capability_announcement.h` — `GossipAdapterPublisher` | Serializes local announcements; inbound JSON is deserialized, requires an explicitly configured `IFederationTrustPolicy`, and dispatches only after that policy permits it. Unknown domain names are rejected rather than mapped to `GENERAL`. |
| `lora_federation_coordinator.h` — `LoRAFederationCoordinator` | Accepts current-round `EncryptedGradient` records and produces a `GlobalAdapterDelta` after participant, policy, and privacy checks. `EncryptedGradient::data` is an opaque JSON value; the type does not encrypt it. |
| `federated_rag_merger.h` — `FederatedRAGMerger` | Merges already-fetched `ShardRetrievalResult` values with RRF, score-weighted, or round-robin strategy; optionally deduplicates and truncates. |
| `cross_shard_feedback_sync.h` — `CrossShardFeedbackSync` | Publishes anonymized-origin feedback summaries through a supplied gossip callback and handles inbound JSON with dedup and optional policy checks. |
| `federated_distillation_coordinator.h` — `FederatedDistillationCoordinator` | Accepts teacher soft labels, enforces configured round/privacy/policy bounds, and dispatches a prepared round to registered student callbacks. |
| `dk_diagnostic_emitter.h` — `DistributedKnowledgeDiagnosticEmitter` | Emits structured events to registered listeners for timeout, dedup, trust, partial merge, and rollback diagnostics. |

## Integration requirements

1. Validate configuration before constructing the component; constructors may
   throw `std::invalid_argument`.
2. Configure an inbound capability trust policy with `setTrustPolicy()` before
   calling `handleInboundMessage()`. Missing policies and rejected announcements
   throw `std::runtime_error`; unknown `domain_type` values throw
   `std::invalid_argument`. An omitted domain remains `GENERAL`. The provided
   `AlwaysPermitTrustPolicy` is explicitly permissive and is not authentication
   or production authorization.
3. Set explicit participant limits, timeouts, policy callbacks, and privacy
   bounds appropriate to the deployment. Do not treat defaults as production
   approval; see [PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md).
4. Handle exceptions at the service boundary. In particular, RAG merge can
   throw when all supplied results time out; aggregation can reject insufficient
   participants or an exhausted privacy budget; distillation can reject policy
   or budget violations.
5. Do not infer success from a publish counter alone. Feedback dispatch is
   callback-based and gossip exceptions are counted as skipped publishes.
6. Treat JSON serialization as schema transport, not authentication,
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

## Consistency and runtime boundary

These are in-process components, not a graph synchronization or distributed
transport implementation. The module does not fetch remote shard results,
authenticate peers, persist rounds, or recover pending state after restart.
`FederatedRAGMerger` merges results already fetched by its caller; its timeout
configuration does not perform fan-out, enforce per-shard deadlines, or retry.
LoRA submissions for non-current rounds and duplicate shard IDs are silently
ignored. Feedback policy rejection is counted and dropped; feedback dedup state
is bounded and in-memory. These outcomes must be surfaced by the integrating
service and are not equivalent to durable delivery or convergence.

`IFederationTrustPolicy` is invoked for inbound capability announcements by
`GossipAdapterPublisher`; it does not authenticate the transport and is not
automatically applied to the other coordinator APIs. The host remains
responsible for authentication, authorization of aggregation inputs, durable
delivery, monitoring export, and restart/recovery.
