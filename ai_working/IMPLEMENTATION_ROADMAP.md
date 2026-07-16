# Gap Scanner Implementation Roadmap — Phase 1-6 Timeline

**Generated:** 2026-05-19  
**Status:** PLANNING & TRACKING  
**Scope:** All 18 scanners (Phase 1-4 + Phase 1-4 Enhanced + Phase 5 + Phase 6)  
**Target Completion:** Q4 2026

> Historical snapshot note (2026-05-27): canonical active issue set is
> master #5172, categories #5184-#5194, modules #5195-#5201 plus
> #5180/#5181/#5182.

---

## Executive Summary

```
Phase 1 (COMPLETE ✅)         — 8 scanners (31,720 gaps)
Phase 1-4 Enhanced (✅ S-1/S-2/S-3) — Security patterns (+80 gaps)
Phase 1-4 Enhanced (NEXT)     — Memory/Concurrency patterns (planned)
Phase 5 (COMPLETE ✅)         — 5 scanners (123,911 gaps)
Phase 6                       — 5 new scanners (+6,000–10,000 gaps)
──────────────────────────────────────────────────────────────
PHASE 1-6 TOTAL               — 18 scanners, ~165,000–185,000 gaps
```

---

## Detailed Phase Timeline

### PHASE 1-4: COMPLETE ✅ (2026-05-18)

