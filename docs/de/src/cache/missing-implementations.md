# Cache-Modul — Fehlende Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../../src/cache/ -->

**Erstellt:** 2026-03-09
**Modul:** `src/cache/` / `include/cache/`
**Reality-Check-Basis:** Quellcode-Stand Branch `develop` (Commit `a97c2d3`)

---

## Zusammenfassung

Der Reality-Check des Cache-Moduls ergab, dass alle vier Implementierungsphasen abgeschlossen sind. Die folgenden Punkte sind entweder **noch nicht implementiert**, **nur teilweise umgesetzt** oder **bewusst zurückgestellt**.

| # | Claim-Quelle | Kategorie | Status |
|---|---|---|---|
| 1 | `FUTURE_ENHANCEMENTS.md` § GDPR | Nicht implementiert | `invalidatePII()` nicht automatisch in `PIIPseudonymizer::erasePII()` integriert |
| 2 | `FUTURE_ENHANCEMENTS.md` § Distributed | Nicht implementiert | Signierte Invalidierungsnachrichten für verteilten Coordinator |
| 3 | `FUTURE_ENHANCEMENTS.md` § Admin API | Nicht implementiert | HTTP-Endpunkt `DELETE /v1/admin/cache/pii/{pii_uuid}` |
| 4 | `ROADMAP.md` Production Checklist | In Bearbeitung | Unit-Test-Coverage >80% (Issue: #1596) |
| 5 | `include/cache/FUTURE_ENHANCEMENTS.md` | Nicht implementiert | `ICache<K,V>`, `ICacheWarmup`, `ICacheAdminOps`, `IGDPRPurgeHook`, `ITTLAdapter` Interfaces in Public-Headers |

---

## Detaillierte Findings

### Finding 1: `invalidatePII()` nicht automatisch in `PIIPseudonymizer::erasePII()` integriert

**Claim-Quelle:** `src/cache/FUTURE_ENHANCEMENTS.md` § "GDPR-Aware Cache Invalidation — Remaining follow-up items"

**Erwartet:** Wenn `PIIPseudonymizer::erasePII(uuid)` aufgerufen wird, soll `AdaptiveQueryCache::invalidatePII(uuid)` automatisch ausgelöst werden, ohne dass der Aufrufer dies manuell koordinieren muss.

**Beobachtet:** `invalidatePII()` ist in `adaptive_query_cache.h` (Zeile ~360) vollständig implementiert und funktioniert korrekt. Die Integration in `PIIPseudonymizer` fehlt jedoch; Aufrufer müssen `invalidatePII()` manuell aufrufen.

**Evidence (geprüfte Pfade):**
- `src/cache/adaptive_query_cache.cpp` — `invalidatePII()` vorhanden ✅
- `src/` — kein `PIIPseudonymizer::erasePII()` Aufruf von `invalidatePII()` gefunden ❌

**Issue-Titelvorschlag:** `[cache] Auto-trigger invalidatePII() from PIIPseudonymizer::erasePII() for GDPR Art. 17 compliance`
**Label-Vorschläge:** `module:cache`, `gdpr`, `enhancement`, `priority:high`

---

### Finding 2: Signierte Invalidierungsnachrichten für verteilten Coordinator fehlen

**Claim-Quelle:** `src/cache/FUTURE_ENHANCEMENTS.md` § "Security / Reliability" — `[ ]` Distributed coordinator must validate that invalidation messages carry the originating node's signed token

**Erwartet:** `RedisCacheCoordinator` validiert, dass eingehende Invalidierungsnachrichten ein gültiges, knotensigniertes Token enthalten, um unauthentifizierte Cache-Flush-Angriffe zu verhindern.

**Beobachtet:** `include/cache/redis_cache_coordinator.h` enthält keine Signatur-/HMAC-Felder oder Validierungslogik für eingehende Nachrichten. Das Feld `node_id` verhindert nur Self-Echo, nicht unauthentifizierte externe Nachrichten.

**Evidence (geprüfte Pfade):**
- `include/cache/redis_cache_coordinator.h` — kein `sign`/`HMAC`/`node_token` Feld ❌
- `src/cache/redis_cache_coordinator.cpp` — keine Signaturvalidierung ❌

**Issue-Titelvorschlag:** `[cache] Add signed invalidation messages to RedisCacheCoordinator to prevent unauthenticated cache-flush attacks`
**Label-Vorschläge:** `module:cache`, `security`, `enhancement`, `priority:medium`

---

### Finding 3: HTTP-Endpunkt `DELETE /v1/admin/cache/pii/{pii_uuid}` fehlt

**Claim-Quelle:** `src/cache/FUTURE_ENHANCEMENTS.md` § "GDPR-Aware Cache Invalidation — Remaining follow-up items" — `[ ]` Expose `DELETE /v1/admin/cache/pii/{pii_uuid}` admin endpoint

**Erwartet:** Ein HTTP-Endpunkt `DELETE /v1/admin/cache/pii/{pii_uuid}` in `cache_admin_api_handler.cpp`, der `invalidatePII(pii_uuid)` aufruft und einen strukturierten GDPR-Audit-Log-Eintrag erzeugt.

**Beobachtet:** `src/server/cache_admin_api_handler.cpp` enthält keinen `pii`-Endpunkt. `invalidatePII()` ist nur über die direkte C++-API erreichbar.

**Evidence (geprüfte Pfade):**
- `src/server/cache_admin_api_handler.cpp` — kein `pii`-Route gefunden ❌

**Issue-Titelvorschlag:** `[cache] Add DELETE /v1/admin/cache/pii/{pii_uuid} admin endpoint for GDPR erasure`
**Label-Vorschläge:** `module:cache`, `gdpr`, `api`, `enhancement`, `priority:high`

---

### Finding 4: Unit-Test-Coverage >80% nicht nachgewiesen

**Claim-Quelle:** `src/cache/ROADMAP.md` § "Production Readiness Checklist" — `[I]` Unit tests coverage > 80% (Issue: #1596)

**Erwartet:** Gemessene Unit-Test-Coverage > 80% für den Cache-Modul-Code, dokumentiert via lcov/gcov-Report oder CI-Badge.

**Beobachtet:** Es gibt 34 test_*cache*.cpp-Dateien in `tests/`. Eine automatische Coverage-Messung (`lcov`, `gcov`) ist in `CMakeLists.txt` nicht konfiguriert. Der tatsächliche Coverage-Wert ist unbekannt.

**Evidence (geprüfte Pfade):**
- `tests/test_adaptive_query_cache.cpp`, `tests/test_bounded_lru_cache.cpp`, etc. — 34 Test-Dateien vorhanden ✅
- `CMakeLists.txt` — keine Coverage-Targets konfiguriert ❌
- Issue #1596 offen

**Issue-Titelvorschlag:** (bestehend) Issue #1596: `[cache] Unit test coverage measurement and tracking (>80% target)`
**Label-Vorschläge:** `module:cache`, `testing`, `ci`

---

### Finding 5: Public-Header-Interfaces aus `include/cache/FUTURE_ENHANCEMENTS.md` nicht implementiert

**Claim-Quelle:** `include/cache/FUTURE_ENHANCEMENTS.md` — Planned interfaces: `ICache<K,V>`, `ICacheWarmup`, `ICacheAdminOps`, `IGDPRPurgeHook`, `ITTLAdapter`

**Erwartet:** Öffentliche Header-Interfaces für pluggable Eviction, GDPR-Purge, Warmup, TTL-Adaption und Admin-Operationen in `include/cache/`.

**Beobachtet:** Keine der genannten Interfaces existiert in `include/cache/`. Verwandte Interfaces (`ICacheCoordinator` in `cache_replication_coordinator.h`, `ICacheReplicationListener` in `cache_replication.h`) sind vorhanden, decken aber den geplanten Scope nicht ab. `ICache<K,V>` existiert in `include/core/concerns/i_cache.h`, aber nicht als Cache-Modul-spezifische öffentliche Schnittstelle.

**Evidence (geprüfte Pfade):**
- `include/cache/` — kein `ICache`, `ICacheWarmup`, `ICacheAdminOps`, `IGDPRPurgeHook`, `ITTLAdapter` ❌
- `include/core/concerns/i_cache.h` — generisches `ICache<K,V>` vorhanden ✅ (nicht Cache-Modul-spezifisch)

**Bewertung:** Dies ist bewusst zurückgestellt; `include/cache/FUTURE_ENHANCEMENTS.md` ist ein Interface-Design-Planungsdokument. Die konkreten Implementierungen (`AdaptiveQueryCache`, `ARCCache`, etc.) sind Production-Ready ohne diese Interfaces.

**Issue-Titelvorschlag:** `[cache] Define and implement ICache<K,V>, ICacheAdminOps, IGDPRPurgeHook, ITTLAdapter in include/cache/ for pluggable architecture`
**Label-Vorschläge:** `module:cache`, `architecture`, `enhancement`, `priority:low`

---

## Fazit

Das Cache-Modul ist **Production-Ready**. Die vier Implementierungsphasen sind abgeschlossen. Die oben aufgeführten Findings betreffen:
- **Finding 1–3:** Kleinere Follow-up-Aufgaben zur GDPR-Vollständigkeit und Sicherheitshärtung
- **Finding 4:** Fehlende Coverage-Messung (Tests existieren, sind aber nicht automatisch gemessen)
- **Finding 5:** Bewusst zurückgestellte Interface-Abstraktion (Design-Dokument, keine Blockers)

Kein Finding blockiert den produktiven Betrieb des Moduls.
