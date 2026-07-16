# ThemisDB auf SoC - Quick Reference

**Version:** 1.5.0  
**Stand:** 6. April 2026

---

> 📘 **Vollständige Dokumentation:** [ThemisDB mit llama.cpp auf SoC-Geräten](THEMIS_LLAMA_CPP_SOC_GUIDE.md)

---

## 🚀 Schnellstart nach Hardware

### Raspberry Pi 5 (8GB)

```yaml
# config/config-rpi5-llm.yaml
llm:
  enabled: true
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  enable_caching: true
```

**Empfohlene Modelle:** Phi-3-Mini (3.8B), Qwen-2.5 (3B)  
**Performance:** ~2-3 Tokens/Sekunde

### Raspberry Pi 4 (4GB)

```yaml
# config/config-rpi4-llm.yaml
llm:
  enabled: true
  model_path: "/data/models/tinyllama-1.1b.Q4_K_M.gguf"
  context_size: 2048
  threads: 4
  enable_caching: true
```

**Empfohlene Modelle:** TinyLlama (1.1B), Phi-3-Mini (3.8B)  
**Performance:** ~1-2 Tokens/Sekunde

### Orange Pi 5 Plus (16GB)

```yaml
# config/config-opi5-llm.yaml
llm:
  enabled: true
  model_path: "/data/models/mistral-7b-instruct.Q4_K_M.gguf"
  context_size: 8192
  threads: 8
  gpu_layers: 25  # Mali GPU Offloading
  enable_caching: true
```

**Empfohlene Modelle:** Mistral-7B, Llama-3.1 (8B)  
**Performance:** ~3-5 Tokens/Sekunde

### NVIDIA Jetson Nano (4GB)

```yaml
# config/config-jetson-llm.yaml
llm:
  enabled: true
  model_path: "/data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf"
  context_size: 4096
  threads: 4
  gpu_layers: 32  # CUDA Offloading
  enable_caching: true
```

**Empfohlene Modelle:** Phi-3-Mini (3.8B), Mistral-7B  
**Performance:** ~5-10 Tokens/Sekunde (mit GPU)

---

## 📦 Modell-Downloads

```bash
# Phi-3-Mini (3.8B) - Beste Balance
wget https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf \
  -O /data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf

# TinyLlama (1.1B) - Für RPi 4
wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf \
  -O /data/models/tinyllama-1.1b.Q4_K_M.gguf

# Mistral-7B - Für OrangePi 5 / Jetson
wget https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf \
  -O /data/models/mistral-7b-instruct.Q4_K_M.gguf

# Qwen-2.5 (3B) - Multilingual
wget https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf \
  -O /data/models/qwen-2.5-3b.Q4_K_M.gguf
```

---

## ⚙️ System-Optimierung

```bash
# Performance-Mode
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Transparent Huge Pages deaktivieren
echo never | sudo tee /sys/kernel/mm/transparent_hugepage/enabled

# Swap konfigurieren (für 4-8GB RAM)
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
# CONF_SWAPSIZE=4096
sudo dphys-swapfile setup && sudo dphys-swapfile swapon

# SSD statt SD-Karte nutzen
# -> 5-10x schneller!
```

---

## 🔧 ThemisDB Build

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# COMMUNITY Edition mit LLM
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTHEMIS_EDITION=COMMUNITY \
      -DTHEMIS_ENABLE_LLM=ON \
      -DTHEMIS_ENABLE_FLASH_ATTENTION=ON \
      -DTHEMIS_ENABLE_ARM_NEON=ON \
      ..

# Kompilieren (20-60 Minuten je nach Hardware)
make -j$(nproc)

# Installieren
sudo make install
```

---

## 🚀 Server starten

```bash
# ThemisDB Server
./themis_server --config config/config.yaml

# Im Hintergrund (mit systemd)
sudo systemctl start themisdb
```

---

## 🧪 Test

```bash
# Health Check
curl http://localhost:8080/health

# LLM-Anfrage (mit Bearer Token)
# Hinweis: Ersetzen Sie YOUR_JWT_TOKEN mit einem gültigen Token
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -d '{
    "prompt": "Write a hello world program in Python",
    "max_tokens": 100,
    "temperature": 0.7
  }'

