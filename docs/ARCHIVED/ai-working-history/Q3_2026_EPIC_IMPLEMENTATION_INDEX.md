# Q3 2026 Epic Implementation Index

**Date:** 2026-07-05  
**Status:** 🟢 Ready for GitHub Issue Creation  
**Total Epics:** 5  
**Total Sub-Issues:** 24  
**Timeline:** W31-W49 2026  

---

## 📋 Overview

This document provides a comprehensive, verifiable implementation plan for ThemisDB Q3 2026 work, organized as concrete GitHub Epic issues with dedicated sub-issues. The plan is driven by the current project state analysis and existing roadmap documentation.

### Key Metrics

| Metric | Baseline | Target | Timeline |
|--------|----------|--------|----------|
| Phase 1-4 Gap Completion | 1.119/1.236 (90.5%) | 1.236/1.236 (100%) | v1.5.0 (2026-08-31) |
| Top-5 Module HIGH Gaps | 59.148 | 52.000 (-12%) | 2026-09-30 |
| Scanner Portfolio | 27 Categories | 32 Categories (+5) | 2026-10-31 |
| Distributed Consistency | 4 WAL Paths | 1 Unified Path | 2026-11-30 |

---

## 🎯 Epics Breakdown

### EPIC-001: Phase 1-4 Gap Remediation Sprint 8-9 (Target: 2026-08-31)

**Status:** Foundation for v1.5.0 Release  
**Owner:** AI Gap Remediation Team  
**Scope:** 117 Gaps (Move: 97, Concurrency: 20)

| Sub-Issue | Title | Effort | Acceptance Criteria |
|-----------|-------|--------|-------------------|
| 001-A | Move Semantics Hardening | 40h | 97/97 gaps, 100% tests |
| 001-B | Concurrency & Data Races | 25h | 20/20 gaps, ThreadSanitizer clean |
| 001-C | Integration & v1.5.0 Release | 15h | 1.236 tests PASS, no regressions |

**Key Dependencies:**
- Blocks EPIC-002 (no module hardening until Phase 1-4 completes)
- Required for v1.5.0 stable release
- Must complete by 2026-08-31 (Week 34)

**Success Criteria:**
- ✅ All 117 gaps identified and categorized
- ✅ Zero false-positives in scanner re-run
- ✅ 100% test coverage verification
- ✅ Zero regressions in v1.5.0 release build

**Source Files:**
- `ai_working/EPIC_001_PHASE_1_4_SPRINT_8_9.md` (full definition)
- Related Issues: #5390 (Cross-Shard FK Validation), #5373 (2PC Protocol)

---

### EPIC-002: High-Gap Module Hardening (Target: 2026-09-30)

**Status:** Parallel Execution with EPIC-001  
**Owner:** Module Hardening Committee  
**Scope:** CRITICAL + HIGH reduction in 5 modules

| Sub-Issue | Module | HIGH Gap Reduction | Effort |
|-----------|--------|-------------------|--------|
| 002-A | llm (multi_lora_manager) | 1.011 → 561 (-45%) | 30h |
| 002-B | server (http_server) | 2.279 → 1.367 (-40%) | 35h |
| 002-C | query (query_engine) | 2.288 → 1.439 (-37%) | 35h |
| 002-D | sharding (2PC/FK validation) | 7.800 → 5.000 (-36%) | 35h |
| 002-E | index (range queries) | 6.503 → 4.000 (-38%) | 30h |

**Validator Patterns:**
- InputValidator (llm module)
- RequestValidator (server module)
- QueryExecutor RAII (query module)
- TimeoutGuard (sharding module)
- RangeQueryValidator (index module)

**Success Criteria:**
- ✅ llm: HIGH 18.885 → 12.000 (-36%)
- ✅ server: HIGH 14.066 → 9.000 (-36%)
- ✅ query: HIGH 12.959 → 8.000 (-38%)
- ✅ sharding: HIGH 7.800 → 5.000 (-36%)
- ✅ index: HIGH 6.503 → 4.000 (-38%)

**Source Files:**
- `ai_working/EPIC_002_MODULE_HARDENING.md` (full definition)
- Reference: `src/graph/MODULE_GAPS.md` (L1 SOT for module state)

---

### EPIC-003: Phase 6 Extended Scanner (Target: 2026-10-31)

**Status:** Parallel with EPIC-002  
**Owner:** Code Quality Scanner Team  
**Scope:** 5 New Scanner Categories (1.500-2.500 gaps)

| Sub-Issue | Scanner Type | Patterns | Estimated Gaps | FP Target |
|-----------|--------------|----------|-----------------|-----------|
| 003-A | Type Conversion | 8-10 | 300-500 | <15% |
| 003-B | Input Validation | 10-12 | 400-600 | <12% |
| 003-C | Exception Safety | 8-10 | 300-450 | <18% |
| 003-D | Uninitialized Vars | 8-10 | 350-500 | <20% |
| 003-E | OOP Design | 14-18 | 450-700 | <25% |
| 003-F | Scanner CI/CD | N/A | N/A | Integration |

