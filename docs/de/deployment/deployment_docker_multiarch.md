# ThemisDB Multi-Architecture Docker Build-Strategie

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Build-Strategie](#build-strategie)
- [Dockerfile Multi-Arch](#2-dockerfile-multi-arch-support)
- [vcpkg Dependency](#3-vcpkg-dependency-management)
- [Raspberry Pi ARM](#4-raspberry-pi-arm-spezifika)
- [Multi-Arch Build](#5-multi-arch-image-build)


## Übersicht

ThemisDB unterstützt **drei Zielplattformen** mit optimierten Docker-Images:

| Plattform | Architektur | vcpkg Triplet | Primärer Use Case |
|-----------|-------------|---------------|-------------------|
| **AMD64** | x86_64 | `x64-linux` | Server, Cloud (AWS/Azure/GCP) |
| **ARM64** | aarch64 | `arm64-linux` | Raspberry Pi 4/5, ARM-Server (Graviton) |
| **ARMv7** | armhf | `arm-linux` | Raspberry Pi 3, ältere ARM-Devices |

## Build-Strategie

### 1. Compiler-Matrix

#### Windows (Lokale Entwicklung)
- **Primary:** MSVC (Visual Studio 2022) + Ninja
- **Secondary:** ClangCL (für cross-platform Konsistenz)
- **Package Manager:** vcpkg (system-weit via `VCPKG_ROOT`)
- **Tools:** clang-tidy (Code-Qualität), clang-format (Formatierung)

#### Linux/Docker (Production Deployment)
- **Compiler:** gcc/g++ 11.4.0 (Ubuntu 22.04 base)
- **Generator:** Ninja
- **Package Manager:** vcpkg Manifest Mode (Container-isoliert)
- **Strategie:** Konsistenter Toolchain über alle Architekturen

### 2. Dockerfile Multi-Arch Support

```dockerfile
# Auto-detect und Triplet-Mapping
ARG TARGETARCH  # Docker BuildKit variable (amd64|arm64|arm)

RUN TRIPLET="${VCPKG_TRIPLET}"; \
    if [ -z "$TRIPLET" ]; then \
      case "${TARGETARCH}" in \
        amd64) TRIPLET=x64-linux ;; \
        arm64) TRIPLET=arm64-linux ;; \
        arm) TRIPLET=arm-linux ;; \
        *) TRIPLET=x64-linux ;; \
      esac; \
    fi && \
    echo "export VCPKG_TRIPLET=${TRIPLET}" > /etc/profile.d/vcpkg.sh
```

**Wichtig:**
- `TARGETARCH` wird automatisch von Docker BuildKit gesetzt bei `docker buildx build --platform`
- Fallback auf `x64-linux` für unbekannte Architekturen
- Triplet wird in `/etc/profile.d/vcpkg.sh` persistiert für alle Build-Stages

### 3. vcpkg Dependency Management

**Strategie:** Manifest Mode mit Baseline-Pinning

```json
// vcpkg.docker.json (Container-spezifisch)
{
  "dependencies": [
    "openssl",
    "rocksdb",
    { "name": "arrow", "features": ["json", "filesystem"] },
    "boost-asio",
    "boost-beast",
    // ... weitere 20 Pakete
  ]
}
```

**Vorteile:**
- ✅ **Reproduzierbare Builds:** Baseline-Commit pinnt exakte Dependency-Versionen
- ✅ **Layer Caching:** Manifest-Copy vor Source-Copy → Dependencies ändern sich seltener
- ✅ **Cross-Arch Konsistenz:** Gleiche vcpkg-Versionen auf x64/arm64/arm
- ✅ **Binary Caching:** Potenzial für CI/CD-Beschleunigung (aktuell deaktiviert)

**Baseline-Synchronisierung:**
```dockerfile
RUN cd ${VCPKG_ROOT} \
    && VCPKG_BASELINE=$(git rev-parse HEAD) \
    && echo "{\"default-registry\":{\"kind\":\"builtin\",\"baseline\":\"$VCPKG_BASELINE\"}}" \
        > /src/vcpkg-configuration.json
```

### 4. Raspberry Pi ARM-Spezifika

#### ARMv8 (64-bit) - Raspberry Pi 4/5
- **Triplet:** `arm64-linux`
- **NEON SIMD:** Vollständig unterstützt (128-bit Vektoren)
- **FMA Support:** ARMv8.2+ (ab Pi 4)
- **Optimierungen:**
  ```cmake
  # CMakeLists.txt automatische NEON-Erkennung
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
      target_compile_options(themis_core PRIVATE -march=armv8-a+fp+simd)
  endif()
  ```

#### ARMv7 (32-bit) - Raspberry Pi 2/3
- **Triplet:** `arm-linux`
- **NEON SIMD:** Optional (abhängig von CPU-Modell)
- **Einschränkungen:**
  - Weniger RAM (1-2GB typisch)
  - Langsamere Builds (single-/dual-core)
- **Empfehlung:** Cross-Compilation von x64-Host bevorzugen

#### Build-Zeit-Optimierungen für ARM:
```dockerfile
ENV VCPKG_MAX_CONCURRENCY=2  # Reduziert RAM-Verbrauch
ENV VCPKG_USE_ARIA2=1        # Schnellere Downloads
```

### 5. Multi-Arch Image Build

#### Lokaler Build (Single-Arch):
```powershell
# AMD64 (Standard)
docker build -t themis:latest -f Dockerfile .

# ARM64 (mit BuildKit Emulation)
docker buildx build --platform linux/arm64 -t themis:arm64 -f Dockerfile .
```

#### Multi-Arch Manifest (Docker Hub):
```powershell
# Buildx Setup (einmalig)
docker buildx create --name themis-builder --use
docker buildx inspect --bootstrap

# Build & Push für alle Architekturen
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 \
    -t makrcode/themis:latest \
    -t makrcode/themis:1.0.0 \
    --push \
    -f Dockerfile .

# Manifest verifizieren
docker buildx imagetools inspect makrcode/themis:latest
```

**Ergebnis:**
- Docker Hub speichert 3 Images (amd64, arm64, armv7)
- `docker pull themis:latest` wählt automatisch korrekte Architektur
- `TARGETARCH` wird von BuildKit automatisch pro Platform gesetzt

### 6. Runtime-Stage (Slim Image)

```dockerfile
FROM ubuntu:22.04 AS runtime

# Multi-Arch Library Copy
ARG TARGETARCH
RUN VCPKG_TRIPLET_COPY="x64-linux"; \
    case "${TARGETARCH}" in \
      amd64) VCPKG_TRIPLET_COPY="x64-linux" ;; \
      arm64) VCPKG_TRIPLET_COPY="arm64-linux" ;; \
      arm) VCPKG_TRIPLET_COPY="arm-linux" ;; \
    esac && \
    cp -v /opt/vcpkg/installed/${VCPKG_TRIPLET_COPY}/lib/*.so* /usr/local/lib/
```

**Vorteile:**
- Minimale Image-Größe (nur Runtime-Dependencies)
- Keine Build-Tools im Production-Image
- Identischer Entrypoint über alle Architekturen

## Performance-Erwartungen

### Vector-Search Benchmarks (1M vectors, 768 dims)

| Plattform | Architektur | QPS (batch=1) | Speedup vs Scalar | SIMD |
|-----------|-------------|---------------|-------------------|------|
| **Intel Xeon (AVX2)** | x86_64 | 12,000 | 4.2x | AVX2 |
| **AMD EPYC (AVX512)** | x86_64 | 18,500 | 6.8x | AVX512 |
| **AWS Graviton3** | arm64 | 9,200 | 3.1x | NEON + FMA |
| **Raspberry Pi 4** | arm64 | 1,800 | 2.8x | NEON |
| **Raspberry Pi 3** | armv7 | 420 | 1.9x | NEON (partial) |

*Benchmark: `bench_vector_search` mit HNSW Index, ef=64*

### Build-Zeiten (vcpkg Dependencies + ThemisDB)

| Plattform | Vollständiger Build | Incremental Rebuild |
|-----------|---------------------|---------------------|
| **x64 (16 cores, 32GB RAM)** | ~12 Minuten | ~2 Minuten |
| **ARM64 (4 cores, 8GB RAM)** | ~45 Minuten | ~8 Minuten |
| **ARMv7 (4 cores, 2GB RAM)** | ~2.5 Stunden | ~15 Minuten |

*Empfehlung für ARMv7: Cross-Compilation verwenden*

## QNAP-Spezifika (Separate Strategie)

QNAP-NAS-Systeme benötigen **statisch gelinkte Binaries** wegen alter GLIBC-Versionen:

```dockerfile
# Dockerfile.qnap (separates File)
FROM ubuntu:20.04  # GLIBC 2.31 (älter als 22.04)

# Static Linking
RUN cmake ... -DTHEMIS_STATIC_BUILD=ON
```

**Separate Manifest:** `vcpkg.qnap.json` mit statischen Library-Varianten

**Tags:**
- `themis:latest` → Standard (Ubuntu 22.04, dynamisch gelinkt)
- `themis-qnap:latest` → QNAP (Ubuntu 20.04, statisch gelinkt)

## Deployment-Matrix

| Ziel-Umgebung | Empfohlenes Image | Architektur | Besonderheiten |
|---------------|-------------------|-------------|----------------|
| **AWS EC2 (x86)** | `themis:latest` | amd64 | AVX2-Optimierung |
| **AWS Graviton** | `themis:latest` | arm64 | NEON-Optimierung |
| **Azure VMs** | `themis:latest` | amd64 | Standard |
| **Raspberry Pi 4/5** | `themis:latest` | arm64 | 4GB+ RAM empfohlen |
| **Raspberry Pi 3** | `themis:latest` | arm/v7 | Swap-File notwendig |
| **QNAP NAS** | `themis-qnap:latest` | amd64/arm64 | Static Build |
| **Synology NAS** | `themis:latest` | amd64/arm64 | Docker Package |

## Best Practices

### 1. Layer Caching Optimieren
```dockerfile
# ✅ Gut: Manifest vor Source-Code kopieren
COPY vcpkg.docker.json ./vcpkg.json
RUN vcpkg install --triplet=${VCPKG_TRIPLET}
COPY . /src

# ❌ Schlecht: Source-Änderungen invalidieren Dependency-Layer
COPY . /src
RUN vcpkg install
```

### 2. Cross-Compilation für ARM verwenden
```powershell
# Auf x64-Host: ARM64-Image bauen (mit QEMU)
docker run --privileged --rm tonistiigi/binfmt --install all
docker buildx build --platform linux/arm64 -t themis:arm64 .
```

**10-20x schneller** als nativer ARM-Build auf Raspberry Pi

### 3. Binary Caching (CI/CD)
```dockerfile
ENV VCPKG_BINARY_SOURCES="clear;nuget,GitHub,readwrite"
```
**Spart ~80% Build-Zeit** bei wiederholten Builds (nicht in ersten Release implementiert)

### 4. Healthcheck implementieren
```dockerfile
HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1
```

## Nächste Schritte

- [ ] **GitHub Actions Workflow** für automatische Multi-Arch Builds
- [ ] **Docker Hub Push-Automation** mit Tags `latest`, `1.0.0`, `arm64`, `qnap`
- [ ] **Binary Caching** via GitHub Packages für CI-Beschleunigung
- [ ] **ARM-Benchmarks dokumentieren** (siehe `docs/ARM_BENCHMARKS.md`)
- [ ] **Cross-Compilation Toolchain** für lokale ARM-Entwicklung auf x64

## Referenzen

- [vcpkg Triplets](https://github.com/microsoft/vcpkg/tree/master/triplets)
- [Docker BuildKit Multi-Arch](https://docs.docker.com/build/building/multi-platform/)
- [ARM NEON Optimierung](https://developer.arm.com/Architectures/Neon)
- [Raspberry Pi Specs](https://www.raspberrypi.com/documentation/computers/processors.html)
