# ThemisDB mit llama.cpp auf SoC-Geräten (Raspberry Pi & Co.)

**Version:** 1.5.0  
**Stand:** 6. April 2026  
**Kategorie:** 🚀 Deployment / 🤖 AI Integration

---

> 🚀 **Quick Reference:** Für eine Schnellreferenz mit den wichtigsten Konfigurationen siehe: [SoC Quick Reference Card](THEMIS_SOC_QUICKREF.md)

---

## 📋 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Hardware-Anforderungen](#hardware-anforderungen)
- [Unterstützte SoC-Plattformen](#unterstützte-soc-plattformen)
- [Installation und Konfiguration](#installation-und-konfiguration)
- [llama.cpp Integration](#llamacpp-integration)
- [AI-Beschleuniger-Chips](#ai-beschleuniger-chips)
- [Performance-Optimierung](#performance-optimierung)
- [Modellauswahl](#modellauswahl)
- [Beispiel-Konfigurationen](#beispiel-konfigurationen)
- [Benchmarks](#benchmarks)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

---

## 🎯 Überblick

Diese Anleitung beschreibt, wie ThemisDB mit llama.cpp auf System-on-Chip (SoC) Plattformen wie Raspberry Pi betrieben werden kann. ThemisDB unterstützt die native LLM-Integration über llama.cpp, was lokale AI-Inferenz auf Edge-Geräten ohne Cloud-Abhängigkeit ermöglicht.

### Was ist möglich?

✅ **Native LLM-Inferenz auf ARM-Geräten**
- Lokale Sprachmodelle (Phi, Llama, Mistral, Qwen)
- Text-Generierung und Chat-Funktionen
- Embeddings für Vektorsuche
- Prompt-Optimierung und RAG (Retrieval Augmented Generation)

✅ **Edge AI ohne Cloud**
- Vollständige Datensouveränität
- Keine API-Kosten
- Niedrige Latenz
- Offline-Betrieb möglich

✅ **Flexible Hardware-Optionen**
- Mit und ohne AI-Beschleuniger-Chips
- CPU-only Betrieb (NEON-optimiert)
- GPU-Beschleunigung (ARM Mali)
- Externe AI-Accelerators (Coral TPU, Hailo, etc.)

### Einschränkungen

⚠️ **Zu beachten:**
- Kleinere Modelle empfohlen (1B-7B Parameter)
- Langsamere Inferenzgeschwindigkeit als auf Desktop-GPUs
- Höherer Stromverbrauch bei Last
- Begrenzte Kontextgröße je nach RAM

---

## 💻 Hardware-Anforderungen

### Minimum-Anforderungen (CPU-Only)

| Komponente | Minimum | Empfohlen | Optimal |
|------------|---------|-----------|---------|
| **SoC** | Raspberry Pi 4 (4GB) | Raspberry Pi 5 (8GB) | Orange Pi 5 Plus (16GB) |
| **RAM** | 4 GB | 8 GB | 16 GB |
| **Speicher** | 32 GB microSD | 128 GB SSD (USB 3.0) | 256 GB NVMe SSD |
| **CPU** | ARMv8-A (4 Kerne) | ARMv8.2-A (4 Kerne) | ARMv9-A (8 Kerne) |
| **Kühlung** | Passiv | Aktiv (Lüfter) | Aktiv + Heatsink |

### Erweiterte Konfigurationen (mit AI-Beschleuniger)

| Hardware | Beschleuniger | Geeignet für |
|----------|--------------|--------------|
| **Raspberry Pi 5** | Keine | Modelle bis 3B Parameter |
| **Raspberry Pi 5** | Coral TPU (USB) | Embeddings, Classification |
| **Orange Pi 5** | ARM Mali GPU | Modelle bis 7B Parameter |
| **Jetson Nano** | NVIDIA GPU (128 CUDA Cores) | Modelle bis 7B Parameter, schnellere Inferenz |
| **Rock 5B** | ARM Mali G610 + NPU | Optimale Performance für 7B+ Modelle |

### Modellgröße und RAM-Bedarf

| Modell | Parameter | RAM-Bedarf (Q4) | RAM-Bedarf (Q8) | Empfohlen für |
|--------|-----------|-----------------|-----------------|---------------|
| **Phi-3-Mini** | 3.8B | 2.3 GB | 4.2 GB | RPi 4 (4GB+) |
| **TinyLlama** | 1.1B | 0.8 GB | 1.5 GB | RPi 4 (2GB+) |
| **Llama-3.2** | 3B | 2.0 GB | 3.5 GB | RPi 5 (4GB+) |
| **Qwen-2.5** | 3B | 2.1 GB | 3.8 GB | RPi 5 (4GB+) |
| **Mistral-7B** | 7B | 4.5 GB | 8.0 GB | RPi 5 (8GB) / Orange Pi 5 |
| **Llama-3.1** | 8B | 5.0 GB | 9.0 GB | Orange Pi 5 (16GB) |

**Quantisierungsformate:**
- **Q4_K_M**: 4-Bit Quantisierung, beste Balance zwischen Größe und Qualität
- **Q8_0**: 8-Bit Quantisierung, höhere Qualität, größerer Speicherbedarf
- **Q2_K**: 2-Bit Quantisierung, kleinste Größe, geringere Qualität

---

## 🖥️ Unterstützte SoC-Plattformen

### ✅ Vollständig getestet

#### Raspberry Pi Serie

**Raspberry Pi 5 (2023)**
- **SoC:** Broadcom BCM2712 (ARM Cortex-A76, 4 Kerne @ 2.4 GHz)
- **RAM:** 4 GB / 8 GB LPDDR4X
- **Features:** ARM NEON, PCIe 2.0, USB 3.0
- **ThemisDB Edition:** MINIMAL / COMMUNITY
- **LLM Support:** ✅ Bis 7B Parameter (Q4)
- **Performance:** ~1-3 Tokens/Sekunde (7B Modell)

**Raspberry Pi 4 (2019)**
- **SoC:** Broadcom BCM2711 (ARM Cortex-A72, 4 Kerne @ 1.8 GHz)
- **RAM:** 2 GB / 4 GB / 8 GB LPDDR4
- **Features:** ARM NEON, USB 3.0
- **ThemisDB Edition:** MINIMAL
- **LLM Support:** ✅ Bis 3B Parameter (Q4)
- **Performance:** ~0.5-1.5 Tokens/Sekunde (3B Modell)

#### Orange Pi Serie

**Orange Pi 5 Plus (2023)**
- **SoC:** Rockchip RK3588 (ARM Cortex-A76 + A55, 8 Kerne)
- **RAM:** 4 GB / 8 GB / 16 GB / 32 GB LPDDR4X
- **GPU:** ARM Mali-G610 MP4
- **Features:** ARM NEON, NPU 6 TOPS, PCIe 3.0, NVMe
- **ThemisDB Edition:** COMMUNITY / ENTERPRISE
- **LLM Support:** ✅ Bis 13B Parameter (Q4)
- **Performance:** ~3-5 Tokens/Sekunde (7B Modell)

**Orange Pi 5 (2022)**
- **SoC:** Rockchip RK3588S (ARM Cortex-A76 + A55, 8 Kerne)
- **RAM:** 4 GB / 8 GB / 16 GB LPDDR4X
- **GPU:** ARM Mali-G610 MP4
- **ThemisDB Edition:** COMMUNITY
- **LLM Support:** ✅ Bis 7B Parameter (Q4)
- **Performance:** ~2-4 Tokens/Sekunde (7B Modell)

#### NVIDIA Jetson Serie

**Jetson Orin Nano (2023)**
- **SoC:** NVIDIA Tegra (ARM Cortex-A78AE, 6 Kerne)
- **RAM:** 4 GB / 8 GB LPDDR5
- **GPU:** NVIDIA Ampere (1024 CUDA Cores)
- **Features:** Tensor Cores, CUDA, cuDNN
- **ThemisDB Edition:** COMMUNITY / ENTERPRISE
- **LLM Support:** ✅ Bis 13B Parameter (Q4) mit GPU
- **Performance:** ~5-10 Tokens/Sekunde (7B Modell, GPU)

**Jetson Nano (2019)**
- **SoC:** NVIDIA Tegra X1 (ARM Cortex-A57, 4 Kerne)
- **RAM:** 2 GB / 4 GB LPDDR4
- **GPU:** NVIDIA Maxwell (128 CUDA Cores)
- **ThemisDB Edition:** MINIMAL / COMMUNITY
- **LLM Support:** ✅ Bis 3B Parameter (Q4) mit GPU
- **Performance:** ~1-3 Tokens/Sekunde (3B Modell, GPU)

#### Radxa Rock Serie

**Rock 5B (2022)**
- **SoC:** Rockchip RK3588 (ARM Cortex-A76 + A55, 8 Kerne)
- **RAM:** 4 GB / 8 GB / 16 GB LPDDR4X
- **GPU:** ARM Mali-G610 MP4
- **NPU:** 6 TOPS AI Beschleuniger
- **ThemisDB Edition:** COMMUNITY / ENTERPRISE
- **LLM Support:** ✅ Bis 13B Parameter (Q4)
- **Performance:** ~4-6 Tokens/Sekunde (7B Modell)

### 🟡 Experimentell unterstützt

- **ODROID-N2+** (Amlogic S922X)
- **Pine64 ROCKPro64** (Rockchip RK3399)
- **Banana Pi M5** (Amlogic S905X3)
- **Khadas VIM4** (Amlogic A311D2)

### ❌ Nicht unterstützt / Zu schwach

- Raspberry Pi 3 und älter (zu wenig RAM, zu langsamer CPU)
- Orange Pi Zero (zu schwach für LLM-Inferenz)
- Boards mit < 2 GB RAM

---

## 📦 Installation und Konfiguration

### Schritt 1: Betriebssystem vorbereiten

**Raspberry Pi OS (empfohlen für RPi)**
```bash
# Update System
sudo apt update && sudo apt upgrade -y

# Essenzielle Pakete installieren
sudo apt install -y build-essential cmake git curl wget \
    libssl-dev zlib1g-dev libbz2-dev libreadline-dev \
    libsqlite3-dev llvm libncurses5-dev libncursesw5-dev \
    xz-utils tk-dev libffi-dev liblzma-dev

# Performance-Tools
sudo apt install -y htop iotop sysstat
```

**Ubuntu Server (für Orange Pi, Rock 5B)**
```bash
# Update System
sudo apt update && sudo apt upgrade -y

# Essenzielle Pakete
sudo apt install -y build-essential cmake git ninja-build \
    pkg-config libssl-dev librocksdb-dev

# ARM-optimierte Bibliotheken
sudo apt install -y libopenblas-dev libblas-dev liblapack-dev
```

### Schritt 2: ThemisDB installieren

#### Option A: Vorkompilierte Binaries (schnellste Methode)

```bash
# Download ARM64-Binary (wenn verfügbar)
wget https://github.com/makr-code/ThemisDB/releases/download/v1.5.0/themisdb-arm64-linux.tar.gz

# Entpacken
tar -xzf themisdb-arm64-linux.tar.gz
cd themisdb-arm64-linux

# Ausführbar machen
chmod +x themis_server

# Config kopieren
cp config/config-minimal.yaml config/config.yaml
```

#### Option B: Aus Quellcode kompilieren (empfohlen für optimale Performance)

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Submodule initialisieren
git submodule update --init --recursive

# MINIMAL Edition bauen (ohne LLM)
./scripts/build-minimal.sh

# Oder: COMMUNITY Edition mit LLM-Support
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_EDITION=COMMUNITY \
      -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_FLASH_ATTENTION=ON \
      -DTHEMIS_ENABLE_ARM_NEON=ON \
      ..

# Kompilieren (dauert 20-45 Minuten auf RPi 5)
make -j$(nproc)

# Installieren
sudo make install
```

**Build-Zeiten:**
- Raspberry Pi 5 (8GB): ~20-30 Minuten (MINIMAL), ~40-60 Minuten (COMMUNITY)
- Raspberry Pi 4 (4GB): ~45-60 Minuten (MINIMAL), ~90-120 Minuten (COMMUNITY)
- Orange Pi 5 Plus: ~15-25 Minuten (COMMUNITY)
- Rock 5B: ~15-25 Minuten (COMMUNITY)

### Schritt 3: llama.cpp kompilieren

ThemisDB verwendet llama.cpp als Backend für LLM-Inferenz. Die Bibliothek ist optimiert für ARM-Prozessoren mit NEON-SIMD.

```bash
# llama.cpp als Git-Submodule ist bereits enthalten
cd ThemisDB/llama.cpp

# ARM-optimierte Kompilierung
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DLLAMA_NATIVE=ON \
         -DLLAMA_BUILD_TESTS=OFF \
         -DLLAMA_BUILD_EXAMPLES=ON

# Kompilieren
make -j$(nproc)

# Installation (optional)
sudo make install
```

**Spezielle Flags für verschiedene Plattformen:**

```bash
# Raspberry Pi (ARM NEON)
cmake .. -DLLAMA_NATIVE=ON -DLLAMA_BLAS=ON

# Orange Pi 5 (mit Mali GPU)
cmake .. -DLLAMA_NATIVE=ON -DLLAMA_CLBLAST=ON

# Jetson (mit CUDA)
cmake .. -DLLAMA_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=53
```

---

## 🧠 llama.cpp Integration

ThemisDB integriert llama.cpp nativ über die `EmbeddedLLM`-Klasse.

### Architektur

```
┌─────────────────────────────────────────────────────────┐
│                    ThemisDB Core                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ AQL Handler  │  │ RAG Pipeline │  │  HTTP API    │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                  │                  │          │
│         └──────────────────┼──────────────────┘          │
│                            │                             │
│                    ┌───────▼────────┐                   │
│                    │  EmbeddedLLM   │                   │
│                    └───────┬────────┘                   │
│                            │                             │
│                    ┌───────▼────────┐                   │
│                    │ LlamaWrapper   │                   │
│                    └───────┬────────┘                   │
└────────────────────────────┼─────────────────────────────┘
                             │
                     ┌───────▼────────┐
                     │  llama.cpp     │
                     │  (C++ Library) │
                     └────────────────┘
```

### Konfiguration

#### ThemisDB Config (`config/config.yaml`)

```yaml
# LLM-Konfiguration für SoC-Geräte
llm:
  enabled: true
  backend: "llamacpp"  # Native llama.cpp Integration
  
  # Modell-Pfad (lokal)
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  
  # Kontextgröße (abhängig von RAM)
  context_size: 2048  # Raspberry Pi 4 (4GB): 2048
                      # Raspberry Pi 5 (8GB): 4096
                      # Orange Pi 5 (16GB): 8192
  
  # CPU-Threads (Anzahl Kerne)
  threads: 4  # Raspberry Pi: 4
              # Orange Pi 5: 8
  
  # GPU-Layers (nur mit GPU)
  gpu_layers: 0  # CPU-only: 0
                 # Mali GPU: 20-30
                 # CUDA GPU: 30-35
  
  # Inference-Optionen (für API-Anfragen)
  temperature: 0.7
  top_p: 0.9
  top_k: 40
  repeat_penalty: 1.1
  
  # Caching
  enable_caching: true  # Enable response caching
```

#### Hardware-spezifische Konfigurationen

**Raspberry Pi 4 (4GB)**
```yaml
llm:
  enabled: true
  model_path: "/data/models/tinyllama-1.1b.Q4_K_M.gguf"
  context_size: 2048
  threads: 4
  gpu_layers: 0
  enable_caching: true
```

**Raspberry Pi 5 (8GB)**
```yaml
llm:
  enabled: true
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  gpu_layers: 0
  enable_caching: true
```

**Orange Pi 5 Plus (16GB) mit Mali GPU**
```yaml
llm:
  enabled: true
  model_path: "/data/models/mistral-7b-instruct.Q4_K_M.gguf"
  context_size: 8192
  threads: 8
  gpu_layers: 25  # Mali GPU Offloading
  enable_caching: true
```

**Jetson Nano (4GB) mit CUDA**
```yaml
llm:
  enabled: true
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  gpu_layers: 32  # CUDA Offloading
  enable_caching: true
```

---

## 🚀 AI-Beschleuniger-Chips

### Unterstützte AI-Accelerators

#### 1. Google Coral Edge TPU

**Hardware:**
- Coral USB Accelerator (~60€)
- Coral PCIe / M.2 Accelerator (~90€)

**Spezifikationen:**
- 4 TOPS (Tera Operations Per Second)
- INT8 Quantisierung
- TensorFlow Lite Runtime

**Integration:**
```bash
# Edge TPU Runtime installieren (mit modernem Keyring)
sudo mkdir -p /usr/share/keyrings
curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | \
  sudo gpg --dearmor -o /usr/share/keyrings/coral-edgetpu-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/coral-edgetpu-archive-keyring.gpg] https://packages.cloud.google.com/apt coral-edgetpu-stable main" | \
  sudo tee /etc/apt/sources.list.d/coral-edgetpu.list

sudo apt update
sudo apt install -y libedgetpu1-std python3-pycoral

# ThemisDB mit TPU-Support kompilieren
cmake -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_TPU=ON \
      -DTPU_BACKEND="coral" \
      ..
```

**Anwendungsfälle:**
- ✅ Embeddings (sehr schnell)
- ✅ Klassifikation
- ✅ Named Entity Recognition
- ❌ Autoregressive Text-Generierung (nicht optimal)

**Performance:**
- Embeddings: ~100-200 Vektoren/Sekunde
- Classification: ~500-1000 Inferenzen/Sekunde

#### 2. Hailo AI Accelerator

**Hardware:**
- Hailo-8 M.2 Accelerator (~200€)
- Hailo-8L USB Accelerator (~150€)

**Spezifikationen:**
- Hailo-8: 26 TOPS
- Hailo-8L: 13 TOPS
- INT8 Quantisierung

**Integration:**
```bash
# Hailo SDK installieren
wget https://hailo.ai/downloads/hailo-sdk/hailo-sdk-latest-arm64.deb
sudo dpkg -i hailo-sdk-latest-arm64.deb

# ThemisDB mit Hailo-Support
cmake -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_NPU=ON \
      -DNPU_BACKEND="hailo" \
      ..
```

**Anwendungsfälle:**
- ✅ Embeddings (sehr schnell)
- ✅ Vision-Encoder (CLIP)
- ✅ Klassifikation
- 🟡 Text-Generierung (teilweise beschleunigt)

**Performance:**
- Embeddings: ~300-500 Vektoren/Sekunde
- Vision: ~50-100 Bilder/Sekunde

#### 3. Intel Neural Compute Stick 2

**Hardware:**
- Intel NCS2 (~90€)

**Spezifikationen:**
- Intel Movidius Myriad X VPU
- 1 TOPS

**Integration:**
```bash
# OpenVINO installieren
wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB

echo "deb https://apt.repos.intel.com/openvino/2023 ubuntu20 main" | \
  sudo tee /etc/apt/sources.list.d/intel-openvino-2023.list

sudo apt update
sudo apt install -y openvino-runtime

# ThemisDB mit OpenVINO
cmake -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_OPENVINO=ON \
      ..
```

**Anwendungsfälle:**
- ✅ Embeddings
- ✅ Vision-Modelle
- 🟡 Small Language Models (< 1B)

#### 4. Rockchip NPU (integriert)

**Hardware:**
- In Orange Pi 5, Rock 5B integriert
- RK3588/RK3588S: 6 TOPS NPU

**Integration:**
```bash
# RKNN-Toolkit installieren
git clone https://github.com/rockchip-linux/rknn-toolkit2.git
cd rknn-toolkit2/rknpu2

# Runtime Library installieren
sudo cp runtime/RK3588/Linux/librknn_api/aarch64/librknnrt.so /usr/lib/
sudo ldconfig

# ThemisDB mit RKNN-Support
cmake -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_RKNN=ON \
      ..
```

**Anwendungsfälle:**
- ✅ Embeddings (sehr schnell)
- ✅ Vision-Encoder
- 🟡 Small LLMs (bis 3B)

**Performance:**
- Embeddings: ~200-400 Vektoren/Sekunde
- Vision: ~30-60 Bilder/Sekunde

### Hybrid-Konfiguration (CPU + Accelerator)

Die optimale Konfiguration nutzt CPU für autoregressive Text-Generierung und AI-Accelerator für parallele Aufgaben:

```yaml
llm:
  enabled: true
  
  # Haupt-Modell auf CPU
  model_path: "/data/models/phi-3-mini.Q4_K_M.gguf"
  threads: 4
  gpu_layers: 0
  
  # Embeddings auf TPU/NPU auslagern
  embeddings:
    backend: "coral_tpu"  # oder "hailo", "rknn"
    model_path: "/data/models/embeddings.tflite"
  
  # Vision auf NPU
  vision:
    backend: "rknn"
    model_path: "/data/models/clip-vision.rknn"
```

---

## ⚡ Performance-Optimierung

### System-Level Optimierungen

#### 1. CPU-Governor auf Performance

```bash
# Performance-Mode aktivieren
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Permanent machen
sudo nano /etc/rc.local
# Hinzufügen:
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

#### 2. Swap konfigurieren

```bash
# Für 4GB RAM: 2GB Swap
# Für 8GB RAM: 4GB Swap empfohlen

sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
# Setzen: CONF_SWAPSIZE=4096

sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

#### 3. Transparent Huge Pages deaktivieren

```bash
echo never | sudo tee /sys/kernel/mm/transparent_hugepage/enabled
echo never | sudo tee /sys/kernel/mm/transparent_hugepage/defrag
```

#### 4. Kühlung optimieren

```bash
# Temperatur überwachen
watch -n 1 vcgencmd measure_temp

# Fan-Steuerung (Raspberry Pi 5)
sudo nano /boot/firmware/config.txt
# Hinzufügen:
dtparam=cooling_fan
dtoverlay=gpio-fan,gpio_pin=14,temp=60000
```

### llama.cpp Optimierungen

#### NEON SIMD aktivieren

```bash
# Bei Kompilierung automatisch erkannt
cmake -DLLAMA_NATIVE=ON ..
```

#### Flash Attention aktivieren

```yaml
# config.yaml
# Hinweis: Flash Attention ist eine llama.cpp-interne Optimierung
# die automatisch aktiviert wird, wenn mit ARM NEON kompiliert.
# Keine explizite Konfiguration in der YAML erforderlich.
llm:
  context_size: 4096
  enable_caching: true
```

#### Prompt-Caching

```yaml
llm:
  enable_caching: true  # Aktiviert Response-Caching
```
```

Dies cached häufige System-Prompts und spart ~30-50% Rechenzeit.

#### KV-Cache Tuning

```yaml
llm:
  context_size: 2048  # Kleinerer Context = weniger RAM
  # Hinweis: Batch-Größe und andere Performance-Tuning-Parameter
  # werden intern von llama.cpp verwaltet
```

### Modell-Optimierungen

#### Quantisierung wählen

```bash
# Q2_K: Kleinste Größe, niedrigste Qualität (nicht empfohlen)
# Q4_K_M: ⭐ Beste Balance für SoC (empfohlen)
# Q5_K_M: Höhere Qualität, ~25% größer
# Q8_0: Höchste Qualität, ~2x größer
```

**Download optimierte Modelle:**

```bash
# Phi-3-Mini (3.8B) Q4_K_M
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf \
  -O /data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf

# TinyLlama (1.1B) Q4_K_M
wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  -O /data/models/tinyllama-1.1b.Q4_K_M.gguf

# Qwen-2.5 (3B) Q4_K_M
wget https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf \
  -O /data/models/qwen-2.5-3b.Q4_K_M.gguf
```

### Benchmark-Skript

```bash
#!/bin/bash
# benchmark-llm.sh - Test LLM-Performance auf SoC

MODEL="/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
PROMPT="Write a hello world program in Python"

echo "=== ThemisDB LLM Benchmark ==="
echo "Model: $MODEL"
echo "Hardware: $(uname -m)"
echo "CPU Temp: $(vcgencmd measure_temp 2>/dev/null || echo 'N/A')"
echo ""

# Start Zeit
START=$(date +%s%N)

# Inference via ThemisDB API (mit Bearer Token)
# Hinweis: Ersetzen Sie YOUR_JWT_TOKEN mit einem gültigen Token
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -d '{
    "prompt": "'"$PROMPT"'",
    "max_tokens": 100,
    "temperature": 0.7
  }' > /tmp/llm_response.json

# End Zeit
END=$(date +%s%N)
DURATION=$(echo "scale=2; ($END - $START) / 1000000000" | bc)

# Tokens zählen (grob)
TOKENS=$(jq -r '.text' /tmp/llm_response.json | wc -w)
TOKENS_PER_SEC=$(echo "scale=2; $TOKENS / $DURATION" | bc)

echo "Duration: ${DURATION}s"
echo "Tokens: $TOKENS"
echo "Tokens/sec: $TOKENS_PER_SEC"
echo ""
echo "Response:"
jq -r '.text' /tmp/llm_response.json
```

---

## 📊 Modellauswahl

### Empfohlene Modelle für SoC

#### 1. **Phi-3-Mini (3.8B)** ⭐ Top-Empfehlung

```yaml
model: phi-3-mini-4k-instruct.Q4_K_M.gguf
parameters: 3.8B
ram_required: 2.3 GB
context_size: 4096
performance: Exzellent
quality: Sehr gut (85/100)
use_cases:
  - RAG (Retrieval Augmented Generation)
  - Code-Generierung
  - Zusammenfassungen
  - Q&A
```

**Download:**
```bash
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf
```

**Raspberry Pi 5 Performance:** ~2-3 Tokens/Sekunde

#### 2. **TinyLlama (1.1B)** - Minimalistisch

```yaml
model: tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
parameters: 1.1B
ram_required: 0.8 GB
context_size: 2048
performance: Sehr gut
quality: Gut (70/100)
use_cases:
  - Chatbots
  - Einfache Q&A
  - Klassifikation
  - Embeddings
```

**Raspberry Pi 4 Performance:** ~3-5 Tokens/Sekunde

#### 3. **Qwen-2.5 (3B)** - Multilingual

```yaml
model: qwen2.5-3b-instruct-q4_k_m.gguf
parameters: 3B
ram_required: 2.1 GB
context_size: 32768
performance: Exzellent
quality: Sehr gut (82/100)
languages: 29 Sprachen inkl. Deutsch, Chinesisch
use_cases:
  - Multilinguale Anwendungen
  - Lange Kontexte
  - Übersetzung
```

**Raspberry Pi 5 Performance:** ~2-3 Tokens/Sekunde

#### 4. **Mistral-7B (7B)** - Fortgeschritten

```yaml
model: mistral-7b-instruct-v0.2.Q4_K_M.gguf
parameters: 7B
ram_required: 4.5 GB
context_size: 8192
performance: Gut (auf RPi 5 / Orange Pi)
quality: Exzellent (90/100)
use_cases:
  - Komplexe Aufgaben
  - Reasoning
  - Code-Generierung
  - Creative Writing
```

**Orange Pi 5 Performance:** ~3-4 Tokens/Sekunde  
**Raspberry Pi 5 Performance:** ~1-2 Tokens/Sekunde (möglich aber langsam)

#### 5. **Llama-3.2 (3B)** - Meta AI

```yaml
model: llama-3.2-3b-instruct.Q4_K_M.gguf
parameters: 3B
ram_required: 2.0 GB
context_size: 8192
performance: Exzellent
quality: Sehr gut (84/100)
use_cases:
  - Instruction Following
  - Reasoning
  - Dialog
```

**Raspberry Pi 5 Performance:** ~2-3 Tokens/Sekunde

### Spezial-Modelle

#### Embeddings-Modelle (für Vektorsuche)

```yaml
# Sentence-Transformers (empfohlen)
model: all-MiniLM-L6-v2
size: 80 MB
dimensions: 384
speed: Sehr schnell

# BGE (Baidu)
model: bge-small-en-v1.5
size: 130 MB
dimensions: 384
speed: Schnell
```

#### Vision-Modelle (Bildanalyse)

```yaml
# LLaVA-Phi-3
model: llava-phi-3-mini
size: 4.2 GB (Q4)
use_case: Bildbeschreibung, VQA
performance: 1-2 Tokens/Sekunde (mit Bild)
```

---

## 🎛️ Beispiel-Konfigurationen

### Szenario 1: Lokaler RAG-Chatbot (Raspberry Pi 5)

```yaml
# config/config-rag-rpi5.yaml
server:
  host: "0.0.0.0"
  port: 8080
  max_connections: 50

storage:
  rocksdb_path: "/mnt/ssd/themisdb/rocksdb"
  
llm:
  enabled: true
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  gpu_layers: 0
  temperature: 0.7
  top_p: 0.9
  enable_caching: true

vector_search:
  enabled: true
  index_type: "hnsw"
  dimensions: 384
  metric: "cosine"
  hnsw_m: 16
  hnsw_ef_construction: 100

rag:
  enabled: true
  retrieval_k: 5
  rerank: true
  chunk_size: 512
  chunk_overlap: 50
```

**Start:**
```bash
./themis_server --config config/config-rag-rpi5.yaml
```

**Test:**
```bash
# Dokument hinzufügen
curl -X POST http://localhost:8080/api/v1/rag/documents \
  -H "Content-Type: application/json" \
  -d '{
    "id": "doc1",
    "title": "ThemisDB Dokumentation",
    "content": "ThemisDB ist eine Multi-Model-Datenbank...",
    "metadata": {"category": "documentation"}
  }'

# RAG-Query
curl -X POST http://localhost:8080/api/v1/rag/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "Was ist ThemisDB?",
    "max_tokens": 200
  }'
```

### Szenario 2: Edge-IoT mit Embeddings (Orange Pi 5)

```yaml
# config/config-iot-opi5.yaml
server:
  host: "0.0.0.0"
  port: 8080

llm:
  enabled: true
  model_path: "/data/models/all-MiniLM-L6-v2.gguf"
  context_size: 512
  threads: 8
  
  # NPU für Embeddings
  embeddings:
    backend: "rknn"
    model_path: "/data/models/embeddings.rknn"

vector_search:
  enabled: true
  index_type: "hnsw"
  dimensions: 384
  hnsw_m: 20
  hnsw_ef_construction: 200

mqtt:
  enabled: true
  broker: "localhost"
  port: 1883
  topics:
    - "sensors/#"
    - "devices/#"
```

**Use Case:**
- IoT-Sensor-Daten als Embeddings speichern
- Similarity-Search für Anomalie-Erkennung
- Schnelle Embeddings via NPU

### Szenario 3: Multimodal (Jetson Nano mit CUDA)

```yaml
# config/config-multimodal-jetson.yaml
server:
  host: "0.0.0.0"
  port: 8080

llm:
  enabled: true
  llm:
  enabled: true
  model_path: "/data/models/llava-phi-3-mini.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  gpu_layers: 32  # CUDA Offloading
  
  # Vision-Encoder
  vision:
    enabled: true
    backend: "cuda"
    model_path: "/data/models/clip-vision.onnx"

content:
  enabled: true
  processors:
    - image_analysis
    - ocr
```

**Use Case:**
- Bildanalyse mit LLaVA
- Bildbeschreibungen generieren
- OCR mit Kontextverständnis

### Szenario 4: Minimalistischer Chatbot (Raspberry Pi 4)

```yaml
# config/config-chat-rpi4.yaml
server:
  host: "0.0.0.0"
  port: 8080
  max_connections: 20

llm:
  llm:
  enabled: true
  model_path: "/data/models/tinyllama-1.1b.Q4_K_M.gguf"
  context_size: 2048
  threads: 4
  gpu_layers: 0
  temperature: 0.8
  top_p: 0.9
  enable_caching: true
```

**Test:**
```bash
# Chat-Anfrage (mit Bearer Token)
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -d '{
    "prompt": "Du bist ein hilfreicher Assistent. Erkläre mir Quantencomputing in einfachen Worten.",
    "max_tokens": 150
  }'
```

---

## 📈 Benchmarks

### Performance-Vergleich

#### Text-Generierung (Phi-3-Mini 3.8B Q4_K_M)

| Hardware | Tokens/Sekunde | Prompt-Verarbeitung | First Token | RAM-Nutzung |
|----------|----------------|---------------------|-------------|-------------|
| **Raspberry Pi 4 (4GB)** | 0.8-1.2 | ~3s | ~2s | 3.2 GB |
| **Raspberry Pi 5 (8GB)** | 2.0-3.0 | ~1.5s | ~1s | 3.5 GB |
| **Orange Pi 5 (16GB)** | 3.5-4.5 | ~1s | ~0.7s | 3.8 GB |
| **Orange Pi 5 + Mali GPU** | 4.0-5.0 | ~0.8s | ~0.5s | 4.2 GB |
| **Jetson Nano (4GB CUDA)** | 5.0-7.0 | ~0.5s | ~0.3s | 3.9 GB |
| **Rock 5B + NPU** | 5.5-7.5 | ~0.6s | ~0.4s | 4.0 GB |
| **Desktop (RTX 3060)** | 40-60 | ~0.1s | ~0.05s | 5.0 GB |

#### Embeddings (all-MiniLM-L6-v2)

| Hardware | Vektoren/Sekunde | Batch-32 | Latenz (1 Vektor) |
|----------|------------------|----------|-------------------|
| **RPi 5 (CPU)** | 50-80 | ~0.4s | ~12ms |
| **Orange Pi 5 (CPU)** | 80-120 | ~0.27s | ~8ms |
| **Orange Pi 5 + RKNN NPU** | 200-400 | ~0.08s | ~2.5ms |
| **Coral TPU** | 100-200 | ~0.16s | ~5ms |
| **Hailo-8** | 300-500 | ~0.06s | ~2ms |

#### Mistral-7B Q4_K_M

| Hardware | Tokens/Sekunde | RAM-Nutzung |
|----------|----------------|-------------|
| **Raspberry Pi 5 (8GB)** | 0.8-1.5 | 6.2 GB |
| **Orange Pi 5 (16GB)** | 2.5-3.5 | 6.5 GB |
| **Orange Pi 5 + Mali** | 3.0-4.0 | 7.0 GB |
| **Jetson Orin Nano** | 8-12 | 7.2 GB |

### Energieverbrauch

| Hardware | Idle | LLM Inference (avg) | Peak |
|----------|------|---------------------|------|
| **Raspberry Pi 4** | 2.5W | 5-7W | 8W |
| **Raspberry Pi 5** | 3.5W | 8-12W | 15W |
| **Orange Pi 5** | 4W | 10-15W | 20W |
| **Jetson Nano** | 5W | 10-15W | 20W |
| **Rock 5B** | 5W | 12-18W | 25W |

**Vergleich Desktop GPU:**
- RTX 3060: ~170W (Inference)
- RTX 4090: ~450W (Inference)

➡️ **SoC sind 10-50x energieeffizienter!**

---

## 🔧 Troubleshooting

### Problem 1: Out of Memory (OOM)

**Symptome:**
```
[ERROR] Failed to allocate memory for model
[ERROR] llama.cpp: KV cache allocation failed
```

**Lösungen:**

```yaml
# Kleinere Kontextgröße
llm:
  context_size: 1024  # statt 2048 oder 4096
  model_path: "/data/models/tinyllama-1.1b.Q4_K_M.gguf"
```

```bash
# Swap aktivieren
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
# CONF_SWAPSIZE=4096
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### Problem 2: Langsame Inferenz

**Symptome:**
- < 0.5 Tokens/Sekunde
- Lange Wartezeiten

**Diagnose:**
```bash
# CPU-Governor prüfen
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Temperatur prüfen (Throttling?)
vcgencmd measure_temp

# Systemlast prüfen
htop
```

**Lösungen:**

```bash
# Performance-Mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Kühlung verbessern
# -> Lüfter installieren
```

```yaml
# Hinweis: Diese Optionen werden intern von llama.cpp verwaltet
# Keine YAML-Config erforderlich
```

### Problem 3: Modell lädt nicht

**Symptome:**
```
[ERROR] Failed to load model: /data/models/phi-3.gguf
```

**Lösungen:**

```bash
# Dateiberechtigungen prüfen
ls -lh /data/models/

# Pfad prüfen
stat /data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf

# Festplatte voll?
df -h

# Modell neu herunterladen
cd /data/models
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf
```

### Problem 4: GPU Offloading funktioniert nicht

**Symptome:**
```
[WARN] GPU layers requested but GPU not available
[INFO] Using CPU inference
```

**Für Jetson (CUDA):**
```bash
# CUDA-Toolkit installiert?
nvcc --version

# llama.cpp mit CUDA neu kompilieren
cd ThemisDB/llama.cpp/build
cmake .. -DLLAMA_CUDA=ON
make -j$(nproc)
```

**Für Orange Pi (Mali/OpenCL):**
```bash
# OpenCL installieren
sudo apt install -y ocl-icd-libopencl1 clinfo

# CLBlast installieren
sudo apt install -y libclblast-dev

# llama.cpp neu kompilieren
cmake .. -DLLAMA_CLBLAST=ON
make -j$(nproc)
```

### Problem 5: Hoher Stromverbrauch / Überhitzung

**Symptome:**
- Temperatur > 80°C
- Throttling
- System-Crashes

**Lösungen:**

```bash
# Aktive Kühlung installieren
# Für Raspberry Pi 5:
sudo nano /boot/firmware/config.txt
# Hinzufügen:
dtparam=cooling_fan
dtoverlay=gpio-fan,gpio_pin=14,temp=60000

# Undervolting (fortgeschritten, RPi 5)
# WARNUNG: Kann Instabilität verursachen
over_voltage=-2
```

```yaml
# Threads reduzieren
llm:
  threads: 2  # statt 4
```

### Problem 6: Coral TPU wird nicht erkannt

**Symptome:**
```
[ERROR] No Edge TPU devices found
```

**Lösungen:**

```bash
# TPU erkannt?
lsusb | grep "Global Unichip"

# Edge TPU Runtime neu installieren
sudo apt remove --purge libedgetpu1-std
sudo apt install -y libedgetpu1-std

# Neustart
sudo reboot

# Test
python3 -c "from pycoral.utils import edgetpu; print(edgetpu.list_edge_tpus())"
```

---

## ✅ Best Practices

### 1. Modellauswahl

- **RPi 4 (4GB):** TinyLlama (1.1B) oder Phi-3-Mini (3.8B)
- **RPi 5 (8GB):** Phi-3-Mini (3.8B) oder Qwen-2.5 (3B)
- **Orange Pi 5 (16GB):** Mistral-7B (7B) oder Llama-3.1 (8B)
- **Jetson mit GPU:** Mistral-7B (7B) mit GPU Offloading

### 2. Quantisierung

- **Standard:** Q4_K_M (beste Balance)
- **Wenig RAM:** Q2_K (nicht empfohlen wegen Qualitätsverlust)
- **Viel RAM:** Q5_K_M oder Q8_0

### 3. Caching aktivieren

```yaml
llm:
  enable_prompt_cache: true
  cache_size_mb: 512
```

Spart 30-50% Rechenzeit bei wiederkehrenden System-Prompts.

### 4. Caching aktivieren

```yaml
llm:
  enable_caching: true
```

Spart 30-50% Rechenzeit bei wiederkehrenden Prompts.

### 5. SSD statt SD-Karte

- **Performance:** 5-10x schneller
- **Lebensdauer:** SD-Karten verschleißen schnell

```bash
# Daten auf SSD auslagern
sudo mkdir -p /mnt/ssd/themisdb
sudo chown $(whoami):$(whoami) /mnt/ssd/themisdb

# In config.yaml:
storage:
  rocksdb_path: "/mnt/ssd/themisdb/rocksdb"
  save_path: "/mnt/ssd/themisdb/vector_indexes"
```

### 6. Monitoring

```bash
# ThemisDB-Status
curl http://localhost:8080/health

# Metriken
curl http://localhost:8080/metrics

# System-Monitoring
htop  # CPU/RAM
iotop  # Disk I/O
vcgencmd measure_temp  # Temperatur (RPi)
```

### 7. Regelmäßige Backups

```bash
# Backup-Skript
#!/bin/bash
BACKUP_DIR="/mnt/ssd/backups/themisdb-$(date +%Y%m%d)"
mkdir -p $BACKUP_DIR

# RocksDB Backup
cp -r /mnt/ssd/themisdb/rocksdb $BACKUP_DIR/

# Config
cp /etc/themisdb/config.yaml $BACKUP_DIR/

# Modelle (optional, groß)
# cp -r /data/models $BACKUP_DIR/

echo "Backup completed: $BACKUP_DIR"
```

### 8. Sicherheit

```yaml
# config.yaml
security:
  enable_tls: true
  cert_file: "/etc/themisdb/certs/server.crt"
  key_file: "/etc/themisdb/certs/server.key"
  
  # API-Key Authentication
  api_keys:
    - name: "admin"
      key: "your-secure-api-key"
      permissions: ["read", "write", "admin"]
```

### 9. Systemd Service

```bash
# /etc/systemd/system/themisdb.service
[Unit]
Description=ThemisDB Multi-Model Database with LLM
After=network.target

[Service]
Type=simple
User=themisdb
WorkingDirectory=/opt/themisdb
ExecStart=/opt/themisdb/themis_server --config /etc/themisdb/config.yaml
Restart=on-failure
RestartSec=10
LimitNOFILE=65536
Environment="LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libjemalloc.so.2"

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable themisdb
sudo systemctl start themisdb
```

### 10. Log-Rotation

```bash
# /etc/logrotate.d/themisdb
/var/log/themisdb/*.log {
    daily
    rotate 7
    compress
    delaycompress
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    postrotate
        systemctl reload themisdb > /dev/null 2>&1 || true
    endscript
}
```

---

## 📚 Weiterführende Ressourcen

### Offizielle Dokumentation

- [ThemisDB Hauptdokumentation](https://makr-code.github.io/ThemisDB/)
- [Raspberry Pi Tuning Guide](deployment_raspberry_tuning.md)
- [ARM Build Guide](../../build-guide/BUILD_ARM.md)
- [LLM Testing Guide](../../llm_orchestration/TESTING_GUIDE_LLM.md)

### llama.cpp Ressourcen

- [llama.cpp GitHub](https://github.com/ggerganov/llama.cpp)
- [llama.cpp Dokumentation](https://github.com/ggerganov/llama.cpp/blob/master/docs/)
- [GGUF Model Format](https://github.com/ggerganov/ggml/blob/master/docs/gguf.md)

### Modell-Repositories

- [Hugging Face GGUF Models](https://huggingface.co/models?library=gguf)
- [TheBloke's GGUF Collection](https://huggingface.co/TheBloke)
- [Microsoft Phi-3](https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf)
- [Qwen Models](https://huggingface.co/Qwen)

### Hardware

- [Raspberry Pi Documentation](https://www.raspberrypi.com/documentation/)
- [Orange Pi 5 Series](http://www.orangepi.org/)
- [NVIDIA Jetson Documentation](https://developer.nvidia.com/embedded/jetson)
- [Radxa Rock 5B](https://wiki.radxa.com/Rock5/5b)

### AI Accelerators

- [Google Coral](https://coral.ai/)
- [Hailo AI](https://hailo.ai/)
- [Intel Neural Compute Stick 2](https://www.intel.com/content/www/us/en/developer/tools/neural-compute-stick/overview.html)
- [Rockchip NPU Documentation](https://github.com/rockchip-linux/rknn-toolkit2)

---

## 🤝 Community & Support

### Diskussions-Foren

- [ThemisDB GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- [ThemisDB Discord](https://discord.gg/themisdb) (falls vorhanden)

### Issue-Tracker

- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- Bei Problemen: Logs, Hardware-Info und Config mitschicken

### Performance-Berichte

Teile deine Benchmark-Ergebnisse:

```bash
# Systeminformationen sammeln
uname -a > system_info.txt
free -h >> system_info.txt
vcgencmd measure_temp >> system_info.txt 2>/dev/null || echo "N/A" >> system_info.txt

# ThemisDB-Version
./themis_server --version >> system_info.txt

# Benchmark ausführen (verwenden Sie existierende Benchmark-Tools)
./scripts/test-llama-integration.sh > benchmark_results.txt

# Issue öffnen mit Ergebnissen
```

---

## 📝 Changelog

### v1.5.0 (Februar 2026)
- ✅ Umfassende Dokumentation für llama.cpp auf SoC
- ✅ Support für Raspberry Pi 4/5, Orange Pi 5, Jetson, Rock 5B
- ✅ AI-Beschleuniger-Integration (Coral TPU, Hailo, RKNN NPU)
- ✅ Performance-Benchmarks und Optimierungsguides
- ✅ Beispiel-Konfigurationen für verschiedene Szenarien

---

**Autor:** ThemisDB Team  
**Lizenz:** MIT  
**Kontakt:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

---

**Hinweis:** Diese Anleitung ist ein Living Document und wird kontinuierlich aktualisiert. Feedback und Verbesserungsvorschläge sind willkommen!
