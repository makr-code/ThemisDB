# Module Hardening — Tier 1 Implementation Guide

**Date:** 2026-08-02  
**Phase:** Module Hardening (Q3-Q4 2026)  
**Objective:** 25% gap reduction across Top 10 modules (20,653 total gap fixes)

---

## Overview

This document provides the execution strategy for Tier 1 Module Hardening across the Top 10 critical modules. Each module targets a 25% gap reduction using the Phase 1-6 scanner suite and root-cause analysis.

### Tier 1 Modules (Prioritized by Gap Count)

| Rank | Module | Phase 1-5 Gaps | Target Reduction (25%) | Remaining | Est. Effort | Priority |
|------|--------|---|-----|----------|--------|----------|
| 1 | llm | 19,838 | -4,960 | 14,878 | ~8 weeks | 🟠 IN PROGRESS |
| 2 | server | 16,183 | -4,046 | 12,137 | ~7 weeks | ⬜ Queued |
| 3 | sharding | 9,296 | -2,324 | 6,972 | ~4 weeks | ⬜ Queued |
| 4 | index | 7,633 | -1,908 | 5,725 | ~4 weeks | ⬜ Queued |
| 5 | query | 7,327 | -1,832 | 5,495 | ~4 weeks | ⬜ Queued |
| 6 | storage | 5,892 | -1,473 | 4,419 | ~3 weeks | ⬜ Queued |
| 7 | analytics | 4,250 | -1,063 | 3,187 | ~2 weeks | ⬜ Queued |
| 8 | rag | 4,100 | -1,025 | 3,075 | ~2 weeks | ⬜ Queued |
| 9 | security | 3,814 | -954 | 2,860 | ~2 weeks | ⬜ Queued |
| 10 | content | 3,278 | -820 | 2,458 | ~2 weeks | ⬜ Queued |

**Tier 1 Total:** 82,611 gaps → 62,000 gaps (20,653 gap reduction)

---

## Module Hardening Strategy

### Phase A: Root-Cause Analysis (Per Module)

For each module, identify the top gap patterns:

1. **Gap Category Breakdown**
   - Run Phase 1-6 scanners against module source
   - Identify distribution: S-1, S-2, S-3, M-1, M-2, C-1, etc.
   - Find patterns causing ≥80% of gaps

2. **Shared Pattern Identification**
   - Compare top patterns across similar modules
   - Example: Memory safety patterns in server/llm/index
   - Create pattern-based remediation batches

3. **Impact Assessment**
   - Estimate fix complexity per pattern
   - Identify architectural vs. localized issues
   - Prioritize by effort vs. gap reduction ratio

### Phase B: Remediation Implementation (Per Module)

1. **Batch Creation**
   - Group fixes by pattern type
   - Example: "Use-After-Free in LLM Module" batch
   - Target: 10-20 fixes per batch to reduce review overhead

2. **Fix Development**
   - Apply standard remediations for pattern type
   - Example: Replace malloc/free with std::unique_ptr
   - Maintain backward compatibility

3. **Testing & Validation**
   - Run focused unit tests
   - Verify no regressions in downstream modules
   - Re-scan module to confirm gap reduction

### Phase C: Acceptance & Sign-Off (Per Module)

1. **Metrics Verification**
   - Confirm ≥25% gap reduction achieved
   - Document gap reduction by category
   - List all pull requests and commits

2. **Documentation Update**
   - Update `src/<module>/ROADMAP.md` with Phase completion
   - Add hardening note to module documentation
   - Link to GitHub issues and PRs

3. **Tier 1 Completion**
   - Mark in `ROADMAP.md` when all Tier 1 modules complete
   - Move to Tier 2 (secondary modules) if resources allow

---

## Block 1: LLM Module Hardening (🟠 IN PROGRESS)

**Target:** 19,838 gaps → 14,878 gaps (-4,960, 25% reduction)

### Completed (2026-07-17)

**Block A Deliverables:**
- `src/llm/llm_client_default.cpp`: Stub TODO replaced with LLMPluginManager delegation + fallback
- `tests/llm/test_llm_hardening_phase4.cpp`: 22 new GTest cases (CBS-H, TQM-H, PCL-H, SHD-H)
- `src/llm/ROADMAP.md`: Phase 4 markers updated to in-progress

### Planned Batches (Remaining Work)

#### LLM Batch 1: Memory Safety Hardening (Est. 2 weeks)
- Focus: M-1 (Use-After-Free), M-2 (Double-Free) in embedding/model lifecycle
- Target gaps: ~500
- Key files: `src/llm/embedded_llm.cpp`, `src/llm/model_cache.cpp`, `src/llm/embedding_pool.cpp`
- Remediation: Replace raw pointers with std::unique_ptr/std::shared_ptr

#### LLM Batch 2: Concurrency & Thread Safety (Est. 2 weeks)
- Focus: C-1 (Race Conditions) in model cache, embedding pool
- Target gaps: ~600
- Key files: `src/llm/embedding_pool.cpp`, `src/llm/final_layer_orchestrator.cpp`
- Remediation: Add proper mutex guards, use std::atomic for flags

#### LLM Batch 3: Security Hardening (Est. 2 weeks)
- Focus: S-1 (API key handling), S-3 (input validation)
- Target gaps: ~400
- Key files: `src/llm/llm_plugin_manager.cpp`, `src/llm/llm_client_default.cpp`
- Remediation: Externalize secrets, validate prompts and model names

#### LLM Batch 4: Error Handling & Edge Cases (Est. 1.5 weeks)
- Focus: Unhandled exceptions, error propagation
- Target gaps: ~300
- Key files: `src/llm/llm_client_*.cpp`
- Remediation: Add try/catch blocks, validate return codes

