# ThemisDB Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Documentation

---

Welcome to the ThemisDB documentation! This directory contains comprehensive documentation for all aspects of the ThemisDB multi-model database system.

## Source-Code Statistiken

| Metrik | Wert |
|--------|------|
| Header-Dateien | 132 |
| Source-Dateien | 124 |
| Lines of Code | ~91,000 |
| Module | 16 |
| Dokumentationsdateien | 363 |
| Dokumentationsordner | 30+ |

**Source-Code-Audit:** [SOURCE_CODE_AUDIT.md](development/SOURCE_CODE_AUDIT.md)

## Quick Navigation

### 📋 Overview
- **[FEATURES.md](../FEATURES.md)** - **Comprehensive features list** with production-ready status indicators (✅/🔧/📋)
- **[DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)** - Vollständiger Dokumentations-Index

### 🚀 Getting Started
- [Main README](../README.md) - Quick start guide and basic usage
- [Architecture Overview](architecture/README.md) - High-level system architecture  
- [Deployment Guide](deployment/README.md) - How to deploy ThemisDB
- [Operations Runbook](guides/guides_operations.md) - Day-to-day operations

### 📖 Core Documentation - Source Modules

#### Query & Analytics
- **[AQL Documentation](aql/README.md)** - AQL Parser, AST Nodes, Query Syntax
- **[Query Module](query/README.md)** - QueryEngine, Optimizer, Execution
- **[Analytics Module](analytics/README.md)** - OLAP, CEP, ColumnarStore
- **[Search Documentation](search/README.md)** - Fulltext, Vector, Hybrid Search

#### Storage & Indexing
- **[Storage Module](storage/README.md)** - RocksDB Wrapper, BaseEntity, BlobRedundancy
- **[Index Module](index/README.md)** - Secondary, Vector (HNSW), Graph Index
- **[Cache Module](cache/README.md)** - SemanticCache, ResultCache
- **[Timeseries Module](timeseries/README.md)** - TimeSeriesStore, Gorilla Compression

#### Distributed Systems
- **[Replication Module](replication/README.md)** - VectorClock, HLC, CRDTs, Multi-Master
- **[Sharding Module](sharding/README.md)** - Horizontal Scaling, Partitioning
- **[Transaction Module](transaction/README.md)** - TransactionManager, MVCC, Isolation

#### Content & Data
- **[Content Module](content/README.md)** - ContentManager, 16 Processors
- **[CDC Module](cdc/README.md)** - Changefeed, ChangeEvent, Long-Polling
- **[Geo Module](geo/README.md)** - ISpatialComputeBackend, Plugin System

#### Server & Integration
- **[Server Module](server/README.md)** - HttpServer, 12 API Handlers
- **[LLM Module](llm/README.md)** - LLMInteractionStore, PromptManager, CoT

#### Security & Governance
- **[Security Module](security/README.md)** - FieldEncryption, KeyProviders, RBAC
- **[Governance Module](governance/README.md)** - PolicyEngine, ClassificationProfile
- **[Compliance Documentation](compliance/README.md)** - BSI C5, ISO 27001, DSGVO

### 🛠️ Development

- **[Development Guides](development/)** - For contributors
  - [Development Overview](development/README.md)
  - [Development Audit Log](development/auditlog.md)
  - [Implementation Status](development/implementation_status.md)
  - [Priorities](development/priorities.md)

- **[Source Code Documentation](src/)** - Code-level docs
  - Auto-generated documentation for all modules

### 🔧 Administration

- **[Admin Tools](admin_tools/)** - GUI administration tools
  - [User Guide](admin_tools/user_guide.md)
  - [Admin Guide](admin_tools/admin_guide.md)
  - [Feature Matrix](admin_tools/feature_matrix.md)

- **[Guides](guides/)** - Operations and setup
  - [TLS Setup](guides/tls_setup.md)
  - [Vault Integration](guides/vault.md)
  - [RBAC](guides/rbac.md)
  - [Code Quality](guides/code_quality.md)

