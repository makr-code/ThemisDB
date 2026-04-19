> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-17 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# distributed_knowledge Module — Future Enhancements

## Scope

Post-v1.0 improvements to the `distributed_knowledge` module (Layers 11A–D).
All items here are explicitly out of scope for DK-1…DK-8.

## Design Constraints for Future Enhancements

- Zero raw-data egress constraint is non-negotiable for all future items
- DP privacy budget management must be extended before any enhancement that increases round frequency
- All new gossip payload types must use the `registerCustomHandler()` API (DK-2) — no protocol-level changes

## Required Interfaces (Post-v1.0)

| Interface | Depends On | Purpose |
|---|---|---|
| `FederatedAIDecisionAuditor` | DK-7 + IMPL-B9 | Global timeline of all-shard decisions for DBA |
| `AdapterCapabilityAnnouncement` v2 | DK-2 + IMPL-B8 | Include `WorkloadFingerprint` in Gossip payload |
| `LoRAFederationCoordinator::RAID6Mode` | DK-3 | Hierarchical aggregation: Shard → Region → Global |

---

## Enhancement Areas

### A — RAID-6 Hierarchical Aggregation (Layer 11B+)

**Scope:** Two-level aggregation: regional coordinators aggregate shard gradients first,
global coordinator aggregates regional results.

**Benefit:** Reduces bandwidth at global coordinator from O(N·K) to O(R·K) where R = regions ≪ N.

**Design Constraints:**
- Regional coordinators are trusted nodes — no DP noise between region-internal shards
- DP noise applied once at regional → global boundary
- `ε_regional = ε_total / R` (privacy budget split)

**Required Interfaces:**
- `RegionalFederationCoordinator` wrapping `LoRAFederationCoordinator`
- `FederationTopology` config: `{regions: [{id, shard_ids}]}`
- `LoRAFederationCoordinator::setRegionalMode(topology)`

**Implementation Notes:**
- Reuse `FederatedAggregator::aggregateUpdates()` at each level
- Extend `GlobalAdapterDelta` with `region_id` field

**Test Strategy:** 9 shards in 3 regions: regional aggregation reduces noise vs. global direct

**Performance Target:** N=64 shards in R=8 regions: aggregation ≤ 200 ms (vs. 500 ms flat)

**Security / Reliability:** Regional coordinator failure → fallback to flat aggregation

---

### B — RAID-10 Specialised + Shared Adapters

**Scope:** Combine shard-specialised adapters (RAID-1 within domain) with globally
federated base adapter (RAID-5 base layer).

```
Global Base Adapter   ← FedAvg of all shards (Layer 11B today)
     +
Domain Specialisation ← Mirror within SECURITY_MONITOR group (RAID-1)
```

**Required Interfaces:**
- `AdapterRegistry::setDomainGroup(adapter_id, domain_type)` — group adapters by domain
- `LoRAFederationCoordinator::setAdapterCompositionStrategy(RAID10)`
- `CompositeAdapter` applying `base_delta + domain_delta`

**Implementation Notes:**
- Layer-11A Gossip must broadcast domain group membership
- `applyGlobalDelta()` must accept both base and domain delta

---

### C — Federated HNSW Index Parameter Sharing

**Scope:** Share `HNSWParameterTuner` results across shards via Gossip.
If Shard A found `ef_construction=200, M=16` to be optimal for `legal_contracts`,
Shard B starts with those parameters instead of default.

**Required Interfaces:**
- `HNSWParameterTuner::exportTuningResult()` → JSON payload for Gossip
- `GossipProtocol::registerCustomHandler("hnsw_params", ...)` — extends DK-2 mechanism
- `HNSWParameterTuner::applyExternalSuggestion(json)` — advisory, local validation required

**Test Strategy:** Shard A tunes to ef=200; Shard B starts with ef=200 → faster convergence

---

### D — Privacy Budget Reset Protocol

**Scope:** When the DP budget (ε_total) is exhausted after 50 rounds (default config),
provide a cryptographically auditable reset mechanism.

**Design Constraints:**
- Reset requires DBA approval (explicit acknowledgment, not automatic)
- Reset event signed with `SphincsPlus` and written to immutable audit log
- New budget starts fresh: `ε_spent = 0`, `round = 0`

**Required Interfaces:**
- `LoRAFederationCoordinator::requestBudgetReset(dba_approval_token)`
- `AIDecisionAuditor::recordBudgetReset(sphincs_signed_record)`
- Admin endpoint `POST /admin/federation/budget-reset`

