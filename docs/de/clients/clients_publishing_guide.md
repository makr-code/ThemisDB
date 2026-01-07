#  SDK Publishing Guide

<!-- Dokumentations-Metadaten -->
**Kategorie:** � SDK Publishing  
**Version:** v1.3.0  
**Status:** ✅ Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [ SDK Feature Matrix](#-sdk-feature-matrix)
- [ Publishing Process](#-publishing-process)
- [ Registry-Spezifische Guides](#-registry-spezifische-guides)
- [ Best Practices](#-best-practices)
- [ Troubleshooting](#-troubleshooting)
- [ Siehe auch](#-siehe-auch)
- [ Changelog](#-changelog)

---

##  Übersicht

Vollständiger Publishing Guide für alle ThemisDB Client SDKs. Alle SDKs werden im `clients/` Verzeichnis gepflegt und haben Feature-Parität.

---

##  SDK Feature Matrix

Alle SDKs unterstützen:

| Feature | Beschreibung | Status |
|---------|--------------|--------|
| **CRUD Operations** | Create, Read, Update, Delete |  Alle SDKs |
| **AQL Queries** | Query execution mit Parametern |  Alle SDKs |
| **Transactions** | ACID mit Isolation Levels |  JS, Python, Rust |
| **Graph API** | traverse, shortestPath, neighbors |  Alle SDKs |
| **Vector API** | vectorSearch, vectorUpsert, vectorDelete |  Alle SDKs |
| **Batch Operations** | Batch get/put/delete |  Alle SDKs |
| **Topology-Aware** | Automatic shard routing |  Alle SDKs |
| **TLS/mTLS** | Secure connections |  Alle SDKs |

---

##  Publishing Process

### Pre-Release Checklist

- [ ] Alle Tests passing
- [ ] Feature parity verifiziert
- [ ] Version numbers aktualisiert
- [ ] CHANGELOG aktualisiert
- [ ] README mit neuen Features
- [ ] Breaking changes dokumentiert

### Release Steps

1. **Create release branch**
2. **Update version numbers**
3. **Run full test suite**
4. **Build packages**
5. **Publish to registries**
6. **Tag release in Git**
7. **Update documentation**

---

##  Registry-Spezifische Guides

###  JavaScript/TypeScript  NPM

**Package:** `@themisdb/client`

```bash
cd clients/javascript

# Build
npm install
npm run build

# Test
npm test

# Publish
npm publish --access public

# Beta Release
npm publish --tag beta
```

**Installation:**
```bash
npm install @themisdb/client
```

---

###  Python  PyPI

**Package:** `themisdb-client`

```bash
cd clients/python

# Setup
python -m venv venv
source venv/bin/activate

# Build
pip install build twine
python -m build

# Test Upload
twine upload --repository testpypi dist/*

# Production Upload
twine upload dist/*
```

**Installation:**
```bash
pip install themisdb-client
```

---

###  Rust  crates.io

**Package:** `themisdb`

```bash
cd clients/rust

# Pre-checks
cargo fmt -- --check
cargo clippy -- -D warnings
cargo test

# Publish
cargo publish
```

**Installation:**
```toml
[dependencies]
themisdb = "1.0.0"
```

---

##  Best Practices

###  Version Consistency

Alle SDKs sollten gleiche Major Version haben:
- `@themisdb/client@1.0.0`
- `themisdb-client==1.0.0`
- `themisdb = "1.0.0"`

###  Feature Parity

Vor Release sicherstellen dass alle SDKs:
- Gleiche API haben
- Gleiche Features unterstützen  
- Kompatible Error Handling
- Ähnliche Performance

###  Documentation Sync

- README aktuell
- API docs generiert
- Examples getestet
- Migration guides vorhanden

---

##  Troubleshooting

###  Build Failures

**Problem:** Package build schlägt fehl

**Lösung:**
```bash
# Clean build artifacts
rm -rf dist/ build/ *.egg-info

# Rebuild
python -m build
npm run build
cargo clean && cargo build
```

###  Test Failures

**Problem:** Tests scheitern vor Release

**Lösung:**
```bash
# Start ThemisDB
docker-compose up -d

# Run tests
npm test
pytest
cargo test
```

---

##  Siehe auch

- [ Publishing Checklist](clients_publishing_checklist.md)
- [ SDK Implementation](clients_sdk_implementation.md)
- [ SDK Analysis](clients_sdk_analysis.md)
- [ SDK Audit](clients_sdk_audit.md)

---

##  Changelog

### Version 1.3.0 (22.12.2025)
-  Aktualisierung auf v1.3.0 Template
-  Feature Matrix erweitert
-  Registry Guides aktualisiert
-  Best Practices Sektion
-  Alle Links aktualisiert

### Version 1.0.0 (05.12.2025)
-  Initial Publishing Guide
-  NPM, PyPI, crates.io Coverage
