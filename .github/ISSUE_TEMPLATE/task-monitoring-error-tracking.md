---
name: Error-Rate und Error-Type-Tracking
about: Detailliertes Error-Tracking mit Kategorisierung und Alerting
title: '[MONITORING] Error-Rate und Error-Type-Tracking'
labels: ['enhancement', 'future', 'content-pipeline', 'monitoring', 'stability', 'priority:high']
assignees: ''
---

## Beschreibung

Implementierung eines umfassenden Error-Tracking-Systems mit Kategorisierung, Rate-Monitoring und Alerting für Content Pipeline Operations.

## Kontext

Fehler-Monitoring ist kritisch für Production-Stabilität. Kategorisiertes Error-Tracking ermöglicht schnelle Diagnose, priorisierte Fehlerbehandlung und proaktives Alerting.

## Ziele

- Strukturierte Error-Taxonomie definieren
- Error-Counter per Category und Type
- Error-Rate-Berechnung und -Monitoring
- Alerting-Rules für kritische Error-Rates
- Error-Dashboard mit Drill-Down-Fähigkeit

## Lösungsansatz

### Schritt 1: Error-Taxonomy-Definition (0.5 Tag)
- **Tasks**:
  - [ ] Error-Kategorien definieren:
    - **Transient Errors** (temporär, retry-fähig)
    - **Permanent Errors** (permanent, nicht retry-fähig)
    - **Client Errors** (user/client-verursacht)
    - **Server Errors** (server/system-verursacht)
  - [ ] Error-Types pro Kategorie:
    - Upload-Errors
    - Chunking-Errors
    - Compression-Errors
    - Storage-Errors
    - Validation-Errors
    - Resource-Errors (Memory, Disk)
    - Network-Errors
  - [ ] Error-Severity-Levels:
    - CRITICAL - System-Down
    - ERROR - Operation-Failed
    - WARNING - Degraded-Performance
    - INFO - Recoverable-Issue

### Schritt 2: Error-Counter per Category (1 Tag)
- **Tasks**:
  - [ ] Prometheus-Counter für Errors
  - [ ] Labels für Kategorisierung:
    - `error_category` (transient, permanent, client, server)
    - `error_type` (upload, chunking, compression, etc.)
    - `error_code` (specific error code)
    - `component` (ContentManager, ZstdCompression, etc.)
  - [ ] Error-Recording-API
  - [ ] Integration in alle Pipeline-Komponenten

### Schritt 3: Alerting-Rules für Error-Rates (1 Tag)
- **Tasks**:
  - [ ] Alert: High Overall Error-Rate (> 5% in 5min)
  - [ ] Alert: High Component Error-Rate (> 10% in 5min)
  - [ ] Alert: Critical Error Spike (> 100% increase in 5min)
  - [ ] Alert: Storage Error-Rate (> 1% in 5min)
  - [ ] Alert: Resource-Exhaustion Errors
  - [ ] Alert-Routing und -Escalation
  - [ ] PagerDuty/Slack-Integration

### Schritt 4: Error-Dashboard (0.5-1 Tag)
- **Tasks**:
  - [ ] Overview: Total Error-Rate
  - [ ] Breakdown: Errors by Category
  - [ ] Breakdown: Errors by Type
  - [ ] Breakdown: Errors by Component
  - [ ] Time-Series: Error-Rate-Trend
  - [ ] Table: Top Error Messages
  - [ ] Drill-Down zu einzelnen Errors (via Logs)

## Error-Taxonomy

```cpp
enum class ErrorCategory {
    TRANSIENT,    // Temporary, can retry
    PERMANENT,    // Permanent, cannot retry
    CLIENT,       // Client/user error
    SERVER        // Server/system error
};

enum class ErrorType {
    UPLOAD_ERROR,
    CHUNKING_ERROR,
    COMPRESSION_ERROR,
    STORAGE_ERROR,
    VALIDATION_ERROR,
    RESOURCE_ERROR,
    NETWORK_ERROR,
    TIMEOUT_ERROR,
    AUTHORIZATION_ERROR
};

enum class ErrorSeverity {
    CRITICAL,     // System down
    ERROR,        // Operation failed
    WARNING,      // Degraded performance
    INFO          // Recoverable issue
};

struct ErrorDetails {
    ErrorCategory category;
    ErrorType type;
    ErrorSeverity severity;
    std::string error_code;      // e.g., "COMPRESS_001"
    std::string component;       // e.g., "ZstdCompression"
    std::string message;         // Human-readable message
    std::string correlation_id;  // For log correlation
    std::string trace_id;        // For trace correlation
    std::map<std::string, std::string> metadata;
};
```

## Error-Tracking-API

