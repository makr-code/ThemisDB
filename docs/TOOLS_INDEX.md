# ThemisDB Tools & Utilities Index

## Overview

ThemisDB includes a comprehensive suite of **30+ tools and utilities** for administration, operations, development, and data ingestion. This index provides a centralized reference to all available tools, organized by category with quick links to detailed documentation.

**For Admin Tools:** See [Admin Tools Overview](tools/admin/ADMIN_TOOLS_OVERVIEW.md) for a comprehensive guide to administrative tools including PII management, key rotation, and compliance reporting.

## Quick Reference Table

| Tool | Category | Purpose | Platform | Documentation |
|------|----------|---------|----------|---------------|
| **Data Ingestion** |
| Themis.IngestionTool | Ingestion | Enterprise data import tool | .NET 8.0 | [Guide](tools/ingestion/ingestion-tool.md) |
| ingest.py | Ingestion | Python data ingestion script | Python 3.8+ | [Guide](tools/ingestion/ingest-py.md) |
| wikipedia-ingestion | Ingestion | Wikipedia data import | Python 3.8+ | [Guide](tools/ingestion/wikipedia-ingestion.md) |
| wordpress_category_extractor.py | Ingestion | WordPress metadata extraction | Python 3.8+ | [Guide](tools/ingestion/wordpress-category-extractor.md) |
| **Operations & Monitoring** |
| shard_bench.py | Operations | Benchmark sharding performance | Python 3.8+ | [Guide](tools/operations/shard-bench.md) |
| shard_loader.py | Operations | Load test data into shards | Python 3.8+ | [Guide](tools/operations/shard-loader.md) |
| aggregate_shard_results.py | Operations | Aggregate shard metrics | Python 3.8+ | [Guide](tools/operations/aggregate-shard-results.md) |
| fault_injector.py | Operations | Test fault tolerance | Python 3.8+ | [Guide](tools/operations/fault-injector.md) |
| compare_hyperscaler.py | Operations | Compare cloud performance | Python 3.8+ | [Guide](tools/operations/compare-hyperscaler.md) |
| **Development Utilities** |
| namespace_analyzer.py | Development | Analyze C++ namespace usage | Python 3.8+ | [Guide](tools/development/namespace-analyzer.md) |
| plugin_signer | Development | **[PRIVATE]** Owner-controlled signing tool (public repo: verify-only) | Python 3.8+ | [Guide](tools/development/plugin-signer.md) |
| sign_pii_engine.py | Development | Sign PII engine components | Python 3.8+ | [Guide](tools/development/sign-pii-engine.md) |
| sign_plugin_manifest.py | Development | **[PRIVATE]** Owner-controlled manifest signer | Python 3.8+ | [Guide](tools/development/sign-plugin-manifest.md) |
| debug_graph_keys.cpp | Development | Debug graph key issues | C++17 | [Guide](tools/development/debug-graph-keys.md) |
| migrate_vector_encryption.cpp | Development | Migrate vector encryption | C++17 | [Guide](tools/development/migrate-vector-encryption.md) |
| txn_smoke.cpp | Development | Transaction smoke tests | C++17 | [Guide](tools/development/txn-smoke.md) |
| publish_wiki.py | Development | Publish docs to wiki | Python 3.8+ | [Guide](tools/development/publish-wiki.md) |
| themis_docs_builder | Development | Build documentation site | C++17 | [Guide](tools/development/themis-docs-builder.md) |
| **LoRA & AI Compliance** |
| lora-provenance | LoRA | LoRA adapter provenance, audit, and snapshot CLI | C++17 | [Guide](compliance/LORA_PROVENANCE_AUDIT.md) |
| **Administration Tools** |
| Themis.AqlQueryBuilder | Admin | Visual AQL query builder | .NET 8.0 | [Guide](tools/admin/aql-query-builder.md) |
| Themis.AuditLogViewer | Admin | Browse and filter audit logs | .NET 8.0 | [Guide](tools/admin/audit-log-viewer.md) |
| Themis.PIIManager | Admin | Manage PII data | .NET 8.0 | [Guide](tools/admin/pii-manager.md) |
| Themis.KeyRotationDashboard | Admin | Manage encryption keys (🚧 In Dev) | .NET 8.0 | [Guide](tools/admin/key-rotation-dashboard.md) |
| Themis.ClassificationDashboard | Admin | Data classification management | .NET 8.0 | [Guide](tools/admin/classification-dashboard.md) |
| Themis.ComplianceReports | Admin | Generate compliance reports (🚧 In Dev) | .NET 8.0 | [Guide](tools/admin/compliance-reports.md) |
| Themis.RetentionManager | Admin | Configure retention policies | .NET 8.0 | [Guide](tools/admin/retention-manager.md) |
| Themis.GISViewer.ControlPanel | Admin | Geospatial data visualization | .NET 8.0 | [Guide](tools/admin/gis-viewer-control-panel.md) |
| Themis.ImpactAnalysisViewer | Admin | Analyze query impact | .NET 8.0 | [Guide](tools/admin/impact-analysis-viewer.md) |
| Themis.SAGAVerifier | Admin | Verify distributed transactions | .NET 8.0 | [Guide](tools/admin/saga-verifier.md) |
| Themis.AdminTools.Shared | Admin | Shared library for admin tools | .NET 8.0 | [Guide](tools/admin/admin-tools-shared.md) |
| **Cost Analysis** |
| tco-calculator | Analysis | Total cost of ownership calculator | Web/WordPress | [Guide](tools/analysis/tco-calculator.md) |

