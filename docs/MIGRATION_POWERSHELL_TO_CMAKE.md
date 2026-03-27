# Migration: PowerShell → CMake-Only

**Übergang von Shell-Skripten zu reinem CMake-Build-System**

## Zusammenfassung

Das Build-System wurde von PowerShell/Batch-Skripten auf ein reines CMake-basiertes System migriert. Dies eliminiert Plattform-Abhängigkeiten und standardisiert alle Build-Operationen.

## Was wurde ersetzt?

### Alte Struktur (PowerShell-basiert)

```
build-vcpkg-packages.ps1              ← 600 Zeilen PowerShell
docker-build-with-prebuilt-packages.ps1 ← 400 Zeilen PowerShell
docker-build-with-cache.ps1           ← 350 Zeilen PowerShell
build-all-platforms.ps1               ← 350 Zeilen PowerShell
QUICKSTART.ps1                        ← 250 Zeilen PowerShell
rebuild_tests.bat                     ← 50 Zeilen Batch
cmake.bat                             ← 30 Zeilen Batch

Gesamt: ~2000 Zeilen Shell-Skripte
```

### Neue Struktur (CMake-only)

```
cmake/VcpkgPackageSystem.cmake        ← 200 Zeilen CMake
cmake/VcpkgPackageBuild.cmake         ← 200 Zeilen CMake
cmake/DockerBuildSystem.cmake         ← 200 Zeilen CMake
CMakePresets.json                     ← 400 Zeilen JSON (deklarativ)

Gesamt: ~600 Zeilen CMake + 400 Zeilen JSON
Ersparnis: ~1000 Zeilen Code!
```

## Migrations-Tabelle

| Alte Methode (PowerShell) | Neue Methode (CMake) | Vorteil |
|----------------------------|----------------------|---------|
| `.\build-vcpkg-packages.ps1 -Platform linux -Configuration release` | `cmake --build build --target build-packages-linux-release` | ✅ Plattformunabhängig |
| `.\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY` | `cmake --build build --target docker-build-community-release` | ✅ IDE-integriert |
| `.\build-all-platforms.ps1 -Quick` | `cmake --workflow --preset windows-full-workflow` | ✅ Standardisiert |
| `.\QUICKSTART.ps1` | `cmake --list-presets` | ✅ Built-in CMake |
| `.\rebuild_tests.bat` | `cmake --build build --target themis_tests` | ✅ Cross-platform |
| `cmake.bat` | `cmake --preset windows-release` | ✅ Keine Wrapper nötig |

## Schritt-für-Schritt Migration

### Schritt 1: CMake aktualisieren

```bash
# CMake Version prüfen (muss >= 3.23 sein)
cmake --version

# Falls zu alt: CMake von cmake.org herunterladen
```

### Schritt 2: vcpkg vorbereiten

```bash
# vcpkg muss vorhanden sein
ls vcpkg/vcpkg.exe    # Windows
ls vcpkg/vcpkg        # Linux

# Falls nicht: vcpkg installieren
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.bat    # Windows
cd vcpkg && ./bootstrap-vcpkg.sh     # Linux
```

### Schritt 3: Alte Presets sichern (optional)

```bash
Move-Item CMakePresets.json CMakePresets.json.old
```

### Schritt 4: Neue Dateien aktivieren

Alle neuen Dateien sind bereits im Repository:
- ✅ `CMakePresets.json` (aktualisiert)
- ✅ `cmake/VcpkgPackageSystem.cmake`
- ✅ `cmake/VcpkgPackageBuild.cmake`
- ✅ `cmake/DockerBuildSystem.cmake`
- ✅ `CMakeLists.txt` (mit Integration)

### Schritt 5: Presets testen

```bash
cmake --list-presets
```

Erwartete Ausgabe:
```
Available configure presets:
  "windows-release"
  "linux-release"
  "windows-release-prebuilt"
  ...
```

### Schritt 6: Ersten Build durchführen

```bash
# Windows
cmake --preset windows-release
cmake --build --preset windows-release

# Linux
cmake --preset linux-release
cmake --build --preset linux-release
```

### Schritt 7: Package-Targets testen

