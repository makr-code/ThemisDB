# Unified Access Model Architecture

**Version:** 1.0  
**Status:** Frozen (v1.x)  
**Validated:** 2026-08-03  
**Links:** [`CACHE_STORAGE_INTEGRATION.md`](./CACHE_STORAGE_INTEGRATION.md) · [`src/access_model/`](../../src/access_model/README.md) · [`include/access_model/`](../../include/access_model/README.md)

---

## 1. Overview

ThemisDB's access model unifies two previously isolated systems:
1. **Cache Layer** — In-memory, hierarchical working/episodic/semantic memory (L1/L2/L3)
2. **Storage Layer** — Persistent, tiered media across NVMe/HDD/Object-Store (hot/warm/cold)

This document defines the **unified access tier abstraction** that bridges both systems with clear separation of concerns, enabling automatic promotion/demotion across tiers while preserving backward compatibility.

### Design Principles

| Principle | Rationale |
|-----------|-----------|
| **Single Responsibility** | Cache manages memory; Storage manages persistence; Coordinator manages transitions |
| **Dependency Inversion** | Both use abstract `AccessTier` interface; no direct cache↔storage coupling |
| **Observability** | Every transition logged with correlation ID; metrics exposed uniformly |
| **Backward Compatibility** | New coordinator is opt-in; existing APIs unchanged |
| **Testability** | Mock `AccessCoordinator` for unit tests; deterministic integration tests |

---

## 2. Canonical Terminology

### Access Tier (Generic)
An abstraction representing any storage/memory location in ThemisDB's hierarchy.
- **Property:** Location-agnostic, defined by behavior contract (not physical medium)
- **Examples:** L1 (in-memory, fast), Warm (HDD-backed), Cold (S3/archive)

### Cache Tier (In-Memory)
A tier within the cache layer, managing working/episodic/semantic memory.

| Tier | Name | Latency | Capacity | Typical Content | Purpose |
|------|------|---------|----------|-----------------|---------|
| **L1** | Working Memory | <1µs | 1-100 MB | Active queries, CTEs, hot joins | Fast query execution |
| **L2** | Episodic Memory | 1-10µs | 100 MB-1 GB | Query results, intermediate tables, session state | Reduce re-computation |
| **L3** | Semantic Memory | 10-100µs | 1-10 GB | Vector indices, RAG embeddings, learned patterns | Knowledge retrieval |
| **L4** | Procedural Memory | variable | N/A (LoRA) | Fine-tuned adapters, LoRA weights | Model customization |

### Storage Tier (Persistent)
A tier within the storage layer, mapping to physical media.

| Tier | Medium | Latency | Capacity | Typical Content | Durability |
|------|--------|---------|----------|-----------------|------------|
| **Hot** | NVMe SSD | 1-10 ms | 0.1-2 TB | Recent writes, active datasets, MVCC versions | Replicated |
| **Warm** | HDD / Warm SSD | 10-100 ms | 2-100 TB | Aged data, historical datasets, PITR snapshots | Replicated |
| **Cold** | S3/GCS/Azure | 100 ms - 1 s | Unlimited | Archives, compliance holds, disaster recovery | Multi-region |

---

## 3. Access Model Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│  APPLICATION LAYER                                              │
│  (Query Engine, LLM, Analytics, etc.)                           │
└────────────┬────────────────────────────────────────────────────┘
             │
┌────────────┴─────────────────────────────────────────┐
│  AccessCoordinator (Broker)                          │
│  ┌─────────────────────────────────────────────────┐ │
│  │ • Promotion/Demotion Orchestration              │ │
│  │ • L1↔L2 cache tier transitions                  │ │
│  │ • L3↔warm storage promotion                     │ │
│  │ • Age-based policy enforcement                  │ │
│  │ • Eviction feedback loops                       │ │
│  └─────────────────────────────────────────────────┘ │
└────────┬─────────────────────────────────────────────┘
         │
         ├──────────────────────────┬────────────────────────────┐
         │                          │                            │