| Component | Status | Details | Issues |
|-----------|--------|---------|--------|
| Security Scanner | ✅ DONE | 1,514 gaps + S-1/S-2/S-3 enhancements (+80) | [#5184](https://github.com/makr-code/ThemisDB/issues/5184) |
| Memory Scanner | ✅ DONE | 2,227 gaps, CWE-120/125/126/190/416 | [#5209](https://github.com/makr-code/ThemisDB/issues/5209) |
| Reliability Scanner | ✅ DONE | 14,519 gaps, timeout/retry/exception | [#5185](https://github.com/makr-code/ThemisDB/issues/5185) |
| Concurrency Scanner | ✅ DONE | 1,834 gaps, CWE-362, deadlocks | [#5186](https://github.com/makr-code/ThemisDB/issues/5186) |
| RAII Scanner | ✅ DONE | 1,855 gaps, resource management | [#5187](https://github.com/makr-code/ThemisDB/issues/5187) |
| Container Scanner | ✅ DONE | 7,629 gaps, O(n²) patterns, iterators | [#5213](https://github.com/makr-code/ThemisDB/issues/5213) |
| Platform Scanner | ✅ DONE | 1,146 gaps, Windows/Linux/POSIX | [#5188](https://github.com/makr-code/ThemisDB/issues/5188) |
| Performance Scanner | ✅ DONE | 1,017 gaps, allocations, string ops | [#5189](https://github.com/makr-code/ThemisDB/issues/5189) |
| **Phase 1-4 Total** | **✅ DONE** | **31,720 gaps** | **[#5172](https://github.com/makr-code/ThemisDB/issues/5172)** |

**Effort Delivered:** 2,800 LOC across 8 scanners  
**Gap Quality:** Validated, CRITICAL/HIGH breakdown: 27%/27%  

---

### PHASE 1-4 ENHANCEMENTS: ✅ S-1/S-2/S-3 COMPLETE (2026-06-03)

**Goal:** Increase Phase 1-4 detection sensitivity via +12 new patterns  
**Actual Effort:** ~240 LOC (S-1: 60 LOC, S-2: 85 LOC, S-3: 95 LOC)  
**Gap Detection:** S-2: +12 gaps, S-3: +68 gaps (command injection +45, path traversal +23)

#### S-1-S-3: Security Pattern Enhancements ✅ COMPLETE

| Pattern | CWE | Patterns | Est. Gaps | Actual | Status |
|---------|-----|----------|-----------|--------|--------|
| S-1: Hardcoded Secrets | CWE-798 | API tokens, SSH keys, DB credentials, certs | +100–160 | 0* | ✅ DONE |
| S-2: Crypto Weaknesses | CWE-327 | Hash algos, DES/3DES, XOR, weak RNG | +70–120 | +12 | ✅ DONE |
| S-3: Injection Attacks | CWE-94 | Command injection, path traversal, XXE, LDAP, format strings | +92–153 | +68 | ✅ DONE |

**Security Scanner Enhancement Total:** +80 new detections  
**Implementation Notes:**
- S-1 conservative approach avoids false positives (e.g., database key patterns)
- S-2 detects EVP_sha1(), MD5, weak RNG, hardcoded IVs/salts
- S-3 detects std::system(), popen(), path traversal, XXE parsing, LDAP injection, format strings
- All patterns integrated into gap_scanner_v3_security.py
- False positive rate validation: <5% (conservative patterns reduce false alarms)

**Completed Subtasks:**
- [x] Write S-1 pattern detection code
- [x] Write S-2 pattern detection code  
- [x] Write S-3 pattern detection code
- [x] Test S-1/S-2/S-3 on full codebase
- [x] Integrate into gap_scanner_v3_security.py
- [x] Validate false positive rate < 5%

#### M-1-M-2: Memory Pattern Enhancements (NEXT PHASE)

| Pattern | CWE | Patterns | Est. Gaps | Effort |
|---------|-----|----------|-----------|--------|
| M-1: Use-After-Free | CWE-416 | Iterator invalidation, temp pointers, post-move | +125–180 | 85 LOC |
| M-2: Double-Free | CWE-415 | Exception paths, loop clearing | +35–60 | 60 LOC |

**Memory Scanner Total:** 2,227 → 2,387–2,467 gaps (+160–240)  
**Subtasks:**
- [ ] Write M-1 pattern detection code
- [ ] Write M-2 pattern detection code
- [ ] Test M-1/M-2 on 5 sample files each
- [ ] Integrate into gap_scanner_v3_memory.py
- [ ] Validate false positive rate < 5%

#### C-1: Concurrency Pattern Enhancement (Week 5-6)

| Pattern | CWE | Patterns | Est. Gaps | Effort |
|---------|-----|----------|-----------|--------|
| C-1: Race Conditions | CWE-362 | TOCTOU, double-checked locking, lost wakeup | +60–95 | 85 LOC |

**Concurrency Scanner Total:** 1,834 → 1,894–1,929 gaps (+60–95)  
**Subtasks:**
- [ ] Write C-1 pattern detection code
- [ ] Test C-1 on 5 sample files
- [ ] Integrate into gap_scanner_v3_concurrency.py
- [ ] Validate false positive rate < 5%

#### Integration & Validation (Week 7-8)

- [ ] Update orchestrator to use enhanced scanners
- [ ] Full Phase 1-4 Enhanced scan (65 modules)
- [ ] Verify gap counts match projections ±10%
- [ ] Generate enhanced summary report
- [ ] Update GitHub issues with new totals

**Phase 1-4 Enhanced Total:** 31,720 → 33,920–34,920 gaps (+2,200–3,200)

---

### PHASE 5: COMPLETE ✅ (2026-05-18)

| Component | Status | Details | Gaps | CWE/Focus |
|-----------|--------|---------|------|-----------|
| Type Conversion | ✅ DONE | Narrowing, overflow, precision loss | 15,930 | CWE-190 |
| Input Validation | ✅ DONE | Buffer overflow, bounds checks | 8,266 | CWE-787 |
| Exception Safety | ✅ DONE | noexcept, move correctness, RAII in exceptions | 31,247 | CWE-544 |
| Uninitialized | ✅ DONE | CWE-457, undefined behavior, use-before-init | 27,563 | CWE-457 |
| OOP Design | ✅ DONE | Virtual misuse, slicing, CRTP, inheritance issues | 16,688 | Various |
| **Phase 5 Total** | **✅ DONE** | **5 new scanners** | **99,694 gaps** | **[#5216-#5220](https://github.com/makr-code/ThemisDB/issues/5216)** |

**Effort Delivered:** 1,370 LOC across 5 scanners  
**Gap Quality:** Validated, Phase 5 expanded detection surface 391%  

---

### PHASE 6: Q3-Q4 2026 (UPCOMING)

**Goal:** Advanced C++ pattern detection for ABI, const-correctness, templates, build system, lifetime  
**Total Effort:** 1,480 LOC across 5 scanners  
**Expected Gap Increase:** +6,000–10,000  
**Timeline:** 8 weeks parallel development, Week 1-8 of Q3 2026

#### Sprint 1 (Week 1-2): ABI Safety + Build System

| Scanner | Status | Patterns | LOC | Est. Gaps | Effort |
|---------|--------|----------|-----|-----------|--------|
| P6-1: ABI Safety | 🟡 PLANNED | Padding, alignment, layout, pack, POD | 320 | +800–1,200 | 8h |
| P6-4: Build System | 🟡 PLANNED | CMake deps, compiler flags, linker scripts | 280 | +200–400 | 7h |

**Sprint 1 Deliverables:**
- [ ] Design P6-1 struct layout analysis engine
- [ ] Implement 8-10 ABI safety patterns
- [ ] Design P6-4 CMake/linker rule checker
- [ ] Implement 6-8 build system patterns
- [ ] Test on representative module set
- [ ] Integration into orchestrator

#### Sprint 2 (Week 3-4): Const Correctness

| Scanner | Status | Patterns | LOC | Est. Gaps | Effort |
|---------|--------|----------|-----|-----------|--------|
| P6-2: Const Correctness | 🟡 PLANNED | Mutable, logical const, getters, method chains | 380 | +2,500–3,500 | 10h |

**Sprint 2 Deliverables:**
- [ ] Design const propagation analyzer
- [ ] Implement 12-15 const-correctness patterns
- [ ] Handle edge cases (mutable, const_cast, reference returns)
- [ ] Test on typical class hierarchies
- [ ] Integrate with symbol analysis

#### Sprint 3 (Week 5-6): Template Meta-Programming

| Scanner | Status | Patterns | LOC | Est. Gaps | Effort |
|---------|--------|----------|-----|-----------|--------|
| P6-3: Template Meta-Programming | 🟡 PLANNED | SFINAE, concepts, instantiation, ADL | 350 | +600–1,000 | 9h |

**Sprint 3 Deliverables:**
- [ ] Design template analysis engine
- [ ] Implement 10-12 template patterns
- [ ] Handle C++20 concepts detection
- [ ] Test on generic code
- [ ] Integrate concept validation

#### Sprint 4 (Week 7-8): Ownership & Lifetime (CRITICAL)

| Scanner | Status | Patterns | LOC | Est. Gaps | Effort |
|---------|--------|----------|-----|-----------|--------|
| P6-5: Ownership & Lifetime | 🟡 PLANNED | Move semantics, dangling refs, lifetime extension | 370 | +3,500–5,000 | 12h |

**Sprint 4 Deliverables:**
- [ ] Design lifetime tracking engine
- [ ] Implement 14-18 ownership patterns
- [ ] Handle move semantics edge cases
- [ ] Implement dangling reference detection
- [ ] Test on complex code patterns
- [ ] Semantic lifetime analysis

#### Integration (Week 9): Full Phase 1-6 Pipeline

- [ ] Integrate all 5 Phase 6 scanners into orchestrator
- [ ] Full Phase 1-6 scan (65 modules, 18 scanners)
- [ ] Aggregate gap reports
- [ ] Generate summary statistics
- [ ] Create GitHub issues for new gaps
- [ ] Update ROADMAP.md and documentation

**Phase 6 Total:** +6,000–10,000 gaps  
**Phase 1-6 Combined:** ~165,000–185,000 gaps

---

## Module Hardening Roadmap (Parallel Track)

### Tier 1: Critical Modules (Top 10)

**Target:** Address gaps in modules producing 85,271 gaps (54.8% of Phase 1-5 total)

| Module | Gaps | Milestone | Owner | Priority |
|--------|------|-----------|-------|----------|
| llm | 19,838 | v1.5.0 | [#5195](https://github.com/makr-code/ThemisDB/issues/5195) | 🔴 P0 |
| server | 16,186 | v1.5.0 | [#5196](https://github.com/makr-code/ThemisDB/issues/5196) | 🔴 P0 |
| sharding | 9,296 | v1.5.0 | [#5197](https://github.com/makr-code/ThemisDB/issues/5197) | 🔴 P0 |
| index | 7,633 | v1.5.0 | [#5198](https://github.com/makr-code/ThemisDB/issues/5198) | 🔴 P0 |
| query | 7,327 | v1.5.0 | [#5199](https://github.com/makr-code/ThemisDB/issues/5199) | 🔴 P0 |
| storage | 6,678 | v1.5.0 | [#5200](https://github.com/makr-code/ThemisDB/issues/5200) | 🔴 P0 |
| analytics | 5,911 | v1.5.0 | [#5201](https://github.com/makr-code/ThemisDB/issues/5201) | 🔴 P0 |
| rag | 4,982 | v1.5.0 | [#5180](https://github.com/makr-code/ThemisDB/issues/5180) | 🔴 P0 |
| security | 4,343 | v1.5.0 | [#5181](https://github.com/makr-code/ThemisDB/issues/5181) | 🔴 P0 |
| content | 4,077 | v1.5.0 | [#5182](https://github.com/makr-code/ThemisDB/issues/5182) | 🔴 P0 |

**Strategy:** 
1. Triage top 10 by severity (CRITICAL gaps first)
2. Fix patterns affecting multiple modules (shared root causes)
3. Target 25% gap reduction (21,300 → 15,975 gaps) by end Q3 2026
4. Validation: Re-run scanner suite, measure gap reduction

### Tier 2: Important Modules (11-25)

**Strategy:** 
1. Secondary priority after Tier 1 critical gaps fixed
2. Target 15% gap reduction by end Q4 2026
3. Focus on CRITICAL/HIGH severity gaps only

### Tier 3: Maintenance Modules (26+)

**Strategy:** 
1. Continuous low-priority fixes
2. Incorporate into regular code review process
3. Track metrics for codebase quality trends

---

## Resource & Effort Tracking

### Phase 1-4 Enhanced (Q3 2026 Week 1-8)

```
Person-Weeks Required: 4.8 PW (4 weeks × 1 person)
Breakdown:
  - Security patterns (S-1/S-2/S-3): 1.6 PW
  - Memory patterns (M-1/M-2): 1.2 PW
  - Concurrency patterns (C-1): 1.0 PW
  - Integration & validation: 1.0 PW
  
Tools: gap_scanner_v3_*.py (add_pattern methods), test files
Code Review: 2 rounds (pattern validation, integration)
```

### Phase 6 Development (Q3-Q4 2026 Week 1-9)

```
Person-Weeks Required: 12 PW (8 weeks × 1 person, 1 week integration)
Breakdown:
  - Sprint 1 (ABI + Build): 2 PW
  - Sprint 2 (Const): 2.5 PW
  - Sprint 3 (Templates): 2.25 PW
  - Sprint 4 (Lifetime): 3 PW
  - Integration: 2.25 PW

Tools: New gap_scanner_v3_p6_*.py modules, orchestrator updates
Code Review: 3 rounds (pattern design, implementation, integration)
```

### Module Hardening (Parallel, Q3-Q4 2026)

```
Estimated Effort (for Tier 1 only): 20-30 PW
Rate: ~15-20% gap reduction per PW invested (varies by module)
Target: 21,300 gaps fixed (Tier 1 critical only)
```

---

## Success Metrics & Validation

### Phase 1-4 Enhanced

- ✅ All 12 new patterns implemented and tested
- ✅ Gap count increases from 31,720 → 33,920–34,920 (+7–10%)
- ✅ False positive rate < 5% per pattern
- ✅ Validation against sample code with known instances

### Phase 6

- ✅ All 18 scanner suite operational (13 Phase 1-5 + 5 Phase 6)
- ✅ Total gap count: 165,000–185,000 (40–45% increase from Phase 1-5)
- ✅ Each Phase 6 scanner produces deterministic results
- ✅ Severity distribution estimated (CRITICAL/HIGH/MEDIUM)
- ✅ Documentation complete and integrated

### Module Hardening

- ✅ Tier 1 gaps reduced 25% (Target: 21,300 → 15,975)
- ✅ Zero CRITICAL security gaps in production modules
- ✅ Gap reduction metrics tracked and reported
- ✅ Build system validated for all modules

---

## Blockers & Dependencies

| Item | Status | Impact | Mitigation |
|------|--------|--------|-----------|
| Phase 1-5 validation | ✅ DONE | None | N/A |
| GitHub issue aggregation | ✅ DONE | None | N/A |
| P6-5 (Lifetime) complexity | 🟡 PLANNED | HIGH | Start with simple patterns; iterate |
| Parallel workstreams (Phase 1-4 + 6) | 🟡 PLANNED | MEDIUM | Clear ownership per stream |
| Module fix prioritization | 🟡 PLANNING | MEDIUM | Data-driven approach (gap count + severity) |

---

## Related Documentation

- [PHASE_5_IMPLEMENTATION_COMPLETE.md](PHASE_5_IMPLEMENTATION_COMPLETE.md) — Phase 1-5 completion
- [PHASE_6_SCANNER_DESIGN.md](PHASE_6_SCANNER_DESIGN.md) — Phase 6 detailed design
- [PHASE_1_4_IMPROVEMENTS.md](PHASE_1_4_IMPROVEMENTS.md) — Phase 1-4 enhancement patterns
- [ROADMAP.md](../ROADMAP.md) — Project roadmap with gap analysis results
- [FUTURE_ENHANCEMENTS.md](../FUTURE_ENHANCEMENTS.md) — Enhancement backlog

---

**Status:** 🟢 PLANNING COMPLETE | 📊 All phases designed | 🚀 Ready for execution Q3 2026
