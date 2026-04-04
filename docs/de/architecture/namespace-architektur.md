# ThemisDB C++ Namespace-Architektur

**Dokumentversion:** 1.0  
**Letzte Aktualisierung:** 15. Januar 2026  
**Status:** Produktiv

## Inhaltsverzeichnis

- [Überblick](#überblick)
- [Namespace-Hierarchie](#namespace-hierarchie)
- [Detaillierte Namespace-Dokumentation](#detaillierte-namespace-dokumentation)
- [Drittanbieter-Bibliotheken](#drittanbieter-bibliotheken)
- [Namespace-Konventionen](#namespace-konventionen)
- [Namespace-Aliase](#namespace-aliase)

---

## Überblick

Die ThemisDB-Codebasis verwendet eine hierarchische Namespace-Struktur, um Code nach funktionalen Domänen zu organisieren. Der primäre Root-Namespace ist `themis`, mit einem alternativen `themisdb`-Namespace für bestimmte Komponenten.

### Architekturprinzipien

- **Modulare Struktur**: Klare Trennung der Verantwortlichkeiten
- **Hierarchische Organisation**: Logische Gruppierung verwandter Komponenten
- **Namenskollisionen vermeiden**: Eindeutige Namespaces für alle Komponenten
- **Konsistente Namensgebung**: Einheitliche Namenskonventionen über die gesamte Codebasis

---

## Namespace-Hierarchie

### Root-Namespace

```
themis/                     # Primärer Root-Namespace
├── analytics/             # Datenanalyse und OLAP
├── api/                   # API-Schnittstellen
├── auth/                  # Authentifizierung & Autorisierung
├── cache/                 # Caching-Mechanismen
├── content/               # Content-Management
├── core/                  # Kernfunktionalität
├── edition/               # Editions-spezifische Features
├── errors/                # Fehlerbehandlung
├── experimental/          # Experimentelle Features
├── geo/                   # Geospatiale Funktionen
├── graphql/               # GraphQL-Unterstützung
├── index/                 # Indexierung
├── license/               # Lizenzverwaltung
├── llm/                   # Large Language Model Integration
│   ├── applications/      # LLM-Anwendungen
│   ├── lora/             # LoRA (Low-Rank Adaptation)
│   └── monitoring/        # LLM-Monitoring
├── memory/                # Speicherverwaltung
├── modules/               # Modul-System
├── network/               # Netzwerk-Kommunikation
├── performance/           # Performance-Optimierungen
├── query/                 # Query-Verarbeitung
│   └── functions/        # Query-Funktionen
├── rpc/                   # Remote Procedure Calls
├── security/              # Sicherheitsfunktionen
├── server/                # Server-Implementierung
├── sharding/              # Sharding & Verteilung
├── simd/                  # SIMD-Optimierungen
├── storage/               # Speicher-Layer
├── time/                  # Zeitbezogene Funktionen
├── tools/                 # Entwicklungswerkzeuge
├── transaction/           # Transaktionsverwaltung
├── updates/               # Update-Mechanismen
├── utils/                 # Hilfsfunktionen
├── version/               # Versionsinformationen
├── voice/                 # Sprachassistenz
└── wire/                  # Wire-Protocol

themisdb/                   # Alternativer Root-Namespace
├── backpressure/          # Backpressure-Protokoll
├── sharding/              # Sharding-Komponenten (erweitert)
├── storage/               # Storage-Komponenten (erweitert)
├── streaming/             # Streaming-Protokoll
└── temporal/              # Temporale Konfliktauflösung
```

---

## Detaillierte Namespace-Dokumentation

### `themis::analytics`

**Zweck**: Datenanalyse und OLAP-Operationen

**Komponenten**:
- NLP-Integration
- OLAP-Engine
- Text-Analyse
- Datenaggregate

**Verwendung**:
```cpp
using namespace themis::analytics;
```

**Wichtige Klassen**:
- `NLPIntegration` - NLP-Textanalyse
- `OLAPEngine` - OLAP-Verarbeitung
- `TextAnalyzer` - Textanalyse-Engine

---

### `themis::auth`

**Zweck**: Authentifizierung und Autorisierung

**Komponenten**:
- JWT-Validierung
- GSSAPI/Kerberos-Integration
- OAuth2-Support
- RBAC (Role-Based Access Control)

**Verwendung**:
```cpp
using namespace themis::auth;
```

**Wichtige Klassen**:
- `JWTValidator` - JWT-Token-Validierung
- `GSSAPIAuthenticator` - Kerberos-Authentifizierung
- `AuthMiddleware` - Authentifizierungs-Middleware

---

### `themis::cache`

**Zweck**: Caching-Mechanismen

**Komponenten**:
- Embedding-Cache
- LIRS-Cache (Low Inter-reference Recency Set)
- LRU-Cache

**Verwendung**:
```cpp
using namespace themis::cache;
```

**Wichtige Klassen**:
- `EmbeddingCache` - Cache für Vektor-Embeddings
- `LIRSCache` - LIRS-Cache-Implementierung

---

### `themis::content`

**Zweck**: Content-Management und Verarbeitung

**Komponenten**:
- Content-Policy-Engine
- MIME-Type-Detection
- CLIP-Integration (Contrastive Language-Image Pre-training)

**Verwendung**:
```cpp
using namespace themis::content;
```

**Wichtige Klassen**:
- `ContentPolicy` - Content-Policy-Engine
- `MimeDetector` - MIME-Type-Erkennung
- `MockCLIP` - CLIP-Mock-Implementierung

---

### `themis::geo`

**Zweck**: Geospatiale Funktionen

**Komponenten**:
- EWKB (Extended Well-Known Binary)
- Spatial-Backend
- Geo-Indexierung

**Verwendung**:
```cpp
using namespace themis::geo;
```

**Wichtige Klassen**:
- `EWKBHandler` - EWKB-Format-Verarbeitung
- `SpatialBackend` - Geospatiale Backend-Engine

---

### `themis::llm`

**Zweck**: Large Language Model Integration

**Komponenten**:
- Inference-Engine
- LLM-Plugins
- Vision-Encoder
- KV-Cache-Management
- Response-Cache
- Flash-Attention

**Verwendung**:
```cpp
using namespace themis::llm;
```

**Wichtige Klassen**:
- `InferenceEngine` - LLM-Inferenz-Engine
- `VisionEncoder` - Vision-Modell-Encoder
- `PrefixCache` - Prefix-Cache für KV-Wiederverwendung
- `ResponseCache` - Antwort-Cache
- `PagedAttention` - Paged-Attention-Implementierung
- `ContinuousBatchScheduler` - Kontinuierlicher Batch-Scheduler

**Subnamespace `themis::llm::lora`**:
- `LoRAFramework` - LoRA-Framework-Integration
- `LoRASecurity` - LoRA-Sicherheitsmechanismen
- `LoRAAdapter` - LoRA-Adapter-Verwaltung

**Subnamespace `themis::llm::monitoring`**:
- `GrafanaMetrics` - Grafana-Metriken für LLM

---

### `themis::query`

**Zweck**: Query-Verarbeitung und AQL (Arangodb Query Language)

**Komponenten**:
- AQL-Parser
- Query-Optimizer
- Subquery-Verarbeitung
- Hybrid-Queries

**Verwendung**:
```cpp
using namespace themis::query;
```

**Wichtige Klassen**:
- `AQLParser` - AQL-Query-Parser
- `QueryOptimizer` - Query-Optimierer
- `SubqueryExecutor` - Subquery-Ausführung

**Subnamespace `themis::query::functions`**:
- LoRA-AQL-Funktionen
- Custom-AQL-Funktionen

---

### `themis::server`

**Zweck**: Server-Implementierung und API-Handler

**Komponenten**:
- HTTP-Server
- WebSocket-Sessions
- SSE (Server-Sent Events)
- API-Handler für verschiedene Funktionen
- Rate-Limiting
- Tenant-Management

**Verwendung**:
```cpp
using namespace themis::server;
```

**Wichtige Klassen**:
- `VectorAPIHandler` - Vector-API-Handler
- `QueryAPIHandler` - Query-API-Handler
- `WALAPIHandler` - Write-Ahead-Log API
- `TimeseriesAPIHandler` - Timeseries-API
- `WebSocketSession` - WebSocket-Session-Management
- `SSEConnectionManager` - SSE-Verbindungsmanager
- `RateLimiter` - Rate-Limiting-Engine
- `TenantManager` - Multi-Tenancy-Management

**Subnamespace `themis::server::rpc`**:
- `RPCServiceImpl` - RPC-Service-Implementierung
- `DifferentialUpdateEngine` - Differential-Update-Engine
- `BlobTransferHandler` - Blob-Transfer-Handler
- `SnapshotTransferHandler` - Snapshot-Transfer-Handler

---

### `themis::sharding`

**Zweck**: Sharding und verteilte Systeme

**Komponenten**:
- Shard-Router
- Replikation
- Distributed-Transactions
- WAL (Write-Ahead Log)
- Konsistenz-Protokolle
- Geo-Replikation

**Verwendung**:
```cpp
using namespace themis::sharding;
```

**Wichtige Klassen**:
- `ShardRouter` - Shard-Routing-Engine
- `ReplicationCoordinator` - Replikations-Koordinator
- `DistributedTransaction` - Verteilte Transaktionen
- `WALManager` - WAL-Manager
- `WALShipper` - WAL-Versand
- `WALApplier` - WAL-Anwendung
- `CircuitBreaker` - Circuit-Breaker-Pattern
- `HealthMonitor` - Gesundheits-Überwachung
- `ConsistentHash` - Konsistente Hash-Verteilung
- `AutoRebalancer` - Automatisches Rebalancing
- `GPUErasureCoder` - GPU-beschleunigtes Erasure-Coding
- `TrueTime` - Spanner-ähnliche Zeitstempel
- `MultiPrimaryCoordinator` - Multi-Primary-Replikation

---

### `themis::storage`

**Zweck**: Speicher-Layer und Persistenz

**Komponenten**:
- Blob-Storage
- RocksDB-Wrapper
- Backup-Management
- PITR (Point-in-Time Recovery)
- Security-Signatures

**Verwendung**:
```cpp
using namespace themis::storage;
```

**Wichtige Klassen**:
- `BlobStorageManager` - Blob-Storage-Verwaltung
- `BlobBackendS3` - S3-Backend
- `BlobBackendAzure` - Azure-Backend
- `BlobBackendFilesystem` - Filesystem-Backend
- `BlobBackendWebDAV` - WebDAV-Backend
- `BackupManager` - Backup-Verwaltung
- `PITRManager` - Point-in-Time-Recovery
- `SecuritySignatureManager` - Sicherheits-Signaturen

---

### `themis::utils`

**Zweck**: Allgemeine Hilfsfunktionen

**Komponenten**:
- Logging
- Serialisierung
- Kryptographie
- String-Utilities
- Input-Validierung
- PII-Detection (Personal Identifiable Information)

**Verwendung**:
```cpp
using namespace themis::utils;
```

**Wichtige Klassen**:
- `Logger` - Logging-Framework (basierend auf spdlog)
- `AuditLogger` - Audit-Logging
- `InputValidator` - Input-Validierung
- `Serializer` - Serialisierungs-Utilities
- `HKDFHelper` - HKDF-Kryptographie
- `PIIDetector` - PII-Erkennung
- `PIIPseudonymizer` - PII-Pseudonymisierung
- `Stemmer` - Text-Stemming
- `Normalizer` - Text-Normalisierung
- `ZstdCodec` - Zstandard-Kompression
- `HTTPClientPool` - HTTP-Client-Pool

---

### `themisdb::sharding`

**Zweck**: Erweiterte Sharding-Komponenten (alternativer Namespace)

**Komponenten**:
- RAFT-Konsensus
- Hot-Spare-Management
- RAID-Optimierungen
- Predictive-Failure-Detection

**Verwendung**:
```cpp
using namespace themisdb::sharding;
```

**Wichtige Klassen**:
- `RAFTConfiguration` - RAFT-Konfiguration
- `RAFTLog` - RAFT-Log-Verwaltung
- `RAFTState` - RAFT-Zustandsmaschine
- `HotSpareManager` - Hot-Spare-Verwaltung
- `PredictiveDetector` - Vorhersage von Ausfällen
- `RedundancyStrategy` - RAID-Redundanz-Strategien

---

### `themisdb::streaming`

**Zweck**: Streaming-Protokolle

**Komponenten**:
- Stream-Protocol
- Backpressure-Management

**Verwendung**:
```cpp
using namespace themisdb::streaming;
```

---

### `themisdb::temporal`

**Zweck**: Temporale Konfliktauflösung

**Komponenten**:
- Temporal-Conflict-Resolver
- Bitemporal-Daten-Support

**Verwendung**:
```cpp
using namespace themisdb::temporal;
```

---

## Drittanbieter-Bibliotheken

ThemisDB integriert eine Vielzahl von Drittanbieter-Bibliotheken über vcpkg und manuelle Integration.

### Kern-Abhängigkeiten

#### **RocksDB** (Storage-Engine)
- **Namespace**: `rocksdb`
- **Zweck**: Eingebettete Key-Value-Datenbank
- **Features**: lz4, zstd Kompression
- **Integration**: Via vcpkg
- **Verwendung**: Primäre Storage-Engine für ThemisDB

```cpp
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}
```

#### **OpenSSL** (Kryptographie)
- **Namespace**: -
- **Zweck**: TLS/SSL, Kryptographie
- **Integration**: Via vcpkg
- **Verwendung**: HTTPS, mTLS, Verschlüsselung

#### **Boost** (Utility-Bibliothek)
- **Namespace**: `boost`, `boost::asio`, `boost::beast`
- **Zweck**: Asynchrone I/O, HTTP/WebSocket-Server
- **Komponenten**:
  - `boost::asio` - Asynchrone I/O
  - `boost::beast` - HTTP/WebSocket
  - `boost::system` - System-Utilities
- **Integration**: Via vcpkg

**Namespace-Aliase**:
```cpp
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
```

#### **simdjson** (JSON-Parsing)
- **Namespace**: `simdjson`
- **Zweck**: Schnelles JSON-Parsing mit SIMD
- **Integration**: Via vcpkg
- **Verwendung**: High-Performance JSON-Verarbeitung

#### **nlohmann-json** (JSON-Bibliothek)
- **Namespace**: `nlohmann`
- **Zweck**: Moderne C++ JSON-Bibliothek
- **Integration**: Via vcpkg
- **Verwendung**: JSON-Serialisierung/-Deserialisierung

#### **spdlog** (Logging)
- **Namespace**: `spdlog`
- **Zweck**: Schnelles C++ Logging
- **Integration**: Via vcpkg
- **Verwendung**: Application-Logging

```cpp
namespace spdlog {
    class logger;
}
```

#### **fmt** (String-Formatierung)
- **Namespace**: `fmt`
- **Zweck**: String-Formatierung (C++20-Style)
- **Integration**: Via vcpkg
- **Verwendung**: String-Formatierung

#### **Intel TBB** (Threading Building Blocks)
- **Namespace**: `tbb`
- **Zweck**: Parallele Programmierung
- **Integration**: Via vcpkg
- **Verwendung**: Parallel-Algorithmen, Task-Scheduling

---

### Vector-Search & SIMD

#### **HNSWlib** (Vector-Search)
- **Namespace**: `hnswlib`
- **Zweck**: Hierarchical Navigable Small World (HNSW) für Approximate Nearest Neighbor Search
- **Integration**: Via vcpkg
- **Verwendung**: Vector-Suche

#### **FAISS** (Vector-Search mit GPU)
- **Namespace**: `faiss`
- **Zweck**: Facebook AI Similarity Search
- **Integration**: Via vcpkg (optional, GPU-Feature)
- **Verwendung**: GPU-beschleunigte Vector-Suche

#### **OpenBLAS** (Linear Algebra)
- **Namespace**: -
- **Zweck**: Optimierte BLAS-Implementierung
- **Integration**: Via vcpkg (GPU-Feature)
- **Verwendung**: Matrix-Operationen für ML

#### **LAPACK** (Linear Algebra)
- **Namespace**: -
- **Zweck**: Linear Algebra Package
- **Integration**: Via vcpkg (GPU-Feature)
- **Verwendung**: Erweiterte lineare Algebra

---

### Kompression

#### **Zstandard (zstd)** (Kompression)
- **Namespace**: `zstd` / `ZSTD`
- **Zweck**: Schnelle Kompression
- **Integration**: Via vcpkg
- **Verwendung**: Daten-Kompression, Blob-Storage

```cpp
namespace themis::utils {
    class ZstdCodec; // Wrapper um zstd
}
```

---

### GPU & CUDA

#### **CUDA** (GPU-Computing)
- **Namespace**: -
- **Zweck**: NVIDIA GPU-Programmierung
- **Integration**: Manuell (CUDA Toolkit erforderlich)
- **CMake**: `find_package(CUDAToolkit REQUIRED)`
- **Verwendung**: GPU-Beschleunigung für Vector-Search, Erasure-Coding

**CUDA-Targets**:
- `CUDA::cudart` - CUDA Runtime
- `CUDA::cuda_driver` - CUDA Driver API

**Verwendung**:
```cmake
target_link_libraries(themis_core PRIVATE CUDA::cudart CUDA::cuda_driver)
```

#### **Vulkan** (GPU-Computing)
- **Namespace**: `vk` / `vulkan`
- **Zweck**: Plattformübergreifende GPU-API
- **Integration**: Via `find_package(Vulkan QUIET)`
- **Verwendung**: Alternative zu CUDA für GPU-Beschleunigung

---

### Speicherverwaltung

#### **mimalloc** (Memory-Allocator)
- **Namespace**: `mi`
- **Zweck**: Performanter Memory-Allocator von Microsoft
- **Integration**: Via vcpkg
- **Verwendung**: Ersatz für Standard-Allocator

```cpp
#include <mimalloc.h>
// Automatisch verwendet durch Linking
```

**Aktivierung**:
```cmake
find_package(mimalloc CONFIG)
target_link_libraries(themis_core PRIVATE mimalloc)
```

---

### Netzwerk & Protokolle

#### **gRPC** (RPC-Framework)
- **Namespace**: `grpc`
- **Zweck**: High-Performance RPC-Framework
- **Integration**: Via vcpkg
- **Verwendung**: RPC-Kommunikation, Sharding

```cpp
namespace grpc {
    class Server;
    class Channel;
}
```

#### **Protobuf** (Serialisierung)
- **Namespace**: `google::protobuf`
- **Zweck**: Protocol Buffers für Serialisierung
- **Integration**: Via vcpkg (Abhängigkeit von gRPC)
- **Verwendung**: RPC-Nachrichten, Daten-Serialisierung

#### **nghttp2** (HTTP/2)
- **Namespace**: -
- **Zweck**: HTTP/2-Implementierung
- **Integration**: Via vcpkg (http2-Feature)
- **Verwendung**: HTTP/2-Server-Push

#### **nghttp3** (HTTP/3)
- **Namespace**: -
- **Zweck**: HTTP/3-Implementierung
- **Integration**: Via vcpkg (http3-Feature)
- **Verwendung**: HTTP/3 (QUIC)-Unterstützung

#### **ngtcp2** (QUIC)
- **Namespace**: -
- **Zweck**: QUIC-Protokoll-Implementierung
- **Integration**: Via vcpkg mit OpenSSL-Feature
- **Verwendung**: QUIC-Transport für HTTP/3

#### **cURL** (HTTP-Client)
- **Namespace**: -
- **Zweck**: HTTP-Client-Bibliothek
- **Integration**: Via vcpkg
- **Verwendung**: HTTP-Anfragen, Webhooks

#### **paho-mqttpp3** (MQTT)
- **Namespace**: `mqtt`
- **Zweck**: MQTT-Protokoll für IoT
- **Integration**: Via vcpkg (mqtt-Feature)
- **Verwendung**: IoT-Deployments, Pub/Sub

---

### Observability

#### **OpenTelemetry** (Tracing & Metrics)
- **Namespace**: `opentelemetry`
- **Zweck**: Observability-Framework (Tracing, Metrics)
- **Integration**: Via vcpkg mit otlp-http-Feature
- **Verwendung**: Distributed Tracing, Metriken

**Namespace-Aliase**:
```cpp
namespace otel = opentelemetry;
namespace otel_sdk = opentelemetry::sdk;
namespace otel_trace = opentelemetry::trace;
namespace otel_resource = opentelemetry::sdk::resource;
namespace otel_exporter = opentelemetry::exporter::otlp;
```

**Wichtige Komponenten**:
```cpp
namespace themis {
    // Wrapper um OpenTelemetry
    class TracingManager;
}
```

#### **Prometheus-cpp** (Metrics)
- **Namespace**: `prometheus`
- **Zweck**: Prometheus-Metriken
- **Integration**: Via vcpkg mit pull-Feature
- **Verwendung**: Metriken-Export für Prometheus

```cpp
namespace themis::sharding {
    class PrometheusMetrics; // Wrapper um prometheus-cpp
}
```

---

### Geospatial

#### **GDAL** (Geospatial Data Abstraction Library)
- **Namespace**: `GDAL` / `OGR`
- **Zweck**: Geospatiale Datenverarbeitung
- **Integration**: Via vcpkg (gdal-Feature)
- **Verwendung**: Shapefile, GeoTIFF-Unterstützung

```cmake
find_package(GDAL CONFIG QUIET)
```

---

### Konfiguration & Utilities

#### **yaml-cpp** (YAML-Parsing)
- **Namespace**: `YAML`
- **Zweck**: YAML-Konfigurationsdateien
- **Integration**: Via vcpkg
- **Verwendung**: Konfigurationsdateien parsen

#### **libzip** (ZIP-Archive)
- **Namespace**: -
- **Zweck**: ZIP-Archive erstellen/lesen
- **Integration**: Via vcpkg
- **Verwendung**: Backup-Archive, Blob-Storage

#### **pugixml** (XML-Parsing)
- **Namespace**: `pugi`
- **Zweck**: Leichtgewichtiges XML-Parsing
- **Integration**: Via vcpkg
- **Verwendung**: XML-Konfiguration, WebDAV

#### **cpp-httplib** (HTTP-Server)
- **Namespace**: `httplib`
- **Zweck**: Einfacher HTTP-Server/Client
- **Integration**: Via vcpkg
- **Verwendung**: Einfache HTTP-Endpunkte

---

### Testing & Benchmarking

#### **Google Test (gtest)** (Unit-Testing)
- **Namespace**: `testing`
- **Zweck**: Unit-Testing-Framework
- **Integration**: Via vcpkg
- **Verwendung**: Unit-Tests

```cpp
#include <gtest/gtest.h>
TEST(TestSuite, TestCase) { ... }
```

#### **Google Benchmark** (Benchmarking)
- **Namespace**: `benchmark`
- **Zweck**: Micro-Benchmarking
- **Integration**: Via vcpkg
- **Verwendung**: Performance-Benchmarks

```cpp
#include <benchmark/benchmark.h>
BENCHMARK(BenchmarkFunction);
```

---

### LLM-Integration

#### **llama.cpp** (LLM-Inferenz)
- **Namespace**: `llama` (intern)
- **Zweck**: LLM-Inferenz-Engine
- **Integration**: Als Git-Submodul oder manuell
- **Verwendung**: On-Device LLM-Inferenz

**Integration**:
```cmake
add_subdirectory(llama.cpp)
target_link_libraries(themis_core PRIVATE llama)
```

**ThemisDB-Wrapper**:
```cpp
namespace themis::llm {
    class LlamaCppIntegration;
    class InferenceEngine;
}
```

---

### ML & AI

#### **ONNXRuntime** (ML-Inferenz)
- **Namespace**: `onnxruntime`
- **Zweck**: ONNX-Modell-Inferenz
- **Integration**: Via vcpkg (Plugin)
- **Verwendung**: CLIP-Modelle für Vision

```cpp
namespace themis::content {
    class ONNXCLIPBackend; // Verwendet onnxruntime
}
```

#### **OpenCV** (Computer Vision)
- **Namespace**: `cv`
- **Zweck**: Computer-Vision-Bibliothek
- **Integration**: Via vcpkg (Plugin)
- **Verwendung**: Bildverarbeitung für CLIP

---

## Namespace-Konventionen

### Namensgebung

1. **Kleinschreibung**: Alle Namespace-Namen in Kleinbuchstaben
   ```cpp
   namespace themis::storage { ... }
   ```

2. **Snake_case für Unternamespaces**: Bei Bedarf
   ```cpp
   namespace themis::build_info { ... }
   ```

3. **Hierarchische Struktur**: Logische Verschachtelung
   ```cpp
   namespace themis {
   namespace llm {
   namespace lora {
       // LoRA-spezifischer Code
   }}}
   ```

4. **Kurze, prägnante Namen**: Vermeidung von Abkürzungen, außer bei Industriestandards (RPC, HTTP, etc.)

### Namespace-Verwendung

#### Best Practices

**DO:**
```cpp
// In Header-Dateien: Explizite Namespace-Qualifikation
namespace themis {
namespace storage {
    class BlobStorageManager {
        // ...
    };
}}
```

```cpp
// In Source-Dateien: using namespace
using namespace themis::storage;
```

**DON'T:**
```cpp
// Vermeiden in Header-Dateien
using namespace themis; // ❌ Niemals in Headers!
```

#### Nested Namespaces (C++17)

```cpp
// Bevorzugt (C++17+)
namespace themis::llm::lora {
    // ...
}

// Statt
namespace themis {
namespace llm {
namespace lora {
    // ...
}}}
```

---

## Namespace-Aliase

### Standard-Aliase

```cpp
// Boost-Aliase (häufig verwendet)
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

// Filesystem-Alias
namespace fs = std::filesystem;

// OpenTelemetry-Aliase
namespace otel = opentelemetry;
namespace otel_sdk = opentelemetry::sdk;
namespace otel_trace = opentelemetry::trace;
namespace otel_resource = opentelemetry::sdk::resource;
namespace otel_exporter = opentelemetry::exporter::otlp;
```

### Verwendung in Dateien

```cpp
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace themis::server {

class HTTPServer {
    net::io_context ioc_;
    beast::tcp_stream stream_;
    // ...
};

} // namespace themis::server
```

---

## Integration von Drittanbieter-Namespaces

### Wrapper-Pattern

ThemisDB verwendet häufig Wrapper-Klassen um Drittanbieter-Bibliotheken:

```cpp
// RocksDB-Wrapper
namespace themis {
    class RocksDBWrapper {
        std::unique_ptr<rocksdb::TransactionDB> db_;
        // ThemisDB-spezifische API
    };
}

// OpenTelemetry-Wrapper
namespace themis {
    class TracingManager {
        std::shared_ptr<opentelemetry::trace::Tracer> tracer_;
        // Vereinfachte API für ThemisDB
    };
}

// mimalloc-Integration (transparent)
// Keine explizite Wrapper-Klasse, automatisch durch Linking
```

### Dependency-Injection

```cpp
namespace themis::storage {
    class BlobBackend {
    public:
        virtual ~BlobBackend() = default;
        virtual void put(const std::string& key, const std::string& data) = 0;
        // ...
    };

    // S3-Implementierung (verwendet AWS SDK)
    class BlobBackendS3 : public BlobBackend {
        // AWS SDK-Namespaces intern verwendet
    };

    // Azure-Implementierung
    class BlobBackendAzure : public BlobBackend {
        // Azure SDK-Namespaces intern verwendet
    };
}
```

---

## Zusammenfassung

Die ThemisDB-Namespace-Architektur bietet:

1. **Klare Strukturierung**: Funktionale Domänen sind klar getrennt
2. **Skalierbarkeit**: Neue Features können einfach integriert werden
3. **Wartbarkeit**: Code ist leicht zu finden und zu verstehen
4. **Integration**: Drittanbieter-Bibliotheken sind sauber gekapselt
5. **Konsistenz**: Einheitliche Namenskonventionen

### Wichtigste Namespaces

| Namespace | Zweck | Wichtigkeit |
|-----------|-------|-------------|
| `themis::storage` | Storage-Engine, Persistenz | ⭐⭐⭐⭐⭐ |
| `themis::server` | HTTP-Server, API-Handler | ⭐⭐⭐⭐⭐ |
| `themis::query` | Query-Verarbeitung, AQL | ⭐⭐⭐⭐⭐ |
| `themis::sharding` | Verteilte Systeme | ⭐⭐⭐⭐⭐ |
| `themis::llm` | LLM-Integration | ⭐⭐⭐⭐ |
| `themis::utils` | Hilfsfunktionen | ⭐⭐⭐⭐ |
| `themis::auth` | Sicherheit, Authentifizierung | ⭐⭐⭐⭐ |
| `themis::geo` | Geospatiale Funktionen | ⭐⭐⭐ |

---

## Siehe auch

- [ThemisDB Architecture Overview](../architecture/README.md)
- [Build System Documentation](../build/README.md)
- [API Reference](../api/README.md)
- [Coding Standards](../../../CODING_STANDARDS.md)

---

**Hinweis**: Diese Dokumentation basiert auf ThemisDB v1.3.0+ und wird regelmäßig aktualisiert.
