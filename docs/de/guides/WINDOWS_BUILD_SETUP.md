[docs](../../README.md) > [de](../README.md) > [guides](README.md)

**Datum:** 2026-11-07

**Status:** review

**Primary (Quelle der Wahrheit):** [Release-Matrix](../../../.github/workflows/release-build-matrix.yml), [Build-Pipeline](../../../.github/workflows/reusable-cmake-build.yml), [Dependency-Supervisor](../../../scripts/prepare-windows-dependencies.py), [Presets](../../../CMakePresets.json)

**Bezug / Reference:** Windows dependency timeout, Actions run 42278019978 / job 128119148701.

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI API assistant
Human_Review: false
Human_Author: ThemisDB Contributors
-->

# Windows Build Setup - Kritische Konfigurationsanforderungen

## Release-CI: Dependencies vor dem ThemisDB-Build sichern

Die Release-Matrix führt für eine aktivierte Windows-Lane zuerst den separaten
Job `windows-dependencies` aus. Er konfiguriert denselben gepinnten `source_sha`
mit demselben Preset, Manifest, Feature-Flags, Toolchain sowie Target-/Host-Triplet
`x64-windows`, kompiliert aber **nicht ThemisDB**. Bewusst wird die echte
Projektkonfiguration verwendet: keine zweite, manuell gepflegte Feature-Liste.
Deaktivierte Windows-Lanes überspringen diesen Job; Linux/macOS laufen auch bei
einem Windows-Vorbereitungsfehler weiter.

- Der Supervisor begrenzt Configure auf höchstens **270 Minuten**, zusätzlich auf
  die verbleibende Zeit bis zur **300-Minuten-Deadline ab Jobbeginn** (vor Checkout,
  Restore und Setup). Beim kontrollierten Timeout beendet er unter Windows mit
  `taskkill.exe /PID <pid> /T /F` den Prozessbaum und liefert Exit-Code **124**.
  Fehlercodes normaler Installationsfehler werden unverändert weitergegeben.
  Der Workflow übergibt den nativen Pfad seiner aktiven Git-Bash an den Supervisor;
  eine `bash`-Suche über Windows-PATH könnte stattdessen WSL ohne Distribution starten
  (Run 42396942036, Job 128476529660).
- Anschließend wird auch bei Installationsfehlern oder kontrolliertem Timeout
  ein neuer partieller Cache-Snapshot gespeichert (Upload-Budget **30 Minuten**).
  Der 360-Minuten-Job lässt damit Zeit für Beendigung und Upload; ein `always()`
  oder `!cancelled()` nach Ablauf des harten Joblimits allein würde nicht reichen.
  Manuelle Cancellation oder Runner-Ausfall können weiterhin den Upload verhindern.
- Erst erfolgreiche Vorbereitung plus Cache-Save veröffentlichen den Snapshot-Key.
  Der eigentliche Windows-Build verlangt dessen **exakten Restore** und installiert
  mit `VCPKG_INSTALL_OPTIONS=--only-binarycaching`. Fehlende Archive, ABI-Abweichungen
  oder ein fehlgeschlagener Prerequisite blockieren Configure/Compile; es gibt keinen
  unbeschränkten Source-Build-Fallback. Ein Cache-Service-Fehler wird spätestens am
  exakten Restore sichtbar. Ein späterer MSVC-Fehler verliert die vorbereiteten ZIPs
  nicht mehr.
- `windows_dependency_concurrency` ist auf **4** voreingestellt, zulässig sind
  **1–4**. `VCPKG_MAX_CONCURRENCY` steuert vcpkg unabhängig vom ThemisDB-Build:
  dieser behält `CMAKE_BUILD_PARALLEL_LEVEL=1` und explizit `--parallel 1` wegen C1060.
  Preset-Environment kann Umgebungswerte überschreiben; entscheidend sind vcpkgs
  eigener Concurrency-Wert und das explizite MSVC-Build-Argument. Run 42468930551,
  Job 128720336009, erreichte mit leerem Cache und zwei Workern das 270-Minuten-Limit
  während RocksDB (209/220); vier Worker sollen kalte Vorbereitung beschleunigen.
  Speicherverbrauch und Laufzeit müssen im nativen Windows-Lauf geprüft werden.
