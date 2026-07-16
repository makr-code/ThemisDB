---
Datum: 2026-04-17
Status: closed
Primary (Quelle der Wahrheit): docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12.8, src/distributed_knowledge/ARCHITECTURE.md §5
Bezug / Reference: src/distributed_knowledge/ROADMAP.md Phase 10, docs/issues/MASTER_IMPLEMENTATION_PLAN.md
---

# DK-OR — Operational Resilience Hardening

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

**Module:** `distributed_knowledge`
**Target:** Session 11 (after DK-8 performance benchmarks)
**Depends on:** DK-1 (build), DK-6 (integration tests), DK-7 (admin API), DK-8 (benchmarks)
**Blocks:** v1.0 production release

---

## 1. Scope

Implement and validate all five Operational Resilience dimensions (Backpressure,
Timeout / Circuit Breaker, Errors / Warnings, Security, Hardening) for the
`distributed_knowledge` module as specified in `ARCHITECTURE.md §5`.

The OR controls are **additive** — they extend the existing four layers (A–D) with
standardised resilience patterns without modifying their core logic.

---

## 2. Background

The seven-class runtime influence taxonomy
(`docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md §12`) classifies every
system control into Switch, Fader, Optimizer, Agentic Solver, Closed Loop,
Open Loop, or Causal Chain. §12.8 then maps five cross-cutting OR dimensions
onto this taxonomy. This issue implements the concrete ThemisDB instances for
the `distributed_knowledge` module.

---

## 3. Acceptance Criteria

| # | Criterion | Test |
|---|---|---|
| AC-1 | `triggerAggregation()` respects `round_timeout_ms`; throws on timeout | DK-OR-B-1 |
| AC-2 | `publishFeedback()` is non-blocking; skips silently when Gossip queue full | DK-OR-B-2 |
| AC-3 | `FederatedRAGMerger::merge()` handles per-shard timeout via `shard_timeout_ms` | DK-OR-T-1 |
| AC-4 | `FederatedRAGMerger::merge()` throws when all shards time out | DK-OR-T-2 |
| AC-5 | `exportGradient()` throws `std::runtime_error` when any value in `data` is NaN | DK-OR-E-1 |
| AC-6 | `AIDecisionAuditor::recordDecision()` called for every federation round | DK-OR-E-2 |
| AC-7 | `handleInboundSummary()` rejects callers with ZeroTrust risk=HIGH | DK-OR-S-1 |
| AC-8 | `handleInboundSummary()` processes callers with ZeroTrust risk=LOW | DK-OR-S-2 |
| AC-9 | `LoRAFederationCoordinator` clears state on `IGdprEraseTarget::erase()` | DK-OR-H-1 |
| AC-10 | `FederatedRAGMerger` clears cached merge contexts on `erase()` | DK-OR-H-2 |
| AC-11 | `CrossShardFeedbackSync` clears dedup cache on `erase()` | DK-OR-H-3 |
| AC-12 | `ARCHITECTURE.md §5.5` hardening checklist: all 9 items green | Manual review |
| AC-13 | OR micro-benchmarks pass (`bench_distributed_knowledge_or.cpp`) | DK-OR-Bench |

---

## 4. Tasks

### 4.1 Backpressure

- [ ] Add `round_timeout_ms` (default: 30 000) to `FederationConfig` struct
  (Target: Session 11)
  - File: `include/distributed_knowledge/lora_federation_coordinator.h`
  - Validation: `isValid()` requires `round_timeout_ms >= 1000`
  - Tests: DK-OR-B-1 (timeout → `std::runtime_error`)

- [ ] Implement non-blocking `publishFeedback()` in `CrossShardFeedbackSync`
  (Target: Session 11)
  - If `gossip_message_fn_` queue is full (detected via return value / exception),
    increment `skipped_publish_count_` and return without throw
  - Expose `getSkippedPublishCount()` accessor
  - File: `src/distributed_knowledge/cross_shard_feedback_sync.cpp`
  - Tests: DK-OR-B-2

### 4.2 Timeout / Circuit Breaker

- [ ] Propagate `shard_timeout_ms` into `FederatedRAGMerger::merge()`
  (Target: Session 11)
  - Each `ShardRetrievalResult` awaited with `shard_timeout_ms` deadline
  - Timed-out shard: `ok = false`, not counted in `shards_responded`
  - All shards timed out: throw `std::runtime_error("all shards timed out")`
  - File: `src/distributed_knowledge/federated_rag_merger.cpp`
  - Tests: DK-OR-T-1 (one timeout → partial merge), DK-OR-T-2 (all → exception),
    DK-OR-T-3 (zero timeout → immediate exception)

