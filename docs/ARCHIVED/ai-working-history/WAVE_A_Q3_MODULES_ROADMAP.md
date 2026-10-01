# WAVE A Q3 Modules — Roadmap & Dependency Blocking Matrix

## Q3 2026 Module Wave (Sept 1 - Sept 30)

**Status**: Parallel execution of 6 HIGH-priority supporting modules
**Context**: While TRANSACTION/GPU/QUERY run Sept 2-30, Q3 WAVE modules begin Sept 7-15

### Modules in Q3 Wave

| # | Module | Owner | Status | Effort | Target Start | Target End |
|---|--------|-------|--------|--------|--------------|------------|
| 1 | SERVER | @server-owner | ⚠️ BLOCKED on sec audit | 12 days | Sept 7 | Sept 22 |
| 2 | STORAGE | @storage-owner | 🟢 READY | 10 days | Sept 7 | Sept 20 |
| 3 | INDEX | @index-owner | 🟢 READY | 8 days | Sept 15 | Sept 28 |
| 4 | SHARDING | @sharding-owner | 🟡 PARTIAL (sig closure) | 5 days | Sept 20 | Sept 30 |
| 5 | DISTRIBUTED_KNOWLEDGE | @distrib-owner | 🟡 PARTIAL (consensus) | 7 days | Sept 20 | Oct 5 |
| 6 | LLM | @llm-owner | 🟢 READY | 6 days | Sept 15 | Sept 25 |

---

## Dependency Blocking Matrix

```
        TRANSACTION GPU QUERY SERVER STORAGE INDEX SHARDING DISTRIB LLM
        ═══════════ ═══ ═════ ══════ ═══════ ═════ ════════ ═══════ ═══
TRANS   —
GPU     ─ —
QUERY   ─ ─ —
SERVER  ─ ─ ─ —
STORAGE ⚠️ ─ ─ ─ —
INDEX   ─ ─ ✅ ─ ─ —
SHARD   ✅ ─ ─ ─ ✅ ✅ —
DISTRIB ✅ ─ ─ ─ ✅ ✅ ✅ —
LLM     ─ ✅ ✅ ─ ─ ─ ─ ─ —

Legend:
  ─     = No dependency
  ✅    = HARD BLOCKER (must complete before starting)
  ⚠️    = SOFT BLOCKER (can run parallel, but needs careful coordination)
```

### Blocking Relationships (Critical Path)

**TRANSACTION → SHARDING**
- TRANSACTION must complete crash-recovery + SAGA validation before SHARDING quorum consensus testing
- **Blocker**: AC-6 crash-recovery (due Sept 9)
- **Impact**: SHARDING can start Sept 15 after TRANSACTION proof-of-work

**TRANSACTION → DISTRIBUTED_KNOWLEDGE**
- TRANSACTION timeout/retry semantics must be stable before DISTRIB consensus protocol testing
- **Blocker**: AC-5 timeout determinism (due Sept 8)
- **Impact**: DISTRIB can start Sept 20 after TRANSACTION timeout validation

**STORAGE → SHARDING**
- STORAGE AccessCoordinator must be wired before SHARDING multi-shard permission verification
- **Blocker**: STORAGE wiring (due Sept 20)
- **Impact**: SHARDING delaying to Sept 25+ if STORAGE not ready

**INDEX → DISTRIB_KNOWLEDGE**
- INDEX query engine must be deterministic before DISTRIB graph queries can be tested
- **Blocker**: INDEX performance gates (due Sept 28)
- **Impact**: DISTRIB phases 5-6 may slip into early Oct if INDEX delays

**QUERY → INDEX** ⚠️
- QUERY FTS executor blocks AQL v2.0 feature completeness for INDEX
- **Blocker**: QUERY Phase 1 implementation (Sept 15-22)
- **Soft**: INDEX can start Phase 1 (traditional indexes) without FTS

**GPU → LLM** ⚠️
- GPU CUDA wrapper adoption may impact LLM NVIDIA-specific kernel dispatch
- **Blocker**: GPU Phase C approval (170-call target, Sept 15)
- **Soft**: LLM can use CPU-fallback during GPU adoption if needed

---

## Q3 Module Detailed Roadmaps

### 1. SERVER Module (Security Audit Closure)

**Owner**: @server-owner
**Current Status**: Security audit in progress (3rd-party vendor)
**Target**: Complete ACL/RBAC verification + audit closure by Sept 22

**Gap 1: ACL Enforcement Validation**
- **AC**: Access control enforced for all endpoints (GET/POST/PUT/DELETE)
- **Tests**: 8 tests (6 unit + 2 integration)
- **Target**: Sept 20
- **Effort**: 5 days
- **Blockers**: None

**Gap 2: RBAC Audit Closure**
- **AC**: Role-based access control verified for all permission levels
- **Tests**: 5 tests (access matrix validation)
- **Target**: Sept 22
- **Effort**: 4 days
- **Blockers**: Third-party audit findings (ongoing)

**Gap 3: Security Audit Compliance**
- **AC**: All audit findings remediated + evidence provided
- **Tests**: N/A (manual verification)
- **Target**: Sept 25
- **Effort**: 3 days
- **Blockers**: Audit completion (target Sept 15)