```cpp
class ErrorTracker {
public:
    void RecordError(const ErrorDetails& error) {
        // Increment Prometheus counter
        error_counter_
            .Labels({
                {"category", CategoryToString(error.category)},
                {"type", TypeToString(error.type)},
                {"severity", SeverityToString(error.severity)},
                {"error_code", error.error_code},
                {"component", error.component}
            })
            .Increment();
        
        // Log structured error
        logger_->LogError(error.component, TypeToString(error.type), {
            {"error_code", error.error_code},
            {"message", error.message},
            {"category", CategoryToString(error.category)},
            {"severity", SeverityToString(error.severity)},
            {"correlation_id", error.correlation_id},
            {"trace_id", error.trace_id},
            {"metadata", error.metadata}
        });
        
        // Update error-rate tracker
        error_rate_tracker_.RecordError(error);
    }
    
    double GetErrorRate(std::chrono::seconds window) {
        return error_rate_tracker_.GetRate(window);
    }
    
    std::map<ErrorType, uint64_t> GetErrorsByType(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {
        return error_rate_tracker_.GetErrorsByType(start, end);
    }

private:
    prometheus::Counter& error_counter_;
    StructuredLogger* logger_;
    ErrorRateTracker error_rate_tracker_;
};

// Usage in ContentManager
class ContentManager {
public:
    Status UploadContent(const ContentData& data) {
        try {
            // ... upload logic ...
        } catch (const CompressionException& e) {
            ErrorDetails error{
                .category = ErrorCategory::SERVER,
                .type = ErrorType::COMPRESSION_ERROR,
                .severity = ErrorSeverity::ERROR,
                .error_code = "COMPRESS_001",
                .component = "ContentManager",
                .message = e.what(),
                .correlation_id = GetCorrelationID(),
                .trace_id = GetTraceID(),
                .metadata = {
                    {"content_type", data.type},
                    {"content_size", std::to_string(data.size)}
                }
            };
            
            error_tracker_->RecordError(error);
            return Status::Error(e.what());
        }
    }
};
```

## Prometheus-Metriken

```prometheus
# Error Counter
content_pipeline_errors_total{
  category="transient",
  type="compression_error",
  severity="error",
  error_code="COMPRESS_001",
  component="ZstdCompression"
} 42

content_pipeline_errors_total{
  category="client",
  type="validation_error",
  severity="warning",
  error_code="VALIDATE_002",
  component="ContentManager"
} 128

# Error Rate (calculated)
content_pipeline_error_rate{window="5m"} 0.023  # 2.3%
content_pipeline_error_rate{window="1h"} 0.015  # 1.5%

# Errors by Type
content_pipeline_errors_by_type{type="compression_error"} 42
content_pipeline_errors_by_type{type="storage_error"} 18
content_pipeline_errors_by_type{type="validation_error"} 128
```

## Alerting-Rules

```yaml
# alerts/error_tracking.yml
groups:
  - name: error_tracking_alerts
    interval: 1m
    rules:
      - alert: HighOverallErrorRate
        expr: |
          rate(content_pipeline_errors_total[5m]) / 
          rate(content_pipeline_uploads_total[5m]) > 0.05
        for: 5m
        labels:
          severity: critical
          component: content-pipeline
        annotations:
          summary: "High overall error rate in content pipeline"
          description: "Error rate is {{ $value | humanizePercentage }}"

      - alert: HighComponentErrorRate
        expr: |
          rate(content_pipeline_errors_total{component=~".+"}[5m]) > 0.10
        for: 5m
        labels:
          severity: warning
          component: "{{ $labels.component }}"
        annotations:
          summary: "High error rate in {{ $labels.component }}"
          description: "Error rate is {{ $value | humanizePercentage }}"

      - alert: ErrorRateSpike
        expr: |
          (rate(content_pipeline_errors_total[5m]) - 
           rate(content_pipeline_errors_total[5m] offset 10m)) /
          rate(content_pipeline_errors_total[5m] offset 10m) > 1.0
        for: 2m
        labels:
          severity: critical
          component: content-pipeline
        annotations:
          summary: "Error rate spike detected"
          description: "Error rate increased by {{ $value | humanizePercentage }}"

      - alert: StorageErrorRate
        expr: |
          rate(content_pipeline_errors_total{type="storage_error"}[5m]) > 0.01
        for: 5m
        labels:
          severity: critical
          component: storage
        annotations:
          summary: "High storage error rate"
          description: "Storage error rate is {{ $value | humanizePercentage }}"

      - alert: ResourceExhaustionErrors
        expr: |
          rate(content_pipeline_errors_total{type="resource_error"}[5m]) > 0
        for: 2m
        labels:
          severity: critical
          component: content-pipeline
        annotations:
          summary: "Resource exhaustion errors detected"
          description: "{{ $value }} resource errors per second"

      - alert: CriticalErrorsDetected
        expr: |
          rate(content_pipeline_errors_total{severity="critical"}[5m]) > 0
        for: 1m
        labels:
          severity: critical
          component: content-pipeline
        annotations:
          summary: "Critical errors detected"
          description: "{{ $value }} critical errors per second"
```

## Error-Dashboard

