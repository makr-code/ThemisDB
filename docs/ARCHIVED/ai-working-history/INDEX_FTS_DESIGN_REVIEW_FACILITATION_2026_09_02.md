# Wave A: INDEX FTS Design Review Facilitation Plan
## Status: ✅ READY FOR KICKOFF (Sept 3-10, 2026)

---

## Executive Summary

**Design Review Gate:** INDEX AC-I2 — FTS Executor Backend Design Approval (Sept 10)
**Timeline:** Sept 3-10, 2026 (7 days)
**Critical Path:** Design approval unblocks Q3 module implementation (Sept 15-30)
**Outcome Required:** All 6 design sections approved + committee sign-off by Sept 10

---

## Current Status Assessment

### What's Ready (✅ PREPARED)

1. **Design Document** (`src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md`)
   - ✅ Algorithm section: BM25 formula + index lookup strategy (complete)
   - ✅ Data structures: On-disk binary format + in-memory LRU cache + Bloom filter (detailed)
   - ✅ API surface: FtsExecutor interface + error taxonomy (7200-7299) (frozen)
   - ✅ Performance targets: ≤50ms single-term, ≤100ms phrase (documented)
   - ✅ Test strategy: 25 tests across unit/integration/performance/determinism (drafted)
   - ✅ Risk assessment: Bloom FP, precision, corruption, lock contention (identified)

2. **Review Template** (`ai_working/QUERY_FTS_DESIGN_REVIEW_2026_09.md`)
   - ✅ 6 review sections with guiding questions (prepared)
   - ✅ Reviewer role assignments (owner → section mapping)
   - ✅ Decision checkboxes + approval signatures (templated)
   - ✅ Timeline: Sept 3-10 with phase-specific review dates (documented)

3. **Supporting References**
   - ✅ FTS extension contract (`src/query/FTS_EXTENSION_CONTRACT.md`)
   - ✅ AQL FTS roadmap (`src/index/AQL_FTS_ROADMAP.md`)
   - ✅ Existing test files (phase/proximity tests present)
   - ✅ Benchmark file (`benchmarks/rag/bench_fts_phase_b.cpp`)

### What Needs Facilitation (🔄 IN PROGRESS)

1. **Committee Coordination**
   - [ ] Identify & notify technical committee members
   - [ ] Assign reviewer roles (Performance, Thread Safety, API, QA, PM, Risk)
   - [ ] Confirm availability Sept 3-10
   - [ ] Distribute design document & review template

2. **Review Synchronization**
   - [ ] Sept 3 kickoff meeting (15 min): Overview + reviewer role mapping
   - [ ] Sept 3-5 async review phase: Sections 1-2 (Algorithm & API)
   - [ ] Sept 4-6 sync discussion (30 min): Clarifications on data structures + performance targets
   - [ ] Sept 5-7 async phase: Sections 3-5 (Performance, Tests, Phases)
   - [ ] Sept 6-7 sync discussion (30 min): Risk & mitigation strategies
   - [ ] Sept 8-9 revision cycle: Address ⚠️ NEEDS_REVISION feedback
   - [ ] Sept 10 final approval: Committee sign-off (15 min)

3. **Known Review Risks**
   - ⚠️ Performance targets may need validation (latency + throughput)
   - ⚠️ Thread safety model (shared_mutex strategy) needs careful review
   - ⚠️ Test count (25 tests) may need adjustment after QA review
   - ⚠️ Implementation effort estimate (30 days, 3 FTE) requires capacity confirmation

---

## Facilitation Roadmap (Sept 3-10)

### Phase 1: Pre-Review Preparation (Sept 2-3)

**Tasks:**
- [ ] Identify committee members from ROADMAP stakeholders
  - Performance Owner: @performance-owner (latency, throughput, benchmarking)
  - Thread Safety Owner: @thread-safety-owner (concurrency, mutex patterns)
  - API Contract Owner: @api-owner (public interface, error codes, stability)
  - QA/Test Owner: @qa-owner (test coverage, chaos scenarios, determinism)
  - Project Manager: @project-manager (effort estimation, capacity, risk)
  - Risk Manager: @risk-manager (unknowns, mitigations, operational impact)
  - Technical Steering Committee: @technical-steering-committee (final authority)