┌────────▼────────────┐  ┌──────────▼──────────┐  ┌────────────▼─────────┐
│  Cache Layer        │  │  [Adapter Layer]    │  │  Storage Layer       │
│  ──────────────     │  │  ───────────────    │  │  ──────────────      │
│ ┌──────────────┐   │  │ ┌───────────────┐   │  │ ┌────────────────┐   │
│ │ L1: Working  │   │  │ │ Promotion API │   │  │ │ Hot (NVMe)     │   │
│ │ (Register)   │   │  │ │ Demotion API  │   │  │ │ [RocksDB L0]   │   │
│ │ <1µs, 1MB   │   │  │ │ Metrics       │   │  │ │ 1-10ms, 0.1TB  │   │
│ └──────────────┘   │  │ └───────────────┘   │  │ └────────────────┘   │
│ ┌──────────────┐   │  │                      │  │ ┌────────────────┐   │
│ │ L2: Episodic │   │  │                      │  │ │ Warm (HDD)     │   │
│ │ (Result Set) │   │  │                      │  │ │ [RocksDB L1+]  │   │
│ │ 1-10µs,100MB-1GB│  │                      │  │ │ 10-100ms, 2TB  │   │
│ └──────────────┘   │  │                      │  │ └────────────────┘   │
│ ┌──────────────┐   │  │                      │  │ ┌────────────────┐   │
│ │ L3: Semantic │   │  │                      │  │ │ Cold (S3)      │   │
│ │ (RAG Index)  │   │  │                      │  │ │ [Tiered Mgr]   │   │
│ │ 10-100µs, 10GB│  │  │                      │  │ │ 100ms-1s, ∞    │   │
│ └──────────────┘   │  │                      │  │ └────────────────┘   │
│                    │  │                      │  │                      │
│ L1 Hit Rate:       │  │ Cross-Module         │  │ Data Migration:      │
│ Target: >90%       │  │ Notifications        │  │ Background workers   │
│ Eviction→Storage   │  │ Correlation IDs      │  │ Age + frequency      │
│                    │  │ Tracing              │  │ based policy         │
└────────────────────┘  └──────────────────────┘  └──────────────────────┘
```

### Key Boundaries

1. **Cache ↔ Coordinator:** `EvictionListener` interface
   - Cache notifies coordinator when evicting L1/L2 entries
   - Coordinator decides if data should be promoted from storage

2. **Storage ↔ Coordinator:** `PromotionListener` interface
   - Storage notifies coordinator when accessing cold data
   - Coordinator triggers async promotion to warm tier

3. **Coordinator ↔ Metrics:** `AccessMetrics` interface
   - All tier transitions recorded (correlation ID, latency, size)
   - Cross-tier analytics (L1 eviction latency → warm demotion latency)

---

## 4. Tier Transition State Machine

```
                         CACHE LAYER
            ┌──────────────────────────────────────┐
            │                                       │
          ┌─▼──────┐     Evict                   ┌─▼──────────┐
          │  L1    │──────────────────────────────│   L2       │
          │Active  │                             │  Staging   │
          └─┬──────┘                             └─┬──────────┘
            │                                      │
            │ Evict to Warm Storage                │ Demotion Signal
            │ (if eligible)                        │
            │                                      │
            │                  ┌──────────────────┘
            │                  │
            │                  ▼
      ┌─────┴──────────────────────────────────────┐
      │        STORAGE LAYER (Persistent)         │
      │                                            │
      │  ┌─────────────┐    ┌────────────────┐   │
      │  │   HOT       │    │     WARM       │   │
      │  │ (NVMe L0)   │───▶│  (HDD L1+)     │   │
      │  │ 1-10ms      │    │  10-100ms      │   │
      │  │ 0.1-2TB     │    │  2-100TB       │   │
      │  └─────┬───────┘    └────────┬───────┘   │
      │        │                     │           │
      │        │ Age-based           │ Age-based │
      │        │ Demotion            │ Demotion  │
      │        │ (>30 days)          │ (>90 days)│
      │        │                     │           │
      │        └─────────────┬───────┘           │
      │                      │                   │
      │                      ▼                   │
      │              ┌──────────────┐            │
      │              │    COLD      │            │
      │              │  (S3/GCS)    │            │
      │              │  100ms-1s    │            │
      │              │  Unlimited   │            │
      │              └──────────────┘            │
      └─────────────────────────────────────────┘
```

### Transition Rules

| From | To | Trigger | Latency | Policy |
|------|----|---------|---------| -------|
| L1 | L2 | LRU/LFU eviction | <1ms | Automatic on capacity |
| L2 | Warm | Cache miss recovery | <10ms | Configurable retention |
| L3 | Warm | Semantic cache miss | <50ms | Async prefetch |
| Warm | Cold | Age >90d + no access | async | Background migration |
| Cold | Warm | Access request | 100-500ms | On-demand promotion |
| Warm | Hot | Hot-data detection | <10ms | Predictive promotion |

---

## 5. Backward Compatibility

### For Cache Module
- Existing `AdaptiveQueryCache`, `BoundedLRUCache`, `EmbeddingCache` APIs **unchanged**
- `AccessCoordinator` is **opt-in** via feature flag `THEMISDB_ACCESS_COORDINATOR_ENABLED`
- Default behavior: cache operates independently (no storage feedback)

### For Storage Module
- Existing `TieredStorageManager` APIs **unchanged**
- Migration policies (hot→warm→cold) continue independently
- `AccessCoordinator` callbacks are **optional** listeners (no blocking)

### For New Code
- New applications should register `AccessCoordinator` at startup
- Existing applications continue to work without registration

---

## 6. Configuration

### Core Settings
```ini
[access_model]
# Enable unified access model coordinator
enabled = true
coordinator_thread_pool_size = 4

