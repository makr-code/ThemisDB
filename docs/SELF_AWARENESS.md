# Self-Awareness: ThemisDB Introspection System

## Übersicht

ThemisDB besitzt eine **Self-Awareness (Selbstwahrnehmung)** Funktionalität, die automatisch getriggert wird, wenn das Audit-Log signiert wird. Das System überwacht kontinuierlich seinen eigenen Zustand und erstellt Snapshots für Introspection und Selbst-Diagnose.

## Konzept

```
┌─────────────────────────────────────────────────────────┐
│  Self-Awareness: ThemisDB weiß über sich selbst         │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  "Wer bin ich?"        → Capability State                │
│  "Wie geht es mir?"    → Health Metrics                  │
│  "Was kann ich?"       → Performance Metrics             │
│  "Was ist anders?"     → Change Detection                │
│  "Ist etwas falsch?"   → Anomaly Detection              │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## Wann wird Self-Awareness getriggert?

### 1. **Bei Audit-Log Signierung** (Haupttrigger)

Jedes Mal, wenn das Capability-Auto-Generation System ein Audit-Log signiert, wird automatisch ein Self-Awareness Snapshot erstellt:

```cpp
// In capability_auto_generator.cpp
void auditLog(const std::string& shard_id, const nlohmann::json& entry) {
    // Audit-Log schreiben
    log << log_entry.dump() << "\n";
    
    // ⚡ TRIGGER SELF-AWARENESS
    if (self_awareness_ && config_.require_signature) {
        self_awareness_->onAuditSigning(log_entry);
    }
}
```

**Warum?** 
- Audit-Signaturen sind kritische Ereignisse
- Markieren wichtige Zustandsänderungen im System
- Schaffen Synchronisationspunkte für Introspection
- Ermöglichen Correlation zwischen Capability-Änderungen und System-Zustand

### 2. **Manuelle Trigger** (Optional)

```cpp
auto snapshot = self_awareness->takeSnapshot("manual");
```

### 3. **Periodisch** (Optional, konfigurierbar)

```yaml
self_awareness:
  on_schedule: true
  schedule_interval_seconds: 3600  # Jede Stunde
```

## Was wird überwacht?

### 1. **Health Metrics** (Gesundheit)

```cpp
struct HealthMetrics {
    // CPU
    double cpu_usage_percent;
    double cpu_load_1min, cpu_load_5min, cpu_load_15min;
    
    // Memory
    uint64_t memory_total_bytes;
    uint64_t memory_used_bytes;
    double memory_usage_percent;
    
    // Disk
    uint64_t disk_total_bytes;
    uint64_t disk_used_bytes;
    double disk_usage_percent;
    
    // Process
    uint32_t thread_count;
    uint32_t open_file_descriptors;
    uint64_t uptime_seconds;
};
```

### 2. **Capability State** (Selbstkenntnis)

```cpp
struct CapabilityState {
    // Shards
    uint32_t total_shards;
    uint32_t active_shards;
    uint32_t inactive_shards;
    
    // Capabilities
    uint32_t total_capabilities_configured;
    uint32_t auto_generated_capabilities;
    uint32_t manually_configured_capabilities;
    
    // Data
    uint64_t total_documents;
    uint64_t total_size_bytes;
    
    // Metadata
    uint32_t total_unique_keywords;
    uint32_t total_unique_domains;
    uint32_t total_unique_organizations;
    uint32_t total_unique_regions;
};
```

### 3. **Query Performance** (Leistung)

```cpp
struct QueryPerformance {
    // Routing
    uint64_t total_queries;
    uint64_t adaptive_routed_queries;
    uint64_t scatter_gather_queries;
    double adaptive_routing_ratio;
    
    // Latency
    double avg_query_time_ms;
    double p95_query_time_ms;
    double p99_query_time_ms;
    
