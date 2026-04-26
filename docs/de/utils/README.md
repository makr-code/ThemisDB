# Utils-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/utils/README.md -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** Hilfsfunktionen / Infrastruktur  
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Utils-Modul stellt gemeinsame Hilfsfunktionen und Infrastrukturkomponenten für ThemisDB bereit: Audit-Logging, Bloom-Filter, Checksum-Utilities, Konsistentes Hashing und mehr.

**Primäre Quelle:** [`src/utils/`](../../../src/utils/) · [`include/utils/`](../../../include/utils/)

---

## Kernkomponenten (Auswahl)

| Komponente | Header | Beschreibung |
|------------|--------|--------------|
| AuditLogger | `audit_logger.h` | Strukturiertes Audit-Logging |
| BloomFilter | `bloom_filter.h` | Probabilistischer Bloom-Filter |
| ConsistentHash | `consistent_hash.h` | Konsistentes Hash-Ringing |
| CronParser | `cron_parser.h` | Cron-Ausdrucks-Parser für Scheduling |
| ChecksumUtils | `checksum_utils.h` | SHA-256/CRC32-Prüfsummen |
| ConcurrentCache | `concurrent_cache.h` | Thread-sicherer generischer Cache |
| BatchOperationManager | `batch_operation_manager.h` | Batch-Operationssteuerung |
| CapabilityAutoGenerator | `capability_auto_generator.h` | Automatische Fähigkeits-Generierung |
| GrpcChannelPool | `grpc_channel_pool.h` | gRPC-Channel-Pool |
| CompressionMetrics | `compression_metrics.h` | Kompressions-Metriken |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/utils/README.md`](../../../src/utils/README.md) | Modulübersicht |
