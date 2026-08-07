> **Status:** 2026-08-06 – Phase 4-6 Production Requirements (v2.0.0)  
> **Updated:** August 2026 with Phase 4-6 deliverables  
> **Scope:** All 14 search components (Phase 1-6 complete)

# ThemisDB Search Module - Production Requirements

## Purpose and Scope

This document is the **canonical reference for production minimum requirements** for the ThemisDB Search Module (v2.0.0).  
It defines mandatory operational, performance, and SLA requirements covering all 14 search components across Phases 1-6.

## Document Boundaries (Canonical Split)

- **`src/search/PRODUCTION_REQUIREMENTS.md` (this document):** Production-minimum operational requirements (MUST/MUST NOT), SLA targets, hardware profiles, data constraints, operational procedures.
- **`src/search/README.md`:** Feature overview, architecture context, API examples.
- **`src/search/ROADMAP.md`:** Delivery phases, completed features, readiness milestones (Phases 1-6).
- **`src/search/FUTURE_ENHANCEMENTS.md`:** Mid-term/long-term enhancements and research roadmap.

## Mandatory Production Requirements

### Phase 1-6 Completion Criteria
- [x] **Phase 1 (Design/API Contract):** v2.0.0 frozen contracts for HybridSearch, DistributedHybridSearch, SearchResultStream
- [x] **Phase 2 (Core Implementation):** Distributed merge hardening with shard-failure handling
- [x] **Phase 3 (Error Handling):** Unified error codes and fail-safe patterns across all 14 components
- [x] **Phase 4 (Test Expansion):** 128+ edge case and stress tests (SDS, RET, FUS, DIS, UTL, ANL, INT series)
- [x] **Phase 5 (Performance Gatekeeping):** SRCP-1..6 release gates locked (p99, throughput, overhead)
- [x] **Phase 6 (Documentation & Acceptance):** 100% Doxygen coverage, FINAL_ACCEPTANCE_CHECKLIST.md signed off

### Mandatory Operational Requirements

- **MUST:** All 14 search components deployed with explicit error handling per `search_error_codes.h`
- **MUST:** SearchStats populated on every search operation with `primary_error_code` and degradation flags
- **MUST:** Configuration validated at startup; missing/invalid values → fail-closed
- **MUST:** k-limit enforced (max k = 10,000 hard limit)
- **MUST NOT:** Disable error/authorization checks in production paths
- **MUST NOT:** Return unlimited result sets without pagination

## Hardware Profile

### Minimum Production Hardware
- **CPU:** 1 core (minimum deployment, degraded performance)
- **RAM:** 2 GB (bare minimum for in-memory indexing)
- **GPU:** None (CPU-only fallback available)
- **Network:** 10 Mbps (sufficient for single-shard deployments)

### Recommended Production Hardware
- **CPU:** 8 cores (4x improvement over minimum)
- **RAM:** 32 GB (full working set for typical datasets)
- **GPU:** 1× NVIDIA RTX class GPU (optional, 8× speedup on vector ops)
- **Network:** 1 Gbps (for multi-shard distributed merges)

## Dependencies

### Required Backends
- **BM25:** Mandatory (lexical retrieval)
- **Vector/Dense:** Optional (vector retrieval available; CPU fallback if absent)
- **LLM:** Optional (reranking available; transparent fallback if absent)

### SLA Targets

#### Latency (p99)
- Hybrid search dispatch (100 shards, 10k candidates): **≤ 15 ms** (Gate: SRCP-1 ≤ 16.5 ms)
- Distributed merge (64 shards, 1K results each): **≤ 50K results/sec** (Gate: SRCP-2 ≥ 45K/sec)
- Reranking overhead (LLM fallback): **≤ 5 ms** (Gate: SRCP-3 ≤ 5.5 ms)
- GPU dispatch + CPU fallback: **GPU ≤ 8 ms, CPU ≤ 10 ms** (Gate: SRCP-4)
- Stream buffer flush (1K batch): **≤ 10 ms** (Gate: SRCP-5 ≤ 11 ms)
- Query expansion (1K queries): **≤ 50 ms** (Gate: SRCP-6 ≤ 55 ms)

