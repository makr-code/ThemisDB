# Architecture - Cache Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot Coding Agent | Human_Review: false | Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone. -->

## Overview

The cache module composes query and embedding cache behaviors into bounded, observable runtime layers. It provides multi-tier data reuse with tenant isolation, invalidation/warmup flows, and distributed coordination for multi-node scenarios.

## Main Execution Planes

1. Multi-tier cache plane
- adaptive query cache through L1/L2/L3 style layers
- embedding and semantic cache execution paths

2. Isolation and policy plane
- tenant-aware keying and quota-aware runtime checks
- invalidation, circuit behavior, and bounded failure semantics

3. Coordination and replication plane
- in-process and remote coordination support
- replication event distribution and consistency hooks

4. Operations and observability plane
- warmup and predictive prefetch paths
- SLO monitoring and runtime metrics surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| cache facade interfaces | provide get/put/invalidate style cache semantics |
| tenant-aware interfaces | apply tenant context and quotas where supported and configured; isolation is not universal across cache classes |
| coordination interfaces | publish entries and invalidation events through the selected coordinator; remote delivery can be asynchronous or partial |
| observability interfaces | expose health/SLO and operational counters |

## Failure Semantics

- `cache_contract.h` defines canonical failure classes and fail-closed classification. Individual cache APIs still expose different result forms (`optional`, `bool`, counts, or diagnostic structures); do not assume every API returns a shared error envelope.
- Coordinator diagnostics expose connection and delivery state. A successful local cache operation is not proof of remote delivery.
- Warmup reports loaded, skipped, and failed records through `WarmupResult`; callers must inspect that result rather than treating invocation as complete success.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` (hashing, time, concurrency helpers) | Cache key hashing, TTL computation, and internal locking primitives |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/cache/semantic_cache.h`, `include/cache/adaptive_query_cache.h`, `include/cache/cache_hit_rate_slo_monitor.h` | Semantic result reuse and SLO monitoring for HTTP query responses |
| `query` | `include/cache/adaptive_query_cache.h` (used via `include/query/query_cache_manager.h`; workload strategy is declared in `include/query/workload_cache_strategy.h`) | Query result reuse and workload-adaptive strategy integration |
| `llm` | `include/cache/embedding_cache.h` | `LLMPrefixCache` uses `EmbeddingCache` for prefix-vector similarity lookup; embedding generation remains outside this cache module |
| `sharding` | `include/cache/bounded_lru_cache.h` | Bounded LRU shard-routing metadata cache for consistent-hash ring lookups |
| `content` | `include/cache/bounded_lru_cache.h` | `deduplication_checker` uses bounded LRU for fingerprint deduplication (`include/content/deduplication_checker.h:20`) |

## Runtime Data Flow and Boundaries

- **Adaptive query cache:** callers create a fingerprint and pass it with query parameters and tenant context to `get()` / `put()`. Reads search the available L1/L2/L3 paths; L3 is absent when its path is empty or initialization fails. `enable_write_through` is opt-in. When a coordinator is configured, entry and invalidation events are published separately from local cache operations.
- **Semantic cache:** `query(prompt, params)` and `put(prompt, params, response, metadata, ttl)` use a key derived from SHA-256 of the prompt concatenated with serialized parameters and access the caller-supplied RocksDB `TransactionDB` and optional column family. This is an exact prompt/parameter key, not embedding-similarity lookup; the database and column-family handles are borrowed.
- **Embedding cache:** callers supply the embedding vector. A configured vector index searches one nearest item and applies the configured similarity threshold; if no index is active, lookup scans the in-memory entries. The cache does not generate embeddings or establish application-level semantic equivalence.
- **SLO monitor:** `evaluate(CacheMetrics)` evaluates supplied metrics; `recordLatency()` and `recordTenantEvictionRate()` are separate reporting calls. The monitor is not an implicit hook on every cache operation.

The cache owns reuse, expiry, tier placement, and cache-specific diagnostics. Callers and data-owning modules remain responsible for authorization, freshness, and ordering invalidations with writes.



- Verified files:
  - src/cache/adaptive_query_cache.cpp
  - src/cache/embedding_cache.cpp
  - src/cache/semantic_cache.cpp
  - src/cache/cache_replication.cpp
  - src/cache/distributed_cache_coordinator.cpp
  - src/cache/redis_cache_coordinator.cpp
  - src/cache/warmup.cpp
  - src/cache/cache_hit_rate_slo_monitor.cpp
- Verified architecture claims:
  - explicit multi-tier, isolation, coordination, and observability planes
  - bounded error behavior around distributed and warmup paths
  - dedicated cache-layer composition for query/embedding reuse