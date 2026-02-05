---
name: Strukturiertes Event-Logging
about: Strukturiertes Logging (JSON) für Pipeline-Events mit Correlation-IDs
title: '[MONITORING] Strukturiertes Event-Logging'
labels: ['enhancement', 'future', 'content-pipeline', 'monitoring', 'observability', 'priority:medium']
assignees: ''
---

## Beschreibung

Implementierung von strukturiertem Logging (JSON-Format) für alle Content Pipeline Events mit Correlation-IDs zur Verfolgung von Requests und Integration mit Log-Aggregation-Systemen (ELK, Loki).

## Kontext

Unstrukturierte Logs sind schwer zu parsen und zu analysieren. Strukturiertes Logging ermöglicht effiziente Log-Aggregation, Suche und Korrelation mit Traces und Metriken.

## Ziele

- Strukturiertes JSON-Logging für alle Pipeline-Events
- Correlation-ID-Propagation für Request-Tracking
- Integration mit spdlog oder ähnlicher Library
- Log-Level-Management und Konfiguration
- Integration mit ELK-Stack oder Grafana Loki

## Lösungsansatz

### Schritt 1: Structured-Logger-Integration (0.5 Tag)
- **Tasks**:
  - [ ] spdlog mit JSON-Support integrieren
  - [ ] Logger-Factory und -Konfiguration
  - [ ] Custom-Formatter für JSON-Output
  - [ ] Log-Level-Management (DEBUG, INFO, WARN, ERROR)

### Schritt 2: Event-Schema-Definition (0.5 Tag)
- **Tasks**:
  - [ ] Base-Event-Schema definieren:
    - `timestamp` - ISO 8601 Format
    - `level` - Log-Level (debug, info, warn, error)
    - `message` - Human-readable Message
    - `correlation_id` - Request-Tracking-ID
    - `trace_id` - OpenTelemetry Trace-ID (optional)
    - `service` - Service-Name
    - `component` - Pipeline-Component
    - `event_type` - Event-Category
  - [ ] Component-spezifische Event-Schemas:
    - Upload-Events
    - Chunking-Events
    - Compression-Events
    - Storage-Events
    - Error-Events

### Schritt 3: Correlation-ID-Propagation (1 Tag)
- **Tasks**:
  - [ ] Correlation-ID-Generation (UUID v4)
  - [ ] Context-Propagation durch Thread-Pool
  - [ ] Context-Propagation durch Async-Operations
  - [ ] HTTP-Header-Integration (`X-Correlation-ID`)
  - [ ] Integration mit OpenTelemetry Trace-ID

### Schritt 4: Log-Aggregation-Setup (1 Tag)
- **Tasks**:
  - [ ] ELK-Stack oder Loki-Setup (Docker Compose)
  - [ ] Log-Shipper-Konfiguration (Fluent Bit, Promtail)
  - [ ] Index-Pattern und -Mapping
  - [ ] Retention-Policy-Konfiguration
  - [ ] Kibana/Grafana-Dashboard-Setup

## Strukturiertes Logging-API

```cpp
// Structured Logger
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <nlohmann/json.hpp>

class StructuredLogger {
public:
    // Event-Level Logging
    void LogInfo(const std::string& component, 
                 const std::string& event_type,
                 const nlohmann::json& data) {
        nlohmann::json log_entry = {
            {"timestamp", GetISO8601Timestamp()},
            {"level", "info"},
            {"message", data.value("message", "")},
            {"correlation_id", GetCorrelationID()},
            {"trace_id", GetTraceID()},
            {"service", "themisdb-content-pipeline"},
            {"component", component},
            {"event_type", event_type},
            {"data", data}
        };
        
        logger_->info(log_entry.dump());
    }
    
    void LogError(const std::string& component,
                  const std::string& event_type,
                  const std::exception& error,
                  const nlohmann::json& data) {
        nlohmann::json log_entry = {
            {"timestamp", GetISO8601Timestamp()},
            {"level", "error"},
            {"message", error.what()},
            {"correlation_id", GetCorrelationID()},
            {"trace_id", GetTraceID()},
            {"service", "themisdb-content-pipeline"},
            {"component", component},
            {"event_type", event_type},
            {"error", {
                {"type", typeid(error).name()},
                {"message", error.what()}
            }},
            {"data", data}
        };
        
        logger_->error(log_entry.dump());
    }
    
private:
    std::shared_ptr<spdlog::logger> logger_;
    
    std::string GetCorrelationID() {
        // Get from thread-local storage
        return correlation_context_.GetID();
    }
    
    std::string GetTraceID() {
        // Get from OpenTelemetry context if available
        return otel_context_.GetTraceID();
    }
};

// Usage in ContentManager
class ContentManager {
public:
    Status UploadContent(const ContentData& data) {
        auto correlation_id = GenerateCorrelationID();
        CorrelationContext::Set(correlation_id);
        
        // Log upload start
        logger_->LogInfo("ContentManager", "upload.started", {
            {"content_type", data.type},
            {"content_size", data.size},
            {"user_id", data.user_id}
        });
        
        try {
            // Process upload...
            auto chunks = ChunkContent(data);
            
            // Log chunking completed
            logger_->LogInfo("ContentManager", "chunking.completed", {
                {"chunk_count", chunks.size()},
                {"chunk_strategy", "text_fixed_size"}
            });
            
            // Log upload completed
            logger_->LogInfo("ContentManager", "upload.completed", {
                {"content_id", content_id},
                {"duration_ms", elapsed_ms}
            });
            
        } catch (const std::exception& e) {
            // Log error
            logger_->LogError("ContentManager", "upload.failed", e, {
                {"content_type", data.type},
                {"content_size", data.size}
            });
            throw;
        }
        
        return Status::OK();
    }
};
```

