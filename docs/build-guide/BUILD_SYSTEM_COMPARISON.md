# Build-System: CMake-Only vs. PowerShell

## Schnellvergleich

| Feature | PowerShell-Skripte | CMake-Only |
|---------|-------------------|------------|
| Plattformen | ❌ Windows (PS), Linux (Bash) | ✅ Windows, Linux, macOS |
| Zeilen Code | ~2000 | ~1100 (45% weniger) |
| IDE-Integration | ⚠️ Begrenzt | ✅ Vollständig |
| CI/CD | ⚠️ Plattform-spezifisch | ✅ Universell |
| Wartbarkeit | ⚠️ Doppelte Logik | ✅ Single Source |
| Lernkurve | Shell + CMake | ✅ Nur CMake |

## Verfügbare Build-Systeme

Das Projekt unterstützt nun **beide** Methoden:

### Option A: CMake-Only (Empfohlen) ⭐

**Neu, standardisiert, plattformunabhängig**

```bash
# Quick Start
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release

# Package-Building
cmake --build build --target build-packages-linux-release

# Docker
cmake --build build --target docker-build-community-release
```

**Dokumentation:**
- [QUICKSTART_CMAKE.md](QUICKSTART_CMAKE.md)
- [CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md)
- [CMakePresets.json](CMakePresets.json)

### Option B: PowerShell-Skripte (Legacy)

**Funktioniert weiterhin, aber deprecated**

```powershell
# Package-Building
.\build-vcpkg-packages.ps1 -Platform linux -Configuration release

# Docker
.\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY

# All-in-One
.\build-all-platforms.ps1 -Quick
```

**Dokumentation:**
- [VCPKG_MULTI_PLATFORM_PACKAGES.md](VCPKG_MULTI_PLATFORM_PACKAGES.md)
- [QUICKSTART.ps1](QUICKSTART.ps1)

## Empfehlung

✅ **Neue Projekte:** Verwenden Sie CMake-Only  
✅ **Bestehende Workflows:** Migrieren Sie zu CMake  
✅ **CI/CD:** CMake für Plattformunabhängigkeit  
✅ **Lokale Entwicklung:** Beides funktioniert  

## Migration

Siehe [MIGRATION_POWERSHELL_TO_CMAKE.md](../migration/MIGRATION_POWERSHELL_TO_CMAKE.md) für detaillierte Anleitung.

**Schnell-Migration:**
```bash
# Alt
.\build-all-platforms.ps1 -Quick

# Neu
cmake --workflow --preset windows-full-workflow
```

## Performance

Beide Methoden nutzen dieselbe Package-Store-Strategie:
- Einmalig: Pakete bauen (~10-15 min)
- Danach: Docker-Builds in 5 min (statt 45 min)
- **89% schneller** als vcpkg install im Docker

## Support

**CMake-Only:** [CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md)  
**PowerShell:** [VCPKG_MULTI_PLATFORM_PACKAGES.md](VCPKG_MULTI_PLATFORM_PACKAGES.md)  
**Migration:** [MIGRATION_POWERSHELL_TO_CMAKE.md](../migration/MIGRATION_POWERSHELL_TO_CMAKE.md)
