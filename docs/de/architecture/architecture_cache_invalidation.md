# Cache-Invalidierungsstrategie

**Stand:** 2026-03-09
**Version:** v1.5.0
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Events](#events)
- [Versionierung](#versionierung)
- [Negative Caching](#negative-caching)
- [Replikationsbewusstsein](#replikationsbewusstsein)
- [Batch-Invalidierung](#batch-invalidierung)
- [GDPR-Invalidierung (PII-Löschung)](#gdpr-invalidierung-pii-löschung)
- [Verteilte Invalidierung](#verteilte-invalidierung)
- [Sicherheit](#sicherheit)
- [Weiterführende Dokumentation](#weiterführende-dokumentation)

---

Ziel: Korrekte Freshness trotz Replikation/Rebalancing und DSGVO-konformer Datenlöschung.

## Events
- WAL/Changefeed: PUT/DELETE → Entity-Invalidate(URN), ResultCache.InvalidatePlan(plan_hash betroffener Abfragen)
- Topology-Änderung: `cache_epoch` bump; veraltete Einträge validieren
- Tenant-Eviction: `cache.invalidateTenant(tenant_id)` — entfernt alle Einträge eines Tenants über alle Tiers (L1/L2/L3)

## Versionierung
- Jede Entity erhält `version` (WAL-Index). Cache speichert `version` und akzeptiert Hits nur, wenn `cached.version >= applied_version` auf Replica oder Lag < Schwellwert.

## Negative Caching
- 404-Ergebnisse mit kurzer TTL (1–5s) zur Entlastung von Hot-Misses

## Replikationsbewusstsein
- Leader invalidiert authoritative; Replikas respektieren Lag-Grenzen

## Batch-Invalidierung
- Präfix-Invalidierung über alle drei Tiers via `cache.invalidate(pattern)`:
  - L1/L2: regulärer Ausdruck auf Fingerprint-Schlüsseln
  - L3: Iterator-basierter Präfix-Scan im RocksDB-Schlüsselraum
- Tenant-Eviction: `cache.invalidateTenant(tenant_id)` löscht alle Einträge des Tenants aus L1, L2 und L3.

## GDPR-Invalidierung (PII-Löschung)

Implementiert gemäß DSGVO Art. 17 ("Recht auf Löschung"). Wenn `PIIPseudonymizer::erasePII()` für ein Datensatz-Subjekt aufgerufen wird, müssen alle Cache-Einträge, die Daten dieses Subjekts enthalten, sofort aus allen drei Cache-Tiers entfernt werden.

### Funktionsweise (`invalidatePII`)

```cpp
// In adaptive_query_cache.h / adaptive_query_cache.cpp
cache.invalidatePII("pii-uuid-1234-abcd");
```

1. **Reverse-Index** (L1/L2): `put()` akzeptiert einen optionalen `pii_uuids`-Vektor. Wenn dieser nicht leer ist, werden die Cache-Schlüssel in einem UUID-basierten Reverse-Index (`pii_key_index_`, mutex-geschützt) registriert.
2. **L3-Sentinel-Schlüssel**: Pro PII-UUID und Fingerprint wird ein `pii_ref:{uuid}:{fingerprint}`-Sentinel-Schlüssel in RocksDB geschrieben.
3. **`invalidatePII(uuid)`**:
   - Liest und löscht den L1/L2-Reverse-Index in einer einzigen Lock-Akquisition.
   - Entfernt betroffene L1- und L2-Einträge über die Eviction-Strategie-Hooks.
   - Scannt den `pii_ref:{uuid}:`-Präfix in RocksDB, löscht sowohl Sentinel-Schlüssel als auch die zugehörigen `query_cache:{fingerprint}`-Dateneinträge.
   - Respektiert den L3-Circuit-Breaker; protokolliert eine Warnung, wenn der Breaker geöffnet ist.
   - Emittiert nach jedem Aufruf einen strukturierten `THEMIS_INFO`-Logeintrag für operative Nachvollziehbarkeit.
4. **`clear()`** leert auch `pii_key_index_` und alle `pii_ref:`-L3-Einträge.

### Aufruf-Beispiel

```cpp
// Beim Speichern von Einträgen mit PII-Referenz:
cache.put(fingerprint, params, result, tenant_id, {"user-uuid-abc"});

// Bei DSGVO-Löschungsanfrage:
cache.invalidatePII("user-uuid-abc");
// → Alle Einträge, die auf "user-uuid-abc" verweisen, werden sofort aus L1/L2/L3 entfernt.
```

### Verbleibende Aufgaben
- Automatische Integration in `PIIPseudonymizer::erasePII()` (aktuell muss der Aufrufer `invalidatePII()` manuell aufrufen).
- HTTP-Endpunkt `DELETE /v1/admin/cache/pii/{pii_uuid}` (geplant).

## Verteilte Invalidierung

Im Multi-Knoten-Betrieb werden Invalidierungsnachrichten über den `RedisCacheCoordinator` (Redis pub/sub) an alle Knoten des Clusters gesendet:

- `publishInvalidation(pattern)` sendet eine `INVALIDATE`-Nachricht an den `{channel_prefix}:replication`-Kanal.
- Empfangende Knoten entfernen übereinstimmende Einträge aus ihren lokalen L1/L2-Caches (L3 ist gemeinsam genutzter RocksDB; wird übersprungen).
- Selbst-Echo-Prävention: Jede Nachricht enthält eine `node_id`; Empfänger verwerfen eigene Nachrichten.
- Graceful Degradation: Wenn Redis nicht verfügbar ist, wird die lokale Cache-Operation trotzdem abgeschlossen; ein Warn-Log wird ausgegeben.

Aktivierung via CMake: `THEMIS_ENABLE_REDIS=ON` (erfordert `hiredis`).

## Sicherheit
- Namespace im Key → Tenant-Isolation (strukturell unmöglich, auf Daten anderer Tenants zuzugreifen)
- Per-Tenant-Größenkontingente verhindern Cache-Erschöpfungsangriffe
- Token-Bucket-Rate-Limiter verhindert Cache-Flooding durch einen einzelnen Tenant
- Admin-API-Invalidierungsendpunkte erfordern `admin:cache:write` JWT-Scope

## Weiterführende Dokumentation

- [Cache Modul README](../../../../src/cache/README.md) — Modulübersicht und Konfigurationsbeispiele
- [Cache Architektur](../../../../src/cache/ARCHITECTURE.md) — Detaillierte Architektur, Datenflüsse, Threading
- [Cache Roadmap](../../../../src/cache/ROADMAP.md) — Implementierungsstatus und bekannte Einschränkungen
- [Cache Future Enhancements](../../../../src/cache/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen
- [Caching-Pattern-Katalog](architecture_caching_patterns.md) — Lookup-Muster und Caching-Hooks
- [Semantic Cache Implementierungsreferenz](../src/cache/semantic_cache.cpp.md) — SemanticCache-Klassenreferenz

