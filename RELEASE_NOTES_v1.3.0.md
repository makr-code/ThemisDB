# ThemisDB v1.3.0 - Native LLM Integration (Optional Feature)

**Release Date:** 20. Dezember 2025  
**Code Name:** "Keep Your Own Llamas"

---

## 🎉 Overview

ThemisDB v1.3.0 introduces **optional native LLM integration** with embedded llama.cpp, enabling you to run AI/LLM workloads directly in your database without external API dependencies. This release includes a complete plugin architecture, GPU acceleration support, and enterprise-grade caching for production LLM deployments.

**"ThemisDB keeps its own llamas."** – Run LLaMA, Mistral, Phi-3 models (1B-70B params) directly in your database.

> **Important**: LLM integration is an **optional feature** that requires:
> - Build flag: `-DTHEMIS_ENABLE_LLM=ON`
> - External dependency: llama.cpp (must be cloned separately)
> - See installation instructions below

---

## 🚀 Major Features

### 🧠 Embedded LLM Engine (llama.cpp) - Optional
- **Native Integration**: llama.cpp support when built with `-DTHEMIS_ENABLE_LLM=ON`
- **Model Support**: GGUF format models (LLaMA 3, Mistral, Phi-3, etc.)
- **Inference Engine**: Full tokenization, evaluation, sampling, and detokenization pipeline
- **Memory Management**: Lazy model loading with configurable VRAM budgets

### ⚡ GPU Acceleration (When LLM Enabled)
- **CUDA Support**: NVIDIA GPU acceleration with significant speedup vs CPU
- **Metal Support**: Apple Silicon optimization
- **Vulkan Support**: Cross-platform GPU backend
- **Automatic Fallback**: Graceful degradation to CPU when GPU unavailable

### 🧩 Plugin Architecture
- **LlamaCppPlugin**: Reference implementation for llama.cpp backend
- **ILLMPlugin Interface**: Extensible plugin system for custom LLM backends
- **Plugin Manager**: Centralized management with lifecycle control
- **Hot-Swappable**: Load/unload models and LoRA adapters dynamically

### 🗃️ Advanced Model Management (When LLM Enabled)
- **Lazy Loading**: Ollama-style on-demand model loading
- **Multi-LoRA Manager**: vLLM-style support for concurrent LoRA adapters
- **Model Pinning**: Prevent eviction of critical models from memory
- **TTL Management**: Automatic model eviction after configurable idle time

### 💾 Enterprise Caching
- **Response Cache**: Semantic caching for identical queries
- **Prefix Cache**: Reuse common prompt prefixes across requests
- **Model Metadata Cache**: TBB lock-free cache for faster metadata access
- **KV Cache Buffer**: Shared read-only buffers for memory optimization

### 🔧 Build & Deployment
- **Windows/MSVC Support**: PowerShell build script with Visual Studio 2022
- **MSVC Fixes**: `/Zc:char8_t-` compiler flag for llama.cpp compatibility
- **Docker BuildKit**: Flexible llama.cpp source (local or git clone)
- **Offline-First**: Root vcpkg standardization for reproducible builds

### 📚 Documentation
- **Consolidated Guides**: Root README + 17 specialized LLM docs (380 KB)
- **Integration Paths**: Local clone approach (no git submodules)
- **API Specifications**: HTTP REST + gRPC binary protocols
- **Client SDKs**: Python, JavaScript, Go, Rust, Java, C# examples

---

## 📦 What's Included

### Binary Artifacts
- `themis_server.exe` (10.2 MB) - Windows x64 Release
- `llama.dll` (2.2 MB) - llama.cpp inference engine
- `ggml*.dll` (1.4 MB) - GGML computation kernels

### Source Components
- **23 LLM Headers** (`include/llm/`)
- **23 LLM Implementations** (`src/llm/`)
- **17 Documentation Guides** (`docs/llm/`)
- **8+ Test Suites** (`tests/test_llm_*.cpp`)
- **PowerShell Build Script** (`scripts/build-themis-server-llm.ps1`)

---

## 🔄 Breaking Changes

### ⚠️ llama.cpp Submodule Removed
- **Before**: `external/llama.cpp` as git submodule
- **After**: Local clone in project root (excluded via `.gitignore`/`.dockerignore`)
- **Migration**: Clone llama.cpp locally: `git clone https://github.com/ggerganov/llama.cpp.git`

