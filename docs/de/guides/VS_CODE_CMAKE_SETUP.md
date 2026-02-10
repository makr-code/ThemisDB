# VS Code CMake Setup für ThemisDB LoRA Migration

## ✅ Was wurde konfiguriert?

1. **CMakeUserPresets.json** - VS Code optimierte CMake Presets
   - `vscode-windows-release` - Release Build für LoRA Tests
   - `vscode-windows-debug` - Debug Build mit vollem Symbol-Info

2. **.vscode/settings.json** - CMake Extension Einstellungen
   - `cmake.configurePreset` - Nutzt vscode-windows-release
   - `cmake.buildPreset` - Ninja Generator + MSVC Compiler
   - Parallele Jobs auf 4 eingestellt (schneller auf dieser Maschine)

3. **.vscode/c_cpp_properties.json** - IntelliSense Konfiguration
   - Include-Pfade für llama.cpp, MSVC Standard Library
   - C++20 Standard enabled
   - MSVC IntelliSense Mode

4. **.vscode/tasks.json** - Neue Build Tasks
   - "CMake: Configure (VS Code Preset)" - Konfiguriert CMake
   - "CMake: Build (VS Code Preset)" - Build mit Ninja

---

## 🚀 Quick Start in VS Code

### Schritt 1: Reload VS Code
```
Ctrl+Shift+P → "Developer: Reload Window"
```

### Schritt 2: CMake Configure
```
Ctrl+Shift+P → "CMake: Configure"
```
**Oder**: Nutze Task "CMake: Configure (VS Code Preset)"

### Schritt 3: Build
```
Ctrl+Shift+P → "CMake: Build"
```
**Oder**: F7 (falls Standard-Build-Task aktiv)

### Schritt 4: Tests Ausführen
```
Ctrl+Shift+P → "CMake: Run CTest"
```
**Oder**: Manuell:
```bash
cd build-msvc-ninja-release
./themis_tests.exe --gtest_filter="*LoRA*"
```

---

## 🔧 Troubleshooting

### Problem: "CMake Error: Could not find cstdint"
**Lösung**: VsDevCmd ist nicht initialisiert
- Reparatur: Reload VS Code (Ctrl+Shift+P → "Developer: Reload Window")
- Falls das nicht hilft: Öffne PowerShell und führe aus:
  ```powershell
  $vsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'
  cmd /c "`"$vsDevCmd`" -arch=x64"
  ```
  Dann VS Code neu starten

### Problem: "Ninja: command not found"
**Lösung**: Ninja ist nicht im PATH
- Check ob Ninja installiert: `ninja --version`
- Falls nicht: `winget install Ninja-build.Ninja`
- PATH neuladen: Restart VS Code

### Problem: "Could NOT find ZLIB"
**Lösung**: vcpkg Abhängigkeiten installieren
```bash
vcpkg install zlib:x64-windows
```

### Problem: IntelliSense zeigt Fehler, aber Build funktioniert
**Lösung**: C++ Extension muss CMake-Compiler kennen
- Warte auf automatische Rekonfiguration (30 Sekunden)
- Oder: Ctrl+Shift+P → "C/C++: Reset IntelliSense Database"

---

## 📊 Status Check

Überprüfe dass alles richtig konfiguriert ist:

1. **CMake Status Bar** (unten rechts)
   - ✅ Sollte `[vscode-windows-release]` anzeigen
   - ✅ Sollte `Ninja` anzeigen
   - ✅ Sollte `MSVC` anzeigen

2. **C++ Extension**
   - ✅ Sollte ein Compiler gefunden haben (cl.exe)
   - ✅ Sollte keine roten Squiggles auf `#include <cstdint>` zeigen

3. **Output Panel**
   ```
   cmake.configurePreset: vscode-windows-release
   cmake.buildPreset: vscode-windows-release
   cmake.testPreset: vscode-lora-tests
   ```

---

## 🧪 LoRA Tests ausführen

Nach erfolgreichem Build:

```bash
# Nur LoRA Tests
cd build-msvc-ninja-release
./themis_tests.exe --gtest_filter="*LoRA*"

# Nur Adapter Tests  
./themis_tests.exe --gtest_filter="*Adapter*"

# Mit verbosem Output
./themis_tests.exe --gtest_filter="*LoRA*" --gtest_print_time=1

# Mit XML Report
./themis_tests.exe --gtest_filter="*LoRA*" --gtest_output=xml:lora_tests.xml
```

---

## 📝 Dateien die erstellt/modifiziert wurden

| Datei | Status | Zweck |
|-------|--------|-------|
| `CMakeUserPresets.json` | ✅ Erstellt | VS Code optimierte CMake Presets |
| `.vscode/settings.json` | ✅ Modified | CMake Extension Einstellungen |
| `.vscode/c_cpp_properties.json` | ✅ Erstellt | IntelliSense / Include-Pfade |
| `.vscode/tasks.json` | ✅ Modified | Neue Build-Tasks hinzugefügt |

---

## 🎯 Nächste Schritte

1. ✅ VS Code reload
2. ✅ CMake configure via "CMake: Configure (VS Code Preset)"
3. ✅ Build via "CMake: Build (VS Code Preset)"  
4. ✅ Tests via "CMake: Run CTest" oder manuell
5. ✅ LoRA Tests überprüfen: `--gtest_filter="*LoRA*"`

---

## 💡 Pro Tips

- **Schnell Konfigurieren**: `Ctrl+Shift+P` → "CMake: Configure"
- **Schnell Builden**: `Ctrl+Shift+P` → "CMake: Build"
- **Test Runner**: `Ctrl+Shift+P` → "CMake: Run CTest"
- **Problem Diagnostizieren**: `Ctrl+Shift+P` → "CMake: View Logs"
- **Clear Cache**: `Ctrl+Shift+P` → "CMake: Delete Cache and Reconfigure"

---

**Setup Complete! 🎉**

Du kannst jetzt direkt in VS Code:
- ✅ CMake konfigurieren
- ✅ Mit Ninja builden  
- ✅ MSVC Compiler nutzen
- ✅ LoRA Tests ausführen
- ✅ Code mit vollem IntelliSense editieren

Viel Erfolg mit den LoRA Tests! 🚀
