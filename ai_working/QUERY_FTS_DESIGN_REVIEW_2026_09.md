# QUERY FTS Executor — Design Review Template

**Date**: 2026-09-02 - 2026-09-10
**Status**: Design Phase (In Review)
**Owner**: @query-module-owner
**Reviewers**: @performance-owner, @thread-safety-owner, @technical-steering-committee

## Purpose

This document guides the technical committee through the FTS Executor design review. All sections must be completed before Sept 10, 2026 to unblock implementation (Sept 15+).

---

## 🔍 Review Sections

### 1. Algorithm & Data Structures Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §2.2 (BM25) + §2.1 (Index Layout)

**Review Questions**:
- [ ] BM25 formula is correct? (k1=1.5, b=0.75, IDF calculation accurate?)
- [ ] Index on-disk binary format is efficient? (Posting list compression strategy sound?)
- [ ] LRU cache strategy appropriate for typical query patterns?
- [ ] Bloom filter false-positive rate (1%) acceptable for pre-filtering?

**Reviewer Feedback**:
```
Performance Owner (@performance-owner):
- [ ] Algorithm complexity: O(n log k) acceptable for top-K?
- [ ] Memory footprint: 100 MB cache sufficient for 100K documents?
- [ ] Index load time target (≤5s) achievable with proposed format?

Thread Safety Owner (@thread-safety-owner):
- [ ] shared_mutex locking strategy correct?
- [ ] No deadlock risks with nested locks (index_lock → cache_lock)?
- [ ] Write path (updateIndex) exclusive lock justified?
```

**Decision** (Sept 3-5):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

### 2. API Surface & Error Handling Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §3 (API) + §3.2 (Error Handling)

**Review Questions**:
- [ ] FtsExecutor interface minimal & complete?
- [ ] Error taxonomy (7200-7299 range) sufficient for all failure modes?
- [ ] Recovery strategies (rebuild on corruption, evict on OOM) sound?
- [ ] Timeout handling (partial results + timeout flag) acceptable?

**Reviewer Feedback**:
```
API Contract Owner:
- [ ] Public API stable for v2.0.0? (No breaking changes expected?)
- [ ] Error codes non-overlapping with other modules?
- [ ] Error messages actionable for operators?

Security Owner:
- [ ] Index format resistant to tampering?
- [ ] Memory safety: bounds checking on all buffer access?
- [ ] Timeout handling resistant to DoS (no unbounded retries)?
```

**Decision** (Sept 5-6):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

### 3. Performance Targets & Benchmarking Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §4 (Performance Gates)

**Review Questions**:
- [ ] Performance targets realistic? (≤50ms single-term, ≤100ms phrase)
- [ ] Latency SLA appropriate for production? (1000ms timeout default)
- [ ] Benchmark methodology: 100K documents representative? Other size categories needed?
- [ ] Optimization techniques (Bloom filter, early termination) necessary for targets?

**Reviewer Feedback**:
```
Performance Owner (@performance-owner):
- [ ] Latency targets: ≤50ms achievable without GPU?
- [ ] Hardware assumptions: CPU cache, memory bandwidth?
- [ ] p95/p99 tail latency gates needed? (Not covered in current design)
- [ ] Throughput (queries/sec) target defined? Missing from design.

Benchmarking Owner:
- [ ] Benchmark dataset: 100K docs representative? (Suggest adding 1M, 10M sizes)
- [ ] Query pattern distribution: Mix of single-term, phrase, boolean? (Define test patterns)
- [ ] Hardware specs: CPU model, RAM, NVMe type? (Ensure reproducibility)
```

**Decision** (Sept 5-6):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

### 4. Test Strategy & Verification Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §5 (Test Strategy)

**Review Questions**:
- [ ] 25 tests sufficient? (Unit/integration/performance/determinism balance)
- [ ] Test cases cover all error paths? (Corruption, OOM, timeout, invalid queries)
- [ ] Determinism tests adequate for production guarantee?
- [ ] Chaos tests represent realistic failure scenarios?

**Reviewer Feedback**:
```
QA Owner:
- [ ] Unit test count (6): sufficient for BM25 scoring + compression?
- [ ] Integration test count (10): missing scenarios? (e.g., cache coherence under concurrent updates)
- [ ] Chaos test scenarios: network partition? Resource exhaustion? (Add if missing)
- [ ] Determinism tests: 100 iterations sufficient for "guarantee"? (Suggest 1000+)

CI/CD Owner:
- [ ] Test execution time: 25 tests on typical hardware? (Estimate in design)
- [ ] Parallel test execution possible? (Thread-safety required)
- [ ] CI integration points? (Performance gates, nightly benchmarks)
```

**Decision** (Sept 6-7):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

### 5. Implementation Phases & Effort Estimate Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §6 (Implementation Phases)

**Review Questions**:
- [ ] 6 phases = 35 days reasonable for 3 FTE? (Sept 15 - Oct 30)
- [ ] Phase sequencing correct? (Can Phase 2 run parallel to Phase 1?)
- [ ] Effort allocation per phase realistic? (5/6/10/10/10/2 days breakdown)
- [ ] Phase 3 (optimization) complexity justified? (10 days for cache+early termination)

