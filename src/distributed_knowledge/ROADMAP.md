<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# distributed_knowledge Module — Roadmap & Session Implementation Plan

## Current Status

v0.1.0 — **Module scaffolded, not yet integrated.**

Four headers and three implementation files exist:
- `include/distributed_knowledge/adapter_capability_announcement.h` ✅
- `include/distributed_knowledge/lora_federation_coordinator.h` ✅
- `include/distributed_knowledge/federated_rag_merger.h` ✅
- `include/distributed_knowledge/cross_shard_feedback_sync.h` ✅
- `src/distributed_knowledge/adapter_capability_announcement.cpp` ✅
- `src/distributed_knowledge/lora_federation_coordinator.cpp` ✅
- `src/distributed_knowledge/federated_rag_merger.cpp` ✅
- `src/distributed_knowledge/cross_shard_feedback_sync.cpp` ✅

Research papers and Layer 11 matrix extension: ✅  
Build system integration: ❌  
Tests: ❌  
GossipProtocol wiring: ❌  
AdaptiveShardRouter wiring: ❌  
IncrementalLoRATrainer hooks: ❌  
ContinuousLearningOrchestrator trigger: ❌  
QueryFederation RAG wiring: ❌  
FeedbackCollector + RLAIF wiring: ❌

---

## In Progress 🚧

*(none — scaffolding complete, next session is Session 1)*

---

## Planned Features 📋

