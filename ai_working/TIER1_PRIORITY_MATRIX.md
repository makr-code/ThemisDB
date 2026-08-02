# Tier 1 Gap Prioritization Matrix

**Purpose:** Categorize and assign ownership for 20,653 gap fixes across Phase 1-4 enhancement categories.  
**Date:** 2026-08-02 (baseline; updated incrementally as scanner results arrive)  
**Target:** Complete triage by 2026-08-15; begin implementation by 2026-08-20

---

## Gap Categories & Tier Assignment

### Release-Critical (Tier 1 — GA Blocker)
**Definition:** Gaps that block GA sign-off, affect release-critical paths, or introduce regressions.  
**Expected Count:** ~25% of total gaps = 85-153 gaps (from 340-610 total)  
**Acceptance Criteria:** Fixed + tested + regression-free on release_critical CI gate

| Category | Examples | Modules | Batch/Block |
|----------|----------|---------|------------|
| **Concurrency (C-1)** | Race conditions in request routing, idempotency cache, model loading, adapter hot-swap | Server, Sharding, LLM | Block 1, Block 2, Batch 2 |
| **Memory Safety (M-1/M-2)** | RAII violations, leak detection failures, cache cleanup deadlocks | LLM (primary), Storage | Batch 1 |
| **Security (S-1/S-2/S-3)** | Hardcoded secrets in model paths, weak crypto in embedding cache, injection in prompt guardrails | LLM (primary), Server, Query | Batch 3 |

### Performance/Hardening (Tier 2 — Quality Gate)
**Definition:** Gaps that improve reliability, reduce operational overhead, or harden under load.  
**Expected Count:** ~50% of total gaps = 170-305 gaps  
**Acceptance Criteria:** Fixed + documented + benchmark-backed verification (if applicable)

| Category | Examples | Modules | Batch/Block |
|----------|----------|---------|------------|
| **Error Handling** | Missing edge cases, incomplete failure envelopes, timeout contract violations | LLM, Query, Retrieval | Batch 3, Phase 3a |
| **Determinism/Ordering** | Lock ordering violations, non-deterministic load balancing, quorum consensus races | Sharding (primary), Server | Block 2 |
| **Observability** | Missing diagnostics, incomplete telemetry, audit logging gaps | Server, Sharding, Query | Blocks 1-2 |

### Documentation (Tier 3 — Reference)
**Definition:** Gaps that clarify API contracts, document removals, or archive evidence.  
**Expected Count:** ~25% of total gaps = 85-153 gaps  
**Acceptance Criteria:** Documented + Doxygen coverage maintained + ROADMAP.md updated

| Category | Examples | Modules | Batch/Block |
|----------|----------|---------|------------|
| **Contract Clarification** | Missing @param / @return documentation, API version notes, lifecycle contracts | All modules | Ongoing (Phase 6) |
| **Removal/Deprecation** | Dead-code stubs, mock paths, legacy compatibility shims | All modules | Phase 3a-6 |

---

## Batch/Block Assignments & Timeline

### Part 1: Week 1-2 (Aug 9-22)

#### Batch 1 — LLM Memory Safety (Aug 12-19)
**Gaps to Fix:** 40-50 M-1/M-2 gaps (RAII, leaks, caching)  
**Tier:** 🔴 Release-Critical + 🟠 Performance  
**Files:** `src/llm/` (~50-70 files identified by M-1/M-2 scanners)

| Gap ID | Type | File | Description | Severity | Tier | Status |
|--------|------|------|-------------|----------|------|--------|
| LLM-MEM-001 | Memory Leak | model_orchestrator.cpp | Model load cleanup path missing | HIGH | Tier 1 | ⏳ TBD |
| LLM-MEM-002 | RAII Violation | adapter_manager.cpp | Adapter unload manual cleanup instead of RAII | MED | Tier 1 | ⏳ TBD |
| LLM-MEM-003 | Cache Deadlock | embedding_cache.cpp | Cache eviction under lock contention | HIGH | Tier 1 | ⏳ TBD |
| LLM-MEM-004...LLM-MEM-050 | (40-47 more) | various | — | — | Tier 1-2 | ⏳ TBD |

**Tests:** MEM-01..MEM-16 in `tests/llm/test_llm_phase5_hardening.cpp`

---

### Part 2: Week 3-8 (Aug 20 → Oct 1)

#### Block 1 — Server Hardening: Concurrency & Auth (Aug 20 → Sep 5)
**Gaps to Fix:** 60-80 C-1 gaps (request routing, idempotency, auth middleware)  
**Tier:** 🔴 Release-Critical  
**Files:** `src/server/http_handler.cpp`, `src/server/request_router.cpp`, `src/server/auth_validator.cpp`