- Die opt-in Modi `prepare`/`consume` der wiederverwendbaren Pipeline nutzen den
  separaten Namespace `release-deps-v2`. OS, Architektur, Compiler, Triplet,
  Manifest-/Overlay-Hash, Configure-Signatur, vcpkg-Revision, MSVC-Version und Source-SHA segmentieren
  Snapshot-Keys; Run-ID/Attempt machen sie absichtlich eindeutig. Prefix-Restore
  erlaubt inkrementelle Vorbereitung aus älteren partiellen Snapshots, während
  vcpkg die Paket-ABI prüft. Cross-OS-Archive und Build-Tree-Restore sind für diese
  Modi deaktiviert; native Windows-Cache-Pfade werden mit `cygpath -m` normalisiert.
  Andere Aufrufer behalten den Standardmodus `off`.

### Dual-Konfiguration und Diagnose

Die Lane behält **Debug und Release** des unveränderten `x64-windows`-Triplets.
Die Release-Matrix unterstützt auch `build_type=Debug`, und Target-/Host-Pakete
müssen dieselbe Triplet-/ABI-Konfiguration verwenden. `VCPKG_BUILD_TYPE=release`
als Umgebungsvariable ist kein sicherer Triplet-Ersatz. Release-only würde ein
eigenes, konsistent für Target und Host eingesetztes Overlay-Triplet samt
ABI-/Linkage- und nativer Port-Validierung benötigen; das wird hier nicht ohne
diese Kompatibilitätsevidenz eingeführt.

Cache-Diagnose unterscheidet `exact-match`, `prefix-restored` und `no-cache`,
zeigt den gematchten Key und zählt nur binäre `*.zip`-Archive. `cache-hit=false`
ist bei eindeutigen Snapshot-Keys **kein Beweis für fehlenden Restore**.
Die Install-Ausgabe `Restored N package(s)` belegt tatsächliche ABI-validierte
Wiederverwendung; ohne solche Ausgabe lautet die Diagnose „not reported“, nicht
„0 Treffer“. Vorhandene `package ABI:`-Zeilen werden ebenfalls angezeigt.
**Zero sccache requests sind keine Cache-Misses**: im beobachteten Lauf begann
der ThemisDB-Build nach sechs Stunden Configure überhaupt nicht.

Lokale Regressionen:

```bash
python -m unittest discover -s tests -p 'test_windows_dependency_preparation.py' -v
python -m unittest discover -s tests -p 'test_windows_release_build_memory.py' -v
PYTHONPATH=tests python -m unittest test_deployment_workflow_regressions.DeploymentWorkflowTests -v
```

Die Tests prüfen Supervisor, Prozessbaum-Timeout auf Linux, Windows-PID-Aufruf,
Fehlerweitergabe, Save-Reihenfolge, Gating, Quell-/Feature-Parität, Cache-Diagnose
und Parallelismus. Sie ersetzen **keinen nativen Windows/MSVC/vcpkg-Lauf**.
Ein neuer CI-Run auf korrigierter Source ist zur Validierung von Windows-Tree-Kill,
partiellem Cache-Upload, ABI-Restore und Packaging erforderlich; ein Retry des
alten Runs nutzt dessen alte Source-SHA.

## Übersicht

Dieses Dokument beschreibt die **exakte** Konfiguration, die für erfolgreiche CMake-Builds unter Windows erforderlich ist. Abweichungen führen zu Exit-Code 1 Fehlern.

---

## 🎯 Kritisches Problem: Compiler-Pfade mit Leerzeichen

### Das Grundproblem

Windows-Pfade wie `C:/Program Files/...` enthalten **Leerzeichen**, die zu kritischen Build-Fehlern führen:

```
CMake Error: The CMAKE_CXX_COMPILER: C:/Program
```

Der Compiler-Pfad wird an Leerzeichen abgeschnitten → `C:/Program` statt vollständigem Pfad.

### Warum tritt das Problem auf?

1. **VS Code CMake Tools** liest Compiler-Pfade aus CMakePresets.json
2. Übergibt sie als **Kommandozeilenargumente** an cmake.bat
3. Batch-Script `%*` (alle Parameter) **verliert Anführungszeichen**
4. CMake erhält defekten Pfad: `C:/Program` statt `"C:/Program Files/..."`

---

