# Docker GPU-Unterstützung für ThemisDB
## Vulkan und CUDA in Containern

**Datum:** 15. Januar 2026  
**Version:** 1.0  
**Kategorie:** Docker / GPU / Deployment

---

## 📋 Problem

**Herausforderung:** GPU/VRAM-Unterstützung ist in ThemisDB seit v1.3.5+ standardmäßig aktiviert (Vulkan), aber Docker-Container haben **eingeschränkten GPU-Zugriff**.

### Warum ist GPU in Containern schwierig?

1. **GPU-Treiber:** Container benötigen Host-GPU-Treiber
2. **Device Access:** `/dev/dri` (Vulkan) oder `/dev/nvidia*` (CUDA) muss gemountet werden
3. **Runtime:** Spezielle Container-Runtime nötig (nvidia-container-runtime, etc.)
4. **SDK Installation:** Vulkan SDK oder CUDA Toolkit im Container-Image
5. **Portabilität:** Image muss ohne GPU funktionieren (CPU-Fallback)

---

## 🎯 Lösung: Strategie mit CPU-Fallback

**Empfohlene Strategie:**

1. ✅ **Build mit Vulkan-Support** (standardmäßig aktiviert)
2. ✅ **Graceful CPU-Fallback** zur Laufzeit (wenn GPU nicht verfügbar)
3. ✅ **Optional: GPU-Runtime** für Performance (via nvidia-docker oder DRI)
4. ✅ **Multi-Stage Build** mit GPU- und CPU-Varianten

### Vorteile dieser Strategie

- ✅ **Funktioniert überall**: Auch ohne GPU (CPU-Fallback)
- ✅ **Beste Performance**: Nutzt GPU wenn verfügbar
- ✅ **Keine Breaking Changes**: Bestehende Deployments funktionieren weiter
- ✅ **Flexible Deployment**: Gleiches Image für GPU- und CPU-Hosts

---

## 🔧 Docker Build-Konfiguration

### Variante 1: CPU-Fallback (Standard, empfohlen)

**Dockerfile.unified (Standard-Build):**

```dockerfile
# Stage 4: Build ThemisDB mit Vulkan (CPU-Fallback enabled)
FROM deps AS build

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=OFF
ARG ENABLE_GPU=OFF  # ← Neu: Explizite GPU-Kontrolle

WORKDIR /src

# Build ThemisDB
RUN set -eux; \
    echo "Building ThemisDB ${THEMIS_EDITION} (LLM=${ENABLE_LLM}, GPU=${ENABLE_GPU})..."; \
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_EDITION=${THEMIS_EDITION} \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} \
        # GPU-Backends nur wenn explizit aktiviert
        $([ "$ENABLE_GPU" = "ON" ] && echo "-DTHEMIS_ENABLE_VULKAN=ON" || echo "-DTHEMIS_ENABLE_VULKAN=OFF") \
        $([ "$ENABLE_GPU" = "ON" ] && echo "-DTHEMIS_ENABLE_CUDA=OFF" || echo "") \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${TRIPLET} && \
    ninja -C build -j$(nproc)
```

**Problem mit dieser Variante:**
- Widerspricht der neuen Default-Policy (Vulkan=ON in COMMUNITY+)
- Nutzer müssen manuell `ENABLE_GPU=ON` setzen

---

### Variante 2: Vulkan ON mit Runtime-Fallback (empfohlen)

**Dockerfile.unified (mit Vulkan SDK):**

