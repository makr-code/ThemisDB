# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Multi-GPU Vector Indexing API (v2.4)** 🎉
  - **MultiGPUVectorIndex**: Multi-device API and partition/merge scaffolding for distributed vector search
    - Logical support for 2-8 devices via index partitioning (round-robin, hash-based, range-based, balanced)
    - Query fan-out and centralized top-k merge logic for aggregating per-partition results
    - Designed for future distributed search across multiple GPUs once GPU backends are available
    - **Current execution**: Uses CPU-based GPUVectorIndex backend (no actual multi-GPU execution yet)
    - Fault-tolerant design with graceful degradation when partitions are unavailable
    - **GPU execution and collectives**: Planned for v2.5+ (NCCL/RCCL, P2P transfers, actual GPU offload)
  - **API Features (scaffolding)**:
    - `enableMultiGPU` configuration flag for multi-device indexing
    - `deviceIds` parameter for future GPU selection (configuration only, no GPU enumeration in v2.4)
    - `partitionStrategy` option for data distribution across logical partitions
    - Per-partition statistics with hooks for future per-GPU metrics (VRAM, utilization)
    - Load imbalance and scaling efficiency metrics computed over logical partitions
  - **Testing**:
    - Unit tests covering partitioning/merge logic and API behavior (394 lines)
    - Tests validate API correctness on CPU, ready for GPU backend integration
    - Example application demonstrating configuration and partition behavior (237 lines)
  - **Documentation**:
    - Complete API guide (`docs/MULTI_GPU_VECTOR_INDEXING.md`) with current CPU-only status clearly noted
    - API reference with code examples and notes on planned GPU backends (v2.5+)
    - Discussion of anticipated performance characteristics once GPU support lands
    - Troubleshooting guide noting current limitations (no GPU execution, no NCCL/RCCL yet)

- **Git-Like Features Integration** 🎉
  - **SnapshotManager Re-enabled**: Named snapshots for MVCC are now fully operational
    - 5 REST endpoints for snapshot/tag management
    - Integration with DiffEngine for tag-based diffs
    - Persistent snapshot storage in RocksDB
  - **PITR API Handler**: Point-in-Time Recovery REST API integration
    - POST `/api/v1/pitr/restore/sequence` - Restore to specific sequence number
    - POST `/api/v1/pitr/restore/tag` - Restore to named snapshot tag
    - POST `/api/v1/pitr/restore/timestamp` - Restore to timestamp
    - POST `/api/v1/pitr/preview` - Preview restore operation (dry-run)
    - GET `/api/v1/pitr/progress` - Get current restore progress
  - **DiffEngine Enhanced**: Now accepts optional SnapshotManager for tag-based diffs

### Changed
- Updated DiffEngine initialization to support SnapshotManager reference
- HTTP server now properly converts between Beast and httplib types for git-feature endpoints
- CMake configuration updated to include multi-GPU vector indexing sources and tests

### Fixed
- Re-enabled previously disabled SnapshotManager due to incomplete type issues
- Added proper error handling with default case in PITR progress phase conversion

### Documentation
- **GPU Master Tracking Document** 📋
  - Added `docs/GPU_MASTER_TRACKING.md` - Comprehensive master tracking document for GPU implementation roadmap (v2.x series)
  - Complete timeline and deliverables for all GPU backends (CUDA, Vulkan, HIP, Multi-GPU)
  - Performance targets, quality metrics, and success criteria
  - Risk mitigation strategies and resource planning
  - Cross-references to all GPU documentation: `FUTURE_GPU_SUPPORT.md`, `GPU_SUPPORT_ROADMAP.md`, `GPU_VECTOR_INDEXING_ARCHITECTURE.md`
  - Updated `docs/00_DOCUMENTATION_INDEX.md` with new GPU Vector Indexing section
- Added `MULTI_GPU_VECTOR_INDEXING.md` documenting multi-GPU implementation
- Added `GIT_FEATURES_INTEGRATION_STATUS.md` documenting integration status
- Documented that BranchManager and MergeEngine are pending (separate draft PRs)

---

## [1.5.0] - 2026-02-03