**Weekly Targets**:
- Week 1 (Sept 7-14): Audit completion + ACL testing
- Week 2 (Sept 15-22): RBAC validation + evidence bundle prep
- Week 3 (Sept 23-30): Final sign-off

---

### 2. STORAGE Module (AccessCoordinator Wiring)

**Owner**: @storage-owner
**Current Status**: AccessCoordinator exists; not yet wired into query execution
**Target**: Complete wiring + permission enforcement by Sept 20

**Gap 1: AccessCoordinator Integration**
- **AC**: Coordinator wired into QueryExecutor, enforces permissions on all reads
- **Tests**: 5 tests (integration)
- **Target**: Sept 15
- **Effort**: 4 days
- **Blockers**: QUERY design approval (needed for integration point definition)

**Gap 2: Permission Enforcement Chain**
- **AC**: Write operations check permissions; deny on auth failure
- **Tests**: 3 tests (write + permission)
- **Target**: Sept 20
- **Effort**: 3 days
- **Blockers**: ACL implementation in SERVER (soft blocker; can use mock)

**Gap 3: Error Handling & Diagnostics**
- **AC**: Permission errors logged + metrics tracked
- **Tests**: 2 tests (error paths)
- **Target**: Sept 22
- **Effort**: 2 days
- **Blockers**: None

**Weekly Targets**:
- Week 1 (Sept 7-14): Design integration points
- Week 2 (Sept 15-22): Implement wiring + tests
- Week 3 (Sept 23-30): Hardening + sign-off

**Coordination**: Wait for QUERY design approval (Sept 10) before detailed integration design (Sept 11-14)

---

### 3. INDEX Module (Query Engine Determinism)

**Owner**: @index-owner
**Current Status**: Traditional indexes implemented; FTS blocked on QUERY executor
**Target**: Complete traditional query determinism gates by Sept 28

**Gap 1: Index Creation Determinism**
- **AC**: Same query → Same plan + execution path (run 100 times)
- **Tests**: 4 tests (single-index, multi-index, mixed scenarios)
- **Target**: Sept 20
- **Effort**: 3 days
- **Blockers**: None

**Gap 2: Range Query Optimization**
- **AC**: Range queries use appropriate index path; performance gate ≤100ms on 100K records
- **Tests**: 3 tests (range query correctness + perf)
- **Target**: Sept 25
- **Effort**: 3 days
- **Blockers**: None

**Gap 3: FTS Index Readiness** (Blocked on QUERY)
- **AC**: FTS index lookups work with QUERY executor backend (post-Sept 22)
- **Tests**: 1 integration test (FTS + executor)
- **Target**: Oct 5
- **Effort**: 2 days
- **Blockers**: QUERY Phase 1 implementation (Sept 15-22)

**Weekly Targets**:
- Week 1 (Sept 7-14): Design + traditional index validation
- Week 2 (Sept 15-22): Determinism testing + perf gates
- Week 3 (Sept 23-30): FTS integration (if QUERY ready) + sign-off
- Week 4 (Oct 1-5): Final FTS validation

---

### 4. SHARDING Module (Quorum Consensus Hardening)

**Owner**: @sharding-owner
**Current Status**: Multi-shard exact-path gate complete (Aug 17); consensus protocol in progress
**Target**: Complete quorum consensus + cascading failure validation by Sept 30

**Prerequisite**: TRANSACTION AC-6 crash-recovery proof (due Sept 9)
**Start Date**: Sept 20 (after TRANSACTION validated)

**Gap 1: Quorum Consensus Recovery**
- **AC**: Lost quorum → auto-rebalance without data loss (under chaos)
- **Tests**: 3 tests (quorum loss scenarios)
- **Target**: Sept 25
- **Effort**: 4 days
- **Blockers**: TRANSACTION crash-recovery tests CI GREEN (due Sept 10)

**Gap 2: Cascading Failure Resilience**
- **AC**: Coordinator crash + 2 followers offline → cluster survives + elects new leader
- **Tests**: 2 tests (cascading failure matrix)
- **Target**: Sept 30
- **Effort**: 3 days
- **Blockers**: STORAGE AccessCoordinator wiring (soft; can mock)

**Weekly Targets**:
- Week 2 (Sept 15-22): TRANSACTION validation + design review
- Week 3 (Sept 23-30): Consensus testing + chaos scenarios
- Week 4 (Oct 1-5): Sign-off + Wave B readiness

---

### 5. DISTRIBUTED_KNOWLEDGE Module (Consensus Protocol)

**Owner**: @distrib-owner
**Current Status**: Consensus protocol interface defined; validator integration pending
**Target**: Complete consensus validation + integration tests by Oct 5

**Prerequisite**: TRANSACTION AC-5 (timeout determinism, due Sept 8) + SHARDING consensus (due Sept 30)
**Start Date**: Sept 20 (can run parallel once TRANSACTION timeout validated)