```dockerfile
# ============================================================================
# Stage 1.5: vulkan-sdk - Optional Vulkan SDK layer
# ============================================================================
FROM base AS vulkan-sdk

ARG INSTALL_VULKAN=ON

# Install Vulkan SDK (optional, für GPU-Builds)
RUN if [ "$INSTALL_VULKAN" = "ON" ]; then \
        echo "Installing Vulkan SDK..."; \
        wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add - && \
        wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list \
            https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list && \
        apt-get update && \
        apt-get install -y --no-install-recommends \
            vulkan-sdk \
            libvulkan-dev \
            vulkan-tools && \
        rm -rf /var/lib/apt/lists/*; \
    else \
        echo "Vulkan SDK not installed (CPU-only build)"; \
    fi

# ============================================================================
# Stage 4: build - Compile ThemisDB (mit Vulkan wenn verfügbar)
# ============================================================================
FROM vulkan-sdk AS build

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=OFF

# Build mit Edition-Defaults (Vulkan=ON in COMMUNITY+, siehe CMake)
RUN set -eux; \
    echo "Building ThemisDB ${THEMIS_EDITION} (LLM=${ENABLE_LLM})..."; \
    # Edition-Defaults aus CMake werden verwendet:
    # - COMMUNITY: THEMIS_ENABLE_VULKAN=ON (automatisch)
    # - MINIMAL: THEMIS_ENABLE_VULKAN=OFF (automatisch)
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_EDITION=${THEMIS_EDITION} \
        -DTHEMIS_ENABLE_LLM=${ENABLE_LLM} \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=${TRIPLET} && \
    ninja -C build -j$(nproc)
```

**Build-Kommandos:**

```bash
# COMMUNITY mit Vulkan (Standard)
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=ON \
  -t themisdb:community-gpu \
  -f docker/Dockerfile.unified .

# COMMUNITY ohne Vulkan (CPU-only)
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=OFF \
  -t themisdb:community-cpu \
  -f docker/Dockerfile.unified .

# MINIMAL (immer CPU-only)
docker build \
  --build-arg THEMIS_EDITION=MINIMAL \
  --build-arg INSTALL_VULKAN=OFF \
  -t themisdb:minimal \
  -f docker/Dockerfile.unified .
```

---

## 🚀 Docker Runtime-Konfiguration

### Option 1: CPU-Only (Fallback, keine GPU)

**docker-compose.yml:**

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb:community-gpu  # Mit Vulkan gebaut
    container_name: themisdb
    ports:
      - "8529:8529"
    environment:
      - THEMIS_EDITION=COMMUNITY
      # Keine GPU-spezifischen Env-Vars
      # → Runtime erkennt fehlende GPU und fällt auf CPU zurück
    restart: unless-stopped
```

**Erwartetes Verhalten:**
```
[INFO] Edition: COMMUNITY - GPU limited to 24GB
[WARN] Vulkan backend: Not available (no GPU devices found in container)
[INFO] Using CPU backend (slower performance)
```

---

### Option 2: Vulkan GPU via DRI (empfohlen für Intel/AMD)

**docker-compose.yml:**

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb:community-gpu
    container_name: themisdb-gpu
    ports:
      - "8529:8529"
    devices:
      - /dev/dri:/dev/dri  # ← GPU-Zugriff via DRI
    volumes:
      - /usr/share/vulkan/icd.d:/usr/share/vulkan/icd.d:ro  # Vulkan ICD
    environment:
      - THEMIS_EDITION=COMMUNITY
      - VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/intel_icd.x86_64.json
    restart: unless-stopped
```

**Für AMD GPUs:**
```yaml
environment:
  - VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json
```

**Erwartetes Verhalten:**
```
[INFO] Edition: COMMUNITY - GPU limited to 24GB
[INFO] Vulkan backend: Available
[INFO] Device: Intel(R) UHD Graphics 770
[INFO] Using Vulkan GPU acceleration
```

---

### Option 3: NVIDIA GPU via nvidia-docker (CUDA)

**Voraussetzungen:**
```bash
# nvidia-container-runtime installieren
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
  sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
sudo systemctl restart docker
```