#### Availability
- Distributed scenarios: **99.9% uptime** (3-nines)
- Single-node deployments: **99.5% uptime** (depends on index availability)

#### Throughput
- Merge throughput: ≥ 45K results/sec
- Query expansion: ≥ 18K queries/sec
- Stream flush: ≥ 100 batches/sec

## Data Constraints

| Constraint | Limit | Enforcement |
|---|---|---|
| Max k (candidates requested) | 10,000 | Hard limit in API |
| Max result set size per query | 1 GB | Soft limit; warns operator |
| Max shards | 1,000 | Soft limit; documented |
| Max concurrent streams | 10,000 per node | Per node limit |
| Max query expansion depth | 5 expansions per query | Configurable; default=5 |
| Max facet cardinality | 100,000 values | Soft limit; graceful degrade |

## Operational Procedures

### Reranker Model Rollback (LLM Unavailability)
- **Trigger:** LLM backend unavailable, timeout, or error
- **Action:** Automatic fallback to base hybrid ranking (no manual intervention needed)
- **Effect:** SearchStats.rerank_fallback = true; transparent to caller
- **Performance Impact:** Negligible (≤ 5 ms overhead vs LLM path)

### Query Expansion Tuning
- **Default:** 5 expansions per query
- **Tuning:** Modify `expansion_limit` per workload (reduce for latency-sensitive, increase for recall-sensitive)
- **Monitoring:** Track SearchStats.primary_error_code == 0x3000 for limit-exceeded events

### SearchStats Degradation Flag Interpretation

| Flag | Meaning | Action |
|---|---|---|
| `partial_result=true` | One or more layers failed but results returned | Check `primary_error_code` for root cause |
| `fusion_failed=true` | RRF failed; using base ranking | Investigate index quality; consider re-tuning weights |
| `rerank_fallback=true` | LLM unavailable; results unranked | LLM backend health check; not user-visible |
| `merge_underflow=true` | Merged result count < k | Normal; not all shards have k results |
| `high_overlap_variance=true` | > 50% documents duplicated across shards | May indicate skewed index distribution |

### Monitoring and Alerting
- **Alert on:** `primary_error_code != 0x0000` in >1% of queries
- **Alert on:** p99 latency > 16.5 ms (SRCP-1 gate) for 5+ consecutive samples
- **Alert on:** Merge throughput < 45K results/sec (SRCP-2 gate)
- **Runbook:** See `docs/operations/search_runbook.md`

## Review and Verification

### Verification Checklist (Pre-Deployment)
- [x] Phase 1: API contracts frozen (v2.0.0)
- [x] Phase 2: Core implementation hardened
- [x] Phase 3: Error handling patterns implemented
- [x] Phase 4: 128+ tests passing
- [x] Phase 5: All SRCP gates ≥ baseline
- [x] Phase 6: 100% Doxygen coverage
- [x] SLA targets validated on reference hardware
- [x] Security/pentest evidence complete (no new CRITICAL findings)

### Affected Components (14 total)
1. hybrid_search.cpp/h — Core hybrid retrieval + fusion
2. distributed_hybrid_search.cpp/h — Distributed merge
3. search_result_stream.cpp/h — Streaming output
4. query_expander.cpp/h — Query expansion
5. faceted_search.cpp/h — Facet counts
6. llm_reranker.cpp/h — LLM reranking
7. learning_to_rank.cpp/h — LTR model scoring
8. multi_modal_search.cpp/h — Text + image fusion
9. neural_sparse_retrieval.cpp/h — Neural sparse retrieval
10. search_analytics.cpp/h — Stats and diagnostics
11. autocomplete.cpp/h — Autocomplete suggestions
12. conversational_search.cpp/h — Conversational context
13. cross_lingual_search.cpp/h — Multi-language search
14. fuzzy_matcher.cpp/h — Fuzzy matching
