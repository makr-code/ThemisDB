# ThemisDB Documentation - Quick Reference (English)

> **Note:** This English quick reference points to the canonical German documents under `../de/` where no English version exists yet.

**Last Updated:** April 2026

## 🚀 Key Links

### For Developers
| What | Where |
|------|-------|
| **Quick Start** | [Project README](../README.md) |
| **Build Strategy** | [Build Strategy](../guides/guides_build_strategy.md) |
| **Enterprise Features** | [Enterprise Overview](../enterprise/README.md) |
| **Development Status** | [Development Summary](../development/DEVELOPMENT_SUMMARY.md) |
| **API Docs** | [OpenAPI Overview](../apis/apis_openapi.md) |

### For Management
| What | Where |
|------|-------|
| **Status Report** | [Themis Sachstandsbericht 2025](../reports/themis_sachstandsbericht_2025.md) |
| **Project Valuation** | [Project Valuation (confidential)](../THEMIS_PROJECT_VALUATION.md) |
| **Roadmap** | [Roadmap Overview](../roadmap/roadmap_overview.md) |
| **Feature Overview** | [Feature Catalog](../features/features_overview.md) |

### For Compliance
| What | Where |
|------|-------|
| **Compliance Dashboard** | [Compliance Dashboard](../compliance/compliance_dashboard.md) |
| **Audit Checklist** | [Full Compliance Checklist](../compliance/compliance_full_checklist.md) |
| **Security Audit** | [Security Audit Report](../security/security_audit_report.md) |
| **DPIA** | [Data Protection Impact Assessment](../compliance/compliance_dpia.md) |

## 📋 Documentation Types

### Overview Documents
- `README.md` - Project entry point (German canonical)
- `FEATURES.md` - Feature list
- `docs/DOCUMENTATION_INDEX.md` - Complete documentation index
- `docs/enterprise/README.md` - Enterprise features overview

### Technical Documentation
- `BUILD_STRATEGY.md` - Build toolchain
- `docs/BUILD_GUIDE.md` - Build guide
- `docs/architecture.md` - System architecture
- `docs/query_engine_aql.md` - Query engine

### Status & Planning
- `DEVELOPMENT_AUDITLOG.md` - Development status
- `ROADMAP.md` - Development roadmap
- `CHANGELOG.md` - Change history
- `TEST_REPORT.md` - Test results

### Enterprise
- `docs/enterprise/README.md` - **Start here!**
- `docs/ENTERPRISE_SCALABILITY.md` - Feature details
- `docs/HTTP_CLIENT_POOL_COMPLETE.md` - HTTP client
- `docs/ENTERPRISE_BUILD_GUIDE.md` - Build guide
- `INTEGRATION_ANALYSIS.md` - Legacy integration

## 🔗 Online Resources

### GitHub
- **Repository:** https://github.com/makr-code/ThemisDB
- **Wiki:** https://github.com/makr-code/ThemisDB/wiki
- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Discussions:** https://github.com/makr-code/ThemisDB/discussions

### CI/CD Badges
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/01-core_themis-core-ci.yml)
- [![Security CI](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml)

## 📖 By Topic

### Multi-Model Features
- **Graph:** [Property Graph Model](../features/features_property_graph.md)
- **Geo/Spatial:** [Geo Architecture](../geo/geo_architecture.md)
- **Time-Series:** [Time-Series](../features/features_time_series.md)
- **Document:** [Content Pipeline](../architecture/architecture_content_pipeline.md)
- **Vector:** [Vector Operations](../features/features_vector_ops.md)

### Query Language (AQL)
- **Syntax:** [AQL Syntax](../aql/aql_syntax.md)
- **Hybrid Queries:** [AQL Hybrid Queries](../aql/aql_hybrid_queries.md)
- **EXPLAIN:** [AQL Explain/Profile](../aql/aql_explain_profile.md)

### Storage & Performance
- **RocksDB Layout:** [RocksDB Storage](../storage/storage_rocksdb.md)
- **MVCC:** [MVCC Design](../architecture/architecture_mvcc.md)
- **Benchmarks:** [Performance Benchmarks](../performance/performance_benchmarks.md)
- **Compression:** [Compression Strategy](../performance/performance_compression_strategy.md)

### Security
- **Overview:** [Security Overview](../security/security_overview.md)
- **Encryption:** [Encryption Strategy](../security/security_encryption_strategy.md)
- **RBAC:** [RBAC Guide](../guides/guides_rbac.md)
- **PII Detection:** [PII Detection](../security/security_pii_detection.md)

## 🛠️ Common Tasks

### Build
```powershell
# Windows
./build.ps1 -BuildType Release

# Enterprise Build
./scripts/build_enterprise.cmd
```

### Run Tests
```powershell
# All tests
build-msvc-ninja-debug/themis_tests.exe

# Enterprise tests
build-msvc-ninja-debug/themis_tests.exe --gtest_filter="*Enterprise*"
```

### Documentation
```powershell
# Local preview
./build-docs.ps1

# Sync wiki
./sync-wiki.ps1
```

## 📞 Support

- **Questions:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Bugs:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Security:** See [SECURITY.md](../SECURITY.md)

---

**Tip:** Full documentation index: [English Index](../DOCUMENTATION_INDEX.md) | [German Index (canonical)](../../de/DOCUMENTATION_INDEX.md)