**docker-compose.yml:**

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb:community-gpu-cuda  # Mit CUDA gebaut
    container_name: themisdb-cuda
    runtime: nvidia  # ← NVIDIA Container Runtime
    ports:
      - "8529:8529"
    environment:
      - THEMIS_EDITION=COMMUNITY
      - NVIDIA_VISIBLE_DEVICES=all
      - NVIDIA_DRIVER_CAPABILITIES=compute,utility
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    restart: unless-stopped
```

**Build mit CUDA:**
```bash
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=ON \
  --build-arg INSTALL_CUDA=ON \
  -t themisdb:community-gpu-cuda \
  -f docker/Dockerfile.unified .
```

**Erwartetes Verhalten:**
```
[INFO] Edition: COMMUNITY - GPU limited to 24GB
[INFO] CUDA backend: Available
[INFO] Device: NVIDIA GeForce RTX 4090
[INFO] VRAM: 24 GB
[INFO] Using CUDA GPU acceleration
```

---

## 📊 Performance-Vergleich: Container vs. Native

| Setup | Vector Search | Build Time | Image Size | Komplexität |
|-------|--------------|------------|------------|-------------|
| **Docker CPU-only** | 120 ms | 20 min | 500 MB | ✅ Einfach |
| **Docker + Vulkan (DRI)** | 8 ms | 25 min | 650 MB | ⚠️ Mittel |
| **Docker + CUDA** | 5 ms | 30 min | 800 MB | ❌ Komplex |
| **Native (Host)** | 5-6 ms | 15 min | - | ✅ Einfach |

**Empfehlung:**
- **Entwicklung:** Docker CPU-only (einfach, portabel)
- **Produktion (Performance):** Native mit GPU (beste Performance)
- **Produktion (Container):** Docker + Vulkan DRI (guter Kompromiss)

---

## 🎯 Empfohlene Docker-Images

### Standard-Images (für vcpkg-Manifest)

```bash
# 1. MINIMAL (CPU-only, kleinste Größe)
docker build \
  --build-arg THEMIS_EDITION=MINIMAL \
  --build-arg INSTALL_VULKAN=OFF \
  -t themisdb:minimal \
  -f docker/Dockerfile.unified .

# 2. COMMUNITY (mit Vulkan, CPU-Fallback)
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=ON \
  -t themisdb:community \
  -f docker/Dockerfile.unified .

# 3. COMMUNITY CPU-only (ohne GPU-Overhead)
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=OFF \
  -t themisdb:community-cpu \
  -f docker/Dockerfile.unified .

# 4. ENTERPRISE (mit Vulkan + CUDA optional)
docker build \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=ON \
  -t themisdb:enterprise \
  -f docker/Dockerfile.unified .
```

### Image-Matrix

| Image-Tag | Edition | GPU | LLM | Größe | Use Case |
|-----------|---------|-----|-----|-------|----------|
| `themisdb:minimal` | MINIMAL | ❌ | ❌ | ~300 MB | IoT, Embedded |
| `themisdb:community-cpu` | COMMUNITY | ❌ | ✅ | ~500 MB | Dev, Testing |
| `themisdb:community` | COMMUNITY | ✅ Vulkan | ✅ | ~650 MB | Production |
| `themisdb:community-cuda` | COMMUNITY | ✅ CUDA | ✅ | ~800 MB | NVIDIA GPUs |
| `themisdb:enterprise` | ENTERPRISE | ✅ Vulkan | ✅ | ~700 MB | Production |

---

## 🔍 Troubleshooting

### Problem 1: "Vulkan backend: Not available" in Container

**Symptom:**
```
[WARN] Vulkan backend: Not available
[INFO] Using CPU backend
```

**Ursachen:**
1. Kein `/dev/dri` gemountet
2. Vulkan ICD nicht verfügbar
3. GPU nicht sichtbar im Container

**Lösung:**
```yaml
# docker-compose.yml
devices:
  - /dev/dri:/dev/dri
volumes:
  - /usr/share/vulkan/icd.d:/usr/share/vulkan/icd.d:ro