```bash
# Nach erfolgreicher Konfiguration
cmake --build build-windows-release --target build-packages-linux-release
```

### Schritt 8: Docker-Targets testen (optional)

```bash
# Voraussetzung: Linux-Pakete vorhanden
cmake --build build-windows-release --target docker-build-community-release
```

## Behavior-Änderungen

| Aspekt | Alt (PowerShell) | Neu (CMake) |
|--------|------------------|-------------|
| **Execution** | Separater Prozess | Integriert in CMake |
| **Logging** | Separate Log-Dateien | CMake Build-Log |
| **Error-Handling** | Try/Catch PowerShell | CMake FATAL_ERROR |
| **Progress** | PowerShell Write-Host | CMake STATUS/WARNING |
| **Platform Detection** | PowerShell $IsWindows | CMake WIN32/UNIX |
| **WSL Invocation** | `wsl bash -c "..."` | `execute_process(COMMAND wsl ...)` |
| **Package Building** | Separate .ps1-Skripte | CMake Custom Targets |
| **Docker Building** | Separate .ps1-Skripte | CMake Custom Targets |

## Vorteile der Migration

### 1. Plattformunabhängigkeit

✅ **Vorher:** Separate PowerShell/Bash-Skripte für Windows/Linux  
✅ **Nachher:** Ein CMake-System für alle Plattformen  

### 2. IDE-Integration

✅ **Visual Studio:** Volle CMake Preset-Unterstützung  
✅ **VSCode:** CMake Tools Extension  
✅ **CLion:** Native CMake-Integration  
✅ **Command Line:** Einheitlich überall  

### 3. CI/CD-Freundlichkeit

```yaml
# GitHub Actions: Vorher
- run: .\build-vcpkg-packages.ps1 -Platform linux
  shell: powershell  # Platform-specific!

# GitHub Actions: Nachher
- run: cmake --build build --target build-packages-linux-release
  # Works on Windows, Linux, macOS!
```

### 4. Weniger Code

- **Vorher:** ~2000 Zeilen Shell-Skripte
- **Nachher:** ~700 Zeilen CMake + 400 Zeilen JSON
- **Ersparnis:** 900 Zeilen (45%)

### 5. Standardisierung

Alle Build-Operationen folgen CMake-Konventionen:
- `cmake --preset <name>` - Configure
- `cmake --build --preset <name>` - Build
- `ctest --preset <name>` - Test
- `cmake --workflow --preset <name>` - All-in-one

### 6. Debugging

CMake bietet bessere Debug-Tools:
```bash
# Trace CMake execution
cmake --preset windows-release --trace-source=VcpkgPackageSystem.cmake

# Verbose build
cmake --build build --verbose

# CMake Debug
cmake --preset windows-release --debug-output
```

## Kompatibilität

### Was bleibt unverändert?

✅ **vcpkg-Integration** - Weiterhin via CMAKE_TOOLCHAIN_FILE  
✅ **vcpkg.json Manifests** - Keine Änderungen  
✅ **Package-Store-Struktur** - `vcpkg_packages/` gleich  
✅ **Docker-Strategie** - Pre-built packages weiterhin verwendet  
✅ **Build-Ausgabe** - Identische Binaries  

### Was ist neu?

🆕 **CMake Presets** - Deklarative Build-Konfigurationen  
🆕 **Workflow Presets** - Multi-Step-Workflows  
🆕 **Custom Targets** - Package/Docker-Builds als CMake-Targets  
🆕 **Plattformunabhängigkeit** - Ein System für alles  

## Alte Skripte (Deprecated)

Die folgenden Skripte sind **deprecated** und werden **nicht mehr benötigt**:

❌ `build-vcpkg-packages.ps1` → Ersetzt durch CMake Targets  
❌ `docker-build-with-prebuilt-packages.ps1` → Ersetzt durch CMake Targets  
❌ `docker-build-with-cache.ps1` → Ersetzt durch CMake Targets  
❌ `build-all-platforms.ps1` → Ersetzt durch CMake Workflow Presets  
❌ `QUICKSTART.ps1` → Siehe `QUICKSTART_CMAKE.md`  
❌ `rebuild_tests.bat` → `cmake --build build --target themis_tests`  
❌ `cmake.bat` → `cmake --preset <name>`  