[cache_tier]
# L1 working memory
l1_max_entries = 10000
l1_promotion_threshold = 10      # Accesses to stay in L1
l1_max_entry_size = 1MB

# L2 episodic memory
l2_max_entries = 100000
l2_promotion_threshold = 5       # Accesses to stay in L2
l2_max_entry_size = 10MB

# L3 semantic memory
l3_max_entries = 1000000
l3_max_entry_size = 100MB

[storage_tier]
# Age-based migration policy
hot_to_warm_days = 30
warm_to_cold_days = 90

# Access-frequency policy
hot_zero_access_days = 14
warm_zero_access_days = 45

# Background worker
migration_check_interval_secs = 3600
max_migrations_per_cycle = 1000

[access_coordinator]
# Promotion/demotion
promote_from_cold_timeout_ms = 100
demote_to_cold_grace_period_secs = 600

# Metrics
enable_correlation_ids = true
correlation_id_sample_rate = 1.0    # 100% sampling by default
```

---

## 7. Example: Data Lifecycle

**Scenario:** User performs analytical query on 50 GB dataset.

```
T=0:      Query enters Q1 (first 100 rows)
          → Results cached in L1 (1 MB)
          → L1 hit rate: 100% (first 100 rows warm)

T=1s:     Query expands to 1000 rows
          → Existing in L1, no storage access
          → L1 hit rate: 100%

T=5m:     Query idle for 5 minutes
          → L1 eviction triggered (LRU)
          → Coordinator: "L1 evicted, notify storage"
          → Storage: Candidate for demotion to warm tier
          
T=10m:    User re-runs same query (pattern detection)
          → L1 miss on first 100 rows
          → Coordinator: Trigger promotion from warm→L1
          → Latency: <50ms (async prefetch)
          → L1 repopulated, hit rate recovered

T=30d:    Data hasn't been accessed in 30 days
          → Age-based demotion: hot→warm initiated
          → Coordinator: Monitor warm-tier growth
          
T=90d:    Data still not accessed (>90 days)
          → Age-based demotion: warm→cold initiated
          → Coordinator: Schedule async migration to S3
          
T=1y:     User requests archived data (cold storage)
          → 500 ms latency (S3 retrieval)
          → Coordinator: Async promotion path initiated
          → Data promoted to warm, then cached in L1 for future access
```

---

## 8. Observability & Diagnostics

### Metrics Exposed

- **L1 Hit Rate** — % of queries served from working memory
- **L2 Hit Rate** — % of queries promoted from episodic memory
- **L3 Hit Rate** — % of RAG queries served from semantic cache
- **Promotion Latency** — Time to move data from cold→warm→L1
- **Demotion Latency** — Time to move data from L1→L2 (cache) or warm→cold (storage)
- **Cross-Tier Correlation** — Latency correlation between cache eviction and storage demotion

### Structured Logging

Every access-model operation is logged:
```json
{
  "timestamp": "2026-08-03T09:18:54Z",
  "correlation_id": "acm-promo-a1b2c3d4e5f6",
  "event": "promotion_complete",
  "from_tier": "cold_storage",
  "to_tier": "warm_storage",
  "data_size_bytes": 1048576,
  "latency_ms": 45,
  "cache_target_tier": "L2",
  "access_pattern": "sequential_read"
}
```

---

## 9. Module Dependencies

```
┌─────────────────────────────────────────┐
│  access_model (NEW)                     │
│  ─────────────────────────────          │
│  • Depends: storage, cache              │
│  • Provides: coordinator, metrics       │
└──────────┬─────────────────────┬────────┘
           │                     │
           ▼                     ▼
┌─────────────────────┐  ┌──────────────────┐
│  cache              │  │  storage         │
│  ─────────────────  │  │  ──────────────  │
│  • Unchanged APIs   │  │  • Unchanged APIs│
│  • Opt-in feedback  │  │  • Opt-in hooks  │
└─────────────────────┘  └──────────────────┘
```

---

## 10. See Also

- [`CACHE_STORAGE_INTEGRATION.md`](./CACHE_STORAGE_INTEGRATION.md) — Integration patterns
- [`src/access_model/ROADMAP.md`](../../src/access_model/ROADMAP.md) — Implementation roadmap
- [`src/cache/ROADMAP.md`](../../src/cache/ROADMAP.md) — Cache module roadmap
- [`src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md) — Storage module roadmap
- [`include/access_model/`](../../include/access_model/README.md) — Public API headers

---

## Revision History

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0 | 2026-08-03 | Copilot | Initial frozen specification |