| Gap ID | Type | File | Description | Severity | Tier | Status |
|--------|------|------|-------------|----------|------|--------|
| SRV-CONC-001 | Race | request_router.cpp | Concurrent routing table updates race | HIGH | Tier 1 | ⏳ TBD |
| SRV-CONC-002 | Deadlock | idempotency_cache.cpp | Idempotency cache lock ordering violation | HIGH | Tier 1 | ⏳ TBD |
| SRV-CONC-003 | Race | auth_validator.cpp | Auth middleware state machine race on multi-protocol | MED | Tier 1 | ⏳ TBD |
| SRV-CONC-004...SRV-CONC-080 | (60-77 more) | various | — | — | Tier 1 | ⏳ TBD |

**Tests:** SRV-01..SRV-12 in `tests/server/test_server_phase5_hardening.cpp`

#### Block 2 — Sharding Hardening: Distributed Coordination (Sep 5 → Sep 12)
**Gaps to Fix:** 80-120 cross-shard concurrency gaps (340+ → 102 target = 70% reduction)  
**Tier:** 🔴 Release-Critical + 🟠 Performance  
**Files:** `src/sharding/coordinator.cpp`, `src/sharding/transaction_manager.cpp`, `src/sharding/wal_manager.cpp`

| Gap ID | Type | File | Description | Severity | Tier | Status |
|--------|------|------|-------------|----------|------|--------|
| SHARD-LOCK-001 | Lock Order | coordinator.cpp | Lock A→B→C ordering violated in failover path | HIGH | Tier 1 | ⏳ TBD |
| SHARD-LOCK-002...SHARD-LOCK-095 | (94 more) | various | Lock ordering (use TSAN lockset) | — | Tier 1 | ⏳ TBD |
| SHARD-COORD-001...SHARD-COORD-025 | (25 more) | various | Coordinator state machine races | HIGH | Tier 1 | ⏳ TBD |

**Tests:** TXC-01..TXC-32 + FLR-01..FLR-20 (extended), FI-01..FI-40 (chaos)

#### Batch 2 — LLM Hardening: Concurrency & Error Handling (Sep 12 → Sep 19)
**Gaps to Fix:** 50-70 C-1 gaps (model loading, adapter swaps, quotas, stream abort)  
**Tier:** 🟠 Performance  
**Files:** `src/llm/model_orchestrator.cpp`, `src/llm/adapter_manager.cpp`, `src/llm/token_quota_manager.cpp`

| Gap ID | Type | File | Description | Severity | Tier | Status |
|--------|------|------|-------------|----------|------|--------|
| LLM-CONC-001 | Race | model_orchestrator.cpp | Concurrent model load race (3+ threads × 1000 ops) | HIGH | Tier 2 | ⏳ TBD |
| LLM-CONC-002 | Race | adapter_manager.cpp | Adapter hot-swap race during inference | MED | Tier 2 | ⏳ TBD |
| LLM-CONC-003 | Unfairness | token_quota_manager.cpp | Token quota fairness under load (starvation) | MED | Tier 2 | ⏳ TBD |
| LLM-CONC-004...LLM-CONC-070 | (60-67 more) | various | — | — | Tier 2 | ⏳ TBD |

**Tests:** LLM-RC-01..LLM-RC-08, CBS-H-01..CBS-H-08, TQM-H-01..TQM-H-04

#### Batch 3 — LLM Hardening: Security & Error Handling (Sep 19 → Oct 1)
**Gaps to Fix:** 40-60 S-1/S-2/S-3 gaps (secrets, crypto, injection)  
**Tier:** 🔴 Release-Critical  
**Files:** `src/llm/model_config.cpp`, `src/llm/prompt_policy.cpp`, `src/llm/embedding_cache.cpp`

| Gap ID | Type | File | Description | Severity | Tier | Status |
|--------|------|------|-------------|----------|------|--------|
| LLM-SEC-001 | Secret | model_config.cpp | Hardcoded API key in model config path | HIGH | Tier 1 | ⏳ TBD |
| LLM-SEC-002 | Crypto | embedding_cache.cpp | Weak random in cache key generation | HIGH | Tier 1 | ⏳ TBD |
| LLM-SEC-003 | Injection | prompt_policy.cpp | Insufficient guardrail coverage for prompt injection | MED | Tier 1 | ⏳ TBD |
| LLM-SEC-004...LLM-SEC-060 | (40-57 more) | various | — | — | Tier 1 | ⏳ TBD |

**Tests:** PCL-H-01..PCL-H-06; error standardization to `themis::Expected<T>`

#### Batch 4 — LLM Hardening: Performance Gates & Sign-Off (Oct 1 → Oct 8)
**Gaps to Fix:** None (consolidation + sign-off)  
**Tier:** 🟡 Documentation  
**Files:** `src/llm/ROADMAP.md`, `benchmarks/llm/bench_llm_hotpaths.cpp`, `docs/sharding/SHARDING_P6_SIGN_OFF.md`