## By Category

### 🔧 Administration Tools

Desktop applications for database administration tasks.

**Requirements:** .NET 8.0+, Windows/Linux/macOS

**📖 [Complete Admin Tools Overview](tools/admin/ADMIN_TOOLS_OVERVIEW.md)** - Comprehensive guide including PII, key rotation, and compliance features

1. **[Themis.AqlQueryBuilder](tools/admin/aql-query-builder.md)** - Visual query builder for AQL (Advanced Query Language)
2. **[Themis.AuditLogViewer](tools/admin/audit-log-viewer.md)** - Browse, filter, and export audit logs
3. **[Themis.PIIManager](tools/admin/pii-manager.md)** - Detect and manage personally identifiable information (✅ Available)
4. **[Themis.KeyRotationDashboard](tools/admin/key-rotation-dashboard.md)** - Manage and rotate encryption keys (🚧 Under Development)
5. **[Themis.ClassificationDashboard](tools/admin/classification-dashboard.md)** - Configure data classification rules
6. **[Themis.ComplianceReports](tools/admin/compliance-reports.md)** - Generate GDPR, HIPAA, PCI-DSS, and other compliance reports (🚧 Under Development)
7. **[Themis.RetentionManager](tools/admin/retention-manager.md)** - Configure and monitor data retention policies
8. **[Themis.GISViewer.ControlPanel](tools/admin/gis-viewer-control-panel.md)** - Visualize and query geospatial data
9. **[Themis.ImpactAnalysisViewer](tools/admin/impact-analysis-viewer.md)** - Analyze query performance and resource impact
10. **[Themis.SAGAVerifier](tools/admin/saga-verifier.md)** - Verify and debug SAGA distributed transactions
11. **[Themis.AdminTools.Shared](tools/admin/admin-tools-shared.md)** - Shared library with API clients and models

### 🔄 Operations & Monitoring

Scripts for operational tasks, monitoring, and performance testing.

**Requirements:** Python 3.8+

1. **[shard_bench.py](tools/operations/shard-bench.md)** - Run benchmark workloads (A-E) across sharded clusters
2. **[shard_loader.py](tools/operations/shard-loader.md)** - Populate shards with configurable test datasets
3. **[aggregate_shard_results.py](tools/operations/aggregate-shard-results.md)** - Combine and analyze sharding benchmark results
4. **[fault_injector.py](tools/operations/fault-injector.md)** - Simulate failures and measure resilience
5. **[compare_hyperscaler.py](tools/operations/compare-hyperscaler.md)** - Compare performance/cost against Aurora, Spanner, Cosmos

### 📦 Data Ingestion

Tools for importing data from various sources.

**Requirements:** Varies by tool

1. **[Themis.IngestionTool](tools/ingestion/ingestion-tool.md)** - Primary enterprise data ingestion tool (.NET)
2. **[ingest.py](tools/ingestion/ingest-py.md)** - Lightweight Python data ingestion script
3. **[wikipedia-ingestion](tools/ingestion/wikipedia-ingestion.md)** - Import Wikipedia dumps into ThemisDB
4. **[wordpress_category_extractor.py](tools/ingestion/wordpress-category-extractor.md)** - Extract categories/tags from documentation

### 💻 Development Utilities

Tools for developers working on or with ThemisDB.

**Requirements:** Varies by tool (Python 3.8+, C++17, or .NET 8.0)

1. **[namespace_analyzer.py](tools/development/namespace-analyzer.md)** - Analyze C++ codebase structure and namespaces
2. **[plugin_signer](tools/development/plugin-signer.md)** - **[PRIVATE]** owner-controlled plugin signing
3. **[sign_pii_engine.py](tools/development/sign-pii-engine.md)** - Sign PII detection engine configurations
4. **[sign_plugin_manifest.py](tools/development/sign-plugin-manifest.md)** - **[PRIVATE]** owner-controlled manifest signing
5. **[debug_graph_keys.cpp](tools/development/debug-graph-keys.md)** - Debug graph database key issues
6. **[migrate_vector_encryption.cpp](tools/development/migrate-vector-encryption.md)** - Migrate vectors to encrypted format
7. **[txn_smoke.cpp](tools/development/txn-smoke.md)** - Run transaction smoke tests
8. **[publish_wiki.py](tools/development/publish-wiki.md)** - Publish documentation to wiki
9. **[themis_docs_builder](tools/development/themis-docs-builder.md)** - Build documentation with RocksDB backend

