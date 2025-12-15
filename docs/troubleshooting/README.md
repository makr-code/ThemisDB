# Troubleshooting Guide - ThemisDB

Schnelle Hilfe bei häufigen Build- und Runtime-Problemen.

## 🔧 Build-Probleme

### Windows MSVC: "unrecognized file format in rocksdb_wrapper.obj"

**Symptom:**
```
Auto build dll exports
unrecognized file format in 'C:/VCC/themis/build-msvc/themis_core.dir/Release/rocksdb_wrapper.obj, 0'
error MSB3073: cmake.exe -E __create_def ... wurde mit dem Code 1 beendet
```

**Lösung:**
```powershell
# Option 1: Verwende statisches Build (empfohlen)
cmake -DTHEMIS_CORE_SHARED=OFF ...

# Option 2: Nutze das Build-Script (nutzt automatisch Static)
.\scripts\build.ps1 -Target windows
```

**Details:** [docs/troubleshooting/rocksdb-windows-build-issues.md](rocksdb-windows-build-issues.md)

---

### vcpkg Cache-Probleme

**Symptom:** Packages werden nicht gefunden oder alte Versionen werden verwendet

**Lösung:**
```powershell
# Cache aktualisieren
.\scripts\update-vcpkg-cache.ps1 -Triplet x64-windows

# Für alle Plattformen
.\scripts\update-vcpkg-cache.ps1 -Triplets x64-windows,x64-linux,arm64-linux
```

---

### CMake Configuration Failed

**Symptom:** "Required package not found" oder Target-Fehler

**Lösung:**
```bash
# Clean Build
rm -rf build-msvc  # Windows
rm -rf build-wsl   # Linux

# Vollständiges Reconfigure
cmake -S . -B build-msvc \
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows \
  ...
```

---

### Compiler Out of Memory

**Symptom:** "fatal error C1060: compiler is out of heap space"

**Lösung:**
```bash
# Reduziere Parallelität
cmake --build build-msvc --config Release --parallel 1

# Oder verwende weniger Jobs
cmake --build build-msvc --config Release --parallel 2
```

---

## 🐳 Docker-Probleme

### Docker Image Pull Failed

**Symptom:** "manifest unknown" oder "image not found"

**Lösung:**
```bash
# Explizite Platform
docker pull --platform linux/amd64 themisdb/themisdb:latest

# Prüfe verfügbare Tags
docker search themisdb/themisdb
```

---

### Container startet nicht

**Symptom:** Container stoppt sofort nach Start

**Diagnose:**
```bash
# Logs überprüfen
docker logs <container-id>

# Interaktiver Debug-Modus
docker run -it --entrypoint /bin/bash themisdb/themisdb:latest

# Health Check
curl http://localhost:8080/health
```

**Häufige Ursachen:**
- Port bereits belegt (8080/18765)
- Volume-Permissions
- Unzureichender Speicher

---

### Volume Permissions

**Symptom:** "Permission denied" beim Schreiben in `/data`

**Lösung:**
```bash
# Named Volume (empfohlen)
docker run -v themis_data:/data themisdb/themisdb:latest

# Host Mount mit UID/GID
docker run -u $(id -u):$(id -g) -v ./data:/data themisdb/themisdb:latest
```

---

## ⚡ Runtime-Probleme

### Server startet nicht

**Symptom:** `themis_server` beendet sich sofort

**Diagnose:**
```bash
# Test mit Verbose Logging
./themis_server --log-level=debug

# Prüfe Config
cat config/themis.yaml
```

**Häufige Ursachen:**
- Config-Datei fehlt oder ungültig
- Port bereits belegt
- Datenbankverzeichnis nicht beschreibbar
- Fehlende Shared Libraries

---

### "Failed to open RocksDB"

**Symptom:** `Failed to open RocksDB TransactionDB: ...`

**Lösung:**
```bash
# Prüfe Verzeichnis-Permissions
ls -la data/rocksdb

# Erstelle Verzeichnis
mkdir -p data/rocksdb
chmod 755 data/rocksdb

# Clean Start
rm -rf data/rocksdb/*
./themis_server
```

---

### High Memory Usage

**Symptom:** Server verwendet > 4GB RAM

**Lösung:**
```yaml
# config/themis.yaml - Reduziere Caches
storage:
  memtable_size_mb: 128     # Standard: 256
  block_cache_size_mb: 512  # Standard: 1024
```

---

### Query Performance Issues

**Symptom:** Langsame Queries

**Diagnose:**
```bash
# Enable Query Tracing
curl http://localhost:8080/_admin/trace/enable

# Prüfe Indexes
curl http://localhost:8080/_admin/indexes

# Statistiken
curl http://localhost:8080/_admin/stats
```

**Optimierung:**
- Erstelle Indexes für häufige Queries
- Nutze Batch-Operations
- Verwende Projection (nur benötigte Felder)

---

## 🔍 Debugging Tools

### Check Dependencies

**Windows:**
```powershell
# DLLs überprüfen
.\scripts\check-dll-dependencies.ps1

# vcpkg Status
vcpkg list
```

**Linux:**
```bash
# Shared Libraries
ldd ./themis_server

# Missing Dependencies
./themis_server 2>&1 | grep "not found"
```

---

### Build Verbose Output

```bash
# CMake Verbose
cmake --build build-msvc --config Release --verbose

# MSBuild Diagnostics
cmake --build build-msvc --config Release -- /verbosity:diagnostic
```

---

### Memory Leak Detection

**Valgrind (Linux):**
```bash
valgrind --leak-check=full --show-leak-kinds=all ./themis_server
```

**AddressSanitizer:**
```bash
cmake -DTHEMIS_ENABLE_ASAN=ON ...
./themis_server
```

---

## 📚 Weitere Ressourcen

- **[RocksDB Windows Build Issues](rocksdb-windows-build-issues.md)** - Detaillierte RocksDB-Problemlösung
- **[Docker Deployment Guide](../DOCKER_DEPLOYMENT.md)** - Docker Setup & Konfiguration
- **[Build Strategy](../docs/guides/guides_build_strategy.md)** - Build-Prozess-Dokumentation
- **[Architecture Overview](../docs/ARCHITECTURE_OVERVIEW.md)** - System-Architektur
- **[Changelog](../CHANGELOG.md)** - Known Issues & Fixes

---

## 🆘 Support

Wenn die Probleme weiterbestehen:

1. **Check Known Issues**: [CHANGELOG.md](../CHANGELOG.md)
2. **GitHub Issues**: [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
3. **Documentation**: [makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
4. **Wiki**: [github.com/makr-code/ThemisDB/wiki](https://github.com/makr-code/ThemisDB/wiki)

### Informationen für Bug Reports

Bitte immer angeben:
- **Platform**: Windows / Linux / Docker / QNAP
- **Build Type**: Release / Debug / Static / Shared
- **CMake Version**: `cmake --version`
- **Compiler**: MSVC / GCC / Clang + Version
- **Error Output**: Vollständige Error-Messages
- **Config**: Relevante CMake-Flags und Config-Dateien
