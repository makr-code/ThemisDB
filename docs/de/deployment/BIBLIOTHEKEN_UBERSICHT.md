# ThemisDB Bibliotheken-Übersicht

**Stand:** 26. Dezember 2025  
**Version:** v1.3.1  
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

**Gesamt:** 17 Kern-Bibliotheken + optionale Feature-Bibliotheken

---

## 📦 Kern-Abhängigkeiten

Diese Bibliotheken sind für alle ThemisDB-Builds erforderlich:

### 1. OpenSSL
- **Version:** Aktuell über vcpkg
- **Zweck:** TLS/SSL-Verschlüsselung, kryptographische Funktionen
- **vcpkg:** `openssl`
- **Dokumentation:** [https://www.openssl.org/docs/](https://www.openssl.org/docs/)
- **Lizenz:** Apache 2.0

### 2. RocksDB
- **Version:** Aktuell über vcpkg
- **Zweck:** Embedded Key-Value Store, LSM-Tree Storage Engine
- **vcpkg:** `rocksdb` mit Features: `lz4`, `zstd`
- **Dokumentation:** [https://rocksdb.org/](https://rocksdb.org/)
- **Lizenz:** Apache 2.0 / GPL 2.0
- **Features:**
  - `lz4`: LZ4 Kompression für schnelle I/O
  - `zstd`: Zstandard Kompression für hohe Kompressionsraten

### 3. simdjson
- **Version:** Aktuell über vcpkg
- **Zweck:** Ultra-schnelles JSON Parsing (Gigabytes pro Sekunde)
- **vcpkg:** `simdjson`
- **Dokumentation:** [https://simdjson.org/](https://simdjson.org/)
- **Lizenz:** Apache 2.0

### 4. Intel TBB (Threading Building Blocks)
- **Version:** Aktuell über vcpkg
- **Zweck:** Task-basierte Parallelisierung, Thread-Pool
- **vcpkg:** `tbb`
- **Dokumentation:** [https://www.intel.com/content/www/us/en/docs/onetbb/](https://www.intel.com/content/www/us/en/docs/onetbb/)
- **Lizenz:** Apache 2.0

### 5. Apache Arrow
- **Version:** Aktuell über vcpkg
- **Zweck:** Spaltenbasierte In-Memory Datenverarbeitung, Zero-Copy Analytics
- **vcpkg:** `arrow` mit Features: `parquet`, `compute`
- **Dokumentation:** [https://arrow.apache.org/docs/](https://arrow.apache.org/docs/)
- **Lizenz:** Apache 2.0
- **Features:**
  - `parquet`: Apache Parquet Format Support
  - `compute`: Arrow Compute Kernels für Analytics

### 6. HNSWlib
- **Version:** Aktuell über vcpkg
- **Zweck:** Approximate Nearest Neighbor Search (Vektor-Ähnlichkeitssuche)
- **vcpkg:** `hnswlib`
- **Dokumentation:** [https://github.com/nmslib/hnswlib](https://github.com/nmslib/hnswlib)
- **Lizenz:** Apache 2.0

### 7. Google Test
- **Version:** Aktuell über vcpkg
- **Zweck:** Unit Testing Framework
- **vcpkg:** `gtest`
- **Dokumentation:** [https://google.github.io/googletest/](https://google.github.io/googletest/)
- **Lizenz:** BSD 3-Clause

### 8. Google Benchmark
- **Version:** Aktuell über vcpkg
- **Zweck:** Microbenchmarking Framework
- **vcpkg:** `benchmark`
- **Dokumentation:** [https://github.com/google/benchmark](https://github.com/google/benchmark)
- **Lizenz:** Apache 2.0

### 9. Boost.Asio
- **Version:** Aktuell über vcpkg
- **Zweck:** Asynchrone Netzwerk-I/O
- **vcpkg:** `boost-asio`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/doc/html/boost_asio.html](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- **Lizenz:** Boost Software License 1.0

### 10. Boost.Beast
- **Version:** Aktuell über vcpkg
- **Zweck:** HTTP/WebSocket Server über Asio
- **vcpkg:** `boost-beast`
- **Dokumentation:** [https://www.boost.org/doc/libs/release/libs/beast/](https://www.boost.org/doc/libs/release/libs/beast/)
- **Lizenz:** Boost Software License 1.0

### 11. spdlog
- **Version:** Aktuell über vcpkg
- **Zweck:** Hochperformantes Logging Framework
- **vcpkg:** `spdlog`
- **Dokumentation:** [https://github.com/gabime/spdlog](https://github.com/gabime/spdlog)
- **Lizenz:** MIT

### 12. nlohmann/json
- **Version:** Aktuell über vcpkg
- **Zweck:** JSON für Modern C++, Schema-Validierung
- **vcpkg:** `nlohmann-json`
- **Dokumentation:** [https://json.nlohmann.me/](https://json.nlohmann.me/)
- **Lizenz:** MIT

### 13. OpenTelemetry C++
- **Version:** Aktuell über vcpkg
- **Zweck:** Distributed Tracing, Observability
- **vcpkg:** `opentelemetry-cpp` mit Feature: `otlp-http`
- **Dokumentation:** [https://opentelemetry.io/docs/instrumentation/cpp/](https://opentelemetry.io/docs/instrumentation/cpp/)
- **Lizenz:** Apache 2.0
- **CMake Option:** `THEMIS_ENABLE_TRACING` (Standard: ON)
- **Features:**
  - `otlp-http`: OTLP HTTP Exporter für Jaeger/Tempo

### 14. libcurl
- **Version:** Aktuell über vcpkg
- **Zweck:** HTTP Client für externe API-Aufrufe
- **vcpkg:** `curl`
- **Dokumentation:** [https://curl.se/libcurl/](https://curl.se/libcurl/)
- **Lizenz:** curl License (MIT-style)

### 15. yaml-cpp
- **Version:** Aktuell über vcpkg
- **Zweck:** YAML Konfigurationsdateien parsen/schreiben
- **vcpkg:** `yaml-cpp`
- **Dokumentation:** [https://github.com/jbeder/yaml-cpp](https://github.com/jbeder/yaml-cpp)
- **Lizenz:** MIT

### 16. Zstandard (zstd)
- **Version:** Aktuell über vcpkg
- **Zweck:** Hochleistungs-Kompression für Storage
- **vcpkg:** `zstd`
- **Dokumentation:** [https://facebook.github.io/zstd/](https://facebook.github.io/zstd/)
- **Lizenz:** BSD / GPLv2

### 17. mimalloc
- **Version:** Aktuell über vcpkg
- **Zweck:** Hochperformanter Memory Allocator
- **vcpkg:** `mimalloc`
- **Dokumentation:** [https://microsoft.github.io/mimalloc/](https://microsoft.github.io/mimalloc/)
- **Lizenz:** MIT
- **CMake Option:** `THEMIS_ENABLE_MIMALLOC` (Standard: OFF)
- **Performance:** +10-20% bei Memory-intensiven Workloads

---

## 🎨 Optionale Features

Diese Bibliotheken werden nur bei Aktivierung des entsprechenden Features geladen:

### GPU Acceleration (Feature: `gpu`)
- **FAISS** - Facebook AI Similarity Search
  - vcpkg: `faiss`
  - CMake: `THEMIS_ENABLE_GPU=ON`
  - Dokumentation: [https://faiss.ai/](https://faiss.ai/)
  - Lizenz: MIT

### LLM Integration (Feature: `llm`)
- **llama.cpp** - Eingebettete LLM Inference
  - Git Submodule: `llama.cpp/`
  - CMake: `THEMIS_ENABLE_LLM=ON`
  - Dokumentation: [https://github.com/ggerganov/llama.cpp](https://github.com/ggerganov/llama.cpp)
  - Lizenz: MIT

### gRPC Support (Feature: `rpc`, `grpc`)
- **gRPC** - RPC Framework
  - vcpkg: `grpc`
  - CMake: `THEMIS_ENABLE_GRPC=ON` (Standard: ON in v1.3.0+)
  - Dokumentation: [https://grpc.io/docs/](https://grpc.io/docs/)
  - Lizenz: Apache 2.0
- **Protocol Buffers**
  - vcpkg: `protobuf`
  - Dokumentation: [https://protobuf.dev/](https://protobuf.dev/)
  - Lizenz: BSD 3-Clause

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

### MQTT Support (Feature: `mqtt`)
- **Eclipse Paho MQTT C++** - MQTT Client
  - vcpkg: `paho-mqttpp3`
  - CMake: `THEMIS_ENABLE_MQTT=ON`
  - Dokumentation: [https://eclipse.dev/paho/](https://eclipse.dev/paho/)
  - Lizenz: EPL 2.0 / EDL 1.0

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
- **Baseline:** `bee7b66f0219eeb463dc1ff77ad6ad0211f94f48`
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
- **vcpkg.json builtin-baseline:** `bee7b66f0219eeb463dc1ff77ad6ad0211f94f48`
- **Reproduzierbare Builds:** Identische Library-Versionen auf allen Plattformen

---

## 🔗 Verwandte Dokumentation

- [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md) - Alle CMake Schalter
- [Deployment Strategy](deployment_strategy.md) - Build & Deployment
- [vcpkg Offline Strategy](VCPKG_OFFLINE_STRATEGY.md) - Offline Build System
- [Docker Deployment](DOCKER_DEPLOYMENT.md) - Container Images

---

## 📊 Statistik

| Kategorie | Anzahl |
|-----------|--------|
| **Kern-Bibliotheken** | 17 |
| **Optional: GPU** | 1 |
| **Optional: LLM** | 1 (Git Submodule) |
| **Optional: Protokolle** | 5 (gRPC, HTTP/2, HTTP/3, MQTT) |
| **Build-Tools** | 2 (vcpkg, CMake) |
| **Gesamt Dependencies** | 26+ |

**Gesamtgröße vcpkg downloads Cache:** ~2.5 GB  
**Build Time (erste Build):** 20-35 min je nach Platform  
**Build Time (inkrementell):** 2-10 min