**Empfehlung:** Skripte können gelöscht oder ins `deprecated/`-Verzeichnis verschoben werden.

## Rollback (falls nötig)

Falls Probleme auftreten, können Sie zur alten Methode zurückkehren:

```bash
# Alte CMakePresets.json wiederherstellen
Move-Item CMakePresets.json.backup CMakePresets.json -Force

# Alte PowerShell-Skripte sind noch vorhanden
.\build-vcpkg-packages.ps1 -Platform linux -Configuration release
```

## Bekannte Einschränkungen

### 1. WSL-Builds auf Windows

CMake ruft WSL via `execute_process(COMMAND wsl ...)` auf. Das funktioniert, ist aber:
- Langsamer als native WSL-Ausführung
- Erfordert korrekte Pfad-Konvertierung (C:\ → /mnt/c/)

**Workaround:** Direkt in WSL arbeiten für Linux-Builds:
```bash
wsl
cmake --preset linux-release
cmake --build --preset linux-release
```

### 2. Live-Output bei langen Builds

CMake-Custom-Targets zeigen Output erst nach Completion. PowerShell-Skripte hatten Live-Output.

**Workaround:** Verwenden Sie `--verbose` für mehr Feedback:
```bash
cmake --build build --target build-packages-linux-release --verbose
```

### 3. Interactive Prompts

PowerShell-Skripte hatten `Read-Host` für Bestätigungen. CMake-Targets laufen ohne Interaktion.

**Workaround:** Überprüfen Sie Voraussetzungen vorher manuell.

## FAQ

### Q: Kann ich weiterhin PowerShell-Skripte verwenden?

**A:** Ja, die alten Skripte funktionieren weiterhin (falls vorhanden). Aber die CMake-Methode ist empfohlen.

### Q: Sind die Package-Store-Strukturen kompatibel?

**A:** Ja, `vcpkg_packages/` bleibt identisch. Alte und neue Methode können denselben Store nutzen.

### Q: Funktioniert das mit meiner IDE?

**A:** Ja! Visual Studio, VSCode (CMake Tools), CLion und andere CMake-fähige IDEs erkennen Presets automatisch.

### Q: Was ist mit CI/CD?

**A:** CMake-Methode ist CI/CD-freundlicher:
```yaml
# Universell für alle Plattformen
- run: cmake --workflow --preset windows-full-workflow
```

### Q: Muss ich meine vcpkg.json-Manifests ändern?

**A:** Nein, vcpkg-Manifests bleiben unverändert.

## Nächste Schritte

1. ✅ **Testen Sie die neuen Presets:**
   ```bash
   cmake --list-presets
   cmake --preset windows-release
   ```

2. ✅ **Bauen Sie ein Testprojekt:**
   ```bash
   cmake --build --preset windows-release
   ```

3. ✅ **Probieren Sie Package-Targets:**
   ```bash
   cmake --build build-windows-release --target build-packages-linux-release
   ```

4. ✅ **Lesen Sie die Dokumentation:**
   - [CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md)
   - [QUICKSTART_CMAKE.md](QUICKSTART_CMAKE.md)

5. ✅ **Optional: Alte Skripte archivieren:**
   ```bash
   mkdir deprecated
   Move-Item *.ps1 deprecated/
   Move-Item *.bat deprecated/
   ```

## Support

Bei Fragen oder Problemen:

1. Überprüfen Sie [CMAKE_ONLY_BUILD_SYSTEM.md](CMAKE_ONLY_BUILD_SYSTEM.md)
2. Prüfen Sie CMake-Version: `cmake --version` (>= 3.23)
3. Validieren Sie Presets: `cmake --list-presets`
4. Testen Sie mit `--verbose` und `--trace-source`

---

**Migration abgeschlossen! 🎉**

Ihr Build-System ist jetzt:
- ✅ Plattformunabhängig
- ✅ IDE-integriert
- ✅ CI/CD-freundlich
- ✅ Standardisiert
- ✅ Wartbar

**Happy Building!**