**Gap 1: Consensus Validator Integration**
- **AC**: Consensus validators integrated into knowledge graph replication
- **Tests**: 3 tests (validator lifecycle)
- **Target**: Sept 30
- **Effort**: 5 days
- **Blockers**: SHARDING consensus proof (due Sept 30)

**Gap 2: Byzantine Fault Tolerance**
- **AC**: Up to f=floor(n/3) Byzantine nodes handled without consensus failure
- **Tests**: 2 tests (Byzantine scenarios)
- **Target**: Oct 5
- **Effort**: 3 days
- **Blockers**: None (runs in parallel)

**Gap 3: Consensus Timeout Semantics**
- **AC**: Consensus respects TRANSACTION timeout boundaries (no leader election overrun)
- **Tests**: 2 tests (timeout interaction)
- **Target**: Oct 5
- **Effort**: 2 days
- **Blockers**: TRANSACTION AC-5 proof (due Sept 8)

**Weekly Targets**:
- Week 2 (Sept 15-22): TRANSACTION timeout validation + design review
- Week 3 (Sept 23-30): Validator integration + SHARDING sync
- Week 4 (Oct 1-5): Byzantine testing + timeout validation
- Week 5 (Oct 6+): Final sign-off

---

### 6. LLM Module (GPU-Accelerated Inference)

**Owner**: @llm-owner
**Current Status**: Model loading working; GPU dispatch optimization pending
**Target**: Complete GPU wrapper adoption alignment + fallback testing by Sept 25

**Prerequisite**: GPU Phase C approval (wrapper adoption, 290→170 calls, due Sept 15)
**Start Date**: Sept 15 (can run parallel after GPU audit completion)

**Gap 1: GPU Kernel Dispatch Optimization**
- **AC**: LLM inference kernels use NVIDIA CUTLASS optimizations (where available)
- **Tests**: 4 tests (inference latency + correctness)
- **Target**: Sept 22
- **Effort**: 4 days
- **Blockers**: GPU wrapper adoption completion (due Sept 15)

**Gap 2: CPU-GPU Fallback Safety**
- **AC**: GPU unavailable → seamless CPU fallback (latency increase acceptable)
- **Tests**: 3 tests (fallback scenarios)
- **Target**: Sept 25
- **Effort**: 2 days
- **Blockers**: GPU Phase C fail-closed validation (due Sept 15)

**Gap 3: Determinism Validation**
- **AC**: GPU inference results ≡ CPU inference (numerical tolerance ≤1e-5)
- **Tests**: 2 tests (determinism across runs)
- **Target**: Sept 30
- **Effort**: 2 days
- **Blockers**: GPU determinism testing (hardware-dependent, due Sept 30)

**Weekly Targets**:
- Week 2 (Sept 15-22): GPU alignment + kernel optimization
- Week 3 (Sept 23-30): Fallback validation + determinism testing
- Week 4 (Oct 1+): Sign-off + Wave B readiness

---

## Execution & Tracking

### Weekly Synchronization (Mondays 09:00 UTC)

**Checkpoint Template** (same as IMMEDIATE modules):
```
## Module: {Name} | Week: Sept {X}-{Y}

### Completed
- [ ] Gap 1: {status}
- [ ] Gap 2: {status}

### Tests Status
- Total: N | Implemented: M | Passing: P | Flaky: F

### Blockers
- {Blocker}: {mitigation}

### Next Week
- {Target 1}
- {Target 2}
```

### GitHub Issues Auto-Generation

Use `tools/ci/generate_wave_a_issues.py` to create GitHub issues per module:

```bash
python3 tools/ci/generate_wave_a_issues.py \
  --modules SERVER,STORAGE,INDEX,SHARDING,DISTRIB_KNOWLEDGE,LLM \
  --milestone "Wave A Q3 2026" \
  --assign-to @module-owners
```

**Auto-Generated Issues** (per module):
- [ ] Gap 1 Implementation ({Owner}, {Effort} days)
- [ ] Gap 1 Tests ({Owner}, {Test count})
- [ ] Gap 1 Evidence Bundle ({Owner}, due {date})
- [... repeat for all gaps ...]
- [ ] Module Evidence Bundle Sign-Off ({Tech owner}, due {date})

---

## Risk Mitigation

### Soft Blockers (Can Run Parallel with Workarounds)

1. **STORAGE → SHARDING**: Use mock AccessCoordinator during Phase 1
2. **QUERY → INDEX**: INDEX starts without FTS; FTS integrated post-Phase 1
3. **GPU → LLM**: LLM uses CPU-only during GPU adoption; switch to GPU Phase 2

### Hard Blockers (Must Wait)

1. **TRANSACTION → SHARDING/DISTRIB**: Cannot start consensus work without crash-recovery proof
2. **SHARDING → DISTRIB**: Cannot validate Byzantine tolerance without quorum consensus working

### Escalation Path

If any module blocked beyond 2 days:
1. Notify @module-governance
2. Open GitHub issue (label: `wave-a-blocker`)
3. Escalate to @technical-steering-committee if not resolved within 24h

---

**Document Version**: 1.0
**Created**: 2026-09-02
**Next Update**: Sept 8 (first Q3 Wave checkpoint)
