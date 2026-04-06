# ThemisDB LLM Plugin System - Quick Start

**Version:** 1.3.0  
**Release:** Dezember 2025

---

## 🚀 Schnellstart

### 1. llama.cpp einrichten

```bash
# Automatisches Setup (lokaler Clone, nicht committen)
bash scripts/setup-llamacpp.sh

# Oder manuell (Root-Verzeichnis)
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
```

### 2. Build mit LLM Support

```bash
# CPU-Only Build
cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build

# Mit CUDA (NVIDIA GPU)
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_CUDA=ON
cmake --build build

# Mit Metal (Apple Silicon)
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_METAL=ON
cmake --build build
```

### 3. Model herunterladen

```bash
mkdir -p models

# Beispiel: Mistral 7B Instruct (Q4 quantized, ~4GB)
# Download from HuggingFace:
# https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF
```

### 4. Konfiguration

```bash
cp config/llm_config.example.yaml config/llm_config.yaml

# Editiere llm_config.yaml:
# - Setze model.path auf dein heruntergeladenes Model
# - Konfiguriere GPU layers (n_layers)
# - Optional: LoRA adapters
```

### 5. ThemisDB mit LLM starten

```bash
./build/themis_server --config config/llm_config.yaml
```

---

## 📚 Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [LLM_PLUGIN_DEVELOPMENT_GUIDE.md](./LLM_PLUGIN_DEVELOPMENT_GUIDE.md) | Vollständiger Entwickler-Guide für Plugin-Entwicklung |
| [LLAMA_CPP_INTEGRATION.md](./LLAMA_CPP_INTEGRATION.md) | llama.cpp Integration Details |
| [AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md) | Distributed Sharding Architektur (Roadmap) |

---

## 🧩 Plugin Architektur

```
┌─────────────────────────────────────────────────────┐
│          ThemisDB LLM Plugin System                 │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ILLMPlugin Interface                               │
│    ↓                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────┐  │
│  │ LlamaCpp     │  │   vLLM       │  │  Custom  │  │
│  │   Plugin     │  │   Plugin     │  │  Plugin  │  │
│  └──────────────┘  └──────────────┘  └──────────┘  │
│         ↓                  ↓                ↓        │
│  ┌──────────────────────────────────────────────┐  │
│  │       LLMPluginManager                       │  │
│  │  - Plugin Discovery & Loading                │  │
│  │  - Model Management                          │  │
│  │  - LoRA Coordination                         │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
└─────────────────────────────────────────────────────┘
```

---

## 💡 Code Beispiele

### Ollama-Style: Lazy Model Loading

```cpp
#include "llm/model_loader.h"

// Lazy loader setup
LazyModelLoader::Config config;
config.max_models = 3;           // Keep up to 3 models in memory
config.max_vram_mb = 24576;      // 24 GB budget
config.model_ttl = std::chrono::seconds(1800);  // 30 min TTL

LazyModelLoader loader(config);

// First request: Loads model lazily (~2-3 seconds)
auto* model = loader.getOrLoadModel(
    "mistral-7b",
    "/models/mistral-7b-instruct-q4.gguf"
);

// Subsequent requests: Instant (cache hit!)
auto* same_model = loader.getOrLoadModel("mistral-7b", "");
// ~0ms!

// Pin important models to prevent eviction
loader.pinModel("mistral-7b");
```

### vLLM-Style: Multi-LoRA Management

```cpp
#include "llm/multi_lora_manager.h"

// Multi-LoRA setup
MultiLoRAManager::Config config;
config.max_lora_slots = 16;      // Up to 16 LoRAs
config.max_lora_vram_mb = 2048;  // 2 GB for LoRAs
config.enable_multi_lora_batch = true;

MultiLoRAManager lora_mgr(config);

// Load multiple LoRAs for same base model
lora_mgr.loadLoRA("legal-qa", "/loras/legal-qa-v1.bin", "mistral-7b");
lora_mgr.loadLoRA("medical-diag", "/loras/medical-v1.bin", "mistral-7b");
lora_mgr.loadLoRA("code-assist", "/loras/code-v1.bin", "mistral-7b");

// Use different LoRAs per request
InferenceRequest req1;
req1.prompt = "Legal question";
req1.lora_adapter_id = "legal-qa";

InferenceRequest req2;
req2.prompt = "Medical question";
req2.lora_adapter_id = "medical-diag";

// Fast LoRA switching (~5ms)
```

### Plugin registrieren

```cpp
#include "llm/llm_plugin_manager.h"
#include "llm/llama_wrapper.h"

// Plugin erstellen und konfigurieren
json config = {
    {"model_path", "/models/mistral-7b-instruct-q4.gguf"},
    {"n_gpu_layers", 32},
    {"n_ctx", 4096}
};

createLlamaWrapper("llamacpp", config["model_path"], config);
```

### Text generieren

```cpp
auto& manager = LLMPluginManager::instance();

InferenceRequest request;
request.prompt = "Was ist ThemisDB?";
request.max_tokens = 512;
request.temperature = 0.7f;

auto response = manager.generate(request);
std::cout << "Response: " << response.text << std::endl;
std::cout << "Tokens: " << response.tokens_generated << std::endl;
std::cout << "Time: " << response.inference_time_ms << "ms" << std::endl;
```

### RAG (Retrieval-Augmented Generation)