- [ ] Send pre-review notification email to committee
  - Subject: "FTS Executor Design Review Kickoff (Sept 3-10) — Role Assignment"
  - Attach: DESIGN_FTS_EXECUTOR_2026-09-10.md + QUERY_FTS_DESIGN_REVIEW_2026_09.md
  - Request: Confirm availability + read design before Sept 3, 09:00 UTC

- [ ] Create GitHub issue: `FTS Executor Design Review (Sept 3-10)` (label: design-review-fts)
  - Issue description: Links to design doc + review template
  - Comment thread: Async feedback collection
  - Milestone: Wave A Critical Path (Sept 10)

- [ ] Schedule review meetings
  - Sept 3, 09:00 UTC: Kickoff (15 min, synchronous)
  - Sept 4, 10:00 UTC: Mid-review sync (30 min, sections 1-4 clarifications)
  - Sept 7, 14:00 UTC: Risk discussion (30 min, sections 5-6)
  - Sept 10, 09:00 UTC: Final approval (15 min, committee sign-off)

**Deliverables:**
- Pre-review notification email (sent)
- GitHub design review issue (created, public)
- Calendar invites (sent to all reviewers)
- Design document ready for distribution

---

### Phase 2: Section-by-Section Review (Sept 3-9)

#### Section 1: Algorithm & Data Structures (Sept 3-5)
**Owner:** @performance-owner
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §2.1-2.2

**Review Questions:**
- Is BM25 formula correct? (k1=1.5, b=0.75, IDF calculation)
- Is on-disk index binary format efficient? (Posting list compression sound?)
- Is LRU cache strategy appropriate? (100 MB capacity for 100K docs)
- Is Bloom filter false-positive rate (1%) acceptable?
- Algorithm complexity O(n log k) acceptable for top-K retrieval?
- Index load time target (≤5s) achievable with proposed format?

**Timeline:**
- Sept 3: Design owner posts section to review template
- Sept 3-4: Async review by @performance-owner
- Sept 4, 10:00 UTC: Sync discussion (5 min allocated)
- Target Decision: ✅ APPROVED (or ⚠️ NEEDS_REVISION + actionable feedback)

---

#### Section 2: API Surface & Error Handling (Sept 3-5)
**Owner:** @api-owner
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §3-3.2

**Review Questions:**
- Is FtsExecutor interface minimal & complete?
- Is error taxonomy (7200-7299) sufficient?
- Are recovery strategies (rebuild on corruption, evict on OOM) sound?
- Is timeout handling (partial results + timeout flag) acceptable?
- Is public API stable for v2.0.0? (No breaking changes?)
- Are error messages actionable for operators?

**Timeline:**
- Sept 3: Design owner posts section
- Sept 3-4: Async review by @api-owner
- Sept 4, 10:00 UTC: Sync discussion (5 min)
- Target Decision: ✅ APPROVED

---

#### Section 3: Performance Targets & Benchmarking (Sept 4-6)
**Owner:** @performance-owner (continued)
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §4

**Review Questions:**
- Are ≤50ms single-term, ≤100ms phrase targets realistic?
- Is 1000ms timeout SLA appropriate for production?
- Is 100K document benchmark representative? Other sizes needed?
- Are Bloom filter + early termination techniques necessary?
- What about p95/p99 tail latency targets? (Missing?)
- What about throughput target (queries/sec)? (Missing?)
- Should benchmarks include 1M, 10M document sizes?

**Timeline:**
- Sept 4: Design owner posts section + benchmark rationale
- Sept 4-5: Async review + @performance-owner may request revisions
- Sept 4, 10:00 UTC: Sync discussion (7 min) — likely iteration here
- Target Decision: ✅ APPROVED or ⚠️ NEEDS_REVISION (add tail latency targets, throughput SLA, larger doc sizes)

