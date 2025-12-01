# ThemisDB v1.0.0 Release Notes

**Release Date:** 1. Dezember 2025

## 🎉 Erste Production-Ready Version

ThemisDB 1.0.0 ist die erste stabile Version unserer hochperformanten Multi-Model-Datenbank mit Geo-Spatial, Graph, Vektor- und Zeitreihen-Unterstützung.

## 📦 Release-Artefakte

### Docker Images (empfohlen)
```bash
# Standard Ubuntu 22.04 (GLIBC 2.38+)
docker pull themisdb/themisdb:1.0.0
docker pull themisdb/themisdb:latest

# QNAP NAS Edition (Ubuntu 20.04, GLIBC 2.31+)
docker pull themisdb/themisdb:1.0.0-qnap
docker pull themisdb/themisdb:qnap
```

### Binaries
- **themisdb-1.0.0-linux-x64.zip** (10.22 MB)
  - Ubuntu 22.04+, Debian 12+, RHEL 9+
  - GLIBC 2.38+
  - Enthält: Binary, README, LICENSE, INSTALL.txt, SHA256SUMS

- **themisdb-1.0.0-qnap-x64.zip** (9.99 MB)
  - QNAP NAS (QTS 5.0+)
  - Ubuntu 20.04+, GLIBC 2.31+
  - Optimiert für ältere Container Station Versionen

- **themis-1.0.0-windows-x64.zip**
  - Windows 10/11 (x64)
  - Visual Studio 2022 Runtime erforderlich
  - Enthält: themis_server.exe, README, LICENSE, Config

## 🚀 Hauptfeatures

### Storage & MVCC
- ✅ RocksDB-basierter Storage mit MVCC (Multi-Version Concurrency Control)
- ✅ Transaktionsunterstützung mit ACID-Garantien
- ✅ Kompression (LZ4, Zstd) mit bis zu 70% Platzeinsparung
- ✅ Verschlüsselung (AES-256-GCM) für sensible Daten

### Query Engine & AQL
- ✅ AQL (Advanced Query Language) mit SQL-ähnlicher Syntax
- ✅ Rekursive Pfadabfragen für Graph-Traversierung
- ✅ Hybrid Queries (Vektor + Fulltext + Geo + Filter)
- ✅ Query-Optimierung mit kostenbasiertem Planer
- ✅ EXPLAIN/PROFILE für Performance-Analyse

### Vektor-Suche
- ✅ HNSW-Index für schnelle Nearest-Neighbor-Suche
- ✅ SIMD-optimierte Distanzberechnungen
- ✅ Unterstützte Metriken: Euclidean, Cosine, Dot Product
- ✅ Persistenz & Warmstart für große Vektorbestände

### Geo-Spatial
- ✅ Geo-Indexierung mit R*-Tree
- ✅ Radius-, Bounding-Box- und Polygon-Queries
- ✅ Integration mit OpenStreetMap-Daten
- ✅ 3D-Koordinaten-Unterstützung

### Content Pipeline
- ✅ Multi-Modal Content-Ingestion (Text, Images, JSON, JSONL)
- ✅ Automatische Metadaten-Extraktion
- ✅ Policy-basierte Content-Validierung
- ✅ Batch-Processing mit FastAPI-Adapter

### Sicherheit
- ✅ RBAC (Role-Based Access Control)
- ✅ JWT-basierte Authentifizierung
- ✅ TLS/SSL-Verschlüsselung
- ✅ Audit-Logging für Compliance
- ✅ Spaltenverschlüsselung mit Key-Rotation

### Enterprise Features
- ✅ HTTP-Client-Pool für hohe Last
- ✅ Rate-Limiting & Load-Shedding
- ✅ Prometheus-Metriken für Monitoring
- ✅ Distributed Tracing mit OpenTelemetry

## 📊 Performance-Benchmarks

- **Vektor-Suche:** 10.000+ QPS bei 1M Vektoren (1536D)
- **Graph-Traversierung:** 50.000+ QPS für 2-Hop-Pfade
- **Geo-Queries:** 15.000+ QPS Radius-Suche
- **MVCC-Overhead:** <5% bei Read-Heavy-Workloads
- **Kompression:** 60-70% Platzeinsparung (ZSTD Level 3)