# Chat (mit Bearer Token)
curl -X POST http://localhost:8080/api/v1/llm/inference \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_JWT_TOKEN" \
  -d '{
    "prompt": "Du bist ein hilfreicher Assistent. Was ist ThemisDB?",
    "max_tokens": 150
  }'
```

---

## 📊 Performance-Matrix

| Hardware | RAM | Modell | Tokens/Sek | Stromverbrauch |
|----------|-----|--------|------------|----------------|
| **RPi 4** | 4GB | TinyLlama 1.1B | 3-5 | 5-7W |
| **RPi 5** | 8GB | Phi-3-Mini 3.8B | 2-3 | 8-12W |
| **OPi 5** | 16GB | Mistral-7B | 3-5 | 10-15W |
| **Jetson Nano** | 4GB | Phi-3 (GPU) | 5-10 | 10-15W |
| **Rock 5B** | 16GB | Mistral-7B | 4-6 | 12-18W |

---

## 🐛 Troubleshooting

### Problem: Out of Memory

```yaml
# Lösung 1: Kleinere Kontextgröße
llm:
  context_size: 1024  # statt 2048
  model_path: "/data/models/tinyllama-1.1b.Q4_K_M.gguf"

# Lösung 2: Swap aktivieren
# Siehe System-Optimierung oben
```

### Problem: Langsame Inferenz

```bash
# CPU-Governor prüfen
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
# -> sollte "performance" sein

# Temperatur prüfen (Throttling?)
vcgencmd measure_temp
# -> < 80°C

# Lüfter installieren!
```

### Problem: Modell lädt nicht

```bash
# Pfad prüfen
stat /data/models/phi-3-mini-4k-instruct.Q4_K_M.gguf

# Berechtigungen prüfen
ls -lh /data/models/

# Neu herunterladen
cd /data/models
wget <model-url>
```

---

## 🔗 AI-Beschleuniger

### Google Coral TPU (USB)

```bash
# Installation (mit modernem Keyring)
sudo mkdir -p /usr/share/keyrings
curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | \
  sudo gpg --dearmor -o /usr/share/keyrings/coral-edgetpu-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/coral-edgetpu-archive-keyring.gpg] https://packages.cloud.google.com/apt coral-edgetpu-stable main" | \
  sudo tee /etc/apt/sources.list.d/coral-edgetpu.list

sudo apt update && sudo apt install -y libedgetpu1-std

# ThemisDB Config
llm:
  embeddings:
    backend: "coral_tpu"
```

**Performance:** ~100-200 Embeddings/Sekunde

### Hailo-8 Accelerator

```bash
# SDK installieren
wget https://hailo.ai/downloads/hailo-sdk/hailo-sdk-latest-arm64.deb
sudo dpkg -i hailo-sdk-latest-arm64.deb

# ThemisDB Config
llm:
  embeddings:
    backend: "hailo"
```

**Performance:** ~300-500 Embeddings/Sekunde

### Rockchip NPU (integriert in OrangePi 5 / Rock 5B)

```bash
# RKNN Runtime
git clone https://github.com/rockchip-linux/rknn-toolkit2.git
cd rknn-toolkit2/rknpu2
sudo cp runtime/RK3588/Linux/librknn_api/aarch64/librknnrt.so /usr/lib/
sudo ldconfig

# ThemisDB Config
llm:
  embeddings:
    backend: "rknn"
```

**Performance:** ~200-400 Embeddings/Sekunde

---

## 📚 Weiterführende Ressourcen

- **[Vollständige Dokumentation](THEMIS_LLAMA_CPP_SOC_GUIDE.md)** ⭐
- **[Raspberry Pi Tuning Guide](deployment_raspberry_tuning.md)**
- **[ARM Build Guide](deployment_arm_build.md)**
- **[ThemisDB Hauptdokumentation](https://makr-code.github.io/ThemisDB/)**

---

## 💡 Best Practices

1. ✅ **SSD statt SD-Karte** - 5-10x schneller
2. ✅ **Aktive Kühlung** - Verhindert Throttling
3. ✅ **Performance CPU-Governor** - Maximale Leistung
4. ✅ **Prompt-Caching aktivieren** - 30-50% schneller
5. ✅ **Q4_K_M Quantisierung** - Beste Balance
6. ✅ **Flash Attention** - NEON-optimiert für ARM
7. ✅ **Swap konfigurieren** - Für 4-8GB RAM

---

**Autor:** ThemisDB Team  
**Lizenz:** MIT  
**Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
