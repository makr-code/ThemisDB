# ThemisDB v1.3.5 CMake find_package FAISS & gRPC - KURZFASSUNG

## 🎯 Problem

```
CMake Error: Could NOT find faiss (missing: faiss_DIR)
CMake Error: Could NOT find gRPC (missing: gRPC_DIR)
```

**Obwohl**: vcpkg hat beides installiert ✅  
**Aber**: CMake findet es nicht ❌

---

## 🔍 Root Cause

- vcpkg.cmake setzt `CMAKE_PREFIX_PATH` 
- Aber: PATH wird bei IDE-Builds oder nested CMake zurückgesetzt
- Result: find_package(faiss CONFIG) und find_package(gRPC CONFIG) scheitern

---

## 💡 Lösung (3 Optionen)

### **Option 1: SOFORT-FIX (Empfohlen für JETZT)**

```powershell
cd C:\VCC\themis

# Script ausführen
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true -EnableLLM $true
```

**Was passiert**: 
- Setzt `CMAKE_PREFIX_PATH` auf vcpkg install dir
- Setzt `-Dfaiss_DIR=...` explizit
- Setzt `-DgRPC_DIR=...` explizit
- Führt CMake Configure durch

### **Option 2: MANUELL (Wenn Script fehlschlägt)**

```powershell
cd C:\VCC\themis

$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"

cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="$VCPKG;$VCPKG\share" `
    -Dfaiss_DIR="$VCPKG\share\faiss" `
    -DgRPC_DIR="$VCPKG\share\grpc"
```

### **Option 3: CODE-PATCH (Permanent-Lösung)**

Patch in 4 Dateien:

1. **[CMakeLists.txt](CMakeLists.txt#L40-L60)** - CMAKE_PREFIX_PATH Fallback hinzufügen
2. **[CMakeLists.txt](CMakeLists.txt#L502-L512)** - FAISS find_package mit HINTS
3. **[CMakeLists.txt](CMakeLists.txt#L925-L940)** - gRPC find_package mit HINTS
4. **[plugins/rpc/grpc/CMakeLists.txt](plugins/rpc/grpc/CMakeLists.txt#L8-L25)** - gRPC_DIR explizit setzen

Details: Siehe [IMPLEMENTATION_CHECKLIST.md](IMPLEMENTATION_CHECKLIST.md)

---

## ✅ Verifizierung

Nach CMake Configure sollte dies in Konsole stehen:

```
-- v1.3.5: CMAKE_PREFIX_PATH set to C:\VCC\themis\vcpkg_installed\x64-windows\share;...
-- Found faiss: C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake
-- Found gRPC: C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc\gRPCConfig.cmake
-- v1.3.5: faiss found at C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss
-- v1.3.5: gRPC found at C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc
```

---

## 📊 Pfade Übersicht

| Was | Pfad |
|-----|------|
| **FAISS Config** | `vcpkg_installed\x64-windows\share\faiss\faiss-config.cmake` |
| **FAISS Library** | `vcpkg_installed\x64-windows\lib\faiss.lib` |
| **gRPC Config** | `vcpkg_installed\x64-windows\share\grpc\gRPCConfig.cmake` |
| **gRPC Library** | `vcpkg_installed\x64-windows\lib\grpc.lib` (MUSS statisch sein!) |

---

## 🛠️ Schnell-Fix (Ohne Code-Patch)

Falls Code-Patch noch nicht durchgeführt:

```powershell
# 1. Diagnose
.\scripts\fix-cmake-prefix-path.ps1 -Action diagnose

# 2. Mit Script bauen
.\scripts\fix-cmake-prefix-path.ps1 -Action build

# 3. Oder manuell mit Direktive Pfade
cmake -S . -B build-msvc `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="C:\VCC\themis\vcpkg_installed\x64-windows;C:\VCC\themis\vcpkg_installed\x64-windows\share" `
    -Dfaiss_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\faiss" `
    -DgRPC_DIR="C:\VCC\themis\vcpkg_installed\x64-windows\share\grpc" `
    -DTHEMIS_ENABLE_GPU=ON

# 4. Build
cmake --build build-msvc --config Release --target themis_server --parallel 8
```

---

## 🔗 Weiterführende Dokumentation

- **Vollständige Anleitung**: [CMAKE_FIND_PACKAGE_SOLUTIONS.md](CMAKE_FIND_PACKAGE_SOLUTIONS.md)
- **Schritt-für-Schritt**: [IMPLEMENTATION_CHECKLIST.md](IMPLEMENTATION_CHECKLIST.md)
- **GitHub Issues & Referenzen**: [GITHUB_ISSUES_REFERENCE.md](GITHUB_ISSUES_REFERENCE.md)
- **Automatisiertes Skript**: [scripts/fix-cmake-prefix-path.ps1](../../scripts/fix-cmake-prefix-path.ps1)

---

## ⚡ SOFORT-Lösung

```powershell
cd C:\VCC\themis
.\scripts\fix-cmake-prefix-path.ps1 -Action build -EnableGPU $true
```

**Fertig in 30 Sekunden!**

---

**Version**: 1.3.5  
**Datum**: 2025-12-26  
**Status**: 🟢 Ready to use
