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

- **Git Flow Branching Strategy** - Comprehensive workflow documentation (~125 KB)
  - 🌿 Implemented Git Flow with `main` (production) and `develop` (integration) branches
  - 📚 Bilingual documentation (DE/EN): BRANCHING_STRATEGY.md, BRANCHING_STRATEGY_EN.md
  - 🚀 Branch-based CI/CD strategy (fast builds on develop, full builds on main)
  - 📖 Visual guides, quick reference cards, and migration guide
  - 🛡️ Branch protection setup and CODEOWNERS configuration
  - 🔧 Integration in COPILOT_INSTRUCTIONS.md and BUILD_STRATEGIES.md

- **Multi-Agent LLM Reasoning Framework (v1.4.0)** - Collaborative AI problem-solving
  - 🧠 MultiAgentOrchestrator for task decomposition and coordination
  - 🤖 LLMAgent with role-based specialization and LoRA adapters
  - 📋 AgentRoleRegistry for managing agent roles and capabilities
  - 🤝 ConsensusBuilder with 5 strategies (MAJORITY_VOTE, WEIGHTED_AVERAGE, BEST_RESPONSE, SYNTHESIZE, HIERARCHICAL)
  - 🎓 LoRARegistry for dynamic LoRA adapter management
  - 🔄 3 execution patterns: PARALLEL, SEQUENTIAL, HIERARCHICAL
  - 📝 Example configurations: Legal Contract Analysis, Code Review, Research Assistant
  - 🔬 Based on state-of-the-art research: AutoGen, LangGraph, MetaGPT, Mixture of Agents
  - ⚡ 3-5x faster for complex multi-step tasks through parallelization
  - 💰 Cost efficient: Use smaller models (7B-13B) instead of large (70B+)
  - 📖 Comprehensive 600+ line concept document with best practices
  - 🚀 HTTP API endpoints: 6 RESTful endpoints (/api/llm/multi-agent/*)
  - ✅ 15+ test cases covering all core components
  - 📚 500+ line deployment guide (Docker, Kubernetes, monitoring)

- **ThemisDB Core Optimizations**
  - 🌐 **gRPC Protocol Support** - +30% performance, -70% serialization overhead
  - 🔍 **HNSW Parameter Tuning** - 5 optimized presets for vector search (+15-40% performance)
  - 📦 **WriteBatch API Documentation** - Complete guide with multi-agent patterns (+2-5× throughput)
  - 🧪 **Google Test & Benchmark Suite** - Comprehensive testing and performance benchmarking

### 🔧 Changed

- **Roadmap Update** - v1.5.0 Embedded LLM moved to v1.3.0, v1.4.0 now Multi-Agent Reasoning
- Updated LLM documentation to reflect new roadmap and v1.4.0 features

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

