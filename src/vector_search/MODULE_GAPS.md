# MODULE_GAPS - Vector Search Module

<!-- Status: current | validated: 2026-09-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Overview

This document tracks known gaps, limitations, and outstanding work items for the vector search module post-Wave D.

## Known Limitations

### 1. No Incremental Index Updates
- **Description:** Parameter changes (e.g., HNSW M factor, IVF cluster count) require full index rebuild
- **Impact:** Operations requiring parameter tuning must recreate the entire index
- **Workaround:** Clone index, build new version, switch on completion
- **Timeline:** Phase 6 (distributed) planned to support incremental updates via versioning
- **Severity:** MEDIUM (operational impact, not correctness)

### 2. Fixed Dimension Vectors
- **Description:** Cannot mix different embedding dimensions in one index
- **Impact:** Separate indices required for different model outputs (e.g., 128-dim vs. 512-dim embeddings)
- **Workaround:** Create separate index per dimension; manage via application logic
- **Timeline:** Phase 7 (post-Wave D) planned multi-dimension support
- **Severity:** LOW (manageable via application design)

### 3. In-Memory Indices Only
- **Description:** No out-of-core support for indices exceeding available system memory
- **Impact:** Index size limited by RAM capacity
- **Workaround:** Distribute index across multiple machines (Phase 6) or partition into separate indices
- **Timeline:** Phase 6 (distributed indexing) addresses via sharding
- **Severity:** MEDIUM (scaling constraint)

### 4. Single-Machine Indexing
- **Description:** No distributed index building across multiple nodes
- **Impact:** Build time scales linearly with vector count; no parallel acceleration
- **Workaround:** Pre-build index offline; use Phase 6 distributed sharding for multi-node search
- **Timeline:** Phase 6 (Q1 2027) planned for distributed build
- **Severity:** MEDIUM (performance, not functionality)

## Gap Categories

### A. Implementation Gaps (Externalized Components)

| Gap ID | Component | Current Status | Target | Priority |
|---|---|---|---|---|
| VS-GAP-A1 | Index persistence (serialization) | Planned in Phase 6 | RocksDB integration | Medium |
| VS-GAP-A2 | Distributed index sharding | Not started | Phase 6 (Q1 2027) | Medium |
| VS-GAP-A3 | Product Quantization (PQ) | Not started | Phase 7 (Q2 2027) | Low |
| VS-GAP-A4 | Binary hash indices | Not started | Phase 7 (Q2 2027) | Low |

### B. Performance Optimization Gaps

| Gap ID | Item | Current | Target | Effort |
|---|---|---|---|---|
| VS-GAP-B1 | SIMD kernel coverage (NEON, AVX-512) | AVX2 only | All platforms | Medium |
| VS-GAP-B2 | Lock-free concurrent search | Read-write mutex | Hazard pointers | High |
| VS-GAP-B3 | Adaptive parameter tuning | Manual | Auto-tuning heuristics | Medium |
| VS-GAP-B4 | Index compaction/defragmentation | Not implemented | Planned Phase 6 | Low |

### C. Operational Gaps

| Gap ID | Item | Current Status | Target | Effort |
|---|---|---|---|---|
| VS-GAP-C1 | Index health checks | Basic validation | Comprehensive audit | Medium |
| VS-GAP-C2 | Corruption recovery procedures | Manual rebuild only | Automated recovery | Medium |
| VS-GAP-C3 | Hot-reload support | Not supported | Zero-downtime reload | High |
| VS-GAP-C4 | Observability (metrics, traces) | Basic latency tracking | Full observability suite | High |

### D. Testing Gaps

| Gap ID | Item | Current Coverage | Target | Status |
|---|---|---|---|---|
| VS-GAP-D1 | Fuzz testing (malicious inputs) | None | Comprehensive | Planned |
| VS-GAP-D2 | ThreadSanitizer race detection | Manual testing | Automated CI gate | Planned |
| VS-GAP-D3 | Large-scale index persistence | None | Multi-GB file tests | Planned |
| VS-GAP-D4 | Cross-platform benchmark validation | Release hardware only | CI + QA hardware matrix | Planned |

## Open Maintenance Items

### Short-term (Q4 2026)

- [ ] Phase 5 hardening: SIMD kernel tuning on additional hardware architectures
- [ ] Phase 5: Lock-free search design phase (research/POC)
- [ ] Performance baseline validation on diverse hardware (CI matrix expansion)
- [ ] Documentation: Operational runbook and troubleshooting guide

### Medium-term (Q1 2027)

- [ ] Phase 6: Distributed index sharding design and implementation
- [ ] Phase 6: Index persistence with checksums and recovery
- [ ] Fuzz testing framework setup
- [ ] Continuous monitoring and alerting system integration

### Long-term (Q2+ 2027)

- [ ] Phase 7: Product Quantization (PQ) for extreme-scale indices
- [ ] Phase 7: Binary hash indices
- [ ] Phase 7: Multi-dimension support
- [ ] Lock-free concurrent structures (if Phase 5 research proves viable)

## Performance Headroom Analysis

Current baseline performance leaves headroom for optimization:

| Metric | Current | Baseline Limit | Headroom |
|---|---|---|---|
| Insert throughput | 1 800 ops/sec | Theory: ~5 000 ops/sec | ~60% |
| Query latency | 8.2 ms | Theory: ~5 ms (SIMD optimal) | ~40% |
| Memory overhead | 31% | Theory: ~15% (compression) | ~50% |
| Concurrent scaling | 100+ queries | Theory: 1 000+ (distributed) | ~90% |

**Analysis:** Significant optimization opportunity in SIMD, compression, and distributed scaling. Prioritize Phase 5 hardening and Phase 6 distribution.

## Integration Dependencies

### Blocked By External Modules

| Blocker | Status | Target Resolution |
|---|---|---|
| Storage module (index persistence) | In progress | Phase 6 (Q1 2027) |
| Distributed coordination framework | Not started | Phase 6 (Q1 2027) |
| Observability/metrics system | Available | Can integrate anytime |

### Blocking Downstream Consumers

| Consumer | Dependency | Impact if Delayed |
|---|---|---|
| RAG module | Stable vector search API | None; currently stable |
| Server module | Vector similarity endpoints | Functional, not blocking |

## Known Workarounds

### For Fixed Dimensions
```cpp
// Create separate indices for different embedding dimensions
auto index_128 = CreateVectorIndex(128, DistanceMetric::COSINE);
auto index_512 = CreateVectorIndex(512, DistanceMetric::COSINE);
// Route queries to appropriate index based on dimension
```

### For Large Indices
```cpp
// Partition into multiple shards manually (Phase 6 will automate)
auto index_0 = CreateVectorIndex(128, ...);  // vectors 0–1M
auto index_1 = CreateVectorIndex(128, ...);  // vectors 1M–2M
// Application merges results from multiple indices
```

### For Parameter Tuning
```cpp
// Build new index with different parameters, then switch
auto new_index = CreateVectorIndex(128, ..., HNSWConfig{M=32, ...});
// Warm up new index offline, then switch at query time
// Keep old index for rollback if needed
```

## Tracking and Updates

- **Last Updated:** 2026-09-22
- **Next Review:** 2026-10-15 (after Phase 5 progress milestone)
- **Updates:** When new gaps are discovered or existing gaps are resolved, update this document
- **Cross-references:** Tied to ROADMAP.md Phase milestones and GitHub issue tracking

---

**Maintenance Responsibility:** Module owner (vector search)  
**Review Cycle:** Bi-weekly with Phase 5 work; monthly after stabilization
