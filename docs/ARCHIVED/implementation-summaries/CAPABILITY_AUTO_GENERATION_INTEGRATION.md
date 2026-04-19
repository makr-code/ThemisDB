# Capability Auto-Generation: Vollständige Integration

## Übersicht

Die Capability-Auto-Generierung ist jetzt als **native C++-Komponente im `themis::util` Namespace** integriert. Sie analysiert automatisch RocksDB-Daten und generiert/aktualisiert Shard-Capability-YAMLs mit vollständigem Audit-Trail.

## Architektur

```
┌──────────────────────────────────────────────────────────────┐
│  ThemisDB Server                                              │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  themis::util::CapabilityAutoGenerator                       │
│  ┌─────────────────────────────────────────┐                │
│  │  Background Thread                      │                │
│  │  ├─ Lädt YAML Config                    │                │
│  │  ├─ Periodisches Scanning (per Schedule)│                │
│  │  └─ Verarbeitet jeden Shard einzeln     │                │
│  └─────────────────────────────────────────┘                │
│         ↓                                                     │
│  ┌─────────────────────────────────────────┐                │
│  │  RocksDB Analyzer                       │                │
│  │  ├─ Öffnet RocksDB (read-only)         │                │
│  │  ├─ Scannt Dokumente (mit sampling)    │                │
│  │  ├─ Extrahiert Metadaten                │                │
│  │  └─ Berechnet Keywords (TF-IDF)        │                │
│  └─────────────────────────────────────────┘                │
│         ↓                                                     │
│  ┌─────────────────────────────────────────┐                │
│  │  Capability Generator                   │                │
│  │  ├─ Merged mit existierendem Capability │                │
│  │  ├─ Berechnet Änderungen                │                │
│  │  ├─ Prüft Schwellwerte                  │                │
│  │  └─ Generiert neue Version              │                │
│  └─────────────────────────────────────────┘                │
│         ↓                                                     │
│  ┌─────────────────────────────────────────┐                │
│  │  Audit & Persistence                    │                │
│  │  ├─ SHA256 Signatur erstellen           │                │
│  │  ├─ Audit-Log schreiben                 │                │
│  │  ├─ YAML-Datei speichern                │                │
│  │  └─ Git commit (optional)               │                │
│  └─────────────────────────────────────────┘                │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

## Konfiguration (YAML)

### Datei: `config/capability_auto_generation.yaml`

```yaml
# Master-Schalter
enabled: true

# Update-Zeitpläne nach Shard-Typ
schedules:
  real-time:       # Gesundheitsdaten, Börsenkurse
    interval_seconds: 1800        # Alle 30 Minuten
    min_document_change: 100
    min_keyword_change: 0.02      # 2%
    require_review: false
    
  high-frequency:  # Bauanträge, Verwaltung
    interval_seconds: 10800       # Alle 3 Stunden
    min_document_change: 500
    min_keyword_change: 0.05      # 5%
    
  normal:          # Rechtsdatenbank, Archive
    interval_seconds: 86400       # Täglich
    min_document_change: 1000
    min_keyword_change: 0.10      # 10%
    
  static:          # Historische Daten
    interval_seconds: 604800      # Wöchentlich
    min_document_change: 5000
    min_keyword_change: 0.15      # 15%
    
  critical:        # Finanzen, medizinische Kerndaten
    interval_seconds: 43200       # Alle 12 Stunden
    require_review: true          # Immer Review!
    auto_approve_threshold: 0

# RocksDB-Analyse
rocksdb_analysis:
  sampling_rate: 100              # Jedes 100. Dokument analysieren
  max_keywords: 100               # Top 100 Keywords
  
# Audit & Security
audit:
  enabled: true
  log_path: /var/log/themisdb/capability-generation.log
  
security:
  require_signature: true
  signing_key_path: /etc/themisdb/keys/capability-signing.key
  
# Output
output:
  directory: config/capabilities
  create_backups: true
  git_commit: false               # Optional: Auto-commit
```

## C++ Integration

### Server-Initialisierung

```cpp
// In main.cpp oder server_init.cpp

#include "utils/capability_auto_generator.h"

// 1. Config laden
auto cap_gen_config = themis::util::CapabilityAutoGenerator::Config::loadFromYAML(
    "config/capability_auto_generation.yaml"
);

// 2. Auto-Generator erstellen
auto capability_auto_gen = std::make_shared<themis::util::CapabilityAutoGenerator>(
    cap_gen_config,
    shard_topology  // Shared pointer zur ShardTopology
);

// 3. Background-Thread starten
if (cap_gen_config.enabled) {
    capability_auto_gen->start();
    LOG(INFO) << "Capability auto-generator started";
}

// 4. Bei Server-Shutdown stoppen
// (automatisch im Destructor, oder manuell)
capability_auto_gen->stop();
```

### Manuelle Trigger via Admin-API

```cpp
// In admin_api.cpp