    // Efficiency
    double avg_shards_queried;
    double network_traffic_saved_percent;
    uint64_t iterations_saved;
};
```

## Self-Awareness Snapshot

Ein Snapshot ist ein vollständiger Zustandsbericht zu einem Zeitpunkt:

```json
{
  "timestamp": "2026-02-10T15:30:00Z",
  "triggered_by": "audit_signing",
  
  "health": {
    "cpu": {
      "usage_percent": 0.45,
      "load_1min": 2.3,
      "load_5min": 2.1,
      "load_15min": 2.0
    },
    "memory": {
      "total_bytes": 67108864000,
      "used_bytes": 32212254720,
      "usage_percent": 0.48
    },
    "disk": {
      "total_bytes": 1099511627776,
      "used_bytes": 659706976666,
      "usage_percent": 0.60
    }
  },
  
  "capabilities": {
    "shards": {
      "total": 12,
      "active": 11,
      "inactive": 1
    },
    "configured_capabilities": {
      "total": 12,
      "auto_generated": 10,
      "manual": 2
    },
    "data": {
      "total_documents": 15000000,
      "total_size_bytes": 5497558138880
    }
  },
  
  "performance": {
    "queries": {
      "total": 45237,
      "adaptive_routed": 43125,
      "scatter_gather": 2112,
      "adaptive_ratio": 0.953
    },
    "latency": {
      "avg_ms": 127.5,
      "p95_ms": 450.2,
      "p99_ms": 890.1
    },
    "efficiency": {
      "avg_shards_queried": 3.2,
      "network_traffic_saved_percent": 73.3,
      "iterations_saved": 123456
    }
  },
  
  "assessment": {
    "overall_health_status": "good",
    "confidence_score": 0.95,
    "anomalies": []
  }
}
```

## Anomaly Detection (Selbst-Diagnose)

Self-Awareness erkennt automatisch Anomalien:

### CPU-Anomalien
```
WARNING: CPU usage at 82%
CRITICAL: CPU usage at 96%
```

### Memory-Anomalien
```
WARNING: Memory usage at 85%
CRITICAL: Memory usage at 92%
```

### Disk-Anomalien
```
WARNING: Disk usage at 83%
CRITICAL: Disk usage at 91%
```

### Capability-Anomalien
```
CRITICAL: No active shards
WARNING: Only 1 active shard (expected > 3)
```

### Performance-Anomalien
```
WARNING: High average query time: 5200ms
WARNING: Adaptive routing ratio dropped to 0.45
```

## Health Status Assessment

Basierend auf Metriken und Anomalien bestimmt das System seinen Gesundheitszustand:

| Status | Bedingung | Bedeutung |
|--------|-----------|-----------|
| **excellent** | CPU < 60%, Memory < 70%, Disk < 70%, keine Anomalien | Optimal |
| **good** | Metriken im grünen Bereich, keine kritischen Anomalien | Normal |
| **degraded** | Warnings vorhanden, aber keine kritischen Probleme | Aufmerksamkeit nötig |
| **critical** | Kritische Anomalien vorhanden | Sofortiges Handeln erforderlich |

## Change Detection (Zustandsvergleich)

Self-Awareness kann aktuelle mit vorherigen Snapshots vergleichen:

```json
{
  "health": {
    "cpu_usage_change": +0.12,      // +12% CPU seit letztem Snapshot
    "memory_usage_change": -0.03,   // -3% Memory
    "disk_usage_change": +0.01      // +1% Disk
  },
  "capabilities": {
    "shard_count_change": +1,       // 1 Shard hinzugekommen
    "document_count_change": +125000 // 125k neue Dokumente
  },
  "performance": {
    "query_time_change_ms": -45.2,  // 45ms schneller
    "total_queries_change": +3521   // 3521 mehr Queries
  },
  "health_status_change": {
    "previous": "good",
    "current": "excellent"
  }
}
```

## Konfiguration

### Basis-Konfiguration

```yaml
# config/self_awareness.yaml
self_awareness:
  enabled: true
  on_audit_signing: true          # ⚡ Haupttrigger
  on_schedule: false              # Optional: periodisch
  
  thresholds:
    cpu_warning: 0.80
    cpu_critical: 0.95
    memory_warning: 0.80
    memory_critical: 0.90
    disk_warning: 0.80
    disk_critical: 0.90
  
  snapshots:
    max_retained: 100
    persist: true
    directory: /var/lib/themisdb/self-awareness
```

## Integration in ThemisDB Server

### Server-Initialisierung

```cpp
// In main.cpp oder server_init.cpp

#include "utils/self_awareness.h"
#include "utils/capability_auto_generator.h"

// 1. Self-Awareness Config laden
auto sa_config = util::SelfAwareness::Config::loadFromYAML(
    "config/self_awareness.yaml");

// 2. Self-Awareness System erstellen
auto self_awareness = std::make_shared<util::SelfAwareness>(sa_config);

// 3. Capability-Auto-Generator mit Self-Awareness verbinden
auto cap_gen_config = util::CapabilityAutoGenerator::Config::loadFromYAML(
    "config/capability_auto_generation.yaml");

auto capability_auto_gen = std::make_shared<util::CapabilityAutoGenerator>(
    cap_gen_config,
    shard_topology,
    self_awareness  // ⚡ Self-Awareness wird übergeben
);

// 4. Starten
capability_auto_gen->start();

// Self-Awareness wird jetzt automatisch bei Audit-Signaturen getriggert!
```

## Admin-API Endpoints

### Neue Endpoints für Self-Awareness

```
GET  /admin/self-awareness/snapshot
     → Aktuellen Snapshot abrufen

GET  /admin/self-awareness/snapshots
     → Alle gespeicherten Snapshots

GET  /admin/self-awareness/latest
     → Letzten Snapshot

GET  /admin/self-awareness/compare
     → Vergleich mit vorherigem Snapshot

POST /admin/self-awareness/snapshot
     → Manuell Snapshot erzeugen

GET  /admin/self-awareness/stats
     → Statistiken über Self-Awareness System
