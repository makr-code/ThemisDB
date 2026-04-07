# llama.cpp Integration für ThemisDB v1.3.0

**Status:** Erforderlich für LLM Plugin Implementation  
**Datum:** Dezember 2025

---

## Übersicht

llama.cpp ist die primäre LLM-Engine für ThemisDB v1.3.0. Diese Dokumentation beschreibt die Integration.

## Build-Optionen

### Option 1: vcpkg Port (Empfohlen für Windows/Cross-Platform)

**Vorteile:**
- Automatisches Dependency Management
- Konsistente Builds
- Einfache Integration in CMake

**Status:** llama.cpp ist NICHT standardmäßig in vcpkg verfügbar, aber wir können einen Custom Port erstellen.

### Option 2: Lokaler Clone (Empfohlen für Entwicklung)

**Vorteile:**
- Direkte Kontrolle über llama.cpp Version
- Einfache Updates
- Zugriff auf neueste Features
- Keine Commits im Repo (per .gitignore/.dockerignore ausgeschlossen)

**Implementierung:** Siehe unten

### Option 3: System-weite Installation

**Vorteile:**
- Shared Library für mehrere Projekte
- Weniger Build-Zeit

**Nachteil:**
- Versionskonflikte möglich

---

## Implementierung: Lokaler Clone (nicht committen)

### 1. llama.cpp lokal klonen

```bash
cd /path/to/ThemisDB

# Lokaler Clone (Root-Verzeichnis):
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

# Optional: auf spezifischen Tag wechseln
cd llama.cpp
git checkout b1696  # Beispiel: Stabiler Release Tag
cd ..

# Hinweis: ./llama.cpp ist per .gitignore/.dockerignore ausgeschlossen
# (wird nicht committed oder in Docker builds kopiert)
```

### 2. CMakeLists.txt Integration

```cmake
# In ThemisDB/CMakeLists.txt

# LLM Support Option
option(THEMIS_ENABLE_LLM "Enable LLM plugin support (llama.cpp)" OFF)

if(THEMIS_ENABLE_LLM)
    message(STATUS "LLM support enabled (llama.cpp)")
    
    # llama.cpp Build Options
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    
    # GPU Support (optional)
    if(THEMIS_ENABLE_CUDA)
        set(LLAMA_CUDA ON CACHE BOOL "" FORCE)
        set(LLAMA_CUDA_F16 ON CACHE BOOL "" FORCE)
    endif()
    
    if(THEMIS_ENABLE_METAL)
        set(LLAMA_METAL ON CACHE BOOL "" FORCE)
    endif()
    
    if(THEMIS_ENABLE_VULKAN)
        set(LLAMA_VULKAN ON CACHE BOOL "" FORCE)
    endif()
    
    # Add llama.cpp subdirectory (Root-Verzeichnis)
    add_subdirectory(llama.cpp)
    
    # Define LLM enabled
    add_compile_definitions(THEMIS_LLM_ENABLED)
endif()
```

### 3. Link llama.cpp zu LLM Plugin

```cmake
# LLM Plugin Source Files
if(THEMIS_ENABLE_LLM)
    set(LLM_PLUGIN_SOURCES
        src/llm/llama_wrapper.cpp
        src/llm/llm_plugin_manager.cpp
    )
    
    target_sources(themis_core PRIVATE ${LLM_PLUGIN_SOURCES})
    
    # Link llama.cpp
    target_link_libraries(themis_core PRIVATE
        llama  # llama.cpp library
    )
    
    target_include_directories(themis_core PRIVATE
        ${CMAKE_SOURCE_DIR}/llama.cpp/include
    )
endif()
```

---

## Verfügbare llama.cpp Features

### CPU-Only Build
```bash
cmake -B build -DTHEMIS_ENABLE_LLM=ON
```

### CUDA Build (NVIDIA GPU)
```bash
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_CUDA=ON \
    -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc
```

### Metal Build (Apple Silicon)
```bash
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_METAL=ON
```

### Vulkan Build (Cross-Platform GPU)
```bash
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_ENABLE_VULKAN=ON
```

---

## llama.cpp API Usage

### Model Loading

```cpp
#include "llama.h"

// Initialize llama backend
llama_backend_init();

// Model parameters
llama_model_params model_params = llama_model_default_params();
model_params.n_gpu_layers = 32;  // GPU offload
model_params.use_mmap = true;

// Load model
llama_model* model = llama_load_model_from_file(
    "/models/mistral-7b-q4.gguf",
    model_params
);

// Context parameters
llama_context_params ctx_params = llama_context_default_params();
ctx_params.n_ctx = 4096;
ctx_params.n_batch = 512;

// Create context
llama_context* ctx = llama_new_context_with_model(model, ctx_params);
```

### Inference

```cpp
// Tokenize prompt
std::vector<llama_token> tokens;
tokens.resize(4096);
int n_tokens = llama_tokenize(
    model,
    prompt.c_str(),
    prompt.size(),
    tokens.data(),
    tokens.size(),
    true,  // add_bos
    false  // special
);
tokens.resize(n_tokens);

// Generate tokens
for (int i = 0; i < max_tokens; ++i) {
    // Evaluate
    llama_eval(ctx, tokens.data(), tokens.size(), 0);
    
    // Sample next token
    llama_token next_token = llama_sample_token(ctx, nullptr);
    
    // Add to sequence
    tokens.push_back(next_token);
    
    // Check for EOS
    if (next_token == llama_token_eos(model)) {
        break;
    }
}

// Decode tokens to text
std::string result = llama_token_to_str(ctx, tokens.data(), tokens.size());
```

