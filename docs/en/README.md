# ThemisDB Documentation Index

> **📝 Note on Documentation Language**  
> This is a translation of the ThemisDB documentation. The **authoritative and most up-to-date documentation is maintained in German** (`docs/de/`).  
> Translations may lag behind the German version. For the latest information, please refer to the [German documentation](../de/README.md).

**Last Updated:** April 2026  
**Version:** 1.8.0-rc1  
**Type:** Documentation Index  
**Language:** English (Translation)

---

## Quick Navigation (maintained entry paths)

These links represent the currently maintained entry points:

- Root entry: [../README.md](../README.md)
- Master index: [../00_DOCUMENTATION_INDEX.md](../00_DOCUMENTATION_INDEX.md)
- Role hub: [../DOCUMENTATION_HUB.md](../DOCUMENTATION_HUB.md)
- Category index: [../CATEGORY_INDEX.md](../CATEGORY_INDEX.md)
- Structure rules: [../DOCS_ORGANIZATION_PLAN.md](../DOCS_ORGANIZATION_PLAN.md)
- Docs PR policy: [../governance/DOCS_PR_POLICY.md](../governance/DOCS_PR_POLICY.md)

Historical reports were moved out of the root and are grouped here:

- Summaries: [../implementation-history/summaries/README.md](../implementation-history/summaries/README.md)
- Phases: [../implementation-history/phases/README.md](../implementation-history/phases/README.md)
- Reviews: [../implementation-history/reviews/README.md](../implementation-history/reviews/README.md)
- Status reports: [../implementation-history/status-reports/README.md](../implementation-history/status-reports/README.md)

---

## 🚀 NEW in v1.4.0-alpha: Advanced LLM Features ✅

**AI directly in the database with advanced capabilities - no external API costs!**

### Advanced LLM Features (v1.4.0-alpha)

- 📝 **Grammar-Constrained Generation** - EBNF/GBNF support for guaranteed valid outputs (95-99% reliability vs 60-70%)
  - Built-in grammars: JSON, XML, CSV, ReAct Agent
  - Thread-safe grammar cache with LRU eviction
  - Zero post-processing required
- 🔭 **RoPE Scaling** - Extended context window from 4K → 32K tokens (8x increase)
  - Linear, NTK-aware, YaRN scaling methods
  - Process entire research papers and codebases
- 🖼️ **Vision Support** - Multi-modal LLMs with CLIP-based image encoding
  - LLaVA integration for image analysis
  - Single and multiple image support
- ⚡ **Flash Attention** - CUDA kernels for 15-25% speedup, 30% memory reduction
  - Optimized attention mechanism
  - Backward pass for training support
- 🎯 **Speculative Decoding** - 2-3x faster inference with draft+target models
- 🔄 **Continuous Batching** - 2x+ throughput with dynamic request batching

### Documentation (v1.4.0-alpha)

- **[Grammar-Constrained Generation](llm/GRAMMAR_CONSTRAINED_GENERATION.md)** ⭐ **v1.4.0-alpha**
  - EBNF/GBNF grammar support
  - Built-in and custom grammars
  - Usage examples and best practices

