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

---

## [v1.3.4-hotfix] - 2026-01-04

### 🐛 Fixed - Critical RAID Sharding Deadlock

- **RocksDB MVCC Deadlock in Sharding Mode** (2026-01-04)
  - 🔧 **CRITICAL**: Fixed server hang at "Adaptive Index Manager initialized" in RAID mode
    - Root Cause: AdaptiveIndexManager attempted MVCC coordination across 2 Column Families before Sharding Manager initialization
    - RocksDB TransactionDB opened 2nd CF (for future MVCC isolation) before sharding context was ready → DEADLOCK
  - ✅ **Solution**: Conditional Column Family opening in `src/storage/rocksdb_wrapper.cpp` (Lines 347-365)
    - Skip 2nd CF when `THEMIS_ENABLE_SHARDING=true` detected via environment variable
    - Only open default CF in sharding mode, defer additional CFs until after cluster coordination
  - 📊 **Solution**: Initialization order fix in `src/server/http_server.cpp` (Lines 287-321)
    - Added sharding detection block BEFORE AdaptiveIndexManager initialization
    - Log sharding context (shard ID, bootstrap node) before MVCC operations
  - ✅ **Verification**: All 9 RAID shards (RAID 0/1/5) successfully reach "READY FOR OPERATIONS"
  - 📝 Documentation: Complete hotfix analysis in HOTFIX_*.md files (ISSUE, DISCOVERY, ROOT_CAUSE, ANALYSIS_COMPLETE)

- **Docker Compose Port Mapping** (2026-01-04)
  - 🔧 **CRITICAL**: Fixed incorrect port mappings in `docker/compose/docker-compose-sharding.yml`
    - All HTTP port mappings were `808X:8080`, but ThemisDB server listens on port `8765` (via THEMIS_PORT)
    - Changed all mappings to `808X:8765` for correct HTTP/REST API exposure
    - Affected: All 9 shards (RAID0: 8080-8082, RAID1: 8083-8084, RAID5: 8085-8087)
  - ✅ **Result**: All HTTP endpoints now respond with 200 OK on /health checks

### ✨ Added - RAID Testing & Monitoring

- **RAID Endurance Test Suite** (2026-01-04)
  - 🧪 **scripts/raid_endurance_test.py** - 2-hour automated endurance test (305 lines)
    - Tests all 3 RAID modes: RAID 0 (Striping), RAID 1 (Mirroring), RAID 5 (Striping + Parity)
    - Batch operations: 100 writes + 50 reads per iteration (375 total ops/cycle)
    - Performance metrics: write/read latency tracking, error rates, success rates
    - Health monitoring: /health endpoint polling every 20 iterations
    - Round-robin load balancing across shards
  - 📊 **scripts/monitor_raid_test.ps1** - Real-time monitoring dashboard
    - Progress tracking: elapsed time, remaining time, percentage complete
    - Container health status: live monitoring of all 9 shards
    - Visual progress bar with 30-second update intervals
  - ✅ **Verification**: Test runs successfully with 0% error rate across all RAID modes

### 🔧 Changed - Docker Build Optimization

- **Dockerfile.themis-server** (2026-01-04)
  - 📦 **vcpkg Integration**: Changed from `git clone` to local COPY for reliability
    - Avoids transient network issues (boost-cmake port failures)
    - Includes `vcpkg/.git` for baseline resolution
    - Disables metrics with `bootstrap-vcpkg.sh -disableMetrics`
    - Uses `VCPKG_BINARY_SOURCES="clear;default"` for default cache behavior
  - ✅ **Result**: Build completes successfully in ~15 minutes, 166MB final image

- **.dockerignore** (2026-01-04)
  - 🗜️ **Build Context Optimization**: Reduced from >3GB to ~85MB
    - Included: `vcpkg/` directory with ports, scripts, and .git
    - Excluded: `vcpkg/buildtrees/`, `vcpkg/downloads/`, `vcpkg/packages/`, `vcpkg/installed/`
    - Preserved baseline resolution capability while removing large artifacts
  - ✅ **Result**: 97% reduction in Docker build context size

### 📚 Documentation

- **Hotfix Documentation Series** (2026-01-04)
  - HOTFIX_ISSUE.md - Initial problem description (server hang in RAID mode)
  - HOTFIX_DISCOVERY.md - Systematic debugging process
  - HOTFIX_ROOT_CAUSE.md - Technical analysis of MVCC deadlock
  - HOTFIX_ANALYSIS_COMPLETE.md - Complete resolution summary
- **Updated docker/compose/docker-compose-sharding.yml**
  - Bootstrap node topology: Shard1 knows full cluster, others discover via THEMIS_BOOTSTRAP_SHARD
  - Added THEMIS_CLUSTER_DISCOVERY_TIMEOUT (15000ms) for cluster coordination
  - Increased healthcheck start_period to 60s for initialization window

---

### 🔒 Security

- **RocksDB Wrapper Security Hotfix** (2026-01-02)
  - 🛡️ Fixed 7 critical security vulnerabilities identified in security audit
  - 🔧 **CRITICAL**: Fixed use-after-free vulnerability in BlockBasedTableOptions (lines 108-117)
    - Properly manage lifetime of filter_policy shared_ptr
    - Ensure BlockBasedTableFactory copies internal structures correctly
  - ✅ **CRITICAL**: Added null-pointer checks for `options_->env` before SetBackgroundThreads (lines 120-124)
    - Initialize env to rocksdb::Env::Default() if null
    - Prevent segmentation faults in background thread configuration
  - 🔄 **MEDIUM**: Improved transaction error handling
    - Removed double rollback after commit failures
    - Better resource cleanup in transaction lifecycle
  - 📊 **MEDIUM**: Enhanced iterator lifecycle management
    - Explicit iterator cleanup to prevent resource leaks
    - Proper lock management in scan operations
  - 📝 Comprehensive security audit completed with detailed findings in [ROCKSDB_WRAPPER_AUDIT_REPORT.md](ROCKSDB_WRAPPER_AUDIT_REPORT.md)
  - 🎯 Audit identified 15 total issues: 11 fixed immediately, 4 scheduled for next phase

### ✨ Added

- **Git Flow Branching Strategy** - Comprehensive workflow documentation (~125 KB)
  - 🌿 Implemented Git Flow with `main` (production) and `develop` (integration) branches
  - 📚 Bilingual documentation (DE/EN): BRANCHING_STRATEGY.md, BRANCHING_STRATEGY_EN.md
  - 🚀 Branch-based CI/CD strategy (fast builds on develop, full builds on main)
  - 📖 Visual guides, quick reference cards, and migration guide
  - 🛡️ Branch protection setup and CODEOWNERS configuration
  - 🔧 Integration in COPILOT_INSTRUCTIONS.md and BUILD_STRATEGIES.md

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