## Event-Schema-Beispiele

### Upload-Started Event
```json
{
  "timestamp": "2026-02-05T16:30:00.123Z",
  "level": "info",
  "message": "Content upload started",
  "correlation_id": "7d8e4c3a-2b1f-0e9d-8c7b-6a5f4e3d2c1b",
  "trace_id": "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6",
  "service": "themisdb-content-pipeline",
  "component": "ContentManager",
  "event_type": "upload.started",
  "data": {
    "content_type": "video",
    "content_size": 52428800,
    "user_id": "user-123",
    "upload_source": "web_ui"
  }
}
```

### Chunking-Completed Event
```json
{
  "timestamp": "2026-02-05T16:30:00.243Z",
  "level": "info",
  "message": "Content chunking completed",
  "correlation_id": "7d8e4c3a-2b1f-0e9d-8c7b-6a5f4e3d2c1b",
  "trace_id": "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6",
  "service": "themisdb-content-pipeline",
  "component": "ContentManager",
  "event_type": "chunking.completed",
  "data": {
    "chunk_count": 25,
    "chunk_strategy": "video_frame_based",
    "avg_chunk_size": 2097152,
    "duration_ms": 120
  }
}
```

### Upload-Failed Event
```json
{
  "timestamp": "2026-02-05T16:30:00.543Z",
  "level": "error",
  "message": "Content upload failed: Compression error",
  "correlation_id": "7d8e4c3a-2b1f-0e9d-8c7b-6a5f4e3d2c1b",
  "trace_id": "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6",
  "service": "themisdb-content-pipeline",
  "component": "ContentManager",
  "event_type": "upload.failed",
  "error": {
    "type": "CompressionException",
    "message": "ZSTD compression failed: buffer too small",
    "stack_trace": "..."
  },
  "data": {
    "content_type": "video",
    "content_size": 52428800,
    "stage": "compression",
    "retry_count": 3
  }
}
```

## Correlation-ID-Propagation

```cpp
// Thread-local Correlation Context
class CorrelationContext {
public:
    static void Set(const std::string& correlation_id) {
        thread_local_id_ = correlation_id;
    }
    
    static std::string Get() {
        if (thread_local_id_.empty()) {
            thread_local_id_ = GenerateCorrelationID();
        }
        return thread_local_id_;
    }
    
private:
    static thread_local std::string thread_local_id_;
    
    static std::string GenerateCorrelationID() {
        // Generate UUID v4
        return boost::uuids::to_string(boost::uuids::random_generator()());
    }
};

// Propagation through Thread-Pool
class AsyncBulkUploader {
private:
    void ProcessUpload(const ContentData& data) {
        auto correlation_id = CorrelationContext::Get();
        
        thread_pool_.Submit([this, data, correlation_id]() {
            // Set context in worker thread
            CorrelationContext::Set(correlation_id);
            
            // All subsequent logs will have the same correlation_id
            logger_->LogInfo("AsyncBulkUploader", "processing.started", {
                {"content_id", data.id}
            });
        });
    }
};
```

## Konfiguration