## ✅ Die Lösung: Dreiteilige Konfiguration

### 1. cmake.bat - VsDevCmd Wrapper

**Datei:** `c:\VCC\themis\cmake.bat`

```batch
@echo off
REM CMake wrapper that initializes VS2022 environment before running CMake
REM Called by VS Code CMake Tools

setlocal EnableDelayedExpansion
set "VSCMD_START_DIR=%CD%"
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo >nul 2>&1

REM Find cmake.exe path
for /f "tokens=*" %%i in ('where cmake.exe 2^>nul') do set "CMAKE_EXE=%%i"
if not defined CMAKE_EXE set "CMAKE_EXE=cmake.exe"

REM Forward all arguments to cmake.exe - properly preserving quotes
"%CMAKE_EXE%" %*

REM Propagate exit code
exit /b %ERRORLEVEL%
```

**Warum ist das notwendig?**

- ✅ **Initialisiert VsDevCmd** → MSVC-Compiler in PATH
- ✅ **Setzt Umgebungsvariablen** → cl.exe, ninja.exe, etc. verfügbar
- ✅ **Kein expliziter Compiler-Pfad nötig** → Vermeidet Leerzeichen-Problem
- ✅ **Exit-Code wird korrekt weitergereicht** → CMake Tools erkennt Erfolg/Fehler

---

### 2. CMakePresets.json - Keine expliziten Compiler-Pfade

**Datei:** `c:\VCC\themis\CMakePresets.json`

```json
{
  "name": "windows-ninja-msvc-release",
  "displayName": "Windows Ninja MSVC Release (LLM+Tests+Benchmarks)",
  "description": "Release build with Ninja, MSVC compiler, LLM support, tests and benchmarks",
  "inherits": ["base"],
  "binaryDir": "${sourceDir}/build-msvc-ninja-release",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Release",
    "VCPKG_TARGET_TRIPLET": "x64-windows",
    "THEMIS_BUILD_BENCHMARKS": "ON"
    // ❌ KEINE CMAKE_C_COMPILER oder CMAKE_CXX_COMPILER hier!
  }
}
```

**Warum keine Compiler-Pfade?**

- ✅ **VsDevCmd hat cl.exe bereits in PATH** → CMake findet es automatisch
- ✅ **Keine Pfade mit Leerzeichen** → Kein Quoting-Problem
- ✅ **VS Code kann keine defekten Pfade überschreiben**
- ❌ **Mit expliziten Pfaden:** VS Code übergibt sie als `-DCMAKE_CXX_COMPILER="C:/Program Files/..."` → Anführungszeichen gehen verloren

---

### 3. CMakeUserPresets.json - Minimal & Vererbt

**Datei:** `c:\VCC\themis\CMakeUserPresets.json`

```json
{
  "version": 6,
  "include": ["CMakePresets.json"],
  "configurePresets": [
    {
      "name": "vscode-windows-release",
      "displayName": "VS Code: Windows MSVC Release (LoRA+Tests)",
      "description": "Uses MSVC with VsDevCmd environment",
      "hidden": false,
      "inherits": "windows-ninja-msvc-release",
      "cacheVariables": {
        "THEMIS_BUILD_TESTS": "ON",
        "THEMIS_ENABLE_LLM": "ON",
        "THEMIS_ENABLE_GPU": "OFF"
        // ❌ KEINE CMAKE_C_COMPILER, CMAKE_CXX_COMPILER, CMAKE_MAKE_PROGRAM!
      }
    }
  ]
}
```

**Warum minimal halten?**

- ✅ **Erbt Konfiguration vom Parent** (`windows-ninja-msvc-release`)
- ✅ **Überschreibt nur Feature-Flags** (Tests, LLM, GPU)
- ✅ **Keine Tool-Pfade** → VsDevCmd regelt das
- ✅ **Keine Umgebungsvariablen nötig** (VCPKG_ROOT wird von Parent geerbt)

---

### 4. VS Code Settings - Keine Compiler-Overrides

**Datei:** `c:\VCC\themis\.vscode\settings.json`

