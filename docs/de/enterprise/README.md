# ThemisDB Enterprise Features

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🏢 Enterprise

---

## 📑 Inhaltsverzeichnis

- [Quick Links](#-quick-links)
- [Features](#features)
- [Editions](#editions)

---

## 🎯 Quick Links

- **[Feature Analysis & Strategy](ENTERPRISE_FEATURE_ANALYSIS.md)** - Complete analysis of enterprise vs community features
- **[Build Guide](ENTERPRISE_BUILD_GUIDE.md)** - How to build enterprise DLL modules
- **[Edition Comparison](EDITION_COMPARISON.md)** - Feature matrix comparing Community/Professional/Enterprise
- **[Implementation Status](enterprise_implementation.md)** - Current implementation details (legacy)

---

## 🆕 New: Modular Enterprise Architecture (December 2025)

ThemisDB now supports a **modular enterprise architecture** where advanced features are distributed as separate DLLs/shared libraries. This enables:

- **Clear separation** between community (free) and enterprise (licensed) features
- **Flexible licensing** - pay only for the modules you need
- **Easy upgrades** - add enterprise modules without rebuilding the core
- **Maintainability** - independent module development and testing

### Enterprise Modules (7 DLLs)

1. **Sharding** - Horizontal scaling, consistent hashing, cross-shard joins
2. **GPU** - CUDA, Vulkan, HIP, DirectX GPU acceleration
3. **Analytics** - OLAP (CUBE/ROLLUP), CEP streaming, Arrow integration
4. **Replication** - Leader-follower, multi-master, CRDTs, geo-replication
5. **Security** - RBAC, HSM, field encryption, enhanced audit logging
6. **Management** - Multi-tenancy, rate limiting, load shedding, admin tools
7. **Content** - PDF, video, audio, geo, CAD, image processors

---

## Übersicht

ThemisDB Enterprise bietet erweiterte Skalierbarkeits- und Performance-Features für unternehmenskritische Deployments mit hohem Durchsatz und strengen SLA-Anforderungen.

## Kern-Features

### 1. Advanced Rate Limiting
- **Token Bucket Rate Limiter** mit Priority Lanes (HIGH/NORMAL/LOW)
- **Per-Client Rate Limiter** mit individuellen Quotas
- Burst-Handling (10k+ requests/sec)
- Thread-safe Atomic Operations

📖 [Detaillierte Dokumentation](../ENTERPRISE_SCALABILITY.md#1-token-bucket-rate-limiter)

### 2. Adaptive Load Shedding
- Multi-Metric Monitoring (CPU, Memory, Queue Depth)
- Gewichtete Schwellwerte (CPU: 50%, Memory: 30%, Queue: 20%)
- Graceful Degradation statt Hard Failures
- HTTP 503 Service Unavailable mit Retry-After Header

📖 [Detaillierte Dokumentation](../ENTERPRISE_SCALABILITY.md#3-load-shedder)

### 3. HTTP Connection Pooling
- Boost.Beast HTTP/HTTPS Client
- SSL/TLS 1.2+ Support mit Peer Verification
- Connection Reuse & Pooling
- Async Requests (std::future)
- Konfigurierbare Timeouts

📖 [HTTP Client Pool Complete](../HTTP_CLIENT_POOL_COMPLETE.md)

### 4. Batch Operations
- Atomic Batch CRUD (`/entities/batch`)
- RocksDB WriteBatch Integration
- Bulk Insert/Update/Delete
- Transaction Semantics (all-or-nothing)

📖 [Batch Operations](../ENTERPRISE_SCALABILITY.md#batch-operations)

## Implementierungsstatus

| Feature | Status | Tests | Dokumentation |
|---------|--------|-------|---------------|
| Token Bucket Rate Limiter | ✅ Produktiv | 5/5 | ✅ |
| Per-Client Rate Limiter | ✅ Produktiv | 3/3 | ✅ |
| Load Shedder | ✅ Produktiv | 5/5 | ✅ |
| HTTP Client Pool | ✅ Produktiv | 6/6 | ✅ |
| Batch Operations | ✅ Produktiv | 1/1 | ✅ |

**Test Coverage:** 20/20 Tests (100%)

## Koexistenz mit Legacy-Code

Die Enterprise-Features koexistieren sicher mit der bestehenden Implementation:

- **Alte Rate Limiter** (`rate_limiter.h`): Weiterhin in Production aktiv
- **Neue Enterprise Features** (`rate_limiter_v2.h`): Verfügbar, nicht aktiv
- **Keine Konflikte**: Separate Namespaces, unterschiedliche Klassennamen
- **Migration**: Feature-Flag-basiert, graduell über 2-3 Monate

📋 [Integration Analysis](../reports/INTEGRATION_ANALYSIS.md)

## Build & Deployment

### Voraussetzungen
- MSVC 2022 Professional (14.44+)
- CMake 3.20+
- vcpkg (Boost, OpenSSL)
- Ninja Build System

### Quick Start

```powershell
# 1. Umgebung vorbereiten
.\setup.ps1

# 2. Build mit Enterprise Features
.\scripts\build_enterprise.cmd

# 3. Enterprise Tests ausführen
build-msvc-ninja-debug\themis_tests.exe --gtest_filter="*Enterprise*"
```

📖 [Enterprise Build Guide](../ENTERPRISE_BUILD_GUIDE.md)

## Architektur

### Rate Limiter v2 Architektur

```
┌─────────────────────────────────────────────────────┐
│              HTTP Request                            │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│         PerClientRateLimiter                         │
│  ┌──────────────────────────────────────────────┐  │
│  │ Client ID → TokenBucketRateLimiter           │  │
│  │                                               │  │
│  │ Priority Lanes:                               │  │
│  │  ┌────────┐ ┌────────┐ ┌────────┐           │  │
│  │  │  HIGH  │ │ NORMAL │ │  LOW   │           │  │
│  │  │  50%   │ │  30%   │ │  20%   │           │  │
│  │  └────────┘ └────────┘ └────────┘           │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│              LoadShedder                             │
│  ┌──────────────────────────────────────────────┐  │
│  │ Metrics:                                      │  │
│  │  • CPU Usage      (50% weight)               │  │
│  │  • Memory Usage   (30% weight)               │  │
│  │  • Queue Depth    (20% weight)               │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────┐
│          Request Processing                          │
└─────────────────────────────────────────────────────┘
```

### HTTP Client Pool Architektur

```
┌─────────────────────────────────────────────────────┐
│         HTTPClientPool                               │
│  ┌──────────────────────────────────────────────┐  │
│  │ Connection Pool (max_connections)            │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐                 │  │
│  │  │Conn 1│ │Conn 2│ │Conn 3│ ...              │  │
│  │  └──────┘ └──────┘ └──────┘                 │  │
│  │                                               │  │
│  │ BeastHTTPClient (Boost.Beast):               │  │
│  │  • HTTP/HTTPS                                │  │
│  │  • SSL/TLS Context                           │  │
│  │  • SNI Support                               │  │
│  │  • Connection Reuse                          │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────┬───────────────────────────────┘
                      │
                      ▼ std::async
┌─────────────────────────────────────────────────────┐
│           External API (HTTPS)                       │
│  • Embedding Services                                │
│  • Classification APIs                               │
│  • Content Analysis                                  │
└─────────────────────────────────────────────────────┘
```

## Performance Targets

| Metric | Target | Aktuell | Status |
|--------|--------|---------|--------|
| Max Requests/s | 50,000 | ~6,500 (Enterprise) | 🟡 In Arbeit |
| Latenz (p50) | <10ms | ~150ns (Rate Limiter) | ✅ |
| Latenz (p99) | <50ms | ~800ns (Rate Limiter) | ✅ |
| Memory Overhead | <2 MB @ 10k clients | 1.9 MB | ✅ |
| CPU Overhead | <5% | ~1-2% | ✅ |

## Dokumentationsübersicht

### User Guides
- [Enterprise Scalability Übersicht](../ENTERPRISE_SCALABILITY.md) - Feature-Beschreibungen mit Code-Beispielen
- [HTTP Client Pool Complete](../HTTP_CLIENT_POOL_COMPLETE.md) - HTTP Client Implementierung
- [Enterprise Build Guide](../ENTERPRISE_BUILD_GUIDE.md) - Build-Anleitung und Deployment

### Architektur & Strategie
- [Enterprise Scalability Strategy](../performance/ENTERPRISE_SCALABILITY_STRATEGY.md) - Ursprüngliche Strategie und Design
- [Integration Analysis](../reports/INTEGRATION_ANALYSIS.md) - Koexistenz mit Legacy-Code

### Status & Reports
- [Enterprise Implementation Status](../ENTERPRISE_IMPLEMENTATION_STATUS.md) - Detaillierter Implementierungs-Status
- [Enterprise Final Report](../ENTERPRISE_FINAL_REPORT.md) - Abschlussbericht mit Statistiken

## Migration Path

### Option A: Feature-Flag (EMPFOHLEN)

```cpp
// http_server.h
class HTTPServer {
    std::unique_ptr<RateLimiter> legacy_rate_limiter_;
    std::unique_ptr<PerClientRateLimiter> enterprise_rate_limiter_;
    bool use_enterprise_features_;  // Config-gesteuert
};
```

**Config:**
```json
{
  "enterprise": {
    "enabled": true,
    "rate_limiting": {
      "mode": "enterprise",
      "tokens_per_second": 1000,
      "bucket_capacity": 5000
    },
    "load_shedding": {
      "enabled": true,
      "cpu_threshold": 0.8,
      "memory_threshold": 0.85
    }
  }
}
```

### Rollout-Plan

**Phase 1 (Wochen 1-2):**
- Feature-Flag implementieren
- Monitoring-Dashboard aufsetzen
- Staging-Tests

**Phase 2 (Wochen 3-4):**
- 10% Production Traffic auf Enterprise
- Metriken-Analyse
- Fehler-Monitoring

**Phase 3 (Wochen 5-8):**
- Graduell auf 100% erhöhen
- Performance-Tuning
- Load-Tests

**Phase 4 (Wochen 9-12):**
- Legacy-Code deprecaten
- Dokumentation aktualisieren
- Migration abschließen

## Support & Troubleshooting

### Häufige Probleme

**Problem:** Build schlägt fehl mit "Boost not found"
```powershell
# Lösung: vcpkg installieren
vcpkg install boost-beast:x64-windows
```

**Problem:** Tests hängen bei HTTP Client Pool
```powershell
# Lösung: Network-Tests überspringen
build-msvc-ninja-debug\themis_tests.exe --gtest_filter="-*HTTPClientPool*"
```

**Problem:** Rate Limiter lässt keine Requests durch
```cpp
// Lösung: Config prüfen
config.capacity = 10000;  // Nicht 0!
config.refill_rate = 1000.0;  // Nicht 0.0!
```

### Logs & Debugging

```cpp
// Debug-Logging aktivieren
#define THEMIS_DEBUG_RATE_LIMITER 1

// Logs:
THEMIS_DEBUG("TokenBucketRateLimiter: Acquired {} tokens (prio={})", tokens, prio);
THEMIS_WARN("PerClientRateLimiter: Max clients reached");
THEMIS_ERROR("LoadShedder: CPU threshold exceeded: {:.1f}%", cpu * 100);
```

## Roadmap

### Phase 2 (Q1 2026)
- [ ] Distributed Rate Limiting (Redis)
- [ ] Circuit Breaker Pattern
- [ ] Request Replay Queue
- [ ] Advanced Metrics (Prometheus)

### Phase 3 (Q2 2026)
- [ ] GraphQL Batch Support
- [ ] gRPC Support
- [ ] Multi-Region Load Balancing
- [ ] Auto-Scaling Integration

### Phase 4 (Q3 2026)
- [ ] AI-basierte Load Prediction
- [ ] Dynamic Quota Adjustment
- [ ] Cost-based Request Routing

## Lizenz

ThemisDB Enterprise Features sind Teil von ThemisDB und unterliegen der gleichen Lizenz.

## Kontakt

- **Issues:** GitHub Issues
- **Fragen:** Diskussionen im GitHub Repo
- **Enterprise Support:** service@themisdb.org

---

**Version:** 1.0  
**Letzte Aktualisierung:** 30. November 2025  
**Status:** ✅ Production Ready