---

#### Section 4: Test Strategy & Verification (Sept 5-6)
**Owner:** @qa-owner
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §5

**Review Questions:**
- Are 25 tests sufficient? (6 unit, 10 integration, 5 performance, 4 chaos)
- Do test cases cover all error paths? (Corruption, OOM, timeout, invalid queries)
- Are determinism tests adequate? (100 iterations sufficient? Suggest 1000+)
- Are chaos test scenarios realistic? (Network partition? Resource exhaustion?)
- Missing scenarios? (Cache coherence under concurrent updates?)
- Can tests run in parallel? (Thread-safety required?)
- Estimated test execution time on typical hardware?

**Timeline:**
- Sept 5: Design owner posts section
- Sept 5-6: Async review by @qa-owner
- May request additional test coverage (concurrent update scenarios, cache coherence)
- Target Decision: ✅ APPROVED or ⚠️ NEEDS_REVISION (increase determinism iterations, add concurrency tests)

---

#### Section 5: Implementation Phases & Effort (Sept 6-7)
**Owner:** @project-manager
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §6

**Review Questions:**
- Are 6 phases = 35 days reasonable for 3 FTE? (Sept 15 - Oct 30)
- Is phase sequencing correct? (Can Phase 1-2 run parallel?)
- Is effort allocation realistic? (5/6/10/10/10/2 day breakdown)
- Is Phase 3 (optimization) complexity justified?
- Can Phases run in parallel to accelerate timeline?
- Is 3 FTE capacity available Sept 15 - Oct 30?
- Is there risk buffer for design review findings?

**Timeline:**
- Sept 6: Design owner posts section + effort breakdown
- Sept 6-7: Async review by @project-manager
- Sept 7, 14:00 UTC: Sync discussion (5 min) on capacity
- Target Decision: ✅ APPROVED (with capacity confirmation)

---

#### Section 6: Risk & Unknowns (Sept 6-7)
**Owner:** @risk-manager
**Reference:** `DESIGN_FTS_EXECUTOR_2026-09-10.md` §8

**Review Questions:**
- Are key risks identified? (Bloom FP, precision loss, corruption, lock contention)
- Are mitigations concrete? (Use proven library, unit test, checksums, profiling)
- Are unknowns acknowledged? (Index corruption recovery strategy)
- Are gate criteria clear? (When does Phase 3 optimization gate pass?)
- Is probability/impact matrix realistic?
- Is "use Boost.Bloom library" dependency already in CMakeLists.txt?
- Is fallback strategy (rebuild from raw docs) operationally feasible?
- Are runbooks documented? (Corruption recovery procedure?)
- Is on-call playbook clear? (What to do when FTS query times out?)

**Timeline:**
- Sept 6: Design owner posts section
- Sept 6-7: Async review by @risk-manager
- Sept 7, 14:00 UTC: Sync discussion (10 min) — likely risk mitigation refinement
- Target Decision: ✅ APPROVED or ⚠️ NEEDS_REVISION (strengthen mitigations, add runbooks)

---

### Phase 3: Revision & Final Review (Sept 8-9)

**Tasks:**
- [ ] Collect ✅ APPROVED decisions from all 6 sections
- [ ] Identify any ⚠️ NEEDS_REVISION sections
- [ ] For each revision item:
  - Design owner updates `DESIGN_FTS_EXECUTOR_2026-09-10.md`
  - Post update to GitHub issue with tag @reviewer-who-requested
  - Reviewer confirms fix acceptable by Sept 9, 12:00 UTC

**Timeline:**
- Sept 8: Revision cycle begins
  - Design owner addresses all ⚠️ NEEDS_REVISION feedback
  - Updates posted to GitHub + email summaries to reviewers
  - Estimated updates: 2-3 hours for typical revisions

