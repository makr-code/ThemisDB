# Shard Repair Engine & Anti-Entropy-Mechanismen

## Übersicht

ThemisDB v1.5 führt eine automatische Self-Healing-Infrastruktur für Parity- (RAID-5/6) und Mirror-Shard-Setups ein. Der **`ShardRepairEngine`** erkennt beschädigte oder fehlende Chunks, stellt sie per Reed-Solomon- oder Mirror-Decoding wieder her und exponiert den Reparatur-Status über Prometheus und das Admin-API.

---

## Kernkomponenten

### ShardRepairEngine (`include/sharding/shard_repair_engine.h`)

Die Hauptklasse koordiniert alle Repair- und Anti-Entropy-Aktivitäten:

| Funktion | Beschreibung |
|---|---|
| **Background Scan-Thread** | Regelmäßige `checkDocumentHealth()`-Aufrufe für alle Shards |
| **Repair Worker-Thread** | Verarbeitet die Job-Queue via `RedundancyStrategy::recoverDocument()` |
| **On-Demand-Trigger** | `triggerRepair()`, `triggerFullScan()`, `triggerDocumentRepair()` |
| **Per-Shard Health-Report** | `ShardHealthReport` mit Status HEALTHY / DEGRADED / FAILED / REBUILDING |
| **Prometheus-Forwarding** | Leitet Ereignisse direkt an `PrometheusMetrics` weiter |

### Verbesserter Reed-Solomon-Decoder

Der `ReedSolomonCoder` wurde von XOR-Only-Parität auf **Vandermonde-Matrix**-basiertes systematisches Encoding über GF(2⁸) umgestellt:

- **Vorher:** Maximal 1 gleichzeitiger Chunk-Ausfall (XOR-Parität)
- **Jetzt:** Bis zu `parity_shards` gleichzeitige Chunk-Ausfälle (echtes RAID-6)

```cpp
// RAID-6: 4 Daten-Shards + 2 Parity-Shards → toleriert 2 gleichzeitige Ausfälle
auto chunks = coder.encode(data, /*k=*/4, /*m=*/2);
auto recovered = coder.decode(available, {0, 3}, 4, 2);  // Chunks 0 und 3 fehlen
```

---

## Konfiguration

```cpp
#include "sharding/shard_repair_engine.h"

themis::sharding::RepairConfig cfg;
cfg.scan_interval         = std::chrono::seconds(300);  // Anti-Entropy alle 5 min
cfg.repair_poll_interval  = std::chrono::seconds(30);   // Worker-Polling-Intervall
cfg.enable_periodic_scan  = true;
cfg.enable_auto_repair    = true;
cfg.default_collection    = "documents";

auto engine = std::make_shared<themis::sharding::ShardRepairEngine>(
    cfg, strategy, ring, topology, readHandler, writeHandler);

// Dokument-Listen-Provider (optional)
engine->setDocumentListProvider([](const std::string& shard_id) {
    return myStorage.listDocuments(shard_id);
});

// Prometheus-Metriken verbinden
engine->setPrometheusMetrics(promMetrics);

engine->start();
```

---

## Admin-API-Endpunkte

| Methode | Pfad | Beschreibung |
|---|---|---|
| `POST` | `/admin/repair` | Reparatur auslösen (`{"shard_id":"..."}` oder `{}` für alle Shards) |
| `POST` | `/admin/repair/scan` | Vollständigen Anti-Entropy-Scan auslösen |
| `GET`  | `/admin/repair/{job_id}` | Repair-Job-Status abfragen |
| `GET`  | `/admin/health` | Enthält jetzt ein `"repair"`-Feld mit Per-Shard-Status |

### Beispiel: Shard reparieren

```bash
# Einzelnen Shard reparieren
curl -X POST http://localhost:8080/admin/repair \
  -H "Authorization: Bearer <operator-cert>" \
  -H "Content-Type: application/json" \
  -d '{"shard_id": "shard_3"}'
# → {"job_id": "repair-1708450000000-42", "status": "queued", "shard_id": "shard_3"}

# Job-Status abfragen
curl http://localhost:8080/admin/repair/repair-1708450000000-42 \
  -H "Authorization: Bearer <operator-cert>"
# → {"job_id": "repair-...", "status": "completed", "shard_id": "shard_3"}
```

### Beispiel: Vollständigen Scan auslösen

```bash
curl -X POST http://localhost:8080/admin/repair/scan \
  -H "Authorization: Bearer <operator-cert>"
# → {"job_id": "repair-1708450001000-43", "status": "queued"}
```

### Health-Endpoint mit Repair-Sektion

```json
GET /admin/health
{
  "status": "healthy",
  "repair": {
    "status": "degraded",
    "engine_running": true,
    "total_scans": 12,
    "repairs_attempted": 4,
    "repairs_successful": 3,
    "repairs_failed": 1,
    "avg_repair_ms": 42,
    "shards": [
      { "shard_id": "shard_1", "status": "healthy",  "documents_scanned": 500 },
      { "shard_id": "shard_3", "status": "degraded", "documents_degraded": 2  }
    ]
  }
}
```

---

## Prometheus-Metriken