### 💰 Cost Analysis

Tools for analyzing deployment costs and TCO.

1. **[tco-calculator](tools/analysis/tco-calculator.md)** - Calculate total cost of ownership for ThemisDB deployments

## Tool Selection Guide

### For Database Administrators

- **Daily Operations:** AuditLogViewer, PIIManager, KeyRotationDashboard
- **Compliance:** ComplianceReports, RetentionManager, ClassificationDashboard
- **Query Building:** AqlQueryBuilder
- **Troubleshooting:** SAGAVerifier, ImpactAnalysisViewer

### For Operations Teams

- **Performance Testing:** shard_bench.py, shard_loader.py
- **Resilience Testing:** fault_injector.py
- **Monitoring:** aggregate_shard_results.py
- **Cost Analysis:** compare_hyperscaler.py, tco-calculator

### For Developers

- **Code Analysis:** namespace_analyzer.py
- **Plugin Development:** plugin signature verification (signing is private-owner-only)
- **Security:** sign_pii_engine.py, migrate_vector_encryption.cpp
- **Testing:** txn_smoke.cpp, debug_graph_keys.cpp
- **Documentation:** publish_wiki.py, themis_docs_builder

### For Data Engineers

- **Data Import:** Themis.IngestionTool, ingest.py
- **Specialized Ingestion:** wikipedia-ingestion, wordpress_category_extractor.py

## Installation & Requirements

### .NET Tools (Admin Applications)

All Themis.* tools require:
- .NET 8.0 SDK or later
- Windows, Linux, or macOS
- Access to running themis_server (default: http://localhost:8080)

Install all .NET tools:
```bash
cd tools
dotnet restore
dotnet build
```

### Python Tools (Scripts)

Python tools require:
- Python 3.8 or later
- Tool-specific dependencies (see individual documentation)

Install common dependencies:
```bash
pip install pyyaml tqdm requests
```

### C++ Utilities

C++ tools require:
- C++17 compatible compiler
- CMake 3.20+
- ThemisDB development headers

Build C++ utilities:
```bash
cd tools
# See individual tool documentation for build instructions
```

## Common Workflows

### Setting Up a New Environment

1. **Install Tools:** Build .NET and C++ tools, install Python dependencies
2. **Configure Connection:** Update API endpoints in admin tools
3. **Verify Access:** Test connection with AuditLogViewer or AqlQueryBuilder
4. **Import Initial Data:** Use Themis.IngestionTool or ingest.py

### Performance Testing

1. **Prepare Test Data:** Use shard_loader.py to populate shards
2. **Run Benchmarks:** Execute shard_bench.py with various workload mixes
3. **Analyze Results:** Use aggregate_shard_results.py for summary statistics
4. **Test Resilience:** Run fault_injector.py to verify fault tolerance
5. **Compare Costs:** Use compare_hyperscaler.py and tco-calculator

### Security & Compliance

1. **Scan for PII:** Use PIIManager to detect sensitive data
2. **Configure Classifications:** Set up rules in ClassificationDashboard
3. **Set Retention Policies:** Configure in RetentionManager
4. **Rotate Keys:** Manage with KeyRotationDashboard
5. **Generate Reports:** Export from ComplianceReports
6. **Audit Activity:** Review logs in AuditLogViewer

### Plugin Development

1. **Develop Plugin:** Create hardware acceleration plugin
2. **Sign Plugin:** Use private owner-controlled signing pipeline
3. **Sign Manifest:** Use private owner-controlled manifest signing pipeline
4. **Test:** Load plugin and verify signature validation
5. **Distribute:** Share signed plugin with users

## Documentation Template

When adding new tools, follow this template structure:

```markdown
# [Tool Name]

## Overview
Brief description (2-3 sentences)

## Use Cases
- Use case 1
- Use case 2
- Use case 3

## Requirements
- Requirement 1
- Requirement 2

## Installation
\```bash
# Installation steps
\```

## Basic Usage
\```bash
# Basic usage example
\```

## Configuration
Configuration options and examples

## Advanced Usage
\```bash
# Advanced examples
\```

## Troubleshooting
Common issues and solutions

## See Also
- Related tool or documentation
```

## Contributing New Tools

1. Place tool source code in appropriate subdirectory under `tools/`
2. Create documentation following the template above
3. Add entry to this index with quick reference
4. Update `tools/README.md` with summary
5. Ensure tool follows security best practices
6. Add tests where applicable

## See Also

- [Main Documentation Index](DOCUMENTATION_INDEX.md)
- [Tools README](../tools/README.md) - Summary of tools in the repository
- [Contributing Guide](../CONTRIBUTING.md) - Development workflows
- [Security Documentation](security/) - Security-related tool usage

## Maintenance

This index is maintained by the ThemisDB development team. For corrections or additions, please:
1. Submit an issue describing the missing/incorrect information
2. Or create a pull request with the documentation changes

**Last Updated:** 2026-04-06