- Sept 8-9: Final review round
  - Reviewers validate revisions
  - Confirmation comments posted to GitHub issue
  - Target: All sections ✅ APPROVED by Sept 9, 18:00 UTC

---

### Phase 4: Final Approval & Gate Decision (Sept 10)

**Final Approval Meeting** (Sept 10, 09:00 UTC, 15 min)
**Attendees:** @query-module-owner, @performance-owner, @thread-safety-owner, @api-owner, @qa-owner, @project-manager, @risk-manager, @technical-steering-committee

**Agenda:**
1. (2 min) Review final status: All 6 sections approved? (Expected: YES)
2. (3 min) Address any last-minute clarifications
3. (5 min) Committee votes: APPROVED, CONDITIONAL, DEFERRED
4. (5 min) Assign implementation team & schedule Sept 15 kickoff

**Expected Outcome:**
```
APPROVAL STATUS: ✅ APPROVED (Ready for Sept 15 Implementation)

Signature Block (all sign):
- @query-module-owner _____ Date: Sept 10
- @performance-owner _____ Date: Sept 10
- @thread-safety-owner _____ Date: Sept 10
- @api-owner _____ Date: Sept 10
- @qa-owner _____ Date: Sept 10
- @project-manager _____ Date: Sept 10
- @risk-manager _____ Date: Sept 10
- @technical-steering-committee _____ Date: Sept 10
```

---

## Facilitation Tooling & Coordination

### Document Management
- **Primary Design Doc:** `src/query/DESIGN_FTS_EXECUTOR_2026-09-10.md` (read-only source of truth)
- **Review Template:** `ai_working/QUERY_FTS_DESIGN_REVIEW_2026_09.md` (feedback collection + sign-off)
- **GitHub Issue:** `FTS Executor Design Review (Sept 3-10)` (async discussion thread)
- **Notifications:** Email to committee + GitHub mentions for real-time updates

### Async Review Process
1. Design owner posts section reference to review template
2. Reviewers read design doc + add feedback to template
3. Questions posted as GitHub issue comments (tag with @reviewer-name)
4. Design owner responds within 24 hours
5. Feedback incorporated into template decision column

### Synchronous Checkpoints
- **Sept 3, 09:00 UTC:** Kickoff (overview + role assignment)
- **Sept 4, 10:00 UTC:** Mid-review sync (sections 1-4 clarifications)
- **Sept 7, 14:00 UTC:** Risk discussion (sections 5-6)
- **Sept 10, 09:00 UTC:** Final approval (committee sign-off)

**Meeting Format:** 30-45 min total, split by section
- Section focus: 5-10 min per section
- Q&A: Clarifications only, not re-review
- Action items: Tracked in GitHub issue

---

## Committee Role Mapping

| Role | Responsibilities | Design Section Focus | Decision Authority |
|---|---|---|---|
| @query-module-owner | Overall design coherence, schedule coordination | All sections (coordinator) | Can approve on behalf of MODULE |
| @performance-owner | Algorithm complexity, latency/throughput targets, benchmark methodology | Sections 1, 3 | Approve/reject performance gates |
| @thread-safety-owner | Concurrency model, mutex strategy, race conditions | Section 1 (locking), Section 2 (thread-safety) | Approve/reject threading model |
| @api-owner | Public interface stability, error taxonomy, contract preservation | Section 2 | Approve/reject API design |
| @qa-owner | Test coverage, chaos scenarios, determinism | Section 4 | Approve/reject test strategy |
| @project-manager | Effort estimation, capacity, risk scheduling | Section 5 | Approve/reject timeline + capacity |
| @risk-manager | Mitigations, operational impact, unknowns | Section 6 | Approve/reject risk posture |
| @technical-steering-committee | Final authority on gate approval | All sections (escalation) | Final sign-off if consensus blocked |

---

## Success Criteria (Sept 10, 18:00 UTC)