- [ ] Add `triggerAggregation(size_t timeout_ms)` overload in
  `LoRAFederationCoordinator`; existing zero-arg overload calls with `round_timeout_ms`
  (Target: Session 11)
  - File: `include/distributed_knowledge/lora_federation_coordinator.h` +
    `src/distributed_knowledge/lora_federation_coordinator.cpp`

### 4.3 Error Signal Paths

- [ ] NaN guard in `exportGradient()`
  (Target: Session 11)
  - After delta computation: iterate `data` map, check `std::isnan(v)` for each value
  - On NaN found: throw `std::runtime_error("NaN detected in gradient data for round " + std::to_string(round))`
  - File: `src/distributed_knowledge/lora_federation_coordinator.cpp` (at integration point
    with `IncrementalLoRATrainer::exportGradient()`)
  - Tests: DK-OR-E-1

- [ ] `AIDecisionAuditor::recordDecision()` after each successful federation round
  (Target: Session 11)
  - Decision record fields:
    - `decision_type = "FEDERATED_ROUND"`
    - `round_id` (from `GlobalAdapterDelta::round`)
    - `epsilon_spent` (from `GlobalAdapterDelta::epsilon_spent`)
    - `participants` (from `GlobalAdapterDelta::participants`)
    - `outcome` = `"SUCCESS"` | `"SKIPPED_BUDGET"` | `"TIMEOUT"`
  - Injected via `LoRAFederationCoordinator::setDecisionAuditor(shared_ptr<AIDecisionAuditor>)`
  - Tests: DK-OR-E-2 (auditor called with correct `decision_type`)

### 4.4 Security Integration

- [ ] ZeroTrust check in `CrossShardFeedbackSync::handleInboundSummary()`
  (Target: Session 11)
  - Call `zero_trust_enforcer_->evaluateRequest(context)` before processing
  - If risk = HIGH: throw `std::runtime_error("inbound feedback rejected: high-risk context")`
  - Injected via `CrossShardFeedbackSync::setZeroTrustEnforcer(shared_ptr<ZeroTrustPolicyEnforcer>)`
  - Tests: DK-OR-S-1 (risk=HIGH → rejected), DK-OR-S-2 (risk=LOW → processed)

- [ ] Register all four components with `GdprSubjectRightsManager` via `IGdprEraseTarget`
  (Target: Session 11)

  **`LoRAFederationCoordinator::erase()`:**
  - Clears `pending_gradients_` map
  - Resets `current_round_` to 0
  - Increments `erase_count_` counter (for audit)

  **`FederatedRAGMerger::erase()`:**
  - Clears any cached last-merge context (if introduced in DK-4 for debugging)
  - Increments `erase_count_` counter

  **`CrossShardFeedbackSync::erase()`:**
  - Clears `seen_summary_ids_` dedup set
  - Increments `erase_count_` counter

  **`GossipAdapterPublisher::erase()`** (Layer A):
  - Clears buffered `AdapterCapabilityAnnouncement` payloads

  Interface: `include/governance/gdpr_subject_rights.h::IGdprEraseTarget`
  Registration: in constructor of each component, call
  `gdpr_manager->registerTarget(this)` if `gdpr_manager` is injected

  Tests: DK-OR-H-1 through DK-OR-H-3

### 4.5 Hardening Verification

- [ ] Verify `ARCHITECTURE.md §5.5` hardening checklist (9 items) — all items must
  have a corresponding passing test or CI gate before v1.0 merge (Target: Session 11)

- [ ] Create `benchmarks/bench_distributed_knowledge_or.cpp`
  (Target: Session 11)
  - `BM_TriggerAggregation_WithTimeout_N64`: P99 ≤ 500 ms
  - `BM_PublishFeedback_QueuePressure`: throughput ≥ 10 000 msg/s
  - `BM_HandleInboundSummary_WithZeroTrustCheck`: overhead ≤ 1 ms per call
  - `BM_FederatedRAGMerger_ShardTimeout`: merge with 1 timed-out shard ≤ 25 ms

---

## 5. Test Plan

New tests go into `tests/test_distributed_knowledge.cpp` (existing file, new section).
Integration assertions go into `tests/test_distributed_knowledge_integration.cpp`
(existing file, new Scenario 7: OR verification round).

