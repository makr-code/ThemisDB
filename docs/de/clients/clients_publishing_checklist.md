#  ThemisDB SDK Publishing Checklist

<!-- Dokumentations-Metadaten -->
**Kategorie:**  SDK Publishing  
**Version:** v1.3.0  
**Status:**  Produktionsreif  
**Letztes Update:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Pre-Publishing Requirements](#-pre-publishing-requirements)
- [ SDK-Spezifische Checklists](#-sdk-spezifische-checklists)
- [ Post-Publishing](#-post-publishing)
- [ Best Practices](#-best-practices)
- [ Troubleshooting](#-troubleshooting)
- [ Siehe auch](#-siehe-auch)
- [ Changelog](#-changelog)

---

##  Übersicht

Vollständige Checkliste für das Publishing aller ThemisDB Client SDKs zu Package Registries (npm, PyPI, crates.io, Maven Central, NuGet).

---

##  Pre-Publishing Requirements

### 1 Version Control

- [ ] Alle Änderungen committed
- [ ] VERSION file aktualisiert
- [ ] CHANGELOG.md mit Release Notes
- [ ] Git tag erstellt (z.B. `v1.0.0`)
- [ ] Branch bereit für Merge

### 2 Testing

- [ ] Alle Unit Tests passing
- [ ] Integration Tests passing  
- [ ] SDK Beispiele funktionieren
- [ ] Cross-platform Builds verifiziert

### 3 Dokumentation

- [ ] API Dokumentation aktuell
- [ ] README.md reviewed
- [ ] Migration Guide (bei Breaking Changes)
- [ ] Code-Beispiele getestet

---

##  SDK-Spezifische Checklists

###  JavaScript/TypeScript (@themisdb/client  NPM)

**Prerequisites:**
- [ ] Node.js 18+ installiert
- [ ] NPM account mit Publishing-Rechten
- [ ] `NPM_TOKEN` environment variable

**Build & Test:**
```bash
cd clients/javascript
npm ci
npm run build
npm test
npm pack --dry-run  # Package-Inhalt prüfen
```

**Publish:**
```bash
npm publish --access public
# Oder Beta:
npm publish --tag beta --access public
```

---

###  Python (themisdb  PyPI)

**Prerequisites:**
- [ ] Python 3.9+ installiert
- [ ] PyPI account mit Publishing-Rechten
- [ ] `PYPI_TOKEN` environment variable

**Build & Test:**
```bash
cd clients/python
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -e ".[dev]"
pytest
python -m build
twine check dist/*
```

**Publish:**
```bash
twine upload dist/* -u __token__ -p $PYPI_TOKEN

# Oder zu TestPyPI:
twine upload --repository testpypi dist/*
```

---

###  Rust (themisdb  crates.io)

**Prerequisites:**
- [ ] Rust stable toolchain
- [ ] crates.io account
- [ ] `cargo login` durchgeführt

**Build & Test:**
```bash
cd clients/rust
cargo fmt -- --check
cargo clippy -- -D warnings
cargo test
cargo package --list  # Package-Inhalt prüfen
```

**Publish:**
```bash
cargo publish

# Oder Dry-Run:
cargo publish --dry-run
```

---

###  Java (themisdb  Maven Central)

**Prerequisites:**
- [ ] Maven/Gradle installiert
- [ ] Sonatype account
- [ ] GPG key für Signing

**Build & Test:**
```bash
cd clients/java
mvn clean test
mvn package
mvn verify
```

**Publish:**
```bash
mvn deploy

# Oder zu Sonatype Staging:
mvn clean deploy -P release
```

---

###  C# (ThemisDB.Client  NuGet)

**Prerequisites:**
- [ ] .NET 6.0+ SDK
- [ ] NuGet account
- [ ] `NUGET_API_KEY` environment variable

**Build & Test:**
```bash
cd clients/csharp
dotnet restore
dotnet build -c Release
dotnet test
dotnet pack -c Release
```

**Publish:**
```bash
dotnet nuget push ThemisDB.Client.*.nupkg --api-key $NUGET_API_KEY --source https://api.nuget.org/v3/index.json
```

---

##  Post-Publishing

###  Verification Steps

- [ ] Package auf Registry sichtbar
- [ ] Installation mit Package Manager getestet
- [ ] README auf Registry-Seite korrekt
- [ ] Version number korrekt angezeigt
- [ ] Download funktioniert

###  Announcements

- [ ] GitHub Release erstellt
- [ ] CHANGELOG aktualisiert
- [ ] Documentation Website aktualisiert
- [ ] Community benachrichtigt (Discord, Twitter, etc.)

---

##  Best Practices

###  DO: Semantic Versioning

```
1.0.0  Major.Minor.Patch

Major: Breaking changes
Minor: New features (backwards compatible)
Patch: Bug fixes
```

###  DO: Pre-Release Tags

```bash
# Alpha
npm publish --tag alpha
pip install themisdb==1.0.0a1

# Beta
npm publish --tag beta
pip install themisdb==1.0.0b1

# Release Candidate
npm publish --tag rc
pip install themisdb==1.0.0rc1
```

###  DO: Changelog pflegen

```markdown
## [1.0.0] - 2025-12-22

### Added
- Transaction support with ACID guarantees
- Vector search with filters

### Changed
- Updated API response format

### Fixed
- Connection timeout handling
```

---

##  Troubleshooting

###  Publishing Fehlgeschlagen

**Problem:** Authentication error

**Lösung:**
```bash
# NPM
npm login
npm whoami

# PyPI
twine upload --verbose dist/*

# Crates.io
cargo login <token>

# NuGet
dotnet nuget list source
```

###  Package bereits existiert

**Problem:** Version already published

**Lösung:**
```bash
# Version number erhöhen
# package.json, pyproject.toml, Cargo.toml, etc.

# Dann neu publishen
```

---

##  Siehe auch

- [ Publishing Guide](clients_publishing_guide.md) - Detaillierter Guide
- [ SDK Implementation](clients_sdk_implementation.md) - Development Guide
- [ SDK Analysis](clients_sdk_analysis.md) - Sprach-Prioritäten
- [ SDK Audit](clients_sdk_audit.md) - Status Overview

---

##  Changelog

### Version 1.3.0 (22.12.2025)
-  Aktualisierung auf v1.3.0 Template
-  Alle SDK Checklists erweitert
-  Post-Publishing Schritte hinzugefügt
-  Best Practices Sektion
-  Alle Links aktualisiert

### Version 1.0.0 (05.12.2025)
-  Initial Publishing Checklist
-  NPM, PyPI, crates.io Support