✅ **All the following must be true:**
- [ ] Section 1 (Algorithm): ✅ APPROVED
- [ ] Section 2 (API): ✅ APPROVED
- [ ] Section 3 (Performance): ✅ APPROVED
- [ ] Section 4 (Tests): ✅ APPROVED
- [ ] Section 5 (Phases): ✅ APPROVED
- [ ] Section 6 (Risk): ✅ APPROVED
- [ ] All reviewers signed off in approval signature block
- [ ] No unresolved ⚠️ NEEDS_REVISION items
- [ ] Design document finalized (no major changes needed)
- [ ] Implementation team capacity (3 FTE) confirmed for Sept 15 - Oct 30
- [ ] GitHub Issues created for Phase 1-6 implementation tasks
- [ ] Sept 15 kickoff meeting scheduled

---

## Known Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| Performance targets need validation | MEDIUM | MEDIUM | Add p95/p99 + throughput clarifications Sept 5 |
| Thread-safety model complex | MEDIUM | HIGH | Schedule extended sync on Sept 4, allocate 10 min |
| Test coverage gaps identified | MEDIUM | MEDIUM | @qa-owner requests additional scenarios Sept 6 |
| 3 FTE capacity unavailable Sept 15 | LOW | HIGH | Confirm Sept 7; if unavailable, defer implementation to Oct 1 |
| Bloom filter dependency missing | LOW | MEDIUM | Verify Boost.Bloom in CMakeLists.txt; add if needed |
| Design contradictions found | LOW | HIGH | Escalate to @technical-steering-committee for decision |

---

## Next Steps

### Today (Sept 2)
- [ ] Finalize committee roster (7 reviewers identified)
- [ ] Send pre-review notifications (design docs + role assignments)
- [ ] Create GitHub issue for async feedback
- [ ] Schedule 4 review meetings (Sept 3, 4, 7, 10)

### Sept 3-10
- [ ] Execute review process per phase roadmap above
- [ ] Track decisions in review template
- [ ] Collect signatures on final approval

### Sept 10, 18:00 UTC (Post-Approval)
- [ ] Mark ROADMAP.md AC-I2 as ✅ APPROVED (gates implementation)
- [ ] Create GitHub issues for Phase 1-6 implementation
- [ ] Assign team members & confirm capacity
- [ ] Schedule Sept 15 kickoff meeting

---

## Facilitator Checklist

### Pre-Review (Sept 2-3)
- [ ] Committee roster finalized (7 reviewers confirmed)
- [ ] Pre-review email sent with design docs + role assignments
- [ ] GitHub issue created (FTS Executor Design Review)
- [ ] Review template populated with section references
- [ ] All 4 review meetings scheduled + calendar invites sent
- [ ] Design document in final read-only state

### During Review (Sept 3-9)
- [ ] Sept 3: Kickoff executed, roles confirmed
- [ ] Sept 3-5: Sections 1-2 reviewed async + Sept 4 sync checkpoint
- [ ] Sept 4-6: Sections 3-4 reviewed async
- [ ] Sept 6-7: Sections 5-6 reviewed async + Sept 7 sync checkpoint
- [ ] Sept 8-9: Revisions addressed, all sections ✅ APPROVED
- [ ] Sept 9, 18:00 UTC: Final pre-approval checklist completed

### Post-Review (Sept 10)
- [ ] Final approval meeting executed (Sept 10, 09:00 UTC)
- [ ] All reviewers signed approval signature block
- [ ] ROADMAP.md AC-I2 marked ✅ APPROVED
- [ ] Implementation issues created (Phase 1-6)
- [ ] Team capacity confirmed Sept 15 - Oct 30
- [ ] Sept 15 kickoff scheduled + agendas sent

---

## Prepared By
- **Facilitator:** Copilot Coding Agent
- **Date:** Sept 2, 2026, 14:02 UTC
- **Session:** Wave A INDEX FTS Design Review Facilitation
- **Gate:** Critical path for Q3 module implementation (Sept 15 start)

**Status:** ✅ READY FOR KICKOFF (Sept 3, 2026, 09:00 UTC)

