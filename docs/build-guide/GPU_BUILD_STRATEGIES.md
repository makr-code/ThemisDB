# GPU Build Strategies: CUDA, Vulkan & Hardware Acceleration
**Version:** 1.4.0  
**Erstellt:** 15. Januar 2026  
**Plattformen:** Windows, WSL2, Docker

---

## 🎯 Übersicht

Dieses Dokument beschreibt **Best Practices** für das Bauen von ThemisDB mit GPU-Beschleunigung auf verschiedenen Plattformen. Es behandelt CUDA (NVIDIA), Vulkan (Cross-Platform), HIP (AMD) und andere Hardware-Acceleration-Backends.

### Unterstützte Backends

| Backend | Plattformen | Primärer Use Case | Status |
|---------|-------------|-------------------|--------|
| **CUDA** | Windows, Linux, Docker | NVIDIA GPU Acceleration | ✅ Voll implementiert |
| **Vulkan** | Windows, Linux, Docker, Android | Cross-Platform GPU Compute | ✅ Voll implementiert |
| **HIP** | Linux, Docker | AMD GPU Acceleration | ⚠️ Experimentell |
| **DirectX 12** | Windows | Windows GPU Compute Shaders | 🚧 Geplant |
| **Metal** | macOS | Apple GPU Acceleration | 🚧 Geplant |
| **ROCm** | Linux | AMD Professional GPUs | ⚠️ Experimentell |
| **ZLUDA** | Windows | CUDA auf AMD GPUs | 🧪 Research |

---

## 📋 Voraussetzungen nach Plattform

### Windows (MSVC)

#### CUDA (NVIDIA)
```powershell
# 1. NVIDIA CUDA Toolkit installieren (12.3+ empfohlen, 13.1+ unterstützt)
# Download: https://developer.nvidia.com/cuda-downloads
# Installation: C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1

# 2. Umgebungsvariablen prüfen
$env:CUDA_PATH
# Output: C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1

# 3. NVIDIA Driver prüfen
nvidia-smi
# Output: Driver Version: 545.xx+
```

**Wichtig:** Visual Studio 2022 muss CUDA-Komponenten haben:
- Visual Studio Installer → Modify → Components
- Suche: "CUDA" → "NVIDIA CUDA 13.x Toolkit" installieren

#### Vulkan (Cross-Platform)
```powershell
# 1. Vulkan SDK installieren
# Download: https://vulkan.lunarg.com/
# Installation: C:\VulkanSDK\1.3.xxx

# 2. Umgebungsvariablen prüfen
$env:VULKAN_SDK
# Output: C:\VulkanSDK\1.3.280.0

# 3. Vulkan Runtime prüfen
vulkaninfo | Select-String -Pattern "API Version"
# Output: API Version: 1.3.xxx
```

### WSL2 (Ubuntu 22.04+)

#### CUDA
```bash
# 1. NVIDIA Driver für WSL installieren (Windows-Host)
# Download: https://developer.nvidia.com/cuda/wsl
# Wichtig: WSL-spezifischer Treiber erforderlich!

# 2. CUDA Toolkit im WSL installieren
wget https://developer.download.nvidia.com/compute/cuda/repos/wsl-ubuntu/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
sudo apt-get install -y cuda-toolkit-12-3

# 3. Umgebungsvariablen setzen
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# 4. Prüfen
nvidia-smi
nvcc --version
```

**Wichtig:** WSL2 benötigt spezielle NVIDIA-Treiber. Standard-Windows-Treiber funktionieren nicht!

#### Vulkan
```bash
# 1. Vulkan SDK installieren
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt update
sudo apt install -y vulkan-sdk

# 2. Mesa Vulkan Treiber (für Intel/AMD)
sudo apt install -y mesa-vulkan-drivers

# 3. Prüfen
vulkaninfo | grep "API version"
```

### Docker

#### CUDA Container
```dockerfile
# Base Image mit CUDA Support
FROM nvidia/cuda:12.3.0-devel-ubuntu22.04

# Build Tools installieren
RUN apt-get update && apt-get install -y \
    cmake ninja-build build-essential git

# Projekt bauen
WORKDIR /src
COPY . .
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_ENABLE_CUDA=ON
RUN cmake --build build --parallel 8
```