### ⚠️ vcpkg Standardization
- **Before**: Multiple vcpkg locations (`external/vcpkg`, `./vcpkg`)
- **After**: Single root `./vcpkg` with `VCPKG_ROOT` standardization
- **Migration**: Set `VCPKG_ROOT=C:\VCC\themis\vcpkg` (or your path)

### ⚠️ Docker Build Context
- **Before**: `external/` copied into build context
- **After**: `external/` excluded; use BuildKit `--build-context` for llama.cpp
- **Migration**: See Docker build commands below

---

## 🛠️ Installation & Upgrade

### Windows (MSVC)

```powershell
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Clone llama.cpp locally (required for LLM support)
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

# Build with LLM support
powershell -File scripts/build-themis-server-llm.ps1

# Verify build
./build-msvc/Release/themis_server.exe --help
```

### Docker (with LLM)

```bash
# With local llama.cpp clone
docker buildx build \
  --build-arg ENABLE_LLM=ON \
  --build-context llama=./llama.cpp \
  -t themisdb:v1.3.0-llm .

# Without local clone (git clone in Docker)
docker buildx build \
  --build-arg ENABLE_LLM=ON \
  -t themisdb:v1.3.0-llm .
```

### Linux/WSL

```bash
# Clone and build
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git clone https://github.com/ggerganov/llama.cpp.git llama.cpp

cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build -j$(nproc)

./build/themis_server --help
```

---

## 📖 Quick Start

### 1. Download a Model

```bash
# Example: Mistral 7B Instruct Q4 (~4GB)
mkdir -p models
cd models
wget https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf
```

### 2. Configure LLM

```yaml
# config/llm_config.yaml
llm:
  enabled: true
  plugin: llamacpp
  model:
    path: ./models/mistral-7b-instruct-v0.2.Q4_K_M.gguf
    n_gpu_layers: 32  # GPU offload layers
    n_ctx: 4096       # Context window
  cache:
    max_models: 3
    max_vram_mb: 24576  # 24 GB
```

### 3. Start Server

```bash
./themis_server --config config/llm_config.yaml
```

### 4. Run Inference (HTTP API)

```bash
curl -X POST http://localhost:8765/api/llm/generate \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "What is ThemisDB?",
    "max_tokens": 512,
    "temperature": 0.7
  }'
```

---

## 🎯 Performance Benchmarks

### Core Database Performance (Windows x64, 20 cores @ 3696 MHz)

**Comprehensive Benchmark Suite Results:**

| Operation | Throughput | Category |
|-----------|-----------|----------|
| RGB Vector Search (KNN) | 59.7M queries/s | Vector Operations |
| Binary Blob Retrieval | 49.0M lookups/s | Storage Layer |
| AQL Join Operations | 10.2M ops/s | Query Engine |
| Graph BFS Traversal | 9.56M traversals/s | Graph Operations |
| RAG Search (Top-50) | 7.17M queries/s | LLM/Vector |
| Simple AQL WHERE | 3.43M queries/s | Query Engine |
| 384D Vector Insert | 411k vectors/s | Vector Operations |

> **Full benchmark results**: See [BENCHMARK_DETAILED_RESULTS.md](benchmarks/BENCHMARK_DETAILED_RESULTS.md)

### LLM Performance (When Enabled with GPU)

**Note**: These are reference performance targets. Actual performance depends on:
- GPU hardware (model, VRAM)
- Model size and quantization
- Batch size and concurrent requests
- System configuration

| Metric | Approximate Range | Notes |
|--------|------------------|-------|
| GPU Speedup vs CPU | 10-100x | Varies by model size |
| Model Load Time | 2-5s | First load (lazy loading) |
| Inference Latency | 0.1-2s | Depends on model and prompt |
| Memory Savings (Caching) | 30-70% | With prefix/KV cache |

**Hardware Requirements for LLM:**
- **Minimum**: 8GB RAM, modern CPU (LLM disabled or CPU-only)
- **Recommended**: 16GB+ RAM, NVIDIA GPU with 8GB+ VRAM
- **Optimal**: 32GB+ RAM, NVIDIA GPU with 24GB+ VRAM