```json
{
  "C_Cpp.default.compilerPath": "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\cl.exe",
  "C_Cpp.default.cppStandard": "c++20",
  "C_Cpp.default.intelliSenseEngine": "default",
  
  "cmake.configurePreset": "vscode-windows-release",
  "cmake.buildPreset": "vscode-windows-release",
  "cmake.testPreset": "vscode-lora-tests",
  
  "cmake.cmakePath": "C:\\VCC\\themis\\cmake.bat",
  "cmake.configureArgs": [],          // ✅ Keine zusätzlichen Args
  "cmake.ignoreKitEnv": true          // ✅ Ignoriert Kit-Umgebung
}
```

**Warum diese Einstellungen?**

- ✅ `C_Cpp.default.compilerPath` → **Nur für IntelliSense**, nicht für CMake
- ✅ `cmake.cmakePath` → **Verwendet cmake.bat statt cmake.exe**
- ✅ `cmake.configureArgs: []` → **VS Code fügt KEINE eigenen Compiler-Pfade hinzu**
- ✅ `cmake.ignoreKitEnv: true` → **Ignoriert VS Code Kit-System**

**❌ Was NICHT funktioniert:**

```json
// ❌ FALSCH - VS Code würde diese als defekte CMD-Args übergeben:
"cmake.generator": "Ninja",
"cmake.buildDirectory": "...",
// Diese Werte sind bereits im Preset definiert!
```

---

## 📦 vcpkg Dependencies - rocksdb + zlib

**Datei:** `c:\VCC\themis\vcpkg.json`

```json
{
  "dependencies": [
    { "name": "openssl", "version>=": "3.6.0" },
    { "name": "zlib", "version>=": "1.3" },           // ✅ KRITISCH für rocksdb
    { "name": "rocksdb", "features": ["lz4", "zlib", "zstd"] },  // ✅ zlib Feature
    // ... weitere Dependencies
  ]
}
```

**Warum ist zlib kritisch?**

- ✅ **rocksdb braucht zlib** als Kompressionsbackend
- ❌ **Ohne explizites zlib:** CMake-Error "ZLIB not found (required >= 1.3)"
- ✅ **Mit zlib Feature:** `rocksdb[core,lz4,zlib,zstd]` linkt korrekt

---

## 🔄 Build-Workflow

### 1. Initiale Konfiguration

```powershell
# VS Code: Ctrl+Shift+P → "CMake: Configure"
# Oder manuell:
cd C:\VCC\themis
cmd.exe /c cmake.bat --preset vscode-windows-release
```

**Was passiert intern:**

1. `cmake.bat` wird aufgerufen
2. VsDevCmd initialisiert MSVC-Umgebung
3. CMake findet `cl.exe` automatisch in PATH
4. CMakePresets.json wird geladen
5. vcpkg installiert Dependencies (zlib, rocksdb, etc.)
6. CMake generiert Ninja-Build-Files

### 2. Build ausführen

```powershell
# VS Code: F7 oder Ctrl+Shift+P → "CMake: Build"
# Oder manuell:
cmake --build build-msvc-ninja-release --config Release --parallel 8
```

### 3. Tests ausführen

```powershell
# VS Code: Ctrl+Shift+P → "CMake: Run Tests"
# Oder manuell:
ctest --test-dir build-msvc-ninja-release --config Release --output-on-failure
```

---

## 🚨 Häufige Fehler & Lösungen

### Fehler 1: "CMAKE_CXX_COMPILER: C:/Program"

**Symptom:**
```
CMake Error: The CMAKE_CXX_COMPILER:
  C:/Program
is not a full path to an existing compiler tool.
```

**Ursache:** Compiler-Pfade in CMakePresets.json oder von VS Code überschrieben

**Lösung:**
1. ✅ Entferne `CMAKE_C_COMPILER` und `CMAKE_CXX_COMPILER` aus CMakePresets.json
2. ✅ Setze `cmake.configureArgs: []` in VS Code settings.json
3. ✅ Stelle sicher, dass cmake.bat verwendet wird

---

### Fehler 2: "ZLIB not found (required >= 1.3)"

**Symptom:**
```
CMake Error: Could NOT find ZLIB (missing: ZLIB_LIBRARY ZLIB_INCLUDE_DIR)
```

**Ursache:** zlib nicht in vcpkg.json oder rocksdb-Features fehlen

