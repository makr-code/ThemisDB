# MODULE_GAPS - LLM Wiki Module

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Overview

This document tracks known gaps, limitations, and outstanding work items for the LLM Wiki module post-Wave B.

## Known Limitations

### 1. No Multi-Backend Plugin Coordination
- **Description:** Currently supports single wiki backend at a time; no multi-backend fallback or routing
- **Impact:** Cannot mix Wikipedia with Confluence/Notion/internal wikis in same workspace
- **Workaround:** Create separate workspaces per backend; application manages routing
- **Timeline:** Phase 6 (multi-plugin coordination) planned for Q1 2027
- **Severity:** MEDIUM (operational impact, not correctness)

### 2. Fixed Provenance Chain Depth Limit
- **Description:** Maximum provenance chain depth hard-limited to 10 levels
- **Impact:** Deep synthesis chains trigger re-anchor requirement (E6505)
- **Workaround:** Limit synthesis depth in LLM orchestration layer; accept re-anchor flagging
- **Timeline:** Phase 6 (adaptive chain optimization) planned for Q1 2027
- **Severity:** LOW (manageable via policy configuration)

### 3. No Adaptive Schema Migration
- **Description:** Schema version compatibility relies on manual migration scripts
- **Impact:** Mixed-version reader/writer workloads may fail during schema transitions
- **Workaround:** Coordinate schema updates offline; single schema version per deployment
- **Timeline:** Phase 6 (adaptive migration runner) planned for Q1 2027
- **Severity:** MEDIUM (deployment complexity)

### 4. Limited Policy Hot-Reload Validation
- **Description:** Policy hot-reload is safe but canary/rollback promotion tests incomplete
- **Impact:** Policy changes cannot be validated in canary environment before full rollout
- **Workaround:** Manual policy validation before reload; full rollback if issues arise
- **Timeline:** Phase 5-6 (canary promotion framework) planned for Q4 2026/Q1 2027
- **Severity:** MEDIUM (operational risk)

### 5. No Representative Hardware Performance Baselines
- **Description:** Performance targets defined but not yet validated on diverse hardware
- **Impact:** Baseline expectations may not hold on customer hardware (ARM, older Intel, etc.)
- **Workaround:** Validate baselines on specific target hardware before production deployment
- **Timeline:** Phase 5 (hardware validation) planned for Q4 2026
- **Severity:** MEDIUM (deployment risk)

## Gap Categories

### A. Implementation Gaps (Planned Features)

| Gap ID | Component | Current Status | Target | Priority | Timeline |
|---|---|---|---|---|---|
| LW-GAP-A1 | Multi-backend plugin coordination | Not started | Phase 6 design | Medium | Q1 2027 |
| LW-GAP-A2 | Adaptive schema migration runner | Not started | Phase 6 implementation | Medium | Q1 2027 |
| LW-GAP-A3 | Cross-workspace query federation | Not started | Phase 7 | Low | Q2 2027 |
| LW-GAP-A4 | Custom knowledge source plugins | Planned API | Phase 6 | Low | Q1 2027 |

### B. Performance Optimization Gaps

| Gap ID | Item | Current | Target | Effort | Timeline |
|---|---|---|---|---|---|
| LW-GAP-B1 | Representative hardware performance validation | In progress | All target platforms | Medium | Q4 2026 (Phase 5) |
| LW-GAP-B2 | Provenance chain optimization | Linear traversal | Memoized computation | Medium | Q1 2027 (Phase 6) |
| LW-GAP-B3 | Evidence caching strategies | Basic LRU | Intelligent semantic cache | Medium | Q1 2027 (Phase 6) |
| LW-GAP-B4 | Concurrent ingest scaling | Serialized writes | Lock-free batch ingestion | High | Q2 2027 (Phase 7) |

### C. Operational Gaps

| Gap ID | Item | Current Status | Target | Effort | Timeline |
|---|---|---|---|---|---|
| LW-GAP-C1 | Workspace health checks | Basic validation | Comprehensive audit procedures | Medium | Q4 2026 (Phase 5) |
| LW-GAP-C2 | Corruption recovery procedures | Manual rebuild only | Automated recovery with validation | Medium | Q1 2027 (Phase 6) |
| LW-GAP-C3 | Policy canary/rollback framework | Planned | Zero-downtime policy updates | High | Q4 2026/Q1 2027 |
| LW-GAP-C4 | Observability integration | Basic logging | Full OpenTelemetry/metrics integration | High | Q1 2027 (Phase 6) |

### D. Testing Gaps

| Gap ID | Item | Current Coverage | Target | Status | Timeline |
|---|---|---|---|---|---|
| LW-GAP-D1 | Fuzz testing (malicious wiki content) | None | Comprehensive fuzzing | Planned | Q4 2026 (Phase 5) |
| LW-GAP-D2 | ThreadSanitizer race detection | Manual testing | Automated CI gate | Planned | Q4 2026 (Phase 5) |
| LW-GAP-D3 | Large-scale workspace persistence | Basic tests | Multi-GB workspace tests | Planned | Q1 2027 (Phase 6) |
| LW-GAP-D4 | Hardware-matrix benchmark validation | Intel AVX2 only | CI + QA hardware matrix | Planned | Q4 2026 (Phase 5) |
| LW-GAP-D5 | Policy hot-reload canary testing | Proposed | Automated canary promotion | Planned | Q4 2026/Q1 2027 |

