> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# Security — distributed_knowledge Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The `distributed_knowledge` module propagates LoRA gradients, RAG retrieval results, and DBA feedback across shard boundaries. Its primary security concerns are: preventing raw training data or query text from leaving a shard, enforcing differential privacy guarantees, authenticating all inter-shard gossip messages, and protecting against privacy budget exhaustion.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Raw training data egress | Only numeric `EncryptedGradient::data` (weight-delta map) transmitted; no verbatim training text (`lora_federation_coordinator.h`) |
| Differential privacy violation | Gaussian mechanism σ = Δ·√(2·ln(1.25/δ))/ε applied before gradient distribution (`lora_federation_coordinator.cpp::applyDifferentialPrivacy()`) |
| Raw query text egress in feedback | `CrossShardFeedbackSync::publishFeedback()` enforces `shard_origin="ANON"`, transmits only `reason_embedding` — no raw query text (`cross_shard_feedback_sync.cpp`) |
| Unauthenticated gossip messages | `GossipProtocol::verifyMessage()` HMAC + mTLS per peer (external: `gossip_protocol`) |
| Privacy budget exhaustion | `DifferentialPrivacyManager::verifyPrivacyBudget()` called before each federation round; round skipped on exhaustion |
| Cross-border data transfer | Caller integrates `CrossBorderTransferPolicy::checkTransfer()` before triggering federation round (external: governance module) |
| Unverified audit log | `SphincsPlus`-signed audit record written after each round (external: `post_quantum_crypto`) |
| ZeroTrust bypass for inbound feedback | `ZeroTrustPolicyEnforcer::evaluateRequest()` called before `handleInboundSummary()` (`cross_shard_feedback_sync.cpp`) |
| NaN gradient leak | `LoRAFederationCoordinator::exportGradient()` throws `std::runtime_error` if NaN detected in gradient data |
| Invalid configuration | `FederationConfig`, `FederatedRAGMergerConfig`, `FeedbackSyncConfig` each validated via `isValid()` at construction; `std::invalid_argument` thrown on invalid config |

## Security Controls

### Zero Raw-Data Egress
- Layer B (`LoRAFederationCoordinator`): only numeric weight deltas in `EncryptedGradient::data` cross shard boundaries; no training text is serialised.
- Layer D (`CrossShardFeedbackSync`): `publishFeedback()` hardcodes `shard_origin = "ANON"` and transmits only `reason_embedding` (fixed-length float vector); raw feedback text is never sent.

### Differential Privacy
- (ε, δ)-Differential Privacy via Gaussian mechanism applied in `applyDifferentialPrivacy()` before distribution of aggregated gradients.
- Privacy budget is tracked by `DifferentialPrivacyManager`; rounds are skipped when budget is exhausted.
- Budget reset available via admin API endpoint `/admin/federation/reset-budget` (DK-7).

### Gossip Authentication
- All inbound gossip messages pass through `GossipProtocol::verifyMessage()` (HMAC + mTLS) before any processing by `GossipAdapterPublisher` or `CrossShardFeedbackSync`.

### Thread Safety
- All public APIs acquire an internal `std::mutex`; callers require no external synchronisation.

## Data Handling

- No raw training data, query content, or user PII is transmitted between shards.
- `reason_embedding` vectors are fixed-length numeric arrays; they do not contain decodable text.
- Post-quantum audit log (`SphincsPlus`) is written after each federation round; records contain round metadata only, not gradient values.

## Known Limitations

- Cross-border transfer enforcement relies on the external `CrossBorderTransferPolicy` module; the `distributed_knowledge` module provides the integration hook but does not implement the policy itself.
- GDPR subject rights (erase/export) are registered via `IGdprEraseTarget`; erasure affects future rounds only — already-aggregated gradients cannot be retracted.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| `gossip_protocol` (external) | Inter-shard gossip transport | HMAC + mTLS authentication required |
| `DifferentialPrivacyManager` (external) | DP noise application and budget tracking | Must be initialised before first federation round |
| `ZeroTrustPolicyEnforcer` (external) | Access control for inbound feedback | Evaluated per `handleInboundSummary()` call |
| `SphincsPlus` (external) | Post-quantum audit log signing | Must be available for audit record integrity |