| Test ID | Assertion | File |
|---|---|---|
| DK-OR-B-1 | `triggerAggregation()` with `round_timeout_ms=1` → throws `std::runtime_error` | test_distributed_knowledge.cpp |
| DK-OR-B-2 | `publishFeedback()` with full queue → no throw, `getSkippedPublishCount() == 1` | test_distributed_knowledge.cpp |
| DK-OR-T-1 | One of three shards times out → merge returns top-5 from two shards | test_distributed_knowledge.cpp |
| DK-OR-T-2 | All shards time out → throws `std::runtime_error("all shards timed out")` | test_distributed_knowledge.cpp |
| DK-OR-T-3 | `shard_timeout_ms=0` → immediate `std::runtime_error` | test_distributed_knowledge.cpp |
| DK-OR-E-1 | `exportGradient()` with NaN in delta → throws `std::runtime_error` | test_distributed_knowledge.cpp |
| DK-OR-E-2 | Successful round → `AIDecisionAuditor::recordDecision()` called once with `decision_type="FEDERATED_ROUND"` | test_distributed_knowledge.cpp |
| DK-OR-S-1 | `handleInboundSummary()` with ZeroTrust risk=HIGH → throws `std::runtime_error` | test_distributed_knowledge.cpp |
| DK-OR-S-2 | `handleInboundSummary()` with ZeroTrust risk=LOW → callback invoked | test_distributed_knowledge.cpp |
| DK-OR-H-1 | `LoRAFederationCoordinator::erase()` → `pending_gradients_` empty | test_distributed_knowledge.cpp |
| DK-OR-H-2 | `FederatedRAGMerger::erase()` → merge context cleared | test_distributed_knowledge.cpp |
| DK-OR-H-3 | `CrossShardFeedbackSync::erase()` → `seen_summary_ids_` empty | test_distributed_knowledge.cpp |
| DK-OR-Int-7 | Integration Scenario 7: full round with ZeroTrust + GDPR erase + AuditLog | test_distributed_knowledge_integration.cpp |

**Total new tests:** 13 unit + 1 integration = **14 tests**

---

## 6. Performance Targets (Binding)

| Benchmark | Target |
|---|---|
| `BM_TriggerAggregation_WithTimeout_N64` | P99 ≤ 500 ms |
| `BM_PublishFeedback_QueuePressure` | ≥ 10 000 msg/s |
| `BM_HandleInboundSummary_WithZeroTrustCheck` | ≤ 1 ms overhead per call |
| `BM_FederatedRAGMerger_ShardTimeout` | ≤ 25 ms with 1 timed-out shard |

---

## 7. Files to Create / Modify

| File | Action | Notes |
|---|---|---|
| `include/distributed_knowledge/lora_federation_coordinator.h` | Modify | Add `round_timeout_ms` to `FederationConfig`; add `triggerAggregation(timeout_ms)` overload; add `setDecisionAuditor()` |
| `include/distributed_knowledge/federated_rag_merger.h` | Modify | `merge()` respects `shard_timeout_ms`; add `erase()` → `IGdprEraseTarget` |
| `include/distributed_knowledge/cross_shard_feedback_sync.h` | Modify | Non-blocking `publishFeedback()`; `setZeroTrustEnforcer()`; `erase()` |
| `include/distributed_knowledge/adapter_capability_announcement.h` | Modify | Add `erase()` → `IGdprEraseTarget` |
| `src/distributed_knowledge/lora_federation_coordinator.cpp` | Modify | NaN guard; audit record; timeout overload |
| `src/distributed_knowledge/federated_rag_merger.cpp` | Modify | Per-shard timeout; erase |
| `src/distributed_knowledge/cross_shard_feedback_sync.cpp` | Modify | ZeroTrust check; non-blocking dispatch; erase |
| `src/distributed_knowledge/adapter_capability_announcement.cpp` | Modify | Erase implementation |
| `tests/test_distributed_knowledge.cpp` | Modify | 13 new OR test cases (DK-OR-B-1 … DK-OR-H-3) |
| `tests/test_distributed_knowledge_integration.cpp` | Modify | Scenario 7: full OR integration |
| `benchmarks/bench_distributed_knowledge_or.cpp` | **Create** | 4 OR micro-benchmarks |

---

## 8. Risk Register

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| ZeroTrust evaluator adds >1 ms per inbound message | Medium | Medium | Benchmark DK-OR-Bench target ≤ 1 ms; if exceeded, switch to async pre-auth |
| GDPR erase clears dedup cache → replay attack on cleared summaries | Low | Medium | Re-seed dedup with timestamp-based IDs after erase; document in ARCHITECTURE.md §5.4 |
| `AIDecisionAuditor` injection breaks existing DK-1…DK-8 tests | Low | High | All existing tests use mock injections; auditor injected as `nullptr` → no-op (null check in implementation) |
| Non-blocking publish silently drops critical feedback | Low | Low | `skipped_publish_count_` exposed via admin API; operator alert if count > threshold |

---

## 9. Definition of Done

- [x] All 13 unit tests + 1 integration test implemented (tests/test_distributed_knowledge_or.cpp, test_distributed_knowledge_integration.cpp)
- [x] All 5 OR benchmarks implemented (benchmarks/bench_distributed_knowledge_or.cpp)
- [x] `ARCHITECTURE.md §5.5` hardening checklist: all 9 items verified
- [x] No new regressions in DK-1 … DK-8 tests
- [x] `docs-lint` clean on this file
