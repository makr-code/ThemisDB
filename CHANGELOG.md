# Changelog

All notable changes to ThemisDB will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added
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
