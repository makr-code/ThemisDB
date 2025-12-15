# Build-System Integration Summary

## ✅ Implementiert: Automatisches Cache-Update vor jedem Build

Das neue Build-System aktualisiert automatisch den lokalen vcpkg-Cache, bevor ein Build startet. Dies stellt sicher, dass immer die neuesten Paketversionen verfügbar sind.

## 📋 Neue Skripte

### Kern-Skripte
- ✅ `scripts/update-vcpkg-cache.ps1` - Aktualisiert vcpkg-Cache (2GB+ Source Archives)
- ✅ `scripts/build.ps1` - Haupteinstiegspunkt für alle Builds
- ✅ `scripts/build-windows.ps1` - Windows-spezifischer Build (MSVC)
- ✅ `scripts/build-docker.ps1` - Docker Multi-Arch Build
- ✅ `scripts/build-linux.sh` - Linux-spezifischer Build (GCC)
- ✅ `quick-build.ps1` - Schnell-Build im Root (cache update + Windows build)

### Dokumentation
- ✅ `BUILDGUIDE.md` - Komplettes Build-System Handbuch

## 🔄 Workflow

```
Benutzer ruft auf: .\scripts\build.ps1 oder .\quick-build.ps1
                            ↓
         [Auto] vcpkg Cache Update
                    • vcpkg git pull
                    • Registry aktualisieren
                    • Source-Archive pre-fetchen
                    • Validieren
                            ↓
                    Build ausführen
                • Windows (MSVC)
                • Linux (GCC/Ninja)
                • Docker (Multi-Arch)
```

## 💡 Verwendungsbeispiele

### Standard-Build (Windows mit automatischer Cache-Aktualisierung)
```powershell
.\scripts\build.ps1
# oder schneller:
.\quick-build.ps1
```

### Docker Multi-Arch Build (mit Cache-Update)
```powershell
.\scripts\build.ps1 -Target docker -Push
```

### Nur Docker amd64 (schnelleres Testing)
```powershell
.\scripts\build-docker.ps1 -Platforms "linux/amd64"
```

### Build ohne Cache-Update (bei bekannten stabilen Versions)
```powershell
.\scripts\build.ps1 -NoCache
```

### Alle Targets (Windows + Linux + Docker)
```powershell
.\scripts\build.ps1 -Target all
```

## 📊 Cache-Strategie

### Was wird aktualisiert?
| Komponente | Größe | Action |
|-----------|-------|--------|
| `vcpkg/` Repository | ~100MB | Git pull (neueste Portfiles) |
| `vcpkg/downloads/` | ~2GB | Source Archives pre-fetchen |
| Registry Baseline | - | Auf neueste Version aktualisieren |

### Was wird in Docker kopiert?
```
✅ vcpkg/downloads/    ~2GB (Source-Archive)
❌ vcpkg/packages/     8.9GB (lokale Build-Artefakte)
❌ vcpkg/buildtrees/   2.47GB (temporäre Dirs)
```

### Offline-Betrieb
- ✅ Alle benötigten Source-Archive im Cache (~2GB)
- ✅ Docker-Build arbeitet komplett offline
- ✅ Keine GitHub-Downloads während Build

## 🔧 Technische Details

### Triplet-Support
- `x64-linux` - x86_64 Linux
- `arm64-linux` - ARM64 Linux  
- `x64-windows` - Windows x64

### Build-Triplets (Auto-Detection)
- Windows: x64-windows (MSVC)
- Linux: x64-linux (GCC)
- Docker amd64: x64-linux
- Docker arm64: arm64-linux

## 📝 Häufige Befehle

```powershell
# Cache aktualisieren
.\scripts\update-vcpkg-cache.ps1

# Windows bauen
.\scripts\build.ps1 -Target windows

# Linux bauen
.\scripts\build.ps1 -Target linux

# Docker bauen (lokal)
.\scripts\build-docker.ps1

# Docker bauen + pushen
.\scripts\build-docker.ps1 -Push

# Alles nacheinander
.\scripts\build.ps1 -Target all

# Ohne Cache-Update
.\scripts\build.ps1 -NoCache
```

## 🐛 Fehlerbehandlung

### Problem: vcpkg install schlägt fehl
```powershell
# Lösung 1: Cache neu aufbauen
.\scripts\update-vcpkg-cache.ps1 -Force

# Lösung 2: Mit älterem Cache bauen
.\scripts\build.ps1 -NoCache
```

### Problem: Docker build "Subprocess aborted"
```powershell
# Lösung: Mit lokalem vcpkg Cache arbeiten
.\scripts\build-docker.ps1 -Platforms "linux/amd64"
```

### Problem: Zu wenig Speicher
```powershell
# Lösung: Nur spezifisches Triplet updaten
.\scripts\update-vcpkg-cache.ps1 -Triplets @("x64-linux")
```

## 📈 Performance

### Zeiten (Schätzungen)
- **Cache-Update**: 5-15 Minuten (beim Umgang mit 60+ Boost-Paketen)
- **Windows-Build**: 5-10 Minuten (mit Cache)
- **Linux-Build**: 5-10 Minuten (mit Cache)
- **Docker Build (amd64)**: 20-30 Minuten
- **Docker Build (multi-arch)**: 40-60 Minuten

### Optimierungen
```powershell
# Verwende -NoCache bei häufigen Builds
.\scripts\build.ps1 -NoCache

# Docker: Nur amd64 testen, dann multi-arch
.\scripts\build-docker.ps1 -Platforms "linux/amd64"

# Paralleljobs erhöhen (in CMake/Ninja)
ninja -C build -j 8
```

## 🚀 Nächste Schritte

1. **Sofort verwenden:**
   ```powershell
   .\scripts\build.ps1
   ```

2. **In CI/CD integrieren:**
   - GitHub Actions
   - GitLab CI
   - Azure Pipelines

3. **Cache in VCS speichern (optional):**
   - `vcpkg/downloads/` in Git LFS
   - Pre-seeded Docker Images

## 📚 Weiterführende Ressourcen

- `BUILDGUIDE.md` - Detailliertes Handbuch
- `scripts/*.ps1` - Shell-Implementierungen
- `Dockerfile` - Docker Multi-Arch Setup
- `CMakeLists.txt` - Build-Konfiguration

---

**Status:** ✅ Vollständig implementiert und getestet
**Stand:** 12. Dezember 2025
**Version:** Build System v1.0
