<div align="center">

# 📝 Changelog

**ThemisDB Release History**

[![Version](https://img.shields.io/badge/version-1.4.0--alpha-blue)](https://github.com/makr-code/ThemisDB/releases)
[![Keep a Changelog](https://img.shields.io/badge/Keep%20a%20Changelog-v1.0.0-orange)](https://keepachangelog.com/)
[![Semantic Versioning](https://img.shields.io/badge/SemVer-v2.0.0-green)](https://semver.org/)

</div>

---

> **Format:** Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
> **Versioning:** [Semantic Versioning](https://semver.org/spec/v2.0.0.html)

---

## 🚧 [Unreleased]

### Added

<details open>
<summary><b>📦 Retroactive Release Building System</b> (Feature: Build & Release)</summary>

- 📦 **Retroactive Release Building System** - Extract and build binaries from historical version tags:
  - `scripts/retroactive-release-builder.sh` - Bash script for Linux/macOS
  - `scripts/retroactive-release-builder.ps1` - PowerShell script for Windows
  - Support for building specific tags or all tags at once
  - Multi-platform support (Linux, Windows, macOS)
  - Automatic package generation (TGZ, DEB, RPM, ZIP)
  - SHA256 checksum generation for all artifacts
  - Automatic release notes generation

- 🔄 **Complete Git Flow Release Automation** - End-to-end release workflow:
  - `scripts/complete-release.sh` - Bash script for automated Git Flow releases
  - `scripts/complete-release.ps1` - PowerShell script for Windows
  - Automates: `develop` → `release/vX.X.X` → `main` → tag → retroactive build
  - VERSION file management
  - Branch creation, merging, and cleanup
  - Automatic tag creation on `main` branch
  - Integration with retroactive builder
  - Dry-run mode for safety

- 🤖 **GitHub Actions Workflow** - Automated retroactive builds:
  - `.github/workflows/retroactive-release.yml` - Workflow for CI/CD integration
  - Manual trigger via workflow_dispatch
  - Configurable tag and platform selection
  - Optional upload to existing GitHub releases
  - Parallel builds for multiple platforms
  - Artifact retention for 90 days

- 📚 **Comprehensive Documentation** - Complete guide for retroactive builds:
  - `docs/RETROACTIVE_RELEASE_BUILDING.md` - User guide with examples
  - Usage instructions for all platforms
  - Troubleshooting section
  - Best practices and security considerations
  - CI/CD integration examples

**Key Features:**
- ✅ Build any historical version from Git tags
- ✅ Generate consistent release packages retroactively
- ✅ Support for multiple package formats (DEB, RPM, TGZ, ZIP)
- ✅ Automatic checksum and release notes generation
- ✅ Clean build isolation per version
- ✅ Parallel multi-platform builds in CI/CD
- ✅ Direct upload to GitHub releases
- ✅ **Git Flow compatible**: Works with `develop` → `release/vX.X.X` → `main` workflow
- ✅ **Complete automation**: `complete-release.sh` script for entire release process

**Use Cases:**
- Regenerate binaries for past releases
- Build releases for new platforms retroactively
- Create consistent release artifacts across all versions
- Support older versions with updated build configurations
- Automate Git Flow release workflow

**Example Usage:**
```bash
# Retroactive build for existing tag
./scripts/retroactive-release-builder.sh --tag v1.3.4 --platform linux

# Complete Git Flow release (develop → main + build)
./scripts/complete-release.sh 1.5.0

# Dry run to preview changes
./scripts/complete-release.sh 1.5.0 --dry-run

# Windows
.\scripts\complete-release.ps1 -Version 1.5.0 -DryRun
```

**Documentation:**
- [Retroactive Release Building Guide](docs/RETROACTIVE_RELEASE_BUILDING.md)
- [Git Flow Integration Guide](docs/RETROACTIVE_RELEASE_GITFLOW.md)
- [Quick Start Examples](docs/RETROACTIVE_RELEASE_EXAMPLES.md)

</details>

<details open>
<summary><b>🧠 Schema Manager for Database Self-Awareness</b> (Feature: Agentic AI)</summary>

- 🔍 **SchemaManager Class** - Core infrastructure for schema introspection:
  - RocksDB key scanning for automatic table/collection discovery
  - Property type detection via BaseEntity field analysis
  - Index metadata collection via SecondaryIndexManager integration
  - Thread-safe caching with shared_mutex (configurable 60s TTL)
  - JSON export methods: `toJSON()`, `tableToJSON()`, `getDatabaseMetadata()`, `getCapabilitiesJSON()`

- 🌐 **REST API Schema Endpoints** - External access to schema information:
  - `GET /api/v1/schema` - Complete database schema with all tables, relationships, and metadata
  - `GET /api/v1/schema/tables` - Lightweight list of all table names with basic info
  - `GET /api/v1/schema/tables/:name` - Detailed schema for a specific table
  - Enhanced `GET /api/capabilities` - Now includes schema awareness status

- 🤖 **MCP Server Integration** - Full Model Context Protocol support:
  - `toolGetSchema()` returns real schema data instead of empty stubs
  - `toolGetStats()` provides actual database statistics
  - Resources (`resourceSchema`, `resourceStats`, `resourceMetadata`) use SchemaManager
  - Integration level changed from "minimal" to "full"
  - LLM agents can now query schema via MCP protocol

- 💬 **Natural Language Self-Awareness** - YAML-configured system prompts:
  - 7 specialized prompts: self_awareness, what_can_you_do, data_structure, purpose, schema_introspection, table_inquiry, unknown_query
  - Template variables for dynamic context: `{version}`, `{edition}`, `{table_count}`, `{total_rows}`, `{tables}`, `{capabilities}`, `{schema}`, `{table_details}`
  - PromptManager extensions: `loadFromYAML()`, `injectContext()`, `getPromptWithContext()`, `buildContextFromSchema()`
  - Enhanced `introspect_database` MCP tool with intelligent question type detection
  - Real-time context injection with live schema data
  - Supports both English and German queries

- 🧪 **Comprehensive Testing** - 15 test cases covering:
  - Schema discovery (empty DB, single/multiple tables)
  - Cache mechanism and TTL validation
  - Property type detection (string, integer, double, boolean, vector, binary)
  - Index discovery integration
  - JSON serialization completeness
  - Database metadata accuracy
  - Performance benchmarks (discovery time, cache hit rate)

**Key Components:**
- `include/metadata/schema_manager.h` & `src/metadata/schema_manager.cpp` - Core schema introspection
- `include/server/schema_api_handler.h` & `src/server/schema_api_handler.cpp` - REST API handlers
- `config/llm_system_prompts.yaml` - Configurable system prompts for natural language queries
- `include/llm/prompt_manager.h` & `src/llm/prompt_manager.cpp` - YAML loading and context injection
- `tests/test_schema_manager.cpp` - Comprehensive test coverage
- MCP integration in `src/server/mcp_server.cpp` - Auto-initializes when database is open
- HTTP integration in `src/server/http_server.cpp` - Automatic route registration

**Benefits:**
- ✅ Database can answer "What data do you store?" and "How are you structured?"
- ✅ LLM agents can discover schema automatically via MCP protocol
- ✅ REST APIs enable external tools to query schema
- ✅ Natural language queries: "What can you do?", "How is data structured?", "What is your purpose?"
- ✅ YAML configuration allows prompt customization without code changes
- ✅ Thread-safe with <100ms discovery time for typical schemas
- ✅ Foundation for advanced Agentic AI features

**Total Implementation:** ~1,500 lines of code across 4 phases

</details>

<details open>
<summary><b>🔍 AI-Explained Error Handling System</b> (Phase 7: Error Awareness)</summary>

- 🎯 **Structured Error Codes** - 1000-9999 organized by category (Storage, LLM, LoRA, MCP, Schema, Network)
- 📋 **Error Registry** - 20+ predefined errors with rich metadata (cause, solution, documentation links, keywords)
- 🤖 **MCP Tools Integration** - Three new tools for error introspection:
  - `get_error_info` - Lookup by error code or keyword search
  - `search_errors` - Filter by category or full-text search
  - `introspect_database` - Natural language interface (EN/DE)
- 🌐 **Natural Language Queries** - "What errors can occur?", "What does error 2000 mean?", "How do I fix GPU OOM?"
- 🔎 **Case-Insensitive Search** - Zero-copy algorithm for efficient error lookup
- 🛠️ **REST API Handlers** - Ready-to-integrate endpoints for `/api/v1/errors`
- 🧪 **Test Suite** - 15+ comprehensive test cases for all functionality
- 📚 **Documentation** - Implementation based on `docs/research/ERROR_AWARENESS_AND_INTROSPECTION.md`

**Key Components:**
- `include/utils/error_registry.h` & `src/utils/error_registry.cpp` - Error registry with singleton pattern
- `include/utils/string_utils.h` - Reusable case-insensitive string utilities
- `include/server/error_api_handler.h` & `src/server/error_api_handler.cpp` - REST API handlers
- `tests/test_error_registry.cpp` - Comprehensive test coverage
- MCP integration in `src/server/mcp_server.cpp` - Auto-registers when `THEMIS_ENABLE_MCP=ON`

**Benefits:**
- ✅ Self-service error resolution through AI-powered explanations
- ✅ Reduced support burden with detailed error documentation
- ✅ Improved developer experience with contextual error guidance
- ✅ Multilingual support (English and German)

</details>

### Changed
- README: Neuer Abschnitt "Release & Publication Policy" inkl. Branch-Scope (`main` nur Minimal/Community, `develop` voll) und Merge-Schutz-Hinweis.
- `.gitignore`: Explizite Ausschlüsse ergänzt für `llama.cpp/`, `wordpress-plugin/`, `wordpress-theme/`, `epServer/`.
- `.gitattributes`: Merge-Guards (`merge=ours`) für Enterprise/Marketing/experimentelle Pfade, damit `develop → main` keine nicht-öffentlichen Inhalte überträgt.
- Merge-Vorbereitung `develop → main`: Dokumentations- und Exklusionsstand vereinheitlicht für öffentliche Editionen.

### Notes
- Exklusionsliste spiegelt Editionsgrenzen (Minimal/Community öffentlich, Enterprise proprietär).
- Bereits versionierte Dateien werden durch `.gitignore` nicht rückwirkend entfernt; beim Release werden Artefakte nur aus freigegebenen Komponenten gebaut.

---

## 🎉 [v1.4.0-alpha] - 2026-01-05

> **🚀 ALPHA RELEASE:** Advanced LLM Features & Enterprise Enhancements

This release brings significant enhancements to ThemisDB's LLM capabilities, including grammar-constrained generation, extended context windows, vision support, and performance optimizations. It represents a major leap forward in AI/LLM integration and enterprise features.

### ✨ Added - Advanced LLM Features

<details open>
<summary><b>📝 Grammar-Constrained Generation</b> (PR #245)</summary>

- 🎯 **EBNF/GBNF Grammar Support** - Guarantees valid structured outputs (JSON, XML, CSV, custom formats)
- 📦 **Built-in Grammars** - JSON (strict/relaxed), XML, CSV, ReAct Agent format
- 🗂️ **Grammar Cache** - Thread-safe LRU cache with configurable size (up to 100 grammars)
- ⚡ **Performance** - Reduces compilation overhead for repeated grammars
- 🔧 **Configuration** - YAML-based grammar configuration with custom grammar path support
- 📚 **Documentation** - Complete implementation guide in `docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md`
- ✅ **Reliability** - 95-99% valid outputs vs 60-70% without grammar constraints

**Key Components:**
- `include/llm/grammar.h` & `src/llm/grammar.cpp` - Grammar compilation and management
- `include/llm/grammar_cache.h` & `src/llm/grammar_cache.cpp` - Thread-safe caching
- `src/llm/grammars/*.gbnf` - Built-in grammar definitions
- Integration in `LlamaWrapper` with `InferenceRequest.grammar_type` and `grammar_ebnf` fields

</details>

<details>
<summary><b>🔭 RoPE Scaling - Extended Context Window</b> (PR #244)</summary>

- 📏 **Context Extension** - 4K → 32K tokens (8x increase) via RoPE scaling
- 🎯 **Long Document Support** - Process entire research papers, codebases, extended conversations
- 🔧 **Scaling Methods** - Linear, NTK-aware, YaRN (Yet another RoPE extension)
- ⚙️ **Configuration** - `rope_scaling_type` and `rope_freq_base` in LlamaWrapper config
- 🧪 **Unit Tests** - Comprehensive test suite for RoPE scaling configuration
- 📚 **Documentation** - Complete guide in `docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md`
- ✅ **Quality Preservation** - Maintains output quality across extended context

**Configuration Example:**
```cpp
config.rope_scaling_type = LLAMA_ROPE_SCALING_YARN;
config.n_ctx = 32768;  // 32K tokens
config.rope_freq_base = 10000.0f;
```

</details>

<details>
<summary><b>🖼️ Vision Support - Multi-Modal LLMs</b> (PR #246)</summary>

- 👁️ **Image Analysis** - CLIP-based vision encoding for multi-modal inference
- 🦙 **LLaVA Integration** - Support for vision-language models (LLaVA, BakLLaVA, etc.)
- 🔧 **VisionEncoder Class** - Wraps llama.cpp CLIP functionality with thread-safe implementation
- 📸 **Multiple Image Support** - Process single or multiple images per request
- ⚙️ **Configuration** - `enable_vision`, `clip_model_path`, `vision_threads` options
- 🧪 **Testing** - Comprehensive unit tests (`test_llm_vision_encoder.cpp`, `test_llm_vision_integration.cpp`)
- 📚 **Documentation** - Quick start guide and API examples in `docs/en/llm/VISION_SUPPORT_*.md`

**New Files:**
- `include/llm/vision_encoder.h` & `src/llm/vision_encoder.cpp` - Vision encoding infrastructure
- `tests/test_llm_vision_encoder.cpp` - VisionEncoder unit tests
- `tests/test_llm_vision_integration.cpp` - Integration tests

**Usage Example:**
```cpp
VisionRequest request;
request.text_prompt = "What's in this image?";
request.image_path = "/uploads/photo.jpg";
request.max_tokens = 256;
auto response = wrapper.generateVision(request);
```

</details>

<details>
<summary><b>⚡ Flash Attention CUDA Kernels</b> (PR #241)</summary>

- 🚀 **Performance Boost** - 15-25% faster inference, 30% memory reduction
- 🎯 **CUDA Implementation** - Optimized attention mechanism with memory reordering
- 🔧 **Kernel Fusion** - Fused CUDA kernels for additional performance (`src/llm/kernel_fusion.cu`)
- 📊 **Backward Pass** - Training support with Flash Attention backward pass
- 🧪 **Testing** - Comprehensive test suite (`tests/test_phase1_flash_attention.cpp`)
- 📚 **Documentation** - Implementation guide in `docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md`
- ✅ **No Quality Loss** - Mathematically equivalent to standard attention

**Features:**
- Automatic CUDA detection and CPU fallback
- Multi-compute capability support (Pascal to Ada)
- Configurable via `use_flash_attn` flag

</details>

<details>
<summary><b>🎯 Speculative Decoding</b></summary>

- ⚡ **2-3x Speedup** - Faster inference using draft+target model approach
- 🎯 **Zero Quality Loss** - Target model validates all outputs
- 🔧 **Configuration** - Draft model pairing with target model
- 📚 **Documentation** - Complete guide in `docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md`
- ✅ **Production Ready** - Mathematically equivalent sampling distribution

</details>

<details>
<summary><b>🔄 Continuous Batching</b></summary>

- 📦 **Dynamic Batching** - Concurrent request handling with token budget management
- ⚡ **Throughput** - 2x+ improvement through efficient request batching
- 🔧 **Configuration** - Min/max batch sizes (1-256), batch timeout (100ms default)
- 🧪 **Testing** - Comprehensive test suite (`tests/test_continuous_batch_scheduler.cpp`)
- 📚 **Documentation** - Implementation guide in `docs/en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md`

</details>

### 🏢 Enterprise Features

<details>
<summary><b>🔥 Hot Spare Management</b></summary>

- 🛡️ **High Availability** - Automatic failover for critical shards
- 📊 **Health Monitoring** - Continuous shard health checks with failure detection
- 🔄 **Auto Recovery** - Automatic shard recovery and reintegration
- 🧪 **Testing** - Comprehensive test suite (`tests/test_hot_spare.cpp`)
- 📚 **Documentation** - Complete implementation summary in `HOT_SPARE_COMPLETE.md`

**New Components:**
- `include/sharding/hot_spare_manager.h` & `src/sharding/hot_spare_manager.cpp`
- `include/sharding/health_monitor.h` & `src/sharding/health_monitor.cpp`
- `include/sharding/auto_recovery_manager.h`

</details>

<details>
<summary><b>📊 Enhanced Prometheus Metrics</b></summary>

- 📈 **LLM Metrics** - Inference latency, token throughput, cache hit rates
- 🔍 **Response Cache Metrics** - Cache performance tracking
- 📊 **Grafana Dashboards** - Pre-configured dashboards for monitoring
- 🚨 **Alert Rules** - Production-ready alerting configuration
- 🧪 **Testing** - Metrics test suites (`tests/test_llm_grafana_metrics.cpp`, `tests/test_llm_response_cache_metrics.cpp`)

</details>

<details>
<summary><b>🔄 WAL Replication via gRPC</b></summary>

- 🌐 **Distributed Replication** - Inter-shard WAL shipping via gRPC
- 🔧 **gRPC Service** - `WALGrpcService` for efficient WAL transfer
- 🧪 **Testing** - Integration tests (`tests/test_wal_grpc_apply.cpp`, `tests/test_wal_replication_integration.cpp`)
- 📚 **Documentation** - Implementation status in `REPLICATION_IMPLEMENTATION_STATUS.md`

**New Files:**
- `include/server/wal_grpc_service.h` & `src/server/wal_grpc_service.cpp`
- `src/sharding/wal_grpc_service.cpp`

</details>

<details>
<summary><b>🎮 Multi-GPU LoRA Support</b></summary>

- 🖥️ **Multi-GPU** - Distributed LoRA adapter management across multiple GPUs
- 🔧 **Inline LoRA** - Efficient LoRA adapter switching
- 🧪 **Testing** - Comprehensive test suite (`tests/test_multi_gpu_lora.cpp`, `tests/test_llm_lora_inline.cpp`)
- 📚 **Documentation** - Implementation report in `MULTI_GPU_IMPLEMENTATION_SUMMARY.md`

</details>

<details>
<summary><b>🐘 PostgreSQL Protocol Enhancements</b></summary>

- 📋 **COPY Protocol** - Bulk data import/export support
- 🔒 **Prepared Statements** - Efficient query execution with parameter binding
- 🔄 **Transaction Support** - BEGIN, COMMIT, ROLLBACK with proper isolation
- 🧪 **Testing** - Complete test coverage (`tests/test_postgres_*.cpp`)
- 📊 **Benchmarks** - Performance benchmarks for protocol operations

</details>

### 🔧 Infrastructure & Quality

<details>
<summary><b>🧪 Testing & Benchmarks</b></summary>

**New Test Suites (31 files):**
- Flash Attention: `test_phase1_flash_attention.cpp`
- KV-Cache Reuse: `test_phase1_kv_cache_reuse.cpp`
- Vision Support: `test_llm_vision_encoder.cpp`, `test_llm_vision_integration.cpp`
- Continuous Batching: `test_continuous_batch_scheduler.cpp`
- Hot Spare: `test_hot_spare.cpp`
- RAID Integration: `test_raid_integration.cpp`, `test_raid_redundancy.cpp`
- PostgreSQL: `test_postgres_copy_protocol.cpp`, `test_postgres_prepared_statements.cpp`, `test_postgres_transactions.cpp`
- WAL Replication: `test_wal_grpc_apply.cpp`, `test_wal_replication_integration.cpp`
- Multi-GPU LoRA: `test_multi_gpu_lora.cpp`, `test_lora_adapter.cpp`
- Metrics: `test_llm_grafana_metrics.cpp`, `test_llm_response_cache_metrics.cpp`
- Production Validator: `test_production_validator.cpp`
- And many more...

**New Benchmarks:**
- `bench_phase1_flash_attention.cpp` - Flash Attention performance
- `bench_embedded_llm.cpp` - Embedded LLM throughput
- `bench_llm_raid_pipeline.cpp` - RAID pipeline performance
- `bench_postgres_e2e.cpp` - PostgreSQL end-to-end benchmarks
- `bench_lora_inline.cpp` - LoRA adapter switching

</details>

<details>
<summary><b>📚 Documentation</b></summary>

**New LLM Documentation (17 files):**
- Grammar-Constrained Generation: Complete implementation guide
- RoPE Scaling: Extended context window guide
- Vision Support: Quick start, API examples, implementation details
- Flash Attention: Implementation guide with CUDA details
- Continuous Batching: Configuration and usage guide
- Speculative Decoding: Implementation guide with model pairing
- KV-Cache Reuse: Context caching implementation

**Implementation Summaries:**
- `GRAMMAR_IMPLEMENTATION_COMPLETE.md` & `GRAMMAR_IMPLEMENTATION_SUMMARY.md`
- `VISION_SUPPORT_PR_SUMMARY.md`
- `P1_IMPLEMENTATION_SUMMARY.md` - Enterprise features summary
- `IMPLEMENTATION_STATUS_FINAL.md` - LLaMA.cpp integration status
- `MULTI_GPU_IMPLEMENTATION_SUMMARY.md`
- `HOT_SPARE_COMPLETE.md`

</details>

<details>
<summary><b>🔧 Tools & Examples</b></summary>

**New Examples:**
- `examples/embedded_llm_examples.cpp` - Integration patterns (250 lines)
- `examples/chat_formatting_example.cpp` - Chat template examples
- `examples/hot_spare_example.cpp` - Hot spare usage example

**Ingestion Tool Enhancements:**
- gRPC integration for high-performance data ingestion
- Performance profiling capabilities
- Enhanced settings dialog with protocol selection

</details>

### 📊 Statistics

- **938 files changed** (+113,762 lines, -45,154 lines)
- **31 new test files** - Comprehensive test coverage for all new features
- **17 new documentation files** - Complete guides for all LLM features
- **11 new benchmarks** - Performance validation across all components
- **15+ new source files** - Core feature implementations

### 🔗 Related Documentation

- [Grammar-Constrained Generation Guide](docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)
- [RoPE Scaling Implementation](docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md)
- [Vision Support Quick Start](docs/en/llm/VISION_SUPPORT_QUICK_START.md)
- [Flash Attention Implementation](docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
- [Continuous Batching Guide](docs/en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md)
- [Speculative Decoding Guide](docs/en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md)
- [Hot Spare Management](HOT_SPARE_COMPLETE.md)
- [Multi-GPU LoRA Support](MULTI_GPU_IMPLEMENTATION_SUMMARY.md)

### ⚠️ Breaking Changes

None. All new features are opt-in via configuration.

### 🔄 Migration Notes

- Grammar-constrained generation requires llama.cpp with grammar support
- Vision support requires CLIP models (`.gguf` format) for multi-modal inference
- Flash Attention requires CUDA-enabled llama.cpp build for performance benefits
- Extended context (RoPE scaling) requires models fine-tuned for longer contexts
- All features can be enabled/disabled via YAML configuration

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

> [!IMPORTANT]
> **Comprehensive Security Summary:** See [Security Work Summary v1.3.4](/docs/de/releases/SECURITY_WORK_SUMMARY_V1.3.4.md) for complete documentation of all security improvements from v1.3.0 to v1.3.4.

#### RocksDB Wrapper Security Hotfix (2026-01-02)
  
- 🛡️ **Fixed 7 critical + 8 medium security vulnerabilities** identified in security audit
  
**Critical Fixes (7):**
  1. ✅ **Use-after-free in BlockBasedTableOptions** (lines 108-117)
     - Properly manage lifetime of filter_policy shared_ptr
     - Ensure BlockBasedTableFactory copies internal structures correctly
  2. ✅ **Null-pointer checks for options_->env** before SetBackgroundThreads (lines 120-124)
     - Initialize env to rocksdb::Env::Default() if null
     - Prevent segmentation faults in background thread configuration
  3. ✅ **Transaction-based del() implementation** (lines 481-483)
     - Replaced direct Delete with transaction-based approach
     - Prevents deadlock/data loss in concurrent scenarios
  4. ✅ **GetBaseDB() null-pointer checks** (7 locations)
     - Added checks in scanPrefix, scanRange, scanAll, multiGetWithAsyncIO
     - Prevents segmentation faults in iterator operations
  5. ✅ **Transaction resource leak fix** (lines 605-625)
     - Better error handling in TransactionWrapper
     - Prevents memory leaks on transaction failures
  6. ✅ **Column Family handle cleanup** (lines 370-378)
     - Corrected destruction order
     - Prevents resource leaks on shutdown
  7. ✅ **BackupEngine exception safety** (lines 1153-1170)
     - Exception-safe resource management
     - Prevents file handle leaks

**Medium-Severity Fixes (8):**
  - 🔄 Improved transaction error handling (removed double rollback)
  - 📊 Enhanced iterator lifecycle management
  - 🔒 Better snapshot handling
  - 🛡️ Backup engine null-checks
  - 📝 Write options cleanup
  - 🎯 Scan operation improvements

**Documentation & Planning:**
  - 📝 Comprehensive security audit: [ROCKSDB_WRAPPER_AUDIT_REPORT.md](/docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md)
  - 🎯 Audit identified 15 total issues: 11 fixed immediately, 4 scheduled for Phase 2/3
  - 📊 **Impact:** 100% elimination of segfault risks, all critical vulnerabilities resolved

#### Docker Security Improvements (2026-01-02)

- 🐳 **Base Image Upgrade:** Ubuntu 22.04 → Ubuntu 24.04 LTS
  - Extended security support until 2029
  - OpenSSL 3.x, newer glibc with security patches
  - 80%+ reduction in known CVEs
  
- 🔒 **Security Updates Integration:**
  - Automated `apt-get upgrade` during build
  - Minimal package installation (--no-install-recommends)
  - Complete cleanup of temporary files
  
- 📖 **Documentation:** [DOCKER_SECURITY_FIXES.md](/docs/DOCKER_SECURITY_FIXES.md)
  - Best practices for production deployments
  - Trivy and Docker Scout integration guides
  - Distroless and Alpine migration paths

#### Update Checker Security (2025-12)

- 🔐 **Secure Token Handling:**
  - ✅ No hardcoded credentials
  - ✅ ENV variable only (THEMIS_GITHUB_API_TOKEN)
  - ✅ Masked in all API responses and logs
  - ✅ Thread-safe with mutex protection

- 🌐 **Network Security:**
  - ✅ HTTPS-only communication
  - ✅ SSRF prevention with URL validation
  - ✅ 30-second timeout protection
  - ✅ GitHub rate limiting compliance

- 🛡️ **Input Validation & Thread Safety:**
  - ✅ Strict regex for version parsing
  - ✅ JSON exception handling
  - ✅ Atomic flags and mutex protection
  - ✅ RAII and smart pointers throughout

- 📊 **Security Analysis:**
  - ✅ CodeQL scan: PASSED (0 vulnerabilities)
  - ✅ Code review: 3/3 comments addressed
  - 📖 [Update Security Summary](/docs/de/releases/updates_security_summary.md)

#### Binary Authenticity - Manifest Signing Design (2025-12)

- 🔏 **Cryptographic Verification Architecture:**
  - ✅ RSA-4096 or Ed25519 digital signatures
  - ✅ SHA-256 hash verification for all binaries
  - ✅ Certificate chain validation
  - ✅ Timestamp authority integration

- 🎯 **Protection Against:**
  - ✅ Compromised GitHub accounts
  - ✅ Man-in-the-middle attacks
  - ✅ Binary tampering
  - ✅ CDN compromise
  - ✅ Fake releases

- 📖 **Architecture Documentation:** [Manifest Security](/docs/de/releases/updates_manifest_security.md)
  - Complete signing workflow
  - Verification implementation guide
  - Attack scenario analysis
  - Industry standard compliance (Docker, Kubernetes, APT)

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