### LoRA Adapters

```cpp
// Load LoRA adapter
llama_lora_adapter* adapter = llama_lora_adapter_load(
    "/loras/legal-qa-v1.bin"
);

// Apply to context
llama_lora_adapter_set(ctx, adapter, 1.0f);  // scale = 1.0

// Use for inference (same as above)

// Remove adapter
llama_lora_adapter_remove(ctx, adapter);

// Free adapter
llama_lora_adapter_free(adapter);
```

### Cleanup

```cpp
// Free resources
llama_free(ctx);
llama_free_model(model);
llama_backend_free();
```

---

## Dependency Tree

```
ThemisDB v1.3.0
    ├─ themis_core (static/shared lib)
    │   ├─ LLM Plugin Support (optional, THEMIS_ENABLE_LLM=ON)
    │   │   └─ llama.cpp
    │   │       ├─ ggml (included in llama.cpp)
    │   │       └─ Optional GPU backends:
    │   │           ├─ CUDA (NVIDIA)
    │   │           ├─ Metal (Apple)
    │   │           ├─ Vulkan (Cross-platform)
    │   │           └─ hipBLAS (AMD ROCm)
    │   └─ ... (other dependencies)
    └─ themis_server (executable)
```

---

## Build Size Impact

| Configuration | Binary Size | VRAM Usage | Build Time |
|---------------|-------------|------------|------------|
| CPU Only | +5 MB | 0 MB | +2 min |
| + CUDA | +15 MB | ~100 MB overhead | +5 min |
| + Metal | +8 MB | ~80 MB overhead | +3 min |
| + Vulkan | +12 MB | ~90 MB overhead | +4 min |

---

## Testing

### Basic Functionality Test

```cpp
#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"

TEST(LlamaIntegration, LoadModel) {
    llama_backend_init();
    
    LlamaWrapper plugin;
    bool loaded = plugin.loadModel("/models/test-model.gguf");
    
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(plugin.isModelLoaded());
    
    llama_backend_free();
}
```

### Inference Test

```cpp
TEST(LlamaIntegration, BasicInference) {
    llama_backend_init();
    
    LlamaWrapper plugin;
    plugin.loadModel("/models/test-model.gguf");
    
    InferenceRequest request;
    request.prompt = "Hello, world!";
    request.max_tokens = 10;
    
    auto response = plugin.generate(request);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    
    llama_backend_free();
}
```

---

## Troubleshooting

### Build Failures

**Problem:** `llama.h not found`
```bash
# Stelle sicher, dass der lokale Clone existiert (Projekt-Root)
ls -la ./llama.cpp

# Falls nicht vorhanden: lokalen Clone erstellen (nicht committen)
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp
```

**Problem:** CUDA not found
```bash
# Setze CUDA_PATH
export CUDA_PATH=/usr/local/cuda
cmake -B build -DTHEMIS_ENABLE_CUDA=ON
```

### Runtime Issues

**Problem:** Model lädt nicht
- Prüfe GGUF Format (llama.cpp v2+)
- Prüfe Dateigröße und Permissions
- Prüfe VRAM Verfügbarkeit

**Problem:** Langsame Inferenz
- Erhöhe `n_gpu_layers` für mehr GPU Offload
- Prüfe ob GPU wirklich genutzt wird (`nvidia-smi`)
- Reduziere `n_ctx` wenn möglich

---

## Windows/MSVC Build (empfohlen)

Für Windows (Visual Studio 2022, x64) steht ein robuster Build-Skript zur Verfügung, das die LLM-Integration aktiviert und die korrekten Generator-/Architektur-Flags setzt.

```powershell
# LLM-Build und Link mit MSVC (Release)
powershell -File scripts/build-themis-server-llm.ps1

# Optional: Hilfe ausgeben
./build-msvc/bin/themis_server.exe --help
```

Hinweise:
- Das Skript setzt `-G "Visual Studio 17 2022" -A x64` und integriert vcpkg (`CMAKE_TOOLCHAIN_FILE`).
- Zur Vermeidung von MSVC-spezifischen `char8_t`-Fehlern wird dem `llama`-Target der Compiler-Schalter `/Zc:char8_t-` hinzugefügt.
- `llama.cpp/` liegt als lokaler Clone im Projekt-Root und ist per `.gitignore` und `.dockerignore` ausgeschlossen.

## Referenzen

- **llama.cpp Repository:** https://github.com/ggerganov/llama.cpp
- **GGUF Format:** https://github.com/ggerganov/ggml/blob/master/docs/gguf.md
- **Model Download:** https://huggingface.co/models?library=gguf
- **ThemisDB LLM Docs:** [LLM_PLUGIN_DEVELOPMENT_GUIDE.md](./LLM_PLUGIN_DEVELOPMENT_GUIDE.md)

---

**Status:** Ready for Implementation  
**Version:** ThemisDB v1.3.0  
**Last Updated:** April 2026