**Scanner Portfolio After Completion:**
- Current: 27 scanner categories
- New: +5 categories → 32 total
- Total Patterns: ~60+
- Expected New Gaps: 1.500-2.500 (baseline)

**Success Criteria:**
- ✅ 5 new scanners fully implemented & tested
- ✅ 1.500-2.500 new gaps discovered
- ✅ FP rate < 15% (validation via spot-checks)
- ✅ Integration with CI/CD pipeline complete

**Source Files:**
- `ai_working/EPIC_003_PHASE_6_SCANNER.md` (full definition)
- Related: `tools/gap_scanner_v3_*.py` (scanner implementation)

---

### EPIC-004: AQL 2.0.0 Mutations Phase 1 (Target: 2026-10-15)

**Status:** Independent Parallel Track  
**Owner:** Query Language Team  
**Scope:** Parser + AST Foundation for DML

| Sub-Issue | Phase | Target | Effort |
|-----------|-------|--------|--------|
| 004-A | Tokenizer DML Extension | AQL Tokens | 12h |
| 004-B | Parser DML Statements | INSERT/UPDATE/DELETE | 20h |
| 004-C | Mutation AST & Validation | Semantic Checking | 15h |
| 004-D | Documentation & Integration | Release Notes | 8h |

**Deliverables:**
- Extended AQL tokenizer with 15+ DML tokens
- 3+ DML statement parsers (INSERT, UPDATE, DELETE)
- MutationStatementAST node hierarchy
- 50+ unit tests covering parser scenarios
- Backward compatibility with AQL v1

**Success Criteria:**
- ✅ 15+ DML tokens added (no breaking changes to v1)
- ✅ 3+ DML parsers implemented
- ✅ 50+ parser test cases
- ✅ AST generation verified
- ✅ Semantic validation tests

**Source Files:**
- `ai_working/EPIC_004_AQL_MUTATIONS_PHASE_1.md` (full definition)
- Reference: `src/query/sql_parser.cpp` (pattern for parseInsert implementation)

---

### EPIC-005: Distributed Consistency Hardening (Target: 2026-11-30)

**Status:** Long-Term Strategic Initiative  
**Owner:** Distributed Systems Team  
**Scope:** Unify commit-orchestration semantics

| Sub-Issue | Focus | Estimated Effort | Impact |
|-----------|-------|------------------|--------|
| 005-A | 2PC/3PC Unification | 40h | Reduce duplication |
| 005-B | WAL Recovery Consolidation | 35h | Single recovery path |
| 005-C | Cross-Shard FK at Scale | 30h | Performance + correctness |
| 005-D | Raft Membership Safety | 25h | Consensus verification |

**Key Outcomes:**
- Unified 2PC/3PC protocol (merge CrossShardTransactionCoordinator + TwoPhaseCommitCoordinator)
- Consolidated WAL recovery interface (4 paths → 1)
- Cross-shard FK batch validation (<50ms for 1K checks)
- Property-based membership tests (15+)

**Success Criteria:**
- ✅ Protocol specification unified & documented
- ✅ Single coordinator implementation active
- ✅ 20+ protocol state tests passing
- ✅ Zero regression on 2PC workloads
- ✅ FK validation at scale verified
- ✅ 15+ property-based membership tests
- ✅ Zero split-brain vulnerabilities

**Source Files:**
- `ai_working/EPIC_005_DISTRIBUTED_CONSISTENCY.md` (full definition)
- Related: Issue #5373 (Raft + JOINT→COMMIT), Issue #5390 (FK Validation)

---

## 📅 Timeline & Sequencing

```
Timeline (Weeks 31-49 in 2026):

Week 31-34: Sprint 8-9 (Move + Concurrency) [EPIC-001]
            └─ v1.5.0 Release Gate [001-C]

Week 35-42: Module Hardening [EPIC-002-A/B/C/D/E]
            └─ Parallel: Phase 6 Scanner [EPIC-003-A/B/C/D/E]

Week 36-45: AQL Mutations Phase 1 [EPIC-004-A/B/C/D]
            
Week 40-49: Distributed Consistency [EPIC-005-A/B/C/D]
```

### Dependency Chain

```
EPIC-001 (Complete Phase 1-4)
    ↓ (must complete before)
EPIC-002 (Module Hardening)
    ↓ (feeds into)
EPIC-003 (Phase 6 Scanner) — parallel start
    ↓ (parallel with EPIC-002)
EPIC-004 (AQL Mutations) — independent
EPIC-005 (Distributed Consistency) — independent
```

---

## 🚀 Batch Issue Creation

### Prerequisites

1. **Authentication:**
   ```bash
   export GITHUB_TOKEN=******
   export GITHUB_REPOSITORY=makr-code/ThemisDB
   ```

2. **Script Location:**
   ```bash
   .github/scripts/create-q3-2026-epics.py
   ```

### Execution Modes

#### Preview (DRY-RUN)
```bash
DRY_RUN=1 python3 .github/scripts/create-q3-2026-epics.py
```