**Reviewer Feedback**:
```
Project Manager:
- [ ] 3 FTE availability confirmed Sept 15 - Oct 30?
- [ ] Parallel execution possible? (Phase 1 + Phase 2 concurrent?)
- [ ] Risk buffer: schedule slack for design review findings?
- [ ] Dependency tracking: Phase 4 (threading) blocks Phase 5 (parallel)?

Tech Lead:
- [ ] Phase 1 (index loading): 5 days achievable? (File I/O + parsing likely)
- [ ] Phase 2 (BM25): 6 days adequate? (Scoring + merge logic non-trivial)
- [ ] Phase 3 (optimization): 10 days justifiable? (Bloom + cache + planning complex)
- [ ] Phase 4 (thread safety): 10 days enough? (Needs review + testing)
```

**Decision** (Sept 7):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

### 6. Risk & Unknowns Review

**Section Reference**: `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` §8 (Risk Assessment)

**Review Questions**:
- [ ] Key risks identified? (Bloom FP, precision, corruption, lock contention)
- [ ] Mitigations concrete? (Use proven library, unit test, checksums, profiling)
- [ ] Unknowns acknowledged? (Index corruption recovery strategy TBD?)
- [ ] Gate criteria clear? (Phase 3 optimization gate required before Phase C?)

**Reviewer Feedback**:
```
Risk Manager:
- [ ] Probability/Impact matrix: realistic? (Medium probability BF issue → High impact?)
- [ ] Mitigation effort: "use Boost.Bloom library" → is it already a dep? (Check CMakeLists.txt)
- [ ] Fallback strategies: rebuild from raw docs → recovery time SLA?
- [ ] Dependencies: no external libraries introduced that conflict with Wave B gates?

Ops/SRE Owner:
- [ ] Operational visibility: metrics available for all error paths?
- [ ] Runbooks: corruption recovery procedure documented?
- [ ] On-call playbook: what to do when FTS query times out?
```

**Decision** (Sept 7):
- ✅ APPROVED / ⚠️ NEEDS_REVISION / ❌ REJECTED
- Comments:

---

## 📋 Final Decision Matrix

| Section | Status | Owner | Target Decision Date |
|---------|--------|-------|----------------------|
| 1. Algorithm & Data Structures | [ ] | @performance-owner | Sept 5 |
| 2. API Surface & Error Handling | [ ] | @api-owner | Sept 6 |
| 3. Performance Targets | [ ] | @performance-owner | Sept 6 |
| 4. Test Strategy | [ ] | @qa-owner | Sept 7 |
| 5. Implementation Phases | [ ] | @project-manager | Sept 7 |
| 6. Risk & Unknowns | [ ] | @risk-manager | Sept 7 |

---

## ✅ Gate: Design Approval (Sept 10)

**All reviews must complete with ✅ APPROVED before Sept 10, 2026.**

If any section has ⚠️ NEEDS_REVISION:
1. Update `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` with feedback
2. Re-review affected section(s)
3. Re-decision by Sept 9

**Final Approval Signature** (Sept 10):
```
Technical Committee Approval:

[ ] @query-module-owner        Date: ________  Signature: _______________
[ ] @performance-owner         Date: ________  Signature: _______________
[ ] @thread-safety-owner       Date: ________  Signature: _______________
[ ] @technical-steering-committee  Date: ________  Signature: _______________

Approval Status: [ ] APPROVED (Ready for Sept 15 Implementation)
                  [ ] CONDITIONAL (Specific conditions documented below)
                  [ ] DEFERRED (Will revisit by: ___________)

Conditional Approval Notes (if applicable):
________________________________________________________________________

Post-Approval Action Items:
- [ ] Update ROADMAP.md with design approval date
- [ ] Create GitHub Issues for Phase 1-6 implementation tasks
- [ ] Assign team members to phases
- [ ] Schedule Sept 15 kickoff meeting
```

---

## 📌 Success Criteria (Post-Review)

✅ **All the following must be true by Sept 10:**
- [ ] All 6 review sections completed
- [ ] All 6 sections marked ✅ APPROVED
- [ ] Technical committee signed off
- [ ] No unresolved ⚠️ NEEDS_REVISION items
- [ ] Design document finalized (no major changes needed)
- [ ] Team capacity (3 FTE) confirmed for Sept 15 - Oct 30
- [ ] GitHub Issues created for Phase 1-6 implementation

---

## 📞 Review Coordination

**Design Review Meetings** (Sept 3-10):
- Sept 3: Kickoff + Sections 1-2 (Algorithm, API)
- Sept 4-5: Sections 3-4 (Performance, Tests)
- Sept 6-7: Sections 5-6 (Phases, Risk)
- Sept 8-9: Revisions + final review cycle
- Sept 10: Final approval + gate decision

**Asynchronous Review**:
- Reviewers add feedback to this document directly
- Questions raised via GitHub issues: label `design-review-fts`
- Design owner responds within 24 hours

**Escalation** (if consensus blocked):
- Escalate to @technical-steering-committee
- Final decision authority by Sept 9, 2pm UTC

---

**Document Version**: 1.0 (Initial)
**Created**: 2026-09-02
**Last Updated**: TBD (Review in progress)
**Status**: AWAITING REVIEW