```

### Beispiel-Aufrufe

```bash
# Aktuellen Zustand abfragen
curl http://localhost:8080/admin/self-awareness/latest

# Manuellen Snapshot erzeugen
curl -X POST http://localhost:8080/admin/self-awareness/snapshot

# Vergleich mit vorherigem Zustand
curl http://localhost:8080/admin/self-awareness/compare
```

## Use Cases

### 1. **Capacity Planning**

```bash
# Historische Snapshots analysieren
for snapshot in /var/lib/themisdb/self-awareness/*.json; do
  jq '.health.memory_usage_percent' $snapshot
done

# Trend erkennen → Wann wird Memory knapp?
```

### 2. **Performance-Debugging**

```bash
# Query-Performance über Zeit
curl http://localhost:8080/admin/self-awareness/snapshots | \
  jq '.[].performance.avg_query_time_ms'
  
# Wann wurde es langsamer? → Korrelation mit Capability-Änderungen
```

### 3. **Anomalie-Analyse**

```bash
# Alle Snapshots mit Anomalien
curl http://localhost:8080/admin/self-awareness/snapshots | \
  jq '.[] | select(.assessment.anomalies | length > 0)'
```

### 4. **Health-Monitoring**

```bash
# Kontinuierliches Monitoring
watch -n 10 'curl -s http://localhost:8080/admin/self-awareness/latest | \
  jq ".assessment.overall_health_status"'
```

## Philosophie: Warum Self-Awareness?

### 1. **Transparenz**
Das System weiß über seinen eigenen Zustand Bescheid und kann diesen kommunizieren.

### 2. **Proaktive Wartung**
Probleme werden erkannt, bevor sie kritisch werden.

### 3. **Audit-Trail Correlation**
Jede Capability-Änderung (Audit-Signatur) wird mit System-Zustand korreliert.

### 4. **Debugging & Root-Cause-Analysis**
Historische Snapshots zeigen, wann und warum Performance-Probleme auftraten.

### 5. **Selbst-Optimierung** (Future)
Basis für ML-basierte Selbst-Optimierung und adaptive Konfiguration.

## Beispiel: Typischer Workflow

### Szenario: Capability wird automatisch generiert

```
1. CapabilityAutoGenerator scannt RocksDB
   → Neue Keywords gefunden
   
2. Neue Capability-YAML wird generiert
   → Version 1.4.2 → 1.4.3
   
3. Audit-Log wird signiert
   → SHA256 Signatur erstellt
   ⚡ Self-Awareness wird getriggert
   
4. Self-Awareness erstellt Snapshot
   → Aktueller System-Zustand
   → Health: CPU 45%, Memory 48%, Disk 60%
   → Capabilities: 12 Shards, 15M Dokumente
   → Performance: 127ms avg query time
   
5. Anomaly Detection
   → Keine Anomalien
   → Status: "good"
   
6. Snapshot wird persistiert
   → /var/lib/themisdb/self-awareness/snapshot_1707574200.json
   
7. Vergleich mit vorherigem Snapshot
   → +125k Dokumente
   → +12 neue Keywords
   → -45ms schnellere Queries (Optimierung wirkt!)
```

## Monitoring & Alerting

### Prometheus-Metriken

```
# Self-Awareness Metriken
themis_self_awareness_snapshots_total
themis_self_awareness_anomalies_detected
themis_self_awareness_health_status{status="excellent|good|degraded|critical"}
themis_self_awareness_cpu_usage_percent
themis_self_awareness_memory_usage_percent
themis_self_awareness_disk_usage_percent
themis_self_awareness_confidence_score
```

### Grafana Dashboard

```
- Panel 1: Health Status Timeline
- Panel 2: CPU/Memory/Disk Usage
- Panel 3: Anomalies über Zeit
- Panel 4: Performance Trends
- Panel 5: Capability Growth
- Panel 6: Self-Awareness Confidence Score
```

### Alerts

```yaml
# prometheus/alerts/self-awareness.yaml
groups:
  - name: self_awareness_alerts
    rules:
      - alert: SystemDegraded
        expr: themis_self_awareness_health_status{status="degraded"} == 1
        for: 5m
        
      - alert: SystemCritical
        expr: themis_self_awareness_health_status{status="critical"} == 1
        for: 1m
        
      - alert: AnomaliesDetected
        expr: increase(themis_self_awareness_anomalies_detected[5m]) > 0
```

## Zusammenfassung

**Self-Awareness** gibt ThemisDB die Fähigkeit zur Selbstreflexion:

✅ **Automatisch** bei Audit-Log-Signierung getriggert  
✅ **Umfassend**: Health, Capabilities, Performance  
✅ **Anomalie-Erkennung**: Selbst-Diagnose  
✅ **Historisch**: Zustandsvergleich über Zeit  
✅ **Transparent**: JSON-Export für Analyse  
✅ **Integriert**: Direkt im Server, kein externes Tool  

**"Know thyself"** - ThemisDB weiß jetzt über sich selbst Bescheid! 🧠