## 🛠️ Installation

### Docker (empfohlen)
```bash
docker run -d \
  -p 18765:18765 \
  -v /path/to/data:/data \
  --name themisdb \
  themisdb/themisdb:1.0.0
```

### QNAP NAS
```bash
docker run -d \
  -p 18765:18765 \
  -v /share/themisdb:/data \
  --name themisdb \
  themisdb/themisdb:1.0.0-qnap
```

### Native Binary
```bash
# Linux: Extrahiere ZIP
unzip themisdb-1.0.0-linux-x64.zip
cd themisdb-1.0.0-linux-x64

# Verifiziere Checksumme
sha256sum -c SHA256SUMS.txt

# Starte Server
chmod +x themis_server
./themis_server
```

### Windows
```powershell
# Extrahiere ZIP
Expand-Archive -Path themis-1.0.0-windows-x64.zip -DestinationPath .
cd windows_x64

# Installiere Visual C++ Redistributable falls nötig
# https://aka.ms/vs/17/release/vc_redist.x64.exe

# Starte Server
.\themis_server.exe
```

Server läuft auf `http://localhost:18765`

## 📚 Dokumentation

- Architektur: [docs/architecture.md](docs/architecture.md)
- AQL Syntax: [docs/aql_syntax.md](docs/aql_syntax.md)
- Deployment: [docs/guides/deployment.md](docs/guides/deployment.md)
- QNAP: [QNAP_QUICKSTART.md](QNAP_QUICKSTART.md)
- API: OpenAPI Spec verfügbar unter `/openapi.json`

## 🔧 Systemanforderungen

**Minimum:**
- CPU: 2 Cores
- RAM: 2 GB
- Disk: 10 GB
- OS: Linux x64 mit GLIBC 2.31+

**Empfohlen:**
- CPU: 4+ Cores
- RAM: 8 GB
- Disk: 100 GB SSD
- OS: Ubuntu 22.04+, Debian 12+

## ⚠️ Breaking Changes

Dies ist das erste Major-Release (1.0.0). Zukünftige Minor-Versionen (1.x) werden API-kompatibel sein.

## 🐛 Bekannte Einschränkungen

- **Sharding:** Noch nicht implementiert (geplant für v1.1)
- **Windows Native:** Experimenteller Support - Docker empfohlen für Production
- **Raspberry Pi ARM:** In Arbeit (verfügbar in v1.1)
- **GPU-Beschleunigung:** Experimentell (aktivierbar mit Feature-Flag)

## 📝 Changelog

Siehe [CHANGELOG.md](../CHANGELOG.md) für detaillierte Änderungen.

## 🤝 Beitragen

- GitHub Repository: https://github.com/makr-code/ThemisDB
- Issues: https://github.com/makr-code/ThemisDB/issues
- Contributing Guide: [CONTRIBUTING.md](../CONTRIBUTING.md)

## 📄 Lizenz

MIT License - siehe [LICENSE](../LICENSE)

---

**Checksummen (SHA256SUMS_ARCHIVES.txt):**

```
themisdb-1.0.0-linux-x64.zip      0C8E327415E69D8B7A723684628BB640F5F2D1BCD33BD4F5C58C86B2E3D15E02
themisdb-1.0.0-qnap-x64.zip       07ED3F3DFF849355A4553D3AE0783BAEDEFDC1DAF5F31D3FC98A69FDD1CDD094
themis-1.0.0-windows-x64.zip      46F727CB443FBAEA956F4231D74C607343B302908FEE680E2B0C3A2C2700E0ED
```

**Verifizierung:**
```bash
# Linux/macOS
sha256sum -c SHA256SUMS_ARCHIVES.txt

# Windows PowerShell
Get-FileHash themis-1.0.0-windows-x64.zip -Algorithm SHA256
```

**Docker Image Digests:**
```bash
docker inspect themisdb/themisdb:1.0.0 | jq '.[0].RepoDigests'
docker inspect themisdb/themisdb:1.0.0-qnap | jq '.[0].RepoDigests'
```