```
┌─────────────────────────────────────────────────────────┐
│         Content Pipeline Error Tracking                 │
├─────────────────────────────────────────────────────────┤
│ Overall Error Rate  | Errors/Min | Total Errors Today   │
│      2.3%           |    12      |      1,234          │
├─────────────────────────────────────────────────────────┤
│              Error Rate Over Time                        │
│              [Time Series Graph]                         │
├─────────────────────────────────────────────────────────┤
│ Errors by Category        | Errors by Type              │
│  [Pie Chart]              | [Bar Chart]                 │
│  - Transient: 45%         | - Validation: 512           │
│  - Client: 35%            | - Compression: 42           │
│  - Server: 15%            | - Storage: 18               │
│  - Permanent: 5%          | - Network: 8                │
├─────────────────────────────────────────────────────────┤
│ Errors by Component       | Top Error Messages          │
│  [Bar Chart]              | [Table]                     │
├─────────────────────────────────────────────────────────┤
│              Error Rate by Severity                      │
│              [Time Series - Stacked]                     │
└─────────────────────────────────────────────────────────┘
```

## Konfiguration

```yaml
# config/error_tracking.yml
error_tracking:
  enabled: true
  
  taxonomy:
    categories:
      - transient
      - permanent
      - client
      - server
    
    types:
      - upload_error
      - chunking_error
      - compression_error
      - storage_error
      - validation_error
      - resource_error
      - network_error
  
  alerting:
    enabled: true
    rules:
      high_error_rate:
        threshold: 0.05  # 5%
        window: 5m
        severity: critical
      
      error_rate_spike:
        threshold: 1.0  # 100% increase
        window: 5m
        severity: critical
  
  notifications:
    slack:
      enabled: true
      webhook_url: "${SLACK_WEBHOOK_URL}"
      channel: "#ops-alerts"
    
    pagerduty:
      enabled: true
      integration_key: "${PAGERDUTY_KEY}"
      severity: critical
```

## Integration Points

- [ ] ContentManager (alle Error-Cases)
- [ ] ChunkStrategy-Implementierungen (Chunking-Errors)
- [ ] ZstdCompression (Compression-Errors)
- [ ] StorageManager (Storage-Errors)
- [ ] ValidationManager (Validation-Errors)
- [ ] AsyncBulkUploader (Upload-Errors)
- [ ] Prometheus-Exporter (Metriken-Export)
- [ ] StructuredLogger (Error-Logging)

## Testing-Anforderungen

### Unit-Tests
```cpp
TEST(ErrorTracker, RecordError) {
    // Test error recording
}

TEST(ErrorTracker, ErrorRateCalculation) {
    // Test error rate calculation
}

TEST(ErrorTracker, ErrorsByType) {
    // Test aggregation by type
}

TEST(ErrorTracker, AlertingThresholds) {
    // Test alerting threshold detection
}
```

### Integration-Tests
- [ ] Errors werden korrekt kategorisiert
- [ ] Prometheus-Counter werden inkrementiert
- [ ] Error-Logs enthalten alle Details
- [ ] Alerting-Rules triggern korrekt
- [ ] Dashboard zeigt korrekte Daten
- [ ] Notifications werden versendet

## Success Criteria

- [ ] Error-Taxonomy definiert und implementiert
- [ ] Error-Tracking in allen Komponenten integriert
- [ ] Prometheus-Metriken exportiert
- [ ] Alerting-Rules konfiguriert und getestet
- [ ] Error-Dashboard funktioniert
- [ ] Notifications funktionieren (Slack, PagerDuty)
- [ ] Error-Rate-Berechnung korrekt
- [ ] Performance-Impact < 0.5%
- [ ] Unit- und Integration-Tests bestehen
- [ ] Dokumentation vollständig

## Priorität

**Hoch** - Kritisch für Production-Stabilität und schnelle Fehlerdiagnose

## Geschätzter Aufwand

**2-3 Tage**

## Dependencies

- **Benötigt**: Prometheus-Metriken für Content Pipeline
- **Benötigt**: Strukturiertes Event-Logging
- **Optional**: OpenTelemetry (für Trace-Correlation)
- **Optional**: Grafana (für Dashboard)
- **Optional**: Slack/PagerDuty (für Notifications)
- **Related**: Grafana-Dashboards (für Visualisierung)

## Referenzen

- [ ] Error-Tracking-Best-Practices
- [ ] Alerting-Best-Practices: https://prometheus.io/docs/practices/alerting/
- [ ] PagerDuty-Integration: https://www.pagerduty.com/docs/
- [ ] ThemisDB Error-Handling: `docs/development/error_handling.md`

---

**Checklist:**
- [ ] Ich habe Error-Taxonomy definiert
- [ ] Ich habe Error-Tracking-API geplant
- [ ] Ich habe Alerting-Rules definiert
- [ ] Ich habe Dashboard-Layout geplant
- [ ] Ich habe Notification-Strategie definiert
- [ ] Ich habe Testing-Anforderungen definiert
