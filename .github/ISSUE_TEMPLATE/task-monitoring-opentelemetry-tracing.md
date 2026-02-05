---
name: OpenTelemetry-Tracing für Upload-Pipelines
about: Distributed-Tracing-Integration für End-to-End-Visibility der Content Pipeline
title: '[MONITORING] OpenTelemetry-Tracing für Upload-Pipelines'
labels: ['enhancement', 'future', 'content-pipeline', 'monitoring', 'observability', 'priority:medium']
assignees: ''
---

## Beschreibung

Integration von OpenTelemetry für Distributed Tracing in der Content Pipeline, um End-to-End-Visibility vom Upload-Request bis zur finalen Speicherung zu ermöglichen.

## Kontext

Während Metriken aggregierte Statistiken bieten, ermöglicht Distributed Tracing die Verfolgung einzelner Requests durch das System. Dies ist essentiell für Performance-Debugging und Bottleneck-Identifikation.

## Ziele

- OpenTelemetry SDK-Integration in C++
- Span-Creation für alle wichtigen Pipeline-Stages
- Trace-Context-Propagation durch asynchrone Operations
- Integration mit Jaeger oder Zipkin für Visualisierung

## Lösungsansatz

### Schritt 1: OpenTelemetry-SDK-Integration (1 Tag)
- **Tasks**:
  - [ ] OpenTelemetry C++ SDK einbinden (vcpkg/CMake)
  - [ ] Tracer-Provider-Konfiguration
  - [ ] OTLP-Exporter-Setup (gRPC oder HTTP)
  - [ ] Span-Processor-Konfiguration (Batch vs. Simple)

### Schritt 2: Span-Creation für Pipeline-Stages (1-1.5 Tage)
- **Tasks**:
  - [ ] **Upload-Phase**:
    - Root-Span: `content.upload`
    - Attribute: `content.type`, `content.size`, `user.id`
  - [ ] **Chunking-Phase**:
    - Child-Span: `content.chunking`
    - Attribute: `chunk.strategy`, `chunk.count`, `chunk.size_avg`
  - [ ] **Compression-Phase**:
    - Child-Span: `content.compression`
    - Attribute: `compression.algorithm`, `compression.ratio`, `compression.level`
  - [ ] **Storage-Phase**:
    - Child-Span: `content.storage`
    - Attribute: `storage.backend`, `storage.tier`, `storage.bytes`
  - [ ] **Metadata-Phase**:
    - Child-Span: `content.metadata`
    - Attribute: `metadata.fields`, `metadata.size`

### Schritt 3: Trace-Context-Propagation (0.5-1 Tag)
- **Tasks**:
  - [ ] Context-Propagation durch Thread-Pool
  - [ ] Context-Propagation durch Async-Operations
  - [ ] Baggage für User-Context (User-ID, Tenant-ID)
  - [ ] Trace-ID in Logs für Correlation

### Schritt 4: Backend-Setup und Testing (0.5-1 Tag)
- **Tasks**:
  - [ ] Jaeger/Zipkin-Backend-Setup (Docker Compose)
  - [ ] OTLP-Collector-Konfiguration
  - [ ] Sampling-Strategy-Konfiguration
  - [ ] Integration-Tests mit Trace-Validation

## OpenTelemetry-Integration