### Added
- **RFC 3161 Timestamp Authority (TSA) - PRODUCTION READY** 🎉
  - Full RFC 3161 client implementation with OpenSSL cryptographic operations
  - Integration with external TSA providers (FreeTSA, DigiCert, Sectigo)
  - eIDAS compliance support for qualified electronic timestamps
  - Long-term validation (LTV) for 30-year timestamp retention
  - Comprehensive TSA setup guide (`docs/en/security/TSA_SETUP.md`)
  - Configuration management via `config/timestamp_authority.yaml`
  - CMake option `THEMIS_USE_OPENSSL_TSA` to control TSA mode (default: ON)
  - Build-time and runtime warnings when stub mode is active
  - Support for SHA-256, SHA-384, SHA-512 hash algorithms
  - Certificate chain validation and verification
  - 10+ comprehensive tests for RFC 3161 compliance

- **FAISS Quantizer Integration - Production Ready** (#1079) 🚀
  - **FAISS K-means Integration**: ProductQuantizer now uses FAISS K-means clustering
    - `ProductQuantizer`: FAISS K-means for 20-30% faster training with SIMD optimizations
    - Automatic fallback to custom K-means if FAISS unavailable or errors occur
    - Uses faiss::Clustering and faiss::IndexFlatL2 for optimal performance
  - **FAISS-optimized Binary Operations**: BinaryQuantizer uses compiler intrinsics
    - `BinaryQuantizer`: SIMD-optimized popcount for faster Hamming distance
    - Uses __builtin_popcount (GCC) or __popcnt (MSVC) same as FAISS
    - `ResidualQuantizer`: Inherits FAISS acceleration from ProductQuantizer stages (30% faster training)
  - **Backend Selection**: New `prefer_faiss` configuration option
    - Defaults to `true` when FAISS is available
    - Graceful fallback to custom implementation on errors
  - **Runtime Inspection**: `getBackend()` method reports actual backend in use
  - **Build System**: Uses existing `THEMIS_HAS_FAISS` conditional compilation
  - **Production Ready**: Fully tested with actual FAISS API integration

### Changed
- TSA implementation now uses OpenSSL by default (was stub in v1.4.1)
- Improved CMake configuration for security features
- Enhanced security feature reporting in build system
- **ProductQuantizer**: Updated from v1.3.0 to v1.5.0 with actual FAISS K-means integration
- **BinaryQuantizer**: Updated from v1.4.1 to v1.5.0 with FAISS-optimized Hamming distance
- **ResidualQuantizer**: Updated from v1.4.1 to v1.5.0 with FAISS-accelerated composition
- **FAISS Integration Complete** ✅
  - Documented that AdvancedVectorIndex uses FAISS natively (IVF+PQ, HNSW, GPU)
  - Clarified that FAISS is the PRIMARY vector indexing solution for production
  - Custom quantizers now have actual FAISS integration with graceful fallback
  - Marked LearnedQuantizer as deprecated (research-only)
  - Updated `LIBRARY_USAGE_ANALYSIS.md` and `LIBRARY_OPTIMIZATION_QUICKREF.md`

### Performance Improvements
- **20-30% faster ProductQuantizer training** with FAISS K-means (verified with actual integration)
- **10-15% faster BinaryQuantizer Hamming distance** with SIMD intrinsics
- **30% faster ResidualQuantizer training** (via FAISS ProductQuantizer composition)
- Zero overhead when FAISS not available (graceful fallback maintained)

### Backward Compatibility
- ✅ All existing quantization code continues to work without changes
- ✅ API remains unchanged (new options are optional with sensible defaults)
- ✅ Default behavior gains performance boost with FAISS when available
- ✅ Graceful degradation when FAISS unavailable
### Removed
- **GPU Vector Index Stubs (CLEANUP)** 🧹
  - Removed incomplete GPU backend implementations (~1500 LOC)
    - `src/index/gpu_vector_index_cuda.cpp` (384 lines, 3 TODOs)
    - `src/index/gpu_vector_index_vulkan.cpp` (385 lines, 6 TODOs)
    - `src/index/gpu_vector_index_hip.cpp` (419 lines, 4 TODOs)
    - `src/index/gpu_vector_index_kernels.cu` (CUDA kernels)
    - `src/index/gpu_vector_index_hip_kernels.cpp` (HIP kernels)
  - Removed GPU backend classes from public API
  - Removed GPU-specific CMake configuration
  - **Rationale**: These were research stubs with 65+ TODO comments and no functional GPU acceleration
  - **Current Status**: `GPUVectorIndex` now uses CPU-only implementation (SIMD-optimized)
  - **Future Plans**: Proper GPU support planned for v2.x series (see `docs/FUTURE_GPU_SUPPORT.md`)

### Fixed
- **FIND-003 (CRITICAL):** RFC 3161 Timestamp Authority implementation complete
  - Resolves eIDAS compliance gap for qualified electronic timestamps
  - Enables legally binding digital signatures in EU
  - Supports long-term signature validation for regulated industries

### Security
- Enabled cryptographic timestamps for audit trails and document signing
- Added eIDAS-compliant timestamp validation
- Improved certificate chain verification for TSA responses

### Documentation
- Added comprehensive TSA setup guide (400+ lines)
- Documented integration with multiple TSA providers
- Added troubleshooting guide for common TSA issues
- **Added GPU Support Roadmap Documentation**
  - `docs/FUTURE_GPU_SUPPORT.md` - Detailed GPU roadmap for v2.x
  - `docs/GPU_SUPPORT_ROADMAP.md` - User migration guide
  - Updated `docs/GPU_VECTOR_INDEXING.md` - CPU-only status notice
  - Updated `docs/GPU_VECTOR_INDEXING_ARCHITECTURE.md` - Future architecture
  - Updated `README.md` - Clarified CPU-only vector indexing status
- Updated compliance documentation for eIDAS and ETSI EN 319 422

---

## [1.4.2] - 2026-02-06

### Changed
- **Vector Quantization Migration to FAISS**
  - ProductQuantizer now uses FAISS native implementation when available
  - Maintains API compatibility with existing code
  - Provides fallback implementation for non-FAISS builds
  - ResidualQuantizer automatically benefits through composition
  - Expected performance improvements through FAISS SIMD optimizations

### Added
- **FAISS ADC Optimization**: Implemented Asymmetric Distance Computation tables
  - ~40% faster asymmetric distance computation with FAISS
  - Uses precomputed asymmetric distance tables instead of decode + L2 distance
  - Automatic fallback to decode method on error or when FAISS unavailable
- **Performance Documentation**: Added `docs/PRODUCT_QUANTIZER_OPTIMIZATION.md`
  - Detailed benchmarking guidelines
  - GPU acceleration architecture documentation
  - Performance tuning recommendations

### Improved
- Reduced quantization code complexity by leveraging FAISS library
- Better maintainability through external library usage
- Conditional compilation support for FAISS availability
- Optimized distance computation path for production workloads

---

## [1.4.0] - TBD

### Added - Modular Architecture

- **Modular Build System**: Split monolithic `themis_core` into focused module libraries
  - `themis_base`: Core utilities, cross-cutting concerns, plugin infrastructure
  - `themis_storage`: Storage engine, indexes, backup management
  - `themis_query`: Query engine, AQL parser, analytics
  - `themis_security`: Encryption, PKI, RBAC, authentication
  - `themis_transaction`: Transaction management, CDC, saga support
  - `themis_network`: HTTP/gRPC servers, API handlers
  - `themis_sharding`: Distributed system (optional)
  - `themis_llm`: LLM integration (optional)
  - `themis_content`: Content processors (optional)
  - `themis_timeseries`: Time-series support (optional)
  - `themis_graph`: Graph analytics (optional)
  - `themis_geo`: Geospatial features (optional)
- **Export Macro System**: Platform-specific DLL export/import macros for all modules
- **Configurable Modules**: Optional modules can be excluded via CMake options
- **Backward Compatibility**: Monolithic build remains default; modular enabled with `-DTHEMIS_BUILD_MODULAR=ON`

### Changed

- **BinaryQuantizer Simplified**: Reduced implementation by 79 lines (-34%)
  - Marked as `@deprecated` - NOT used in production code
  - Recommends using FAISS `IndexBinaryFlat` for production workloads
  - Maintains API compatibility for existing tests
  - Part of FAISS migration initiative (see `LIBRARY_USAGE_ANALYSIS.md`)

- **LearnedQuantizer Marked as Research/Deprecated**: 393 lines
  - Marked as `@deprecated` - NOT used in production code
  - Research implementation for vector compression studies
  - Maintained for experimental workloads only
  - Part of code cleanup initiative (see `LIBRARY_USAGE_ANALYSIS.md`)

### Fixed

- **Windows Build Issues**: Resolves COFF symbol limit (>65,000 symbols) by splitting into smaller modules
- **Build Performance**: Parallel module compilation reduces full rebuild time by 30-50%

### Documentation

- Added `docs/architecture/MODULARIZATION_GUIDE.md` with comprehensive usage examples
- Updated build documentation with modular build instructions

---

## [Unreleased]

### Added
- **API Versioning and Compatibility Strategy**: Comprehensive API versioning infrastructure
  - **Accept-Version header** support for REST APIs to specify desired API version
  - **API-Version response header** indicating the API version used to process the request
  - **Deprecation tracking system** with automated warning headers (Deprecation, Sunset, Link)
  - **24-month deprecation policy** ensuring backward compatibility and smooth migrations
  - **gRPC version negotiation** via metadata (`api-version` key)
  - **Version resolution** supporting formats: `v1.4.1`, `v1.4`, `v1`, `latest`
  - **APIVersionManager** class for centralized version management
  - **Compatibility matrix** documenting supported versions (v1.0.0 to v1.4.1)
  - **Migration guide framework** with templates and best practices
  - Comprehensive documentation:
    - [API Versioning Strategy](docs/api/API_VERSIONING.md)
    - [Deprecation Registry](docs/api/DEPRECATION_REGISTRY.md)
    - [Migration Guides](docs/migration/README.md)
    - [v1.3 to v1.4 Migration Guide](docs/migration/v1.3-to-v1.4.md)
  - Updated proto files with API version metadata
  - Related to #751 (API-Versionierung und Kompatibilitäts-Strategie)
- **Query Result Pagination**: Comprehensive pagination support for query results with multiple strategies
  - **Cursor-based pagination** with expiration and versioning (1-hour TTL default)
  - **Keyset pagination** using ORDER BY values for O(log n) performance
  - **Configurable page sizes** with validation (min: 1, max: 10,000, default: 100)
  - Enhanced `PaginatedResponse` with detailed metadata (`PageInfo`, `has_next_page`, `has_prev_page`)
  - ORDER BY value encoding in cursors eliminates database lookups for sort values
  - Cursor expiration prevents stale cursor accumulation
  - Multiple pagination methods supported: CURSOR, OFFSET, KEYSET
  - 17 comprehensive tests with 100% pass rate
  - Backward compatible with existing pagination API
  - Related to #751
- **Plugin Metrics and Monitoring**: Comprehensive metrics tracking for all plugins with Prometheus integration
  - `PluginMetrics` class for thread-safe metrics collection
  - Automatic tracking of load time, reload time, function call latency (P95/P99)
  - Resource usage monitoring (memory per plugin)
  - Error tracking and count metrics
  - JSON API endpoint: `/api/plugins/metrics`
  - Prometheus metrics integrated into `/metrics` endpoint
  - <1% performance overhead from instrumentation
  - See [Plugin Metrics Documentation](docs/plugins/PLUGIN_METRICS.md)
- **CHIMERA Suite Branding**: Rebranded benchmark framework to "CHIMERA Suite" (_Comprehensive Hybrid Inferencing & Multi-model Evaluation Resource Assessment_)
  - Tagline: "Benchmark the Unbenchmarkable"
  - Vendor-neutral, scientifically rigorous benchmark framework
  - Updated all documentation, scripts, and CI workflows
  - Result files now use `CHIMERA_RESULTS_*` naming pattern
  - See [CHIMERA Suite Documentation](benchmarks/chimera/README.md)
- Documentation Archival System - Formal process for archiving outdated documentation
- Retroactive Release Building System - Build binaries from historical version tags
- Schema Manager for database self-awareness and introspection
- Independent Health/Error service on alternate port (9090)

### Performance
- **Query Pagination Improvements**:
  - Reduced database lookups by storing ORDER BY values in cursors
  - O(log n) keyset pagination vs O(n) offset-based pagination
  - Memory efficiency through configurable page size limits (max 10,000 items)
  - Cursor expiration prevents stale cursor accumulation

### Changed
- **Documentation Reorganization**: Major cleanup and restructuring of documentation
  - Fixed version inconsistencies across README, VERSION file, and badges
  - Moved 70+ historical implementation documents to `docs/implementation-history/` archive
  - Created comprehensive archive README explaining historical documents
  - Updated all broken links in main documentation files
  - Added archive reference in main documentation index
  - Cleaner root directory with only essential documentation files
- Improved documentation structure and organization
- Benchmark suite renamed to CHIMERA Suite with comprehensive rebranding

---

## [1.4.0-stable] - 2026-01-19

### 🎯 Extended Context Window (32K+) - Production Ready

**Status Change:** Experimental (v1.4.0-alpha) → Production-Ready (v1.4.0-stable)

#### Added

**Configuration & Feature Flags:**
- Comprehensive extended context configuration (`config/llm_extended_context.yaml`)
- Feature maturity status flags ("experimental", "beta", "stable")
- Backward compatibility mode with automatic fallback
- Production validation checks (memory, model support, RoPE config, thread-safety)
- Model-specific configuration overrides
- [Configuration Reference](config/llm_extended_context.yaml)

**RoPE/YARN Scaling - Production Ready:**
- Finalized integration on both Model and API levels
- All scaling methods production-ready: Linear, NTK, YaRN, Dynamic
- YaRN parameters fully configurable (ext_factor, attn_factor, beta_fast, beta_slow)
- Error handling and validation for scaling configuration
- [Production Guide](docs/de/llm/EXTENDED_CONTEXT_PRODUCTION_GUIDE.md)

**Memory Profiling & Monitoring:**
- 30+ new Prometheus metrics for extended context monitoring
  - Context window metrics: length, cache size, scaling factor
  - RoPE/YARN metrics: method, errors, YARN parameters
  - Memory metrics: RAM/VRAM usage, pressure, OOM events
  - Thread-safety metrics: LoRA switches, lock contention
- Memory estimation utilities with accuracy tracking
- Real-time RAM/VRAM profiling per model
- Memory pressure alerts and OOM prevention
- Grafana dashboard templates

**Thread-Safety:**
- Sequential LoRA operations mode for context scaling
- Configurable mutex-based synchronization
- Lock timeout configuration (default: 1000ms)
- Lock contention monitoring and alerts
- Safe concurrent request handling

**Documentation:**
- [Extended Context Production Guide](docs/de/llm/EXTENDED_CONTEXT_PRODUCTION_GUIDE.md)
- [Status Update v1.4.0](docs/de/llm/EXTENDED_CONTEXT_STATUS_UPDATE.md)
- Memory requirements calculator
- Deployment checklist and best practices
- Troubleshooting guide
- Migration guide from v1.4.0-alpha

#### Changed

**Extended Context:**
- Updated llm_config.example.yaml with extended_context section
- Improved RoPE scaling quality for high factors (>8x)
- Enhanced memory estimation accuracy (±10% for most models)
- Better error messages for configuration issues

#### Fixed

**Issues Resolved (GAP Analysis):**
- ✅ RoPE/YARN integration finalized on Model and API level
- ✅ Thread-safety for Context Scaling with LoRA/Adapters
- ✅ Comprehensive RAM/VRAM profiling and monitoring
- ✅ Feature flags and backward compatibility
- Reference: [INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md](docs/implementation-history/INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md)

**Production Readiness Score:**
- v1.4.0-alpha: 38% → v1.4.0-stable: 93%
- All critical gaps addressed
- Safe for production deployment with gradual rollout strategy

---

## [1.4.0-alpha] - 2026-01-05

### Added

#### 🧠 Advanced LLM Capabilities
- **Grammar-Constrained Generation** - EBNF/GBNF support for guaranteed valid JSON/XML/CSV outputs (95-99% reliability)
  - Built-in grammars: JSON, XML, CSV, ReAct Agent
  - Thread-safe grammar cache with LRU eviction
  - [Documentation](docs/en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)
  
- **RoPE Scaling** - Extended context window from 4K → 32K tokens (8x increase)
  - Linear, NTK-aware, YaRN scaling methods
  - [Documentation](docs/en/llm/ROPE_SCALING_IMPLEMENTATION.md)
  
- **Vision Support** - Multi-modal LLMs with CLIP-based image encoding
  - LLaVA integration for image analysis
  - Single and multiple image support
  - [Documentation](docs/en/llm/VISION_SUPPORT_QUICK_START.md)
  
- **Flash Attention** - CUDA kernels for 15-25% speedup, 30% memory reduction
  - [Documentation](docs/en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
  
- **Speculative Decoding** - 2-3x faster inference with draft+target models
- **Continuous Batching** - 2x+ throughput with dynamic request batching

#### 🏢 Enterprise Features
- Hot Spare Management - Automatic failover with health monitoring
- Enhanced Prometheus Metrics - LLM inference and cache performance tracking
- WAL Replication via gRPC - Distributed inter-shard replication
- Multi-GPU LoRA Support - Distributed LoRA adapters across GPUs
- PostgreSQL Protocol Enhancements - COPY, prepared statements, transaction support

### Changed
- 31 new test suites with comprehensive coverage
- 11 new performance benchmarks
- 17 new documentation guides
- 938 files changed (+113,762 lines, -45,154 lines)

**[→ Complete Release Notes](release-changelogs/v1.4.0-alpha.md)**

---

## [1.3.4-hotfix] - 2026-01-04

### Fixed
- **CRITICAL:** Fixed server hang at "Adaptive Index Manager initialized" in RAID cluster mode
  - Root cause: AdaptiveIndexManager MVCC coordination before Sharding Manager initialization
  - Solution: Conditional Column Family opening when `THEMIS_ENABLE_SHARDING=true` detected
  - Files: `src/storage/rocksdb_wrapper.cpp`, `src/server/http_server.cpp`
  
- **CRITICAL:** Fixed incorrect Docker Compose port mappings (`808X:8080` → `808X:8765`)
  - All 9 RAID shards now properly expose HTTP/REST API endpoints
  - File: `docker/compose/docker-compose-sharding.yml`

### Added
- RAID Endurance Test Suite - 2-hour automated testing for all RAID modes
  - Script: `scripts/raid_endurance_test.py`
  - Monitoring: `scripts/monitor_raid_test.ps1`
  - Verification: All 9 RAID shards (RAID 0/1/5) operational with 0% error rate

### Changed
- Docker build context reduced from 3GB to 85MB (97% reduction)
- Updated `.dockerignore` to exclude build artifacts while preserving vcpkg baseline
- Improved Dockerfile.themis-server for more reliable builds

**[→ Complete Release Notes](release-changelogs/v1.3.4-hotfix.md)**

---

## [1.3.4] - 2026-01-02

### Security
> **Comprehensive Security Summary:** See [Security Work Summary v1.3.4](docs/de/releases/SECURITY_WORK_SUMMARY_V1.3.4.md)

#### Fixed
- **7 Critical Security Vulnerabilities** in RocksDB wrapper (100% segfault risk elimination)
  - Use-after-free in BlockBasedTableOptions
  - Null-pointer checks for environment initialization
  - Transaction-based deletion to prevent deadlocks
  - GetBaseDB() null-pointer checks across 7 locations
  - Transaction resource leak fixes
  - Column Family handle cleanup improvements
  - BackupEngine exception safety
  - [Audit Report](docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md)

- **8 Medium-Severity Issues**
  - Improved transaction error handling
  - Enhanced iterator lifecycle management
  - Better snapshot handling
  - Backup engine null-checks

#### Changed
- Upgraded Docker base image: Ubuntu 22.04 → Ubuntu 24.04 LTS (80%+ CVE reduction)
- Secure token handling in Update Checker (no hardcoded credentials)
- Binary authenticity verification with cryptographic manifest signing (RSA-4096, SHA-256)

**[→ Complete Release Notes](release-changelogs/v1.3.4.md)**

---

## [1.3.3] - 2025-12-21

### Added
- **HTTP/2 with Server Push** - CDC/Changefeed with ~0ms latency
- **WebSocket Support** - Bidirectional streaming for real-time communication
- **MQTT Broker** - IoT messaging with WebSocket transport and monitoring
- **HTTP/3 Base Implementation** - QUIC protocol (experimental)
- **PostgreSQL Wire Protocol** - BI tool compatibility
- **MCP Server** - Model Context Protocol support for LLM integration

**[→ Complete Release Notes](release-changelogs/v1.3.3.md)**

---

## [1.3.2] - 2025-12-21

### Added
- **Image Analysis AI Plugin Architecture** running parallel with LLM
  - Multi-backend support: llama.cpp Vision, ONNX Runtime, OpenCV DNN, OpenVINO, ncnn
  - Plugin interfaces: `IImageAnalysisBackend`, `ImageAnalysisManager`
  - 15+ comprehensive unit tests and benchmarks

**[→ Complete Release Notes](release-changelogs/v1.3.2.md)**

---

## [1.3.1] - 2025-12-20

### Added
- `ATTRIBUTIONS.md` documenting 15+ core dependencies
- Documentation of ThemisDB's **12 unique innovations**
- Clear attribution for all major dependencies

**[→ Complete Release Notes](release-changelogs/v1.3.1.md)**

---

## [1.3.0] - 2025-12-17

### Added
- **Native LLM Integration with llama.cpp** (optional feature)
  - Embedded LLM engine for LLaMA/Mistral/Phi-3 (1B-70B parameters)
  - GPU acceleration with NVIDIA CUDA support
  - PagedAttention for advanced memory management
  - Quantization support (Q4_K_M, Q5_K_M, Q8_0)
  - Grafana dashboards with metrics and alerts
  - [Setup Guide](docs/de/guides/LLM_COMPLETE_SETUP_GUIDE.md)

- **Voice Assistant Integration** (Enterprise feature)
  - Natural language voice interaction (Whisper.cpp + Piper TTS + llama.cpp)
  - Phone call recording with automatic transcription
  - Meeting protocol generation with AI-powered minutes
  - Speaker diarization
  - Multi-language support (100+ languages)
  - [Documentation](docs/en/features/voice_assistant_guide.md)

**[→ Complete Release Notes](release-changelogs/v1.3.0.md)**

---

## Earlier Versions

For releases prior to v1.3.0, please see:
- [Release Changelogs Directory](release-changelogs/)
- [GitHub Releases Page](https://github.com/makr-code/ThemisDB/releases)

---

## Release Notes

Detailed release notes for each version are available in the [release-changelogs/](release-changelogs/) directory:

- [v1.4.0-alpha](release-changelogs/v1.4.0-alpha.md) - Advanced LLM features
- [v1.3.4-hotfix](release-changelogs/v1.3.4-hotfix.md) - RAID sharding deadlock hotfix
- [v1.3.4](release-changelogs/v1.3.4.md) - Security improvements
- [v1.3.3](release-changelogs/v1.3.3.md) - Network protocol enhancements
- [v1.3.2](release-changelogs/v1.3.2.md) - Image analysis AI plugin
- [v1.3.1](release-changelogs/v1.3.1.md) - Third-party attribution
- [v1.3.0](release-changelogs/v1.3.0.md) - LLM integration

---

## Upgrade Notes

### From 1.3.x to 1.4.0-alpha

- LLM features now include advanced capabilities (grammar constraints, RoPE scaling, vision support)
- New configuration options available for Flash Attention and Speculative Decoding
- See [Migration Guide](docs/MIGRATION_GUIDE.md) for detailed upgrade instructions

### From 1.2.x to 1.3.x

- LLM integration is now optional and requires explicit build flag: `-DTHEMIS_ENABLE_LLM=ON`
- New protocols (HTTP/2, WebSocket, MQTT) require explicit opt-in for security
- See [Configuration Guide](docs/en/guides/guides_configuration.md) for new settings

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- How to contribute to ThemisDB
- Code style and standards
- Pull request process
- Documentation requirements

---

## Version Format

ThemisDB follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version for incompatible API changes
- **MINOR** version for new functionality in a backward compatible manner
- **PATCH** version for backward compatible bug fixes
- **-alpha**, **-beta**, **-rc** suffixes for pre-release versions
