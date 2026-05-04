# Cache-Modul — Übersicht

**Stand:** 2026-03-09
**Version:** v1.5.0
**Status:** ✅ Production Ready
**Kategorie:** 🗄️ Caching

---

## 📑 Inhaltsverzeichnis

- [Zweck](#zweck)
- [Architektur](#architektur)
- [Hauptkomponenten](#hauptkomponenten)
- [Konfiguration](#konfiguration)
- [Schnellstart](#schnellstart)
- [Primärdokumentation](#primärdokumentation)
- [Sekundärdokumentation](#sekundärdokumentation)

---

## Zweck

Das Cache-Modul implementiert einen **mehrschichtigen, adaptiven Abfrageergebnis-Cache** für ThemisDB. Es sitzt zwischen der Query-Engine und der Storage-Schicht und bedient wiederholte oder semantisch ähnliche Abfragen aus schnellem In-Memory- oder komprimiertem Speicher, ohne RocksDB zu belasten.

Hauptziele:
- Reduzierung der Datenbanklatenz für heiße Abfragen
- Kostensenkung durch Wiederverwendung von LLM-Einbettungsergebnissen
- Tenant-isolierter, sicherheitsbewusster Cache-Betrieb
- DSGVO-konforme Datenlöschung (Art. 17)

---

## Architektur

Der Cache ist in drei Stufen aufgebaut:

```
Query Engine (src/query/)
        │
        ▼
AdaptiveQueryCache (Hauptfassade)
  ┌─────────────────────────────────────┐
  │  SemanticCache: embedding-Ähnlichkeit│
  └─────────┬───────────────────────────┘
            │
  ┌─────────▼──────┐  ┌──────────────┐  ┌──────────────────┐
  │  L1: BoundedLRU│→ │ L2: Zstd/LZ4 │→ │ L3: RocksDB      │
  │  (In-Memory)   │  │ (komprimiert) │  │ (Circuit Breaker) │
  └────────────────┘  └──────────────┘  └──────────────────┘
  Token-Bucket Rate Limiter │ Tenant-Namespace-Enforcer
```

| Tier | Kapazität | TTL (Standard) | Technologie |
|------|-----------|----------------|-------------|
| L1 (HOT) | 10.000 Einträge, max 1 KB/Eintrag | 5 min | In-Memory HashMap + LRU |
| L2 (WARM) | 50.000 Einträge, max 10 KB/Eintrag | 30 min | Komprimierter In-Memory (Zstd/LZ4) |
| L3 (COLD) | unbegrenzt (RocksDB) | 24 h | RocksDB mit Circuit Breaker |

---

## Hauptkomponenten

| Datei | Rolle |
|-------|-------|
| `adaptive_query_cache.h/cpp` | Hauptfassade: L1→L2→L3 Lookup- und Schreib-Pipeline |
| `bounded_lru_cache.h/cpp` | L1 In-Memory LRU-Cache mit Per-Entry-Größenlimits |
| `semantic_cache.h/cpp` | Vektorähnlichkeitsbasierter Cache-Lookup für fast identische Abfragen |
| `embedding_cache.h/cpp` | Dedizierter Cache für LLM-Einbettungsvektoren |
| `arc_cache.h` | Adaptive Replacement Cache (ARC) Eviction Policy |
| `eviction_policy.h` | Eviction-Policy-Enum (LRU, LFU, ARC) |
| `warmup.cpp` | Bulk-Cache-Warmup aus Query-Log-Snapshots |
| `predictive_prefetcher.h/cpp` | Prädiktives Prefetching basierend auf Query-Sequenzhistorie |
| `cache_hit_rate_slo_monitor.h/cpp` | Cache-Trefferquoten-SLO-Alerting und Monitoring |
| `cache_replication.h/cpp` | Cache-Replikations-Event-Handler für HA-Deployments |
| `cache_replication_coordinator.h/cpp` | In-Process Cache-Replikationskoordination |
| `distributed_cache_coordinator.h/cpp` | Verteilte Cache-Koordination (Node-lokaler Bus) |
| `redis_cache_coordinator.h/cpp` | Redis pub/sub-basierter verteilter Cache-Koordinator |
| `cache_metrics.h` | Thread-sichere Metriken-Sammlung (atomare Zähler) |

---

## Konfiguration

```cpp
AdaptiveQueryCache::Config config;

// L1/L2/L3-Grundkonfiguration
config.l1_max_entries = 10000;
config.l2_max_entries = 50000;
config.l3_db_path = "./themis_query_cache";

// Phase 1: Circuit Breaker
config.enable_circuit_breaker = true;
config.cb_failure_threshold = 5;   // OPEN nach 5 Fehlern
config.cb_timeout_ms = 60000;      // 1 Minute Timeout

// Phase 2: Rate Limiting und Tenant-Isolation
config.enable_rate_limiting = true;
config.max_requests_per_second = 10000;
config.enable_tenant_isolation = true;
config.per_tenant_max_bytes = 104857600;  // 100 MB pro Tenant

// Phase 4: Write-Through und Adaptive TTL
config.enable_write_through = false;  // opt-in
config.enable_adaptive_ttl = false;   // opt-in
config.adaptive_ttl_min_seconds = 60;
config.adaptive_ttl_max_seconds = 86400;

AdaptiveQueryCache cache(config);
```

---

## Schnellstart

```cpp
// Cache-Eintrag speichern
std::string fp = cache.generateFingerprint(query, params, tenant_id);
cache.put(fp, params, result, tenant_id);

// Cache-Eintrag abfragen
auto cached = cache.get(fp, tenant_id);
if (cached) {
    return *cached;  // Cache-Treffer
}

// DSGVO-Löschung
cache.invalidatePII("user-pii-uuid-1234");

// Admin: Gesundheitsstatus
nlohmann::json health = cache.getHealthStatus();
// → {"healthy": true, "l1": {...}, "l2": {...}, "l3": {...}, "circuit_breaker": "CLOSED"}
```

---

## Primärdokumentation

Die maßgebliche Entwicklerdokumentation befindet sich in `src/cache/` und `include/cache/`:

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| **README** | [`src/cache/README.md`](../../../../src/cache/README.md) | Modulübersicht, Konfigurationsreferenz, Phasenbeschreibungen |
| **Architektur** | [`src/cache/ARCHITECTURE.md`](../../../../src/cache/ARCHITECTURE.md) | Komponentendiagramm, Datenflüsse, Threading, Sicherheit |
| **Roadmap** | [`src/cache/ROADMAP.md`](../../../../src/cache/ROADMAP.md) | Implementierungsstatus, bekannte Einschränkungen |
| **Erweiterungen** | [`src/cache/FUTURE_ENHANCEMENTS.md`](../../../../src/cache/FUTURE_ENHANCEMENTS.md) | Geplante Features, Interface-Designs |
| **Header-README** | [`include/cache/README.md`](../../../../include/cache/README.md) | Übersicht der öffentlichen Header-Schnittstellen |

---

## Sekundärdokumentation

Erklärende Dokumentation und themenspezifische Guides in `docs/de/`:

| Dokument | Pfad | Inhalt |
|----------|------|--------|
| **Semantic Cache** (Impl.) | [`docs/de/src/cache/semantic_cache.cpp.md`](semantic_cache.cpp.md) | SemanticCache-Klassenreferenz, Methoden, Serialisierungsformat |
| **Semantic Cache** (Feature) | [`docs/de/features/features_semantic_cache.md`](../../features/features_semantic_cache.md) | Nutzerdokumentation, HTTP-API, Performance-Benchmarks |
| **Cache-Invalidierung** | [`docs/de/architecture/architecture_cache_invalidation.md`](../../architecture/architecture_cache_invalidation.md) | Invalidierungsstrategien, GDPR-Löschung, verteilte Invalidierung |
| **Caching-Pattern-Katalog** | [`docs/de/architecture/architecture_caching_patterns.md`](../../architecture/architecture_caching_patterns.md) | Lookup-Muster und zugehörige Caching-Hooks |
| **Cache-Roadmap** | [`docs/cache_roadmap.md`](../../roadmap/cache_roadmap.md) | Umfassende Cache-Roadmap (Legacy) |
| **Fehlende Implementierungen** | [`docs/de/src/cache/MISSING_IMPLEMENTATIONS.md`](MISSING_IMPLEMENTATIONS.md) | Reality-Check-Ergebnisse: nicht implementierte / teilweise umgesetzte Punkte |