```yaml
# config/logging.yml
logging:
  level: info  # debug, info, warn, error
  format: json  # json or text
  
  output:
    console:
      enabled: true
      level: info
    
    file:
      enabled: true
      path: "/var/log/themisdb/content-pipeline.log"
      rotation:
        max_size: 100MB
        max_files: 10
    
    syslog:
      enabled: false
      host: "localhost"
      port: 514
  
  structured:
    service_name: "themisdb-content-pipeline"
    include_trace_id: true
    include_correlation_id: true
  
  sampling:
    enabled: false  # Sample logs for high-throughput
    rate: 0.1  # Sample 10% of logs
```

## Log-Aggregation-Setup

```yaml
# docker-compose.yml
version: '3'
services:
  loki:
    image: grafana/loki:latest
    ports:
      - "3100:3100"
    volumes:
      - ./loki-config.yml:/etc/loki/local-config.yaml

  promtail:
    image: grafana/promtail:latest
    volumes:
      - /var/log:/var/log
      - ./promtail-config.yml:/etc/promtail/config.yml
    command: -config.file=/etc/promtail/config.yml

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    depends_on:
      - loki
```

```yaml
# promtail-config.yml
server:
  http_listen_port: 9080
  grpc_listen_port: 0

positions:
  filename: /tmp/positions.yaml

clients:
  - url: http://loki:3100/loki/api/v1/push

scrape_configs:
  - job_name: content-pipeline
    static_configs:
      - targets:
          - localhost
        labels:
          job: content-pipeline
          __path__: /var/log/themisdb/content-pipeline.log
    
    pipeline_stages:
      - json:
          expressions:
            level: level
            component: component
            event_type: event_type
            correlation_id: correlation_id
      - labels:
          level:
          component:
          event_type:
```

## Integration Points

- [ ] ContentManager (alle Upload-Events)
- [ ] ChunkStrategy-Implementierungen (Chunking-Events)
- [ ] ZstdCompression (Compression-Events)
- [ ] StorageManager (Storage-Events)
- [ ] ErrorHandler (Error-Events)
- [ ] AsyncBulkUploader (Async-Processing-Events)

## Testing-Anforderungen

### Unit-Tests
```cpp
TEST(StructuredLogging, JSONFormat) {
    // Test JSON output format
}

TEST(StructuredLogging, CorrelationID) {
    // Test correlation ID propagation
}

TEST(StructuredLogging, ErrorLogging) {
    // Test error event structure
}

TEST(StructuredLogging, ContextPropagation) {
    // Test context propagation through threads
}
```

### Integration-Tests
- [ ] Logs in JSON-Format geschrieben
- [ ] Correlation-ID in allen Logs eines Requests
- [ ] Trace-ID korrekt wenn OpenTelemetry aktiv
- [ ] Log-Rotation funktioniert
- [ ] Log-Aggregation (Loki/ELK) erhält Logs
- [ ] Logs durchsuchbar nach Correlation-ID

## Success Criteria

- [ ] Strukturiertes JSON-Logging implementiert
- [ ] Correlation-ID-Propagation funktioniert
- [ ] Event-Schemas für alle wichtigen Events definiert
- [ ] Integration mit Log-Aggregation funktioniert
- [ ] Logs durchsuchbar und filterbar
- [ ] Performance-Impact < 1% auf Durchsatz
- [ ] Unit- und Integration-Tests bestehen
- [ ] Dokumentation vollständig

## Priorität

**Mittel** - Wichtig für Production-Debugging und Troubleshooting

## Geschätzter Aufwand

**2-3 Tage**

## Dependencies

- **Benötigt**: spdlog oder ähnliche Logging-Library
- **Benötigt**: JSON-Library (nlohmann/json)
- **Optional**: OpenTelemetry (für Trace-ID-Integration)
- **Optional**: Log-Aggregation-System (Loki, ELK)
- **Related**: OpenTelemetry-Tracing (Trace-ID-Correlation)

## Referenzen

- [ ] spdlog Documentation: https://github.com/gabime/spdlog
- [ ] Structured Logging Best Practices: https://www.loggly.com/ultimate-guide/structured-logging/
- [ ] Grafana Loki: https://grafana.com/oss/loki/
- [ ] ELK Stack: https://www.elastic.co/elastic-stack
- [ ] ThemisDB Logging-Architektur: `docs/monitoring/logging.md`

---

**Checklist:**
- [ ] Ich habe die Anforderungen verstanden
- [ ] Ich habe Event-Schemas definiert
- [ ] Ich habe Correlation-ID-Strategie geplant
- [ ] Ich habe Log-Aggregation-Setup geplant
- [ ] Ich habe Testing-Anforderungen definiert
- [ ] Ich habe Success-Criteria festgelegt