```cpp
// Tracer-Setup
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/trace/provider.h>

namespace trace = opentelemetry::trace;
namespace sdk = opentelemetry::sdk::trace;
namespace otlp = opentelemetry::exporter::otlp;

class TracingManager {
public:
    void Initialize(const std::string& service_name, 
                   const std::string& collector_endpoint) {
        // OTLP Exporter
        otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint = collector_endpoint;
        auto exporter = std::make_unique<otlp::OtlpGrpcExporter>(opts);
        
        // Batch Span Processor
        sdk::BatchSpanProcessorOptions processor_opts;
        auto processor = std::make_unique<sdk::BatchSpanProcessor>(
            std::move(exporter), processor_opts);
        
        // Tracer Provider
        auto provider = std::make_shared<sdk::TracerProvider>(
            std::move(processor));
        trace::Provider::SetTracerProvider(provider);
        
        tracer_ = provider->GetTracer(service_name, "1.0.0");
    }
    
    std::shared_ptr<trace::Tracer> GetTracer() { return tracer_; }
    
private:
    std::shared_ptr<trace::Tracer> tracer_;
};

// Span-Usage in ContentManager
class ContentManager {
public:
    Status UploadContent(const ContentData& data) {
        auto tracer = tracing_manager_->GetTracer();
        
        // Root Span
        auto span = tracer->StartSpan("content.upload");
        auto scope = tracer->WithActiveSpan(span);
        
        // Attributes
        span->SetAttribute("content.type", data.type);
        span->SetAttribute("content.size", data.size);
        span->SetAttribute("user.id", data.user_id);
        
        try {
            // Chunking (Child Span)
            {
                auto chunk_span = tracer->StartSpan("content.chunking",
                    {{"parent", span->GetContext()}});
                auto chunk_scope = tracer->WithActiveSpan(chunk_span);
                
                auto chunks = ChunkContent(data);
                chunk_span->SetAttribute("chunk.count", chunks.size());
                chunk_span->SetStatus(trace::StatusCode::kOk);
            }
            
            // Compression (Child Span)
            {
                auto compress_span = tracer->StartSpan("content.compression");
                auto compress_scope = tracer->WithActiveSpan(compress_span);
                
                auto compressed = CompressContent(data);
                compress_span->SetAttribute("compression.ratio", 
                    (double)data.size / compressed.size);
                compress_span->SetStatus(trace::StatusCode::kOk);
            }
            
            // Storage (Child Span)
            {
                auto storage_span = tracer->StartSpan("content.storage");
                auto storage_scope = tracer->WithActiveSpan(storage_span);
                
                StoreContent(compressed);
                storage_span->SetAttribute("storage.backend", "rocksdb");
                storage_span->SetStatus(trace::StatusCode::kOk);
            }
            
            span->SetStatus(trace::StatusCode::kOk);
            return Status::OK();
            
        } catch (const std::exception& e) {
            span->RecordException(e);
            span->SetStatus(trace::StatusCode::kError, e.what());
            throw;
        }
    }
};
```

## Context-Propagation

```cpp
// Context-Propagation durch Thread-Pool
class AsyncBulkUploader {
private:
    void ProcessUpload(const ContentData& data) {
        // Get current context
        auto ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        
        // Submit to thread pool with context
        thread_pool_.Submit([this, data, ctx]() {
            // Attach context in worker thread
            auto token = opentelemetry::context::RuntimeContext::Attach(ctx);
            
            // Span inherits context automatically
            auto tracer = tracing_manager_->GetTracer();
            auto span = tracer->StartSpan("content.async_processing");
            
            // Process upload...
        });
    }
};
```

## Konfiguration

```yaml
# config/tracing.yml
opentelemetry:
  enabled: true
  service_name: "themisdb-content-pipeline"
  
  exporter:
    type: otlp_grpc  # otlp_grpc, otlp_http, jaeger, zipkin
    endpoint: "http://otel-collector:4317"
    headers:
      api-key: "${OTEL_API_KEY}"
  
  sampler:
    type: parent_based  # always_on, always_off, traceidratio, parent_based
    ratio: 0.1  # Sample 10% of traces
  
  span_processor:
    type: batch  # batch or simple
    max_queue_size: 2048
    schedule_delay_millis: 5000
    max_export_batch_size: 512
  
  resource_attributes:
    service.name: "themisdb-content-pipeline"
    service.version: "${VERSION}"
    deployment.environment: "${ENVIRONMENT}"
    host.name: "${HOSTNAME}"
```

## Trace-Beispiel

```
Trace ID: 7d8e4c3a2b1f0e9d8c7b6a5f4e3d2c1b

Span: content.upload [ROOT] (450ms)
  ├─ Attributes:
  │  ├─ content.type: "video"
  │  ├─ content.size: 52428800 (50MB)
  │  └─ user.id: "user-123"
  │
  ├─ Span: content.chunking (120ms)
  │  ├─ Attributes:
  │  │  ├─ chunk.strategy: "video_frame_based"
  │  │  ├─ chunk.count: 25
  │  │  └─ chunk.size_avg: 2097152 (2MB)
  │  └─ Status: OK
  │
  ├─ Span: content.compression (200ms)
  │  ├─ Attributes:
  │  │  ├─ compression.algorithm: "zstd"
  │  │  ├─ compression.ratio: 2.8
  │  │  └─ compression.level: 3
  │  └─ Status: OK
  │
  ├─ Span: content.storage (100ms)
  │  ├─ Attributes:
  │  │  ├─ storage.backend: "rocksdb"
  │  │  ├─ storage.tier: "hot"
  │  │  └─ storage.bytes: 18724571
  │  └─ Status: OK
  │
  └─ Span: content.metadata (30ms)
     ├─ Attributes:
     │  ├─ metadata.fields: 12
     │  └─ metadata.size: 1024
     └─ Status: OK
```