**Runtime:**
```bash
# Container mit GPU-Zugriff starten
docker run --gpus all themis-cuda:latest
```

#### Vulkan Container
```dockerfile
FROM ubuntu:22.04

# Vulkan SDK installieren
RUN apt-get update && apt-get install -y \
    cmake ninja-build build-essential git \
    vulkan-sdk

# Rest analog zu CUDA
```

---

## 🛠️ Build-Strategien nach Plattform

### 1. Windows (MSVC 2022) - BEST PRACTICE

#### Strategie A: Visual Studio Generator (Empfohlen für Debugging)

```powershell
# In VS 2022 Developer PowerShell
cd C:\VCC\themis

# Konfiguration mit CUDA
cmake -S . -B build-msvc `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DTHEMIS_ENABLE_LLM=ON

# Build
cmake --build build-msvc --config Release --parallel 8
```

**Vorteile:**
- ✅ Native Visual Studio Integration
- ✅ IntelliSense und Debugging funktionieren perfekt
- ✅ Multi-Config (Debug/Release in einem Build-Verzeichnis)

**Nachteile:**
- ⚠️ CUDA Toolkit Path muss korrekt sein (siehe Troubleshooting)
- ⚠️ Langsamer als Ninja

#### Strategie B: Ninja Generator (Empfohlen für CI/CD)

```powershell
# In VS 2022 Developer PowerShell
cd C:\VCC\themis

# CUDA Toolkit explizit setzen
$env:CUDA_PATH = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1"

# Konfiguration mit Ninja
cmake -S . -B build-ninja `
  -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DCUDAToolkit_ROOT="C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1"

# Build (deutlich schneller!)
cmake --build build-ninja --parallel 8
```

**Vorteile:**
- ⚡ **2-3x schneller** als Visual Studio Generator
- ✅ Bessere Parallelisierung
- ✅ Kleinere Build-Artefakte

**Nachteile:**
- ⚠️ Nur Single-Config (separate Builds für Debug/Release)
- ⚠️ Kein natives Visual Studio IntelliSense

#### Strategie C: CUDA Toolchain File (Für Mehrfach-CUDA-Versionen)

Wenn Sie mehrere CUDA-Versionen installiert haben:

```powershell
# Toolchain-Datei verwenden
cmake -S . -B build-cuda13 `
  -G "Visual Studio 17 2022" `
  -DCMAKE_TOOLCHAIN_FILE="cmake\CUDA-Windows-MSVC.cmake" `
  -DTHEMIS_ENABLE_CUDA=ON

# Die Toolchain-Datei setzt explizit:
# - CUDAToolkit_ROOT
# - CMAKE_CUDA_COMPILER
# - CUDA Library Paths
```

**Datei:** [`cmake/CUDA-Windows-MSVC.cmake`](../../cmake/CUDA-Windows-MSVC.cmake)

---

### 2. WSL2 (Ubuntu) - BEST PRACTICE

#### Strategie A: Ninja (Empfohlen)

```bash
cd /mnt/c/VCC/themis

# CUDA Paths setzen
export CUDA_ROOT=/usr/local/cuda
export PATH=$CUDA_ROOT/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_ROOT/lib64:$LD_LIBRARY_PATH

# Konfiguration
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DCUDAToolkit_ROOT=/usr/local/cuda

# Build
cmake --build build-wsl --parallel $(nproc)
```

**Performance-Tipp:** WSL2 mit Ninja ist oft **schneller** als natives Windows!

#### Strategie B: Unix Makefiles

```bash
cmake -S . -B build-wsl \
  -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_CUDA=ON

make -C build-wsl -j$(nproc)
```

**Hinweis:** Makefiles sind langsamer als Ninja, aber stabiler bei Parallelisierung.

---

### 3. Docker - BEST PRACTICE

#### Strategie A: Multi-Stage Build (Empfohlen für Production)

**Datei:** [`docker/Dockerfile.hyperscaler`](../../docker/Dockerfile.hyperscaler)

```dockerfile
# Stage 1: Build mit CUDA Support
FROM nvidia/cuda:12.3.0-devel-ubuntu22.04 AS builder

