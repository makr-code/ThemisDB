# Utils-Modul

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/utils/README.md · ../../../include/utils/README.md -->

**Stand:** 13. Mai 2026
**Version:** v1.5.0
**Kategorie:** Hilfsfunktionen / Infrastruktur
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Utils-Modul stellt gemeinsame Hilfsfunktionen und Infrastrukturkomponenten für ThemisDB bereit. Es wird von allen anderen Modulen verwendet und deckt folgende Bereiche ab: strukturiertes Logging, manipulationssichere Audit-Protokollierung, PII-Erkennung und -Pseudonymisierung, HKDF-Schlüsselableitung, LEK-Verwaltung (Local Encryption Key), verteiltes Tracing, Kompression (ZSTD/LZ4), Textverarbeitung (Normalisierung, Stemming, Stoppwörter), Serialisierung, Geodaten-Hilfsfunktionen, Paginierungs-Cursor, Nebenläufigkeitsprimitive und sichere Arithmetik.

**Namespace:** `themis::utils` (Geodaten: `themis::geo`)

**Primäre Quellen:**
- [`src/utils/`](../../../src/utils/) — Implementierung und Planungsdokumente
- [`include/utils/`](../../../include/utils/) — Öffentliche Header-Dateien

---

## Kernkomponenten

### Logging und Observability

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `Logger` | `logger.h` | Strukturiertes spdlog-basiertes Logging-Facade mit Level-Metriken |
| `AuditLogger` | `audit_logger.h` | Manipulationssichere, append-only Audit-Protokollierung (SHA-256-Hashkette) |
| `SagaLogger` / `SAGALogCompactor` | `saga_logger.h` | WAL-basierte SAGA-Schritt-Protokollierung, Kompaktierung und Replay |
| `Tracer` / `Span` | `tracing.h` | OpenTelemetry-kompatibles verteiltes Tracing; No-Op bei nicht erreichbarem Collector |

### PII-Erkennung und Datenschutz

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `PIIDetector` | `pii_detector.h` | Plugin-basierter PII-Detektor; YAML-konfigurierbar, Laufzeit-Reload |
| `PIIDetectionEngine` | `pii_detection_engine.h` | Orchestrierung mehrerer Erkennungs-Engines |
| `PIIPseudonymizer` | `pii_pseudonymizer.h` | Deterministisches HMAC-basiertes Pseudonymisieren |
| `PIIRedactingSink` | `pii_redacting_sink.h` | spdlog-Sink, das PII inline schwärzt |
| `RegexDetectionEngine` | `regex_detection_engine.h` | Regex-Erkennung (E-Mail, Telefon, SSN, IBAN, IP, URL) |
| `NERDetectionEngine` | `ner_detection_engine.h` | MITIE/ONNX-basierte Named-Entity-Erkennung (optional) |

### Kryptografische Hilfsfunktionen

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `HkdfHelper` | `hkdf_helper.h` | HKDF-SHA-256-Schlüsselableitung gemäß RFC 5869 |
| `HkdfCache` | `hkdf_cache.h` | Thread-sicherer LRU-Cache für abgeleitetes Schlüsselmaterial (TTL: 300 s, Kapazität: 1 000) |
| `LEKManager` | `lek_manager.h` | Local Encryption Key (AES-256 DEK) Verwaltung und automatisierte Rotation |
| `PKIClient` | `pki_client.h` | Zertifikats-Ausstellung und -Verifikation; Graceful-Degradierung bei CA-Ausfall |
| `OpenSSLDeleter` | `openssl_deleter.h` | RAII-Deleter für OpenSSL-Objekte |
| `crc32()` / `sha256()` | `checksum_utils.h` | CRC-32- und SHA-256-Prüfsummen |
| `generateUUID()` / `generate_uuid_v7()` | `uuid.h` | RFC 4122 UUIDv4 und RFC 9562 UUIDv7 (monoton, thread-safe) |

### Kompression und Serialisierung

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `ZstdCodec` | `zstd_codec.h` | Zstandard-Kompression/Dekompression; Streaming-API mit 4-GB-DoS-Schutz |
| `LZ4Codec` | `lz4_codec.h` | LZ4-Block-Kompression/Dekompression |
| `serialize()` / `deserialize()` | `serialization.h` | Generische Binär-Serialisierung |
| `LosslessVectorCompressor` | `lossless_vector_compression.h` | Verlustfreie Float-Vektor-Kompression (Bit-Packing) |

### Textverarbeitung

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `Normalizer` | `normalizer.h` | Unicode/Umlaut-Text- und Zahl-Normalisierung |
| `Stemmer` | `stemmer.h` | Porter/Snowball-Stemming für Suchanfragen |
| `Stopwords` | `stopwords.h` | Stoppwortfilter für die Textverarbeitung |
| `trim()` / `split()` / `toLower()` | `string_utils.h` | Allgemeine String-Hilfsfunktionen |