| Metrik | Typ | Beschreibung |
|---|---|---|
| `themis_shard_repair_scans_total` | Counter | Abgeschlossene Anti-Entropy-Scans |
| `themis_shard_repair_operations_total{result}` | Counter | Repair-Versuche nach Ergebnis (success/failure) |
| `themis_shard_repair_duration_seconds` | Histogram | Repair-Dauer pro Dokument |
| `themis_shard_repair_health{shard_id,status}` | Gauge | Per-Shard-Status (1 = aktiv, 0 = inaktiv) |
| `themis_shard_repair_avg_duration_ms` | Gauge | Gleitender Mittelwert der Repair-Dauer (ms) |
| `themis_shard_degraded_documents{shard}` | Gauge | Anzahl degradierter Dokumente pro Shard |

Vollständige Alerting-Regeln und Schwellenwerte:
→ `config/monitoring/prometheus/prometheus_repair_metrics.yaml`

---

## Integration mit anderen Komponenten

### AutoRecoveryManager

`AutoRecoveryManager::repairDocument()` war bisher ein Stub (`return false`). Mit `setRepairEngine()` wird die Funktion auf den `ShardRepairEngine` delegiert:

```cpp
themisdb::sharding::AutoRecoveryManager arm(cfg, strategy, ring, topology);
arm.setRepairEngine(engine);  // repairDocument() delegiert jetzt an ShardRepairEngine
```

### HotSpareManager

Nach einem erfolgreichen Failover via `activateSpare()` wird `ShardRepairEngine::triggerRepair(spare_id)` aufgerufen, um erasure-aware Datenwiederherstellung auf dem neuen Spare-Shard einzuleiten:

```cpp
HotSpareManager hot_spare(cfg, strategy, topology);
hot_spare.setRepairEngine(engine);
// Nach activateSpare() wird automatisch engine->triggerRepair(spare_id) aufgerufen
```

### ShardingMetricsHandler

```cpp
// Repair-Metriken im Haupt-Scrape-Endpunkt (/metrics/sharding) exponieren
metricsHandler->setRepairEngine(engine);
// getMetrics() hängt jetzt automatisch die Repair-Metriken an
```

---

## gRPC-RPC (Phase 3)

Das Protokoll-Buffer-Schema in `proto/sharding/shard_rpc.proto` enthält Repair-RPCs für die spätere gRPC-Integration:

```protobuf
service ShardService {
  rpc TriggerRepair(RepairRequest)        returns (RepairJobResponse);
  rpc TriggerRepairScan(RepairScanRequest) returns (RepairJobResponse);
  rpc GetRepairStatus(RepairStatusRequest) returns (RepairStatusResponse);
}
```

---

## Architekturdiagramm

```
┌─────────────────────────────────────────────────────────────────┐
│                       ShardRepairEngine                         │
│                                                                 │
│   ┌──────────────────┐         ┌─────────────────────────────┐  │
│   │  Scan-Thread     │         │  Repair Worker-Thread       │  │
│   │  (periodic)      │──jobs──▶│  (Queue-Verarbeitung)       │  │
│   │                  │         │                             │  │
│   │  checkDocument   │         │  recoverDocument()          │  │
│   │  Health()        │         │  → ReedSolomon / Mirror     │  │
│   └──────────────────┘         └─────────────────────────────┘  │
│            │                              │                     │
│            ▼                              ▼                     │
│   ┌──────────────────┐         ┌─────────────────────────────┐  │
│   │  ShardHealthReport│        │  RepairMetrics (intern)     │  │
│   │  per Shard        │        │  + PrometheusMetrics        │  │
│   └──────────────────┘         └─────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
         │                              │
         ▼                              ▼
  GET /admin/health              POST /metrics (Prometheus)
  (repair-Sektion)
```

---

## Implementierungsstatus

| Komponente | Status |
|---|---|
| ShardRepairEngine (Kern) | ✅ Vollständig implementiert |
| Vandermonde-RS-Decoder | ✅ Vollständig implementiert |
| Cauchy-RS missing_indices-Validierung | ✅ Vollständig implementiert |
| Prometheus-Metriken | ✅ Vollständig implementiert |
| Admin-API-Endpunkte | ✅ Vollständig implementiert |
| GET /admin/health Integration | ✅ Vollständig implementiert |
| AutoRecoveryManager-Integration | ✅ Vollständig implementiert |
| HotSpareManager-Integration | ✅ Vollständig implementiert |
| ShardingMetricsHandler-Integration | ✅ Vollständig implementiert |
| Unit-Tests (43) | ✅ Vollständig implementiert |
| OpenAPI-Spec | ✅ Dokumentiert |
| gRPC-Proto (Phase 3) | ✅ Definiert (Implementierung in Phase 3) |

---

## Referenzen

- [src/sharding/shard_repair_engine.cpp] – Vollständige Implementierung
- [include/sharding/shard_repair_engine.h] – API-Dokumentation
- [tests/test_sharding_repair.cpp] – 43 Unit-Tests
- [config/monitoring/prometheus/prometheus_repair_metrics.yaml] – Prometheus-Alerting-Regeln
- [proto/sharding/shard_rpc.proto] – gRPC-Protokolldefinitionen
- [openapi/openapi.yaml] – REST-API-Spezifikation
- CockroachDB Repair Patterns, Ceph Recovery, EC-Storage Literature