---

### E — Streaming Federated RAG (Layer 11C+)

**Scope:** Stream partial merged RAG results to LLM as shards respond —
don't wait for all N shards before beginning generation.

**Design Constraints:**
- First result must reach LLM within 1 × (fastest shard latency)
- Merge strategy must support incremental RRF score updates
- Compatible with `StreamingRetriever` (RAG module)

**Required Interfaces:**
- `FederatedRAGMerger::streamMerge(shard_results_channel, callback)`
- `MergedRAGContext` as streaming type (not batch)

**Performance Target:** First token latency ≤ fastest shard p99 + 5 ms overhead

---

### F — Differential Privacy Accounting Improvements

**Scope:** Replace simple per-round ε accounting with Rényi Differential Privacy (RDP)
for tighter composition bounds.

**Research Reference:** Mironov (2017) — RDP allows more rounds for same total privacy loss.

**Benefit:** With RDP: 200+ rounds feasible for same ε_total=5.0 (vs. 50 rounds with
basic composition). Factor ~4× improvement.

**Implementation Notes:**
- `DifferentialPrivacyManager` extended with `computeRdpBudget(alpha, epsilon_per_round, n_rounds)`
- Existing DP infrastructure in `include/importers/federated_learning.h` as base

---

### G — Adaptive Operational Resilience (ML-Driven Control)

**Scope:** Replace static OR thresholds (fixed `circuit_breaker.failure_threshold`,
fixed `max_pending_requests`) with ML-learned adaptive thresholds that react to
observed system state and federation health.

**Design Constraints:**
- Adaptive controller runs as an Agentic Solver (class 4) layered on top of the
  existing static OR controls — it adjusts thresholds but never disables them
- Changes to `circuit_breaker.failure_threshold` via hot-reload only; no restart required
- Privacy invariant preserved: adaptive controller observes federation-round metadata
  only, never gradient content

**Required Interfaces:**
- `FederationHealthMonitor` — time-series aggregation of per-round success/latency/epsilon_spent
- `AdaptiveCircuitBreakerPolicy` — replaces static `failure_threshold` with ML prediction
- `BackpressurePredictor` — predicts `max_pending_requests` saturation 30 s ahead
- `FederationConfig::or_adaptive_enabled` (bool, default: false) — feature flag

**Implementation Notes:**
- `FederationHealthMonitor` subscribes to `LoRAFederationCoordinator` post-round events
- Sliding window (configurable, default: 100 rounds) feeds a lightweight EWMA model
- `AdaptiveCircuitBreakerPolicy::computeThreshold(metrics)` → new `failure_threshold`
  applied via `FederationConfig` hot-reload (SIGHUP or admin API)
- Fallback: if adaptive model confidence < 0.7, use static threshold from config

**Test Strategy:**
- Simulation test: 200 rounds with injected failure bursts → adaptive threshold converges
  to stable value within 20 rounds (vs. manual static setting)
- Regression test: adaptive mode disabled (`or_adaptive_enabled=false`) → identical
  behaviour to current static implementation

**Performance Targets:**
- `FederationHealthMonitor::recordRound()` overhead ≤ 0.1 ms
- `AdaptiveCircuitBreakerPolicy::computeThreshold()` ≤ 5 ms
- Memory: sliding window of 100 rounds ≤ 50 KB

**Security / Reliability:**
- Adaptive threshold changes logged as `AIDecisionAuditor::recordDecision()` entries
  with `decision_type = "OR_ADAPTIVE_THRESHOLD_CHANGE"`
- Min/max clamps on thresholds enforced in `FederationConfig::isValid()`

---

## Performance Targets (Post-v1.0)

| Enhancement | Target |
|---|---|
| RAID-6 hierarchical aggregation | ≤ 200 ms for N=64, R=8 |
| Streaming RAG first-result | ≤ fastest-shard-p99 + 5 ms |
| RDP accounting: rounds before budget | ≥ 200 (vs. 50 basic) |
| HNSW cross-shard cold-start improvement | ≥ 30 % fewer tuning iterations |

---

## Security / Reliability Notes

- RAID-6 regional coordinators introduce a new trust boundary — mTLS required between regional and global coordinator
- Budget reset protocol must be tested for replay attack resistance (nonce in approval token)
- Streaming RAG partial results must not reveal shard order (privacy side-channel)
