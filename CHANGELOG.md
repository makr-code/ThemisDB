<div align="center">

# 📝 Changelog

**ThemisDB Release History**

[![Version](https://img.shields.io/badge/version-1.3.4-blue)](https://github.com/makr-code/ThemisDB/releases)
[![Keep a Changelog](https://img.shields.io/badge/Keep%20a%20Changelog-v1.0.0-orange)](https://keepachangelog.com/)
[![Semantic Versioning](https://img.shields.io/badge/SemVer-v2.0.0-green)](https://semver.org/)

</div>

---

> **Format:** Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
> **Versioning:** [Semantic Versioning](https://semver.org/spec/v2.0.0.html)

---

## 🚧 [Unreleased]

### ✨ Added

- Future features and improvements will be listed here

---

## 🎉 [1.3.4] - 2025-12-28

> **📚 FOCUS:** Documentation Update & Build System

### ✨ Added

<details>
<summary><b>Documentation System Enhancement</b></summary>

- 📚 **MkDocs Integration** with Material theme for modern documentation
- 📄 **PDF Export** capability using mkdocs-with-pdf and print-site plugins
- 🌐 **GitHub Wiki Generation** from docs/de/ directory
- 🔍 **Full-text Search** across all documentation
- 📖 **Enhanced Navigation** with tabs, instant loading, and code copy
- 📑 **Print-Friendly Output** with single-page HTML for easy printing
- 🔖 **Hierarchical TOC** with bookmarks for PDF output

</details>

### 🔧 Changed

<details>
<summary><b>Version & Documentation Updates</b></summary>

- 📌 **Version Updated** to 1.3.4 across all documentation files
- 📝 **Release Notes** added for v1.3.4
- 🔄 **Cross-References** improved throughout documentation
- 📚 **German Documentation** fully updated in docs/de/

</details>

---

## 🎉 [1.3.3] - 2025-12-21

> **🌐 FOCUS:** Network Protocol Enhancements

### ✨ Added

<details>
<summary><b>Network Protocol Enhancements</b> (PR #111)</summary>

- 🌐 **HTTP/2 with Server Push** for CDC/Changefeed (proactive event delivery, ~0ms latency)
- 📡 **WebSocket support** with CDC streaming (bidirectional real-time communication)
- 📬 **MQTT broker** with WebSocket transport, rate limiting, monitoring metrics
- ⚡ **HTTP/3 base implementation** with QUIC (ngtcp2 + nghttp3)
- 🐘 **PostgreSQL Wire Protocol** with SQL-to-Cypher translation for BI tool compatibility
- 🔌 **MCP Server** (Model Context Protocol) with cross-platform stdio/SSE/WebSocket transports
- 🔒 **Production-ready security:** Explicit opt-in build switches, TLS/mTLS support
- ✅ **Comprehensive testing** with Google Test framework

</details>

---

## 🎉 [1.3.2] - 2025-12-21

> **🖼️ FOCUS:** Image Analysis AI Plugin Architecture

### ✨ Added

<details>
<summary><b>Image Analysis AI Plugin Architecture</b> (PR #118)</summary>

- 🖼️ **Plugin architecture** for image analysis AI (Stable Diffusion/CLIP) running parallel with LLM
- 🔧 **Multi-backend support:** llama.cpp Vision (primary), ONNX Runtime, OpenCV DNN, OpenVINO, ncnn
- ✅ Complete license compatibility analysis (all MIT/Apache 2.0/BSD compatible)
- 🛠️ Plugin interfaces: `IImageAnalysisBackend`, `ImageAnalysisManager`
- 📖 Comprehensive documentation: 7 C++ libraries evaluated, benchmarks, optimization guide
- 📦 Example ONNX CLIP plugin implementation
- ⚙️ Configuration templates for plugin management
- 🧪 Comprehensive unit tests (15+ test cases) and benchmarks (11+ categories)

</details>

---

## 🎉 [1.3.1] - 2025-12-20

> **📄 FOCUS:** Third-Party Attribution Documentation

### ✨ Added

<details>
<summary><b>Third-Party Attribution Documentation</b> (PR #119)</summary>

- 📄 Added `ATTRIBUTIONS.md` documenting 15+ core dependencies
- 🏆 Documented ThemisDB's **12 unique innovations** vs third-party features
- ✅ Clear attribution for RocksDB, FAISS, hnswlib, simdjson, Arrow, TBB, Boost, OpenSSL
- 📜 License information and repository links for all dependencies

</details>

---

## 🎉 [1.3.0] - 2025-12-17

> **🤖 PRIMARY FEATURE:** Optional LLM Integration (requires `-DTHEMIS_ENABLE_LLM=ON`)

### ✨ Added - LLM Integration (OPTIONAL)

<details open>
<summary><b>llama.cpp Integration</b></summary>

- 🧠 **Optional native LLM inference engine** (requires `-DTHEMIS_ENABLE_LLM=ON`)
- 🔌 Complete plugin-based architecture (`ILLMPlugin`, `LLMPluginManager`)
- 📦 GGUF model loader with Blob Store integration
- ⚡ Asynchronous inference engine for non-blocking operations
- 🦙 Support for LLaMA, Mistral, Phi-3 models (1B-70B parameters)
- 📊 Quantization support: Q4_K_M, Q5_K_M, Q8_0
- 📌 Requires external llama.cpp clone (not included in repository)

</details>

<details>
<summary><b>GPU Acceleration & Performance</b></summary>

- 🎮 **NVIDIA CUDA support** with automatic detection and graceful CPU fallback
- ⚡ Significant speedup vs CPU-only inference (hardware dependent)
- 📄 **PagedAttention** with BlockTable and Copy-on-Write prefix sharing
- 📅 **Continuous Batch Scheduler** supporting concurrent requests
- 🔥 **Kernel Fusion** with fused CUDA kernels for additional performance
- 🎯 Multi-compute capability support (Pascal to Ada architectures)

</details>

<details>
<summary><b>Advanced LLM Features</b></summary>

- 🦙 Ollama-style lazy model loading
- 🎨 vLLM-style multi-LoRA management
- 📦 Prefix caching for repeated prompts
- 💾 Response caching for common queries
- 🗂️ Model metadata caching

</details>

<details>
<summary><b>Monitoring & Observability</b></summary>

- 📊 **Grafana/Prometheus integration** with metrics
- 📈 Dashboard panels for LLM performance monitoring
- 🚨 Alert rules for production deployment
- 🐳 Docker Compose deployment stack

</details>

<details>
<summary><b>Testing & Quality</b></summary>

- ✅ Comprehensive unit tests for LLM functionality
- 🔗 Integration tests
- 🎬 End-to-end scenarios
- ⚡ Benchmark scenarios
- 📊 High test coverage

</details>

<details>
<summary><b>LLM Documentation (33 guides)</b></summary>

- 🎮 GPU integration guide
- 📊 Quantization guide
- ⚡ Performance benchmarks
- 🚀 Deployment guide
- 📖 Complete API documentation (HTTP, gRPC, AQL extensions)

</details>

### ✨ Added - RPC Framework (SUPPORTING INFRASTRUCTURE)

<details>
<summary><b>Distributed Communication</b></summary>

- 🔌 Protocol-agnostic `IRPCPlugin` and `IRPCServer` interfaces
- 📡 gRPC plugin implementation (258 LOC, security hardened)
- 🔒 TLS/mTLS support with X.509 certificates
- 🛠️ 15 RPC methods for CRUD, query, transactions, authentication

</details>

---

> [!NOTE]
> **For older releases and detailed changelogs, see the sections below.**

<details>
<summary><b>📅 Previous Releases (Click to expand)</b></summary>

## 🎯 [1.2.0] - 2025-11-28

### ✨ Added
- Enterprise security features
- Kubernetes operator
- Advanced monitoring

### 🔧 Changed
- Improved query performance
- Enhanced storage engine

### 🐛 Fixed
- Connection pool leaks
- Memory optimization issues

---

## 🚀 [1.1.0] - 2025-10-15

### ✨ Added
- Multi-model support enhancements
- GPU acceleration (10 backends)
- Client SDKs (7 languages)

### 🔧 Changed
- Refactored storage layer
- Improved API consistency

---

## 🎊 [1.0.0] - 2025-09-01

### ✨ Added
- Initial stable release
- ACID transactions with MVCC
- Horizontal sharding
- Basic security features

</details>

---

<div align="center">

**📖 Full Release History:** [GitHub Releases](https://github.com/makr-code/ThemisDB/releases)

[⬆️ Back to Top](#-changelog)

</div>