**Lösung:**
```json
// vcpkg.json
{
  "dependencies": [
    { "name": "zlib", "version>=": "1.3" },  // ✅ Explizit hinzufügen
    { "name": "rocksdb", "features": ["lz4", "zlib", "zstd"] }  // ✅ zlib Feature
  ]
}
```

---

### Fehler 3: Exit-Code 1 ohne Fehlermeldung

**Symptom:** CMake Tools zeigt "Exit-Code 1", aber Build sieht erfolgreich aus

**Ursache:** cmake.bat gibt Exit-Code falsch weiter

**Lösung:**
```batch
REM Am Ende von cmake.bat:
exit /b %ERRORLEVEL%  // ✅ /b ist kritisch für Batch-Exit-Code
```

---

### Fehler 4: "No CMAKE_CXX_COMPILER could be found"

**Symptom:**
```
CMake Error: No CMAKE_CXX_COMPILER could be found.
```

**Ursache:** VsDevCmd wurde nicht initialisiert

**Lösung:**
1. ✅ Prüfe, dass cmake.bat VsDevCmd aufruft
2. ✅ Teste manuell: `cmd.exe /c cmake.bat --version`
3. ✅ VS Code muss cmake.bat verwenden, nicht cmake.exe

---

## 🔍 Diagnose-Kommandos

### Test 1: cmake.bat funktioniert

```powershell
cd C:\VCC\themis
cmd.exe /c cmake.bat --version
# Erwartete Ausgabe: cmake version 4.1.x (oder höher)
```

### Test 2: Compiler in PATH nach VsDevCmd

```cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
where cl.exe
# Erwartete Ausgabe: C:\Program Files\Microsoft Visual Studio\...\cl.exe
```

### Test 3: CMake findet Compiler automatisch

```powershell
cmd.exe /c cmake.bat --preset vscode-windows-release
# Erwartete Ausgabe: "-- The CXX compiler identification is MSVC 19.44..."
```

### Test 4: vcpkg Dependencies vollständig

```powershell
C:\VCC\themis\vcpkg\vcpkg.exe list | findstr "zlib rocksdb"
# Erwartete Ausgabe:
# zlib:x64-windows      1.3.1
# rocksdb:x64-windows   9.7.2
```

---

## 📝 Zusammenfassung: Warum diese Konfiguration?

| Komponente | Kritischer Grund |
|------------|------------------|
| **cmake.bat** | Initialisiert VsDevCmd → Compiler in PATH → Keine expliziten Pfade nötig |
| **CMakePresets.json** | KEINE Compiler-Pfade → Vermeidet Leerzeichen-Problem → Überschreibung unmöglich |
| **CMakeUserPresets.json** | Minimal → Erbt vom Parent → Überschreibt nur Feature-Flags |
| **VS Code settings.json** | `cmake.configureArgs: []` → Keine zusätzlichen Compiler-Args → Keine Überschreibung |
| **vcpkg.json** | Explizites `zlib` + `rocksdb[zlib]` → rocksdb-Kompression funktioniert |

---

## ✅ Checkliste für erfolgreichen Build

- [ ] cmake.bat existiert und ruft VsDevCmd auf
- [ ] CMakePresets.json hat KEINE CMAKE_C_COMPILER/CMAKE_CXX_COMPILER
- [ ] CMakeUserPresets.json ist minimal (nur Feature-Flags)
- [ ] VS Code settings.json hat `cmake.cmakePath` auf cmake.bat
- [ ] VS Code settings.json hat `cmake.configureArgs: []`
- [ ] vcpkg.json hat `zlib` als explizite Dependency
- [ ] vcpkg.json hat `rocksdb` mit `["lz4", "zlib", "zstd"]` Features
- [ ] Exit-Code von cmake.bat ist 0: `cmd /c cmake.bat --preset vscode-windows-release; $LASTEXITCODE`

---

## 📚 Referenzen

- [CMakePresets.json Schema](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [VS Code CMake Tools](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/README.md)
- [vcpkg Manifest Mode](https://learn.microsoft.com/en-us/vcpkg/users/manifests)
- [VsDevCmd.bat Dokumentation](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell)

---

**Letzte Aktualisierung:** 2026-01-29  
**Getestet mit:**
- CMake 4.1.x
- Visual Studio 2022 Professional (17.14.20)
- vcpkg (git+https://github.com/microsoft/vcpkg)
- Ninja 1.12.1