> **Important**: Performance claims are targets and may vary significantly based on hardware and workload.

### Lazy Loading Impact
| Scenario | Cold Start | Warm Cache | Benefit |
|----------|------------|------------|---------|
| First Request | 2.8s | - | - |
| Subsequent Requests | - | **~0ms** | Instant |
| After TTL Expiry | 2.8s | - | Auto-reload |

---

## 🔒 Security Considerations

- **Model Files**: Store models outside web root with proper permissions
- **API Authentication**: Enable Bearer Token (JWT) authentication in production
- **Rate Limiting**: Configure per-user quotas for inference requests
- **Resource Limits**: Set `max_vram_mb` and `max_models` to prevent exhaustion
- **Audit Logging**: All LLM operations logged for compliance

---

## 📊 Known Limitations

1. **Windows DLL Export Limit**: Use static build (`THEMIS_CORE_SHARED=OFF`) to avoid 65k symbol limit
2. **GPU Memory**: Requires sufficient VRAM for model + overhead (~100 MB CUDA)
3. **Model Format**: Only GGUF format supported (llama.cpp v2+)
4. **Concurrent Requests**: Limited by available VRAM and KV cache size
5. **Docker BuildKit**: Requires Docker 19.03+ and BuildKit enabled

---

## 🐛 Bug Fixes

- Fixed MSVC `char8_t` compilation errors in llama.cpp via `/Zc:char8_t-` flag
- Resolved vcpkg path conflicts between `external/vcpkg` and root `./vcpkg`
- Corrected Docker `.dockerignore` to exclude `llama.cpp/` from build context
- Removed circular submodule dependencies (`docker/tmp/openssl`, `external/llama.cpp`)
- Fixed CMake generator/architecture issues on Windows (requires `-A x64`)

---

## 📝 Deprecations

- **Git Submodules for llama.cpp**: Deprecated in favor of local clone approach
- **external/vcpkg**: Deprecated in favor of root `./vcpkg` location
- **Manual llama.cpp Setup**: Use `scripts/setup-llamacpp.sh` or PowerShell build script

---

## 🔮 Roadmap (v1.4.0)

- **Streaming Generation**: Server-Sent Events (SSE) for real-time responses
- **Batch Inference**: Process multiple requests in single forward pass
- **Distributed Sharding**: Multi-node LLM deployment with etcd coordination
- **vLLM Plugin**: Native vLLM backend for PagedAttention and continuous batching
- **Model Replication**: Raft consensus for cross-shard model synchronization
- **Advanced Quantization**: Support for AWQ, GPTQ, and custom quantization schemes

---

## 🙏 Acknowledgments

- **llama.cpp Team**: For the incredible inference engine (MIT License)
- **GGML**: For efficient tensor operations on CPU/GPU
- **HuggingFace**: For GGUF model hosting and community
- **ThemisDB Contributors**: For testing, feedback, and documentation improvements

---

## 📚 Documentation

- **LLM Integration Guide**: [docs/llm/LLAMA_CPP_INTEGRATION.md](docs/llm/LLAMA_CPP_INTEGRATION.md)
- **Plugin Development**: [docs/llm/README_PLUGINS.md](docs/llm/README_PLUGINS.md)
- **Architecture Review**: [docs/llm/INTEGRATION_REVIEW_AND_SEQUENCE.md](docs/llm/INTEGRATION_REVIEW_AND_SEQUENCE.md)
- **HTTP API Spec**: [docs/llm/HTTP_API_SPECIFICATION.md](docs/llm/HTTP_API_SPECIFICATION.md)
- **Docker Deployment**: [DOCKER_DEPLOYMENT.md](DOCKER_DEPLOYMENT.md)
- **Build Guide**: [docs/build/README.md](docs/build/README.md)

---

## 📞 Support & Community

- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Join community discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **Security**: [Report vulnerabilities](SECURITY.md)

---

## 📄 License

ThemisDB: MIT License  
llama.cpp: MIT License  
GGML: MIT License

---

**Full Changelog**: https://github.com/makr-code/ThemisDB/compare/v1.2.0...v1.3.0

**Download**: https://github.com/makr-code/ThemisDB/releases/tag/v1.3.0