// Endpoint: POST /admin/capability/generate/{shard_id}
void handleCapabilityGenerate(const Request& req) {
    std::string shard_id = req.params["shard_id"];
    bool force = req.query["force"] == "true";
    
    bool success = capability_auto_gen->generateCapability(shard_id, force);
    
    if (success) {
        return Response{200, "Capability generated successfully"};
    } else {
        return Response{500, "Failed to generate capability"};
    }
}

// Endpoint: GET /admin/capability/stats
void handleCapabilityStats(const Request& req) {
    auto stats = capability_auto_gen->getStatistics();
    return Response{200, stats.dump()};
}
```

## Funktionsweise

### 1. Periodisches Scanning

```
Zeit    0    30m   1h   1.5h  2h   2.5h  3h
        │     │    │     │    │     │    │
Real:   ■─────■────■─────■────■─────■────■  (alle 30 min)
High:   ■────────────────■──────────────── (alle 3h)
Normal: ■────────────────────────────────  (täglich)
Static: ■──────────────────────────────... (wöchentlich)
```

### 2. RocksDB-Analyse

```cpp
// Pseudo-Code der Analyse
AnalysisResult analyzeShardData(shard_id, data_path) {
    db = RocksDB::open(data_path, read_only=true);
    
    for (doc in db) {
        // Sampling: Nur jedes 100. Dokument
        if (doc_count % sampling_rate != 0) continue;
        
        // Metadaten extrahieren
        if (doc.has("type"))         → data_types
        if (doc.has("organization")) → organizations
        if (doc.has("region"))       → regions
        
        // Keywords extrahieren (TF-IDF)
        text = doc["title"] + doc["description"] + doc["content"]
        keywords = extractKeywords(text)
    }
    
    return AnalysisResult{
        keywords: top_100_by_frequency,
        data_types: unique(data_types),
        organizations: unique(organizations),
        regions: unique(regions),
        document_count: total_docs,
        total_size_bytes: total_size
    }
}
```

### 3. Change-Detection

```cpp
bool shouldUpdate(shard, current_analysis) {
    // Keine vorherige Capability → immer updaten
    if (shard.capability.isEmpty()) return true;
    
    // Keyword-Änderung berechnen (Jaccard-Distance)
    old_keywords = set(shard.capability.keywords)
    new_keywords = set(current_analysis.keywords)
    
    intersection = old_keywords ∩ new_keywords
    union = old_keywords ∪ new_keywords
    
    similarity = |intersection| / |union|
    change = 1.0 - similarity
    
    // Threshold check
    schedule = getScheduleForShard(shard)
    return change >= schedule.min_keyword_change
}
```

### 4. Capability-Generierung mit Audit

```cpp
DomainCapability generateFromAnalysis(result, previous) {
    capability = {
        domains: result.domains,
        organizations: result.organizations,
        regions: result.regions,
        data_types: result.data_types,
        keywords: result.keywords
    }
    
    // Preserve manuelle Edits
    if (previous) {
        for (domain in previous.domains) {
            if (domain not in capability.domains) {
                capability.domains.add(domain)  // Keep manual addition
            }
        }
    }
    
    return capability
}

// Mit Audit-Trail
audit_trail = {
    generation_method: "auto-generated",
    generated_at: now(),
    generated_by: "system",
    previous_version: "1.4.2",
    change_summary: "Added 12 keywords; Document count: +212",
    signature: sha256(capability)
}
```

## Update-Frequenzen

### Empfohlene Zeitpläne

| Shard-Typ | Frequenz | Beispiel | Begründung |
|-----------|----------|----------|------------|
| **Real-time** | 30 Min | Gesundheitsdaten, COVID-Dashboard | Daten ändern sich ständig |
| **High-frequency** | 3 Stunden | Bauanträge, Verwaltungsvorgänge | Mehrmals täglich neue Dokumente |
| **Normal** | 24 Stunden | Rechtsdatenbank, Unternehmensarchive | Tägliche Aktualisierungen ausreichend |
| **Static** | 7 Tage | Historische Daten, Referenzdaten | Selten Änderungen |
| **Critical** | 12 Stunden + Review | Finanzdaten, medizinische Kerndaten | Sicherheit > Frequenz |

### Adaptive Frequenz (Optional)

```cpp
// Experimentell: Frequenz basierend auf Änderungsrate anpassen
void adaptSchedule(shard_id, recent_changes) {
    avg_change_rate = sum(recent_changes) / recent_changes.size()
    
    if (avg_change_rate > 0.10) {  // 10% Änderung/Tag
        // Mehr Updates nötig
        schedule.interval_seconds *= 0.5
    } else if (avg_change_rate < 0.01) {  // <1% Änderung/Tag
        // Weniger Updates ausreichend
        schedule.interval_seconds *= 2.0
    }
}
```

## Auditierung

### 1. Audit-Log Format

```json
{
  "timestamp": "2026-02-10T15:30:00Z",
  "shard_id": "shard_hamburg_bauamt_001",
  "version": "1.4.3",
  "status": "success",
  "change_summary": "Added 12 keywords; Document count: 1247893 → 1248105 (+212)",
  "signature": "a3f5d8c9e2b1f4a7d6c5b8e9f1a2d3c4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0",
  "keywords_added": ["sanierungsgenehmigung", "denkmalschutz", ...],
  "keywords_removed": ["outdated_term"],
  "generation_time_ms": 1247,
  "documents_analyzed": 12479,
  "require_review": false
}
```

### 2. YAML Audit-Sektion

```yaml
# In der generierten capability.yaml
audit_trail:
  generation_method: auto-generated
  generated_at: "2026-02-10T15:30:00Z"
  generated_by: system
  previous_version: "1.4.2"
  change_summary: "Added 12 keywords; Document count +212"
  signature: a3f5d8c9e2b1f4a7...
  
  # Change-Details
  changes:
    keywords_added: 12
    keywords_removed: 1
    data_types_added: 0
    organizations_added: 1
  
  # Validierung
  validated: true
  validation_timestamp: "2026-02-10T15:30:05Z"