### Datenstrukturen und Nebenläufigkeit

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `BloomFilter` | `bloom_filter.h` | Probabilistischer Bloom-Filter (Doppel-Hashing, `std::shared_mutex`) |
| `ConcurrentCache` | `concurrent_cache.h` | Thread-sicherer generischer LRU-Cache |
| `ConsistentHashRing` | `consistent_hash.h` | FNV-1a 64-Bit Consistent-Hash-Ring mit virtuellen Knoten |
| `RateLimiter` | `rate_limiter.h` | Token-Bucket-Rate-Limiter; `try_acquire` (nicht blockierend) und `acquire` (blockierend) |
| `BatchOperationManager` | `batch_operation_manager.h` | Gebündelte asynchrone Operationsauslösung |
| `ThreadPoolManager` | `thread_pool_manager.h` | Verwaltung benannter Thread-Pools |
| `PoolAllocator` | `memory/pool_allocator.h` | Speicher-Pool-Allokator für feste Objektgrößen |

### Scheduling und Lebenszyklus

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| `Cursor` | `cursor.h` | Opaker Paginierungs-Cursor; serialisierbar |
| `CronParser` | `cron_parser.h` | Cron-Ausdrucks-Parser und Trigger-Berechnung |
| `RetentionManager` | `retention_manager.h` | Datenhaltungsrichtlinien und Ablauf-Durchsetzung |
| `IClock` / `SystemClock` | `clock.h` | Mock-fähige Systemuhr-Abstraktion |
| `nowMs()` / `formatTimestamp()` | `timestamp_utils.h` | ISO 8601 / RFC 3339 Zeitstempel-Formatierung und -Analyse |

### Geodaten

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| EWKB encode/decode | `geo/ewkb.h` | PostGIS Extended Well-Known Binary Geometrie-Kodierung |
| `GeoValidator` | `geo/validator.h` | Geometrie-Validierung und Grenzwertprüfung |
| `haversine()` / `euclidean()` | `geometric_distances.h` | Geospatiale Distanzberechnungen |
| `simdCosine()` / `simdL2()` | `simd_distance.h` | SIMD-beschleunigte Kosinus- und L2-Vektordistanz |

---

## Konfigurationsoptionen

| Komponente | Konfigurierbare Parameter |
|------------|--------------------------|
| `RateLimiter` | `rate_per_second`, `burst_size`; änderbar via `set_rate()` |
| `HkdfCache` | Kapazität (Standard: 1 000 Einträge), TTL (Standard: 300 s) |
| `BloomFilter` | Erwartete Elementanzahl `n`, Falschpositivrate `p` |
| `ZstdCodec` | Komprimierungslevel 1–22; `max_output_bytes` DoS-Schutz |
| `Tracer` | Sampling-Strategie: `ALWAYS_ON`, `ALWAYS_OFF`, `PROBABILITY`, `PARENT_BASED`, `ADAPTIVE` |
| `PIIDetector` | YAML-Konfigurationsdatei; Engine-Auswahl (`regex`, `ner`, `embedding`) |
| `LEKManager` | Rotationsintervall; DEK-Schlüssellänge |

---

## Laufzeitverhalten, Fehlerfälle und Grenzen

- **AuditLogger**: Audit-Ereignisse werden nie stillschweigend verworfen. Bei Schreibfehler wird auf niedrigerem Level geloggt.
- **HkdfCache**: `purge_by_ikm_hash()` ist O(n) über den Cache — nicht in kritischen Pfaden aufrufen. Rohes IKM wird nie gespeichert.
- **PIIDetector**: Bei fehlendem oder fehlerhaftem YAML-Config fällt das System auf Regex-only-Modus zurück; der Fehler wird als WARN geloggt.
- **ZstdCodec (Streaming)**: `zstd_decompress_stream()` wirft `std::runtime_error` bei Überschreitung von `max_output_bytes` (Standard: 4 GB) als DoS-Schutz.
- **RateLimiter**: `acquire(n)` blockiert unbegrenzt; externe Abbruchlogik ist Aufgabe des Aufrufers.
- **ConsistentHashRing**: `getNodes(key, n)` liefert weniger als `n` Knoten, wenn der Ring weniger Knoten enthält.
- **Tracer**: Alle Span-Operationen sind No-Ops bei nicht erreichbarem Collector; keine Ausnahme wird ausgelöst.
- **PKIClient**: Bei CA-Ausfall wird auf Stub-Verifikation (immer `verified=true`) zurückgefallen — nicht für sicherheitskritische Entscheidungen geeignet.
- **BloomFilter**: Wird nicht über Neustarts hinweg persistiert; nur Falschpositive, keine Falschnegative.
- **`safeCast<T>()`**: Wirft `std::overflow_error` bei Einengungskonversionen.

---

## Verwendungsbeispiele