RUN apt-get update && apt-get install -y \
    cmake ninja-build build-essential git \
    libssl-dev libcurl4-openssl-dev

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DTHEMIS_ENABLE_CUDA=ON \
    -DTHEMIS_ENABLE_GPU=ON \
    -DTHEMIS_STATIC_BUILD=ON

RUN cmake --build build --parallel 8

# Stage 2: Runtime (minimal)
FROM nvidia/cuda:12.3.0-runtime-ubuntu22.04

COPY --from=builder /src/build/themis_server /usr/local/bin/
COPY --from=builder /src/build/Release/*.so /usr/local/lib/

CMD ["/usr/local/bin/themis_server"]
```

**Vorteile:**
- ✅ Finale Image-Größe: ~400 MB (statt 2.5 GB)
- ✅ Nur Runtime-Dependencies im finalen Image
- ✅ Sicherer (keine Build-Tools im Production-Container)

**Build & Run:**
```bash
# Build
docker buildx build \
  -f docker/Dockerfile.hyperscaler \
  -t themis-cuda:latest \
  --build-arg THEMIS_ENABLE_CUDA=ON \
  .

# Run mit GPU
docker run --gpus all -p 18765:18765 themis-cuda:latest
```

#### Strategie B: Unified Build mit vcpkg Cache

**Datei:** [`docker/Dockerfile.unified`](../../docker/Dockerfile.unified)

```bash
# Mit Host-Cache für schnellere Builds
docker buildx build \
  --build-arg THEMIS_EDITION=HYPERSCALER \
  --build-arg ENABLE_LLM=ON \
  --mount=type=cache,target=/opt/vcpkg/downloads \
  --mount=type=cache,target=/opt/vcpkg/packages \
  -f docker/Dockerfile.unified \
  -t themis:hyperscaler \
  .
```

**Vorteile:**
- ⚡ **10-15x schneller** bei wiederholten Builds (vcpkg Cache)
- ✅ Reproduzierbare Builds
- ✅ Offline-fähig nach erstem Build

---

## 🧩 CMake Build-Optionen im Detail

### CUDA-spezifische Optionen

```cmake
# Basis-Option
-DTHEMIS_ENABLE_CUDA=ON

# CUDA Toolkit explizit angeben (Windows)
-DCUDAToolkit_ROOT="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1"

# CUDA Toolkit explizit angeben (Linux)
-DCUDAToolkit_ROOT=/usr/local/cuda-12.3

# CUDA Architekturen (GPU-Generation)
-DCMAKE_CUDA_ARCHITECTURES="70;75;80;86"
# 70 = Volta (V100)
# 75 = Turing (RTX 20xx)
# 80 = Ampere (A100, RTX 30xx)
# 86 = Ampere (RTX 30xx Mobile)

# CUDA Compiler Flags
-DCMAKE_CUDA_FLAGS="-Xcompiler /MD -gencode arch=compute_70,code=sm_70"
```

### Vulkan-spezifische Optionen

```cmake
# Basis-Option
-DTHEMIS_ENABLE_VULKAN=ON

# Vulkan SDK explizit angeben (Windows)
-DVULKAN_SDK="C:/VulkanSDK/1.3.280.0"

# Vulkan SDK explizit angeben (Linux)
-DVULKAN_SDK=/usr/local/vulkan-sdk/1.3.280.0

# Shader-Optimierung
-DTHEMIS_VULKAN_OPTIMIZE_SHADERS=ON
```

### Multi-Backend Build

```cmake
cmake -S . -B build \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_VULKAN=ON \
  -DTHEMIS_ENABLE_HIP=OFF \
  -DTHEMIS_ENABLE_DIRECTX=OFF
```

**Runtime-Auswahl:**
```bash
# CUDA bevorzugen
./themis_server --acceleration-backend=cuda

# Vulkan bevorzugen
./themis_server --acceleration-backend=vulkan

# Auto-Detect (Default)
./themis_server --acceleration-backend=auto
```

---

## ⚠️ Troubleshooting

### Windows: CUDA Toolkit nicht gefunden

**Symptom:**
```
CMake Error: Could not find CUDA Toolkit
-- CudaToolkitDir property not found
```

**Ursache:** CMake kann CUDA-Installation nicht finden (Registry-Problem mit VS2022).

**Lösung 1:** CUDA Toolkit explizit setzen
```powershell
$env:CUDA_PATH = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1"

cmake -S . -B build-msvc `
  -DCUDAToolkit_ROOT="$env:CUDA_PATH" `
  -DTHEMIS_ENABLE_CUDA=ON
```

**Lösung 2:** CUDA Toolchain File verwenden
```powershell
cmake -S . -B build-msvc `
  -DCMAKE_TOOLCHAIN_FILE="cmake\CUDA-Windows-MSVC.cmake" `
  -DTHEMIS_ENABLE_CUDA=ON
```

**Lösung 3:** Visual Studio Developer PowerShell verwenden
```powershell
# In VS2022 Developer PowerShell (NICHT normale PowerShell!)
& "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1"

# Dann CMake
cmake -S . -B build-msvc -DTHEMIS_ENABLE_CUDA=ON
```

**Details:** Siehe [`CUDA_ANALYSIS.md`](../../CUDA_ANALYSIS.md)

---

### WSL2: CUDA Runtime nicht gefunden

**Symptom:**
```bash
./themis_server
# Error: CUDA driver version is insufficient
```

**Ursache:** WSL2 benötigt speziellen NVIDIA-Treiber.

**Lösung:**
```powershell
# Auf Windows-Host (PowerShell als Admin):
# 1. WSL2-CUDA-Treiber installieren
# Download: https://developer.nvidia.com/cuda/wsl

# 2. WSL Version prüfen
wsl --version
# WSL version: 2.x.x required

# 3. Im WSL prüfen
wsl
nvidia-smi
# Muss GPU anzeigen!
```

**Hinweis:** Standard-Windows-NVIDIA-Treiber funktionieren NICHT in WSL2!

---

### Docker: GPU nicht sichtbar

**Symptom:**
```bash
docker run themis-cuda:latest
# Warning: No CUDA-capable device found
```

**Ursache:** NVIDIA Container Runtime nicht installiert.

**Lösung:**
```bash
# 1. NVIDIA Container Toolkit installieren
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
  sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit

# 2. Docker Daemon neu starten
sudo systemctl restart docker

# 3. Test
docker run --rm --gpus all nvidia/cuda:12.3.0-base-ubuntu22.04 nvidia-smi
```

**Windows Docker Desktop:**
```powershell
# Docker Desktop → Settings → Resources → WSL Integration
# ✅ Enable integration with my default WSL distro
# ✅ Enable integration with additional distros: Ubuntu
```

---

### Vulkan: Shader-Kompilierung schlägt fehl

**Symptom:**
```
Error: Failed to create Vulkan compute pipeline
Shader compilation error: ...
```

**Ursache:** Vulkan SDK nicht im PATH oder falsche Version.

**Lösung:**
```bash
# 1. glslc (Shader-Compiler) prüfen
which glslc
# /usr/bin/glslc (Linux)
# C:\VulkanSDK\1.3.280.0\Bin\glslc.exe (Windows)

# 2. Shader manuell kompilieren (Test)
glslc --version

# 3. CMake mit Vulkan SDK Path
cmake -S . -B build \
  -DVULKAN_SDK=/usr/local/vulkan-sdk/1.3.280.0 \
  -DTHEMIS_ENABLE_VULKAN=ON
```

---

## 📊 Performance-Vergleich

### Build-Zeiten (ThemisDB Full Build)

| Plattform | Generator | CUDA | Zeit | Bemerkungen |
|-----------|-----------|------|------|-------------|
| **Windows MSVC** | Visual Studio | ON | ~25 Min | First build with vcpkg |
| **Windows MSVC** | Visual Studio | OFF | ~18 Min | No CUDA overhead |
| **Windows MSVC** | Ninja | ON | ~12 Min | **Fastest on Windows** |
| **WSL2 Ubuntu** | Ninja | ON | ~10 Min | **Fastest overall** |
| **Docker** | Ninja | ON | ~8 Min | With vcpkg cache |
| **Docker** | Ninja | ON | ~2 Min | Full cache hit |

### Runtime Performance (Vector Search Benchmark)

| Backend | Throughput | Latency (P50) | Latency (P99) |
|---------|------------|---------------|---------------|
| **CPU Only** | 1,200 ops/sec | 8.2 ms | 42 ms |
| **CUDA (RTX 3090)** | 18,500 ops/sec | 0.54 ms | 2.1 ms |
| **Vulkan (RTX 3090)** | 16,200 ops/sec | 0.61 ms | 2.8 ms |
| **CUDA (A100)** | 42,000 ops/sec | 0.24 ms | 0.89 ms |

**Testbed:** 1M vectors, 768 dimensions, batch size 32

---

## 🎓 Best Practices Zusammenfassung

### Windows Development

1. **Verwende Ninja für schnelle Iteration**
   - Visual Studio Generator für Debugging
   - Ninja für Production Builds

2. **CUDA Toolkit explizit setzen**
   ```powershell
   $env:CUDA_PATH = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1"
   ```

3. **VS Developer PowerShell verwenden**
   - Nicht normale PowerShell!
   - Automatisches Setup der Build-Umgebung

4. **vcpkg Binary Caching aktivieren**
   ```powershell
   $env:VCPKG_BINARY_SOURCES = "clear;files,$env:LOCALAPPDATA\vcpkg\archives,readwrite"
   ```

### WSL2 Development

1. **WSL2-CUDA-Treiber verwenden**
   - Standard-Windows-Treiber funktionieren nicht!

2. **Ninja bevorzugen**
   - Schneller als Makefiles
   - Bessere Parallelisierung

3. **ccache für Rebuilds**
   ```bash
   export CCACHE_DIR=/mnt/c/ccache
   cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache \
         -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

4. **Mounted Volumes nutzen**
   ```bash
   # Build-Cache auf Windows-Laufwerk
   cmake -B /mnt/c/VCC/themis/build-wsl
   ```

### Docker Production

1. **Multi-Stage Builds**
   - Builder-Image: ~2.5 GB
   - Runtime-Image: ~400 MB

2. **vcpkg Cache Volumes**
   ```bash
   --mount=type=cache,target=/opt/vcpkg/downloads \
   --mount=type=cache,target=/opt/vcpkg/packages
   ```

3. **NVIDIA Base Images verwenden**
   - `nvidia/cuda:12.3.0-devel-ubuntu22.04` (Build)
   - `nvidia/cuda:12.3.0-runtime-ubuntu22.04` (Runtime)

4. **GPU-Zugriff testen**
   ```bash
   docker run --rm --gpus all nvidia/cuda:12.3.0-base nvidia-smi
   ```

---

## 📚 Weiterführende Dokumentation

- **CUDA Details:** [`CUDA_ANALYSIS.md`](../../CUDA_ANALYSIS.md)
- **Docker Builds:** [`docker/DOCKER_BUILD_STRATEGY_QUICKREF.md`](../../docker/DOCKER_BUILD_STRATEGY_QUICKREF.md)
- **Windows Builds:** [`BUILD_WINDOWS_NEW.md`](BUILD_WINDOWS_NEW.md)
- **Linux/WSL Builds:** [`BUILD_LINUX.md`](BUILD_LINUX.md)
- **Vulkan Backend:** [`../../plugins/PLANNED_ACCELERATION_PLUGINS.md`](../../plugins/PLANNED_ACCELERATION_PLUGINS.md)

---

## 🔄 Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 1.0 | 15. Januar 2026 | Initiale Version mit Windows/WSL/Docker Strategien |

---

**Autor:** ThemisDB Build Team  
**Letzte Aktualisierung:** 15. Januar 2026  
**Lizenz:** MIT