## Open Maintenance Items

### Short-term (Q4 2026)

- [ ] Phase 5: Representative hardware performance baseline validation (Intel, ARM, older platforms)
- [ ] Phase 5: Wikipedia ingest throughput validation on CI lanes
- [ ] Phase 5: Workspace health check procedures documentation
- [ ] Phase 5: Fuzzing framework setup for guardrail pattern evasion testing
- [ ] Phase 5: ThreadSanitizer CI gate integration for concurrency validation
- [ ] Phase 5: Policy canary/rollback framework design and initial implementation
- [ ] Phase 5: Performance hardening for p95 latency targets (200 ms → 150 ms goal)

### Medium-term (Q1 2027)

- [ ] Phase 6: Multi-backend plugin coordination design and proof-of-concept
- [ ] Phase 6: Adaptive schema migration runner implementation
- [ ] Phase 6: Workspace corruption recovery automation
- [ ] Phase 6: Full OpenTelemetry and metrics integration
- [ ] Phase 6: Large-scale workspace persistence testing (multi-GB)
- [ ] Phase 6: Cross-workspace query federation design (Phase 7 prep)
- [ ] Phase 6: Complete provenance chain optimization and caching strategies

### Long-term (Q2+ 2027)

- [ ] Phase 7: Cross-workspace query federation implementation
- [ ] Phase 7: Custom knowledge source plugin SDK and examples
- [ ] Phase 7: Lock-free batch ingestion for extreme-scale workloads
- [ ] Phase 7: Distributed wiki sharding across multiple nodes

## Performance Headroom Analysis

Current Phase 5 baseline targets leave headroom for optimization:

| Metric | Phase 5 Target | Theory Maximum | Headroom | Path |
|---|---|---|---|---|
| Query latency p95 | 200 ms | ~50 ms (ideal) | ~75% | Provenance optimization, caching, parallelization |
| Ingest throughput | 100 pages/sec | ~500 pages/sec | ~80% | Batch optimization, parallelization, lock-free structures |
| Workspace isolation overhead | 10% | ~2% (ideal) | ~80% | Data structure optimization, lock-free reads |
| Guardrail pattern matching | 5 ms | ~1 ms (optimal) | ~80% | Pattern compilation, SIMD matching, caching |
| Concurrent query scaling | 10 concurrent | 100+ concurrent | ~90% | Distributed coordination (Phase 6) |

**Analysis:** Significant optimization opportunity in Phase 5 performance hardening and Phase 6 parallelization. Prioritize representative hardware validation and provenance optimization.

## Integration Dependencies

### Blocked By External Modules

| Blocker | Status | Target Resolution | Impact |
|---|---|---|---|
| LLM module orchestration API | Stable | Currently available | None |
| Retrieval module ranking API | Stable | Currently available | None |
| Storage module (persistence) | In progress | Phase B/6 | Optional; degrades to in-memory fallback |
| Observability/metrics system | Available | Can integrate anytime | Non-blocking; recommended |

### Blocking Downstream Consumers

| Consumer | Dependency | Impact if Delayed | Status |
|---|---|---|---|
| LLM module | Stable wiki API | None; currently stable | ✅ UNBLOCKED |
| Server module | Wiki query endpoints | Functional, not blocking | ✅ UNBLOCKED |
| Plugins | Plugin SDK API | Functional, not blocking | ✅ UNBLOCKED |

## Known Workarounds

### For Multi-Backend Workflows
```cpp
// Create separate workspaces per backend
auto wiki_wikipedia = llm_wiki::CreateWorkspace("wiki_wikipedia", "wikipedia");
auto wiki_internal = llm_wiki::CreateWorkspace("wiki_internal", "internal_wiki");

// Route queries based on query type
if (query_type == QueryType::GENERAL_KNOWLEDGE) {
    result = wiki_wikipedia->Query(query);
} else if (query_type == QueryType::COMPANY_SPECIFIC) {
    result = wiki_internal->Query(query);
}
```

### For Long Provenance Chains
```cpp
// Configure stricter re-anchor threshold to limit chain depth
auto config = llm_wiki::LoadPolicyConfig("process_policy.yaml");
config.max_chain_depth = 5;  // Stricter than default 10
config.re_anchor_threshold = 0.8;  // Higher confidence requirement
llm_wiki::SetProcessPolicy(config);
```

### For Hardware-Specific Tuning
```cpp
// Override performance parameters for specific hardware
auto config = llm_wiki::LoadPolicyConfig("process_policy.yaml");
if (IsPlatformARM()) {
    config.query_timeout_ms = 300;  // Looser timeout on slower hardware
    config.ingest_batch_size = 500;  // Smaller batches for ARM
}
llm_wiki::SetProcessPolicy(config);
```

## Tracking and Updates

- **Last Updated:** 2026-09-22
- **Next Review:** 2026-10-15 (after Phase 5 progress milestone)
- **Updates:** When new gaps are discovered or existing gaps are resolved, update this document
- **Cross-references:** Tied to ROADMAP.md Phase milestones and GitHub issue tracking

---

**Maintenance Responsibility:** LLM Wiki module owner  
**Review Cycle:** Bi-weekly with Phase 5 work (Q4 2026); monthly after stabilization  
**Escalation:** Gaps blocking Phase 5 release gates must be escalated immediately
