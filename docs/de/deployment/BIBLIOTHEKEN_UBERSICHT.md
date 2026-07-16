# ThemisDB Bibliotheken-Übersicht

**Stand:** 6. April 2026  
**Version:** v1.5.0-dev  
**Kategorie:** 🚀 Deployment  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Kern-Abhängigkeiten](#-kern-abhängigkeiten)
- [Optionale Features](#-optionale-features)
- [Build-System](#-build-system)
- [Paketmanagement](#-paketmanagement)

---

## 🎯 Übersicht

ThemisDB nutzt moderne C++ Bibliotheken für Hochleistungs-Datenbank-Funktionalität. Alle Abhängigkeiten werden über **vcpkg** verwaltet für reproduzierbare Builds auf allen Plattformen.

**Gesamt:** 30 Kern-Bibliotheken + optionale Feature-Bibliotheken

---

## 📦 Kern-Abhängigkeiten

Diese Bibliotheken sind für alle ThemisDB-Builds erforderlich:

### 1. OpenSSL
- **Version:** >= 3.6.0 (über vcpkg)
- **Zweck:** TLS/SSL-Verschlüsselung, kryptographische Funktionen
- **vcpkg:** `openssl`
- **Dokumentation:** [https://www.openssl.org/docs/](https://www.openssl.org/docs/)
- **Lizenz:** Apache 2.0

### 2. zlib
- **Version:** >= 1.3 (über vcpkg)
- **Zweck:** Allgemeine Datenkompression (gzip/deflate)
- **vcpkg:** `zlib`
- **Dokumentation:** [https://zlib.net/](https://zlib.net/)
- **Lizenz:** zlib License (permissiv)

### 3. RocksDB
- **Version:** Aktuell über vcpkg
- **Zweck:** Embedded Key-Value Store, LSM-Tree Storage Engine
- **vcpkg:** `rocksdb` mit Features: `lz4`, `snappy`, `zlib`, `zstd`
- **Dokumentation:** [https://rocksdb.org/](https://rocksdb.org/)
- **Lizenz:** Apache 2.0 / GPL 2.0
- **Features:**
  - `lz4`: LZ4 Kompression für schnelle I/O
  - `snappy`: Snappy Kompression (Google)
  - `zlib`: zlib/gzip Kompression
  - `zstd`: Zstandard Kompression für hohe Kompressionsraten

### 4. simdjson
- **Version:** >= 3.0.0 (über vcpkg)
- **Zweck:** Ultra-schnelles JSON Parsing (Gigabytes pro Sekunde)
- **vcpkg:** `simdjson`
- **Dokumentation:** [https://simdjson.org/](https://simdjson.org/)
- **Lizenz:** Apache 2.0

### 5. Intel TBB (Threading Building Blocks)
- **Version:** >= 2021.5.0 (über vcpkg)
- **Zweck:** Task-basierte Parallelisierung, Thread-Pool
- **vcpkg:** `tbb`
- **Dokumentation:** [https://www.intel.com/content/www/us/en/docs/onetbb/](https://www.intel.com/content/www/us/en/docs/onetbb/)
- **Lizenz:** Apache 2.0

### 6. HNSWlib
- **Version:** Aktuell über vcpkg
- **Zweck:** Approximate Nearest Neighbor Search (Vektor-Ähnlichkeitssuche)
- **vcpkg:** `hnswlib`
- **Dokumentation:** [https://github.com/nmslib/hnswlib](https://github.com/nmslib/hnswlib)
- **Lizenz:** Apache 2.0

### 7. FAISS
- **Version:** Aktuell über vcpkg
- **Zweck:** Facebook AI Similarity Search – hochperformante Vektorsuche
- **vcpkg:** `faiss`
- **Dokumentation:** [https://faiss.ai/](https://faiss.ai/)
- **Lizenz:** MIT

### 8. Google Test
- **Version:** >= 1.14.0 (über vcpkg)
- **Zweck:** Unit Testing Framework
- **vcpkg:** `gtest`
- **Dokumentation:** [https://google.github.io/googletest/](https://google.github.io/googletest/)
- **Lizenz:** BSD 3-Clause

### 9. Google Benchmark
- **Version:** >= 1.8.0 (über vcpkg)
- **Zweck:** Microbenchmarking Framework
- **vcpkg:** `benchmark`
- **Dokumentation:** [https://github.com/google/benchmark](https://github.com/google/benchmark)
- **Lizenz:** Apache 2.0

### 10. Boost.Asio
- **Version:** >= 1.83.0 (über vcpkg)
- **Zweck:** Asynchrone Netzwerk-I/O
- **vcpkg:** `boost-asio`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/doc/html/boost_asio.html](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- **Lizenz:** Boost Software License 1.0

### 11. Boost.Beast
- **Version:** >= 1.83.0 (über vcpkg)
- **Zweck:** HTTP/WebSocket Server über Asio
- **vcpkg:** `boost-beast`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/libs/beast/](https://www.boost.org/doc/libs/release/libs/beast/)
- **Lizenz:** Boost Software License 1.0

### 12. Boost.Variant
- **Version:** >= 1.83.0 (über vcpkg)
- **Zweck:** Typsichere Union-Typen für C++
- **vcpkg:** `boost-variant`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/doc/html/variant.html](https://www.boost.org/doc/libs/release/doc/html/variant.html)
- **Lizenz:** Boost Software License 1.0

### 13. Boost.URL
- **Version:** >= 1.83.0 (über vcpkg)
- **Zweck:** URL-Parsing und -Manipulation
- **vcpkg:** `boost-url`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/libs/url/](https://www.boost.org/doc/libs/release/libs/url/)
- **Lizenz:** Boost Software License 1.0

### 14. spdlog
- **Version:** >= 1.12.0 (über vcpkg)
- **Zweck:** Hochperformantes Logging Framework
- **vcpkg:** `spdlog`
- **Dokumentation:** [https://github.com/gabime/spdlog](https://github.com/gabime/spdlog)
- **Lizenz:** MIT

### 15. nlohmann/json
- **Version:** >= 3.11.3 (über vcpkg)
- **Zweck:** JSON für Modern C++, Schema-Validierung
- **vcpkg:** `nlohmann-json`
- **Dokumentation:** [https://json.nlohmann.me/](https://json.nlohmann.me/)
- **Lizenz:** MIT

### 16. c-ares
- **Version:** Aktuell über vcpkg
- **Zweck:** Asynchrone DNS-Auflösung (wird von gRPC verwendet)
- **vcpkg:** `c-ares`
- **Dokumentation:** [https://c-ares.org/](https://c-ares.org/)
- **Lizenz:** MIT

### 17. gRPC
- **Version:** >= 1.60.0 (über vcpkg)
- **Zweck:** RPC Framework für Shard-Kommunikation
- **vcpkg:** `grpc`
- **Dokumentation:** [https://grpc.io/docs/](https://grpc.io/docs/)
- **Lizenz:** Apache 2.0

### 18. OpenTelemetry C++
- **Version:** >= 1.24.0 (über vcpkg)
- **Zweck:** Distributed Tracing, Observability
- **vcpkg:** `opentelemetry-cpp` mit Feature: `otlp-http`
- **Dokumentation:** [https://opentelemetry.io/docs/instrumentation/cpp/](https://opentelemetry.io/docs/instrumentation/cpp/)
- **Lizenz:** Apache 2.0
- **CMake Option:** `THEMIS_ENABLE_TRACING` (Standard: ON)
- **Features:**
  - `otlp-http`: OTLP HTTP Exporter für Jaeger/Tempo

### 19. libcurl
- **Version:** >= 8.10.0 (über vcpkg)
- **Zweck:** HTTP Client für externe API-Aufrufe
- **vcpkg:** `curl`
- **Dokumentation:** [https://curl.se/libcurl/](https://curl.se/libcurl/)
- **Lizenz:** curl License (MIT-style)

### 20. yaml-cpp
- **Version:** >= 0.7.0 (über vcpkg)
- **Zweck:** YAML Konfigurationsdateien parsen/schreiben
- **vcpkg:** `yaml-cpp`
- **Dokumentation:** [https://github.com/jbeder/yaml-cpp](https://github.com/jbeder/yaml-cpp)
- **Lizenz:** MIT

### 21. Zstandard (zstd)
- **Version:** >= 1.5.0 (über vcpkg)
- **Zweck:** Hochleistungs-Kompression für Storage
- **vcpkg:** `zstd`
- **Dokumentation:** [https://facebook.github.io/zstd/](https://facebook.github.io/zstd/)
- **Lizenz:** BSD / GPLv2

### 22. mimalloc
- **Version:** >= 2.2.6 (über vcpkg)
- **Zweck:** Hochperformanter Memory Allocator
- **vcpkg:** `mimalloc`
- **Dokumentation:** [https://microsoft.github.io/mimalloc/](https://microsoft.github.io/mimalloc/)
- **Lizenz:** MIT
- **CMake Option:** `THEMIS_ENABLE_MIMALLOC` (Standard: OFF)
- **Performance:** +10-20% bei Memory-intensiven Workloads

### 23. prometheus-cpp
- **Version:** Aktuell über vcpkg
- **Zweck:** Prometheus Metriken-Export für Monitoring
- **vcpkg:** `prometheus-cpp` mit Feature: `pull`
- **Dokumentation:** [https://github.com/jupp0r/prometheus-cpp](https://github.com/jupp0r/prometheus-cpp)
- **Lizenz:** MIT
- **Features:**
  - `pull`: HTTP Pull-Endpunkt für Prometheus-Scraping

### 24. libzip
- **Version:** Aktuell über vcpkg
- **Zweck:** ZIP-Archiv-Erstellung und -Extraktion
- **vcpkg:** `libzip`
- **Dokumentation:** [https://libzip.org/](https://libzip.org/)
- **Lizenz:** BSD 3-Clause

### 25. pugixml
- **Version:** Aktuell über vcpkg
- **Zweck:** Schnelles XML-Parsing und -Manipulation
- **vcpkg:** `pugixml`
- **Dokumentation:** [https://pugixml.org/](https://pugixml.org/)
- **Lizenz:** MIT

### 26. crc32c
- **Version:** Aktuell über vcpkg
- **Zweck:** Schnelle CRC32C-Prüfsummenberechnung für Datenintegrität
- **vcpkg:** `crc32c`
- **Dokumentation:** [https://github.com/google/crc32c](https://github.com/google/crc32c)
- **Lizenz:** BSD 3-Clause

### 27. cpp-httplib
- **Version:** Aktuell über vcpkg
- **Zweck:** Einfacher HTTP/HTTPS Server und Client (Header-only)
- **vcpkg:** `cpp-httplib`
- **Dokumentation:** [https://github.com/yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)
- **Lizenz:** MIT

### 28. msgpack
- **Version:** Aktuell über vcpkg
- **Zweck:** Effiziente binäre Serialisierung (MessagePack)
- **vcpkg:** `msgpack`
- **Dokumentation:** [https://msgpack.org/](https://msgpack.org/)
- **Lizenz:** Boost Software License 1.0

### 29. tl-expected
- **Version:** Aktuell über vcpkg
- **Zweck:** Fehlerbehandlung ohne Exceptions (std::expected-Implementierung)
- **vcpkg:** `tl-expected`
- **Dokumentation:** [https://github.com/TartanLlama/expected](https://github.com/TartanLlama/expected)
- **Lizenz:** CC0 1.0

### 30. ONNX Runtime
- **Version:** Aktuell über vcpkg
- **Zweck:** ML-Inferenz für eingebettete Modelle (ONNX-Format)
- **vcpkg:** `onnxruntime`
- **Dokumentation:** [https://onnxruntime.ai/docs/](https://onnxruntime.ai/docs/)
- **Lizenz:** MIT

---

## 🎨 Optionale Features

Diese Bibliotheken werden nur bei Aktivierung des entsprechenden Features geladen:

### GPU Acceleration (Feature: `gpu`)
- FAISS ist als Kern-Abhängigkeit bereits enthalten; dieses Feature aktiviert GPU-spezifische Pfade.
- CMake: `THEMIS_ENABLE_GPU=ON`

### LLM Integration (Feature: `llm`)
- **llama.cpp** - Eingebettete LLM Inference
  - Git Submodule: `llama.cpp/`
  - CMake: `THEMIS_ENABLE_LLM=ON`
  - Dokumentation: [https://github.com/ggerganov/llama.cpp](https://github.com/ggerganov/llama.cpp)
  - Lizenz: MIT

### gRPC / RPC Support (Feature: `rpc`, `grpc`)
- gRPC ist als Kern-Abhängigkeit bereits enthalten; dieses Feature ergänzt:
- **Protocol Buffers**
  - vcpkg: `protobuf`
  - Dokumentation: [https://protobuf.dev/](https://protobuf.dev/)
  - Lizenz: BSD 3-Clause
- CMake: `THEMIS_ENABLE_GRPC=ON`

### HTTP/2 Support (Feature: `http2`)
- **nghttp2** - HTTP/2 C Library
  - vcpkg: `nghttp2`
  - CMake: `THEMIS_ENABLE_HTTP2=ON`
  - Dokumentation: [https://nghttp2.org/](https://nghttp2.org/)
  - Lizenz: MIT

### HTTP/3 Support (Feature: `http3`)
- **nghttp3** - HTTP/3 Library
  - vcpkg: `nghttp3`
  - CMake: `THEMIS_ENABLE_HTTP3=ON`
  - Dokumentation: [https://nghttp2.org/nghttp3/](https://nghttp2.org/nghttp3/)
  - Lizenz: MIT
- **ngtcp2** - QUIC Protocol Implementation
  - vcpkg: `ngtcp2` mit Feature: `openssl`
  - Dokumentation: [https://nghttp2.org/ngtcp2/](https://nghttp2.org/ngtcp2/)
  - Lizenz: MIT

### WebSocket / CDC Support (Feature: `websocket`)
- Verwendet Boost.Beast (bereits als Kern-Abhängigkeit enthalten)
- CMake: `THEMIS_ENABLE_WEBSOCKET=ON`

### Model Context Protocol (Feature: `mcp`)
- **Model Context Protocol (MCP)** - LLM-Tool-Integration
  - Keine zusätzlichen vcpkg-Abhängigkeiten
  - CMake: `THEMIS_ENABLE_MCP=ON`
  - Dokumentation: [https://modelcontextprotocol.io/](https://modelcontextprotocol.io/)

### MQTT Support (Feature: `mqtt`)
- **Eclipse Paho MQTT C++** - MQTT Client
  - vcpkg: `paho-mqttpp3`
  - CMake: `THEMIS_ENABLE_MQTT=ON`
  - Dokumentation: [https://eclipse.dev/paho/](https://eclipse.dev/paho/)
  - Lizenz: EPL 2.0 / EDL 1.0

### PostgreSQL Wire Protocol (Feature: `postgres-wire`)
- Keine zusätzlichen vcpkg-Abhängigkeiten
- CMake: `THEMIS_ENABLE_POSTGRES_WIRE=ON`

### GDAL Integration (Feature: `gdal`)
- **GDAL** - Geospatial Data Abstraction Library (Shapefile, GeoTIFF)
  - vcpkg: `gdal`
  - CMake: `THEMIS_ENABLE_GDAL=ON`
  - Dokumentation: [https://gdal.org/](https://gdal.org/)
  - Lizenz: MIT

### Cloud Storage (Feature: `cloud-storage`)
- **AWS SDK C++** - Amazon S3 / Transfer
  - vcpkg: `aws-sdk-cpp` mit Features: `s3`, `transfer`
  - Dokumentation: [https://docs.aws.amazon.com/sdk-for-cpp/](https://docs.aws.amazon.com/sdk-for-cpp/)
  - Lizenz: Apache 2.0
- **Azure Storage C++** - Azure Blob Storage
  - vcpkg: `azure-storage-cpp`
  - Dokumentation: [https://azure.github.io/azure-storage-cpp/](https://azure.github.io/azure-storage-cpp/)
  - Lizenz: Apache 2.0
- **Google Cloud C++** - Google Cloud Storage
  - vcpkg: `google-cloud-cpp` mit Feature: `storage`
  - Dokumentation: [https://cloud.google.com/cpp/docs/reference/storage/latest](https://cloud.google.com/cpp/docs/reference/storage/latest)
  - Lizenz: Apache 2.0

### CUDA Support (Feature: `cuda`)
- **NVIDIA CUDA Toolkit** - GPU Compute
  - Externe Installation erforderlich
  - CMake: `THEMIS_ENABLE_CUDA=ON`
  - Dokumentation: [https://docs.nvidia.com/cuda/](https://docs.nvidia.com/cuda/)
  - Lizenz: NVIDIA EULA

---

## 🔧 Build-System

### vcpkg - Paketmanager
- **Version:** Integriert als Git Submodule
- **Baseline:** `19e99cbbaa1e9857d55153a1cf3e74ee969719e0`
- **Modus:** Offline-First (vcpkg/downloads/ Cache)
- **Dokumentation:** [https://vcpkg.io/](https://vcpkg.io/)
- **vcpkg.json:** Deklarative Abhängigkeitsverwaltung

### CMake - Build Generator
- **Minimum Version:** 3.20
- **Standard:** C++20
- **Presets:** CMakePresets.json für VS 2022, Linux, Docker
- **Dokumentation:** [https://cmake.org/documentation/](https://cmake.org/documentation/)

---

## 📋 Paketmanagement

### Dependency Resolution

ThemisDB verwendet eine **vcpkg manifest mode** Architektur:

1. **vcpkg.json** - Deklariert alle Abhängigkeiten
2. **vcpkg-configuration.json** - vcpkg Registries
3. **CMakeLists.txt** - Build-Logik, Feature-Flags
4. **CMakePresets.json** - Vorkonfigurierte Build-Varianten

### Offline-First Strategy

```bash
# 1. Einmalig: Download aller Source-Archive
./scripts/setup-vcpkg-offline.sh    # Linux/macOS
.\scripts\setup-vcpkg-offline.ps1   # Windows

# 2. Build (100% offline möglich)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

**Siehe:** [VCPKG_OFFLINE_STRATEGY.md](VCPKG_OFFLINE_STRATEGY.md)

### Version Pinning

- **vcpkg baseline:** Fixiert auf getesteten Stand
- **vcpkg.json builtin-baseline:** `19e99cbbaa1e9857d55153a1cf3e74ee969719e0`
- **Reproduzierbare Builds:** Identische Library-Versionen auf allen Plattformen

---

## 🔗 Verwandte Dokumentation

- [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md) - Alle CMake Schalter
- [Deployment Strategy](deployment_strategy.md) - Build & Deployment
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Offline Build System
- [Docker Deployment](DOCKER_DEPLOYMENT.md) - Container Images
- [README](README.md) - Deployment Dokumentations-Übersicht

---

## 🚀 Quick Reference

### Häufig verwendete vcpkg Features

```bash
# LLM Support aktivieren
cmake -B build -DTHEMIS_ENABLE_LLM=ON

# GPU Support aktivieren
cmake -B build -DTHEMIS_ENABLE_GPU=ON
# vcpkg.json Feature: "gpu"

# gRPC Support aktivieren
cmake -B build -DTHEMIS_ENABLE_GRPC=ON
# vcpkg.json Feature: "grpc"

# HTTP/2 Support aktivieren
cmake -B build -DTHEMIS_ENABLE_HTTP2=ON
# vcpkg.json Feature: "http2"

# HTTP/3 (QUIC) Support aktivieren
cmake -B build -DTHEMIS_ENABLE_HTTP3=ON
# vcpkg.json Feature: "http3"

# WebSocket / CDC Support aktivieren
cmake -B build -DTHEMIS_ENABLE_WEBSOCKET=ON
# vcpkg.json Feature: "websocket"

# MCP Support aktivieren
cmake -B build -DTHEMIS_ENABLE_MCP=ON
# vcpkg.json Feature: "mcp"

# MQTT Support aktivieren
cmake -B build -DTHEMIS_ENABLE_MQTT=ON
# vcpkg.json Feature: "mqtt"

# PostgreSQL Wire Protocol aktivieren
cmake -B build -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
# vcpkg.json Feature: "postgres-wire"

# GDAL Integration aktivieren
cmake -B build -DTHEMIS_ENABLE_GDAL=ON
# vcpkg.json Feature: "gdal"

# Cloud Storage aktivieren
# vcpkg.json Feature: "cloud-storage"
```

### vcpkg.json Features nutzen

```bash
# Feature bei vcpkg install angeben
vcpkg install themisdb[gpu,llm,grpc,mqtt,gdal,cloud-storage]

# Oder in CMake über Feature-Flag
cmake -B build \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GRPC=ON
```

---

## 📊 Statistik

| Kategorie | Anzahl |
|-----------|--------|
| **Kern-Bibliotheken** | 30 |
| **Optional: GPU** | aktiviert FAISS GPU-Pfade (FAISS = Kern) |
| **Optional: LLM** | 1 (Git Submodule: llama.cpp) |
| **Optional: Protokolle** | gRPC+Protobuf, HTTP/2, HTTP/3, WebSocket, MCP, MQTT, Postgres Wire = 7 Features |
| **Optional: Geo** | 1 (GDAL) |
| **Optional: Cloud Storage** | 3 (AWS, Azure, GCP) |
| **Build-Tools** | 2 (vcpkg, CMake) |
| **Gesamt Dependencies** | 40+ |

**Gesamtgröße vcpkg downloads Cache:** ~2.5 GB  
**Build Time (erste Build):** 20-35 min je nach Platform  
**Build Time (inkrementell):** 2-10 min