- **[RoPE Scaling Implementation](llm/ROPE_SCALING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Extended context windows (4K→32K)
  - Scaling methods comparison
  - Configuration guide

- **[Vision Support Quick Start](llm/VISION_SUPPORT_QUICK_START.md)** ⭐ **v1.4.0-alpha**
  - Multi-modal LLM setup
  - CLIP model integration
  - Image analysis examples

- **[Flash Attention Implementation](llm/FLASH_ATTENTION_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - CUDA kernel optimization
  - Performance benchmarks
  - Configuration guide

- **[Speculative Decoding](llm/SPECULATIVE_DECODING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Draft+target model pairing
  - 2-3x speedup guide
  - Model recommendations

- **[Continuous Batching](llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md)** ⭐ **v1.4.0-alpha**
  - Dynamic batching configuration
  - Throughput optimization
  - Token budget management

## 🚀 LLM Integration (Optional Feature) - v1.3.0 Base

> **Important**: LLM Integration is an **optional feature** in v1.3.0+:
> - Requires build flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Requires external dependency: llama.cpp (clone separately)
> - See [Build Guide](guides/guides_build_strategy.md) for setup instructions

ThemisDB can be extended as the first multi-model database with an **embedded LLM engine**:

### Core Features (v1.3.0)

- 🧠 **Embedded llama.cpp** - SLMs/LLMs (1B-70B parameters) directly on GPU ✅
- ⚡ **GPU Acceleration** - Significant speedup with NVIDIA CUDA support ✅
- 💾 **PagedAttention** - Optimized memory management ✅
- 🎯 **Continuous Batching** - Multiple concurrent requests ✅
- 🔧 **Kernel Fusion** - CUDA kernels for additional speedup ✅
- 📊 **Production Monitoring** - Grafana/Prometheus integration ✅
- 🔌 **Plugin Architecture** - Extensible LLM backend system ✅
- 🌐 **RPC Framework** - Inter-shard communication for distributed LLM ops ✅
- 🖼️ **Image Analysis Plugins** - Multi-backend AI (llama.cpp Vision, ONNX CLIP, OpenCV DNN) ✅

### Network Protocol Enhancements (v1.3.0)

- 🌐 **HTTP/2 with Server Push** - CDC/Changefeed with proactive event delivery (~0ms latency) ✅
- 🔌 **WebSocket Support** - CDC streaming with bidirectional real-time communication ✅
- 📡 **MQTT Broker** - WebSocket transport, rate limiting, monitoring metrics ✅
- 🚀 **HTTP/3 Base** - QUIC-based implementation (ngtcp2 + nghttp3) 🚧
- 🐘 **PostgreSQL Wire Protocol** - SQL-to-Cypher translation for BI tool compatibility ✅
- 🤖 **MCP Server** - Model Context Protocol with cross-platform support ✅

### Performance Metrics (with GPU)

- **Significant speedup** with GPU vs CPU-only
- **Memory savings** with PagedAttention
- **Additional optimization** with kernel fusion
- **Comprehensive test coverage** with unit tests

### GPU Tier Recommendations

| GPU Tier | Hardware | Model | Use Case | Cost/1M Tokens | vs. GPT-4 |
|----------|----------|-------|----------|----------------|-----------|
| **Entry** | RTX 4060 Ti (16GB) | Phi-3-Mini (3.8B) | FAQ, simple RAG | €0.02 | **1500x cheaper** |
| **Mid-Range** | RTX 4090 (24GB) | Mistral-7B | Production RAG | €0.05 | **600x cheaper** |
| **High-End** | A100 (80GB) | Llama-3-70B | Enterprise Scale | €0.15 | **200x cheaper** |

**Break-Even vs. Hyperscaler:** 2-7 months depending on hardware tier

### Documentation (v1.3.0)

- **[GPU Inference Guide](llm/GPU_INFERENCE_GUIDE.md)** ⭐ **v1.3.0**
  - CUDA setup and configuration
  - Performance tuning
  - Troubleshooting

- **[Quantization Guide](llm/QUANTIZATION_GUIDE.md)** ⭐ **v1.3.0**
  - Q4_K_M, Q5_K_M, Q8_0 formats
  - Memory vs. quality trade-offs
  - Best practices

- **[Performance Benchmarks](llm/PERFORMANCE_BENCHMARKS.md)** ⭐ **v1.3.0**
  - GPU vs. CPU comparisons
  - Throughput measurements
  - Latency analysis

- **[Deployment Guide](llm/DEPLOYMENT_GUIDE.md)** ⭐ **v1.3.0**
  - Docker with GPU support
  - Kubernetes deployment
  - Production best practices

- **[RPC Framework](plugins/RPC_PLUGIN_ARCHITECTURE.md)** ⭐ **v1.3.0**
  - Inter-shard communication
  - TLS/mTLS security
  - Snapshot/blob transfer

- **[GPU Tier Analysis & Hyperscaler Comparison](llm/GPU_TIER_ANALYSIS_HYPERSCALER_COMPARISON.md)**
  - SLM/LLM performance on entry/mid/high-end GPUs
  - TCO analysis over 3 years
  - ROI calculation vs. AWS/Azure/GCP

- **[All LLM Documentation](llm/README.md)** - Complete index (31 guides)

---

## 📁 Documentation Structure (Newly Organized)

The documentation has been restructured for better clarity:

**Root Documents (essentials only):**
- `README.md` - Main documentation
- `index.md` - Documentation index
- `glossary.md` - Terminology

**Organized Folders:**
- `aql/` - **AQL Grammar (EBNF)** ⭐ **v1.3.0**
- `build/` - Build system documentation (BUILD-SYSTEM.md, BUILDGUIDE.md, etc.)
- `development/` - Development documentation (IMPLEMENTATION-*.md, CODE_REVIEW-*.md)
- `guides/` - User and developer guides (RAILWAY_COMPLETE_GUIDE.md, etc.)
- `architecture/` - Architecture documentation (ARCHITECTURE_OVERVIEW.md, etc.)
- `stakeholder/` - Stakeholder documentation
- `releases/` - Release notes (v1.3.0.md, v1.2.0.md, v1.1.0.md, etc.)
- `llm/` - **LLM & AI Integration** ⭐ **v1.3.0 RELEASED**
- `plugins/` - **RPC Framework** ⭐ **v1.3.0**
- `archive/` - Old/historical documentation

---

> **🔮 COMING SOON - v1.1.0 Optimization Release (Q1 2026):**
> 
> **Focus:** Better utilize existing libraries + vLLM co-location  
> **Highlights:**
> - ✅ RocksDB TTL, Incremental Backup, Stats (no new lib!)
> - ✅ TBB Parallel Sort, Concurrent Containers (no new lib!)
> - ✅ Arrow Parquet Export (no new lib!)
> - ✅ **CUDA as core** (when GPU available, NOT Enterprise!)
> - ✅ **🆕 ThemisDB + vLLM Synergy** (optimized CPU/GPU/RAM coordination)
> - ✅ mimalloc (only new dependency, 20-40% memory boost)
> 
> **Engineering:** 9-11 weeks | **Impact:** 3-10x performance  
> **Details:** [v1.1.0 Variant Strategy](analysis/VARIANT_STRATEGY_v1.1.0.md)

> **🚀 PLANNED - v1.2.0 Enterprise Features (Q2 2026):**
> 
> **Focus:** vLLM AI Support (LoRA), Geo-Spatial (PostGIS), IoT/Timescale  
> **Highlights:**
> - ✅ **LoRA Manager** - Multi-tenant LoRA serving (HuggingFace PEFT)
> - ✅ **FAISS Advanced** - IVF+PQ vector search (already integrated, expand!)
> - ✅ **GEOS + PROJ** - PostGIS compatibility (topology + geography)
> - ✅ **Hypertables** - TimescaleDB-compatible via RocksDB CF (code only!)
> - ✅ **cuSpatial** - GPU geo ops (optional, uses Arrow + CUDA)
> 
> **Engineering:** 12-16 weeks | **Impact:** PostGIS + LoRA + TimescaleDB compatibility  
> **Details:** [Enterprise Features Strategy](analysis/ENTERPRISE_FEATURES_STRATEGY.md)

---

## 📚 Main Documentation

### Overview Documents
- **[Changelog](../CHANGELOG.md)** - Complete version history (v1.8.0-rc1, v1.5.0, v1.4.0, v1.3.0, …)
- **[🆕 Roadmap v2.0](../../roadmap.md)** - **UPDATED:** Aggregated roadmap across all 46 modules
- **[Architecture Overview](architecture/ARCHITECTURE_OVERVIEW.md)** - Complete system architecture with diagrams
- **[Source Code Changes v1.0](development/SOURCE_CODE_CHANGES_v1.0.md)** - Detailed source code documentation (191 files, 26 modules)
- **[Features List](features/features_overview.md)** - Complete feature overview with status

---

## 🎯 By Target Audience

### For Stakeholders & Management
- **[Themis Status Report 2025](reports/themis_sachstandsbericht_2025.md)** - Executive summary, status v1.0.1
- **[🆕 v1.1.0 Variant Strategy](analysis/VARIANT_STRATEGY_v1.1.0.md)** - **Q1 2026:** Optimization strategy with vLLM co-location (9-11 weeks, 1 new lib)
- **[🆕 v1.2.0 Enterprise Features](analysis/ENTERPRISE_FEATURES_STRATEGY.md)** - **Q2 2026:** vLLM AI (LoRA), geo-spatial (PostGIS), IoT/Timescale (12-16 weeks, 3 new libs)
- ~~Project cost estimation & total value~~ - 🔒 Confidential (available to licensed customers only)
- **[Release Strategy Audit](../RELEASE_STRATEGY_AUDIT.md)** - SLSA compliance, SBOM (8.5/10 rating)
- **[Release & Benchmarking Summary](../RELEASE_AND_BENCHMARKING_SESSION_SUMMARY.md)** - v1.0.1 session report

### For Developers
- **[Development Summary](development/DEVELOPMENT_SUMMARY.md)** - Development status v1.0.1
- **[🆕 External Libraries Analysis](analysis/EXTERNAL_LIBRARIES_FEATURES_ANALYSIS.md)** - **NEW:** Feature gap analysis (RocksDB, TBB, CUDA, Arrow)
- **[🆕 Library Interactions](analysis/LIBRARY_INTERACTIONS_AND_EXTENSIONS.md)** - **NEW:** Interactions & additional libraries
- **[Source Code Audit](development/SOURCE_CODE_AUDIT.md)** - Code analysis (132 headers, 124 sources, 90,829 LOC)
- **[Documentation Index](DOCUMENTATION_INDEX.md)** - Complete documentation index with module mapping
- **[Documentation Verification](reports/documentation_verification_report.md)** - Verification documentation ↔ code

### For DevOps & Operations
- **[Operations Runbook](guides/guides_operations_runbook.md)** - Daily operations
- **[Deployment Guide](deployment/README.md)** - Deployment strategies
- **[Build Strategy](guides/guides_build_strategy.md)** - Build toolchain
- **[Docker Guide](../README.docker.md)** - Container deployment

### For Security & Compliance
- **[Compliance Dashboard](compliance/compliance_dashboard.md)** - Overview of all compliance activities
- **[Security Audit Report](reports/SECURITY_AUDIT_REPORT.md)** - Completed security audit
- **[Compliance Full Checklist](compliance/compliance_full_checklist.md)** - BSI C5, ISO 27001, GDPR, eIDAS, SOC 2
- **[Security Policy](../SECURITY.md)** - Vulnerability disclosure
- **[Incident Response Plan](security/security_incident_response.md)** - Emergency plan (BSI IT-Grundschutz & NIST CSF)
- **[SBOM Documentation](security/security_sbom.md)** - Software Bill of Materials (CycloneDX 1.4)
- **[DPIA](compliance/compliance_dpia.md)** - Data protection impact assessment (GDPR Art. 35)
- **[BCP/DRP](compliance/compliance_bcp_drp.md)** - Business continuity (ISO 22301 & NIS2)

---

## 🏗️ By Architecture Layer

### Query & Analytics Layer
- **[AQL Documentation](aql/README.md)** - Advanced Query Language (parser, optimizer, 240K LOC)
- **[Query Module](query/README.md)** - Query engine, execution
- **[Analytics Module](analytics/README.md)** - OLAP engine (CUBE, ROLLUP), CEP, process mining (57K LOC)
- **[Search Documentation](search/README.md)** - Fulltext (BM25), vector, hybrid search

### Storage & Index Layer
- **[Storage Module](storage/README.md)** - RocksDB wrapper, LSM-tree, MVCC (76K LOC)
- **[Index Module](index/README.md)** - Vector HNSW, graph, secondary, spatial (400K LOC)
- **[Cache Module](cache/README.md)** - Semantic cache, result cache
- **[Timeseries Module](timeseries/README.md)** - Gorilla compression, aggregates (39K LOC)

### Distribution & Scaling Layer
- **[Sharding Module](sharding/README.md)** - VCC-URN sharding, auto-rebalancing, gossip (300K LOC)
- **[Replication Module](replication/README.md)** - Leader-follower, multi-master CRDTs (12K LOC)
- **[Transaction Module](transaction/README.md)** - MVCC, SAGA patterns (42K LOC)

### Acceleration Layer
- **[GPU Acceleration Plan](performance/GPU_ACCELERATION_PLAN.md)** - 10 GPU backends (173K LOC)
  - CUDA, Vulkan, FAISS, DirectX, HIP, OpenCL, OneAPI, ZLUDA

### Content & Data Processing
- **[Content Module](content/README.md)** - 15 file format processors (256K LOC)
- **[CDC Module](cdc/README.md)** - Change data capture, changefeed
- **[Geo Module](geo/README.md)** - Spatial operations, plugin system

### Server & API Layer
- **[Server Module](server/README.md)** - HTTP server, 21 API handlers (164K LOC)
- **[HTTP API Reference](apis/HTTP_API_REFERENCE.md)** - **Complete HTTP endpoint documentation** ⭐
- **[API Documentation](api/README.md)** - REST API overview
- **[LLM Module](llm/README.md)** - LLM interaction store, prompt manager

### Security & Governance Layer
- **[Security Module](security/README.md)** - Field encryption, HSM/PKI, RBAC, Ranger (187K LOC)
- **[Governance Module](governance/README.md)** - Policy engine, data classification
- **[Auth Module](auth/README.md)** - JWT validation, multi-tenancy

---

## 🚀 Quick Start Guides

### Installation & Deployment
- **[Main README](../README.md)** - Project overview and quick start
- **[Deployment Guide](deployment/README.md)** - Deployment options
- **[Docker Guide](../README.docker.md)** - Container deployment
- **[QNAP Quickstart](../QNAP_QUICKSTART.md)** - ARM deployment

### Getting Started
- **[Architecture Overview](ARCHITECTURE_OVERVIEW.md)** - Understanding system architecture
- **[Features Overview](features/features_overview.md)** - Available features
- **[AQL Tutorial](aql/README.md)** - Learning the query language

---

## 📖 Reference Documentation

### Client SDKs
- **[SDK Audit](clients/clients_sdk_audit.md)** - Overview of all 7 SDKs
- **[Python SDK](clients/python_sdk_quickstart.md)** - Python client
- **[JavaScript SDK](clients/javascript_sdk_quickstart.md)** - Node.js/browser client
- **[Rust SDK](clients/rust_sdk_quickstart.md)** - Rust client
- **[Go SDK](clients/go_sdk_quickstart.md)** - Go client
- **[Java SDK](clients/java_sdk_quickstart.md)** - Java client
- **[C# SDK](clients/csharp_sdk_quickstart.md)** - .NET client
- **[Swift SDK](clients/swift_sdk_quickstart.md)** - iOS/macOS client

### Data Import/Export
- **[Exporters](exporters/README.md)** - Data export
  - **[JSONL LLM Exporter](exporters/JSONL_LLM_EXPORTER.md)** - LLM training data export
- **[Importers](importers/README.md)** - Data import
  - **[PostgreSQL Importer](importers/POSTGRES_IMPORTER.md)** - PostgreSQL migration

### Plugin Development
- **[Plugins](plugins/README.md)** - Plugin system
- **[Plugin Security](plugins/PLUGIN_SECURITY.md)** - Security & sandboxing
- **[Plugin Migration](plugins/PLUGIN_MIGRATION.md)** - Migration guide

---

## 🔧 Administration & Operations

### Admin Tools
- **[Admin Tools](admin_tools/README.md)** - 7 WPF administration tools
- **[User Guide](admin_tools/user_guide.md)** - User manual
- **[Admin Guide](admin_tools/admin_guide.md)** - Administrator manual
- **[Feature Matrix](admin_tools/feature_matrix.md)** - Tool overview

### Operations Guides
- **[Operations Runbook](guides/guides_operations_runbook.md)** - Daily operations
- **[TLS Setup](guides/tls_setup.md)** - TLS/mTLS configuration
- **[Vault Integration](guides/vault.md)** - HashiCorp Vault setup
- **[RBAC Setup](guides/rbac.md)** - Access control configuration
- **[Code Quality](guides/code_quality.md)** - Code quality tools

### Performance & Monitoring
- **[Performance Tuning](performance/README.md)** - Performance optimization
- **[Benchmarks](performance/benchmarks.md)** - Performance benchmarks
- **[Memory Tuning](performance/memory_tuning.md)** - Memory optimization
- **[Observability](observability/README.md)** - Monitoring & metrics

---

## 📊 Reports & Status

### Development Reports
- **[Development Summary](development/DEVELOPMENT_SUMMARY.md)** - Current development status v1.0.1
- **[Audit Log](development/auditlog.md)** - Development audit log
- **[Implementation Status](development/implementation_status.md)** - Implementation status
- **[Priorities](development/priorities.md)** - Development priorities

### Status Reports
- **[Themis Status Report](reports/themis_sachstandsbericht_2025.md)** - Main status report v1.5
- **[Documentation Summary](reports/DOCUMENTATION_SUMMARY.md)** - Documentation overview
- **[Benchmark Audit](reports/BENCHMARK_AND_TEST_AUDIT.md)** - Test & benchmark status
- **[Security Audit](reports/SECURITY_AUDIT_REPORT.md)** - Security audit results

### Roadmap & Planning
- **[Roadmap Overview](roadmap/roadmap_overview.md)** - Development roadmap (complete 2026!)
- **[Features Priorities](features/features_priorities.md)** - Q1 2026 priorities
- **[Database Capabilities](reports/database_capabilities_roadmap.md)** - Capabilities roadmap

---

## 📦 Integration & Ingestion

### Data Ingestion
- **[Ingestion](ingestion/README.md)** - Data ingestion patterns
- **[VCC CLARA](../adapters/vcc_clara_ingestion/README.md)** - CLARA adapter
- **[VCC VERITAS](../adapters/vcc_veritas/README.md)** - VERITAS adapter
- **[VCC Base](../adapters/vcc_base/README.md)** - Base adapter framework

### Enterprise Integration
- **[Enterprise Features](enterprise/README.md)** - Rate limiting, load shedding
- **[Integration Analysis](reports/INTEGRATION_ANALYSIS.md)** - Legacy code integration

---

## 🔍 Source Code Documentation

### Module Documentation (src/)
All 26 modules with detailed documentation in [src/](src/README.md):

- **Acceleration** - GPU/CPU backends (173K LOC)
- **Analytics** - OLAP, CEP (57K LOC)
- **API** - GraphQL, geo hooks
- **Auth** - JWT validation
- **Cache** - Semantic cache
- **CDC** - Change data capture
- **Content** - 15 file processors (256K LOC)
- **Exporters** - Data export
- **Geo** - Spatial operations
- **Governance** - Policy engine
- **Importers** - Data import
- **Index** - Vector, graph, secondary (400K LOC)
- **LLM** - LLM integration
- **Network** - Wire protocol
- **Observability** - Metrics, tracing
- **Plugins** - Plugin system
- **Query** - AQL engine (240K LOC)
- **Replication** - Leader-follower, multi-master (12K LOC)
- **Security** - Encryption, RBAC (187K LOC)
- **Server** - HTTP, API handlers (164K LOC)
- **Sharding** - VCC-URN, gossip (300K LOC)
- **Storage** - RocksDB, MVCC (76K LOC)
- **Timeseries** - Gorilla compression (39K LOC)
- **Transaction** - MVCC, SAGA (42K LOC)
- **Updates** - Schema migration
- **Utils** - Utilities (120K LOC)

---

## 🎓 Additional Resources

### External Links
- **[GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)** - Community wiki
- **[GitHub Pages](https://makr-code.github.io/ThemisDB/)** - Online documentation
- **[PDF Documentation](https://makr-code.github.io/ThemisDB/themisdb-docs-complete.pdf)** - Complete documentation as PDF

### Benchmarking & Performance
- **[Benchmarks Suite](../benchmarks/README.md)** - Benchmark framework
- **[Docker Benchmarks](../benchmarks/DOCKER_COMPARATIVE_BENCHMARKS_README.md)** - Competitive benchmarks
- **[Hardware Constraints](../benchmarks/HARDWARE_CONSTRAINTS_README.md)** - Resource constraints testing

### Release Documentation
- **[v1.0.1 Release Notes](../CHANGELOG.md#101---2025-12-09)** - Latest release
- **[v1.0.0 Release Notes](../RELEASE_NOTES_v1.0.0.md)** - Production release
- **[Release Package Structure](../RELEASE_PACKAGE_STRUCTURE.md)** - Package organization

---

## 📝 Documentation Standards

### Format & Structure
- **Format**: Markdown (.md)
- **Encoding**: UTF-8
- **Line Endings**: LF (Unix-style)
- **Code Blocks**: Always specify language
- **Links**: Use relative paths

### Contributing
1. **Follow structure** - Place docs in appropriate subdirectory
2. **Link properly** - Use relative links to other documents
3. **Update README** - Update relevant README.md files
4. **Markdown style** - Follow [Style Guide](guides/styleguide.md)
5. **Keep current** - Update docs when features change

### Build Process
```powershell
# Install dependencies
pip install -r requirements-docs.txt

# Build documentation
.\build-docs.ps1

# Test locally
mkdocs serve
```

Documentation is automatically deployed to GitHub Pages on merge to main.

---

## 📞 Support & Community

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Wiki**: [GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)
- **Security**: [Security Policy](../SECURITY.md)

---

## 📊 Documentation Statistics

| Metric | Value |
|--------|-------|
| **Documentation Files** | 456+ |
| **Documentation Folders** | 71 |
| **Source Code LOC** | 90,829 |
| **Source Files** | 191 (.cpp) |
| **Header Files** | 132 (.h) |
| **Modules** | 26 directories |
| **Logical Components** | 16 |

---

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**License:** See [LICENSE](../LICENSE)