```

### 3. Signatur-Verifikation

```cpp
bool verifyCapabilitySignature(capability, signature) {
    // Berechne Signatur neu
    content = serialize(capability)
    expected_signature = sha256(content)
    
    // Vergleiche
    return signature == expected_signature
}
```

## Admin-API Endpoints

### Neue Endpoints

```
POST   /admin/capability/generate/{shard_id}?force=true
       → Manuell Capability-Generierung triggern

GET    /admin/capability/stats
       → Statistiken abrufen

GET    /admin/capability/schedule
       → Aktuellen Zeitplan anzeigen

PUT    /admin/capability/config
       → Konfiguration zur Laufzeit ändern

GET    /admin/capability/audit/{shard_id}
       → Audit-Log für Shard abrufen
```

### Beispiel-Aufrufe

```bash
# Manuell generieren (force)
curl -X POST http://localhost:8080/admin/capability/generate/shard_hamburg_bauamt_001?force=true

# Statistiken
curl http://localhost:8080/admin/capability/stats
{
  "total_generations": 247,
  "successful_generations": 245,
  "failed_generations": 2,
  "auto_approved": 230,
  "manual_review_required": 15,
  "running": true
}

# Config zur Laufzeit ändern
curl -X PUT http://localhost:8080/admin/capability/config \
  -H "Content-Type: application/json" \
  -d '{
    "schedules": {
      "real-time": {"interval_seconds": 900}
    }
  }'
```

## Monitoring

### Prometheus Metriken

```
# Anzahl Generierungen
themis_capability_generations_total{shard_type="real-time"} 247

# Dauer der Generierung
themis_capability_generation_duration_seconds{shard_id="shard_001"} 1.247

# Fehler
themis_capability_generation_failures_total 2

# Letzte Aktualisierung
themis_capability_last_update_timestamp{shard_id="shard_001"} 1707574200

# Extrahierte Keywords
themis_capability_keywords_extracted{shard_id="shard_001"} 87

# Analysierte Dokumente
themis_capability_documents_analyzed{shard_id="shard_001"} 12479
```

### Grafana Dashboard

```
- Panel 1: Generierungen pro Stunde (nach Shard-Typ)
- Panel 2: Success Rate (%)
- Panel 3: Durchschnittliche Generierungszeit
- Panel 4: Keywords pro Shard (Trend)
- Panel 5: Review-Queue Länge
```

## Best Practices

### 1. Entwicklungsphase

```yaml
# Dev-Config: Häufige Updates, kein Review
enabled: true
schedules:
  normal:
    interval_seconds: 300      # Alle 5 Minuten (für Testing)
    min_keyword_change: 0.01   # 1% (sensitiv)
    require_review: false
```

### 2. Produktionsphase

```yaml
# Prod-Config: Konservativ, mit Review
enabled: true
schedules:
  normal:
    interval_seconds: 86400    # Täglich
    min_keyword_change: 0.05   # 5% (stabil)
    require_review: true       # Review für Sicherheit
```

### 3. Hochlast-Szenarien

```yaml
# Performance-Optimierung
rocksdb_analysis:
  sampling_rate: 1000          # Nur jedes 1000. Dokument
  max_keywords: 50             # Weniger Keywords
  
performance:
  max_concurrent_analyses: 2   # Weniger parallel
  inter_analysis_delay_ms: 5000  # 5s Pause zwischen Shards
```

## Zusammenfassung

Die Capability-Auto-Generierung ist nun vollständig in ThemisDB integriert:

✅ **C++ Native**: Im `themis::util` Namespace  
✅ **YAML Config**: Flexibel und wartbar  
✅ **Background Thread**: Automatische Updates nach Schedule  
✅ **RocksDB Integration**: Direkte Datenanalyse  
✅ **Audit Trail**: Vollständig nachvollziehbar  
✅ **Produktionsreif**: Security, Monitoring, Error-Handling  

**Die Grätchen-Frage ist gelöst!** 🎉