| Deliverable | Type | Description | Status |
|-------------|------|-------------|--------|
| LLM-01..LLM-08 benchmarks | Gates | Lock to performance baselines | ⏳ TBD |
| Phase 5-L01 + L02 sign-off | Docs | Exception safety + memory leak evidence bundle | ⏳ TBD |
| GA sign-off evidence | Docs | Link into `docs/governance/GA_PROMOTION_SIGN_OFF.md` | ⏳ TBD |

---

### Part 3: Week 9+ (Oct 15 → Nov 30)

#### Phase 3a — Remaining Module Hardening (Oct 8 → Nov 15)

| Module | Gaps | Priority | Files | Tier | Timeline |
|--------|------|----------|-------|------|----------|
| Query | 100-150 | 🟠 MED | `src/aql/`, `src/query/` | Tier 2-3 | Oct 8-22 |
| Storage | 80-120 | 🟠 MED | `src/storage/`, `src/persistence/` | Tier 1-2 | Oct 15-29 |
| Analytics | 60-100 | 🟠 MED | `src/analytics/` | Tier 2-3 | Oct 22 → Nov 5 |
| Index | 50-80 | 🟡 LOW | `src/index/` | Tier 2-3 | Oct 29 → Nov 12 |
| Retrieval | 40-60 | 🟡 LOW | `src/retrieval/`, `src/vector_search/` | Tier 2-3 | Nov 5-19 |
| Other (6 modules) | 150-200 | 🟡 LOW | RAG, Security, Content, Utils, Graph, Performance | Tier 2-3 | Nov 12-22 |

#### Phase 3b — Tier 1 Validation & Sign-Off (Nov 15-30)

| Activity | Timeline | Output |
|----------|----------|--------|
| Cumulative regression testing (release_critical suite) | Nov 15-20 | 100% pass on develop head |
| Gap closure sign-off (20,653+ fixes merged) | Nov 20-25 | Module ROADMAP.md Phase 5-6 marked [x] |
| GA promotion decision | Nov 25-30 | GO / DEFER with documented rationale |

---

## Priority Rules for Triage (Aug 12-15)

When classifying a gap from scanner output:

1. **Is it in a release-critical path?** (server auth, sharding coordinator, LLM inference)
   - YES → **Tier 1 (Release-Critical)** → Schedule in Block 1-4
   - NO → Continue to (2)

2. **Does it affect performance, reliability, or operational safety?**
   - YES → **Tier 2 (Performance/Hardening)** → Schedule in Phase 3a
   - NO → Continue to (3)

3. **Is it documentation, comments, or contract clarification only?**
   - YES → **Tier 3 (Reference)** → Phase 6 / ongoing
   - NO → **Escalate** (unclear — requires manual review)

---

## Success Metrics

### By Phase

| Phase | Metric | Target |
|-------|--------|--------|
| Part 1 (W1-2) | Tier 1 Release-Critical gaps identified & categorized | 100% |
| Part 1 (W1-2) | LLM Batch 1 (Memory Safety) tests delivered | MEM-01..MEM-16 pass 100% |
| Part 2 (W3-8) | Gap fixes merged (Server + Sharding + LLM) | 250-300 merged PRs |
| Part 2 (W3-8) | release_critical CI gate: 100% pass rate | 0 regressions |
| Part 3 (W9+) | Total gap reduction (Tier 1 target) | 20,653 fixes |
| Part 3 (W9+) | Module sign-offs (Phase 5-6 closure) | 15+ modules |
| Part 3 (W9+) | GA promotion decision | GO or DEFER + rationale |

---

## Appendix: Gap Distribution Assumptions

Based on prior Phase 1-4 scanner runs and repository size:

- **Total gap pool:** 82,611 gaps (per FINAL_GA_READINESS_CHECKLIST.md)
- **Tier 1 target:** 25% reduction = 20,653 fixes
- **Distribution by category:**
  - Concurrency (C-1): ~40% of Tier 1 (8,261 fixes)
  - Memory Safety (M-1/M-2): ~30% of Tier 1 (6,196 fixes)
  - Security (S-1/S-2/S-3): ~20% of Tier 1 (4,131 fixes)
  - Documentation/Other: ~10% of Tier 1 (2,065 fixes)

- **Distribution by module:**
  - Server: ~33% (6,800 fixes)
  - Sharding: ~33% (6,800 fixes)
  - LLM: ~33% (6,800 fixes)
  - Other 10 modules: ~1% (253 fixes)

---

**Last Updated:** 2026-08-02  
**Next Sync:** 2026-08-15 (after gap triage completion)
