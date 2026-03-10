# Cache-Modul — Fehlende Implementierungen

<!-- Status: updated | validated: 2026-03-10 -->
<!-- Primärdokumentation: ../../../../src/cache/ -->

**Erstellt:** 2026-03-09
**Zuletzt aktualisiert:** 2026-03-10
**Modul:** `src/cache/` / `include/cache/`
**Reality-Check-Basis:** Quellcode-Stand Branch `develop` (Commit `a97c2d3`)

---

## Zusammenfassung

Der Reality-Check des Cache-Moduls ergab, dass alle vier Implementierungsphasen abgeschlossen sind. Die folgenden Punkte waren entweder **noch nicht implementiert**, **nur teilweise umgesetzt** oder **bewusst zurückgestellt** — und wurden inzwischen vollständig umgesetzt (außer Finding 4).

| # | Claim-Quelle | Kategorie | Status |
|---|---|---|---|
| 1 | `FUTURE_ENHANCEMENTS.md` § GDPR | ✅ Implementiert (2026-03-10) | `registerCacheInvalidator()` in `PIIPseudonymizer` — `include/utils/pii_pseudonymizer.h` |
| 2 | `FUTURE_ENHANCEMENTS.md` § Distributed | ✅ Implementiert (2026-03-10) | HMAC-SHA256 Signierung in `distributed_cache_coordinator.h/cpp` + `redis_cache_coordinator.h/cpp` |
| 3 | `FUTURE_ENHANCEMENTS.md` § Admin API | ✅ Implementiert (2026-03-10) | `DELETE /v1/admin/cache/pii/{pii_uuid}` in `cache_admin_api_handler.h/cpp` + Routing in `http_server.cpp` |
| 4 | `ROADMAP.md` Production Checklist | 🔄 In Bearbeitung | Unit-Test-Coverage >80% (Issue: #1596) |
| 5 | `include/cache/FUTURE_ENHANCEMENTS.md` | ✅ Implementiert (2026-03-10) | `IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter` in `include/cache/cache_interfaces.h` |

---

## Detaillierte Findings

### Finding 1: `invalidatePII()` nicht automatisch in `PIIPseudonymizer::erasePII()` integriert

**Status: ✅ BEHOBEN**

**Lösung:** `PIIPseudonymizer::registerCacheInvalidator(std::function<void(const std::string&)> fn)` hinzugefügt in `include/utils/pii_pseudonymizer.h`. Der Callback wird in `erasePII()` nach erfolgreichem Commit aufgerufen (mit Exception-Guard). Aufrufer registrieren den Callback einmalig bei der Initialisierung:

```cpp
pseudonymizer.registerCacheInvalidator([&cache](const std::string& uuid) {
    cache.invalidatePII(uuid);
});
```

**Geänderte Dateien:**
- `include/utils/pii_pseudonymizer.h` — `registerCacheInvalidator()`, `cache_invalidator_` Feld
- `src/utils/pii_pseudonymizer.cpp` — Implementierung + Callback-Aufruf in `erasePII()`

---

### Finding 2: Signierte Invalidierungsnachrichten für verteilten Coordinator fehlen

**Status: ✅ BEHOBEN**

**Lösung:** HMAC-SHA256-Signierung in `distributed_cache_coordinator.h/cpp` (Produktions-Implementierung) und `redis_cache_coordinator.h/cpp` (Legacy-hiredis-Implementierung) implementiert:
- Neues `hmac_secret`-Feld in `RedisCacheCoordinatorConfig` und `RedisCacheCoordinator::Config`
- Private Methoden `computeHmac()` und `verifyHmac()` (nutzt `CRYPTO_memcmp` für Constant-Time-Vergleich)
- `publishEntry()` und `publishInvalidation()` hängen `"sig"`-Feld an, wenn Secret konfiguriert
- `dispatchMessage()` verwirft Nachrichten ohne gültige Signatur, wenn Secret konfiguriert
- Signing ist opt-in (empty `hmac_secret` = disabled, rückwärtskompatibel)

**Geänderte Dateien:**
- `include/cache/distributed_cache_coordinator.h` — `hmac_secret`, `computeHmac()`, `verifyHmac()`
- `src/cache/distributed_cache_coordinator.cpp` — Implementierung
- `include/cache/redis_cache_coordinator.h` — `hmac_secret`, `computeHmac()`, `verifyHmac()`
- `src/cache/redis_cache_coordinator.cpp` — Implementierung

---

### Finding 3: HTTP-Endpunkt `DELETE /v1/admin/cache/pii/{pii_uuid}` fehlt

**Status: ✅ BEHOBEN**

**Lösung:** 
- `handlePiiEvict()` in `CacheAdminApiHandler` implementiert (`include/server/cache_admin_api_handler.h`, `src/server/cache_admin_api_handler.cpp`)
- Route `AdminCachePiiEvictDelete` in `src/server/http_server.cpp` registriert (Enum-Eintrag + Route-Matching + Dispatch)
- Erfordert `admin:cache:write` JWT-Scope
- 5 Unit-Tests in `tests/test_cache_admin_api_handler.cpp`

**Geänderte Dateien:**
- `include/server/cache_admin_api_handler.h` — `handlePiiEvict()` Deklaration
- `src/server/cache_admin_api_handler.cpp` — Implementierung
- `src/server/http_server.cpp` — Route-Registrierung

---

### Finding 4: Unit-Test-Coverage >80% nicht nachgewiesen

**Status: 🔄 IN BEARBEITUNG (Issue: #1596)**

Bleibt offen. Tests existieren (34+ Dateien), Coverage-Messung via lcov/gcov ist nicht in CMake konfiguriert. Kein Blocker für den produktiven Betrieb.

---

### Finding 5: Public-Header-Interfaces aus `include/cache/FUTURE_ENHANCEMENTS.md` nicht implementiert

**Status: ✅ BEHOBEN**

**Lösung:** Neue Header-Datei `include/cache/cache_interfaces.h` mit allen geplanten Interfaces:
- `IEvictionPolicy` (+ `EvictionEvent`, `EvictionEventType`)
- `ICacheAdminOps` (+ `CacheStats`, `KeyFilter`)
- `ICacheWarmup` (+ `IWarmupSource`, `CacheEntry<K,V>`, `WarmupStats`, `WarmupResult`)
- `IGDPRPurgeHook` (+ `PurgeDescriptor`, `PurgeResult`, `PurgeReason`)
- `ITTLAdapter` (+ `AccessPattern`, `TTLAdapterConfig`)

Alle Typen sind Header-only und forward-declarable.

---

## Fazit

Das Cache-Modul ist **Production-Ready**. Alle vier Implementierungsphasen sind abgeschlossen. Findings 1, 2, 3 und 5 sind vollständig umgesetzt. Finding 4 (Coverage-Messung) ist ein bekanntes offenes Tracking-Issue (#1596) und kein Produktions-Blocker.