```cpp
// Dokumente aus ThemisDB abrufen (Vector Search)
RAGContext context;
context.query = "Rechtliche Aspekte der Datenspeicherung";
context.documents = {
    {.content = "Dokument 1 Inhalt...", .source = "doc1.pdf", .relevance_score = 0.95},
    {.content = "Dokument 2 Inhalt...", .source = "doc2.pdf", .relevance_score = 0.87}
};

InferenceRequest request;
request.prompt = context.query;

auto response = manager.generateRAG(context, request);
```

### LoRA Adapter laden

```cpp
auto* plugin = manager.getPlugin("llamacpp");

// LoRA laden
plugin->loadLoRA(
    "legal-qa-v1",
    "/loras/legal-qa-v1.bin",
    1.0f  // scale
);

// Mit LoRA inferieren
InferenceRequest request;
request.prompt = "Rechtliche Frage...";
request.lora_adapter_id = "legal-qa-v1";

auto response = plugin->generate(request);
```

---

## 🎯 Features

### ✅ Implementiert (v1.3.0)

- ✅ Plugin-basierte Architektur
- ✅ llama.cpp Integration (Reference Implementation)
- ✅ Model Loading (GGUF Format)
- ✅ LoRA Adapter Support
- ✅ GPU Acceleration (CUDA, Metal, Vulkan, HIP)
- ✅ RAG Integration
- ✅ Memory Management & Statistics
- ✅ Multi-Plugin Support
- ✅ **Ollama-style Lazy Loading** (v1.3.0)
- ✅ **vLLM-style Multi-LoRA** (v1.3.0)

### 🚧 Roadmap (Future)

- 🚧 HTTP API Endpoints
- 🚧 Streaming Generation
- 🚧 Batch Inference
- 🚧 Distributed Sharding (etcd + gRPC)
- 🚧 Cross-Shard LoRA Transfer
- 🚧 Federated RAG Queries
- 🚧 vLLM Plugin Implementation
- 🚧 Model Replication (Raft Consensus)

---

## 🔧 Build Optionen

| Option | Beschreibung | Default |
|--------|--------------|---------|
| `THEMIS_ENABLE_LLM` | LLM Plugin Support aktivieren | OFF |
| `THEMIS_ENABLE_CUDA` | CUDA GPU Support | OFF |
| `THEMIS_ENABLE_METAL` | Metal GPU Support (macOS) | OFF |
| `THEMIS_ENABLE_VULKAN` | Vulkan GPU Support | OFF |
| `THEMIS_ENABLE_HIP` | AMD HIP/ROCm Support | OFF |

---

## 📊 Performance

### Typische Latenz (Mistral-7B Q4, RTX 4090)

| Operation | Latenz | Throughput |
|-----------|--------|------------|
| Model Loading | ~2-3 Sekunden | - |
| Text Generation (512 tokens) | ~300ms | ~1700 tokens/s |
| RAG Query (10 docs) | ~320ms | - |
| LoRA Loading | ~50ms | - |
| LoRA Switch | ~5ms | - |
| Embedding (512 tokens) | ~5ms | - |

### VRAM Usage

| Model | Quantization | VRAM |
|-------|--------------|------|
| Phi-3-Mini | Q4_K_M | ~2 GB |
| Mistral-7B | Q4_K_M | ~4 GB |
| Llama-2-13B | Q4_K_M | ~8 GB |
| Llama-3-70B | Q4_K_M | ~40 GB |

---

## 🆘 Troubleshooting

### Model lädt nicht

```bash
# Prüfe Model Format (muss GGUF sein)
file models/mistral-7b.gguf

# Prüfe Permissions
ls -la models/

# Teste mit kleinerem Model
# Phi-3-Mini: https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf
```

### Langsame Inferenz

```bash
# Erhöhe GPU layers in config
n_layers: 32  # → 35 oder mehr

# Prüfe GPU Auslastung
nvidia-smi  # (CUDA)
# Oder
sudo powermetrics --samplers gpu_power  # (Metal/macOS)

# Reduziere Context Size wenn möglich
n_ctx: 4096  # → 2048
```

### Build Fehler

```bash
# llama.cpp lokaler Clone fehlt?
ls -la ./llama.cpp
# Falls nicht vorhanden: lokalen Clone erstellen (nicht committen)
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

# CUDA nicht gefunden?
export CUDA_PATH=/usr/local/cuda
cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_CUDA=ON
```

### Windows/MSVC Build mit LLM

```powershell
# Empfohlen: Skript für MSVC Release-Build mit LLM
powershell -File scripts/build-themis-server-llm.ps1

# Sanity-Check
./build-msvc/bin/themis_server.exe --help
```

Hinweise:
- Das Skript setzt Visual Studio 2022 (`-G "Visual Studio 17 2022"`) und x64 Architektur (`-A x64`).
- vcpkg-Toolchain wird eingebunden; `llama.cpp/` ist lokaler Clone und per `.gitignore`/`.dockerignore` ausgeschlossen.

---

## 🔗 Links

- **llama.cpp:** https://github.com/ggerganov/llama.cpp
- **GGUF Models:** https://huggingface.co/models?library=gguf
- **LoRA Fine-tuning:** https://github.com/tloen/alpaca-lora
- **ThemisDB:** https://github.com/makr-code/ThemisDB

---

## 📝 Lizenz

ThemisDB: MIT License  
llama.cpp: MIT License

---

**Version:** 1.3.0  
**Last Updated:** April 2026  
**Status:** Production Ready (Reference Implementation)