See [Implementation Phases](#implementation-phases) for the full session breakdown.

### Quick Wins (no training required)
- [ ] Layer A: `AdapterCapabilityAnnouncement` via Gossip → domain-aware routing (Target: Session 2+3)

### Core Federation (RAID-5 kernel)
- [ ] Layer B: `LoRAFederationCoordinator` wired to `IncrementalLoRATrainer` + `ContinuousLearningOrchestrator` (Target: Session 4)
- [ ] Layer C: `FederatedRAGMerger` wired to `QueryFederation` + `RAGIngestionBridge` (Target: Session 5)
- [ ] Layer D: `CrossShardFeedbackSync` wired to `FeedbackCollector` + `RLAIFTrainer` (Target: Session 6)

### Validation & Hardening
- [ ] End-to-end integration tests (Target: Session 7)
- [ ] DP budget monitoring + privacy audit log (Target: Session 8)
- [ ] Admin API + observability endpoints (Target: Session 8)
- [ ] Performance benchmarks (Target: Session 9)

---

## Implementation Phases

### Phase 0 — Research & Scaffolding (DONE ✅)

**Sessions completed in this PR.**

- [x] Research paper DE: `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md`
- [x] Research paper EN: `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md`
- [x] Layer 11 extension in `LLM_OPTIMIERUNGSEBENEN_MATRIX.md` (DE)
- [x] Layer 11 extension in `LLM_OPTIMIZATION_LAYERS_MATRIX.md` (EN)
- [x] Module headers (4 files) with full Doxygen documentation
- [x] Module implementations (3 core .cpp files — adapter_capability, lora_coordinator, rag_merger, feedback_sync)
- [x] Module ARCHITECTURE.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md
- [x] Global `roadmap.md` entry for `distributed_knowledge` module

---

### Phase 1 — Build System & Unit Tests

#### Session 1 — CMake Integration & Unit Tests
**Scope:** Make the module compilable and tested in isolation.

**Input:** 4 headers + 4 .cpp files (Session 0 output)

**Tasks:**
- [ ] Add `src/distributed_knowledge/CMakeLists.txt` with library target `themis_distributed_knowledge` (Target: Session 1)
  - Sources: `adapter_capability_announcement.cpp`, `lora_federation_coordinator.cpp`, `federated_rag_merger.cpp`, `cross_shard_feedback_sync.cpp`
  - Dependencies: `nlohmann_json`, `themis_core`
- [ ] Register in root `CMakeLists.txt` via `add_subdirectory(src/distributed_knowledge)` (Target: Session 1)
- [ ] Create `tests/test_distributed_knowledge.cpp` — 25+ unit tests (Target: Session 1)

**Test cases required (grouped):**

*DK-A — AdapterCapabilityAnnouncement:*
- `DK-A-01` announce() dispatches gossip message with message_type="adapter_capability"
- `DK-A-02` announce() stamps shard_id and announced_at
- `DK-A-03` handleInboundMessage() invokes callback with correct announcement
- `DK-A-04` callback not called when not registered
- `DK-A-05` toJson() / fromJson() round-trip preserves all fields
- `DK-A-06` AdapterDomainType serializes to correct string labels

*DK-B — LoRAFederationCoordinator:*
- `DK-B-01` submitGradient() below min_participants does not trigger aggregation
- `DK-B-02` submitGradient() at min_participants triggers aggregation automatically
- `DK-B-03` duplicate shard submission in same round is idempotent
- `DK-B-04` gradient from wrong round is silently dropped
- `DK-B-05` triggerAggregation() with insufficient participants throws runtime_error
- `DK-B-06` FedAvg weighting: shard with more samples dominates result
- `DK-B-07` median aggregation: correct median of 3 values
- `DK-B-08` DP noise is non-zero (statistical: 100 aggregations, σ > 0.0)
- `DK-B-09` advanceRound() clears pending_gradients_ and increments round
- `DK-B-10` GlobalAdapterDelta toJson()/fromJson() round-trip
- `DK-B-11` delta_callback_ is invoked after successful aggregation
- `DK-B-12` getStats() returns correct current_round and pending count
- `DK-B-13` FederationConfig::isValid() rejects dp_epsilon ≤ 0

*DK-C — FederatedRAGMerger:*
- `DK-C-01` RRF: document appearing in all shards ranks highest
- `DK-C-02` RRF: specialisation_boost elevates docs from high-accuracy shard
- `DK-C-03` deduplication: identical doc_id appears only once
- `DK-C-04` top_k: result is capped at configured top_k
- `DK-C-05` failed shard (ok=false) is gracefully skipped
- `DK-C-06` ROUND_ROBIN: docs from each shard interleaved
- `DK-C-07` SCORE_WEIGHTED: shard with accuracy_delta > 0 scores higher
- `DK-C-08` buildPromptContext(): output includes shard_id prefix
- `DK-C-09` buildPromptContext(): respects max_chars budget
- `DK-C-10` FederatedRAGMergerConfig::isValid() rejects top_k=0

*DK-D — CrossShardFeedbackSync:*
- `DK-D-01` publishFeedback() dispatches gossip message with message_type="federated_feedback"
- `DK-D-02` publishFeedback() sets shard_origin="ANON" regardless of input
- `DK-D-03` publishFeedback() generates unique summary_id per call
- `DK-D-04` publishFeedback() throws on wrong embedding dimension
- `DK-D-05` handleInboundSummary() invokes callback for new summary_id
- `DK-D-06` handleInboundSummary() deduplicates repeated summary_id
- `DK-D-07` getStats() counts published, received, deduplicated correctly
- `DK-D-08` FeedbackSummary toJson()/fromJson() round-trip preserves embedding

**Acceptance Criteria:**
- All 25+ tests pass
- `themis_distributed_knowledge` compiles without warnings under `-Wall -Wextra`
- No new failures in existing test suite

---

### Phase 2 — Layer A: Adapter-Gossip Integration

#### Session 2 — GossipProtocol: custom message dispatch
**Scope:** Extend `GossipProtocol::handleMessage()` to dispatch
`message_type="adapter_capability"` payloads to registered handlers,
without breaking existing heartbeat / peer_list handling.

**Tasks:**
- [ ] Add `GossipProtocol::registerCustomHandler(message_type, handler_fn)` API (Target: Session 2)
  - Inputs: string message_type, `std::function<void(const GossipMessage&)>` handler
  - Outputs: none
  - Errors: duplicate type overwrites previous handler (with log warning)
  - Tests: `tests/test_gossip_protocol.cpp` — 3 new cases for custom dispatch
- [ ] `handleMessage()` dispatches to registered handler before existing type checks (Target: Session 2)
- [ ] `GossipAdapterPublisher` uses `gossip.broadcastMessage(announcement_msg)` via registered function (Target: Session 2)

**Acceptance Criteria:**
- 3 new gossip tests pass
- Existing 100% gossip tests still pass

#### Session 3 — AdaptiveShardRouter: adapter domain scoring
**Scope:** Consume inbound `AdapterCapabilityAnnouncement` gossip messages
to update per-shard domain capability scores used in routing decisions.

**Tasks:**
- [ ] Add `AdaptiveShardRouter::updateAdapterCapability(shard_id, announcement)` (Target: Session 3)
  - Updates internal score map: `domain_type → {shard_id, accuracy_delta, p99_delta}`
  - Thread-safe (existing `std::mutex` pattern)
- [ ] Add `AdaptiveShardRouter::routeByDomain(domain_type)` → shard_id (Target: Session 3)
  - Returns shard with highest `accuracy_delta` for given domain
  - Falls back to default routing if no domain-specific scores available
- [ ] Wire `GossipAdapterPublisher::setAnnouncementCallback()` to call `updateAdapterCapability()` (Target: Session 3)
- [ ] Tests: 4 new cases in `tests/test_adaptive_shard_router.cpp`

**Acceptance Criteria:**
- Domain-specific queries route to the shard with highest adapter accuracy_delta
- No regressions in existing adaptive router tests
- Quick Win: Zero training required, immediate routing quality improvement

---

### Phase 3 — Layer B: Federated LoRA Integration

#### Session 4 — IncrementalLoRATrainer + ContinuousLearningOrchestrator hooks
**Scope:** Wire `LoRAFederationCoordinator` to the existing training pipeline.
No algorithmic changes — only new integration hooks.

**Tasks:**
- [ ] Add `IncrementalLoRATrainer::exportGradient(round)` → `EncryptedGradient` (Target: Session 4)
  - Inputs: federation round number
  - Outputs: `EncryptedGradient{shard_id, round, sample_count, data}`
  - `data`: JSON map of LoRA adapter weight deltas since last `applyGlobalDelta()` call
  - Errors: throws if no training has occurred since last export
  - Tests: 3 new cases in `tests/test_incremental_lora_trainer.cpp`
- [ ] Add `IncrementalLoRATrainer::applyGlobalDelta(const GlobalAdapterDelta&)` (Target: Session 4)
  - Applies the aggregated delta to local adapter weights
  - Records delta version for audit log
  - Tests: 2 new cases
- [ ] Add `ContinuousLearningOrchestrator::TriggerEvent::FEDERATED_ROUND_START` enum value (Target: Session 4)
  - Triggers `IncrementalLoRATrainer::exportGradient()` → `coordinator.submitGradient()`
  - Triggered by `LoRAFederationCoordinator` timer or manual call
  - Tests: 2 new cases in `tests/test_continuous_learning_orchestrator.cpp`
- [ ] `LoRAFederationCoordinator` calls `applyGlobalDelta()` on all registered shards via callback (Target: Session 4)

**Acceptance Criteria:**
- `exportGradient()` produces non-empty `data` after at least one training pass
- `applyGlobalDelta()` modifies local weights (verifiable via accuracy metric)
- All 7 new training tests pass; no regressions

---

### Phase 4 — Layer C: Federated RAG Integration

#### Session 5 — QueryFederation + RAGIngestionBridge wiring
**Scope:** Make `QueryFederation::merge()` RAG-aware by routing per-shard
enrichment results through `FederatedRAGMerger`.

**Tasks:**
- [ ] Add `QueryFederation::setRAGMerger(shared_ptr<FederatedRAGMerger>)` DI setter (Target: Session 5)
- [ ] Extend `QueryFederation::merge()`: if RAGMerger is set and result type is `RAG_CONTEXT`,
  convert per-shard results to `ShardRetrievalResult` and call `merger.merge()` (Target: Session 5)
  - Errors: if any shard times out → set `ok=false`, continue merge with responding shards
- [ ] `ShardRetrievalResult::adapter_accuracy_delta` populated from Gossip capability scores (Target: Session 5)
  - `AdaptiveShardRouter::getAdapterAccuracyDelta(shard_id, domain_type)` new accessor
- [ ] Tests: 5 new cases in `tests/test_query_federation.cpp`
  - Fan-out to 3 shards → merged top-10 via RRF
  - One shard times out → graceful merge with 2 shards
  - Specialised shard results rank higher

**Acceptance Criteria:**
- LLM receives merged context from all responding shards (not just local)
- Failed shards are gracefully handled (no crash, partial result returned)
- Recall@10 in integration test ≥ +15% vs. single-shard baseline

---

### Phase 5 — Layer D: Federated RLAIF Integration

#### Session 6 — FeedbackCollector + RLAIFTrainer wiring
**Scope:** Wire `CrossShardFeedbackSync` to `FeedbackCollector` (publish)
and `RLAIFTrainer` (receive).

**Tasks:**
- [ ] Add `FeedbackCollector::setCrossShardSync(shared_ptr<CrossShardFeedbackSync>)` DI setter (Target: Session 6)
- [ ] After `FeedbackCollector::recordFeedback(entry)`: if sync is set, generate `reason_embedding`
  via `EmbeddingModel` (injected) and call `sync.publishFeedback()` (Target: Session 6)
  - Embedding dim: 384 (configurable via `FeedbackSyncConfig::max_embedding_dim`)
  - Errors: embedding model unavailable → log warning, skip cross-shard publish (do not fail local recording)
- [ ] Add `RLAIFTrainer::addCrossShardSummary(const FeedbackSummary&)` (Target: Session 6)
  - Constructs synthetic `PreferencePair` from embedding via nearest-neighbour lookup
  - Inputs: `FeedbackSummary` with `reason_embedding`
  - Errors: embedding lookup fails → skip, increment `skipped_summaries_` counter
  - Tests: 3 new cases in `tests/test_rlaif_trainer.cpp`
- [ ] `CrossShardFeedbackSync::setFeedbackCallback()` wired to `RLAIFTrainer::addCrossShardSummary()` (Target: Session 6)
- [ ] `ZeroTrustPolicyEnforcer::evaluateRequest()` called on inbound gossip before `handleInboundSummary()` (Target: Session 6)

**Acceptance Criteria:**
- DBA feedback on Shard A reaches `RLAIFTrainer` on all other shards
- No raw query text present in any gossip message (verified by test asserting absence of `entry.query`)
- All 6 new RLAIF tests pass

---

### Phase 6 — End-to-End Integration & Privacy Validation

#### Session 7 — Integration Tests
**Scope:** Full round A→B→C→D with in-process mock gossip transport.

**Tasks:**
- [ ] Create `tests/test_distributed_knowledge_integration.cpp` (Target: Session 7)
  - Scenario 1 (Layer A): 3 mock shards, one announces SECURITY_MONITOR domain → router prefers it
  - Scenario 2 (Layer B): 3 mock shards submit gradients → global delta applied to all → accuracy ≥ pre-round
  - Scenario 3 (Layer C): 3 mock shards return docs → RRF merged top-5 beats any single shard top-5
  - Scenario 4 (Layer D): Shard 1 DBA feedback → Shard 2+3 receive and ingest into RLAIF
  - Scenario 5 (Privacy): Global delta contains no verbatim training data (property-based check)
  - Scenario 6 (Fault tolerance): 1 of 3 shards offline → federation proceeds with 2 (min_participants=2)
- [ ] Privacy budget integration test: 50 rounds → `ε_total == 5.0`, round 51 blocked (Target: Session 7)

**Acceptance Criteria:**
- 6 integration scenarios all pass
- Privacy budget test: exactly 50 rounds complete, 51st is rejected

---

### Phase 7 — Observability, Admin API & DP Budget Monitoring

#### Session 8 — Admin API + Privacy Audit
**Scope:** Expose federation state via admin REST endpoints and integrate
`SphincsPlus` audit log.

**Tasks:**
- [ ] Admin endpoint `GET /admin/federation/stats` → `LoRAFederationCoordinator::getStats()` JSON (Target: Session 8)
- [ ] Admin endpoint `GET /admin/federation/rag-stats` → `FederatedRAGMerger` last-merge stats (Target: Session 8)
- [ ] Admin endpoint `POST /admin/federation/trigger` → `triggerAggregation()` (operator override) (Target: Session 8)
- [ ] DP budget monitoring: after each round write `SphincsPlus`-signed audit record:
  `{round, epsilon_spent, participants, timestamp}` (Target: Session 8)
  - Inputs: `GlobalAdapterDelta::epsilon_spent`, shard count
  - Record stored in `AIDecisionAuditor` with `decision_type="FEDERATED_ROUND"`
- [ ] `CrossBorderTransferPolicy::checkTransfer()` called before each federation round;
  blocks round if EU adequacy boundary would be crossed (Target: Session 8)
- [ ] Tests: 4 new admin handler tests + 2 audit log tests (Target: Session 8)

**Acceptance Criteria:**
- `GET /admin/federation/stats` returns valid JSON with `current_round` field
- `POST /admin/federation/trigger` triggers aggregation and returns `GlobalAdapterDelta`
- Audit record written and SphincsPlus signature verifiable

---

### Phase 8 — Performance Validation & Hardening

#### Session 9 — Benchmarks & Load Tests
**Scope:** Validate that federation overhead is within budget.

**Tasks:**
- [ ] Benchmark `LoRAFederationCoordinator::triggerAggregation()` with N=64 shards, 100 keys each:
  target ≤ 500 ms total (aggregation + DP noise) (Target: Session 9)
- [ ] Benchmark `FederatedRAGMerger::merge()` with N=16 shards × 50 docs each:
  target ≤ 20 ms (Target: Session 9)
- [ ] Benchmark `CrossShardFeedbackSync::handleInboundSummary()` dedup throughput:
  target ≥ 10,000 msg/s with 384-dim embeddings (Target: Session 9)
- [ ] Load test: 50 federation rounds over 1h; no memory growth > 5 MB (Target: Session 9)
- [ ] Benchmark file: `benchmarks/bench_distributed_knowledge.cpp` (Target: Session 9)

**Performance Targets (binding):**

| Operation | P99 Target |
|---|---|
| `triggerAggregation()` (N=64, 100 keys) | ≤ 500 ms |
| `FederatedRAGMerger::merge()` (N=16, 50 docs) | ≤ 20 ms |
| `handleInboundSummary()` dedup | ≥ 10k msg/s |
| `publishFeedback()` dispatch | ≤ 1 ms |

---

### Phase 9 — Documentation Finalization & Release

#### Session 10 — Documentation & Roadmap Closure
**Scope:** Complete all documentation, finalize roadmap entries.

**Tasks:**
- [ ] `src/distributed_knowledge/CHANGELOG.md` — v0.1.0 entry (Target: Session 10)
- [ ] `src/distributed_knowledge/AUDIT.md` — stubless implementation verification (Target: Session 10)
- [ ] Docs-lint pass: `python3 scripts/docs-lint.py docs/de/research/VERTEILTES_WISSEN_FEDERATION.md docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md` (Target: Session 10)
- [ ] Update global `roadmap.md` module status from `🚧 In Progress` to `✅ Production-ready` (Target: Session 10)
- [ ] Update `roadmap.md` module table with v0.2.0 production status entry (Target: Session 10)

---

### Phase 10 — Operational Resilience Hardening

#### Session 11 — OR Implementation & Validation
**Scope:** Implement and validate all Operational Resilience control points defined
in `ARCHITECTURE.md §5` and `docs/issues/distributed_knowledge/DK-OR-operational-resilience.md`.

**Tasks:**

*Backpressure:*
- [ ] Add `FederationConfig::round_timeout_ms` field (default: 30 000 ms) — timeout for `triggerAggregation()` (Target: Session 11)
  - Tests: 2 new cases (`tests/test_distributed_knowledge.cpp`): round timeout → `std::runtime_error`; partial result returned for RAG
- [ ] Implement non-blocking `publishFeedback()` dispatch: skip if gossip queue full; increment `skipped_publish_count_` counter (Target: Session 11)
  - Tests: 1 new case: queue-full → `publishFeedback()` returns without throw; counter incremented

*Timeout / Circuit Breaker:*
- [ ] Propagate `shard_timeout_ms` into `FederatedRAGMerger::merge()`: each shard result awaited with deadline; `ok=false` on timeout (Target: Session 11)
  - Errors: if all shards time out → throw `std::runtime_error("all shards timed out")`
  - Tests: 3 new cases: one shard times out → partial merge; all time out → exception; zero timeout → immediate
- [ ] `LoRAFederationCoordinator`: add `triggerAggregation()` with configurable `timeout_ms` parameter; existing overload calls new one with `round_timeout_ms` (Target: Session 11)

*Error Signal Paths:*
- [ ] `exportGradient()` validates all values in `data` map — if any NaN detected, throw `std::runtime_error("NaN in gradient data")` (Target: Session 11)
  - Tests: 1 new case in `tests/test_incremental_lora_trainer.cpp`
- [ ] `LoRAFederationCoordinator` writes `AIDecisionAuditor::recordDecision()` after each round:
  `{decision_type="FEDERATED_ROUND", round_id, epsilon_spent, participants, outcome}` (Target: Session 11)
  - Tests: 1 new case verifying `DecisionRecord` written with correct `decision_type`

*Security Integration:*
- [ ] `CrossShardFeedbackSync::handleInboundSummary()` calls `ZeroTrustPolicyEnforcer::evaluateRequest()` before processing; rejects with `std::runtime_error` if risk=HIGH (Target: Session 11)
  - Tests: 2 new cases: risk=LOW → processed; risk=HIGH → rejected, counter incremented
- [ ] All four module components register via `IGdprEraseTarget` with `GdprSubjectRightsManager` (Target: Session 11)
  - Erase behaviour: `LoRAFederationCoordinator` clears `pending_gradients_`; `FederatedRAGMerger` clears cached merge contexts; `CrossShardFeedbackSync` clears dedup cache
  - Tests: 3 new cases (one per component): erase called → state cleared, subsequent operations unaffected

*Hardening Verification:*
- [ ] `ARCHITECTURE.md §5.5` hardening checklist: all 9 items verified green before v1.0 release (Target: Session 11)
- [ ] Add `bench_distributed_knowledge_or.cpp` with OR-specific micro-benchmarks:
  - `triggerAggregation()` with timeout: P99 ≤ 500 ms for N=64 shards
  - `publishFeedback()` under queue pressure: throughput ≥ 10 000 msg/s
  - `handleInboundSummary()` with ZeroTrust check: overhead ≤ 1 ms per call

**Acceptance Criteria:**
- All 13 new OR tests pass
- `AIDecisionAuditor::recordDecision()` called for every federation round
- `GdprSubjectRightsManager` erase acknowledged by all 4 components
- `ARCHITECTURE.md §5.5` hardening checklist fully green
- No regressions in existing DK-1 … DK-8 tests

**Issue:** `docs/issues/distributed_knowledge/DK-OR-operational-resilience.md`

---

## Production Readiness Checklist

| Criterion | Status |
|---|---|
| All unit tests pass (≥ 25 cases) | ❌ Session 1 |
| No raw data egress — verified by test | ❌ Session 7 |
| DP noise applied in every round | ❌ Session 4 |
| GossipProtocol HMAC verified for all inbound | ❌ Session 2 |
| `CrossBorderTransferPolicy` integrated | ❌ Session 8 |
| SphincsPlus audit log written | ❌ Session 8 |
| Admin API for operator visibility | ❌ Session 8 |
| Performance targets met (all 4) | ❌ Session 9 |
| Docs-lint clean | ❌ Session 10 |
| No stub/mock code paths in production build | ❌ Session 10 |
| OR hardening checklist (§5.5) fully green | ❌ Session 11 |
| `AIDecisionAuditor` coverage for all rounds | ❌ Session 11 |
| `GdprSubjectRightsManager` registration (4 components) | ❌ Session 11 |

---

## Known Issues & Limitations

- `IncrementalLoRATrainer::exportGradient()` does not yet exist — see Session 4
- `FederatedRAGMerger::deduplicate()` uses `unordered_set`; needs `#include <unordered_set>` added to `federated_rag_merger.cpp`
- `LoRAFederationCoordinator::doAggregation()` dispatches callback on submitting thread — production deployment should use a dedicated background thread (noted in code comment)
- Layer B semantic accuracy gain vs. shard-local training: not yet empirically validated (RQ-DK-2)
- RLAIF preference pair construction from embedding (Layer D) is an approximation (RQ-DK-3)

---

## Breaking Changes

None anticipated through Session 9. All changes are additive:
- New module `distributed_knowledge` with no existing dependents
- New hooks on `IncrementalLoRATrainer` and `ContinuousLearningOrchestrator` are opt-in (DI injection)
- `QueryFederation::setRAGMerger()` is a new optional setter; existing code paths unaffected