```cpp
// Strukturiertes Logging
#include "utils/logger.h"
auto log = themis::utils::Logger::get("mein_modul");
log->info("Verarbeite Datensatz id={}", record_id);
```

```cpp
// Audit-Ereignis schreiben und Kette prüfen
#include "utils/audit_logger.h"
themis::utils::AuditLogger audit("/var/log/themis/audit.jsonl");
audit.log(themis::utils::SecurityEventType::LOGIN_SUCCESS, nutzer_id, {{"ip", client_ip}});

themis::utils::AuditLogVerifier verifier;
auto result = verifier.verify("/var/log/themis/audit.jsonl");
if (!result.ok) { /* Manipulierter Eintrag: result.first_bad_entry */ }
```

```cpp
// PII-Erkennung und Pseudonymisierung
#include "utils/pii_detector.h"
#include "utils/pii_pseudonymizer.h"
themis::utils::PIIDetector detektor("/etc/themis/pii.yaml");
auto spans = detektor.scan(rohtext);
themis::utils::PIIPseudonymizer pseudo(tenant_hmac_schluessel);
std::string sicherer_text = pseudo.pseudonymize(rohtext, spans);
```

```cpp
// Zstd-Kompression (Block-API)
#include "utils/zstd_codec.h"
themis::utils::ZstdCodec codec(/*level=*/3);
auto komprimiert = codec.compress(daten);
auto original    = codec.decompress(komprimiert);
```

```cpp
// Token-Bucket-Rate-Limiting
#include "utils/rate_limiter.h"
static themis::utils::RateLimiter limiter(500.0, 50.0);
if (!limiter.try_acquire(1)) {
    return tl::make_unexpected("Rate-Limit überschritten");
}
```

```cpp
// UUID v7 (zeitgeordnet, monoton)
#include "utils/uuid.h"
std::string id = themis::utils::generate_uuid_v7();
```

---

## Troubleshooting

| Symptom | Wahrscheinliche Ursache | Empfohlene Maßnahme |
|---------|------------------------|---------------------|
| `AuditLogVerifier` meldet manipulierte Kette | Log-Datei extern verändert oder Chain-Head veraltet | Dateisystemzugriff prüfen; Replay ab bekannt gültigem Genesis |
| PII-Engine fällt auf Regex-only zurück | YAML-Config fehlt oder fehlerhaft; Engine-Bibliothek nicht verfügbar | Logs auf YAML-Parse-Fehler prüfen; MITIE/ONNX-Verfügbarkeit sicherstellen |
| `ZstdCodec::decompress` wirft Ausnahme | Eingabe überschreitet DoS-Schutzgrenze | Eingabequelle validieren; Grenze in Produktion nicht deaktivieren |
| Tracing-Spans erscheinen nicht im Collector | Collector nicht erreichbar oder `THEMIS_ENABLE_TRACING` nicht gesetzt | Collector-Endpunkt und Compile-Flag prüfen |
| `RateLimiter::acquire` blockiert dauerhaft | Refill-Rate zu niedrig für Anfragevolumen | `rate_per_second` erhöhen oder Abbruch-Timeout hinzufügen |
| Hohe HkdfCache-Miss-Rate | TTL zu kurz oder hohe IKM-Fluktuation | TTL anpassen; `purge_by_ikm_hash`-Aufrufe reduzieren |
| `PKIClient` nutzt Stub-Fallback | CA-Endpunkt nicht erreichbar | CA-URL in ThemisDB-Konfiguration und Netzwerkkonnektivität prüfen |

---

## Installation

Dieses Modul ist Bestandteil von ThemisDB. Die Header-Dateien werden über den ThemisDB-Include-Pfad eingebunden:

```cmake
target_include_directories(mein_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Verwendung / Usage
Alle öffentlichen Schnittstellen sind über die Header unter `include/utils/` verfügbar.
Siehe [`include/utils/README.md`](../../../include/utils/README.md) für die vollständige Public-API-Dokumentation und Verwendungsbeispiele.

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`include/utils/README.md`](../../../include/utils/README.md) | Öffentliche API-Dokumentation (alle Header) |
| [`src/utils/README.md`](../../../src/utils/README.md) | Implementierungsübersicht und Laufzeitverhalten |
| [`src/utils/ROADMAP.md`](../../../src/utils/ROADMAP.md) | Liefer-Roadmap und Produktionsreife-Checkliste |
| [`src/utils/FUTURE_ENHANCEMENTS.md`](../../../src/utils/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen und Verbesserungen |
| [`src/utils/ARCHITECTURE.md`](../../../src/utils/ARCHITECTURE.md) | Komponentendiagramm und Abhängigkeitsregeln |
| [`src/utils/SECURITY.md`](../../../src/utils/SECURITY.md) | Sicherheitsbaseline (Schlüsselverwaltung, PII, Audit) |
| [`src/utils/CHANGELOG.md`](../../../src/utils/CHANGELOG.md) | Änderungshistorie |