- **[Performance](performance/)** - Performance optimization
  - [Benchmarks](performance/benchmarks.md)
  - [Memory Tuning](performance/memory_tuning.md)
  - [GPU Acceleration](performance/GPU_ACCELERATION_PLAN.md)

### 📦 Integration

- **[Client SDKs](clients/)** - Language-specific clients
  - [Python SDK](clients/python_sdk_quickstart.md)
  - [JavaScript SDK](clients/javascript_sdk_quickstart.md)
  - [Rust SDK](clients/rust_sdk_quickstart.md)

- **[API Documentation](api/)** - REST API reference
- **[Plugins](plugins/)** - Plugin development
  - [Plugin Security](plugins/PLUGIN_SECURITY.md)
  - [Plugin Migration](plugins/PLUGIN_MIGRATION.md)

- **[Exporters](exporters/)** & **[Importers](importers/)** - Data migration
  - [JSONL LLM Exporter](exporters/JSONL_LLM_EXPORTER.md)
  - [PostgreSQL Importer](importers/POSTGRES_IMPORTER.md)

### 📊 Reports & Planning

- **[Reports](reports/)** - Development reports and analysis
  - [Themis Implementation Summary](reports/themis_implementation_summary.md)
  - [Database Capabilities Roadmap](reports/database_capabilities_roadmap.md)
  - [Phase Reports](reports/) - Detailed phase completion reports
  - [Documentation Status](reports/DOCUMENTATION_SUMMARY.md)

- **[Roadmap](roadmap.md)** - Development roadmap

### 📚 Reference

- **[Glossary](glossary.md)** - Terminology reference
- **[Style Guide](guides/styleguide.md)** - Documentation standards
- **[Changelog](changelog.md)** - Version history
- **[Home](home.md)** - Documentation home

## Documentation Organization

```
docs/
├── admin_tools/          # Administration tool guides
├── aql/                  # AQL query language
├── api/                  # API documentation
├── architecture/         # System architecture
├── auth/                 # Authentication docs
├── clients/              # Client SDK guides
├── content/              # Content management
├── deployment/           # Deployment guides
├── development/          # Development documentation
├── exporters/            # Data export
├── features/             # Feature documentation
├── geo/                  # Geospatial features
├── guides/               # User and admin guides
├── importers/            # Data import
├── ingestion/            # Data ingestion
├── observability/        # Monitoring and metrics
├── performance/          # Performance tuning
├── plugins/              # Plugin development
├── query/                # Query features
├── release_notes/        # Release notes
├── reports/              # Status reports
├── search/               # Search features
├── security/             # Security documentation
├── sharding/             # Sharding and scaling
├── src/                  # Source code documentation
└── storage/              # Storage layer
```

## Contributing to Documentation

When adding or updating documentation:

1. **Follow the structure** - Place docs in the appropriate subdirectory
2. **Link properly** - Use relative links to other documentation
3. **Add to README** - Update relevant README.md files
4. **Use markdown** - Follow the [Style Guide](guides/styleguide.md)
5. **Keep it current** - Update docs when features change

## Documentation Standards

- **Format**: Markdown (.md)
- **Encoding**: UTF-8
- **Line endings**: LF (Unix-style)
- **Max line length**: None (wrap for readability)
- **Code blocks**: Always specify language
- **Links**: Use relative paths

## Building Documentation

The documentation can be built using MkDocs:

```powershell
# Install dependencies
pip install -r requirements-docs.txt

# Build documentation
.\build-docs.ps1

# Preview locally
mkdocs serve
```

Documentation is automatically deployed to GitHub Pages on merge to main.

## Getting Help

- **Issues**: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Wiki**: [GitHub Wiki](https://github.com/makr-code/ThemisDB/wiki)
- **Main README**: [Project README](../README.md)

## License

This documentation is part of ThemisDB and is licensed under the same terms. See [LICENSE](../LICENSE) for details.

---

**Last Updated**: December 2025
**Version**: 1.0.0