```

**Verifizieren:**
```bash
# In Container
docker exec themisdb vulkaninfo | head -20
# Sollte GPU zeigen
```

---

### Problem 2: "Permission denied" auf /dev/dri

**Symptom:**
```
[ERROR] Failed to open /dev/dri/renderD128: Permission denied
```

**Lösung:**
```yaml
# docker-compose.yml
services:
  themisdb:
    devices:
      - /dev/dri:/dev/dri
    group_add:
      - video  # ← Gruppe für GPU-Zugriff
      - render
```

---

### Problem 3: CUDA nicht verfügbar trotz nvidia-docker

**Symptom:**
```
[ERROR] CUDA error: no CUDA-capable device is detected
```

**Diagnose:**
```bash
# Auf Host
nvidia-smi  # Sollte GPU zeigen

# In Container
docker run --rm --gpus all nvidia/cuda:12.0-base nvidia-smi
# Sollte gleiche GPU zeigen
```

**Lösung:**
```yaml
runtime: nvidia  # Oder deploy.resources.reservations.devices
environment:
  - NVIDIA_VISIBLE_DEVICES=all
```

---

### Problem 4: Großes Image (>1GB)

**Ursache:** CUDA Toolkit, Vulkan SDK, Debug-Symbole

**Lösung: Multi-Stage Build mit slimmer Runtime**

```dockerfile
# Stage 5: runtime (slim)
FROM ubuntu:22.04 AS runtime

# Nur Runtime-Libs kopieren, keine Build-Tools
COPY --from=build /src/build/themis_server /usr/local/bin/
COPY --from=build /opt/llama.cpp/build/lib/*.so /usr/local/lib/

# Nur Vulkan Runtime (nicht SDK)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libvulkan1 \  # ← Runtime-only, keine Dev-Headers
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Image-Größe: ~400 MB statt 800 MB
```

---

## 📝 Zusammenfassung & Empfehlungen

### Für Entwicklung

```bash
# CPU-only, schnell, einfach
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=OFF \
  -t themisdb:dev \
  -f docker/Dockerfile.unified .

docker run -d -p 8529:8529 themisdb:dev
```

### Für Produktion (ohne dedizierte GPU)

```bash
# Mit Vulkan-Support, CPU-Fallback
docker build \
  --build-arg THEMIS_EDITION=COMMUNITY \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_VULKAN=ON \
  -t themisdb:prod \
  -f docker/Dockerfile.unified .

docker run -d -p 8529:8529 \
  --device /dev/dri:/dev/dri \
  themisdb:prod
```

### Für Produktion (mit NVIDIA GPU)

```bash
# Mit CUDA-Support
docker build \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --build-arg ENABLE_LLM=ON \
  --build-arg INSTALL_CUDA=ON \
  -t themisdb:prod-cuda \
  -f docker/Dockerfile.unified .

docker run -d -p 8529:8529 \
  --gpus all \
  --runtime nvidia \
  themisdb:prod-cuda
```

---

## 🎯 Nächste Schritte

1. **Dockerfile.unified aktualisieren** mit Vulkan SDK Stage
2. **docker-compose.yml** Beispiele für GPU-Runtime hinzufügen
3. **Build-Skripte** für Image-Matrix erstellen
4. **CI/CD** anpassen für Multi-Variant Builds
5. **Dokumentation** in README.md verlinken

---

**Wichtig:** Die neue Default-Policy (Vulkan=ON in COMMUNITY+) funktioniert in Docker **mit CPU-Fallback**. Image-Builds ohne Vulkan SDK sind kleiner, nutzen aber keine GPU zur Laufzeit.

**Empfehlung:** Zwei Image-Varianten bereitstellen:
- `themisdb:community` - Mit Vulkan (größer, GPU-fähig)
- `themisdb:community-cpu` - Ohne Vulkan (kleiner, nur CPU)