#### LLM Batch 5: Documentation & Sign-Off (Est. 0.5 weeks)
- Update `src/llm/ROADMAP.md` with final metrics
- Update module Doxygen headers
- Create hardening summary

**LLM Total:** ~4,960 gap reduction = ~1,960 lines of remediation code

---

## Block 2-5: Server, Sharding, Index, Query, Storage Hardening (⬜ QUEUED)

### Server Module Hardening (Block 2)
- **Target:** 16,183 gaps → 12,137 gaps (-4,046)
- **Key Areas:** Network I/O, RPC handling, concurrent connections
- **Estimated Batches:** 5-6 parallel work streams
- **Effort:** ~7 weeks

### Sharding Module Hardening (Block 3)
- **Target:** 9,296 gaps → 6,972 gaps (-2,324)
- **Key Areas:** State machine consistency, rebalancing logic
- **Estimated Batches:** 3-4 work streams
- **Effort:** ~4 weeks

### Index Module Hardening (Block 4)
- **Target:** 7,633 gaps → 5,725 gaps (-1,908)
- **Key Areas:** Index structure integrity, query validation
- **Estimated Batches:** 3-4 work streams
- **Effort:** ~4 weeks

### Query Module Hardening (Block 5)
- **Target:** 7,327 gaps → 5,495 gaps (-1,832)
- **Key Areas:** Query execution, cardinality estimation, plan safety
- **Estimated Batches:** 3-4 work streams
- **Effort:** ~4 weeks

### Storage Module Hardening (Block 6)
- **Target:** 5,892 gaps → 4,419 gaps (-1,473)
- **Key Areas:** Persistence guarantees, transaction isolation
- **Estimated Batches:** 2-3 work streams
- **Effort:** ~3 weeks

---

## Block 6-10: Analytics, RAG, Security, Content Hardening (⬜ QUEUED)

### Analytics Module (Block 7)
- **Target:** 4,250 gaps → 3,187 gaps (-1,063)
- **Effort:** ~2 weeks

### RAG Module (Block 8)
- **Target:** 4,100 gaps → 3,075 gaps (-1,025)
- **Effort:** ~2 weeks

### Security Module (Block 9)
- **Target:** 3,814 gaps → 2,860 gaps (-954)
- **Effort:** ~2 weeks

### Content Module (Block 10)
- **Target:** 3,278 gaps → 2,458 gaps (-820)
- **Effort:** ~2 weeks

---

## Success Metrics

### Per-Module Acceptance Criteria

✅ Gap reduction ≥25% (tracked via gap scanner rescan)  
✅ All fixes merged to develop branch  
✅ All focused tests pass (`ctest -L "module_<name>" --timeout 120`)  
✅ No new regressions in integration tests  
✅ Module ROADMAP.md updated with hardening closure  
✅ Doxygen documentation coverage ≥95%

### Tier 1 Completion Criteria

✅ All 10 modules achieve 25% gap reduction  
✅ Total 20,653 gap fixes merged and verified  
✅ Sanitizer/ASAN tests pass for all modules  
✅ Release-critical integration tests green  
✅ Root ROADMAP.md updated with Tier 1 completion  

---

## Risk Mitigation

### Risk: Merge Conflicts from Parallel Work
**Mitigation:**
- Divide modules geographically (index/query/storage in one block, server/sharding in another)
- Use feature branches with clear naming: `hardening/llm-memory-safety`
- Daily sync meetings to coordinate across blocks

### Risk: Regression in Production Paths
**Mitigation:**
- 100% test coverage requirement per batch (focused tests + integration)
- ASAN/UBSan enabled in all test runs
- Pre-merge verification against baseline benchmark

### Risk: Underestimated Effort
**Mitigation:**
- Weekly progress tracking against 25% reduction target
- Flex scheduling: if module hardening underperforms, backfill with Tier 2 modules
- Post-mortem on estimation vs. actual for next Tier

---

## Timeline

| Phase | Dates | Modules | Status |
|-------|-------|---------|--------|
| LLM Hardening | 2026-07-17 → 2026-09-15 | llm | 🟠 IN PROGRESS |
| Server/Sharding Hardening | 2026-08-15 → 2026-10-15 | server, sharding | ⬜ Pending |
| Index/Query/Storage Hardening | 2026-08-20 → 2026-10-20 | index, query, storage | ⬜ Pending |
| Analytics/RAG Hardening | 2026-09-01 → 2026-09-30 | analytics, rag | ⬜ Pending |
| Security/Content Hardening | 2026-09-15 → 2026-10-15 | security, content | ⬜ Pending |
| **Tier 1 Completion** | **2026-11-30** | **All 10** | **⬜ Target** |

---

## References

- Root Roadmap: `/home/runner/work/ThemisDB/ThemisDB/ROADMAP.md`
- Phase 1-6 Scanner Suite: `/home/runner/work/ThemisDB/ThemisDB/tools/scanners/`
- Module ROADMAPs: `/home/runner/work/ThemisDB/ThemisDB/src/<module>/ROADMAP.md`
- LLM Phase 4 Tests: `/home/runner/work/ThemisDB/ThemisDB/tests/llm/test_llm_hardening_phase4.cpp`
- Gap Analysis: `/home/runner/work/ThemisDB/ThemisDB/ai_working/IMPLEMENTATION_ROADMAP.md`

---

## Contact & Escalation

For questions or blockers during hardening:
1. Check module-specific ROADMAP.md for guidance
2. Review gap pattern documentation in scanner files
3. Escalate architectural changes to maintainers team

---

**Document Version:** 1.0  
**Last Updated:** 2026-08-02  
**Next Review:** 2026-08-15
