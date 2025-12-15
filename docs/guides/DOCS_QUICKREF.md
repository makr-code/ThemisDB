# ThemisDB Dokumentation - Quick Reference

**Stand:** 30. November 2025

## 🚀 Wichtigste Links

### Für Entwickler
| Was | Wo |
|-----|-----|
| **Quick Start** | [README.md](../README.md) |
| **Build Guide** | [BUILD_STRATEGY.md](BUILD_STRATEGY.md) |
| **Enterprise Features** | [docs/enterprise/README.md](enterprise/README.md) |
| **Development Status** | [DEVELOPMENT_AUDITLOG.md](../DEVELOPMENT_AUDITLOG.md) |
| **API Docs** | [docs/apis/openapi.md](apis/openapi.md) |

### Für Management
| Was | Wo |
|-----|-----|
| **Sachstandsbericht** | [docs/THEMIS_SACHSTANDSBERICHT_2025.md](THEMIS_SACHSTANDSBERICHT_2025.md) |
| **Projektkostenschätzung** | [docs/THEMIS_PROJECT_VALUATION.md](THEMIS_PROJECT_VALUATION.md) |
| **Roadmap** | [ROADMAP.md](ROADMAP.md) |
| **Features Übersicht** | [FEATURES.md](FEATURES.md) |

### Für Compliance
| Was | Wo |
|-----|-----|
| **Compliance Dashboard** | [docs/COMPLIANCE_DASHBOARD.md](docs/COMPLIANCE_DASHBOARD.md) |
| **Audit Checklist** | [docs/FULL_AUDIT_CHECKLIST.md](docs/FULL_AUDIT_CHECKLIST.md) |
| **Security Audit** | [docs/security/SECURITY_AUDIT_REPORT.md](docs/security/SECURITY_AUDIT_REPORT.md) |
| **DPIA** | [docs/compliance/DPIA.md](docs/compliance/DPIA.md) |

## 📋 Dokumentations-Typen

### Übersichts-Dokumente
- `README.md` - Projekt-Einstieg
- `FEATURES.md` - Feature-Liste (✅ Production | 🔧 Beta | 📋 Geplant)
- `docs/DOCUMENTATION_INDEX.md` - Vollständiger Dokumentations-Index
- `docs/enterprise/README.md` - Enterprise Features Übersicht

### Technische Dokumentation
- `BUILD_STRATEGY.md` - Build-Toolchain
- `docs/BUILD_GUIDE.md` - Build-Anleitung
- `docs/architecture.md` - System-Architektur
- `docs/query_engine_aql.md` - Query Engine

### Status & Planung
- `DEVELOPMENT_AUDITLOG.md` - Entwicklungsstand
- `ROADMAP.md` - Entwicklungs-Roadmap
- `CHANGELOG.md` - Änderungshistorie
- `TEST_REPORT.md` - Test-Ergebnisse

### Enterprise
- `docs/enterprise/README.md` - **Einstieg hier!**
- `docs/ENTERPRISE_SCALABILITY.md` - Feature-Details
- `docs/HTTP_CLIENT_POOL_COMPLETE.md` - HTTP Client
- `docs/ENTERPRISE_BUILD_GUIDE.md` - Build Guide
- `INTEGRATION_ANALYSIS.md` - Legacy Integration

## 🔗 Online-Ressourcen

### GitHub
- **Repository:** https://github.com/makr-code/ThemisDB
- **Wiki:** https://github.com/makr-code/ThemisDB/wiki
- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Discussions:** https://github.com/makr-code/ThemisDB/discussions

### CI/CD Badges
- [![CI](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/ci.yml)
- [![Code Quality](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml/badge.svg)](https://github.com/makr-code/ThemisDB/actions/workflows/code-quality.yml)

## 📖 Nach Thema

### Multi-Model Features
- **Graph:** [docs/property_graph_model.md](property_graph_model.md)
- **Geo/Spatial:** [docs/GEO_ARCHITECTURE.md](GEO_ARCHITECTURE.md)
- **Time-Series:** [docs/time_series.md](time_series.md)
- **Document:** [docs/content_pipeline.md](content_pipeline.md)
- **Vector:** [docs/vector_ops.md](vector_ops.md)

### Query Language (AQL)
- **Syntax:** [docs/aql_syntax.md](aql_syntax.md)
- **Hybrid Queries:** [docs/aql-hybrid-queries.md](aql-hybrid-queries.md)
- **EXPLAIN:** [docs/aql_explain_profile.md](aql_explain_profile.md)

### Storage & Performance
- **RocksDB Layout:** [docs/storage/rocksdb_layout.md](storage/rocksdb_layout.md)
- **MVCC:** [docs/mvcc_design.md](mvcc_design.md)
- **Benchmarks:** [docs/performance_benchmarks.md](performance_benchmarks.md)
- **Compression:** [docs/compression_strategy.md](compression_strategy.md)

### Security
- **Overview:** [docs/security/overview.md](security/overview.md)
- **Encryption:** [docs/encryption_strategy.md](encryption_strategy.md)
- **RBAC:** [docs/rbac_authorization.md](rbac_authorization.md)
- **PII Detection:** [docs/security/pii_detection.md](security/pii_detection.md)

## 🛠️ Häufige Aufgaben

### Build
```powershell
# Windows
.\build.ps1 -BuildType Release

# Enterprise Build
.\scripts\build_enterprise.cmd
```

### Tests ausführen
```powershell
# Alle Tests
build-msvc-ninja-debug\themis_tests.exe

# Enterprise Tests
build-msvc-ninja-debug\themis_tests.exe --gtest_filter="*Enterprise*"
```

### Dokumentation
```powershell
# Lokale Vorschau
.\build-docs.ps1

# Wiki synchronisieren
.\sync-wiki.ps1
```

## 📞 Support

- **Fragen:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Bugs:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **Security:** Siehe [SECURITY.md](SECURITY.md)

---

**Tipp:** Vollständigen Dokumentations-Index siehe [docs/DOCUMENTATION_INDEX.md](DOCUMENTATION_INDEX.md)
