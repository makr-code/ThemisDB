# Quick Start: CMake-Only Build System

**Alles was Sie brauchen - ohne PowerShell oder Batch-Skripte!**

## 1. Voraussetzungen prüfen

```bash
# CMake Version (>= 3.23 erforderlich)
cmake --version

# vcpkg vorhanden?
ls vcpkg/vcpkg.exe    # Windows
ls vcpkg/vcpkg        # Linux

# Falls nicht: vcpkg installieren
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.bat    # Windows
cd vcpkg && ./bootstrap-vcpkg.sh     # Linux
```

## 2. Verfügbare Build-Konfigurationen anzeigen

```bash
cmake --list-presets
```

## 3. Standard-Build (Windows)

```bash
# Konfigurieren
cmake --preset windows-release

# Bauen
cmake --build --preset windows-release

# Testen
ctest --preset windows-release
```

## 4. Standard-Build (Linux)

```bash
# Konfigurieren
cmake --preset linux-release

# Bauen
cmake --build --preset linux-release

# Testen
ctest --preset linux-release
```

## 5. Alles in einem Schritt (Workflow)

```bash
# Windows
cmake --workflow --preset windows-full-workflow

# Linux
cmake --workflow --preset linux-full-workflow
```

## 6. Docker-Image bauen

```bash
# Schritt 1: Linux-Pakete vorab kompilieren (einmalig, ~10-15 min)
cmake --preset windows-release
cmake --build build-windows-release --target build-packages-linux-release

# Schritt 2: Docker-Image bauen (~5 min statt 45!)
cmake --build build-windows-release --target docker-build-community-release

# Schritt 3: Container testen
docker run --rm themisdb:community-release themis_server --version
```

## 7. Verschiedene Editionen

```bash
# MINIMAL (keine LLM/GPU)
cmake --preset minimal
cmake --build build-minimal

# ENTERPRISE (mit GPU)
cmake --preset enterprise
cmake --build build-enterprise

# HYPERSCALER (alle Features)
cmake --preset hyperscaler
cmake --build build-hyperscaler
```

## 8. Mit vorkompilierten Paketen (schneller!)

```bash
# Einmalig: Pakete bauen
cmake --preset windows-release
cmake --build build-windows-release --target build-packages-windows-release

# Danach: Schnelle Builds
cmake --preset windows-release-prebuilt
cmake --build --preset windows-release-prebuilt
# → Überspringt vcpkg install, nutzt fertige Pakete!
```

## 9. Package-Management

```bash
# Alle Pakete bauen (Windows + Linux, Debug + Release)
cmake --build build-windows-release --target build-all-packages

# Nur Windows Release
cmake --build build-windows-release --target build-packages-windows-release

# Nur Linux Release (für Docker)
cmake --build build-windows-release --target build-packages-linux-release

# Pakete werden gespeichert in: vcpkg_packages/
```

## 10. Troubleshooting

### Preset nicht gefunden?

```bash
# CMake Version prüfen
cmake --version  # Muss >= 3.23 sein

# Presets anzeigen
cmake --list-presets
```

### vcpkg nicht gefunden?

```bash
# Option 1: Umgebungsvariable setzen
export VCPKG_ROOT=/pfad/zu/vcpkg    # Linux
set VCPKG_ROOT=C:\pfad\zu\vcpkg     # Windows

# Option 2: vcpkg ins Projektverzeichnis klonen
git clone https://github.com/microsoft/vcpkg.git
```

### WSL für Linux-Builds auf Windows?

```bash
# WSL installieren
wsl --install Ubuntu

# Build-Tools in WSL
wsl bash -c "sudo apt update && sudo apt install build-essential cmake ninja-build"
```

## Performance-Vergleich

| Methode | Zeit | Bedingung |
|---------|------|-----------|
| **Alte Methode** (PowerShell + vcpkg install in Docker) | 45 min | Jeder Docker-Build |
| **Neue Methode** (CMake + pre-built packages) | 5 min | Nach einmaligem Package-Build |
| **Zeitersparnis** | **89%** | 🚀 |

## Nächste Schritte

Für vollständige Dokumentation siehe:
- **[CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md)** - Vollständige CMake-Dokumentation
- **[CMakePresets.json](CMakePresets.json)** - Alle verfügbaren Presets
- CMake Modules in `cmake/`:
  - `VcpkgPackageSystem.cmake` - Package-Building
  - `DockerBuildSystem.cmake` - Docker-Integration
  - `VcpkgConfiguration.cmake` - vcpkg-Setup

## Vorteile

✅ **Keine Shell-Skripte mehr** - Alles mit CMake  
✅ **Plattformunabhängig** - Windows, Linux, macOS  
✅ **IDE-integriert** - VS, VSCode, CLion  
✅ **89% schneller** - Mit pre-built packages  
✅ **Standardisiert** - Nur CMake, keine Custom-Tools  

**Happy Building! 🎉**
