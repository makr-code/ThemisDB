# Cache-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/cache/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Caching / Performance  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Cache-Modul implementiert mehrstufiges, adaptives Query-Result-Caching für ThemisDB mit semantisch-bewussten Lookups, Circuit-Breaker-Fehlerisolierung, Tenant-Isolation und konfigurierbarem Rate-Limiting.

**Primäre Quelle:** [`src/cache/`](../../../src/cache/) · [`include/cache/`](../../../include/cache/)

---

## Kernkomponenten

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AdaptiveQueryCache | `adaptive_query_cache.h` | `adaptive_query_cache.cpp` | Haupt-Cache-Fassade (L1/L2/L3-Pipeline) |
| BoundedLruCache | `bounded_lru_cache.h` | `bounded_lru_cache.cpp` | L1 In-Memory-LRU-Cache |
| EmbeddingCache | `embedding_cache.h` | `embedding_cache.cpp` | Embedding-Ergebnis-Cache |
| CacheReplication | `cache_replication.h` | `cache_replication.cpp` | Replikation für HA-Multi-Node-Deployments |
| DistributedCacheCoordinator | `distributed_cache_coordinator.h` | `distributed_cache_coordinator.cpp` | Verteilte Cache-Koordination |
| CacheHitRateSLOMonitor | `cache_hit_rate_slo_monitor.h` | `cache_hit_rate_slo_monitor.cpp` | Cache-Hit-Rate-SLO-Alerting |
| PredictivePrefetcher | *(cache_provider.h)* | `predictive_prefetcher.cpp` | Prädiktives Pre-Fetching |
| GrpcRemoteCachePeer | `grpc_remote_cache_peer.h` | `grpc_remote_cache_peer.cpp` | gRPC-basierte Remote-Cache-Peer-Kommunikation |

---

## Cache-Hierarchie

- **L1**: In-Memory-LRU (ultra-schnell, begrenzte Kapazität)
- **L2**: Komprimierter Cache (höhere Kapazität, geringfügig langsamer)
- **L3**: RocksDB-backed Persistenz-Cache (Überlauf, disk-basiert)

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/cache/README.md`](../../../src/cache/README.md) | Modulübersicht und Interfaces |
| [`src/cache/ARCHITECTURE.md`](../../../src/cache/ARCHITECTURE.md) | Systemarchitektur |
| [`src/cache/ROADMAP.md`](../../../src/cache/ROADMAP.md) | Feature-Roadmap |