#### Create All Epics
```bash
python3 .github/scripts/create-q3-2026-epics.py
```

#### Create Single Epic
```bash
EPIC=001 python3 .github/scripts/create-q3-2026-epics.py  # EPIC-001 only
EPIC=002 python3 .github/scripts/create-q3-2026-epics.py  # EPIC-002 only
```

### Script Behavior

- **Dry-Run:** Prints all issues without creating
- **Full Mode:** Batch-creates 28 GitHub issues (5 epics + 23 sub-issues)
- **Idempotent:** Skips existing issues by title match
- **Rate-Limiting:** 1 second delay between issue creations (GitHub API compliance)

---

## 📊 Success Metrics

| Metric | Q3 Baseline | Q3 Target | Acceptance |
|--------|------------|-----------|-----------|
| Phase 1-4 Completion | 90.5% | 100% | v1.5.0 Release ✅ |
| Module HIGH Gaps | 59.148 | 52.000 | -12% Reduction ✅ |
| Scanner Categories | 27 | 32 | Phase 6 Complete ✅ |
| Test Coverage | 326 → 350+ | +24 tests | All Epics ✅ |
| Findings/KLOC (themis_core) | 8.964 | <8.000 | -11% Density ✅ |
| Production Readiness | v1.9.0-beta | v2.4 Stable | Certified ✅ |

---

## 📁 Related Documentation

### Core Source Files
- `ROADMAP.md` — Authoritative project roadmap (L0 SOT)
- `FUTURE_ENHANCEMENTS.md` — Module-level enhancement specs
- `BRANCHING_STRATEGY.md` — Branch governance (permanent branches)

### Epic Definitions (Generated)
- `ai_working/EPIC_001_PHASE_1_4_SPRINT_8_9.md` — Sprint 8-9 closure
- `ai_working/EPIC_002_MODULE_HARDENING.md` — Top-5 module hardening
- `ai_working/EPIC_003_PHASE_6_SCANNER.md` — 5 new scanner categories
- `ai_working/EPIC_004_AQL_MUTATIONS_PHASE_1.md` — AQL 2.0.0 DML foundation
- `ai_working/EPIC_005_DISTRIBUTED_CONSISTENCY.md` — 2PC/WAL/Raft unification

### Supporting References
- `src/graph/MODULE_GAPS.md` — Graph module Phase 2 completion
- `.github/scripts/create-q3-2026-epics.py` — Batch issue creation script
- `tools/gap_scanner_v3_*.py` — Scanner implementations (Phase 1-6)
- `include/security/safe_iterator.h` — SafeIterator library (Sprint 7)

---

## ✅ Acceptance Checklist

### Pre-Implementation
- [x] Deep-dive analysis complete
- [x] 5 Epic definitions created with full structure
- [x] 24 Sub-issue definitions with acceptance criteria
- [x] Timeline sequencing verified
- [x] Batch creation script tested (DRY-RUN verified)

### GitHub Issue Creation
- [ ] EPIC-001 created (#XXX)
- [ ] EPIC-002 created (#XXX)
- [ ] EPIC-003 created (#XXX)
- [ ] EPIC-004 created (#XXX)
- [ ] EPIC-005 created (#XXX)
- [ ] All 23 sub-issues linked to parent epics
- [ ] Labels applied correctly per epic
- [ ] Issue descriptions match markdown definitions

### Post-Implementation
- [ ] ROADMAP.md updated with epic issue numbers
- [ ] GitHub Projects configured for Q3 2026 tracking
- [ ] Initial sprint assignments (W31-W34 for EPIC-001)
- [ ] CI/CD workflow for issue creation documented
- [ ] Team notifications sent (Slack, email)

---

## 🔗 Related Issues

| Issue | Title | Status | Link |
|-------|-------|--------|------|
| #5373 | Raft v2: JOINT→COMMIT Consensus Safety | Open | EPIC-005 |
| #5390 | Cross-Shard FK Validation & 2PC Timeout | Open | EPIC-002-D |
| Various | Phase 1-4 Gap Remediations (117 gaps) | 90.5% Done | EPIC-001 |

---

## 📝 Notes

### Design Rationale

1. **Five Epics:** Aligned with project work streams (Phase 1-4, Modules, Scanners, AQL, Distributed)
2. **24 Sub-Issues:** Concrete, verifiable deliverables with acceptance criteria
3. **Timeline Sequencing:** EPIC-001 is critical path (blocks EPIC-002); others run in parallel
4. **Success Metrics:** All tied to baseline/target reductions in ROADMAP.md

### Next Steps

1. Execute batch issue creation script
2. Link parent epics to sub-issues via GitHub Projects
3. Assign initial sprint for EPIC-001 (W31-W34)
4. Schedule EPIC-002 start for W35 (post-v1.5.0 release)

---

**Last Updated:** 2026-07-05  
**Owner:** Copilot AI Code Generation Agent  
**Status:** 🟢 Ready for Execution