## Jaeger-Backend-Setup

```yaml
# docker-compose.yml
version: '3'
services:
  jaeger:
    image: jaegertracing/all-in-one:latest
    ports:
      - "5775:5775/udp"  # Zipkin compact
      - "6831:6831/udp"  # Jaeger compact
      - "6832:6832/udp"  # Jaeger binary
      - "5778:5778"      # Config
      - "16686:16686"    # UI
      - "14268:14268"    # Jaeger HTTP
      - "14250:14250"    # Jaeger gRPC
      - "9411:9411"      # Zipkin
    environment:
      - COLLECTOR_OTLP_ENABLED=true
      - COLLECTOR_ZIPKIN_HOST_PORT=:9411

  otel-collector:
    image: otel/opentelemetry-collector:latest
    command: ["--config=/etc/otel-collector-config.yml"]
    volumes:
      - ./otel-collector-config.yml:/etc/otel-collector-config.yml
    ports:
      - "4317:4317"  # OTLP gRPC
      - "4318:4318"  # OTLP HTTP
    depends_on:
      - jaeger
```

## Integration Points

- [ ] ContentManager (Root-Spans für Uploads)
- [ ] ChunkStrategy-Implementierungen (Chunking-Spans)
- [ ] ZstdCompression (Compression-Spans)
- [ ] StorageManager (Storage-Spans)
- [ ] AsyncBulkUploader (Async-Context-Propagation)
- [ ] Strukturiertes Logging (Trace-ID-Correlation)

## Testing-Anforderungen

### Unit-Tests
```cpp
TEST(OpenTelemetryTracing, SpanCreation) {
    // Test span creation and attributes
}

TEST(OpenTelemetryTracing, NestedSpans) {
    // Test parent-child span relationships
}

TEST(OpenTelemetryTracing, ContextPropagation) {
    // Test context propagation through async ops
}

TEST(OpenTelemetryTracing, ErrorRecording) {
    // Test exception recording in spans
}
```

### Integration-Tests
- [ ] End-to-End-Trace für erfolgreichen Upload
- [ ] End-to-End-Trace für fehlgeschlagenen Upload
- [ ] Context-Propagation durch Thread-Pool
- [ ] Trace-Sampling funktioniert korrekt
- [ ] Spans erscheinen in Jaeger/Zipkin
- [ ] Trace-ID in Logs korrekt

## Success Criteria

- [ ] OpenTelemetry SDK integriert und funktional
- [ ] Alle wichtigen Pipeline-Stages haben Spans
- [ ] Context-Propagation durch Async-Ops funktioniert
- [ ] Traces in Jaeger/Zipkin sichtbar
- [ ] Trace-ID in Logs für Correlation
- [ ] Sampling-Strategy konfigurierbar
- [ ] Performance-Impact < 2% auf Durchsatz
- [ ] Unit- und Integration-Tests bestehen
- [ ] Dokumentation vollständig

## Priorität

**Mittel** - Wichtig für Production-Debugging, aber nicht kritisch für Basis-Funktionalität

## Geschätzter Aufwand

**3-4 Tage**

## Dependencies

- **Benötigt**: OpenTelemetry C++ SDK
- **Benötigt**: Jaeger oder Zipkin Backend
- **Benötigt**: OTLP Collector (optional)
- **Related**: Strukturiertes Event-Logging (Trace-ID-Correlation)
- **Related**: Prometheus-Metriken (komplementär)

## Referenzen

- [ ] OpenTelemetry C++ SDK: https://github.com/open-telemetry/opentelemetry-cpp
- [ ] OpenTelemetry Specification: https://opentelemetry.io/docs/specs/otel/
- [ ] Jaeger Documentation: https://www.jaegertracing.io/docs/
- [ ] OTLP Protocol: https://opentelemetry.io/docs/specs/otlp/
- [ ] ThemisDB Tracing-Architektur: `docs/monitoring/tracing.md`

---

**Checklist:**
- [ ] Ich habe die Anforderungen verstanden
- [ ] Ich habe einen detaillierten Lösungsansatz erstellt
- [ ] Ich habe alle Pipeline-Stages für Tracing identifiziert
- [ ] Ich habe Context-Propagation-Strategie definiert
- [ ] Ich habe Backend-Setup geplant
- [ ] Ich habe Testing-Anforderungen definiert
- [ ] Ich habe Success-Criteria festgelegt
